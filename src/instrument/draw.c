static void samplerInstUICommonCallback(short x, short y, Instrument *iv, uint8_t index)
{
	switch (index)
	{
		case 0:
			printf("\033[%d;%dHC5 rate: [        ]", y, x);
			addControlInt(x+10, y, &iv->sample->rate, 8, 0x0, 0xffffffff, iv->sample->defrate, 0, 0, (void(*)(void*))instrumentControlCallback, NULL);
			break;
		case 1:
			printf("\033[%d;%dHquality: [ ][ ][  ]", y, x);
			addControlInt(x+10, y, &iv->interpolate, 0, 0,   1,      0,    0, 0, (void(*)(void*))instrumentControlCallback, NULL);
			addControlInt(x+13, y, &iv->bitdepth,    1, 0x0, 0xf,    0xf,  0, 0, (void(*)(void*))instrumentControlCallback, NULL);
			addControlInt(x+16, y, &iv->samplerate,  2, 0x0, 0xff,   0xff, 0, 0, (void(*)(void*))instrumentControlCallback, NULL);
			break;
		case 2:
			printf("\033[%d;%dHgain env:    [    ]", y, x);
			addControlInt(x+14, y, &iv->envelope, 4, 0x0, 0xffff, 0x00f0, 0, 0, (void(*)(void*))instrumentControlCallback, NULL);
			break;
		case 3:
			printf("\033[%d;%dHchannel:   [      ]", y, x);
			addControlInt(x+12, y, &iv->channelmode, 1, 0, 4, 0, 6, 5, (void(*)(void*))instrumentControlCallback, NULL);
				addScalePointInt("STEREO", SAMPLE_CHANNELS_STEREO);
				addScalePointInt("  LEFT", SAMPLE_CHANNELS_LEFT  );
				addScalePointInt(" RIGHT", SAMPLE_CHANNELS_RIGHT );
				addScalePointInt("   MIX", SAMPLE_CHANNELS_MIX   );
				addScalePointInt("  SWAP", SAMPLE_CHANNELS_SWAP  );
			break;
		case 4:
			printf("\033[%d;%dHgain:      [ ][   ]", y, x);
			addControlInt(x+12, y, &iv->invert, 0, 0,    1,   0, 0, 0, (void(*)(void*))instrumentControlCallback, NULL);
			addControlInt(x+15, y, &iv->gain,   3, -128, 127, 0, 0, 0, (void(*)(void*))instrumentControlCallback, NULL);
			break;
	}
}
void initInstUICommonSamplerBlock(InstUIBlock *block)
{
	block->count = 5;
	block->callback = samplerInstUICommonCallback;
}

static void samplerInstUIRangeCallback(short x, short y, Instrument *iv, uint8_t index)
{
	switch (index)
	{
		case 0: printf("\033[%d;%dHoffset:  [        ]", y, x); addControlInt(x+10, y, &iv->trimstart,  8, 0, iv->sample->length-1, 0,                    0, 0, (void(*)(void*))instrumentControlCallback, NULL); break;
		case 1: printf("\033[%d;%dHlength:  [        ]", y, x); addControlInt(x+10, y, &iv->trimlength, 8, 0, iv->sample->length-1, iv->sample->length-1, 0, 0, (void(*)(void*))instrumentControlCallback, NULL); break;
		case 2: printf("\033[%d;%dHloop:    [        ]", y, x); addControlInt(x+10, y, &iv->looplength, 8, 0, iv->trimlength,       0,                    0, 0, (void(*)(void*))instrumentControlCallback, NULL); break;
		case 3: printf("\033[%d;%dHframe:         [  ]", y, x); addControlInt(x+10, y, &iv->frame,      2, 0,                  255, 0,                    0, 0, (void(*)(void*))instrumentControlCallback, NULL); break;
		case 4:
			printf("\033[%d;%dHpp/ramp:    [ ][  ]", y, x);
			addControlInt(x+13, y, &iv->pingpong, 0, 0,   1,    0, 0, 0, (void(*)(void*))instrumentControlCallback, NULL);
			addControlInt(x+16, y, &iv->loopramp, 2, 0x0, 0xff, 0, 0, 0, (void(*)(void*))instrumentControlCallback, NULL);
			break;
	}
}
void initInstUIRangeSamplerBlock(InstUIBlock *block)
{
	block->count = 5;
	block->callback = samplerInstUIRangeCallback;
}

