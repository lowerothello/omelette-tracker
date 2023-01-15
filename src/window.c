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
	w->page = PAGE_VARIANT;
	w->mode = MODE_NORMAL;
	freePreviewSample();
	p->redraw = 1;
}
void showInstrument(void)
{
	w->showfilebrowser = 0;
	w->page = PAGE_INSTRUMENT;
	w->mode = MODE_NORMAL;

	if (s->instrument->i[w->instrument] != INSTRUMENT_VOID)
		resetWaveform();
	freePreviewSample();
	p->redraw = 1;
}

UI *allocWindow(void)
{
	UI *ret = calloc(1, sizeof(UI));
	if (!ret) return NULL;

	ret->octave = 4;
	ret->defvariantlength = 0x7;
	ret->trackerfy = STATE_ROWS;

	return ret;
}
void freeWindow(UI *cw)
{
	if (!cw) return;

	_delInstrument(&cw->instrumentbuffer);

	if (cw->recbuffer) free(cw->recbuffer);
	if (cw->previewsample) free(cw->previewsample);

	for (short i = 0; i < cw->pbtrackc; i++)
	{
		free(cw->pbvariantv[i]);
		free(cw->vbtrig[i]);
	}

	freeWaveform();

	free(cw);
}
