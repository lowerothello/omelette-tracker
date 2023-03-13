void pushEvent(Event *e)
{
#ifdef DEBUG_LOGS
	FILE *fp = fopen(".oml_eventdump", "a");
	fprintf(fp, "index %d - sem:%d, dest:%p, src:%p, callback:%p, callbackarg:%p\n", p->eventc, e->sem, e->dest, e->src, e->callback, e->callbackarg);
	fclose(fp);
#endif

	assert(p->eventc < EVENT_QUEUE_MAX); /* TODO: do something more useful than aborting */
	memcpy(&p->event[p->eventc], e, sizeof(Event));
	p->eventc++;
}

/* free events that haven't been handled yet */
void freeEvents(void)
{
	while (p->eventc)
	{
		switch (p->event[0].sem)
		{
			case M_SEM_INPUT:
				free(p->event[0].callbackarg);
				break;
			default: break;
		}
		memmove(&p->event[0], &p->event[1], sizeof(Event) * (EVENT_QUEUE_MAX-1));
		p->eventc--;
	}
}

/* return true to block the main thread (skip input&redraw) */
bool mainM_SEM(void)
{
mainM_SEM_start: /* allow for processing multiple events in one go */
	if (p->eventc)
		switch (p->event[0].sem)
		{
			case M_SEM_DONE:
mainM_SEM_pop: /* allow for processing and popping off the stack in one go */
				memmove(&p->event[0], &p->event[1], sizeof(Event) * (EVENT_QUEUE_MAX-1));
				p->eventc--;
				goto mainM_SEM_start;

			case M_SEM_RELOAD_REQ:
			case M_SEM_SWAP_REQ:
				return 1;

			case M_SEM_BLOCK_CALLBACK:
			case M_SEM_CALLBACK:
				if (p->event[0].callback)
					p->event[0].callback(&p->event[0]);
				goto mainM_SEM_pop;

#ifdef OML_X11
			case M_SEM_INPUT: {
					int keysymindex;
					unsigned int state;
					KeySym keysym;
					XEvent *ev = p->event[0].callbackarg;

					state = ((XKeyEvent*)ev)->state;
					if (state&LockMask) state^=ShiftMask;
					keysymindex = (state&ShiftMask);
					keysym = XLookupKeysym((XKeyEvent*)ev, keysymindex);

					/* turn off the shift flag if it's already been processed */
					if (isascii(keysym) && state&ShiftMask)
						state^=ShiftMask;

					inputTooltip(state, keysym, ev->type - KeyPress);

					free(p->event[0].callbackarg);
				} goto mainM_SEM_pop;
#endif

			default: break;
		}
	return 0;
}