static void samplerInstUIGranularCallback(short x, short y, Instrument *iv, uint8_t index)
{
	switch (index)
	{
		case 0: printf("\033[%d;%dHunison:   [ ][ ][ ]", y, x); break;
		case 1:
			printf("\033[%d;%dHrev/ramp:   [  ][ ]", y, x);
			addControlInt(x+13, y, &iv->granular.reversegrains,     2, 0x0,    0xff,   0,      0, 0, (void(*)(void*))instrumentControlCallback, NULL);
			addControlInt(x+17, y, &iv->granular.rampgrains,        1, 0x0,    0xf,    8,      0, 0, (void(*)(void*))instrumentControlCallback, NULL);
			break;
		case 2:
			printf("\033[%d;%dHcycle:   [    ][  ]", y, x);
			addControlInt(x+10, y, &iv->granular.cyclelength,       4, 0x0,    0xffff, 0x3fff, 0, 0, (void(*)(void*))instrumentControlCallback, NULL);
			addControlInt(x+16, y, &iv->granular.cyclelengthjitter, 2, 0x0,    0xff,   0x0,    0, 0, (void(*)(void*))instrumentControlCallback, NULL);
			break;
		case 3:
			printf("\033[%d;%dHtime     [ ][     ]", y, x);
			addControlInt(x+10, y, &iv->granular.notestretch,       0, 0,      1,      0,      0, 0, (void(*)(void*))instrumentControlCallback, NULL);
			addControlInt(x+13, y, &iv->granular.timestretch,       5, 0x8bff, 0x7bff, 0,      0, 0, (void(*)(void*))instrumentControlCallback, NULL);
			break;
		case 4:
			printf("\033[%d;%dHpan jitter:    [  ]", y, x);
			addControlInt(x+16, y, &iv->granular.panjitter, 2, 0x00, 0xff, 0, 0, 0, (void(*)(void*))instrumentControlCallback, NULL);
			break;
		case 5:
			printf("\033[%d;%dHptr jitter:    [  ]", y, x);
			addControlInt(x+16, y, &iv->granular.ptrjitter, 2, 0x00, 0xff, 0, 0, 0, (void(*)(void*))instrumentControlCallback, NULL);
			break;
	}
}
void initInstUIGranularSamplerBlock(InstUIBlock *block)
{
	block->count = 6;
	block->callback = samplerInstUIGranularCallback;
}

static void samplerInstUIPitchCallback(short x, short y, Instrument *iv, uint8_t index)
{
	switch (index)
	{
		case 0:
			printf("\033[%d;%dHpitch: [   ][     ]", y, x);
			addControlInt(x+8 , y, &iv->granular.pitchstereo,       3, -128,   127,    0, 0, 0, (void(*)(void*))instrumentControlCallback, NULL);
			addControlInt(x+13, y, &iv->granular.pitchshift,        5, 0x8bff, 0x7bff, 0, 0, 0, (void(*)(void*))instrumentControlCallback, NULL);
			break;
		case 1:
			printf("\033[%d;%dHjitter:  [  ][ ][ ]", y, x);
			addControlInt(x+10, y, &iv->granular.pitchjitter,       2, 0x00,   0xff,   0, 0, 0, (void(*)(void*))instrumentControlCallback, NULL);
			addControlInt(x+14, y, &iv->granular.pitchoctaverange,  1, 0x0,    0xf,    0, 0, 0, (void(*)(void*))instrumentControlCallback, NULL);
			addControlInt(x+17, y, &iv->granular.pitchoctavechance, 1, 0x0,    0xf,    0, 0, 0, (void(*)(void*))instrumentControlCallback, NULL);
			break;
	}
}
void initInstUIPitchSamplerBlock(InstUIBlock *block)
{
	block->count = 2;
	block->callback = samplerInstUIPitchCallback;
}

