void setBpm(uint16_t *spr, uint8_t newbpm)
{ *spr = samplerate * (60.f / newbpm) / p->s->rowhighlight; }

/* freewheel to fill up the ramp buffer */
void ramp(uint32_t fptr, uint16_t *spr, uint32_t sprp, float rowprogress, Track *cv, uint8_t realinstrument)
{
	// VALGRIND_PRINTF("ramp(%d, %d, %d, %p, %f, %d)\n", fptr, *spr, sprp, cv, rowprogress, realinstrument);
	/* clear the rampbuffer properly so cruft isn't played in edge cases */
	memset(cv->rampbuffer, 0, sizeof(float) * rampmax * 2);

	float finetune = 0.0f;
	uint32_t pointeroffset = cv->pointer;
	uint32_t pitchedpointeroffset = cv->pitchedpointer;
	macroCallbackVolatile(fptr, 1, spr, sprp, cv, &finetune, &pointeroffset, &pitchedpointeroffset);

	if (realinstrument < p->s->inst->c) /* TODO: should use instSafe */
	{
		short l = 0, r = 0; /* initialized to 0 in case the InstAPI is invalid */
		Inst *iv = &p->s->inst->v[realinstrument];
		const InstAPI *api;
		char inststatebak[instGetPlaybackStateSize()];
		memcpy(inststatebak, cv->inststate, instGetPlaybackStateSize());

		pitchedpointeroffset++;
		if (cv->reverse)
		{
			uint32_t localrampmax;
			if (pointeroffset < rampmax)
				localrampmax = rampmax - pointeroffset;
			else localrampmax = rampmax;

			for (uint16_t i = 0; i < localrampmax; i++)
			{
				if (pitchedpointeroffset) pitchedpointeroffset--;
				if ((api = instGetAPI(iv->type)))
					api->process(iv, cv, rowprogress, pointeroffset+i, pitchedpointeroffset, finetune, &l, &r);
				cv->rampbuffer[i*2 + 0] = (float)l*DIVSHRT;
				cv->rampbuffer[i*2 + 1] = (float)r*DIVSHRT;
			}
		} else
			for (uint16_t i = 0; i < rampmax; i++)
			{
				pitchedpointeroffset++;
				if ((api = instGetAPI(iv->type)))
					api->process(iv, cv, rowprogress, pointeroffset+i, pitchedpointeroffset, finetune, &l, &r);
				cv->rampbuffer[i*2 + 0] = (float)l*DIVSHRT;
				cv->rampbuffer[i*2 + 1] = (float)r*DIVSHRT;
			}
		memcpy(cv->inststate, inststatebak, instGetPlaybackStateSize());
	} cv->rampindex = 0;
}

void midiNoteOff(uint32_t fptr, uint8_t midichannel, uint8_t note, uint8_t velocity)
{
	if (note != NOTE_VOID)
	{
		unsigned char event[3] = {0b10000000 | midichannel, note, velocity};
		audio_api.writeMidi(fptr, event, 3);
	}
}
void midiNoteOn(uint32_t fptr, uint8_t midichannel, uint8_t note, uint8_t velocity)
{
	if (note != NOTE_VOID)
	{
		unsigned char event[3] = {0b10010000 | midichannel, note, velocity};
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
			midiNoteOff(0, ((InstMidiState*)iv->state)->channel, cv->r.note, (cv->gain.rand>>4)<<3);
			cv->r.note = NOTE_VOID;
		}
	}
}

bool triggerMidi(uint32_t fptr, InstMidiState *ims, Track *cv, uint8_t oldnote, uint8_t note, uint8_t inst)
{
	if (note != NOTE_VOID && !cv->mute)
	{
		if (ims->channel != -1)
		{
			/* always stop the prev. note */
			midiNoteOff(fptr, ims->channel, oldnote, (cv->gain.rand>>4)<<3);
			midiNoteOn (fptr, ims->channel, note,    (cv->gain.rand>>4)<<3);
			return 1;
		}
	} return 0;
}

/* TODO: should maybe take oldnote and a row as args */
void triggerNote(uint32_t fptr, Track *cv, uint8_t oldnote, uint8_t note, short inst)
{
	Inst *iv;
	const InstAPI *api;

	switch (note)
	{
		case NOTE_VOID:
		case NOTE_OFF:
			cv->release = 1;
			break;
		case NOTE_CUT:
			/* always ramp when triggering anyway */
			cv->r.note = NOTE_VOID;
			break;
		default:
			if (note < NOTE_MAX) /* legato */
				cv->pointer = cv->pitchedpointer = 0;

			cv->r.note = note%NOTE_MAX;
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

			macroCallbackTriggerNote(fptr, cv, oldnote, note, inst);

			break;
	}

	if (instSafe(p->s->inst, inst))
	{ /* TODO: don't really like that this is checked twice */
		iv = &p->s->inst->v[p->s->inst->i[inst]];
		if ((api = instGetAPI(iv->type)))
			api->triggernote(fptr, iv, cv, oldnote, note, inst);
	}
}

