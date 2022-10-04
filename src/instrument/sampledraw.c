void drawMarker(uint32_t marker, size_t offset)
{
	size_t xpos;
	if (marker >= offset && marker < offset + w->waveformwidth)
	{
		xpos = (float)(marker - offset) / (float)w->waveformwidth * w->waveformw;
		for (size_t i = 0; i < w->waveformh; i++) set_pixel(w->waveformcanvas, i%2, xpos, i);
	}
}

void instrumentSamplerControlCallback(void *casttocc, bool keyadjustchanged, bool mouseadjustchanged, bool valuechanged)
{
	ControlState *cc = (ControlState *)casttocc;
	switch (cc->cursor)
	{
		case 9: case 10: case 11: /* start / length / loop */
			if (keyadjustchanged || mouseadjustchanged || valuechanged)
				w->waveformdrawpointer = 0;
			break;
	}
}

void drawInstrumentSampler(Instrument *iv)
{
	short x, y;
	const char *text;

	clearControls(&cc);
	setControlCallback(&cc, instrumentSamplerControlCallback);

	if (ws.ws_col - INSTRUMENT_INDEX_COLS < 57)
	{
		x = INSTRUMENT_INDEX_COLS + ((ws.ws_col - INSTRUMENT_INDEX_COLS - 38)>>1) +1;
		y = ws.ws_row - 10;

		printf("\033[%d;%dHC5rate: [        ]  channels: [      ]", y+0, x);
		printf("\033[%d;%dHdecimate:  [ ][  ]  MIDI channel:  [ ]", y+1, x);
		printf("\033[%d;%dHenvelope:  [  ][ ]  gain:     [ ][   ]", y+2, x);
		printf("\033[%d;%dH                                      ", y+3, x);
		printf("\033[%d;%dH +  trim range  +    +   granular   + ", y+4, x);
		printf("\033[%d;%dHstart:  [        ]  grain size: [    ]", y+5, x);
		printf("\033[%d;%dHlength: [        ]  rearrange:    [  ]", y+6, x);
		printf("\033[%d;%dHloop:   [        ]  reverse:       [ ]", y+7, x);
		printf("\033[%d;%dHpingpong loop: [ ]  time:   [ ][     ]", y+8, x);
		printf("\033[%d;%dHloop ramping: [  ]  pitch:[     ][   ]", y+9, x);

		addControl(&cc, x+9,  y+0, &iv->c5rate,      8, 0x0, 0xffffffff, 0);
		addControl(&cc, x+12, y+1, &iv->bitdepth,    1, 0x0, 0xf, 0);
		addControl(&cc, x+15, y+1, &iv->samplerate,  2, 0x0, 0xff, 0);
		addControl(&cc, x+12, y+2, &iv->envelope,    2, 0x0, 0xff, 0);
		addControl(&cc, x+16, y+2, &iv->sustain,     0, 0, 1, 0);
		addControl(&cc, x+31, y+0, &iv->channelmode, 1, 0, 4, 6);
			setControlPrettyName(&cc, "STEREO");
			setControlPrettyName(&cc, "  LEFT");
			setControlPrettyName(&cc, " RIGHT");
			setControlPrettyName(&cc, "   MIX");
			setControlPrettyName(&cc, "  SWAP");
		addControl(&cc, x+36, y+1, &iv->midichannel, 1, -1, 15, 0);
		addControl(&cc, x+31, y+2, &iv->invert,      0, 0, 1, 0);
		addControl(&cc, x+34, y+2, &iv->gain,        3, 0, 0, 0);

		/* range */
		iv->trimlength = MIN(iv->trimlength, iv->length-1 - iv->trimstart);
		iv->looplength = MIN(iv->looplength, iv->trimlength);
		addControl(&cc, x+9,  y+5, &iv->trimstart,  8, 0x0, iv->length-1, 0);
		addControl(&cc, x+9,  y+6, &iv->trimlength, 8, 0x0, iv->length-1 - iv->trimstart, 0);
		addControl(&cc, x+9,  y+7, &iv->looplength, 8, 0x0, iv->trimlength, 0);
		addControl(&cc, x+16, y+8, &iv->pingpong,   0, 0, 1, 0);
		addControl(&cc, x+15, y+9, &iv->loopramp,   2, 0x0, 0xff, 0);

		/* granular */
		addControl(&cc, x+33, y+5, &iv->cyclelength,   4, 0x0, 0xffff, 0);
		addControl(&cc, x+35, y+6, &iv->rearrange,     2, 0x0, 0xff, 0);
		addControl(&cc, x+36, y+7, &iv->reversegrains, 0, 0, 1, 0);
		addControl(&cc, x+29, y+8, &iv->notestretch,   0, 0, 1, 0);
		addControl(&cc, x+32, y+8, &iv->timestretch,   5, 0, 0, 0);
		addControl(&cc, x+27, y+9, &iv->pitchshift,    5, 0, 0, 0);
		addControl(&cc, x+34, y+9, &iv->pitchstereo,   3, 0, 0, 0);
	} else
	{
		x = INSTRUMENT_INDEX_COLS + ((ws.ws_col - INSTRUMENT_INDEX_COLS - 58)>>1) +1;
		y = ws.ws_row - 6;

		printf("\033[%d;%dHC5rate: [        ]   +  trim range  +    +   granular   + ", y+0, x);
		printf("\033[%d;%dHdecimate:  [ ][  ]  start:  [        ]  grain size: [    ]", y+1, x);
		printf("\033[%d;%dHenvelope:  [  ][ ]  length: [        ]  rearrange:    [  ]", y+2, x);
		printf("\033[%d;%dHchannels: [      ]  loop:   [        ]  reverse:       [ ]", y+3, x);
		printf("\033[%d;%dHMIDI channel:  [ ]  pingpong loop: [ ]  time:   [ ][     ]", y+4, x);
		printf("\033[%d;%dHgain:     [ ][   ]  loop ramping: [  ]  pitch:[     ][   ]", y+5, x);

		addControl(&cc, x+9,  y+0, &iv->c5rate,      8, 0x0, 0xffffffff, 0);
		addControl(&cc, x+12, y+1, &iv->bitdepth,    1, 0x0, 0xf, 0);
		addControl(&cc, x+15, y+1, &iv->samplerate,  2, 0x0, 0xff, 0);
		addControl(&cc, x+12, y+2, &iv->envelope,    2, 0x0, 0xff, 0);
		addControl(&cc, x+16, y+2, &iv->sustain,     0, 0, 1, 0);
		addControl(&cc, x+11, y+3, &iv->channelmode, 1, 0, 4, 6);
			setControlPrettyName(&cc, "STEREO");
			setControlPrettyName(&cc, "  LEFT");
			setControlPrettyName(&cc, " RIGHT");
			setControlPrettyName(&cc, "   MIX");
			setControlPrettyName(&cc, "  SWAP");
		addControl(&cc, x+16, y+4, &iv->midichannel, 1, -1, 15, 0);
		addControl(&cc, x+11, y+5, &iv->invert,      0, 0, 1, 0);
		addControl(&cc, x+14, y+5, &iv->gain,        3, 0, 0, 0);

		/* range */
		iv->trimlength = MIN(iv->trimlength, iv->length-1 - iv->trimstart);
		iv->looplength = MIN(iv->looplength, iv->trimlength);
		addControl(&cc, x+29, y+1, &iv->trimstart,  8, 0x0, iv->length-1, 0);
		addControl(&cc, x+29, y+2, &iv->trimlength, 8, 0x0, iv->length-1 - iv->trimstart, 0);
		addControl(&cc, x+29, y+3, &iv->looplength, 8, 0x0, iv->trimlength, 0);
		addControl(&cc, x+36, y+4, &iv->pingpong,   0, 0, 1, 0);
		addControl(&cc, x+35, y+5, &iv->loopramp,   2, 0x0, 0xff, 0);

		/* granular */
		addControl(&cc, x+53, y+1, &iv->cyclelength,   4, 0x0, 0xffff, 0);
		addControl(&cc, x+55, y+2, &iv->rearrange,     2, 0x0, 0xff, 0);
		addControl(&cc, x+56, y+3, &iv->reversegrains, 0, 0, 1, 0);
		addControl(&cc, x+49, y+4, &iv->notestretch,   0, 0, 1, 0);
		addControl(&cc, x+52, y+4, &iv->timestretch,   5, 0, 0, 0);
		addControl(&cc, x+47, y+5, &iv->pitchshift,    5, 0, 0, 0);
		addControl(&cc, x+54, y+5, &iv->pitchstereo,   3, 0, 0, 0);
	}

	if (!iv->samplelength || !iv->sampledata)
	{
		text = "PRESS 'o' TO OPEN A SAMPLE";
		printf("\033[%d;%dH%s", CHANNEL_ROW + ((y - CHANNEL_ROW)>>1), INSTRUMENT_INDEX_COLS + ((ws.ws_col - INSTRUMENT_INDEX_COLS - (short)strlen(text))>>1), text);
	} else if (!(w->instrumentlockv != INST_GLOBAL_LOCK_OK && w->instrumentlocki == s->instrument->i[w->instrument]))
	{
		if (w->waveformbuffer)
		{
			/* get the cursor screen pos and the waveform scroll offset */
			size_t offset;
			if (w->waveformcursor < (w->waveformwidth>>1))                   offset = 0;
			else if (w->waveformcursor > iv->length - (w->waveformwidth>>1)) offset = (iv->length - w->waveformwidth);
			else                                                             offset = w->waveformcursor - (w->waveformwidth>>1);

			if (w->waveformdrawpointer == 0)
				fill(w->waveformcanvas, 0);

			size_t k, x;
			uint32_t l;
			float channelmix = 1.0f / (float)iv->channels;
			double divmaxj = 1.0f / (float)w->waveformwidth;
			float o = (float)w->waveformh * 0.5f;
			float sample;
			double samplesperpixel = (double)w->waveformwidth / (double)w->waveformw;
			if (w->waveformdrawpointer < w->waveformwidth)
			{
				for (uint32_t j = 0; j < WAVEFORM_LAZY_BLOCK_SIZE; j++)
				{
					/* switch to left->right rendering if zoomed in far enough */
					if (w->waveformw > w->waveformwidth) l = w->waveformdrawpointer;
					else l = (w->waveformdrawpointer%w->waveformw)*samplesperpixel + w->waveformdrawpointer/w->waveformw;

					k = (float)l * divmaxj * (float)w->waveformwidth;
					x = (float)l * divmaxj * (float)w->waveformw;

					sample = 0.0f;
					for (uint8_t i = 0; i < iv->channels; i++) /* mix all channels */
						sample += (iv->sampledata[(offset + k) * iv->channels + i] * channelmix);
					sample = (sample*DIVSHRT) * o + o;

					/* TODO: use set_pixel_unsafe */
					set_pixel(w->waveformcanvas, 1, x, sample);

					w->waveformdrawpointer++;
					if (w->waveformdrawpointer >= w->waveformwidth) break;
				} p->dirty = 1; /* continue drawing asap */
			}

			drawMarker(iv->trimstart,  offset);
			drawMarker(iv->trimstart + iv->trimlength, offset);
			drawMarker(iv->trimstart + iv->trimlength - iv->looplength, offset);
			draw(w->waveformcanvas, w->waveformbuffer);
			for (size_t i = 0; w->waveformbuffer[i] != NULL; i++)
				if (w->waveformbuffer)
					printf("\033[%ld;%dH%s", CHANNEL_ROW + i, INSTRUMENT_INDEX_COLS, w->waveformbuffer[i]);
		}
	}

	drawControls(&cc);
}
