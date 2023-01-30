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
		case INST_ALG_SIMPLE:
			iui = allocInstUI(1);
			iui->width = INSTUI_SAMPLER_WIDTH;
			initInstUICommonSamplerBlock(&iui->block[0]);
			iui->flags |= INSTUI_DRAWWAVEFORM;
			break;
		case INST_ALG_MIDI: initInstUIMidi(&iui); break;
		case INST_ALG_CYCLIC: initInstUICyclic(&iui); break;
		// case INST_ALG_TONAL:
		// 	drawTonal(cc, iv, xx);
		// 	break;
		// case INST_ALG_BEAT:
		// 	drawBeat(cc, iv, xx);
		// 	break;
	}
	return iui;
}

void drawInstrument(ControlState *cc)
{
	switch (w->mode)
	{
		case MODE_INSERT:
			if (cc->mouseadjust || cc->keyadjust) printf("\033[%d;0H\033[1m-- INSERT ADJUST --\033[m\033[4 q", ws.ws_row);
			else                                  printf("\033[%d;0H\033[1m-- INSERT --\033[m\033[6 q",        ws.ws_row);
			w->command.error[0] = '\0';
			break;
		default:
			if (cc->mouseadjust || cc->keyadjust) { printf("\033[%d;0H\033[1m-- ADJUST --\033[m\033[4 q", ws.ws_row); w->command.error[0] = '\0'; }
			break;
	}

	short x = drawInstrumentIndex(1, 1, ws.ws_col) + 2;

	if (instrumentSafe(s->instrument, w->instrument))
	{
		if (w->showfilebrowser)
			drawBrowser(fbstate);
		else
		{
			Instrument *iv = &s->instrument->v[s->instrument->i[w->instrument]];
			InstUI *iui = initInstrumentUI(iv);

			clearControls(cc);
			short y = TRACK_ROW+1;
			if (iui->flags&INSTUI_DRAWWAVEFORM)
			{
				drawWaveform(iv);
				y += w->waveformh>>2;
			}

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
			if (!rows) goto drawInstrumentEnd;

			short iui_x = x + ((ws.ws_col - x - (cols*(iui->width + INSTUI_PADDING) - INSTUI_PADDING))>>1);

			size_t ei = 0;
			for (uint8_t i = 0; i < iui->blocks; i++)
				for (uint8_t j = 0; j < iui->block[i].count; j++)
				{
					iui->block[i].callback(iui_x + (ei/rows)*(iui->width + INSTUI_PADDING), y + ei%rows, iv, j);
					ei++;
				}

drawInstrumentEnd:
			// drawInstrumentSampler(cc, iv, x, ws.ws_col - x);
			drawControls(cc);
		}
	} else
	{
	}
}
