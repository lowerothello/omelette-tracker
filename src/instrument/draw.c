void drawMarker(uint32_t marker, uint32_t offset)
{
	size_t xpos;
	if (marker >= offset && marker < offset + w->waveformwidth)
	{
		xpos = (float)(marker - offset) / (float)w->waveformwidth * w->waveformw;
		for (size_t i = 0; i < w->waveformh-1; i++) set_pixel(w->waveformcanvas, i%2, xpos, i);
	}
}

void drawInstrument(void)
{
	printf("\033[%d;%dH\033[2mPATTERN\033[m \033[1mINSTRUMENT\033[m", CHANNEL_ROW-2, (ws.ws_col-18) / 2);
	if (cc.mouseadjust || cc.keyadjust)
	{
		if (w->mode == I_MODE_PREVIEW) printf("\033[%d;0H\033[1m-- PREVIEWADJUST --\033[m", ws.ws_row);
		else                           printf("\033[%d;0H\033[1m-- ADJUST --\033[m", ws.ws_row);
		w->command.error[0] = '\0';
	} else
		switch (w->mode)
		{
			case I_MODE_PREVIEW: case I_MODE_INDICES_PREVIEW: printf("\033[%d;0H\033[1m-- PREVIEW --\033[m", ws.ws_row); w->command.error[0] = '\0'; break;
			case I_MODE_VISUAL:                               printf("\033[%d;0H\033[1m-- VISUAL --\033[m", ws.ws_row); w->command.error[0] = '\0'; break;
		}

	int i;
	instrument *iv;

	for (i = 0; i < 255; i++)
		if (w->centre - w->instrument + i > CHANNEL_ROW && w->centre - w->instrument + i < ws.ws_row)
		{
			if (w->mode != I_MODE_INDICES && w->mode != I_MODE_INDICES_PREVIEW && w->instrument == i)
			{
				if (s->instrumenti[i] < s->instrumentc)
				{
					iv = &s->instrumentv[s->instrumenti[i]];
					if (iv->triggerflash) printf("\033[%d;1H\033[7m %02x %02x \033[4%dm[%08x]\033[40;22m \033[27m", w->centre - w->instrument + i, i, s->instrumenti[i], i%6 + 1, iv->length);
					else                  printf("\033[%d;1H\033[7m %02x %02x \033[1m[%08x]\033[22m \033[27m",      w->centre - w->instrument + i, i, s->instrumenti[i], iv->length);
				} else    printf("\033[%d;1H\033[7m %02x %02x  ........  \033[27m",                                 w->centre - w->instrument + i, i, s->instrumenti[i]);
			} else
			{
				if (s->instrumenti[i] < s->instrumentc)
				{
					iv = &s->instrumentv[s->instrumenti[i]];
					if (iv->triggerflash) printf("\033[%d;1H %02x \033[2m%02x\033[22m \033[3%dm[%08x]\033[37;22m ", w->centre - w->instrument + i, i, s->instrumenti[i], i%6 + 1, iv->length);
					else                  printf("\033[%d;1H %02x \033[2m%02x\033[22m \033[1m[%08x]\033[22m ", w->centre - w->instrument + i, i, s->instrumenti[i], iv->length);
				} else    printf("\033[%d;1H %02x \033[2m%02x\033[22m  ........  ",                            w->centre - w->instrument + i, i, s->instrumenti[i]);
			}
		}

	if (s->instrumenti[w->instrument] < s->instrumentc)
	{
		iv = &s->instrumentv[s->instrumenti[w->instrument]];

		unsigned short x = INSTRUMENT_INDEX_COLS + (ws.ws_col - INSTRUMENT_INDEX_COLS - INSTRUMENT_CONTROL_COLS)/2;
		unsigned short y = ws.ws_row - INSTRUMENT_CONTROL_ROW;

		clearControls(&cc);
		if (!iv->samplelength) addControl(&cc, INSTRUMENT_INDEX_COLS + (ws.ws_col - INSTRUMENT_INDEX_COLS - 9)/2, w->centre, NULL, 0, 0, 0, 0);
		else                   addControl(&cc, INSTRUMENT_INDEX_COLS + (ws.ws_col - INSTRUMENT_INDEX_COLS - 12)/2 +2, CHANNEL_ROW, NULL, 0, 0, 0, 0);
		addControl(&cc, x+11, y+0, &iv->c5rate,      8, 0x0, 0xffffffff, 0);
		addControl(&cc, x+14, y+1, &iv->bitdepth,    1, 0x0, 0xf,        0);
		addControl(&cc, x+17, y+1, &iv->samplerate,  2, 0x0, 0xff,       0);
		addControl(&cc, x+13, y+2, &iv->channelmode, 1, 0,   4,          6);
			setControlPrettyName(&cc, "stereo");
			setControlPrettyName(&cc, "  left");
			setControlPrettyName(&cc, " right");
			setControlPrettyName(&cc, "   mix");
			setControlPrettyName(&cc, "  swap");
		addControl(&cc, x+14, y+3, &iv->envelope,    2, 0x0, 0xff,       0);
		addControl(&cc, x+18, y+3, &iv->sustain,     0, 0,   1,          0);
		addControl(&cc, x+32, y+0, &iv->gain,        2, 0x0, 0xff,       0);
		addControl(&cc, x+36, y+0, &iv->invert,      0, 0,   1,          0);
		addControl(&cc, x+51, y+0, &iv->midichannel, 1, -1,  15,         0);
		addControl(&cc, x+34, y+2, &iv->pingpong,    0, 0,   1,          0);
		addControl(&cc, x+33, y+3, &iv->loopramp,    2, 0x0, 0xff,       0);
		addControl(&cc, x+52, y+2, &iv->cyclelength, 4, 0x0, 0xffff,     0);
		addControl(&cc, x+51, y+3, &iv->timestretch, 0, 0,   1,          0);
		addControl(&cc, x+54, y+3, &iv->pitchshift,  2, 0x0, 0xff,       0);

		if (!iv->samplelength)
			printf("\033[%d;%dH [NO SAMPLE] ", w->centre, INSTRUMENT_INDEX_COLS + (ws.ws_col - INSTRUMENT_INDEX_COLS - 13)/2);
		else if (!(w->instrumentlockv != INST_GLOBAL_LOCK_OK && w->instrumentlocki == s->instrumenti[w->instrument]))
		{
			printf("\033[%d;%dH [WAVEFORM] ", CHANNEL_ROW, INSTRUMENT_INDEX_COLS + (ws.ws_col - INSTRUMENT_INDEX_COLS - 12)/2);
			if (w->waveformbuffer)
			{
				/* draw visual/cursor */
				size_t cursorxpos, visualxpos;
				uint32_t offset;
				if (w->waveformcursor < (w->waveformwidth>>1))
				{
					cursorxpos = (float)w->waveformcursor / (float)w->waveformwidth * w->waveformw;
					offset = 0;
				} else if (w->waveformcursor > iv->length - (w->waveformwidth>>1))
				{
					cursorxpos = (float)(w->waveformcursor - (iv->length - w->waveformwidth)) / (float)w->waveformwidth * w->waveformw;
					offset = (iv->length - w->waveformwidth);
				} else
				{
					cursorxpos = w->waveformw>>1;
					offset = w->waveformcursor - (w->waveformwidth>>1);
				}

				if (w->mode == I_MODE_VISUAL)
				{
					if (w->waveformvisual <= offset)
						visualxpos = 0;
					else if (w->waveformvisual >= offset + w->waveformwidth)
						visualxpos = w->waveformw-1;
					else
						visualxpos = (float)(w->waveformvisual - offset) / (float)w->waveformwidth * w->waveformw;
				} else visualxpos = cursorxpos;

				size_t lowxpos =  MIN(cursorxpos, visualxpos);
				size_t highxpos = MAX(cursorxpos, visualxpos);

				if (w->waveformdrawpointer == 0)
				{
					fill(w->waveformcanvas, 0);
					if (!cc.cursor)
						for (size_t i = lowxpos; i <= highxpos; i++)
							for (size_t j = 0; j < w->waveformh-1; j++)
								/* TODO: use set_pixel_unsafe */
								set_pixel(w->waveformcanvas, 1, i, j);
				}

				size_t k, x;
				uint32_t l;
				float channelmix = 1.0 / (float)iv->channels;
				double divmaxj = 1.0 / (float)w->waveformwidth;
				float o = (float)w->waveformh * 0.5;
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

						sample = 0.0;
						for (uint8_t i = 0; i < iv->channels; i++) /* mix all channels */
							sample = sample + (iv->sampledata[(offset + k) * iv->channels + i] * channelmix);
						sample = (sample*DIVSHRT) * o + o;

						if (!cc.cursor)
						{
							/* TODO: use set_pixel_unsafe */
							if (x < lowxpos || x > highxpos) set_pixel(w->waveformcanvas, 1, x, sample);
							else                             set_pixel(w->waveformcanvas, 0, x, sample);
						} else set_pixel(w->waveformcanvas, 1, x, sample);

						w->waveformdrawpointer++;
						if (w->waveformdrawpointer >= w->waveformwidth)
						{
							w->waveformdrawpointer++;
							break;
						}
					} p->dirty = 1; /* continue drawing asap */
				}

				drawMarker(iv->trim[0], offset);
				drawMarker(iv->trim[1], offset);
				drawMarker(iv->loop, offset);
				draw(w->waveformcanvas, w->waveformbuffer);
				for (size_t i = 0; w->waveformbuffer[i] != NULL; i++)
					printf("\033[%ld;%dH%s", CHANNEL_ROW + i +1, INSTRUMENT_INDEX_COLS, w->waveformbuffer[i]);
			}
		}

		printf("\033[%d;%dHC-5 rate: [        ]   output: [  ][ ]      MIDI: [ ]    ", y+0, x);
		printf("\033[%d;%dHdecimate:    [ ][  ]  ┌─   LOOP   ─┐  ┌─  TIMESTRETCH  ─┐", y+1, x);
		printf("\033[%d;%dHchannels:   [      ]  ping-pong: [ ]  cyclelength: [    ]", y+2, x);
		printf("\033[%d;%dHenvelope:    [  ][ ]  ramping:  [  ]  time/pitch: [ ][  ]", y+3, x);

		drawControls(&cc);
	} else printf("\033[%d;%dH [NOT ADDED] \033[11D", w->centre, INSTRUMENT_INDEX_COLS + (ws.ws_col - INSTRUMENT_INDEX_COLS - 13)/2);

	switch (w->mode)
	{
		case I_MODE_INDICES: case I_MODE_INDICES_PREVIEW:
			printf("\033[%d;%dH", w->centre + w->fyoffset, 9);
			break;
	}
}


