#ifndef NO_MULTITHREADING
bool preview_thread_running[PREVIEW_TRACKS];
pthread_t preview_thread_ids[PREVIEW_TRACKS];
bool thread_running[TRACK_MAX];
pthread_t thread_ids[TRACK_MAX]; /* index 0 is the preview track */
#endif

void setBpm(uint16_t *spr, uint8_t newbpm)
{ *spr = samplerate * (60.f / newbpm) / p->s->rowhighlight; }

/* freewheel to fill up the ramp buffer */
void ramp(uint32_t fptr, uint16_t *spr, uint32_t sprp, float rowprogress, Track *cv, uint8_t realinstrument)
{
	/* clear the rampbuffer properly so cruft isn't played in edge cases */
	memset(cv->rampbuffer, 0, sizeof(float) * rampmax * 2);

	float note = cv->r.note;
	macroCallbackVolatile(fptr, 1, spr, sprp, cv, &note);

	if (realinstrument < p->s->inst->c) /* TODO: should use instSafe */
	{
		short l = 0, r = 0; /* initialized to 0 in case the InstAPI is invalid */
		Inst *iv = &p->s->inst->v[realinstrument];
		const InstAPI *api;
		char inststatebak[instGetPlaybackStateSize()];
		memcpy(inststatebak, cv->inststate, instGetPlaybackStateSize());

		if (cv->reverse)
		{
			uint32_t localrampmax;
			if (cv->pointer < rampmax)
				localrampmax = rampmax - cv->pointer;
			else localrampmax = rampmax;

			for (uint16_t i = 0; i < localrampmax; i++)
			{
				if ((api = instGetAPI(iv->type)))
					api->process(iv, cv, rowprogress, cv->pointer+i, note, &l, &r);
				cv->rampbuffer[i*2 + 0] = (float)l*DIVSHRT;
				cv->rampbuffer[i*2 + 1] = (float)r*DIVSHRT;
			}
		} else
			for (uint16_t i = 0; i < rampmax; i++)
			{
				if ((api = instGetAPI(iv->type)))
					api->process(iv, cv, rowprogress, cv->pointer+i, note, &l, &r);
				cv->rampbuffer[i*2 + 0] = (float)l*DIVSHRT;
				cv->rampbuffer[i*2 + 1] = (float)r*DIVSHRT;
			}
		memcpy(cv->inststate, inststatebak, instGetPlaybackStateSize());
	} cv->rampindex = 0;
}

void midiNoteOff(uint32_t fptr, uint8_t midichannel, float note, uint8_t velocity)
{
	if (note != NOTE_VOID)
	{
		unsigned char event[3] = {0b10000000 | midichannel, (int)note, velocity};
		audio_api.writeMidi(fptr, event, 3);
	}
}
void midiNoteOn(uint32_t fptr, uint8_t midichannel, float note, uint8_t velocity)
{
	if (note != NOTE_VOID)
	{
		unsigned char event[3] = {0b10010000 | midichannel, (int)note, velocity};
		audio_api.writeMidi(fptr, event, 3);
	}
}

void midiPC(uint32_t fptr, uint8_t midichannel, uint8_t program)
{
	unsigned char event[2] = {0b11000000 | midichannel, program};
	audio_api.writeMidi(fptr, event, 2);
}
void midiCC(uint32_t fptr, uint8_t midichannel, uint8_t controller, uint8_t value)
{
	unsigned char event[3] = {0b10110000 | midichannel, controller, value};
	audio_api.writeMidi(fptr, event, 3);
}

void midiMute(Inst *iv, Track *cv)
{
	if (iv->type == INST_TYPE_MIDI) /* TODO: should be in "instrument/midi.c" */
	{
		if (((InstMidiState*)iv->state)->channel != -1)
		{
			MacroState *gainstate = cv->macrostate[MACRO_GAIN];
			midiNoteOff(0, ((InstMidiState*)iv->state)->channel, cv->r.note, (gainstate->rand>>4)<<3);
			cv->r.note = NOTE_VOID;
		}
	}
}

bool triggerMidi(uint32_t fptr, InstMidiState *ims, Track *cv, float oldnote, float note, uint8_t inst)
{
	if (note != NOTE_VOID && !cv->mute)
	{
		if (ims->channel != -1)
		{
			/* always stop the prev. note */
			MacroState *gainstate = cv->macrostate[MACRO_GAIN];
			midiNoteOff(fptr, ims->channel, oldnote, (gainstate->rand>>4)<<3);
			midiNoteOn (fptr, ims->channel, note,    (gainstate->rand>>4)<<3);
			return 1;
		}
	} return 0;
}

