#define INSTUI_SAMPLE_CALLBACK_MAX 6
static void instUISampleCallback(short x, short y, Instrument *iv, uint8_t index)
{
	Sample *sample = (*iv->sample)[w->sample];
	if (!sample) return;

	switch (index)
	{
		case 0:
			printf("\033[%d;%dHrate:  [        ]", y, x);
			addControlInt(x+8, y, &sample->rate, 8, 0x0, 0xffffffff, sample->defrate, 0, 0, (void(*)(void*))instrumentControlCallback, NULL);
			break;
		case 1:
			printf("\033[%d;%dHgain:    [ ][   ]", y, x);
			addControlInt(x+10, y, &sample->invert, 0,    0,   1, 0, 0, 0, (void(*)(void*))instrumentControlCallback, NULL);
			// addControlInt(x+13, y, &iv->gain,       3, -128, 127, 0, 0, 0, (void(*)(void*))instrumentControlCallback, NULL);
			break;
		case 2: printf("\033[%d;%dHstart: [        ]", y, x); addControlInt(x+8, y, &sample->trimstart,  8, 0, sample->length-1  , 0,                0, 0, (void(*)(void*))instrumentControlCallback, NULL); break;
		case 3: printf("\033[%d;%dHdelta: [        ]", y, x); addControlInt(x+8, y, &sample->trimlength, 8, 0, sample->length-1  , sample->length-1, 0, 0, (void(*)(void*))instrumentControlCallback, NULL); break;
		case 4: printf("\033[%d;%dHloop:  [        ]", y, x); addControlInt(x+8, y, &sample->looplength, 8, 0, sample->trimlength, 0,                0, 0, (void(*)(void*))instrumentControlCallback, NULL); break;
		case 5:
			printf("\033[%d;%dHpp/ramp:  [ ][  ]", y, x);
			addControlInt(x+11, y, &sample->pingpong, 0,   0,    1, 0, 0, 0, (void(*)(void*))instrumentControlCallback, NULL);
			addControlInt(x+14, y, &sample->loopramp, 2, 0x0, 0xff, 0, 0, 0, (void(*)(void*))instrumentControlCallback, NULL);
			break;
	}
}

