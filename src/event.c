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

/* return true to block the main thread */
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
		}
	return 0;
}

/* return true to skip processing entirely */
bool processM_SEM(void)
{
	void *hold;
	Track *cv;
	Instrument *iv;
	if (p->eventc)
	{
		switch (p->eventv[0].sem)
		{
			case M_SEM_RELOAD_REQ: /* blocking */
				p->eventv[0].sem = M_SEM_BLOCK_CALLBACK;
			case M_SEM_BLOCK_CALLBACK:
				return 1;

			case M_SEM_SWAP_PREVIEWSAMPLE_PREVIEW_REQ:
				_previewNote(p->w, (int)(size_t)p->eventv[0].callbackarg, 0);
				p->w->previewtrigger = PTRIG_FILE;
			case M_SEM_SWAP_REQ: /* non-blocking */
				hold = *p->eventv[0].dest;
				*p->eventv[0].dest = p->eventv[0].src;
				p->eventv[0].src = hold;
				p->eventv[0].sem = M_SEM_CALLBACK;
				break;

			case M_SEM_BPM:
				setBpm(&p->s->spr, p->s->songbpm);
				p->eventv[0].sem = M_SEM_DONE;
				break;

			case M_SEM_TRACK_MUTE:
				for (uint8_t c = 0; c < p->s->track->c; c++)
				{
					cv = &p->s->track->v[c];
					if (cv->data.mute && instrumentSafe(p->s, cv->r.inst))
					{
						iv = &p->s->instrument->v[p->s->instrument->i[cv->r.inst]];
						if (iv->midi.track != -1)
						{
							midiNoteOff(0, iv->midi.track, cv->r.note, (cv->gain.rand>>4)<<3);
							cv->r.note = NOTE_VOID;
						}
					}
				}
				p->eventv[0].sem = M_SEM_DONE;
				break;
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