void processRow(uint32_t fptr, uint16_t *spr, bool midi, Track *cv, Row *r)
{
	bool triggerramp = 0;
	Track oldcv;
	memcpy(&oldcv, cv, sizeof(Track));

	for (int i = 0; i <= cv->variant->macroc; i++)
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

	lf = hardclip(lf);
	rf = hardclip(rf);

	/* gain multipliers */
	macroStateGetStereo(&cv->gain, rp, &cv->mainmult[0][fptr], &cv->mainmult[1][fptr]);

	cv->effect->input[0][fptr] = lf;
	cv->effect->input[1][fptr] = rf;

	if (cv->send.target != -1)
	{
		cv->sendmult[0][fptr] = (cv->send.rand>>4)*DIV16 + ((cv->send.target>>4) - (cv->send.rand>>4))*DIV16 * rp;
		cv->sendmult[1][fptr] = (cv->send.rand&15)*DIV16 + ((cv->send.target&15) - (cv->send.rand>>4))*DIV16 * rp;
	} else if (cv->send.rand)
	{
		cv->sendmult[0][fptr] = (cv->send.rand>>4)*DIV16;
		cv->sendmult[1][fptr] = (cv->send.rand&15)*DIV16;
	}
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
			if (cv->pitchedpointer > *spr) cv->pitchedpointer -= *spr;
			else                           cv->pitchedpointer = 0;
		} else cv->pitchedpointer += *spr;

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
	uint8_t newnote = macroCallbackSampleRow(fptr, 1, spr, sprp, cv);
	if (newnote != NOTE_VOID)
	{
		if (instSafe(p->s->inst, cv->r.inst))
			ramp(fptr, spr, sprp, rowprogress, cv, p->s->inst->i[cv->r.inst]);
		triggerNote(fptr, cv, cv->r.note, newnote, cv->r.inst);

		macroCallbackPostTrig(fptr, spr, cv, &cv->r);
	}

	macroCallbackPersistent(fptr, 1, spr, sprp, cv);

	float finetune = 0.0f;
	uint32_t pointer = cv->pointer;
	uint32_t pitchedpointer = cv->pitchedpointer;
	macroCallbackVolatile(fptr, 1, spr, sprp, cv, &finetune, &pointer, &pitchedpointer);

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
					api->process(iv, cv, rowprogress, pointer, pitchedpointer, finetune, &li, &ri);

				if (cv->reverse) { if (cv->pitchedpointer) cv->pitchedpointer--; }
				else                                       cv->pitchedpointer++;

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
		/* scope lookback notes within the most recent vtrig */
		if (cv->variant->trig[i].index != VARIANT_VOID) { cv->r.note = NOTE_VOID; }

		r = getTrackRow(cv, i);
		if (p->w->bpmcachelen > i && p->w->bpmcache[i] != -1) macroBpm(fptr, spr, p->w->bpmcache[i], cv, r);
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
				if (p->s->loop[1] && *playfy > p->s->loop[1])
				{
					*playfy = p->s->loop[0];
					if (p->s->loop[2])
					{
						p->s->loop[1] = p->s->loop[2];
						p->s->loop[2] = 0;
					}
					lookback(fptr, spr, *playfy, cv);
					if (p->w->instrecv == INST_REC_LOCK_CUE_CONT) p->w->instrecv = INST_REC_LOCK_END;
					if (p->w->instrecv == INST_REC_LOCK_CUE_START) p->w->instrecv = INST_REC_LOCK_CUE_CONT;
				} else if (!p->s->loop[1] && *playfy >= p->s->songlen)
				{
					*playfy = STATE_ROWS;
					lookback(fptr, spr, *playfy, cv);
					if (p->w->instrecv == INST_REC_LOCK_CUE_CONT) p->w->instrecv = INST_REC_LOCK_END;
					if (p->w->instrecv == INST_REC_LOCK_CUE_START) p->w->instrecv = INST_REC_LOCK_CUE_CONT;
				}

				/* preprocess track */
				r = getTrackRow(cv, *playfy);
				if (p->w->bpmcachelen > *playfy && p->w->bpmcache[*playfy] != -1) macroBpm(fptr, spr, p->w->bpmcache[*playfy], cv, r);
				processRow(fptr, spr, 1, cv, r);
			}
		}
	}

	/* track insert effects */
	if (cv->effect) /* previewtrack doesn't have any effects so this check is required */
		for (uint8_t i = 0; i < cv->effect->c; i++)
			runEffect(buffersize, cv->effect, &cv->effect->v[i]);
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
	for (int i = 0; i < p->s->inst->c; i++)
		if (p->s->inst->v[i].triggerflash)
		{
			if (p->s->inst->v[i].triggerflash == 1) p->redraw = 1;
			p->s->inst->v[i].triggerflash--;
		}

	for (int i = 0; i < p->s->track->c; i++)
		if (p->s->track->v[i]->triggerflash)
		{
			if (p->s->track->v[i]->triggerflash == 1) p->redraw = 1;
			p->s->track->v[i]->triggerflash--;
		}
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
#ifndef NO_MULTITHREADING
	bool preview_thread_failed[PREVIEW_TRACKS];
	memset(preview_thread_failed, 1, PREVIEW_TRACKS);
	bool thread_failed[p->s->track->c-1];
	memset(thread_failed, 1, p->s->track->c-1);
	pthread_t preview_thread_ids[PREVIEW_TRACKS];
	pthread_t thread_ids[p->s->track->c-1]; /* index 0 is the preview track */