void instUICyclicCallback(short x, short y, Instrument *iv, uint8_t index)
{
	switch (index)
	{
		case 0:
			printf("\033[%d;%dHquality: [ ][ ][  ]", y, x);
			addControlInt(x+10, y, &iv->interpolate, 0,   0,    1,    0, 0, 0, (void(*)(void*))instrumentControlCallback, NULL);
			addControlInt(x+13, y, &iv->bitdepth,    1, 0x0,  0xf,  0xf, 0, 0, (void(*)(void*))instrumentControlCallback, NULL);
			addControlInt(x+16, y, &iv->samplerate,  2, 0x0, 0xff, 0xff, 0, 0, (void(*)(void*))instrumentControlCallback, NULL);
			break;
		case 1:
			printf("\033[%d;%dHgain env:    [    ]", y, x);
			addControlInt(x+14, y, &iv->envelope, 4, 0x0, 0xffff, 0x00f0, 0, 0, (void(*)(void*))instrumentControlCallback, NULL);
			break;
		case 2:
			printf("\033[%d;%dHchannel:   [      ]", y, x);
			addControlInt(x+12, y, &iv->channelmode, 1, 0, 4, 0, 6, 5, (void(*)(void*))instrumentControlCallback, NULL);
				addScalePointInt("STEREO", SAMPLE_CHANNELS_STEREO);
				addScalePointInt("  LEFT", SAMPLE_CHANNELS_LEFT  );
				addScalePointInt(" RIGHT", SAMPLE_CHANNELS_RIGHT );
				addScalePointInt("   MIX", SAMPLE_CHANNELS_MIX   );
				addScalePointInt("  SWAP", SAMPLE_CHANNELS_SWAP  );
			break;
		case 3:
			printf("\033[%d;%dHgain:         [   ]", y, x);
			// if (sample) addControlInt(x+12, y, &sample->invert, 0,    0,   1, 0, 0, 0, (void(*)(void*))instrumentControlCallback, NULL);
			addControlInt(x+15, y, &iv->gain,       3, -128, 127, 0, 0, 0, (void(*)(void*))instrumentControlCallback, NULL);
			break;
		case 4:
			printf("\033[%d;%dHunison:   [ ][ ][ ]", y, x);
			break;
		case 5:
			printf("\033[%d;%dHrev/ramp:   [  ][ ]", y, x);
			addControlInt(x+13, y, &iv->granular.reversegrains, 2, 0x0, 0xff, 0, 0, 0, (void(*)(void*))instrumentControlCallback, NULL);
			addControlInt(x+17, y, &iv->granular.rampgrains,    1, 0x0, 0xf,  8, 0, 0, (void(*)(void*))instrumentControlCallback, NULL);
			break;
		case 6:
			printf("\033[%d;%dHcycle:   [    ][  ]", y, x);
			addControlInt(x+10, y, &iv->granular.cyclelength,       4, 0x0, 0xffff, 0x3fff, 0, 0, (void(*)(void*))instrumentControlCallback, NULL);
			addControlInt(x+16, y, &iv->granular.cyclelengthjitter, 2, 0x0, 0xff,   0x0,    0, 0, (void(*)(void*))instrumentControlCallback, NULL);
			break;
		case 7:
			printf("\033[%d;%dHtime     [ ][     ]", y, x);
			addControlInt(x+10, y, &iv->granular.notestretch, 0, 0,      1,      0, 0, 0, (void(*)(void*))instrumentControlCallback, NULL);
			addControlInt(x+13, y, &iv->granular.timestretch, 5, 0x8bff, 0x7bff, 0, 0, 0, (void(*)(void*))instrumentControlCallback, NULL);
			break;
		case 8:
			printf("\033[%d;%dHpan jitter:    [  ]", y, x);
			addControlInt(x+16, y, &iv->granular.panjitter, 2, 0x00, 0xff, 0, 0, 0, (void(*)(void*))instrumentControlCallback, NULL);
			break;
		case 9:
			printf("\033[%d;%dHptr jitter:    [  ]", y, x);
			addControlInt(x+16, y, &iv->granular.ptrjitter, 2, 0x00, 0xff, 0, 0, 0, (void(*)(void*))instrumentControlCallback, NULL);
			break;
		case 10:
			printf("\033[%d;%dHframe:         [  ]", y, x); addControlInt(x+10, y, &iv->frame, 2, 0, 255, 0, 0, 0, (void(*)(void*))instrumentControlCallback, NULL);
			break;
		case 11:
			printf("\033[%d;%dHpitch: [   ][     ]", y, x);
			addControlInt(x+8 , y, &iv->granular.pitchstereo, 3, -128,   127,    0, 0, 0, (void(*)(void*))instrumentControlCallback, NULL);
			addControlInt(x+13, y, &iv->granular.pitchshift,  5, 0x8bff, 0x7bff, 0, 0, 0, (void(*)(void*))instrumentControlCallback, NULL);
			break;
		case 12:
			printf("\033[%d;%dHjitter:  [  ][ ][ ]", y, x);
			addControlInt(x+10, y, &iv->granular.pitchjitter,       2, 0x00, 0xff, 0, 0, 0, (void(*)(void*))instrumentControlCallback, NULL);
			addControlInt(x+14, y, &iv->granular.pitchoctaverange,  1, 0x0,  0xf,  0, 0, 0, (void(*)(void*))instrumentControlCallback, NULL);
			addControlInt(x+17, y, &iv->granular.pitchoctavechance, 1, 0x0,  0xf,  0, 0, 0, (void(*)(void*))instrumentControlCallback, NULL);
			break;
	}
}

#define EMPTY_INST_UI_TEXT "PRESS 'aa' TO ADD A SAMPLE"
static void emptyInstUICallback(short x, short y, Instrument *iv, uint8_t index)
{
	printf("\033[%d;%d%s", y, x, EMPTY_INST_UI_TEXT);
}