InstUI *allocInstUI(uint8_t blocks)
{
	InstUI *iui = malloc(sizeof(InstUI) + sizeof(InstUIBlock)*blocks);
	iui->blocks = blocks;
	return iui;
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
		iui = allocInstUI(1);
		iui->width = strlen(EMPTY_INST_UI_TEXT);
		iui->block[0].count = 1;
		iui->block[0].callback = emptyInstUICallback;
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
					if (iv->algorithm == INST_ALG_MIDI) snprintf(buffer, 11, "-  MIDI  -");
					else if (iv->sample)                snprintf(buffer, 11, "<%08x>", iv->sample->length);
					else                                snprintf(buffer, 11, "<%08x>", 0);
				} else snprintf(buffer, 11, " ........ ");
				printCulling(buffer, x, w->centre - w->instrument + i, minx, maxx);
			}
			x += 10;

			printf("\033[40;37;22;27m");
		}
	return x - bx;
}

void drawInstrument(void)
{
	switch (w->mode)
	{
		case MODE_INSERT:
			if (cc.mouseadjust || cc.keyadjust) printf("\033[%d;0H\033[1m-- INSERT ADJUST --\033[m\033[4 q", ws.ws_row);
			else                                  printf("\033[%d;0H\033[1m-- INSERT --\033[m\033[6 q",        ws.ws_row);
			w->command.error[0] = '\0';
			break;
		default:
			if (cc.mouseadjust || cc.keyadjust) { printf("\033[%d;0H\033[1m-- ADJUST --\033[m\033[4 q", ws.ws_row); w->command.error[0] = '\0'; }
			break;
	}

	short x = drawInstrumentIndex(1, 1, ws.ws_col) + 2;
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

			size_t iui_entryc = 0;
			for (uint8_t i = 0; i < iui->blocks; i++)
				iui_entryc += iui->block[i].count;
			if (!iui_entryc)
				goto drawInstrumentEnd;

			short cols = (ws.ws_col - x) / (iui->width + INSTUI_PADDING);
			if (!cols) goto drawInstrumentEnd;

			short rows = iui_entryc / cols;
			/* round up instead of down */
			/* TODO: sometimes leaves empty columns */
			if (iui_entryc%cols)
				rows++;

			if (!rows)
				goto drawInstrumentEnd;

			short y = TRACK_ROW+1;
			if (iui->flags&INSTUI_DRAWWAVEFORM)
			{
				short wh = MAX((ws.ws_row - 1 - y) - rows, INSTUI_WAVEFORM_MIN);
				drawWaveform(iv, wh);
				y += wh;
			}

			short iui_x = x + ((ws.ws_col - x - (cols*(iui->width + INSTUI_PADDING) - INSTUI_PADDING))>>1);

			short viewportheight = ws.ws_row - 1 - y;
			if (viewportheight <= 0)
				goto drawInstrumentEnd;

			/* ensure that scrolloffset is in range */
			/* TODO: scrolloffset padding */
			// scrolloffset = MIN(scrolloffset + viewportheight, rows           ); /* within range */
			// scrolloffset = MIN(scrolloffset, cc.cursor%rows                 ); /* push up      */
			// scrolloffset = MAX(scrolloffset, cc.cursor%rows - viewportheight); /* push down    */

			size_t ei = 0;
			short cx, cy;
			for (uint8_t i = 0; i < iui->blocks; i++)
				for (uint8_t j = 0; j < iui->block[i].count; j++)
				{
					cx = iui_x + (ei/rows)*(iui->width + INSTUI_PADDING);
					cy = y + ei%rows;
					if (cy < ws.ws_row - 1 && cy > scrolloffset)
						iui->block[i].callback(cx, cy - scrolloffset, iv, j);
					ei++;
				}

drawInstrumentEnd:
			// drawInstrumentSampler(iv, x, ws.ws_col - x);
			drawControls();
			free(iui);
		}
	} else
	{
	}
}
