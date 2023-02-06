void setBpmCount(void)
{
	s->songbpm = MIN(255, MAX(32, w->count));
	reapplyBpm();
	p->redraw = 1;
}
void setRowHighlightCount(void)
{
	s->rowhighlight = MIN(16, w->count);
	regenGlobalRowc(s);
	p->redraw = 1;
}
void setOctaveCount(void)
{
	w->octave = MIN(w->count, MAX_OCTAVE);
	p->redraw = 1;
}
void setStepCount(void)
{
	w->step = w->count;
	p->redraw = 1;
}

void addOctave(int delta)
{
	w->octave += delta;
	w->octave = MIN(w->octave, MAX_OCTAVE);
	w->octave = MAX(w->octave, MIN_OCTAVE);
	p->redraw = 1;
}
void addStep(int delta)
{
	w->step += delta;
	w->step = MIN(w->step, MAX_STEP);
	w->step = MAX(w->step, MIN_STEP);
	p->redraw = 1;
}

void showTracker(void)
{
	setAutoRepeatOn();
	w->page = PAGE_VARIANT;
	w->mode = MODE_NORMAL;
	freePreviewSample();
	p->redraw = 1;
}
void showInstrument(void)
{
	setAutoRepeatOn();
	w->showfilebrowser = 0;
	w->page = PAGE_INSTRUMENT;
	w->mode = MODE_NORMAL;

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

	__addInstrument(&ret->instrumentbuffer, INST_ALG_NULL);
	for (int i = 0; i < PREVIEW_TRACKS; i++)
		addTrackRuntime(&ret->previewtrack[i]);

	// addTrackData(&ret->trackbuffer, 1);
	ret->trackbuffer.effect = newEffectChain(NULL, NULL);
	initTrackData(&ret->trackbuffer, 0);
	

	return ret;
}
void freeWindow(UI *cw)
{
	if (!cw) return;

	clearTrackData(&cw->trackbuffer, 1);
	_delInstrument(&cw->instrumentbuffer);
	freeEffect(&cw->effectbuffer);

	if (cw->recbuffer) free(cw->recbuffer);
	if (cw->previewsample) free(cw->previewsample);

	for (short i = 0; i < cw->pbtrackc; i++)
	{
		free(cw->pbvariantv[i]);
		free(cw->vbtrig[i]);
	}

	free(cw);
}