static InstUI *initInstrumentUI(Instrument *iv)
{
	InstUI *iui;
	if (!iv)
	{
		iui = malloc(sizeof(InstUI));
		iui->width = strlen(EMPTY_INST_UI_TEXT);
		iui->count = 1;
		iui->callback = emptyInstUICallback;
		return iui;
	}

	switch (iv->algorithm)
	{
		case INST_ALG_NULL: break;
		case INST_ALG_MIDI: iui = initInstUIMidi(); break;
		case INST_ALG_SAMPLER: iui = initInstUICyclic(); break;
	}
	return iui;
}


static short drawInstrumentIndex(short bx, short minx, short maxx)
{
	Instrument *iv;
	char buffer[11];
	short x = 0;
	for (int i = 0; i < INSTRUMENT_MAX; i++)
		if (w->centre - w->instrument + i > TRACK_ROW && w->centre - w->instrument + i < ws.ws_row)
		{
			x = bx;

			if (instrumentSafe(s->instrument, i))
			{
				iv = &s->instrument->v[s->instrument->i[i]];
				if (iv->triggerflash) printf("\033[3%dm", i%6+1);
			}
			if (w->instrument + w->fyoffset == i)
			{
				printf("\033[7m");

				if (x <= ws.ws_col)
				{
					snprintf(buffer, 4, "%02x ", i);
					printCulling(buffer, x, w->centre - w->instrument + i, minx, maxx);
				} x += 3;

				if (x <= ws.ws_col)
				{
					snprintf(buffer, 4, "%02x ", s->instrument->i[i]);
					printCulling(buffer, x, w->centre - w->instrument + i, minx, maxx);
				} x += 3;
			} else
			{
				if (x <= ws.ws_col)
				{
					snprintf(buffer, 4, "%02x ", i);
					printCulling(buffer, x, w->centre - w->instrument + i, minx, maxx);
				} x += 3;

				if (x <= ws.ws_col)
				{
					snprintf(buffer, 4, "%02x ", s->instrument->i[i]);
					printf("\033[2m");
					printCulling(buffer, x, w->centre - w->instrument + i, minx, maxx);
					printf("\033[22m");
				} x += 3;
			}

			if (x <= ws.ws_col)
			{
				if (instrumentSafe(s->instrument, i))
				{
					iv = &s->instrument->v[s->instrument->i[i]];
					printf("\033[1m");
					if (iv->algorithm == INST_ALG_MIDI)
						snprintf(buffer, 11, "-  MIDI  -");
					else
					{
						uint32_t samplesize = 0;
						FOR_SAMPLECHAIN(j, iv->sample)
							samplesize += (*iv->sample)[j]->length;
						snprintf(buffer, 11, "<%08x>", samplesize);
					}
				} else snprintf(buffer, 11, " ........ ");
				printCulling(buffer, x, w->centre - w->instrument + i, minx, maxx);
			}
			x += 10;

			printf("\033[40;37;22;27m");
		}
	return x - bx;
}

short getInstUIRows(InstUI *iui, short cols)
{
	size_t entryc = iui->count;
	short ret = entryc / cols;

	/* round up instead of down */
	if (entryc%cols)
		ret++;

	return ret;
}
short getInstUICols(InstUI *iui, short rows)
{
	size_t entryc = iui->count;
	short ret = entryc / rows;

	/* round up instead of down */
	if (entryc%rows)
		ret++;

	return ret;
}

void drawInstUI(InstUI *iui, Instrument *iv, short x, short w, short y, short scrolloffset, short rows)
{
	short cols = MIN(w / (iui->width + INSTUI_PADDING), getInstUICols(iui, rows));
	x += (w - (cols*(iui->width + INSTUI_PADDING)) + INSTUI_PADDING)>>1;
	short cx, cy;
	for (uint8_t i = 0; i < iui->count; i++)
	{
		cx = x + (i/rows)*(iui->width + INSTUI_PADDING);
		cy = y + i%rows;
		if (cy < ws.ws_row - 1 && cy > scrolloffset)
			iui->callback(cx, cy - scrolloffset, iv, i);
	}
}

