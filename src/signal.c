void pushEvent(Event *e)
{
	assert(p->eventc < EVENT_QUEUE_MAX); /* TODO: do something more useful than aborting */
	memcpy(&p->eventv[p->eventc], e, sizeof(Event));
	/* p->eventv[p->eventc].sem = e->sem;
	p->eventv[p->eventc].swap1 = e->swap1;
	p->eventv[p->eventc].swap2 = e->swap2;
	p->eventv[p->eventc].callback = e->callback;
	p->eventv[p->eventc].callbackarg = e->callbackarg; */
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
	Channel *cv;
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

			case M_SEM_CHANNEL_MUTE:
				for (uint8_t c = 0; c < p->s->channel->c; c++)
				{
					cv = &p->s->channel->v[c];
					if (cv->data.mute && instrumentSafe(p->s, cv->r.inst))
					{
						iv = &p->s->instrument->v[p->s->instrument->i[cv->r.inst]];
						if (iv->midi.channel != -1)
						{
							midiNoteOff(0, iv->midi.channel, cv->r.note, (cv->randgain>>4)<<3);
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
	w->trackerfy = STATE_ROWS;
	w->page = PAGE_CHANNEL_VARIANT;
	regenGlobalRowc(s);
	reapplyBpm();
}
