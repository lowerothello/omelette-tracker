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
	if (s->loop[1]) w->playfy = s->loop[0];
	else            w->playfy = 0;
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
	} else
	{
		if (s->loop[1]) w->trackerfy = s->loop[0];
		else            w->trackerfy = 0;
	}
	w->mode = 0; /* always go to mode 0 on stop */
	p->redraw = 1;
}
