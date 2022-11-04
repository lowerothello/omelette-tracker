void drawMarker(uint32_t marker, size_t offset, size_t width)
{
	size_t xpos;
	if (marker >= offset && marker < offset + width)
	{
		xpos = (float)(marker - offset) / (float)width * w->waveformw;
		for (size_t i = 0; i < w->waveformh; i++) set_pixel(w->waveformcanvas, i%2, xpos, i);
	}
}

void instrumentSamplerControlCallback(void *casttocc)
{ w->waveformdrawpointer = 0; }

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
	float channelmix = 1.0f / (float)iv->sample->channels;
	struct timespec req;

	while (w->waveformdrawpointer < width)
	{
		/* switch to left->right rendering if zoomed in far enough */
		if (w->waveformw > width) l =  w->waveformdrawpointer;
		else                      l = (w->waveformdrawpointer%w->waveformw)*samplesperpixel + w->waveformdrawpointer/w->waveformw;

		k = (float)l * divmaxj * (float)width;
		xx = (float)l * divmaxj * (float)w->waveformw;

		sample = 0.0f;
		for (i = 0; i < iv->sample->channels; i++) /* mix all channels */
			sample += (iv->sample->data[(offset + k) * iv->sample->channels + i] * channelmix);
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
		float channelmix = 1.0f / (float)iv->sample->channels;
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
				for (uint8_t i = 0; i < iv->sample->channels; i++) /* mix all channels */
					sample += (iv->sample->data[(offset + k) * iv->sample->channels + i] * channelmix);
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
			printf("\033[%ld;%dH%s", CHANNEL_ROW+1 + i, INSTRUMENT_INDEX_COLS, w->waveformbuffer[i]);
	}
}

