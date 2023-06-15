static void leaveSpecialModes(void)
{
	switch (w->page)
	{
		case PAGE_VARIANT:
		case PAGE_PATTERN:
			switch (w->mode)
			{
				case MODE_INSERT: break;
				default: w->mode = MODE_NORMAL; break;
			} break;
		default: break;
	}
}

void startPlayback(void)
{
	leaveSpecialModes();
	w->playfy = w->trackerfy - (w->trackerfy % (s->plen+1));
	w->sprp = 0;
	if (w->follow)
		w->trackerfy = w->playfy;
	Event ev;
	ev.sem = M_SEM_PLAYING_START;
	pushEvent(&ev);
	p->redraw = 1;
}

void stopPlayback(void)
{
	leaveSpecialModes();
	if (w->playing)
	{
		if (w->instrecv == INST_REC_LOCK_CONT || w->instrecv == INST_REC_LOCK_CUE_CONT)
			w->instrecv = INST_REC_LOCK_PREP_END;
		Event ev;
		ev.sem = M_SEM_PLAYING_STOP;
		pushEvent(&ev);
	} else if (!(w->trackerfy % (s->plen+1)))
	{
		w->follow = 0;
		w->trackerfy = 0;
	} else
	{
		w->follow = 0;
		w->trackerfy -= w->trackerfy % (s->plen+1);
	}

	w->mode = MODE_NORMAL;
	p->redraw = 1;
}
