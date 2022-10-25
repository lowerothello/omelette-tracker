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
			} p->redraw = 1; /* continue drawing asap */
		}

		if (iv->algorithm != INST_ALG_WAVETABLE)
		{
			drawMarker(iv->trimstart,                                                                                  offset, width);
			drawMarker(MIN(iv->trimstart + iv->trimlength, iv->sample->length-1),                                      offset, width);
			drawMarker(MAX(iv->trimstart, MIN(iv->trimstart + iv->trimlength, iv->sample->length-1) - iv->looplength), offset, width);
		}

		draw(w->waveformcanvas, w->waveformbuffer);
		for (size_t i = 0; w->waveformbuffer[i] != NULL; i++)
			printf("\033[%ld;%dH%s", CHANNEL_ROW + i, INSTRUMENT_INDEX_COLS, w->waveformbuffer[i]);
	}
}

void drawInstrumentSampler(Instrument *iv)
{
	short x = INSTRUMENT_INDEX_COLS + ((ws.ws_col - INSTRUMENT_INDEX_COLS - 38)>>1) +1;
	short y = ws.ws_row - 12;

	if (iv->algorithm == INST_ALG_MIDI)
	{
		x = INSTRUMENT_INDEX_COLS + ((ws.ws_col - INSTRUMENT_INDEX_COLS - 18)>>1) +1;
		printf("\033[%d;%dHMIDI channel:  [ ]", y+0, x);
		printf("\033[%d;%dHMIDI program: [  ]", y+1, x);
		addControl(&cc, x+16, y+0, &iv->midi.channel, 1, 0x0, 0xf, -1, 0, instrumentSamplerControlCallback, NULL);
	} else
	{
		if (!iv->sample->length)
		{
			w->dirx = INSTRUMENT_INDEX_COLS + 2;
			w->diry = CHANNEL_ROW;
			w->dirw = ws.ws_col - INSTRUMENT_INDEX_COLS;
			w->dirh = ws.ws_row - CHANNEL_ROW - 1;
			changeDirectory();
			w->filebrowserCallback = &sampleLoadCallback;
			drawFilebrowser();
		} else
		{
			clearControls(&cc);

			printf("\033[%d;%dH%ds", CHANNEL_ROW - 1, ws.ws_col - 10, (int)((iv->sample->length * (float)iv->sample->rate/(float)samplerate) / samplerate) + 1);

			drawWaveform(iv, x, y);
			printf("\033[%d;%dHC5 rate: [        ]  channel: [      ]", y+0, x);
			printf("\033[%d;%dHquality: [ ][ ][  ]  gain:    [ ][   ]", y+1, x);
			printf("\033[%d;%dHgain env:    [    ]  [    ]:  [  ][  ]", y+2, x);

			addControl(&cc, x+10, y+0, &iv->sample->rate, 8, 0x0, 0xffffffff, iv->sample->defrate, 0, instrumentSamplerControlCallback, NULL);
			addControl(&cc, x+10, y+1, &iv->interpolate, 0, 0,   1,      0,      0, instrumentSamplerControlCallback, NULL);
			addControl(&cc, x+13, y+1, &iv->bitdepth,    1, 0x0, 0xf,    0xf,    0, instrumentSamplerControlCallback, NULL);
			addControl(&cc, x+16, y+1, &iv->samplerate,  2, 0x0, 0xff,   0xff,   0, instrumentSamplerControlCallback, NULL);
			addControl(&cc, x+14, y+2, &iv->envelope,    4, 0x0, 0xffff, 0x00f0, 0, instrumentSamplerControlCallback, NULL);
			addControl(&cc, x+31, y+0, &iv->channelmode, 1, 0,   4,      0,      6, instrumentSamplerControlCallback, NULL);
				setControlPrettyName(&cc, "STEREO");
				setControlPrettyName(&cc, "  LEFT");
				setControlPrettyName(&cc, " RIGHT");
				setControlPrettyName(&cc, "   MIX");
				setControlPrettyName(&cc, "  SWAP");
			addControl(&cc, x+31, y+1, &iv->invert,     0, 0,    1,   0, 0, instrumentSamplerControlCallback, NULL);
			addControl(&cc, x+34, y+1, &iv->gain,       3, -128, 127, 0, 0, instrumentSamplerControlCallback, NULL);
			addControl(&cc, x+22, y+2, &iv->filtermode, 1, 0,    7,   0, 4, instrumentSamplerControlCallback, NULL);
				setControlPrettyName(&cc, "LP12");
				setControlPrettyName(&cc, "HP12");
				setControlPrettyName(&cc, "BP12");
				setControlPrettyName(&cc, "NT12");
				setControlPrettyName(&cc, "LP24");
				setControlPrettyName(&cc, "HP24");
				setControlPrettyName(&cc, "BP24");
				setControlPrettyName(&cc, "NT24");
			addControl(&cc, x+31, y+2, &iv->filtercutoff,    2, 0x0, 0xff, 0xff, 0, instrumentSamplerControlCallback, NULL);
			addControl(&cc, x+35, y+2, &iv->filterresonance, 2, 0x0, 0xff, 0x0,  0, instrumentSamplerControlCallback, NULL);

			addControl(&cc, x+4, y+4, &iv->algorithm, 1, 0, 2, 0, 10, instrumentSamplerControlCallback, NULL);
				setControlPrettyName(&cc, "   MINIMAL");
				setControlPrettyName(&cc, "  GRANULAR");
				setControlPrettyName(&cc, " WAVETABLE");

			switch (iv->algorithm)
			{
				case INST_ALG_SIMPLE:
					printf("\033[%d;%dH + [          ] + ", y+4, x);
					break;
				case INST_ALG_GRANULAR:
					drawWaveform(iv, x, y);
					printf("\033[%d;%dH + [          ] +   offset: [        ]", y+4, x);
					printf("\033[%d;%dH     [      ]       length: [        ]", y+5, x);
					printf("\033[%d;%dHgrain size: [    ]  loop:   [        ]", y+6, x);
					printf("\033[%d;%dHgrain reverse: [ ]  pingpong loop: [ ]", y+7, x);
					printf("\033[%d;%dHwindow ramp:   [ ]  loop ramping: [  ]", y+8, x);

					printf("\033[%d;%dHtimestretch / notestretch:  [     ][ ]", y+10, x);
					printf("\033[%d;%dHpitchshift / pitchwidth:  [     ][   ]", y+11, x);

					/* granular */
					addControl(&cc, x+13, y+6, &iv->granular.cyclelength,   4, 0x0, 0xffff, 0x3fff, 0, instrumentSamplerControlCallback, NULL);
					addControl(&cc, x+16, y+7, &iv->granular.reversegrains, 0, 0,   1,      0,      0, instrumentSamplerControlCallback, NULL);
					addControl(&cc, x+16, y+8, &iv->granular.rampgrains,    1, 0x0, 0xf,    8,      0, instrumentSamplerControlCallback, NULL);

					/* range */
					iv->trimlength = MIN(iv->trimlength, iv->sample->length-1);
					iv->looplength = MIN(iv->looplength, iv->sample->length-1);
					addControl(&cc, x+29, y+4, &iv->trimstart,  8, 0x0, iv->sample->length-1,                    0, 0, instrumentSamplerControlCallback, NULL);
					addControl(&cc, x+29, y+5, &iv->trimlength, 8, 0x0, iv->sample->length-1, iv->sample->length-1, 0, instrumentSamplerControlCallback, NULL);
					addControl(&cc, x+29, y+6, &iv->looplength, 8, 0x0, iv->trimlength,                          0, 0, instrumentSamplerControlCallback, NULL);
					addControl(&cc, x+36, y+7, &iv->pingpong,   0, 0,   1,    0, 0, instrumentSamplerControlCallback, NULL);
					addControl(&cc, x+35, y+8, &iv->loopramp,   2, 0x0, 0xff, 0, 0, instrumentSamplerControlCallback, NULL);

					/* stritch strotch */
					addControl(&cc, x+29, y+10, &iv->granular.timestretch, 5, 0x8bff, 0x7bff, 0, 0, instrumentSamplerControlCallback, NULL);
					addControl(&cc, x+36, y+10, &iv->granular.notestretch, 0, 0,      1,      0, 0, instrumentSamplerControlCallback, NULL);
					addControl(&cc, x+27, y+11, &iv->granular.pitchshift,  5, 0x8bff, 0x7bff, 0, 0, instrumentSamplerControlCallback, NULL);
					addControl(&cc, x+34, y+11, &iv->granular.pitchstereo, 3, -128,   127,    0, 0, instrumentSamplerControlCallback, NULL);
					break;
				case INST_ALG_WAVETABLE:
					drawWaveform(iv, x, y);
					printf("\033[%d;%dH + [          ] +   - MOD - <env><lfo>", y+4,  x);
					printf("\033[%d;%dHoffset: [        ]  gain:         [  ]", y+5,  x);
					printf("\033[%d;%dHframe:  [        ]  wt pos: [   ][   ]", y+6,  x);
					printf("\033[%d;%dHwt pos:       [  ]  sync:   [   ][   ]", y+7,  x);
					printf("\033[%d;%dHsync/pw [   ][   ]  filter: [   ][   ]", y+8,  x);
					printf("\033[%d;%dHphase dynmcs [   ]  phase:  [   ][   ]", y+9,  x);
					printf("\033[%d;%dHenvelope:   [    ]  pwm:    [   ][   ]", y+10, x);
					printf("\033[%d;%dHlfo:  [  ][   ][ ]  pdyn:   [   ][   ]", y+11, x);

					iv->wavetable.framelength = MIN(iv->wavetable.framelength, iv->sample->length-1);
					addControl(&cc, x+9,  y+5,  &iv->trimstart, 8, 0x0, iv->sample->length-1, 0, 0, instrumentSamplerControlCallback, NULL);
					addControl(&cc, x+9,  y+6,  &iv->wavetable.framelength, 8, 0x0, iv->sample->length-1, iv->sample->length-1, 0, instrumentSamplerControlCallback, NULL);
					addControl(&cc, x+15, y+7,  &iv->wavetable.wtpos,         2, 0x0, 0xff,  0, 0, instrumentSamplerControlCallback, NULL);
					addControl(&cc, x+9,  y+8,  &iv->wavetable.syncoffset,    3, -128, 127,  0, 0, instrumentSamplerControlCallback, NULL);
					addControl(&cc, x+14, y+8,  &iv->wavetable.pulsewidth,    3, -128, 127,  0, 0, instrumentSamplerControlCallback, NULL);
					addControl(&cc, x+14, y+9,  &iv->wavetable.phasedynamics, 3, -128, 127,  0, 0, instrumentSamplerControlCallback, NULL);
					addControl(&cc, x+13, y+10, &iv->wavetable.envelope, 4, 0x0, 0xffff, 0x00f0, 0, instrumentSamplerControlCallback, NULL);
					addControl(&cc, x+7,  y+11, &iv->wavetable.lfospeed,      2, 0x0,  0xff, 0, 0, instrumentSamplerControlCallback, NULL);
					addControl(&cc, x+11, y+11, &iv->wavetable.lfoduty,       3, -128, 127,  0, 0, instrumentSamplerControlCallback, NULL);
					addControl(&cc, x+16, y+11, &iv->wavetable.lfoshape,      0, 0,    0,    0, 0, instrumentSamplerControlCallback, NULL);

					addControl(&cc, x+35, y+5, &iv->wavetable.lfo.gain, 2, 0x0, 0xff, 0, 0, instrumentSamplerControlCallback, NULL);
					addControl(&cc, x+29, y+6, &iv->wavetable.env.wtpos,  3, -128, 127, 0, 0, instrumentSamplerControlCallback, NULL);
					addControl(&cc, x+34, y+6, &iv->wavetable.lfo.wtpos,  3, -128, 127, 0, 0, instrumentSamplerControlCallback, NULL);
					addControl(&cc, x+29, y+7, &iv->wavetable.env.sync,   3, -128, 127, 0, 0, instrumentSamplerControlCallback, NULL);
					addControl(&cc, x+34, y+7, &iv->wavetable.lfo.sync,   3, -128, 127, 0, 0, instrumentSamplerControlCallback, NULL);
					addControl(&cc, x+29, y+8, &iv->wavetable.env.cutoff, 3, -128, 127, 0, 0, instrumentSamplerControlCallback, NULL);
					addControl(&cc, x+34, y+8, &iv->wavetable.lfo.cutoff, 3, -128, 127, 0, 0, instrumentSamplerControlCallback, NULL);
					addControl(&cc, x+29, y+9, &iv->wavetable.env.phase,  3, -128, 127, 0, 0, instrumentSamplerControlCallback, NULL);
					addControl(&cc, x+34, y+9, &iv->wavetable.lfo.phase,  3, -128, 127, 0, 0, instrumentSamplerControlCallback, NULL);
					addControl(&cc, x+29, y+10, &iv->wavetable.env.pwm,   3, -128, 127, 0, 0, instrumentSamplerControlCallback, NULL);
					addControl(&cc, x+34, y+10, &iv->wavetable.lfo.pwm,   3, -128, 127, 0, 0, instrumentSamplerControlCallback, NULL);
					addControl(&cc, x+29, y+11, &iv->wavetable.env.pdyn,  3, -128, 127, 0, 0, instrumentSamplerControlCallback, NULL);
					addControl(&cc, x+34, y+11, &iv->wavetable.lfo.pdyn,  3, -128, 127, 0, 0, instrumentSamplerControlCallback, NULL);
					break;
			}

			drawControls(&cc);
		}
	}
}
