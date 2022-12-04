void setBpmCount(void)
{
	if (w->count)
	{
		s->songbpm = MIN(255, MAX(32, w->count));
		reapplyBpm();
		p->redraw = 1;
	}
}

void showTracker(void)
{
	switch (w->page)
	{
		case PAGE_TRACK_VARIANT:
			w->effectscroll = 0;
			w->mode = T_MODE_NORMAL;
			w->page = PAGE_TRACK_EFFECT;
			break;
		case PAGE_TRACK_EFFECT:
			w->page = PAGE_TRACK_VARIANT;
			break;
		default:
			w->page = PAGE_TRACK_VARIANT;
			w->mode = T_MODE_NORMAL;
			break;
	}
	freePreviewSample();
	p->redraw = 1;
}
void showInstrument(void)
{
	w->showfilebrowser = 0;
	w->page = PAGE_INSTRUMENT;
	w->mode = I_MODE_NORMAL;

	if (s->instrument->i[w->instrument] != INSTRUMENT_VOID)
		resetWaveform();
	freePreviewSample();
	p->redraw = 1;
}
void showMaster(void)
{
	switch (w->page)
	{
		case PAGE_EFFECT_MASTER: w->page = PAGE_EFFECT_SEND; break;
		case PAGE_EFFECT_SEND: w->page = PAGE_EFFECT_MASTER; break;
		default:
			w->page = PAGE_EFFECT_MASTER;
			w->mode = 0;
			break;
	}
	freePreviewSample();
	p->redraw = 1;
}