/* TODO: should maybe take oldnote and a row as args */
void triggerNote(uint32_t fptr, Track *cv, float oldnote, float note, short inst)
{
	Inst *iv;
	const InstAPI *api;

	switch ((int)note)
	{
		case NOTE_VOID:
		case NOTE_OFF:
			cv->release = 1;
			break;
		case NOTE_CUT:
			/* always ramp when triggering anyway */
			cv->release = 1;
			cv->r.note = NOTE_VOID;
			break;
		default:
			if (note < NOTE_MAX) /* not legato */
				cv->pointer = 0; /* TODO: should affect pitchedpointer */

			cv->r.note = fmodf(note + cv->transpose, NOTE_MAX);
			cv->reverse = 0;
			cv->release = 0;

			if (instSafe(p->s->inst, inst))
			{
				cv->r.inst = inst;
				iv = &p->s->inst->v[p->s->inst->i[inst]];
				iv->triggerflash = cv->triggerflash = samplerate / buffersize * INSTRUMENT_TRIGGER_FLASH_S;
				p->redraw = 1;
				cv->file = 0;
			} else if (inst < 0)
			{
				cv->r.inst = INST_VOID;
				cv->file = 1;
			}

			macroCallbackTriggerNote(fptr, cv, oldnote, cv->r.note, inst);

			break;
	}

	if (instSafe(p->s->inst, inst))
	{ /* TODO: don't really like that this is checked twice */
		iv = &p->s->inst->v[p->s->inst->i[inst]];
		if ((api = instGetAPI(iv->type)))
			api->triggernote(fptr, iv, cv, oldnote, cv->r.note, inst);
	}
}

void processRow(uint32_t fptr, uint16_t *spr, bool midi, Track *cv, Row *r)
{
	if (!r) return;

	bool triggerramp = 0;
	Track oldcv;
	memcpy(&oldcv, cv, sizeof(Track));

	for (int i = 0; i <= cv->pattern->macroc; i++)
		cv->r.macro[i] = r->macro[i];

	/* try to persist old state a little bit */
	if      (r->note != NOTE_VOID && r->inst == INST_VOID) r->inst = cv->r.inst;
	else if (r->note == NOTE_VOID && r->inst != INST_VOID) r->note = cv->r.note;

	macroCallbackPreTrig(fptr, spr, cv, r);

	if (r->note != NOTE_VOID)
	{
		triggerNote(fptr, cv, cv->r.note, r->note, r->inst);
		triggerramp = 1;
	}

	macroCallbackPostTrig(fptr, spr, cv, r);

	if (cv->pointer && ifMacroRamp(cv, r))
		triggerramp = 1;

	/* ramp based on an old state */
	if (triggerramp && instSafe(p->s->inst, cv->r.inst))
		ramp(fptr, spr, 0, 0.0f, &oldcv, p->s->inst->i[cv->r.inst]);
}

void postSampler(uint32_t fptr, Track *cv, float rp, float lf, float rf)
{
	macroCallbackPostSampler(fptr, cv, rp, &lf, &rf);

	/* main track clipping */
	lf = hardclip(lf);
	rf = hardclip(rf);

	/* gain multipliers */
	float lm, rm;
	macroStateGetStereo(cv->macrostate[MACRO_GAIN], rp, &lm, &rm);

	cv->effect->input[0][fptr] = lf*lm;
	cv->effect->input[1][fptr] = rf*rm;
}

void playTrackLookback(uint32_t fptr, uint16_t *spr, Track *cv)
{
	uint8_t newnote = macroCallbackSampleRow(fptr, *spr, spr, 0, cv);
	if (newnote != NOTE_VOID)
	{
		triggerNote(fptr, cv, cv->r.note, newnote, cv->r.inst);
		macroCallbackPostTrig(fptr, spr, cv, &cv->r);
	}

	macroCallbackPersistent(fptr, *spr, spr, 0, cv);

	/* process the sampler */
	if (instSafe(p->s->inst, cv->r.inst)
			&& cv->r.note != NOTE_VOID && cv->r.inst != INST_VOID)
	{
		Inst *iv = &p->s->inst->v[p->s->inst->i[cv->r.inst]];
		const InstAPI *api;
		if ((api = instGetAPI(iv->type)))
			api->lookback(iv, cv, spr);

		if (cv->reverse)
		{
			if (cv->pointer > *spr) cv->pointer -= *spr;
			else                    cv->pointer = 0;
		} else cv->pointer += *spr;
	}
}