void drawInstrument(void)
{
	switch (w->mode)
	{
		case MODE_INSERT:
			if (cc.mouseadjust || cc.keyadjust) printf("\033[%d;0H\033[1m-- INSERT ADJUST --\033[m\033[4 q", ws.ws_row);
			else                                printf("\033[%d;0H\033[1m-- INSERT --\033[m\033[6 q",        ws.ws_row);
			w->command.error[0] = '\0';
			break;
		default:
			if (cc.mouseadjust || cc.keyadjust) { printf("\033[%d;0H\033[1m-- ADJUST --\033[m\033[4 q", ws.ws_row); w->command.error[0] = '\0'; }
			break;
	}

	short minx = 1;
	short maxx = ws.ws_col;
	short x = drawInstrumentIndex(1, minx, maxx) + 2;
	static short scrolloffset;

	if (instrumentSafe(s->instrument, w->instrument))
	{
		if (w->showfilebrowser)
			drawBrowser(fbstate);
		else
		{
			Instrument *iv = &s->instrument->v[s->instrument->i[w->instrument]];
			InstUI *iui = initInstrumentUI(iv);

			clearControls();

			short cols = (ws.ws_col - x) / (iui->width + INSTUI_PADDING);
			if (!cols) goto drawInstrumentEnd;

			short rows = getInstUIRows(iui, cols);
			if (!rows) goto drawInstrumentEnd;

			short y = TRACK_ROW+1;
			short wh = MAX((ws.ws_row - 1 - y) - rows, INSTUI_WAVEFORM_MIN);

			switch (iv->algorithm)
			{
				case INST_ALG_NULL: break;
				case INST_ALG_MIDI: break;
				case INST_ALG_SAMPLER:
					drawBoundingBox(x, y, ws.ws_col - x, wh - 1, minx, maxx, 1, ws.ws_row);

					InstUI *sample_iui = malloc(sizeof(InstUI));
					sample_iui->width = 17;
					sample_iui->count = INSTUI_SAMPLE_CALLBACK_MAX;
					sample_iui->callback = instUISampleCallback;
					short sample_rows = getInstUIRows(sample_iui, (ws.ws_col - x - 2) / (sample_iui->width + INSTUI_PADDING));

					short whh = wh - sample_rows;

					/* multisample indices */
					addControlDummy(x + 3, y + (whh>>1));
					char buffer[5];
					short siy;
					for (uint8_t i = 0; i < SAMPLE_MAX; i++)
					{
						siy = y + (whh>>1) - w->sample + i;
						if (siy > y && siy < y + (whh - 1))
						{ /* vertical bounds limiting */
							if (i == w->sample) printf("\033[1;7m");
							if ((*iv->sample)[i]) snprintf(buffer, 5, " %01x", i);
							else                  snprintf(buffer, 5, " .");
							printCulling(buffer, x+1, siy, minx, maxx);
							if (i == w->sample) printf("\033[22;27m");
						}
					}

					drawWaveform(iv, x+INSTUI_MULTISAMPLE_WIDTH + 1, y+1, ws.ws_col - (x+INSTUI_MULTISAMPLE_WIDTH) - 1, whh - 2);

					drawInstUI(sample_iui, iv, x+1, ws.ws_col - (x+1), y + whh - 1, scrolloffset, sample_rows);

					free(sample_iui);
					break;
			}

			y += wh;

			short viewportheight = ws.ws_row - 1 - y;
			if (viewportheight <= 0)
				goto drawInstrumentEnd;

			/* ensure that scrolloffset is in range */
			/* TODO: scrolloffset padding */
			// scrolloffset = MIN(scrolloffset + viewportheight, rows          ); /* within range */
			// scrolloffset = MIN(scrolloffset, cc.cursor%rows                 ); /* push up      */
			// scrolloffset = MAX(scrolloffset, cc.cursor%rows - viewportheight); /* push down    */

			drawInstUI(iui, iv, x, ws.ws_col - x, y, scrolloffset, rows);

drawInstrumentEnd:
			drawControls();
			free(iui);
		}
	} else
	{
	}
}
