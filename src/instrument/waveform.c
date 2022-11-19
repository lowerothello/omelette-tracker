void instrumentSamplerControlCallback(void *arg)
{
	w->waveformdrawpointer = 0;
}

void resetWaveform(void)
{
	if (instrumentSafe(s, w->instrument))
	{
		w->waveformdrawpointer = 0;
		p->redraw = 1;
	}
}

void resizeWaveform(void)
{
	w->waveformw = (ws.ws_col - INSTRUMENT_INDEX_COLS +1)<<1;
	w->waveformh = (ws.ws_row - TRACK_ROW - 13)<<2;

	if (w->waveformcanvas) { free_canvas(w->waveformcanvas); w->waveformcanvas = NULL; }
	if (w->waveformbuffer) { free_buffer(w->waveformbuffer); w->waveformbuffer = NULL; }

	if (w->waveformw > 0 && w->waveformh > 0)
		w->waveformcanvas = new_canvas(w->waveformw, w->waveformh);

	if (w->waveformcanvas)
		w->waveformbuffer = new_buffer(w->waveformcanvas);

	w->waveformdrawpointer = 0;
}
void freeWaveform(void)
{
	if (w->waveformcanvas) free_canvas(w->waveformcanvas);
	if (w->waveformbuffer) free_buffer(w->waveformbuffer);
}

void drawMarker(uint32_t marker, size_t offset, size_t width)
{
	size_t xpos;
	if (marker >= offset && marker < offset + width)
	{
		xpos = (float)(marker - offset) / (float)width * w->waveformw;
		for (size_t i = 0; i < w->waveformh; i++) set_pixel(w->waveformcanvas, i%2, xpos, i);
	}
}

/* arg should be iv */
void *walkWaveformRoutine(void *arg)
{
	Instrument *iv = arg;
	size_t offset, width;
	if (iv->algorithm == INST_ALG_WAVETABLE)
	{
		offset = iv->trimstart + (MIN((iv->sample->length - iv->trimstart) / MAX(1, iv->wavetable.framelength) - 1, iv->wavetable.wtpos)) * iv->wavetable.framelength;
		width = iv->wavetable.framelength;
	} else
	{
		offset = 0;
		width = iv->sample->length;
	}

	if (w->waveformdrawpointer == 0)
		fill(w->waveformcanvas, 0);

	uint8_t i;
	size_t k, xx;
	uint32_t l;
	double samplesperpixel = (double)width / (double)w->waveformw;
	double divmaxj = 1.0f / (double)width;
	float o = (float)w->waveformh * 0.5f;
	float sample;
	float trackmix = 1.0f / (float)iv->sample->tracks;
	struct timespec req;

	while (w->waveformdrawpointer < width)
	{
		/* switch to left->right rendering if zoomed in far enough */
		if (w->waveformw > width) l =  w->waveformdrawpointer;
		else                      l = (w->waveformdrawpointer%w->waveformw)*samplesperpixel + w->waveformdrawpointer/w->waveformw;

		k = (float)l * divmaxj * (float)width;
		xx = (float)l * divmaxj * (float)w->waveformw;

		sample = 0.0f;
		for (i = 0; i < iv->sample->tracks; i++) /* mix all tracks */
			sample += (iv->sample->data[(offset + k) * iv->sample->tracks + i] * trackmix);
		sample = (sample*DIVSHRT) * o + o;

		set_pixel(w->waveformcanvas, 1, xx, sample);

		w->waveformdrawpointer++;
		p->redraw = 1;

		req.tv_sec  = 0; /* nanosleep can set this higher sometimes, so set every cycle */
		req.tv_nsec = WORK_UPDATE_DELAY;
		while(nanosleep(&req, &req) < 0);
	}
	return NULL;
}

void drawWaveform(Instrument *iv, short x, short y)
{
	if (w->waveformbuffer)
	{
		size_t offset, width;
		if (iv->algorithm == INST_ALG_WAVETABLE)
		{
			offset = iv->trimstart + (MIN((iv->sample->length - iv->trimstart) / MAX(1, iv->wavetable.framelength) - 1, iv->wavetable.wtpos)) * iv->wavetable.framelength;
			width = iv->wavetable.framelength;
		} else
		{
			offset = 0;
			width = iv->sample->length;
		}

		if (w->waveformdrawpointer == 0)
			fill(w->waveformcanvas, 0);

		size_t k, xx;
		uint32_t l;
		float trackmix = 1.0f / (float)iv->sample->tracks;
		double divmaxj = 1.0f / (float)width;
		float o = (float)w->waveformh * 0.5f;
		float sample;
		double samplesperpixel = (double)width / (double)w->waveformw;
		if (w->waveformdrawpointer < width)
		{
			for (uint32_t j = 0; j < WAVEFORM_LAZY_BLOCK_SIZE; j++)
			{
				/* switch to left->right rendering if zoomed in far enough */
				if (w->waveformw > width) l = w->waveformdrawpointer;
				else l = (w->waveformdrawpointer%w->waveformw)*samplesperpixel + w->waveformdrawpointer/w->waveformw;

				k = (float)l * divmaxj * (float)width;
				xx = (float)l * divmaxj * (float)w->waveformw;

				sample = 0.0f;
				for (uint8_t i = 0; i < iv->sample->tracks; i++) /* mix all tracks */
					sample += (iv->sample->data[(offset + k) * iv->sample->tracks + i] * trackmix);
				sample = (sample*DIVSHRT) * o + o;

				set_pixel(w->waveformcanvas, 1, xx, sample);

				w->waveformdrawpointer++;
				if (w->waveformdrawpointer >= width) break;
			}
#ifndef NO_VALGRIND
			if (RUNNING_ON_VALGRIND)
				w->waveformdrawpointer = width;
			else
				p->redraw = 1; /* continue drawing asap */
#else
			p->redraw = 1; /* continue drawing asap */
#endif
		}

		if (iv->algorithm != INST_ALG_WAVETABLE)
		{
			drawMarker(iv->trimstart,                                                                                  offset, width);
			drawMarker(MIN(iv->trimstart + iv->trimlength, iv->sample->length-1),                                      offset, width);
			drawMarker(MAX(iv->trimstart, MIN(iv->trimstart + iv->trimlength, iv->sample->length-1) - iv->looplength), offset, width);
		}

		draw(w->waveformcanvas, w->waveformbuffer);
		for (size_t i = 0; w->waveformbuffer[i] != NULL; i++)
			printf("\033[%ld;%dH%s", TRACK_ROW+1 + i, INSTRUMENT_INDEX_COLS, w->waveformbuffer[i]);
	}
}