void playTrack(uint32_t fptr, uint16_t *spr, uint32_t sprp, Track *cv)
{
	float rowprogress = (float)sprp / (float)(*spr);
	float newnote = macroCallbackSampleRow(fptr, 1, spr, sprp, cv);
	if (newnote != NOTE_VOID)
	{
		if (instSafe(p->s->inst, cv->r.inst))
			ramp(fptr, spr, sprp, rowprogress, cv, p->s->inst->i[cv->r.inst]);
		triggerNote(fptr, cv, cv->r.note, newnote, cv->r.inst);

		macroCallbackPostTrig(fptr, spr, cv, &cv->r);
	}

	macroCallbackPersistent(fptr, 1, spr, sprp, cv);

	float note = cv->r.note;
	macroCallbackVolatile(fptr, 1, spr, sprp, cv, &note);

	short li = 0;
	short ri = 0;

	if (cv->file)
	{
		if (!cv->mute && p->w->previewsample && cv->r.note != NOTE_VOID)
		{
			processMinimal(p->w->previewsample, cv->pointer, 0xff, 0xf, cv->r.note, &li, &ri);
			cv->pointer++;
			cv->effect->input[0][fptr] = li*DIVSHRT;
			cv->effect->input[1][fptr] = ri*DIVSHRT;
		} else
		{
			cv->effect->input[0][fptr] = 0.0f;
			cv->effect->input[1][fptr] = 0.0f;
		}
	} else
	{
		if (instSafe(p->s->inst, cv->r.inst))
		{
			/* process the sampler */
			if (cv->r.inst != INST_VOID && cv->r.note != NOTE_VOID)
			{
				Inst *iv = &p->s->inst->v[p->s->inst->i[cv->r.inst]];
				const InstAPI *api;
				if ((api = instGetAPI(iv->type)))
					api->process(iv, cv, rowprogress, cv->pointer, note, &li, &ri);

				if (cv->reverse) { if (cv->pointer) cv->pointer--; }
				else                                cv->pointer++;
			}
		}

		float lf = li*DIVSHRT;
		float rf = ri*DIVSHRT;

		if (cv->rampindex < rampmax)
		{ // ramping
			if (!cv->mute)
			{
				float gain = (float)cv->rampindex / (float)rampmax;

				if (instSafe(p->s->inst, cv->r.inst))
				{
					lf *= gain;
					rf *= gain;
				}

				lf = lf + cv->rampbuffer[cv->rampindex*2 + 0] * (1.0f - gain);
				rf = rf + cv->rampbuffer[cv->rampindex*2 + 1] * (1.0f - gain);

				postSampler(fptr, cv, rowprogress, lf, rf);
			} else cv->effect->input[0][fptr] = cv->effect->input[1][fptr] = 0.0f;

			cv->rampindex++;
		} else if (!cv->mute && instSafe(p->s->inst, cv->r.inst))
		{
			postSampler(fptr, cv, rowprogress, lf, rf);
		} else cv->effect->input[0][fptr] = cv->effect->input[1][fptr] = 0.0f;
	}
}

void lookback(uint32_t fptr, uint16_t *spr, uint16_t playfy, Track *cv)
{
	clearTrackRuntime(cv);

	/* for every row before the current one */
	Row *r;
	for (uint16_t i = 0; i < playfy; i++)
	{
		r = getTrackRow(cv->pattern, i, 0);
		/* TODO: proper implementation of master track macros, including bpm */
		// if (p->w->bpmcachelen > i && p->w->bpmcache[i] != -1) macroBpm(fptr, spr, p->w->bpmcache[i], cv, r);
		if (r)
			processRow(fptr, spr, 0, cv, r);
		playTrackLookback(fptr, spr, cv);
	}
}

