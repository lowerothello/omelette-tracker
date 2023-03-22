pthread_t waveformthread;

Canvas   *waveformworkcanvas;
Canvas   *waveformdrawcanvas;
char    **waveformbuffer;
int       waveformw;
int       waveformh;
Sample   *waveformsample;

bool waveformthreadreap = 0;

static void *walkWaveformRoutine(void *_)
{
	waveformthreadreap = 1;
	size_t offset = 0;

	uint32_t drawpointer = 0;
	fill(waveformworkcanvas, 0);

	uint8_t i;
	size_t xx;
	uint32_t l, m;
	double samplesperpixel = (double)waveformsample->length / (double)waveformw;
	float o = (float)waveformh * 0.5f;
	float sample;

	struct timespec req;
	req.tv_sec = 0;
	req.tv_nsec = 1;

	while (1)
	{
		m = WAVEFORM_BLOCK_SIZE;
		while (m--)
		{
			if (drawpointer > waveformsample->length)
				goto walkWaveformRoutineEnd;

			/* switch to left->right rendering if zoomed in far enough */
			if (waveformw > waveformsample->length)
				l = drawpointer;
			else
				l = (drawpointer%waveformw)*samplesperpixel + drawpointer/waveformw;

			xx = (float)l / (float)waveformsample->length * (float)waveformw;

			sample = 0.0f;
			for (i = 0; i < waveformsample->channels; i++) /* mix all channels */
				sample += (waveformsample->data[(offset + l) * waveformsample->channels + i] / (float)waveformsample->channels);
			sample = (sample*DIVSHRT) * o + o;

			set_pixel(waveformworkcanvas, 1, xx, sample);

			drawpointer++;
		}
		p->redraw = 1;
		nanosleep(&req, NULL);
	}
walkWaveformRoutineEnd:
	p->redraw = 1;
	return NULL;
}

static void stopWaveformThread(void)
{
	if (waveformthreadreap)
	{
		pthread_cancel(waveformthread);
		pthread_join(waveformthread, NULL);
		waveformsample = NULL;
		waveformthreadreap = 0;
	}
}

static void resetWaveform(Sample *sample)
{
	stopWaveformThread();

	if (!waveformworkcanvas) return;

	if (sample)
	{
		waveformsample = sample; /* TODO: will probably lead to memory errors */
		pthread_create(&waveformthread, NULL, (void*(*)(void*))walkWaveformRoutine, NULL);
	}
	p->redraw = 1;
}

void freeWaveform(void)
{
	stopWaveformThread();
	waveformw = 0;
	waveformh = 0;
	if (waveformworkcanvas) { free_canvas(waveformworkcanvas); waveformworkcanvas = NULL; }
	if (waveformdrawcanvas) { free_canvas(waveformdrawcanvas); waveformdrawcanvas = NULL; }
	if (waveformbuffer) { free_buffer(waveformbuffer); waveformbuffer = NULL; }
}

static void resizeWaveform(Sample *sample, short w, short h)
{
	freeWaveform();

	waveformw = w<<1;
	waveformh = h<<2;

	if (w <= 0 || h <= 0) return;

	waveformworkcanvas = new_canvas(waveformw, waveformh);
	waveformdrawcanvas = new_canvas(waveformw, waveformh);
	waveformbuffer = new_buffer(waveformdrawcanvas);

	resetWaveform(sample);
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
void drawWaveform(Sample *sample, short x, short y, short width, short height)
{
	if (waveformw != width<<1 || waveformh != height<<2) /* new size */
		resizeWaveform(sample, width, height);
	else if (sample != waveformsample)
		resetWaveform(sample);

	if (!waveformworkcanvas) return;

	// offset = 0;
	// width = sample->length;
	if (!sample) return;

	memcpy(waveformdrawcanvas->canvas, waveformworkcanvas->canvas, waveformw * waveformh);
	drawMarker(sample->trimstart,                                                                                          0, sample->length);
	drawMarker(MIN(sample->trimstart + sample->trimlength, sample->length-1),                                              0, sample->length);
	drawMarker(MAX(sample->trimstart, MIN(sample->trimstart + sample->trimlength, sample->length-1) - sample->looplength), 0, sample->length);
	draw(waveformdrawcanvas, waveformbuffer);
	for (size_t i = 0; waveformbuffer[i] != NULL; i++)
		printf("\033[%ld;%dH%s", y + i, x, waveformbuffer[i]);
}