/* return true to skip processing entirely */
bool processM_SEM(void)
{
	Track *cv;
	int i;
	if (p->eventc)
	{
		switch (p->event[0].sem)
		{
			case M_SEM_RELOAD_REQ: /* blocking */
				p->event[0].sem = M_SEM_BLOCK_CALLBACK;
			case M_SEM_BLOCK_CALLBACK:
				return 1;

			case M_SEM_SWAP_REQ: { /* non-blocking */
					void *hold = *p->event[0].dest;
					*p->event[0].dest = p->event[0].src;
					p->event[0].src = hold;
					p->event[0].sem = M_SEM_CALLBACK;
				} break;
			case M_SEM_BPM:
				setBpm(&p->s->spr, p->s->songbpm);
				p->event[0].sem = M_SEM_DONE;
				break;
			case M_SEM_TRACK_MUTE: {
					Track *cv;
					Instrument *iv;
					for (uint8_t c = 0; c < p->s->track->c; c++)
					{
						cv = &p->s->track->v[c];
						if (cv->mute && instrumentSafe(p->s->instrument, cv->r.inst))
						{
							iv = &p->s->instrument->v[p->s->instrument->i[cv->r.inst]];
							if (iv->midi.channel != -1)
							{
								midiNoteOff(0, iv->midi.channel, cv->r.note, (cv->gain.rand>>4)<<3);
								cv->r.note = NOTE_VOID;
							}
						}
					}
					p->event[0].sem = M_SEM_DONE;
				} break;
			case M_SEM_PREVIEW:
				if (p->event[0].arg3) /* release */
				{
					int voice = getPreviewVoice(p->event[0].arg1, 1);
					if (voice != -1)
						triggerNote(0, &p->w->previewtrack[voice], p->w->previewtrack[voice].r.note, NOTE_OFF, p->event[0].arg2);
				} else
				{
					if (p->event[0].arg1 == NOTE_OFF)
					{
						for (int i = 0; i < PREVIEW_TRACKS; i++)
							if (p->w->previewtrack[i].r.note != NOTE_VOID)
								triggerNote(0, &p->w->previewtrack[i], p->w->previewtrack[i].r.note, NOTE_OFF, p->event[0].arg2);
					} else
					{
						int voice = getPreviewVoice(p->event[0].arg1, 0);
						if (voice != -1)
							triggerNote(0, &p->w->previewtrack[voice], p->w->previewtrack[voice].r.note, p->event[0].arg1, p->event[0].arg2);
					}
				}
				p->event[0].sem = M_SEM_DONE;
				p->redraw = 1;
				break;
			case M_SEM_PLAYING_START:
				setBpm(&p->s->spr, p->s->songbpm);
				p->s->sprp = 0;

				/* stop preview */
				for (i = 0; i < PREVIEW_TRACKS; i++)
				{
					p->w->previewtrack[i].r.note = NOTE_VOID;
					p->w->previewtrack[i].r.inst = INST_VOID;
				}

				/* TODO: also stop the sampler's follower note */
				/* TODO: is it worth it to multithread this?   */
				/* clear the tracks */
				for (uint8_t c = 0; c < p->s->track->c; c++)
				{
					cv = &p->s->track->v[c];
					triggerNote(0, cv, cv->r.note, NOTE_OFF, cv->r.inst);

					lookback(0, &p->s->spr, p->s->playfy, cv);
					processRow(0, &p->s->spr, 1, &p->s->track->v[c], getTrackRow(&p->s->track->v[c], p->s->playfy));
				}


				/* start recording if cueing */
				if (p->w->instrumentrecv == INST_REC_LOCK_CUE_START)
					p->w->instrumentrecv = INST_REC_LOCK_CUE_CONT;

				p->s->playing = 1;
				p->redraw = 1;
				p->event[0].sem = M_SEM_DONE;
				break;
			case M_SEM_PLAYING_STOP:
				/* stop tracks */
				for (uint8_t i = 0; i < p->s->track->c; i++)
				{
					cv = &p->s->track->v[i];
					cv->delaysamples = 0;
					cv->cutsamples = 0;
					if (instrumentSafe(p->s->instrument, cv->r.inst))
						ramp(cv, (float)p->s->sprp / (float)p->s->spr, p->s->instrument->i[cv->r.inst]);
					triggerNote(0, cv, cv->r.note, NOTE_OFF, cv->r.inst);
				}

				if (p->s->loop[2])
				{
					p->s->loop[1] = p->s->loop[2];
					p->s->loop[2] = 0;
				}

				p->s->playing = 0;
				p->redraw = 1;
				p->event[0].sem = M_SEM_DONE;
				break;
			default: break;
		}
	}
	return 0;
}

void cb_reloadFile(Event *e)
{
	Song *cs = readSongNew(w->filepath);
	if (cs) { freeSong(s); s = cs; }
	p->s = s;
	if (s->loop[1]) w->trackerfy = s->loop[0];
	else            w->trackerfy = STATE_ROWS;
	w->page = PAGE_VARIANT;
	regenGlobalRowc(s);
	reapplyBpm();
}