static void _trackThreadRoutine(Track *cv, uint16_t *spr, uint16_t *sprp, uint16_t *playfy, bool readrows)
{
	Row *r;
	for (uint32_t fptr = 0; fptr < buffersize; fptr++) /* TODO: probably shouldn't rely on buffersize here */
	{
		playTrack(fptr, spr, *sprp, cv);

		/* next row */
		if ((*sprp)++ > *spr)
		{
			*sprp = 0;
			if (readrows && p->w->playing)
			{
				/* walk the pointer and loop */
				(*playfy)++;
				/* TODO: looping :3 */
				if (*playfy >= (p->s->slen+1) * (p->s->plen+1))
				{
					*playfy = 0;
					lookback(fptr, spr, *playfy, cv);
					if (p->w->instrecv == INST_REC_LOCK_CUE_CONT) p->w->instrecv = INST_REC_LOCK_END;
					if (p->w->instrecv == INST_REC_LOCK_CUE_START) p->w->instrecv = INST_REC_LOCK_CUE_CONT;
				}

				/* preprocess track */
				r = getTrackRow(cv->pattern, *playfy, 0);
				/* TODO: proper implementation of master track macros, including bpm */
				// if (p->w->bpmcachelen > *playfy && p->w->bpmcache[*playfy] != -1) macroBpm(fptr, spr, p->w->bpmcache[*playfy], cv, r);
				if (r)
					processRow(fptr, spr, 1, cv, r);
			}
		}
	}

	/* track insert effects */
	if (cv->effect) /* previewtrack doesn't have any effects so this check is required */
		runEffectChain(buffersize, cv->effect);
}

static void *trackThreadRoutine(void *arg) /* wrapper for some temp variables */
{
	uint16_t spr = p->w->spr;
	uint16_t sprp = p->w->sprp;
	uint16_t playfy = p->w->playfy;
	_trackThreadRoutine(arg, &spr, &sprp, &playfy, 1);
	return NULL;
}
static void *previewTrackThreadRoutine(void *arg) /* don't try to read rows that don't exist */
{
	uint16_t spr = p->w->spr;
	uint16_t sprp = p->w->sprp;
	uint16_t playfy = p->w->playfy;
	_trackThreadRoutine(arg, &spr, &sprp, &playfy, 0);
	return NULL;
}

static void triggerFlash(PlaybackInfo *p)
{
	/* triggerflash animations */
	Inst *iv;
	for (int i = 0; i < p->s->inst->c; i++)
	{
		iv = &p->s->inst->v[i];
		if (iv->triggerflash)
		{
			if (iv->triggerflash == 1) p->redraw = 1;
			iv->triggerflash--;
		}
	}

	Track *cv;
	for (int i = 0; i < p->s->track->c; i++)
	{
		cv = p->s->track->v[i];
		if (cv->triggerflash)
		{
			if (cv->triggerflash == 1) p->redraw = 1;
			cv->triggerflash--;
		}
	}
}

void joinProcessThreads(void)
{
#ifndef NO_MULTITHREADING
	if (!RUNNING_ON_VALGRIND)
	{
		/* join with the track threads */
		for (uint8_t i = 1; i < p->s->track->c; i++)
		{
			if (thread_running[i])
				pthread_join(thread_ids[i], NULL);
			thread_running[i] = 0;
		}

		/* join with the prevew threads */
		for (uint8_t i = 0; i < PREVIEW_TRACKS; i++)
		{
			if (preview_thread_running[i])
				pthread_join(preview_thread_ids[i], NULL);
			preview_thread_running[i] = 0;
		}
	}
#endif
}

