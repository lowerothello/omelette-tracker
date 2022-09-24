#define I_MODE_INDICES 0
#define I_MODE_NORMAL 1
#define I_MODE_PREVIEW 2
#define I_MODE_VISUAL 5
#define I_MODE_INDICES_PREVIEW 6

#define MAX_INSTRUMENT_INDEX 15

#define INSTRUMENT_BODY_COLS 70
#define INSTRUMENT_BODY_ROWS 20
#define INSTRUMENT_TYPE_ROWS 14

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
					if (iv->triggerflash) printf("\033[%d;1H\033[7m\033[2m %02x %02x [%08x] \033[22m\033[27m", w->centre - w->instrument + i, i, s->instrumenti[i], iv->length);
					else                  printf("\033[%d;1H\033[7m %02x %02x \033[1m[%08x]\033[22m \033[27m", w->centre - w->instrument + i, i, s->instrumenti[i], iv->length);
				} else    printf("\033[%d;1H\033[7m %02x %02x  ........  \033[27m",                            w->centre - w->instrument + i, i, s->instrumenti[i]);
			} else
			{
				if (s->instrumenti[i] < s->instrumentc)
				{
					iv = &s->instrumentv[s->instrumenti[i]];
					if (iv->triggerflash) printf("\033[%d;1H\033[2m %02x %02x [%08x]\033[22m ",                w->centre - w->instrument + i, i, s->instrumenti[i], iv->length);
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
		addControl(&cc, x+13, y+2, &iv->channels,    1, 0,   4,          6);
			setControlPrettyName(&cc, "stereo");
			setControlPrettyName(&cc, "  left");
			setControlPrettyName(&cc, " right");
			setControlPrettyName(&cc, "   mix");
			setControlPrettyName(&cc, "  swap");
		addControl(&cc, x+14, y+3, &iv->envelope,    2, 0x0, 0xff,       0);
		addControl(&cc, x+18, y+3, &iv->sustain,     0, 0,   1,          0);
		addControl(&cc, x+32, y+0, &iv->gain,        2, 0x0, 0xff,       0);
		addControl(&cc, x+32, y+0, &iv->invert,      0, 0,   1,          0);
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

void sampleApplyTrimming(instrument *iv)
{
	pushInstrumentHistoryIfNew(iv);
	if (iv->samplelength > 0)
	{
		uint32_t newlen
			= MAX(iv->trim[0], iv->trim[1])
			- MIN(iv->trim[0], iv->trim[1]) + 1;

		// malloc a new buffer
		short *sampledata = malloc(sizeof(short) * newlen * iv->channels);
		if (sampledata == NULL)
		{
			strcpy(w->command.error, "failed to apply trim, out of memory");
			return;
		}

		uint32_t startOffset = MIN(iv->trim[0], iv->trim[1]);
		memcpy(sampledata,
				iv->sampledata+(sizeof(short) * startOffset),
				sizeof(short) * newlen * iv->channels);

		free(iv->sampledata); iv->sampledata = NULL;
		iv->sampledata = sampledata;
		iv->samplelength = newlen * iv->channels;
		iv->length = newlen;
		iv->trim[0] = iv->trim[0] - startOffset;
		iv->trim[1] = iv->trim[1] - startOffset;
	}
}
int sampleExportCallback(char *command, unsigned char *mode)
{
	if (s->instrumenti[w->instrument] >= s->instrumentc) return 1;
	instrument *iv = &s->instrumentv[s->instrumenti[w->instrument]];

	sampleApplyTrimming(iv);
	if (!iv->sampledata) return 1;

	char *buffer = malloc(strlen(command) + 1);
	wordSplit(buffer, command, 0);

	SNDFILE *sndfile;
	SF_INFO sfinfo;
	memset(&sfinfo, 0, sizeof(sfinfo));

	sfinfo.samplerate = iv->c5rate;
	sfinfo.frames = iv->length;
	sfinfo.channels = iv->channels;

	sfinfo.format = SF_FORMAT_WAV | SF_FORMAT_PCM_16;
	sndfile = sf_open(fileExtension(buffer, ".wav"), SFM_WRITE, &sfinfo);
	if (sndfile == NULL) { free(buffer); return 1; }

	// write the sample data to disk
	sf_writef_short(sndfile, iv->sampledata, iv->length);
	sf_close(sndfile);

	free(buffer);
	return 0;
}
void resetWaveform(void)
{
	if (s->instrumenti[w->instrument] < s->instrumentc)
	{
		w->waveformwidth = s->instrumentv[s->instrumenti[w->instrument]].length;
		w->waveformcursor = 0;
		w->waveformdrawpointer = 0;
		redraw();
	}
}
void sampleLoadCallback(char *path)
{
	loadSample(w->instrument, path);
	pushInstrumentHistory(&s->instrumentv[s->instrumenti[w->instrument]]);
	w->popup = 1;
	w->mode = w->oldmode;
	resetWaveform();
}

void instrumentUpArrow(int count)
{
	switch (w->mode)
	{
		case I_MODE_INDICES: case I_MODE_INDICES_PREVIEW:
			w->instrument -= count;
			if (w->instrument < 0) w->instrument = 0;
			resetWaveform();
			break;
		case I_MODE_VISUAL: w->mode = I_MODE_NORMAL; w->waveformdrawpointer = 0; redraw();
		case I_MODE_NORMAL: case I_MODE_PREVIEW:
			decControlCursor(&cc, count);
			if (!cc.cursor) w->waveformdrawpointer = 0;
			break;
	}
	if (s->instrumenti[w->instrument] < s->instrumentc)
		pushInstrumentHistoryIfNew(&s->instrumentv[s->instrumenti[w->instrument]]);
}
void instrumentDownArrow(int count)
{
	switch (w->mode)
	{
		case I_MODE_INDICES: case I_MODE_INDICES_PREVIEW:
			w->instrument += count;
			if (w->instrument > 254) w->instrument = 254;
			resetWaveform();
			break;
		case I_MODE_VISUAL: w->mode = I_MODE_NORMAL; w->waveformdrawpointer = 0; redraw();
		case I_MODE_NORMAL: case I_MODE_PREVIEW:
			incControlCursor(&cc, count);
			break;
	}
	if (s->instrumenti[w->instrument] < s->instrumentc)
		pushInstrumentHistoryIfNew(&s->instrumentv[s->instrumenti[w->instrument]]);
}
void instrumentLeftArrow(void)
{
	uint32_t delta;
	switch (w->mode)
	{
		case I_MODE_NORMAL: case I_MODE_PREVIEW: case I_MODE_VISUAL:
			if (!cc.cursor)
			{
				if (s->instrumenti[w->instrument] < s->instrumentc)
				{
					delta = w->waveformwidth / WAVEFORM_COARSE_SLICES;
					if (delta > w->waveformcursor) w->waveformcursor = 0;
					else                           w->waveformcursor -= delta;
					w->waveformdrawpointer = 0; redraw();
				}
			} else incControlFieldpointer(&cc);
			break;
	}
	if (s->instrumenti[w->instrument] < s->instrumentc)
		pushInstrumentHistoryIfNew(&s->instrumentv[s->instrumenti[w->instrument]]);
}
void instrumentRightArrow(void)
{
	uint32_t delta;
	switch (w->mode)
	{
		case I_MODE_NORMAL: case I_MODE_PREVIEW: case I_MODE_VISUAL:
			if (!cc.cursor)
			{
				if (s->instrumenti[w->instrument] < s->instrumentc)
				{
					delta = w->waveformwidth / WAVEFORM_COARSE_SLICES;
					if (delta > s->instrumentv[s->instrumenti[w->instrument]].length-1 - w->waveformcursor)
						w->waveformcursor = s->instrumentv[s->instrumenti[w->instrument]].length-1;
					else
						w->waveformcursor += delta;
					w->waveformdrawpointer = 0; redraw();
				}
			} else decControlFieldpointer(&cc);
			break;
	}
	if (s->instrumenti[w->instrument] < s->instrumentc)
		pushInstrumentHistoryIfNew(&s->instrumentv[s->instrumenti[w->instrument]]);
}
void instrumentCtrlLeftArrow(void)
{
	uint32_t delta;
	if (!cc.cursor && s->instrumenti[w->instrument] < s->instrumentc)
		switch (w->mode)
		{
			case I_MODE_NORMAL: case I_MODE_PREVIEW: case I_MODE_VISUAL:
				delta = w->waveformwidth / WAVEFORM_FINE_SLICES;
				if (delta > w->waveformcursor) w->waveformcursor = 0;
				else                           w->waveformcursor -= delta;
				w->waveformdrawpointer = 0; redraw(); break;
		}
}
void instrumentCtrlRightArrow(void)
{
	uint32_t delta;
	if (!cc.cursor && s->instrumenti[w->instrument] < s->instrumentc)
		switch (w->mode)
		{
			case I_MODE_NORMAL: case I_MODE_PREVIEW: case I_MODE_VISUAL:
				delta = w->waveformwidth / WAVEFORM_FINE_SLICES;
				if (delta > s->instrumentv[s->instrumenti[w->instrument]].length-1 - w->waveformcursor)
					w->waveformcursor = s->instrumentv[s->instrumenti[w->instrument]].length-1;
				else
					w->waveformcursor += delta;
				w->waveformdrawpointer = 0; redraw(); break;
		}
}
void instrumentHome(void)
{
	switch (w->mode)
	{
		case I_MODE_INDICES: case I_MODE_INDICES_PREVIEW:
			w->instrument = 0;
			resetWaveform();
			break;
		case I_MODE_NORMAL: case I_MODE_PREVIEW: case I_MODE_VISUAL:
			if (cc.cursor) setControlCursor(&cc, 1);
			else if (s->instrumenti[w->instrument] < s->instrumentc)
			{
				w->waveformcursor = 0;
				w->waveformdrawpointer = 0;
			} redraw(); break;
	}
	if (s->instrumenti[w->instrument] < s->instrumentc)
		pushInstrumentHistoryIfNew(&s->instrumentv[s->instrumenti[w->instrument]]);
}
void instrumentEnd(void)
{
	switch (w->mode)
	{
		case I_MODE_INDICES: case I_MODE_INDICES_PREVIEW:
			w->instrument = 254;
			resetWaveform();
			break;
		case I_MODE_NORMAL: case I_MODE_PREVIEW: case I_MODE_VISUAL:
			if (cc.cursor) setControlCursor(&cc, cc.controlc-1);
			else if (s->instrumenti[w->instrument] < s->instrumentc)
			{
				w->waveformcursor = s->instrumentv[s->instrumenti[w->instrument]].length-1;
				w->waveformdrawpointer = 0;
			} redraw(); break;
	}
	if (s->instrumenti[w->instrument] < s->instrumentc)
		pushInstrumentHistoryIfNew(&s->instrumentv[s->instrumenti[w->instrument]]);
}

void instrumentModeToIndices(void)
{
	switch (w->mode)
	{
		case I_MODE_PREVIEW: case I_MODE_INDICES_PREVIEW:
			w->mode = I_MODE_INDICES_PREVIEW; break;
		default:
			w->mode = I_MODE_INDICES; break;
	}
}
void instrumentModeToNormal(void)
{
	switch (w->mode)
	{
		case I_MODE_PREVIEW: case I_MODE_INDICES_PREVIEW:
			w->mode = I_MODE_PREVIEW; break;
		case I_MODE_INDICES:
			w->mode = I_MODE_NORMAL; break;
	}
}

void toggleRecording(uint8_t inst, char cue)
{
	if (w->instrumentrecv == INST_REC_LOCK_OK) w->instrumentreci = inst;
	if (w->instrumentreci == inst)
	{
		switch (w->instrumentrecv)
		{
			case INST_REC_LOCK_OK:
				w->recbuffer = malloc(sizeof(short) * RECORD_LENGTH * samplerate * 2);
				if (!w->recbuffer)
				{
					strcpy(w->command.error, "failed to start recording, out of memory");
					break;
				}
				w->recptr = 0;
				if (cue) w->instrumentrecv = INST_REC_LOCK_CUE_START;
				else     w->instrumentrecv = INST_REC_LOCK_START;
				break;
			default: w->instrumentrecv = INST_REC_LOCK_PREP_END; break;
		}
	} redraw();
}
void recordBinds(short instrument, int input)
{
	switch (input)
	{
		case 'R': case 'r': /* start/stop */
			if (w->instrumentrecv != INST_REC_LOCK_OK && w->instrumentreci != instrument)
			{ /* stop whichever instrument is already recording */
				toggleRecording(w->instrumentreci, 0);
			} else
			{
				if (s->instrumenti[instrument] >= s->instrumentc) addInstrument(instrument);
				toggleRecording(instrument, 0);
			} break;
		case 'Q': case 'q': /* start/stop cue */
			if (w->instrumentrecv != INST_REC_LOCK_OK && w->instrumentreci != instrument)
			{ /* stop whichever instrument is already recording */
				toggleRecording(w->instrumentreci, 1);
			} else
			{
				if (s->instrumenti[instrument] >= s->instrumentc) addInstrument(instrument);
				toggleRecording(instrument, 1);
			} break;
		case 'C': case 'c': /* cancel */
			if (w->instrumentrecv != INST_REC_LOCK_OK)
				w->instrumentrecv = INST_REC_LOCK_PREP_CANCEL;
			break;
	}
}


void instrumentInput(int input)
{
	instrument *iv;
	int button, x, y;
	unsigned short yo;
	if (s->instrumenti[w->instrument] >= s->instrumentc) setControlCursor(&cc, 0);
	switch (input)
	{
		case '\033':
			switch (getchar())
			{
				case 'O':
					if (s->instrumenti[w->instrument] < s->instrumentc)
						pushInstrumentHistoryIfNew(&s->instrumentv[s->instrumenti[w->instrument]]);
					previewNote(' ', INST_VOID, w->channel);
					switch (getchar())
					{
						case 'P': /* xterm f1 */ showTracker(); break;
						case 'Q': /* xterm f2 */ showInstrument(); break;
					} redraw(); break;
				case '[':
					switch (getchar())
					{
						case '[':
							if (s->instrumenti[w->instrument] < s->instrumentc)
								pushInstrumentHistoryIfNew(&s->instrumentv[s->instrumenti[w->instrument]]);
							previewNote(' ', INST_VOID, w->channel);
							switch (getchar())
							{
								case 'A': /* linux f1 */ showTracker(); break;
								case 'B': /* linux f2 */ showInstrument(); break;
								case 'E': /* linux f5 */ startPlayback(); break;
							} redraw(); break;
						case 'A': /* up arrow    */ instrumentUpArrow(1);   redraw(); break;
						case 'B': /* down arrow  */ instrumentDownArrow(1); redraw(); break;
						case 'D': /* left arrow  */ instrumentLeftArrow();  redraw(); break;
						case 'C': /* right arrow */ instrumentRightArrow(); redraw(); break;
						case '1': /* mod+arrow / f5 - f8 */
							switch (getchar())
							{
								case '5': /* xterm f5, play  */ startPlayback(); getchar(); break;
								case '7': /* f6, stop        */
									previewNote(' ', INST_VOID, w->channel);
									stopPlayback(); getchar(); break;
								case ';': /* mod+arrow */
									switch (getchar())
									{
										case '5': /* ctrl+arrow */
											switch (getchar())
											{
												case 'D': /* left  */ instrumentCtrlLeftArrow();  redraw(); break;
												case 'C': /* right */ instrumentCtrlRightArrow(); redraw(); break;
											} break;
									} break;
								case '~': /* linux home */ instrumentHome(); redraw(); break;
							} break;
						case 'H': /* xterm home */ instrumentHome(); redraw(); break;
						case '4': /* end        */ if (getchar() == '~') { instrumentEnd(); redraw(); } break;
						case '5':
							switch (getchar())
							{
								case '~': /* page up        */ instrumentUpArrow(ws.ws_row>>1); redraw(); break;
								case ';': /* shift+scrollup */ getchar(); break;
							} break;
						case '6':
							switch (getchar())
							{
								case '~': /* page down      */ instrumentDownArrow(ws.ws_row>>1); redraw(); break;
								case ';': /* shift+scrolldn */ getchar(); break;
							} break;
						case 'M': /* mouse */
							button = getchar();
							x = getchar() - 32;
							y = getchar() - 32;
							yo = ws.ws_row - INSTRUMENT_CONTROL_ROW;
							if (s->instrumenti[w->instrument] < s->instrumentc && y > CHANNEL_ROW-2 && x >= INSTRUMENT_INDEX_COLS)
							{
								instrumentModeToNormal();
								iv = &s->instrumentv[s->instrumenti[w->instrument]];
								if (y >= yo)
								{
									mouseControls(&cc, button, x, y);
									pushInstrumentHistoryIfNew(iv);
								}
								else
								{
									setControlCursor(&cc, 0);
									uint32_t offset;
									switch (button)
									{
										case WHEEL_UP: case WHEEL_UP_CTRL:
											w->waveformwidth /= 2;
											w->waveformdrawpointer = 0;
											break;
										case WHEEL_DOWN: case WHEEL_DOWN_CTRL:
											w->waveformwidth = MIN(iv->length, w->waveformwidth*2);
											w->waveformdrawpointer = 0;
											break;
										case BUTTON1: case BUTTON1_CTRL:
											if (w->mode == I_MODE_VISUAL) w->mode = I_MODE_NORMAL;

											if (w->waveformcursor < (w->waveformwidth>>1))                   offset = 0;
											else if (w->waveformcursor > iv->length - (w->waveformwidth>>1)) offset = (iv->length - w->waveformwidth)<<1;
											else                                                             offset = (w->waveformcursor<<1) - w->waveformwidth;

											w->waveformcursor = MIN(iv->length-1, (uint32_t)((offset>>2) + (float)(x - INSTRUMENT_INDEX_COLS) / (float)w->waveformw * w->waveformwidth)<<1);
											w->waveformdrawpointer = 0;
											break;
										case BUTTON3: case BUTTON3_CTRL:
											if (w->mode != I_MODE_VISUAL) { w->mode = I_MODE_VISUAL; w->waveformvisual = w->waveformcursor; }

											if (w->waveformcursor < (w->waveformwidth>>1))                   offset = 0;
											else if (w->waveformcursor > iv->length - (w->waveformwidth>>1)) offset = (iv->length - w->waveformwidth)<<1;
											else                                                             offset = (w->waveformcursor<<1) - w->waveformwidth;

											w->waveformcursor = MIN(iv->length-1, (uint32_t)((offset>>2) + (float)(x - INSTRUMENT_INDEX_COLS) / (float)w->waveformw * w->waveformwidth)<<1);
											w->waveformdrawpointer = 0;
											break;
									}
								}
							} else
							{
								instrumentModeToIndices();
								switch (button)
								{
									case WHEEL_UP: case WHEEL_UP_CTRL:
										if (w->instrument > WHEEL_SPEED) w->instrument -= WHEEL_SPEED;
										else                             w->instrument = 0;
										resetWaveform(); break;
									case WHEEL_DOWN: case WHEEL_DOWN_CTRL:
										if (w->instrument < 254 - WHEEL_SPEED) w->instrument += WHEEL_SPEED;
										else                                   w->instrument = 254;
										resetWaveform(); break;
									case BUTTON_RELEASE: case BUTTON_RELEASE_CTRL:
										switch (w->mode)
										{
											case I_MODE_INDICES: case I_MODE_INDICES_PREVIEW:
												if ((short)w->instrument + w->fyoffset < 0)        w->instrument = 0;
												else if ((short)w->instrument + w->fyoffset > 254) w->instrument = 254;
												else                                               w->instrument += w->fyoffset;
												resetWaveform();
												w->fyoffset = 0;
												break;
										} break;
									default:
										if (s->instrumenti[w->instrument] < s->instrumentc)
											pushInstrumentHistoryIfNew(&s->instrumentv[s->instrumenti[w->instrument]]);
										if (y <= CHANNEL_ROW-2)
										{
											if (x < (ws.ws_col-17) / 2 + 7) showTracker();
											else                            showInstrument();
											break;
										}

										switch (button)
										{
											case BUTTON2: case BUTTON2_CTRL:
												if (!(w->instrumentrecv != INST_REC_LOCK_OK && w->instrumentreci == w->instrument + y - w->centre))
												{
													yankInstrument(w->instrument + y - w->centre);
													delInstrument (w->instrument + y - w->centre);
												}
											case BUTTON1: case BUTTON1_CTRL:
												w->fyoffset = y - w->centre;
												break;
											case BUTTON3: case BUTTON3_CTRL:
												if ((short)w->instrument + (y - w->centre) < 0)        w->instrument = 0;
												else if ((short)w->instrument + (y - w->centre) > 254) w->instrument = 254;
												else                                                   w->instrument += y - w->centre;

												if (s->instrumenti[w->instrument] >= s->instrumentc) addInstrument(w->instrument);
												w->popup = 2;
												w->fyoffset = 0;
												w->oldmode = w->mode;
												w->mode = 0;
												w->filebrowserCallback = &sampleLoadCallback;
												break;
										}
										previewNote(' ', INST_VOID, w->channel);
										break;
								}
							} redraw(); break;
					} break;
				default:
					previewNote(' ', INST_VOID, w->channel);
					cc.mouseadjust = cc.keyadjust = 0;
					switch (w->mode)
					{
						case I_MODE_VISUAL: w->waveformdrawpointer = 0; /* fall through */
						case I_MODE_PREVIEW:
							w->mode = I_MODE_NORMAL; break;
						case I_MODE_INDICES_PREVIEW:
							w->mode = I_MODE_INDICES; break;
					} redraw(); break;
			} break;
		default:
			switch (w->mode)
			{
				case I_MODE_INDICES:
					switch (input)
					{ /* set count first */
						case '0': w->count *= 10; w->count += 0; redraw(); return;
						case '1': w->count *= 10; w->count += 1; redraw(); return;
						case '2': w->count *= 10; w->count += 2; redraw(); return;
						case '3': w->count *= 10; w->count += 3; redraw(); return;
						case '4': w->count *= 10; w->count += 4; redraw(); return;
						case '5': w->count *= 10; w->count += 5; redraw(); return;
						case '6': w->count *= 10; w->count += 6; redraw(); return;
						case '7': w->count *= 10; w->count += 7; redraw(); return;
						case '8': w->count *= 10; w->count += 8; redraw(); return;
						case '9': w->count *= 10; w->count += 9; redraw(); return;
						default:
							if (w->chord)
							{
								w->count = MIN(256, w->count);
								switch (w->chord)
								{
									case 'r': /* record */ recordBinds(w->instrument, input); break;
									case 'K': /* keyboard macro */ /* ignores w->count */
										w->keyboardmacro = '\0';
										if (input != 'k' && input != 'K') changeMacro(input, &w->keyboardmacro);
										break;
								} w->count = 0; redraw();
							} else
							{
								switch (input)
								{
									case '\t': /* not indices    */ w->mode = I_MODE_NORMAL; resetWaveform(); redraw(); break;
									case 'i':  /* preview        */ setControlCursor(&cc, 0); w->mode = I_MODE_INDICES_PREVIEW; redraw(); break;
									case 'r':  /* record         */ w->chord = 'r'; redraw(); return;
									case 'K':  /* keyboard macro */ w->chord = 'K'; redraw(); return;
									case 'k':  /* up arrow       */ instrumentUpArrow(1);   redraw(); break;
									case 'j':  /* down arrow     */ instrumentDownArrow(1); redraw(); break;
									case 'h':  /* left arrow     */ instrumentLeftArrow();  redraw(); break;
									case 'l':  /* right arrow    */ instrumentRightArrow(); redraw(); break;
									case 'a':  /* add            */
										if (s->instrumenti[w->instrument] >= s->instrumentc) { addInstrument(w->instrument); redraw(); }
										else                                                 { previewNote('a', w->instrument, w->channel); }
										break;
									case ' ': if (s->instrumenti[w->instrument] < s->instrumentc) previewNote(' ', w->instrument, w->channel); break;
									case 'd': case 'x': case 127: case '\b': /* delete */
										if (!(w->instrumentrecv != INST_REC_LOCK_OK && w->instrumentreci == w->instrument) && s->instrumenti[w->instrument] < s->instrumentc)
										{
											yankInstrument(w->instrument);
											delInstrument (w->instrument);
										} redraw(); break;
									case 'y':  /* yank    */ yankInstrument(w->instrument); redraw(); break;
									case 'p':  /* put     */
										if (!(w->instrumentrecv != INST_REC_LOCK_OK && w->instrumentreci == w->instrument))
										{
											if (s->instrumenti[w->instrument] >= s->instrumentc) addInstrument(w->instrument);
											w->instrumentlocki = s->instrumenti[w->instrument];
											w->instrumentlockv = INST_GLOBAL_LOCK_PREP_PUT;
										} redraw(); break;
									case '\n': case '\r':
										if (s->instrumenti[w->instrument] >= s->instrumentc) addInstrument(w->instrument);
										w->popup = 2;
										w->fyoffset = 0;
										w->oldmode = w->mode;
										w->mode = 0;
										w->filebrowserCallback = &sampleLoadCallback;
										redraw(); break;
									case 'e': /* export   */
										setCommand(&w->command, &sampleExportCallback, NULL, NULL, 0, "File name: ", "");
										w->mode = 255;
										redraw(); break;
								}
							} break;
					} break;
				case I_MODE_VISUAL:
					switch (input)
					{ /* set count first */
						case '0': w->count *= 10; w->count += 0; redraw(); return;
						case '1': w->count *= 10; w->count += 1; redraw(); return;
						case '2': w->count *= 10; w->count += 2; redraw(); return;
						case '3': w->count *= 10; w->count += 3; redraw(); return;
						case '4': w->count *= 10; w->count += 4; redraw(); return;
						case '5': w->count *= 10; w->count += 5; redraw(); return;
						case '6': w->count *= 10; w->count += 6; redraw(); return;
						case '7': w->count *= 10; w->count += 7; redraw(); return;
						case '8': w->count *= 10; w->count += 8; redraw(); return;
						case '9': w->count *= 10; w->count += 9; redraw(); return;
						default:
							if (w->chord)
							{
								w->count = MIN(256, w->count);
								switch (w->chord)
								{
									case 'z':
										switch (input)
										{
											case 'z': /* reset zoom */ if (s->instrumenti[w->instrument] < s->instrumentc) { w->waveformwidth = s->instrumentv[s->instrumenti[w->instrument]].length; w->waveformdrawpointer = 0; } break;
											case 'i': /* zoom in    */ if (s->instrumenti[w->instrument] < s->instrumentc) { w->waveformwidth /= 2; w->waveformdrawpointer = 0; } break;
											case 'o': /* zoom out   */ if (s->instrumenti[w->instrument] < s->instrumentc) { w->waveformwidth = MIN(s->instrumentv[s->instrumenti[w->instrument]].length, w->waveformwidth*2); w->waveformdrawpointer = 0; } break;
										} w->waveformdrawpointer = 0; redraw(); break;
								} w->count = 0; redraw();
							} else
								switch (input)
								{
									case 'v':  /* normal           */ w->mode = I_MODE_NORMAL; redraw(); break;
									case '\t': /* indices          */ w->mode = I_MODE_INDICES; resetWaveform(); redraw(); break;
									case 'i':  /* preview          */ w->mode = I_MODE_PREVIEW; w->waveformdrawpointer = 0; redraw(); break;
									case 'z':  /* zoom             */ if (!cc.cursor) w->chord = 'z'; redraw(); return;
									case '+': case '=': /* zoom in */ if (s->instrumenti[w->instrument] < s->instrumentc) { w->waveformwidth /= 2; w->waveformdrawpointer = 0; redraw(); } break;
									case '-':  /* zoom out         */ if (s->instrumenti[w->instrument] < s->instrumentc) { w->waveformwidth = MIN(s->instrumentv[s->instrumenti[w->instrument]].length, w->waveformwidth*2); w->waveformdrawpointer = 0; redraw(); } break;
									case 'k':  /* up arrow         */ instrumentUpArrow(1);   redraw(); break;
									case 'j':  /* down arrow       */ instrumentDownArrow(1); redraw(); break;
									case 'h':  /* left arrow       */ instrumentLeftArrow();  redraw(); break;
									case 'l':  /* right arrow      */ instrumentRightArrow(); redraw(); break;
									case 't':  /* trim             */
										if (s->instrumenti[w->instrument] < s->instrumentc)
										{
											s->instrumentv[s->instrumenti[w->instrument]].trim[0] = MIN(w->waveformcursor, w->waveformvisual);
											s->instrumentv[s->instrumenti[w->instrument]].trim[1] = MAX(w->waveformcursor, w->waveformvisual);
											w->mode = I_MODE_NORMAL; w->waveformdrawpointer = 0; redraw();
										} break;
								}
							break;
					} break;
				case I_MODE_NORMAL:
					if (!cc.cursor)
						switch (input)
						{ /* check counts first */
							case '0': w->count *= 10; w->count += 0; redraw(); return;
							case '1': w->count *= 10; w->count += 1; redraw(); return;
							case '2': w->count *= 10; w->count += 2; redraw(); return;
							case '3': w->count *= 10; w->count += 3; redraw(); return;
							case '4': w->count *= 10; w->count += 4; redraw(); return;
							case '5': w->count *= 10; w->count += 5; redraw(); return;
							case '6': w->count *= 10; w->count += 6; redraw(); return;
							case '7': w->count *= 10; w->count += 7; redraw(); return;
							case '8': w->count *= 10; w->count += 8; redraw(); return;
							case '9': w->count *= 10; w->count += 9; redraw(); return;
						}

					if (w->chord)
					{
						w->count = MIN(256, w->count);
						switch (w->chord)
						{
							case 'r': /* record */ recordBinds(w->instrument, input); break;
							case 'z': /* zoom   */
								switch (input)
								{
									case 'z': /* reset zoom */ if (s->instrumenti[w->instrument] < s->instrumentc) { w->waveformwidth = s->instrumentv[s->instrumenti[w->instrument]].length; w->waveformdrawpointer = 0; } break;
									case 'i': /* zoom in    */ if (s->instrumenti[w->instrument] < s->instrumentc) { w->waveformwidth /= 2; w->waveformdrawpointer = 0; } break;
									case 'o': /* zoom out   */ if (s->instrumenti[w->instrument] < s->instrumentc) { w->waveformwidth = MIN(s->instrumentv[s->instrumenti[w->instrument]].length, w->waveformwidth*2); w->waveformdrawpointer = 0; } break;
								} break;
							case 'K': /* keyboard macro */ /* ignores w->count */
								w->keyboardmacro = '\0';
								if (input != 'k' && input != 'K') changeMacro(input, &w->keyboardmacro);
								break;
							case 'd': /* delete */
								if (input == 'd')
								{
									if (s->instrumenti[w->instrument] < s->instrumentc)
									{
										if (s->instrumentv[s->instrumenti[w->instrument]].samplelength)
											free(s->instrumentv[s->instrumenti[w->instrument]].sampledata);
										s->instrumentv[s->instrumenti[w->instrument]].sampledata = NULL;
										s->instrumentv[s->instrumenti[w->instrument]].samplelength = 0;
										s->instrumentv[s->instrumenti[w->instrument]].channels = 0;
										s->instrumentv[s->instrumenti[w->instrument]].length = 0;
										s->instrumentv[s->instrumenti[w->instrument]].c5rate = 0;
										s->instrumentv[s->instrumenti[w->instrument]].trim[0] = 0;
										s->instrumentv[s->instrumenti[w->instrument]].trim[1] = 0;
										s->instrumentv[s->instrumenti[w->instrument]].loop = 0;
									}
								} break;
						} w->count = 0; redraw();
					} else
					{
						if (cc.cursor)
							switch (input)
							{
								case '\n': case '\r':             toggleKeyControl(&cc); redraw(); break;
								case '\t': /* indices          */ w->mode = I_MODE_INDICES; resetWaveform(); redraw(); break;
								case 'i':  /* preview          */ w->mode = I_MODE_PREVIEW; redraw(); break;
								case 'r':  /* record           */ w->chord = 'r'; redraw(); return;
								case 'K':  /* keyboard macro   */ w->chord = 'K'; redraw(); return;
								case 'u':  /* undo             */ if (s->instrumenti[w->instrument] < s->instrumentc) popInstrumentHistory(&s->instrumentv[s->instrumenti[w->instrument]], s->instrumenti[w->instrument]); w->waveformdrawpointer = 0; redraw(); break;
								case 18:   /* ^R redo          */ if (s->instrumenti[w->instrument] < s->instrumentc) unpopInstrumentHistory(&s->instrumentv[s->instrumenti[w->instrument]], s->instrumenti[w->instrument]); w->waveformdrawpointer = 0; redraw(); break;
								case 'k':  /* up arrow         */ instrumentUpArrow(1);   redraw(); break;
								case 'j':  /* down arrow       */ instrumentDownArrow(1); redraw(); break;
								case 'h':  /* left arrow       */ instrumentLeftArrow();  redraw(); break;
								case 'l':  /* right arrow      */ instrumentRightArrow(); redraw(); break;
								case 127: case '\b': /* backspace */ decControlFieldpointer(&cc); redraw(); break;
								case ' ':            /* space     */ incControlFieldpointer(&cc); redraw(); break;
								case 1:  /* ^a */
									if (s->instrumenti[w->instrument] < s->instrumentc)
									{
										incControlValue(&cc);
										redraw();
									} break;
								case 24: /* ^x */
									if (s->instrumenti[w->instrument] < s->instrumentc)
									{
										decControlValue(&cc);
										redraw();
									} break;
								case '0':           if (s->instrumenti[w->instrument] < s->instrumentc) { hexControlValue(&cc, 0); redraw();  } break;
								case '1':           if (s->instrumenti[w->instrument] < s->instrumentc) { hexControlValue(&cc, 1); redraw();  } break;
								case '2':           if (s->instrumenti[w->instrument] < s->instrumentc) { hexControlValue(&cc, 2); redraw();  } break;
								case '3':           if (s->instrumenti[w->instrument] < s->instrumentc) { hexControlValue(&cc, 3); redraw();  } break;
								case '4':           if (s->instrumenti[w->instrument] < s->instrumentc) { hexControlValue(&cc, 4); redraw();  } break;
								case '5':           if (s->instrumenti[w->instrument] < s->instrumentc) { hexControlValue(&cc, 5); redraw();  } break;
								case '6':           if (s->instrumenti[w->instrument] < s->instrumentc) { hexControlValue(&cc, 6); redraw();  } break;
								case '7':           if (s->instrumenti[w->instrument] < s->instrumentc) { hexControlValue(&cc, 7); redraw();  } break;
								case '8':           if (s->instrumenti[w->instrument] < s->instrumentc) { hexControlValue(&cc, 8); redraw();  } break;
								case '9':           if (s->instrumenti[w->instrument] < s->instrumentc) { hexControlValue(&cc, 9); redraw();  } break;
								case 'A': case 'a': if (s->instrumenti[w->instrument] < s->instrumentc) { hexControlValue(&cc, 10); redraw(); } break;
								case 'B': case 'b': if (s->instrumenti[w->instrument] < s->instrumentc) { hexControlValue(&cc, 11); redraw(); } break;
								case 'C': case 'c': if (s->instrumenti[w->instrument] < s->instrumentc) { hexControlValue(&cc, 12); redraw(); } break;
								case 'D': case 'd': if (s->instrumenti[w->instrument] < s->instrumentc) { hexControlValue(&cc, 13); redraw(); } break;
								case 'E': case 'e': if (s->instrumenti[w->instrument] < s->instrumentc) { hexControlValue(&cc, 14); redraw(); } break;
								case 'F': case 'f': if (s->instrumenti[w->instrument] < s->instrumentc) { hexControlValue(&cc, 15); redraw(); } break;
							}
						else
							switch (input)
							{
								case '\n': case '\r':
									if (s->instrumenti[w->instrument >= s->instrumentc])
										addInstrument(w->instrument);
									w->popup = 2;
									w->fyoffset = 0;
									w->oldmode = w->mode;
									w->mode = 0;
									w->filebrowserCallback = &sampleLoadCallback;
									redraw(); break;
								case 'v':  /* visual               */ w->mode = I_MODE_VISUAL; w->waveformvisual = w->waveformcursor; w->waveformdrawpointer = 0; redraw(); break;
								case 'z':  /* zoom                 */ w->chord = 'z'; redraw(); return;
								case '+': case '=': /* zoom in     */ if (s->instrumenti[w->instrument] < s->instrumentc) { w->waveformwidth /= 2; w->waveformdrawpointer = 0; redraw(); } break;
								case '-':  /* zoom out             */ if (s->instrumenti[w->instrument] < s->instrumentc) { w->waveformwidth = MIN(s->instrumentv[s->instrumenti[w->instrument]].length, w->waveformwidth*2); w->waveformdrawpointer = 0; redraw(); } break;
								case 's':  /* trim start to cursor */ if (s->instrumenti[w->instrument] < s->instrumentc) { s->instrumentv[s->instrumenti[w->instrument]].trim[0] = w->waveformcursor; pushInstrumentHistory(&s->instrumentv[s->instrumenti[w->instrument]]); w->waveformdrawpointer = 0; redraw(); } break;
								case 'e':  /* trim end to cursor   */ if (s->instrumenti[w->instrument] < s->instrumentc) { s->instrumentv[s->instrumenti[w->instrument]].trim[1] = w->waveformcursor; pushInstrumentHistory(&s->instrumentv[s->instrumenti[w->instrument]]); w->waveformdrawpointer = 0; redraw(); } break;
								case 'l':  /* loop to cursor       */ if (s->instrumenti[w->instrument] < s->instrumentc) { s->instrumentv[s->instrumenti[w->instrument]].loop = w->waveformcursor; pushInstrumentHistory(&s->instrumentv[s->instrumenti[w->instrument]]); w->waveformdrawpointer = 0; redraw(); } break;
								case 'd':  /* delete               */ w->chord = 'd'; redraw(); return;
								case '\t': /* indices              */ w->mode = I_MODE_INDICES; resetWaveform(); redraw(); break;
								case 'i':  /* preview              */ w->mode = I_MODE_PREVIEW; redraw(); break;
								case 'r':  /* record               */ w->chord = 'r'; redraw(); return;
								case 'K':  /* keyboard macro       */ w->chord = 'K'; redraw(); return;
								case 'u':  /* undo                 */ if (s->instrumenti[w->instrument] < s->instrumentc) { popInstrumentHistory(&s->instrumentv[s->instrumenti[w->instrument]], s->instrumenti[w->instrument]); w->waveformdrawpointer = 0; redraw(); } break;
								case 18:   /* ^R redo              */ if (s->instrumenti[w->instrument] < s->instrumentc) { unpopInstrumentHistory(&s->instrumentv[s->instrumenti[w->instrument]], s->instrumenti[w->instrument]); w->waveformdrawpointer = 0; redraw(); } break;
								// case 'k':  /* up arrow             */ if (s->instrumenti[w->instrument] < s->instrumentc) { instrumentUpArrow(1, &s->instrumentv[s->instrumenti[w->instrument]]); redraw();   } break;
								// case 'j':  /* down arrow           */ if (s->instrumenti[w->instrument] < s->instrumentc) { instrumentDownArrow(1, &s->instrumentv[s->instrumenti[w->instrument]]); redraw(); } break;
								// case 'h':  /* left arrow           */ if (s->instrumenti[w->instrument] < s->instrumentc) { instrumentLeftArrow(&s->instrumentv[s->instrumenti[w->instrument]]); redraw();    } break;
								// case 'l':  /* right arrow          */ if (s->instrumenti[w->instrument] < s->instrumentc) { instrumentRightArrow(is->instrumentv[s->instrumenti[w->instrument]]); redraw();   } break; /* conflicts with setting the loop mark */
								case 'a': if (s->instrumenti[w->instrument] < s->instrumentc) previewNote('a', w->instrument, w->channel); break;
								case ' ': if (s->instrumenti[w->instrument] < s->instrumentc) previewNote(' ', w->instrument, w->channel); break;
							}
					} break;
				case I_MODE_PREVIEW: case I_MODE_INDICES_PREVIEW:
					switch (input)
					{
						case '\n': case '\r':
							switch (w->mode)
							{
								case I_MODE_PREVIEW:
									if (!cc.cursor)
									{
										if (s->instrumenti[w->instrument] >= s->instrumentc) addInstrument(w->instrument);
										w->popup = 2;
										w->fyoffset = 0;
										w->oldmode = I_MODE_NORMAL;
										w->mode = 0;
										w->filebrowserCallback = &sampleLoadCallback;
									} else toggleKeyControl(&cc);
									redraw(); break;
								case I_MODE_INDICES_PREVIEW:
									if (s->instrumenti[w->instrument] >= s->instrumentc) addInstrument(w->instrument);
									w->popup = 2;
									w->fyoffset = 0;
									w->oldmode = I_MODE_NORMAL;
									w->mode = 0;
									w->filebrowserCallback = &sampleLoadCallback;
									redraw(); break;
							} break;
						case '\t':
							switch (w->mode)
							{
								case I_MODE_PREVIEW: w->mode = I_MODE_INDICES_PREVIEW; break;
								case I_MODE_INDICES_PREVIEW: w->mode = I_MODE_PREVIEW; break;
							} resetWaveform(); redraw(); break;
						case '0': w->octave = 0; redraw(); break;
						case '1': w->octave = 1; redraw(); break;
						case '2': w->octave = 2; redraw(); break;
						case '3': w->octave = 3; redraw(); break;
						case '4': w->octave = 4; redraw(); break;
						case '5': w->octave = 5; redraw(); break;
						case '6': w->octave = 6; redraw(); break;
						case '7': w->octave = 7; redraw(); break;
						case '8': w->octave = 8; redraw(); break;
						case '9': w->octave = 9; redraw(); break;
						default: previewNote(input, w->instrument, w->channel); break;
					} break;
			} break;
	}
	if (w->count) { w->count = 0; redraw(); }
	if (w->chord) { w->chord = '\0'; redraw(); }
}
