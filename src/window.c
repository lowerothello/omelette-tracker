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
	if (input_api.autorepeaton)
		input_api.autorepeaton();

	w->page = PAGE_VARIANT;
	w->mode = MODE_NORMAL;
	freePreviewSample();
	p->redraw = 1;
}
void showInstrument(void)
{
	if (input_api.autorepeaton)
		input_api.autorepeaton();

	w->page = PAGE_INSTRUMENT;
	w->mode = MODE_NORMAL;

	freePreviewSample();
	p->redraw = 1;
}

UI *allocWindow(void)
{
	UI *ret = calloc(1, sizeof(UI));
	if (!ret) return NULL;

	ret->octave = DEF_OCTAVE;
	ret->step = DEF_STEP;

	ret->defvariantlength = 0x7;
	ret->trackerfy = STATE_ROWS;
	ret->playfy = STATE_ROWS;

	// __addInstrument(&ret->instbuffer, INST_TYPE_NULL);
	for (int i = 0; i < PREVIEW_TRACKS; i++)
		ret->previewtrack[i] = allocTrack(NULL, NULL);

	ret->trackbuffer.effect = newEffectChain();
	initTrackData(&ret->trackbuffer, 0);

	return ret;
}
void freeWindow(UI *cw)
{
	if (!cw) return;

	for (int i = 0; i < PREVIEW_TRACKS; i++)
	{
		_delTrack(NULL, cw->previewtrack[i]);
		free(cw->previewtrack[i]);
	}

	// clearTrackData(&cw->trackbuffer, 1);
	_delTrack(NULL, &cw->trackbuffer);
	const InstAPI *api;
	if ((api = instGetAPI(cw->instbuffer.type))) api->free(&cw->instbuffer);
	freeEffect(&cw->effectbuffer);

	if (cw->recbuffer) free(cw->recbuffer);
	if (cw->previewsample) free(cw->previewsample);

	for (short i = 0; i < cw->pbtrackc; i++)
	{
		free(cw->pbvariantv[i]);
		free(cw->vbtrig[i]);
	}

	if (cw->bpmcache) free(cw->bpmcache);

	free(cw);
}
