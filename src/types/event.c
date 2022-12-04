void pushEvent(Event *e)
{
#ifdef DEBUG_LOGS
	FILE *fp = fopen(".oml_eventdump", "a");
	fprintf(fp, "index %d - sem:%d, dest:%p, src:%p, callback:%p, callbackarg:%p\n", p->eventc, e->sem, e->dest, e->src, e->callback, e->callbackarg);
	fclose(fp);
#endif

	assert(p->eventc < EVENT_QUEUE_MAX); /* TODO: do something more useful than aborting */
	memcpy(&p->eventv[p->eventc], e, sizeof(Event));
	p->eventc++;
}

/* return true to block the main thread (skip input&redraw) */
bool mainM_SEM(void)
{
	if (p->eventc)
		switch (p->eventv[0].sem)
		{
			case M_SEM_DONE:
				memmove(&p->eventv[0], &p->eventv[1], sizeof(Event) * (EVENT_QUEUE_MAX-1));
				p->eventc--;
				break;

			case M_SEM_RELOAD_REQ:
			case M_SEM_SWAP_REQ:
				return 1;

			case M_SEM_BLOCK_CALLBACK:
			case M_SEM_CALLBACK:
				if (p->eventv[0].callback)
					p->eventv[0].callback(&p->eventv[0]);
				p->eventv[0].sem = M_SEM_DONE;
				break;

			case M_SEM_INPUT: {
					int keysymindex;
					unsigned int state;
					KeySym keysym;
					XEvent *ev = p->eventv[0].callbackarg;
					switch (ev->type)
					{
						case KeyPress:
							state = ((XKeyEvent*)ev)->state;
							if (state&LockMask) state^=ShiftMask;
							keysymindex = (state&ShiftMask);
							if (state&ShiftMask) state^=ShiftMask;
							keysym = XLookupKeysym((XKeyEvent*)ev, keysymindex);
							inputTooltip(&tt, state, keysym, 0);
							break;
						case KeyRelease:
							state = ((XKeyEvent*)ev)->state;
							if (state&LockMask) state^=ShiftMask;
							keysymindex = (state&ShiftMask);
							if (state&ShiftMask) state^=ShiftMask;
							keysym = XLookupKeysym((XKeyEvent*)ev, keysymindex);
							inputTooltip(&tt, state, keysym, 1);
							break;
						case Expose: while (XCheckTypedEvent(dpy, Expose, ev)); break;
					}
					free(p->eventv[0].callbackarg);
					p->eventv[0].sem = M_SEM_DONE;
				} break;
		}
	return 0;
}

/* return true to skip processing entirely */
bool processM_SEM(void)
{
	if (p->eventc)
	{
		switch (p->eventv[0].sem)
		{
			case M_SEM_RELOAD_REQ: /* blocking */
				p->eventv[0].sem = M_SEM_BLOCK_CALLBACK;
			case M_SEM_BLOCK_CALLBACK:
				return 1;

			case M_SEM_SWAP_PREVIEWSAMPLE_PREVIEW_REQ:
				previewFileNote(p->w, (int)(size_t)p->eventv[0].callbackarg);
			case M_SEM_SWAP_REQ: { /* non-blocking */
					void *hold = *p->eventv[0].dest;
					*p->eventv[0].dest = p->eventv[0].src;
					p->eventv[0].src = hold;
					p->eventv[0].sem = M_SEM_CALLBACK;
				} break;
			case M_SEM_BPM:
				setBpm(&p->s->spr, p->s->songbpm);
				p->eventv[0].sem = M_SEM_DONE;
				break;
			case M_SEM_TRACK_MUTE: {
					Track *cv;
					Instrument *iv;
					for (uint8_t c = 0; c < p->s->track->c; c++)
					{
						cv = &p->s->track->v[c];
						if (cv->data.mute && instrumentSafe(p->s, cv->r.inst))
						{
							iv = &p->s->instrument->v[p->s->instrument->i[cv->r.inst]];
							if (iv->midi.channel != -1)
							{
								midiNoteOff(0, iv->midi.channel, cv->r.note, (cv->gain.rand>>4)<<3);
								cv->r.note = NOTE_VOID;
							}
						}
					}
					p->eventv[0].sem = M_SEM_DONE;
				} break;
		}
	}
	return 0;
}

void cb_reloadFile(Event *e)
{
	Song *cs = readSong(w->newfilename);
	if (cs) { delSong(s); s = cs; }
	p->s = s;
	if (s->loop[1]) w->trackerfy = s->loop[0];
	else            w->trackerfy = STATE_ROWS;
	w->page = PAGE_TRACK_VARIANT;
	regenGlobalRowc(s);
	reapplyBpm();
}
