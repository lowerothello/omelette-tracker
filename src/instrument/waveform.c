pthread_t waveformthread;
Canvas   *waveformworkcanvas;
Canvas   *waveformdrawcanvas;
char    **waveformbuffer;
int       waveformw;
int       waveformh;

bool waveformthreadrunning = 0;

static void *walkWaveformRoutine(Instrument *iv)
{
	waveformthreadrunning = 1;
	size_t offset, width;
	offset = 0;
	width = iv->sample->length;

	uint32_t drawpointer = 0;
	fill(waveformworkcanvas, 0);

	uint8_t i;
	size_t k, xx;
	uint32_t l, m;
	double samplesperpixel = (double)width / (double)waveformw;
	double divmaxj = 1.0f / (double)width;
	float o = (float)waveformh * 0.5f;
	float sample;
	float trackmix = 1.0f / (float)iv->sample->channels;
	struct timespec req;

	while (drawpointer < width)
	{
		for (m = 0; m < WORK_BLOCK_SIZE; m++)
		{
			/* switch to left->right rendering if zoomed in far enough */
			if (waveformw > width) l =  drawpointer;
			else                      l = (drawpointer%waveformw)*samplesperpixel + drawpointer/waveformw;

			k = (float)l * divmaxj * (float)width;
			xx = (float)l * divmaxj * (float)waveformw;

			sample = 0.0f;
			for (i = 0; i < iv->sample->channels; i++) /* mix all channels */
				sample += (iv->sample->data[(offset + k) * iv->sample->channels + i] * trackmix);
			sample = (sample*DIVSHRT) * o + o;

			set_pixel(waveformworkcanvas, 1, xx, sample);

			drawpointer++;
		}
		p->redraw = 1;

		req.tv_sec  = 0; /* nanosleep can set this higher sometimes, so set every cycle */
		req.tv_nsec = WORK_UPDATE_DELAY;
		while(nanosleep(&req, &req) < 0);
	}
	waveformthreadrunning = 0;
	p->redraw = 1;
	return NULL;
}

static void stopWaveformThread(void)
{
	if (waveformthreadrunning)
	{
		pthread_cancel(waveformthread);
		pthread_join(waveformthread, NULL);
		waveformthreadrunning = 0;
	}
}

void resetWaveform(void)
{
	if (!waveformworkcanvas) return;

	if (instrumentSafe(s->instrument, w->instrument))
	{
		stopWaveformThread();
		pthread_create(&waveformthread, NULL, (void*(*)(void*))walkWaveformRoutine, &s->instrument->v[w->instrument]);
	}
}

void freeWaveform(void)
{
	stopWaveformThread();
	if (waveformworkcanvas) { free_canvas(waveformworkcanvas); waveformworkcanvas = NULL; }
	if (waveformdrawcanvas) { free_canvas(waveformdrawcanvas); waveformdrawcanvas = NULL; }
	if (waveformbuffer) { free_buffer(waveformbuffer); waveformbuffer = NULL; }
}

static void resizeWaveform(short w, short h)
{
	freeWaveform();

	waveformw = w<<1;
	waveformh = h<<2;

	if (w <= 0 || h <= 0) return;

	waveformworkcanvas = new_canvas(waveformw, waveformh);
	waveformdrawcanvas = new_canvas(waveformw, waveformh);
	waveformbuffer = new_buffer(waveformdrawcanvas);

	resetWaveform();
}

static void drawMarker(uint32_t marker, size_t offset, size_t width)
{
	size_t xpos;
	if (marker >= offset && marker < offset + width)
	{
		xpos = (float)(marker - offset) / (float)width * waveformw;
		for (size_t i = 0; i < waveformh; i++) set_pixel(waveformdrawcanvas, i%2, xpos, i);
	}
}

/* height in cells */
void drawWaveform(Instrument *iv, short h)
{
	short w = (ws.ws_col - INSTRUMENT_INDEX_COLS + 1);
	if (waveformw != w<<1 || waveformh != h<<2) /* new size */
		resizeWaveform(w, h);

	if (!waveformworkcanvas) return;

	// offset = 0;
	// width = iv->sample->length;
	memcpy(waveformdrawcanvas->canvas, waveformworkcanvas->canvas, waveformw * waveformh);
	drawMarker(iv->trimstart,                                                                                  0, iv->sample->length);
	drawMarker(MIN(iv->trimstart + iv->trimlength, iv->sample->length-1),                                      0, iv->sample->length);
	drawMarker(MAX(iv->trimstart, MIN(iv->trimstart + iv->trimlength, iv->sample->length-1) - iv->looplength), 0, iv->sample->length);
	draw(waveformdrawcanvas, waveformbuffer);
	for (size_t i = 0; waveformbuffer[i] != NULL; i++)
		printf("\033[%ld;%dH%s", TRACK_ROW+1 + i, INSTRUMENT_INDEX_COLS, waveformbuffer[i]);
}