#endif

	/* handle the preview tracks first */
	for (uint8_t i = 0; i < PREVIEW_TRACKS; i++)
	{
		if (p->w->previewtrack[i]->r.note != NOTE_VOID
				&& p->w->previewtrack[i]->r.inst != INST_VOID)
		{
#ifdef NO_MULTITHREADING
			previewTrackThreadRoutine(p->w->previewtrack[i]);
#else
			preview_thread_failed[i] = 0; /* set low if the thread needs to be joined with */
			if (RUNNING_ON_VALGRIND)
				previewTrackThreadRoutine(p->w->previewtrack[i]);
			else
				if (audio_api.realtimeThread(&preview_thread_ids[i], previewTrackThreadRoutine, p->w->previewtrack[i]))
				{
					preview_thread_failed[i] = 1;
					previewTrackThreadRoutine(p->w->previewtrack[i]);
				}
#endif
		}
	}

	/* spawn threads for each track except for track 0 */
	for (uint8_t i = 1; i < p->s->track->c; i++)
	{
		cv = p->s->track->v[i];
#ifdef NO_MULTITHREADING
		trackThreadRoutine(cv);
#else
		thread_failed[i-1] = 0;
		if (RUNNING_ON_VALGRIND)
			trackThreadRoutine(cv);
		else
			if (audio_api.realtimeThread(&thread_ids[i-1], trackThreadRoutine, cv))
			{
				thread_failed[i-1] = 1;
				trackThreadRoutine(cv);
			}
#endif
	}

	/* run track 0 in this thread */
	uint16_t c0spr = p->w->spr;
	uint16_t c0sprp = p->w->sprp;
	uint16_t c0playfy = p->w->playfy;
	if (p->s->track->c)
		_trackThreadRoutine(p->s->track->v[0], &c0spr, &c0sprp, &c0playfy, 1);

#ifndef NO_MULTITHREADING
	if (!RUNNING_ON_VALGRIND)
	{
		/* join with the track threads */
		for (uint8_t i = 1; i < p->s->track->c; i++)
			if (!thread_failed[i-1])
				pthread_join(thread_ids[i-1], NULL);

		/* join with the prevew threads */
		for (uint8_t i = 1; i < p->s->track->c; i++)
			if (!preview_thread_failed[i])
				pthread_join(preview_thread_ids[i], NULL);
	}
#endif

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
				p->s->master->input[0][fptr] += cv->effect->input[0][fptr] * cv->mainmult[0][fptr];
				p->s->master->input[1][fptr] += cv->effect->input[1][fptr] * cv->mainmult[1][fptr];
				p->s->send->input[0][fptr] += cv->effect->input[0][fptr] * cv->sendmult[0][fptr];
				p->s->send->input[1][fptr] += cv->effect->input[1][fptr] * cv->sendmult[1][fptr];
			}
	}

	/* send chain */
	for (uint8_t i = 0; i < p->s->send->c; i++)
		runEffect(nfptr, p->s->send, &p->s->send->v[i]);
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
			p->s->master->input[0][fptr] += p->w->previewtrack[i]->effect->input[0][fptr] * p->w->previewtrack[i]->mainmult[0][fptr];
			p->s->master->input[1][fptr] += p->w->previewtrack[i]->effect->input[1][fptr] * p->w->previewtrack[i]->mainmult[1][fptr];
		}
	}

	/* master chain */
	for (uint8_t i = 0; i < p->s->master->c; i++)
		runEffect(nfptr, p->s->master, &p->s->master->v[i]);

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