void drawInstrumentSampler(Instrument *iv)
{
	short x = INSTRUMENT_INDEX_COLS + ((ws.ws_col - INSTRUMENT_INDEX_COLS - 38)>>1) +1;
	short y = ws.ws_row - 12;

	if (iv->algorithm == INST_ALG_MIDI)
	{
		clearControls(&cc);

		x = INSTRUMENT_INDEX_COLS + ((ws.ws_col - INSTRUMENT_INDEX_COLS - 18)>>1) +1;
		printf("\033[%d;%dHMIDI channel:  [ ]", y+0, x);
		printf("\033[%d;%dHMIDI program: [  ]", y+1, x);
		addControlInt(&cc, x+16, y+0, &iv->midi.channel, 1, -1, 15, -1, 0, instrumentSamplerControlCallback, NULL);

		drawControls(&cc);
	} else
	{
		if (w->showfilebrowser)
		{
			drawBrowser(fbstate);
		} else
		{
			clearControls(&cc);

			printf("\033[%d;%dH%ds", CHANNEL_ROW - 1, ws.ws_col - 10, (int)((iv->sample->length * (float)iv->sample->rate/(float)samplerate) / samplerate) + 1);

			drawWaveform(iv, x, y);
			printf("\033[%d;%dHC5 rate: [        ]  channel: [      ]", y+0, x);
			printf("\033[%d;%dHquality: [ ][ ][  ]  gain:    [ ][   ]", y+1, x);
			printf("\033[%d;%dHgain env:    [    ]  [    ]:  [  ][  ]", y+2, x);

			addControlInt(&cc, x+10, y+0, &iv->sample->rate, 8, 0x0, 0xffffffff, iv->sample->defrate, 0, instrumentSamplerControlCallback, NULL);
			addControlInt(&cc, x+10, y+1, &iv->interpolate, 0, 0,   1,      0,      0, instrumentSamplerControlCallback, NULL);
			addControlInt(&cc, x+13, y+1, &iv->bitdepth,    1, 0x0, 0xf,    0xf,    0, instrumentSamplerControlCallback, NULL);
			addControlInt(&cc, x+16, y+1, &iv->samplerate,  2, 0x0, 0xff,   0xff,   0, instrumentSamplerControlCallback, NULL);
			addControlInt(&cc, x+14, y+2, &iv->envelope,    4, 0x0, 0xffff, 0x00f0, 0, instrumentSamplerControlCallback, NULL);
			addControlInt(&cc, x+31, y+0, &iv->channelmode, 1, 0,   4,      0,      6, instrumentSamplerControlCallback, NULL);
				setControlPrettyName(&cc, "STEREO");
				setControlPrettyName(&cc, "  LEFT");
				setControlPrettyName(&cc, " RIGHT");
				setControlPrettyName(&cc, "   MIX");
				setControlPrettyName(&cc, "  SWAP");
			addControlInt(&cc, x+31, y+1, &iv->invert,     0, 0,    1,   0, 0, instrumentSamplerControlCallback, NULL);
			addControlInt(&cc, x+34, y+1, &iv->gain,       3, -128, 127, 0, 0, instrumentSamplerControlCallback, NULL);
			addControlInt(&cc, x+22, y+2, &iv->filtermode, 1, 0,    7,   0, 4, instrumentSamplerControlCallback, NULL);
				setControlPrettyName(&cc, "LP12");
				setControlPrettyName(&cc, "HP12");
				setControlPrettyName(&cc, "BP12");
				setControlPrettyName(&cc, "NT12");
				setControlPrettyName(&cc, "LP24");
				setControlPrettyName(&cc, "HP24");
				setControlPrettyName(&cc, "BP24");
				setControlPrettyName(&cc, "NT24");
			addControlInt(&cc, x+31, y+2, &iv->filtercutoff,    2, 0x0, 0xff, 0xff, 0, instrumentSamplerControlCallback, NULL);
			addControlInt(&cc, x+35, y+2, &iv->filterresonance, 2, 0x0, 0xff, 0x0,  0, instrumentSamplerControlCallback, NULL);

			addControlInt(&cc, x+4, y+4, &iv->algorithm, 1, 0, 4, 1, 10, instrumentSamplerControlCallback, NULL);
				setControlPrettyName(&cc, "   MINIMAL");
				setControlPrettyName(&cc, "    CYCLIC");
				setControlPrettyName(&cc, "     TONAL");
				setControlPrettyName(&cc, "      BEAT");
				setControlPrettyName(&cc, " WAVETABLE");

			switch (iv->algorithm)
			{
				case INST_ALG_SIMPLE:
					printf("\033[%d;%dH + [          ] +                     ", y+4, x);
					printf("\033[%d;%dH                                      ", y+5, x);
					printf("\033[%d;%dH    I'd advise against using this     ", y+6, x);
					printf("\033[%d;%dH    algorithm for the time being,     ", y+7, x);
					printf("\033[%d;%dH     it's unpolished and broken!      ", y+8, x);
					break;
				case INST_ALG_CYCLIC:
					drawWaveform(iv, x, y);
					printf("\033[%d;%dH + [          ] +   offset: [        ]", y+4, x);
					printf("\033[%d;%dHunison:  [ ][ ][ ]  length: [        ]", y+5, x);
					printf("\033[%d;%dHrev/ramp:  [  ][ ]  loop:   [        ]", y+6, x);
					printf("\033[%d;%dHcycle:  [    ][  ]  pp/ramp:   [ ][  ]", y+7, x);
					printf("\033[%d;%dHtime:   [ ][     ]                    ", y+8, x);
					printf("\033[%d;%dH                     -    PITCH     - ", y+9, x);
					printf("\033[%d;%dHpan jitter:   [  ]  root: [   ][     ]", y+10, x);
					printf("\033[%d;%dHptr jitter:   [  ]  jitter: [  ][ ][ ]", y+11, x);

					/* granular */
					addControlInt(&cc, x+12, y+6, &iv->granular.reversegrains,     2, 0x0,    0xff,   0,      0, instrumentSamplerControlCallback, NULL);
					addControlInt(&cc, x+16, y+6, &iv->granular.rampgrains,        1, 0x0,    0xf,    8,      0, instrumentSamplerControlCallback, NULL);
					addControlInt(&cc, x+9,  y+7, &iv->granular.cyclelength,       4, 0x0,    0xffff, 0x3fff, 0, instrumentSamplerControlCallback, NULL);
					addControlInt(&cc, x+15, y+7, &iv->granular.cyclelengthjitter, 2, 0x0,    0xff,   0x0,    0, instrumentSamplerControlCallback, NULL);
					addControlInt(&cc, x+9,  y+8, &iv->granular.notestretch,       0, 0,      1,      0,      0, instrumentSamplerControlCallback, NULL);
					addControlInt(&cc, x+12, y+8, &iv->granular.timestretch,       5, 0x8bff, 0x7bff, 0,      0, instrumentSamplerControlCallback, NULL);

					/* range */
					iv->trimlength = MIN(iv->trimlength, iv->sample->length-1);
					iv->looplength = MIN(iv->looplength, iv->sample->length-1);
					addControlInt(&cc, x+29, y+4, &iv->trimstart,  8, 0x0, iv->sample->length-1,                    0, 0, instrumentSamplerControlCallback, NULL);
					addControlInt(&cc, x+29, y+5, &iv->trimlength, 8, 0x0, iv->sample->length-1, iv->sample->length-1, 0, instrumentSamplerControlCallback, NULL);
					addControlInt(&cc, x+29, y+6, &iv->looplength, 8, 0x0, iv->trimlength,                          0, 0, instrumentSamplerControlCallback, NULL);
					addControlInt(&cc, x+32, y+7, &iv->pingpong,   0, 0,   1,    0, 0, instrumentSamplerControlCallback, NULL);
					addControlInt(&cc, x+35, y+7, &iv->loopramp,   2, 0x0, 0xff, 0, 0, instrumentSamplerControlCallback, NULL);

					/* jitter */
					addControlInt(&cc, x+15, y+10, &iv->granular.panjitter, 2, 0x00, 0xff, 0, 0, instrumentSamplerControlCallback, NULL);
					addControlInt(&cc, x+15, y+11, &iv->granular.panjitter, 2, 0x00, 0xff, 0, 0, instrumentSamplerControlCallback, NULL);

					/* pitch */
					addControlInt(&cc, x+27, y+10, &iv->granular.pitchstereo,       3, -128,   127,    0, 0, instrumentSamplerControlCallback, NULL);
					addControlInt(&cc, x+32, y+10, &iv->granular.pitchshift,        5, 0x8bff, 0x7bff, 0, 0, instrumentSamplerControlCallback, NULL);
					addControlInt(&cc, x+29, y+11, &iv->granular.pitchjitter,       2, 0x00,   0xff,   0, 0, instrumentSamplerControlCallback, NULL);
					addControlInt(&cc, x+33, y+11, &iv->granular.pitchoctaverange,  1, 0x0,    0xf,    0, 0, instrumentSamplerControlCallback, NULL);
					addControlInt(&cc, x+36, y+11, &iv->granular.pitchoctavechance, 1, 0x0,    0xf,    0, 0, instrumentSamplerControlCallback, NULL);
					break;
				case INST_ALG_TONAL:
					drawWaveform(iv, x, y);
					printf("\033[%d;%dH + [          ] +   offset: [        ]", y+4, x);
					printf("\033[%d;%dHunison:  [ ][ ][ ]  length: [        ]", y+5, x);
					printf("\033[%d;%dHrev/ramp:  [  ][ ]  loop:   [        ]", y+6, x);
					printf("\033[%d;%dH                    pp/ramp:   [ ][  ]", y+7, x);
					printf("\033[%d;%dHtime:   [ ][     ]                    ", y+8, x);
					printf("\033[%d;%dHpitch [   ][     ]   -   AUTOTUNE   - ", y+9, x);
					printf("\033[%d;%dHformant:   [     ]  tune: [     ][   ]", y+10, x);
					printf("\033[%d;%dHjitter:   [  ][  ]  spd/mix:  [  ][  ]", y+11, x);

					/* granular */
					addControlInt(&cc, x+12, y+6, &iv->granular.reversegrains, 2, 0x0, 0xff, 0, 0, instrumentSamplerControlCallback, NULL);
					addControlInt(&cc, x+16, y+6, &iv->granular.rampgrains,    1, 0x0, 0xf,  8, 0, instrumentSamplerControlCallback, NULL);

					addControlInt(&cc, x+9,  y+8,  &iv->granular.notestretch,   0, 0,      1,      0, 0, instrumentSamplerControlCallback, NULL);
					addControlInt(&cc, x+12, y+8,  &iv->granular.timestretch,   5, 0x8bff, 0x7bff, 0, 0, instrumentSamplerControlCallback, NULL);
					addControlInt(&cc, x+7,  y+9,  &iv->granular.pitchstereo,   3, -128,   127,    0, 0, instrumentSamplerControlCallback, NULL);
					addControlInt(&cc, x+12, y+9,  &iv->granular.pitchshift,    5, 0x8bff, 0x7bff, 0, 0, instrumentSamplerControlCallback, NULL);
					addControlInt(&cc, x+12, y+10, &iv->granular.formantshift,  5, 0x8bff, 0x7bff, 0, 0, instrumentSamplerControlCallback, NULL);
					addControlInt(&cc, x+11, y+11, &iv->granular.pitchjitter,   2, 0x00,   0xff,   0, 0, instrumentSamplerControlCallback, NULL);
					addControlInt(&cc, x+15, y+11, &iv->granular.formantjitter, 2, 0x00,   0xff,   0, 0, instrumentSamplerControlCallback, NULL);

					/* range */
					iv->trimlength = MIN(iv->trimlength, iv->sample->length-1);
					iv->looplength = MIN(iv->looplength, iv->sample->length-1);
					addControlInt(&cc, x+29, y+4, &iv->trimstart,  8, 0x0, iv->sample->length-1,                    0, 0, instrumentSamplerControlCallback, NULL);
					addControlInt(&cc, x+29, y+5, &iv->trimlength, 8, 0x0, iv->sample->length-1, iv->sample->length-1, 0, instrumentSamplerControlCallback, NULL);
					addControlInt(&cc, x+29, y+6, &iv->looplength, 8, 0x0, iv->trimlength,                          0, 0, instrumentSamplerControlCallback, NULL);
					addControlInt(&cc, x+32, y+7, &iv->pingpong,   0, 0,   1,    0, 0, instrumentSamplerControlCallback, NULL);
					addControlInt(&cc, x+35, y+7, &iv->loopramp,   2, 0x0, 0xff, 0, 0, instrumentSamplerControlCallback, NULL);

					addControlInt(&cc, x+27, y+10, &iv->granular.autotune,  5, 0x8bff, 0x7bff, 0, 0, instrumentSamplerControlCallback, NULL);
					addControlInt(&cc, x+34, y+10, &iv->granular.autoscale, 1, 0,      2,      0, 3, instrumentSamplerControlCallback, NULL);
						setControlPrettyName(&cc, "MAJ");
						setControlPrettyName(&cc, "MIN");
						setControlPrettyName(&cc, "CHR");
					addControlInt(&cc, x+31, y+11, &iv->granular.autospeed,    2, 0x00, 0xff, 0, 0, instrumentSamplerControlCallback, NULL);
					addControlInt(&cc, x+35, y+11, &iv->granular.autostrength, 2, 0x00, 0xff, 0, 0, instrumentSamplerControlCallback, NULL);
					break;
				case INST_ALG_BEAT:
					drawWaveform(iv, x, y);
					printf("\033[%d;%dH + [          ] +   offset: [        ]", y+4, x);
					printf("\033[%d;%dHsensitivity:  [  ]  length: [        ]", y+5, x);
					printf("\033[%d;%dHdecay:        [  ]  loop:   [        ]", y+6, x);
					printf("\033[%d;%dH                    pp/ramp:   [ ][  ]", y+7, x);
					printf("\033[%d;%dHtime:   [ ][     ]                    ", y+8, x);
					printf("\033[%d;%dHpitch [   ][     ]                    ", y+9, x);

					/* granular */
					addControlInt(&cc, x+15, y+5, &iv->granular.beatsensitivity, 2, 0x0, 0xff, 0x80, 0, instrumentSamplerControlCallback, NULL);
					addControlInt(&cc, x+15, y+6, &iv->granular.beatdecay,       2, 0x0, 0xff, 0xff, 0, instrumentSamplerControlCallback, NULL);

					addControlInt(&cc, x+9,  y+8, &iv->granular.notestretch, 0, 0,      1,      0, 0, instrumentSamplerControlCallback, NULL);
					addControlInt(&cc, x+12, y+8, &iv->granular.timestretch, 5, 0x8bff, 0x7bff, 0, 0, instrumentSamplerControlCallback, NULL);
					addControlInt(&cc, x+7,  y+9, &iv->granular.pitchstereo, 3, -128,   127,    0, 0, instrumentSamplerControlCallback, NULL);
					addControlInt(&cc, x+12, y+9, &iv->granular.pitchshift,  5, 0x8bff, 0x7bff, 0, 0, instrumentSamplerControlCallback, NULL);

					/* range */
					iv->trimlength = MIN(iv->trimlength, iv->sample->length-1);
					iv->looplength = MIN(iv->looplength, iv->sample->length-1);
					addControlInt(&cc, x+29, y+4, &iv->trimstart,  8, 0x0, iv->sample->length-1,                    0, 0, instrumentSamplerControlCallback, NULL);
					addControlInt(&cc, x+29, y+5, &iv->trimlength, 8, 0x0, iv->sample->length-1, iv->sample->length-1, 0, instrumentSamplerControlCallback, NULL);
					addControlInt(&cc, x+29, y+6, &iv->looplength, 8, 0x0, iv->trimlength,                          0, 0, instrumentSamplerControlCallback, NULL);
					addControlInt(&cc, x+32, y+7, &iv->pingpong,   0, 0,   1,    0, 0, instrumentSamplerControlCallback, NULL);
					addControlInt(&cc, x+35, y+7, &iv->loopramp,   2, 0x0, 0xff, 0, 0, instrumentSamplerControlCallback, NULL);
					break;
				case INST_ALG_WAVETABLE:
					drawWaveform(iv, x, y);
					printf("\033[%d;%dH + [          ] +   [                ]", y+4,  x);
					printf("\033[%d;%dHunison:  [ ][ ][ ]                    ", y+5,  x);
					printf("\033[%d;%dHoffset: [        ]                    ", y+6,  x);
					printf("\033[%d;%dHframe:  [        ]                    ", y+7,  x);
					printf("\033[%d;%dH                                      ", y+8,  x);
					printf("\033[%d;%dHenvelope:   [    ]                    ", y+9,  x);
					printf("\033[%d;%dHlfo speed:    [  ]                    ", y+10, x);
					printf("\033[%d;%dHlfo shape [   ][ ]                    ", y+11, x);

					iv->wavetable.framelength = MIN(iv->wavetable.framelength, iv->sample->length-1);
					addControlInt(&cc, x+9,  y+6,  &iv->trimstart, 8, 0x0, iv->sample->length-1, 0, 0, instrumentSamplerControlCallback, NULL);
					addControlInt(&cc, x+9,  y+7,  &iv->wavetable.framelength, 8, 0x0, iv->sample->length-1, iv->sample->length-1, 0, instrumentSamplerControlCallback, NULL);

					addControlInt(&cc, x+13, y+9,  &iv->wavetable.envelope, 4, 0x0,  0xffff, 0x00f0, 0, instrumentSamplerControlCallback, NULL);
					addControlInt(&cc, x+15, y+10, &iv->wavetable.lfospeed, 2, 0x0,  0xff,   0,      0, instrumentSamplerControlCallback, NULL);
					addControlInt(&cc, x+11, y+11, &iv->wavetable.lfoduty,  3, -128, 127,    0,      0, instrumentSamplerControlCallback, NULL);
					addControlInt(&cc, x+16, y+11, &iv->wavetable.lfoshape, 0, 0,    1,      0,      0, instrumentSamplerControlCallback, NULL);

					addControlInt(&cc, x+21, y+4, &w->wtparam, 1, 0, 8, 0, 16, instrumentSamplerControlCallback, NULL);
						setControlPrettyName(&cc, "   WAVETABLE POS");
						setControlPrettyName(&cc, "            GAIN");
						setControlPrettyName(&cc, " OSCILLATOR SYNC");
						setControlPrettyName(&cc, "          FILTER");
						setControlPrettyName(&cc, "PHASE MODULATION");
						setControlPrettyName(&cc, "FREQ. MODULATION");
						setControlPrettyName(&cc, "     PULSE WIDTH");
						setControlPrettyName(&cc, "  PHASE DYNAMICS");
						setControlPrettyName(&cc, "           MIXER");

					/* switch (w->wtparam)
					{
						case WT_PARAM_WTPOS:
							printf("\033[%d;%dHoffset:      [   ]", y+5,  x+21);
							printf("\033[%d;%dHlfo mod:     [   ]", y+6,  x+21);
							printf("\033[%d;%dHenv mod:     [   ]", y+7,  x+21);
							printf("\033[%d;%dHgainenv mod: [   ]", y+8,  x+21);
							printf("\033[%d;%dH                  ", y+9,  x+21);
							printf("\033[%d;%dH                  ", y+10, x+21);
							printf("\033[%d;%dH                  ", y+11, x+21);

							addControlInt(&cc, x+35, y+5, &iv->wavetable.wtpos.offset,  3, -128, 127, 0, 0, instrumentSamplerControlCallback, NULL);
							addControlInt(&cc, x+35, y+6, &iv->wavetable.wtpos.lfo,     3, -128, 127, 0, 0, instrumentSamplerControlCallback, NULL);
							addControlInt(&cc, x+35, y+7, &iv->wavetable.wtpos.env,     3, -128, 127, 0, 0, instrumentSamplerControlCallback, NULL);
							addControlInt(&cc, x+35, y+8, &iv->wavetable.wtpos.gainenv, 3, -128, 127, 0, 0, instrumentSamplerControlCallback, NULL);
							break;
					} */
					break;
			}

			drawControls(&cc);
		}
	}
}
