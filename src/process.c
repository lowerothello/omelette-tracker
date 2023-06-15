typedef struct Thread
{
	pthread_t id;
	bool running;
} Thread;

int threadcount;
Thread  *thread;


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
		r = getTrackRow(cv, i, 0);
		/* TODO: proper implementation of master track macros, including bpm */
		// if (p->w->bpmcachelen > i && p->w->bpmcache[i] != -1) macroBpm(fptr, spr, p->w->bpmcache[i], cv, r);
		if (r)
			processRow(fptr, spr, 0, cv, r);
		playTrackLookback(fptr, spr, cv);
	}
}

static int walkSongPointer(uint32_t bufsize, uint16_t *spr, uint16_t *sprp, uint16_t *playfy, bool readrows)
{
	if (!(*spr)) return 0; /* don't try to iterate over a NULL'd spr */

	(*sprp) += bufsize;

	int ret = 0;
	while (*sprp > *spr)
	{
		*sprp -= *spr;
		if (readrows && p->w->playing)
		{
			/* walk the pointer and loop */
			(*playfy)++;
			if (p->w->loop && !(*playfy % (p->s->plen+1))) { *playfy -= p->s->plen + 1; ret = MAX(ret, 2); }
			if (*playfy >= (p->s->slen+1) * (p->s->plen+1)) { *playfy = 0; ret = MAX(ret, 2); }

			ret = MAX(ret, 1);
		}
	}
	return ret;
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

#ifndef DEBUG_DISABLE_AUDIO_OUTPUT
static void _trackThreadRoutine(Track *cv, bool readrows)
{
	Row *r;
	uint16_t spr = p->w->spr;
	uint16_t sprp = p->w->sprp;
	uint16_t playfy = p->w->playfy;

	for (uint32_t fptr = 0; fptr < buffersize; fptr++) /* TODO: probably shouldn't rely on buffersize here */
	{
		playTrack(fptr, &spr, sprp, cv);

		switch (walkSongPointer(1, &spr, &sprp, &playfy, readrows))
		{
			case 2:
				lookback(fptr, &spr, playfy, cv);
				switch (p->w->instrecv)
				{
					case INST_REC_LOCK_CUE_CONT:
						p->w->instrecv = INST_REC_LOCK_END;
						break;
					case INST_REC_LOCK_CUE_START:
						p->w->instrecv = INST_REC_LOCK_CUE_CONT;
						break;
					default: break;
				}
				/* fall through */
			case 1:
				/* TODO: proper implementation of master track macros, including bpm */
				// if (p->w->bpmcachelen > *playfy && p->w->bpmcache[*playfy] != -1) macroBpm(fptr, &spr, p->w->bpmcache[*playfy], cv, r);
				if ((r = getTrackRow(cv, playfy, 0)))
					processRow(fptr, &spr, 1, cv, r);
				break;
		}
	}

	/* track insert effects */
	if (cv->effect) /* previewtrack doesn't have any effects so this check is required */
		runEffectChain(buffersize, cv->effect);
}

static void procThreadTrack(size_t threadcount, size_t threadindex, uint8_t previewtrackcount)
{
	uint8_t i, trackcount;

	/* recalculate every time in case trackc has changed */
	trackcount = p->s->track->c / threadcount;
	if (p->s->track->c % threadcount > threadcount)
		trackcount++;

	for (i = 0; i < trackcount; i++)
		_trackThreadRoutine(p->s->track->v[(i*threadcount) + threadindex], 1);

	for (i = 0; i < previewtrackcount; i++)
		_trackThreadRoutine(p->w->previewtrack[(i*threadcount) + threadindex], 0);
}

static void *procThreadRoutine(void *arg)
{
	size_t i = (size_t)arg;
	struct timespec ts;

	/* PREVIEW_TRACKS is constant */
	uint8_t previewtrackcount = PREVIEW_TRACKS / threadcount;
	if (PREVIEW_TRACKS % threadcount > i)
		previewtrackcount++;

	while (1)
	{
		if (thread[i].running)
		{
			procThreadTrack(threadcount, i, previewtrackcount);
			thread[i].running = 0;
		}

		/* give time back to the system */
		ts.tv_sec = 0;
		ts.tv_nsec = 1;
		nanosleep(&ts, NULL);
	}
	return NULL;
}
#endif /* DEBUG_DISABLE_AUDIO_OUTPUT */

void processOutput(uint32_t nfptr)
{
	/* wait for every work thread to be done */
#ifndef DEBUG_DISABLE_AUDIO_OUTPUT
	struct timespec ts;
	if (!RUNNING_ON_VALGRIND)
	{
pollThreads:
		for (int i = 0; i < threadcount; i++)
		{
			if (thread[i].running)
			{
				ts.tv_sec = 0;
				ts.tv_nsec = 1;
				nanosleep(&ts, NULL);
				goto pollThreads;
			}
		}
	}
#endif /* DEBUG_DISABLE_AUDIO_OUTPUT */


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


	if (walkSongPointer(nfptr, &p->w->spr, &p->w->sprp, &p->w->playfy, 1))
		p->redraw = 1;

	if (p->w->follow && p->w->trackerfy != p->w->playfy)
	{
		p->w->trackerfy = p->w->playfy;
		p->redraw = 1; /* set redraw high AGAIN just to make sure it catches the trackerfy change */
	}


	/* clear the master ports */
	memset(p->s->master->input[0], 0, nfptr * sizeof(float));
	memset(p->s->master->input[1], 0, nfptr * sizeof(float));

	Track *cv;
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

	for (uint8_t i = 0; i < PREVIEW_TRACKS; i++)
	{
		for (uint32_t fptr = 0; fptr < nfptr; fptr++)
		{
			p->s->master->input[0][fptr] += p->w->previewtrack[i]->effect->input[0][fptr];
			p->s->master->input[1][fptr] += p->w->previewtrack[i]->effect->input[1][fptr];
		}
	}

#ifndef DEBUG_DISABLE_AUDIO_OUTPUT
	if (!RUNNING_ON_VALGRIND)
	{
		/* start up the next batch of work */
		for (int i = 0; i < threadcount; i++)
			thread[i].running = 1;
	} else
		procThreadTrack(1, 0, PREVIEW_TRACKS);
#endif /* DEBUG_DISABLE_AUDIO_OUTPUT */

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

void spawnProcThreads(void)
{
#ifndef DEBUG_DISABLE_AUDIO_OUTPUT
	if (!RUNNING_ON_VALGRIND)
	{
		threadcount = get_nprocs(); /* TODO: linux-specific */
		thread = malloc(sizeof(Thread) * threadcount);
		for (int i = 0; i < threadcount; i++)
		{
			thread[i].running = 0;
			pthread_create(&thread[i].id, NULL, procThreadRoutine, (void*)(size_t)i);
		}
	}
#endif /* DEBUG_DISABLE_AUDIO_OUTPUT */
}
void killProcThreads(void)
{
#ifndef DEBUG_DISABLE_AUDIO_OUTPUT
	if (!RUNNING_ON_VALGRIND)
	{
		for (int i = 0; i < threadcount; i++)
		{
			pthread_cancel(thread[i].id);
			pthread_join(thread[i].id, NULL);
		}

		free(thread);
	}
#endif /* DEBUG_DISABLE_AUDIO_OUTPUT */
}