void processOutput(uint32_t nfptr)
{
	Track *cv;

	if (processM_SEM())
	{
		for (uint32_t fptr = 0; fptr < nfptr; fptr++)
			audio_api.writeAudio(fptr, 0.0f, 0.0f);
		return;
	}

	/* TODO: should be events */
	/* will no longer write to the record buffer */
	if (p->w->instrecv == INST_REC_LOCK_PREP_END)    p->w->instrecv = INST_REC_LOCK_END;
	if (p->w->instrecv == INST_REC_LOCK_PREP_CANCEL) p->w->instrecv = INST_REC_LOCK_CANCEL;
	/* start recording immediately if not cueing */
	if (p->w->instrecv == INST_REC_LOCK_START)
	{
		p->w->instrecv = INST_REC_LOCK_CONT;
		p->redraw = 1;
	}

	/*                        MULTITHREADING                         */
	/* isn't strictly realtime-safe, but *should* be ok (maybe)      */
	/* honestly half this file probably isn't strictly realtime-safe */

	/* handle the preview tracks first */
	for (uint8_t i = 0; i < PREVIEW_TRACKS; i++)
	{
		cv = p->w->previewtrack[i];
		preview_thread_running[i] = 0;
		if (cv->r.note != NOTE_VOID
				&& cv->r.inst != INST_VOID)
		{
#ifdef NO_MULTITHREADING
			previewTrackThreadRoutine(cv);
#else
			if (RUNNING_ON_VALGRIND)
				previewTrackThreadRoutine(cv);
			else
			{
				if (audio_api.realtimeThread(&preview_thread_ids[i], previewTrackThreadRoutine, cv))
					previewTrackThreadRoutine(cv);
				else
					preview_thread_running[i] = 1;
			}
#endif
		}
	}

	/* spawn threads for each track except for track 0 */
	for (uint8_t i = 1; i < p->s->track->c; i++)
	{
		thread_running[i] = 0;
		cv = p->s->track->v[i];
#ifdef NO_MULTITHREADING
		trackThreadRoutine(cv);
#else
		if (RUNNING_ON_VALGRIND)
			trackThreadRoutine(cv);
		else
		{
			if (audio_api.realtimeThread(&thread_ids[i], trackThreadRoutine, cv))
				trackThreadRoutine(cv);
			else
				thread_running[i] = 1;
		}
#endif
	}

	/* run track 0 in this thread */
	uint16_t c0spr = p->w->spr;
	uint16_t c0sprp = p->w->sprp;
	uint16_t c0playfy = p->w->playfy;
	if (p->s->track->c)
		_trackThreadRoutine(p->s->track->v[0], &c0spr, &c0sprp, &c0playfy, 1);

	joinProcessThreads();

	/* apply the new sprp and playfy track0 calculated */
	if (c0playfy != p->w->playfy)
	{
		if (p->w->follow) p->w->trackerfy = c0playfy;
		p->redraw = 1;
	}
	p->w->playfy = c0playfy;
	p->w->sprp = c0sprp;
	p->w->spr = c0spr;

	/* clear the master and send chain ports */
	memset(p->s->master->input[0], 0, nfptr * sizeof(float));
	memset(p->s->master->input[1], 0, nfptr * sizeof(float));
	memset(p->s->send->input[0], 0, nfptr * sizeof(float));
	memset(p->s->send->input[1], 0, nfptr * sizeof(float));

	/* sum the output from each track thread */
	for (uint8_t i = 0; i < p->s->track->c; i++)
	{
		cv = p->s->track->v[i];
		if (cv->effect->input[0] && cv->effect->input[1])
			for (uint32_t fptr = 0; fptr < nfptr; fptr++)
			{
				p->s->master->input[0][fptr] += cv->effect->input[0][fptr];
				p->s->master->input[1][fptr] += cv->effect->input[1][fptr];
			}
	}

	runEffectChain(nfptr, p->s->send);
	/* mix the send chain output into the master input */
	for (uint32_t fptr = 0; fptr < nfptr; fptr++)
	{
		p->s->master->input[0][fptr] += p->s->send->input[0][fptr];
		p->s->master->input[1][fptr] += p->s->send->input[1][fptr];
	}

	for (uint8_t i = 0; i < PREVIEW_TRACKS; i++)
	{
		for (uint32_t fptr = 0; fptr < nfptr; fptr++)
		{
			p->s->master->input[0][fptr] += p->w->previewtrack[i]->effect->input[0][fptr];
			p->s->master->input[1][fptr] += p->w->previewtrack[i]->effect->input[1][fptr];
		}
	}

	runEffectChain(nfptr, p->s->master);

	/* output */
	for (uint32_t fptr = 0; fptr < nfptr; fptr++)
		audio_api.writeAudio(fptr,
				p->s->master->input[0][fptr],
				p->s->master->input[1][fptr]);

	triggerFlash(p);
}

void processInput(uint32_t nfptr)
{
	if (p->w->instrecv == INST_REC_LOCK_CONT
			|| p->w->instrecv == INST_REC_LOCK_CUE_CONT)
	{
		if (p->w->recptr + nfptr > RECORD_LENGTH * samplerate)
		{
			p->w->instrecv = INST_REC_LOCK_END;
			strcpy(p->w->command.error, "record buffer full");
			p->redraw = 1;
		} else
		{
			float left, right;
			for (uint32_t fptr = 0; fptr < nfptr; fptr++)
			{
				if (p->w->recptr % samplerate == 0) p->redraw = 1; /* redraw the record time seconds counter */
				audio_api.readAudio(fptr, &left, &right);
				p->w->recbuffer[(p->w->recptr<<1) + 0] = hardclip(left) * SHRT_MAX;
				p->w->recbuffer[(p->w->recptr<<1) + 1] = hardclip(right) * SHRT_MAX;
				p->w->recptr++;
			}
		}
	}
}
