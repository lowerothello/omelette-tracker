#include "types/sampler.c"

#define I_MODE_INDICES 0
#define I_MODE_INDICES_PREVIEW 1
#define I_MODE_NORMAL 2
#define I_MODE_PREVIEW 3
#define I_MODE_ADJUST 4
#define I_MODE_MOUSEADJUST 5

#define MAX_INSTRUMENT_INDEX 10

#define INSTRUMENT_BODY_COLS 70
#define INSTRUMENT_BODY_ROWS 20
#define INSTRUMENT_TYPE_ROWS 14


void resizeWaveform(void)
{
	instrument *iv = s->instrumentv[s->instrumenti[w->instrument]];
	if (iv->samplelength)
	{
		fill(w->waveformcanvas, 0);

		/* draw sample data */
		uint32_t maxj = w->waveformw * w->waveformh * WAVEFORM_OVERSAMPLING;
		for (uint8_t i = 0; i < iv->channels; i++) /* mix all channels */
			for (uint32_t j = 0; j < maxj; j++)
			{
				size_t k = (float)j / maxj * w->waveformwidth;
				size_t x = ((float)k / w->waveformwidth) * w->waveformw;
				float sample = iv->sampledata[w->waveformoffset + k * iv->channels + i]*DIVSHRT;
				float offset = (w->waveformh-2)*0.5; // leave space for the scrollbar
				set_pixel_unsafe(w->waveformcanvas, 1, x, sample * offset + offset);
			}

		/* draw scrollbar */
		/* size_t scrollbarstart = w->waveformoffset / (float)iv->length * w->waveformw;
		size_t scrollbarend = scrollbarstart + w->waveformwidth / (float)iv->length * w->waveformw;
		for (size_t i = scrollbarstart; i < scrollbarend; i++)
			set_pixel_unsafe(w->waveformcanvas, 1, i, w->waveformh-1); */
	}
}

void drawInstrument(void)
{
	printf("\033[%d;%dH\033[2mPATTERN\033[m \033[1mINSTRUMENT\033[m", CHANNEL_ROW-2, (ws.ws_col-18) / 2);
	switch (w->mode)
	{
		case I_MODE_INDICES: case I_MODE_NORMAL:          printf("\033[0 q"); break;
		case I_MODE_PREVIEW: case I_MODE_INDICES_PREVIEW: printf("\033[%d;0H\033[1m-- PREVIEW --\033[m\033[3 q", ws.ws_row); w->command.error[0] = '\0'; break;
		case I_MODE_ADJUST:  case I_MODE_MOUSEADJUST:     printf("\033[%d;0H\033[1m-- ADJUST --\033[m\033[0 q", ws.ws_row); w->command.error[0] = '\0'; break;
	}

	int i;
	instrument *iv;

	for (i = 0; i < 255; i++)
		if (w->centre - w->instrument + i > CHANNEL_ROW && w->centre - w->instrument + i < ws.ws_row)
		{
			switch (w->mode)
			{
				case I_MODE_INDICES: case I_MODE_INDICES_PREVIEW: break;
				default: if (w->instrument == i) printf("\033[7m"); break;
			}

			iv = s->instrumentv[s->instrumenti[i]];
			if (iv) printf("\033[%d;%dH %02x \033[1m[%08x]\033[m ", w->centre - w->instrument + i, 1, i, iv->length);
			else    printf("\033[%d;%dH %02x  ........  ",          w->centre - w->instrument + i, 1, i);
			printf("\033[m");
		}

	iv = s->instrumentv[s->instrumenti[w->instrument]];
	if (iv)
	{
		if (!iv->samplelength)
		{
			printf("\033[%d;%dH [NO SAMPLE] \033[10D", w->centre, INSTRUMENT_INDEX_COLS + (ws.ws_col - INSTRUMENT_INDEX_COLS - 13) / 2);
		} else
		{
			if (w->waveformbuffer)
			{
				w->waveformoffset = 0;
				w->waveformwidth = iv->length;
				resizeWaveform();
				draw(w->waveformcanvas, w->waveformbuffer);
				for (size_t i = 0; w->waveformbuffer[i] != NULL; i++)
					printf("\033[%ld;%dH%s", CHANNEL_ROW + i, INSTRUMENT_INDEX_COLS, w->waveformbuffer[i]);
			} else resize(0);
		}

		unsigned short x = INSTRUMENT_INDEX_COLS + (ws.ws_col - INSTRUMENT_INDEX_COLS - 58) / 2;
		unsigned short y = ws.ws_row - INSTRUMENT_CONTROL_ROW;
		printf("\033[%d;%dHC-5 rate:  [%08x]                                     ",           y+0, x, iv->c5rate);
		printf("\033[%d;%dHdecimate:     [%x][%02x]  ┌─   LOOP   ─┐  ┌─  TIMESTRETCH  ─┐",    y+1, x, iv->bitdepth, iv->samplerate);
		printf("\033[%d;%dHchannels:    [      ]  ping-pong: ", y+2, x);
		drawBit(iv->flags&S_FLAG_PPLOOP); printf("  cycle size:  [%04x]", iv->cyclelength);
		printf("\033[%d;%dHenv a[d,r]/s: [%02x][ ]  ramping:  [%02x]  time/pitch: ", y+3, x, 0, iv->loopramp);
		drawBit(iv->flags&S_FLAG_TTEMPO); printf("[%02x]", iv->pitchshift);

		switch (w->instrumentindex)
		{
			case 0:  printf("\033[%d;%dH", y+0, x+19 - w->fieldpointer); break;
			case 1:  printf("\033[%d;%dH", y+1, x+15); break;
			case 2:  printf("\033[%d;%dH", y+1, x+19 - w->fieldpointer); break;
			case 3:  printf("\033[%d;%dH", y+2, x+19); break;
			case 4:  printf("\033[%d;%dH", y+3, x+16 - w->fieldpointer); break;
			case 5:  printf("\033[%d;%dH", y+3, x+19); break;
			case 6:  printf("\033[%d;%dH", y+2, x+35); break;
			case 7:  printf("\033[%d;%dH", y+3, x+35 - w->fieldpointer); break;
			case 8:  printf("\033[%d;%dH", y+2, x+56 - w->fieldpointer); break;
			case 9:  printf("\033[%d;%dH", y+3, x+52); break;
			case 10: printf("\033[%d;%dH", y+3, x+56 - w->fieldpointer); break;
		}
	} else printf("\033[%d;%dH [NOT ADDED] ", w->centre, INSTRUMENT_INDEX_COLS + (ws.ws_col - INSTRUMENT_INDEX_COLS - 13) / 2);

	if (w->mode == I_MODE_INDICES || w->mode == I_MODE_INDICES_PREVIEW)
		printf("\033[%d;%dH", w->centre + w->fyoffset, 6);
}

void instrumentIncFieldPointer(short index)
{
	switch (index)
	{
		case 0: w->fieldpointer--; if (w->fieldpointer < 0) w->fieldpointer = 7; break;
		case 8: w->fieldpointer--; if (w->fieldpointer < 0) w->fieldpointer = 3; break;
		default: w->fieldpointer = 0; break;
	}
}
void instrumentDecFieldPointer(short index)
{
	switch (index)
	{
		case 0: w->fieldpointer++; if (w->fieldpointer > 7) w->fieldpointer = 0; break;
		case 8: w->fieldpointer++; if (w->fieldpointer > 3) w->fieldpointer = 0; break;
		default: w->fieldpointer = 1; break;
	}
}
void inputInstrumentHex(unsigned short index, instrument *iv, char value)
{
	switch (index)
	{
		case 0:  updateField(w->fieldpointer, (uint32_t *)&iv->c5rate, value); break;
		case 1:  iv->bitdepth = value; break;
		case 2:  updateFieldPush(&iv->samplerate, value); break;
		case 4:  break; // a[d,r]
		case 7:  updateFieldPush(&iv->loopramp, value); break;
		case 8:  updateField(w->fieldpointer, (uint32_t *)&iv->cyclelength, value); break;
		case 10: updateFieldPush(&iv->pitchshift, value); break;
	} instrumentIncFieldPointer(index);
}

void instrumentAdjustUp(instrument *iv, short index, char mouse)
{
	uint32_t temp;
	if (!mouse)
		switch (index)
		{
			case 0:  incField(w->fieldpointer, &iv->c5rate, 0xffffffff); break;
			case 1:  if (iv->bitdepth < 15) iv->bitdepth++; break;
			case 2:  if (w->fieldpointer) { if (iv->samplerate < 255 - 16) iv->samplerate+=16; else iv->samplerate = 255; } else if (iv->samplerate < 255) iv->samplerate++; break;
			case 7:  if (w->fieldpointer) { if (iv->loopramp < 255 - 16) iv->loopramp+=16; else iv->loopramp = 255; } else if (iv->loopramp < 255) iv->loopramp++; break;
			case 8:  temp = iv->cyclelength; incField(w->fieldpointer, &temp, 0xffff); iv->cyclelength = temp; break;
			case 10: if (w->fieldpointer) { if (iv->pitchshift < 255 - 16) iv->pitchshift+=16; else iv->pitchshift = 255; } else if (iv->pitchshift < 255) iv->pitchshift++; break;
		}
}
void instrumentAdjustDown(instrument *iv, short index, char mouse)
{
	uint32_t temp;
	if (!mouse)
		switch (index)
		{
			case 0:  decField(w->fieldpointer, &iv->c5rate); break;
			case 1:  if (iv->bitdepth) iv->bitdepth--; break;
			case 2:  if (w->fieldpointer) { if (iv->samplerate > 16) iv->samplerate-=16; else iv->samplerate = 0; } else if (iv->samplerate) iv->samplerate--; break;
			case 7:  if (w->fieldpointer) { if (iv->loopramp > 16) iv->loopramp-=16; else iv->loopramp = 0; } else if (iv->loopramp) iv->loopramp--; break;
			case 8:  temp = iv->cyclelength; decField(w->fieldpointer, &temp); iv->cyclelength = temp; break;
			case 10: if (w->fieldpointer) { if (iv->pitchshift > 16) iv->pitchshift-=16; else iv->pitchshift = 0; } else if (iv->pitchshift) iv->pitchshift--; break;
		}
}
void instrumentAdjustLeft (instrument *iv, short index, char mouse)
{
	uint32_t temp;
	if (mouse)
		switch (index)
		{
			case 0:  decField(w->fieldpointer, &iv->c5rate); break;
			case 1:  if (iv->bitdepth) iv->bitdepth--; break;
			case 2:  if (w->fieldpointer) { if (iv->samplerate > 16) iv->samplerate-=16; else iv->samplerate = 0; } else if (iv->samplerate) iv->samplerate--; break;
			case 7:  if (w->fieldpointer) { if (iv->loopramp > 16) iv->loopramp-=16; else iv->loopramp = 0; } else if (iv->loopramp) iv->loopramp--; break;
			case 8:  temp = iv->cyclelength; decField(w->fieldpointer, &temp); iv->cyclelength = temp; break;
			case 10: if (w->fieldpointer) { if (iv->pitchshift > 16) iv->pitchshift-=16; else iv->pitchshift = 0; } else if (iv->pitchshift) iv->pitchshift--; break;
		}
	else instrumentDecFieldPointer(index);
}
void instrumentAdjustRight(instrument *iv, short index, char mouse)
{
	uint32_t temp;
	if (mouse)
		switch (index)
		{
			case 0:  incField(w->fieldpointer, &iv->c5rate, 0xffffffff); break;
			case 1:  if (iv->bitdepth < 15) iv->bitdepth++; break;
			case 2:  if (w->fieldpointer) { if (iv->samplerate < 255 - 16) iv->samplerate+=16; else iv->samplerate = 255; } else if (iv->samplerate < 255) iv->samplerate++; break;
			case 7:  if (w->fieldpointer) { if (iv->loopramp < 255 - 16) iv->loopramp+=16; else iv->loopramp = 255; } else if (iv->loopramp < 255) iv->loopramp++; break;
			case 8:  temp = iv->cyclelength; incField(w->fieldpointer, &temp, 0xffff); iv->cyclelength = temp; break;
			case 10: if (w->fieldpointer) { if (iv->pitchshift < 255 - 16) iv->pitchshift+=16; else iv->pitchshift = 255; } else if (iv->pitchshift < 255) iv->pitchshift++; break;
		}
	else instrumentIncFieldPointer(index);
}

void sampleApplyTrimming(instrument *iv)
{
	pushInstrumentHistoryIfNew(iv);
	if (iv->samplelength > 0)
	{
		uint32_t newlen
			= MAX(iv->trim[0], iv->trim[1])
			- MIN(iv->trim[0], iv->trim[1]);

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
	char *buffer = malloc(strlen(command) + 1);
	wordSplit(buffer, command, 0);
	sampleApplyTrimming(s->instrumentv[s->instrumenti[w->instrument]]);

	instrument *iv = s->instrumentv[s->instrumenti[w->instrument]];
	if (!iv->sampledata) { free(buffer); return 1; }

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
void sampleLoadCallback(char *path)
{
	loadSample(w->instrument, path);
	pushInstrumentHistory(s->instrumentv[s->instrumenti[w->instrument]]);
	w->popup = 1;
	w->mode = 0;
}


void instrumentInput(int input)
{
	instrument *iv = s->instrumentv[s->instrumenti[w->instrument]];
	int button, x, y;
	if (!s->instrumenti[w->instrument])
		w->instrumentindex = 0;
	previewNote(255, 255, w->channel);
	switch (input)
	{
		case '\033':
			switch (getchar())
			{
				case 'O':
					pushInstrumentHistoryIfNew(s->instrumentv[s->instrumenti[w->instrument]]);
					handleFKeys(getchar());
					redraw(); break;
				case '[':
					switch (getchar())
					{
						case 'A': /* up arrow */
							switch (w->mode)
							{
								case I_MODE_INDICES: case I_MODE_INDICES_PREVIEW:
									if (w->instrumentrecv == INST_REC_LOCK_OK && w->instrument)
										w->instrument--;
									break;
								case I_MODE_NORMAL: case I_MODE_PREVIEW:
									if (s->instrumentv[s->instrumenti[w->instrument]] && w->instrumentindex)
										w->instrumentindex--;
									w->fieldpointer = 0;
									break;
								case I_MODE_ADJUST: case I_MODE_MOUSEADJUST:
									instrumentAdjustUp(iv, w->instrumentindex, 0);
									break;
							} redraw(); break;
						case 'B': /* down arrow */
							switch (w->mode)
							{
								case I_MODE_INDICES: case I_MODE_INDICES_PREVIEW:
									if (w->instrumentrecv == INST_REC_LOCK_OK && w->instrument < 254)
										w->instrument++;
									break;
								case I_MODE_NORMAL: case I_MODE_PREVIEW:
									if (s->instrumentv[s->instrumenti[w->instrument]] && w->instrumentindex < MAX_INSTRUMENT_INDEX)
										w->instrumentindex++;
									w->fieldpointer = 0;
									break;
								case I_MODE_ADJUST: case I_MODE_MOUSEADJUST:
									instrumentAdjustDown(iv, w->instrumentindex, 0);
									break;
							} redraw(); break;
						case 'D': /* left arrow  */
							switch (w->mode)
							{
								case I_MODE_NORMAL: case I_MODE_PREVIEW:
								case I_MODE_ADJUST: case I_MODE_MOUSEADJUST:
									instrumentAdjustLeft(iv, w->instrumentindex, 0);
									break;
							} redraw(); break;
						case 'C': /* right arrow */
							switch (w->mode)
							{
								case I_MODE_NORMAL: case I_MODE_PREVIEW:
								case I_MODE_ADJUST: case I_MODE_MOUSEADJUST:
									instrumentAdjustRight(iv, w->instrumentindex, 0);
									break;
							} redraw(); break;
						case '1': /* mod+arrow / f5 - f8 */
							switch (getchar())
							{
								case '5': /* f5, play  */ startPlayback(); getchar(); break;
								case '7': /* f6, stop  */ stopPlayback(); getchar(); break;
								case ';': /* mod+arrow */ getchar(); getchar(); break;
							} break;
						case 'H': /* home */
							switch (w->mode)
							{
								case I_MODE_INDICES: case I_MODE_INDICES_PREVIEW:
									if (w->instrumentrecv == INST_REC_LOCK_OK)
										w->instrument = 0;
									break;
								case I_MODE_NORMAL: case I_MODE_PREVIEW:
									w->instrumentindex = 0;
									w->fieldpointer = 0;
									break;
							} redraw(); break;
						case '4': /* end  */
							if (getchar() == '~')
								switch (w->mode)
								{
									case I_MODE_INDICES: case I_MODE_INDICES_PREVIEW:
										if (w->instrumentrecv == INST_REC_LOCK_OK)
											w->instrument = 254;
										break;
									case I_MODE_NORMAL: case I_MODE_PREVIEW:
										w->instrumentindex = MAX_INSTRUMENT_INDEX;
										w->fieldpointer = 0;
										break;
								} redraw();
							break;
						case '5': /* page up   */ getchar(); redraw(); break;
						case '6': /* page down */ getchar(); redraw(); break;
						case 'M': /* mouse */
							button = getchar();
							x = getchar() - 32;
							y = getchar() - 32;
							switch (button)
							{
								case WHEEL_UP:   case WHEEL_UP_CTRL:   /* scroll up   */
									if (w->instrument > WHEEL_SPEED) w->instrument -= WHEEL_SPEED;
									else                             w->instrument = 0;
									break;
								case WHEEL_DOWN: case WHEEL_DOWN_CTRL: /* scroll down */
									if (w->instrument < 254 - WHEEL_SPEED) w->instrument += WHEEL_SPEED;
									else                                   w->instrument = 254;
									break;
								case BUTTON_RELEASE: case BUTTON_RELEASE_CTRL: /* release click */
									switch (w->mode)
									{
										case I_MODE_INDICES: case I_MODE_INDICES_PREVIEW:
											if ((short)w->instrument + w->fyoffset < 0)        w->instrument = 0;
											else if ((short)w->instrument + w->fyoffset > 254) w->instrument = 254;
											else                                               w->instrument += w->fyoffset;
											w->fyoffset = 0;
											break;
										case I_MODE_MOUSEADJUST: w->mode = w->oldmode; break;
									} break;
								case BUTTON1_HOLD: case BUTTON1_HOLD_CTRL:
									if (w->mode == I_MODE_MOUSEADJUST)
									{
										if      (x > w->mousex) instrumentAdjustRight(iv, w->instrumentindex, 1);
										else if (x < w->mousex) instrumentAdjustLeft(iv, w->instrumentindex, 1);
										if      (y > w->mousey) instrumentAdjustDown(iv, w->instrumentindex, 1);
										else if (y < w->mousey) instrumentAdjustUp(iv, w->instrumentindex, 1);
										w->mousey = y; w->mousex = x;
									} break;
								case BUTTON1:      case BUTTON3:
								case BUTTON1_CTRL: case BUTTON3_CTRL:
									if (y == CHANNEL_ROW-2)
									{
										if (x < (ws.ws_col-17) / 2 + 7) handleFKeys('P');
										else                            handleFKeys('Q');
										break;
									}
									pushInstrumentHistoryIfNew(s->instrumentv[s->instrumenti[w->instrument]]);

									if (x < INSTRUMENT_INDEX_COLS)
									{
										switch (w->mode)
										{
											case I_MODE_INDICES: case I_MODE_INDICES_PREVIEW: break;
											case I_MODE_PREVIEW: w->mode = I_MODE_INDICES_PREVIEW; break;
											default: w->mode = I_MODE_INDICES; break;
										} w->fyoffset = y - w->centre;
									} else
									{
										switch (w->mode)
										{
											case I_MODE_INDICES: w->mode = I_MODE_NORMAL; break;
											case I_MODE_INDICES_PREVIEW: w->mode = I_MODE_PREVIEW; break;
										}
										unsigned short xo = INSTRUMENT_INDEX_COLS + (ws.ws_col - INSTRUMENT_INDEX_COLS - 58) / 2 -1;
										unsigned short yo = ws.ws_row - INSTRUMENT_CONTROL_ROW;
										if (y >= yo)
											switch (y - yo)
											{
												case 0:
													w->instrumentindex = 0;
													if (x - xo < 13)      w->fieldpointer = 7;
													else if (x - xo > 20) w->fieldpointer = 0;
													else                  w->fieldpointer = (x - xo - 13)*-1 + 7;
													break;
												case 1:
													if (x - xo < 18) w->instrumentindex = 1;
													else
													{
														w->instrumentindex = 2;
														if (x - xo < 20) w->fieldpointer = 1;
														else             w->fieldpointer = 0;
													} break;
												case 2:
													if (x - xo < 23)        w->instrumentindex = 3;
													else if (x - xo < 39) { w->instrumentindex = 6; iv->flags ^= S_FLAG_PPLOOP; }
													else
													{
														w->instrumentindex = 8;
														if (x - xo < 54)      w->fieldpointer = 3;
														else if (x - xo > 57) w->fieldpointer = 0;
														else                  w->fieldpointer = (x - xo - 54)*-1 + 3;
													} break;
												case 3:
													if (x - xo < 19) // TODO: envelope
													{
														w->instrumentindex = 4;
														if (x - xo < 17) w->fieldpointer = 1;
														else             w->fieldpointer = 0;
													} else if (x - xo < 23) { w->instrumentindex = 5; } // TODO: envelope
													else if (x - xo < 39)
													{
														w->instrumentindex = 7;
														if (x - xo < 36) w->fieldpointer = 1;
														else             w->fieldpointer = 0;
													} else if (x - xo < 55) { w->fieldpointer = 9; iv->flags ^= S_FLAG_TTEMPO; }
													else
													{
														w->instrumentindex = 10;
														if (x - xo < 57) w->fieldpointer = 1;
														else             w->fieldpointer = 0;
													} break;
											}

										/* enter mouseadjust mode */
										w->oldmode = w->mode;
										w->mode = I_MODE_MOUSEADJUST;
										w->mousey = y; w->mousex = x;
									} break;
							} redraw(); break;
					} break;
				default:
					switch (w->mode)
					{
						case I_MODE_INDICES_PREVIEW:             w->mode = I_MODE_INDICES; break;
						case I_MODE_PREVIEW: case I_MODE_ADJUST: w->mode = I_MODE_NORMAL; w->fieldpointer = 0; break;
						case I_MODE_MOUSEADJUST:                 w->mode = w->oldmode; break;
					} redraw(); break;
			} break;
		default:
			switch (w->mode)
			{
				case I_MODE_INDICES:
					switch (input)
					{
						case '\t': /* normal  */ w->mode = I_MODE_NORMAL; w->fieldpointer = 0; redraw(); break;
						case 'i':  /* preview */ w->mode = I_MODE_INDICES_PREVIEW; redraw(); break;
						case 'a':  /* add     */ if (!s->instrumenti[w->instrument]) addInstrument(w->instrument); redraw(); break;
						case 'd':  /* delete  */
							if (w->instrumentrecv == INST_REC_LOCK_OK && s->instrumenti[w->instrument])
							{
								yankInstrument(w->instrument);
								delInstrument(w->instrument);
							} redraw(); break;
						case 'y':  /* yank    */ yankInstrument(w->instrument); redraw(); break;
						case 'p':  /* put     */
							if (w->instrumentrecv == INST_REC_LOCK_OK)
							{
								if (!s->instrumenti[w->instrument]) addInstrument(w->instrument);
								w->instrumentlocki = s->instrumenti[w->instrument];
								w->instrumentlockv = INST_GLOBAL_LOCK_PREP_PUT;
							} redraw(); break;
						case '\n': case '\r':
							if (!s->instrumenti[w->instrument]) addInstrument(w->instrument);
							w->popup = 2;
							w->fyoffset = 0;
							w->filebrowserCallback = &sampleLoadCallback;
							redraw(); break;
						case 'e': /* export */
							setCommand(&w->command, &sampleExportCallback, NULL, NULL, 0, "File name: ", "");
							w->mode = 255;
							redraw(); break;
					}
					break;
				case I_MODE_NORMAL:
					if (w->chord)
						switch (w->chord)
						{
							case 'r':
								switch (input)
								{
									case 's': /* start/stop */ toggleRecording(w->instrument); break;
									case 'c': /* cancel     */
										if (w->instrumentrecv != INST_REC_LOCK_OK)
											w->instrumentrecv = INST_REC_LOCK_PREP_CANCEL;
										break;
								}
						}
					else
					{
						instrument *iv = s->instrumentv[s->instrumenti[w->instrument]];
						switch (input)
						{
							case '\n': case '\r':
								switch (w->instrumentindex)
								{
									case 5: break; // sustain
									case 6: iv->flags ^= S_FLAG_PPLOOP; break;
									case 9: iv->flags ^= S_FLAG_TTEMPO; break;
									default: w->mode = I_MODE_ADJUST; break;
								}
								redraw(); break;
							case '\t': /* indices */ w->mode = I_MODE_INDICES; redraw(); break;
							case 'i':  /* preview */ w->mode = I_MODE_PREVIEW; redraw(); break;
							case 'r':  /* record  */ w->chord = 'r'; redraw(); goto i_afterchordunset;
							case 'u':  /* undo    */ popInstrumentHistory(s->instrumenti[w->instrument]); redraw(); break;
							case 18:   /* ^R redo */ unpopInstrumentHistory(s->instrumenti[w->instrument]); redraw(); break;
							case 127: case 8: /* backspace */ if (w->instrumentindex >= 0 && iv) instrumentDecFieldPointer(w->instrumentindex); redraw(); break;
							case ' ':         /* space     */ if (w->instrumentindex >= 0 && iv) instrumentIncFieldPointer(w->instrumentindex); redraw(); break;
							case 1:  /* ^a */
								switch (w->instrumentindex)
								{
									case 0:  iv->c5rate++; break;
									case 1:  iv->bitdepth++; break;
									case 2:  iv->samplerate++; break;
									case 3:  break; // channels
									case 4:  break; // a[d,r]
									case 7:  iv->loopramp++; break;
									case 8:  iv->cyclelength++; break;
									case 10: iv->pitchshift++; break;
								} redraw(); break;
							case 24: /* ^x */
								switch (w->instrumentindex)
								{
									case 0:  iv->c5rate--; break;
									case 1:  iv->bitdepth--; break;
									case 2:  iv->samplerate--; break;
									case 3:  break; // channels
									case 4:  break; // a[d,r]
									case 7:  iv->loopramp--; break;
									case 8:  iv->cyclelength--; break;
									case 10: iv->pitchshift--; break;
								} redraw(); break;
							case '0':           inputInstrumentHex(w->instrumentindex, iv, 0);  redraw(); break;
							case '1':           inputInstrumentHex(w->instrumentindex, iv, 1);  redraw(); break;
							case '2':           inputInstrumentHex(w->instrumentindex, iv, 2);  redraw(); break;
							case '3':           inputInstrumentHex(w->instrumentindex, iv, 3);  redraw(); break;
							case '4':           inputInstrumentHex(w->instrumentindex, iv, 4);  redraw(); break;
							case '5':           inputInstrumentHex(w->instrumentindex, iv, 5);  redraw(); break;
							case '6':           inputInstrumentHex(w->instrumentindex, iv, 6);  redraw(); break;
							case '7':           inputInstrumentHex(w->instrumentindex, iv, 7);  redraw(); break;
							case '8':           inputInstrumentHex(w->instrumentindex, iv, 8);  redraw(); break;
							case '9':           inputInstrumentHex(w->instrumentindex, iv, 9);  redraw(); break;
							case 'A': case 'a': inputInstrumentHex(w->instrumentindex, iv, 10); redraw(); break;
							case 'B': case 'b': inputInstrumentHex(w->instrumentindex, iv, 11); redraw(); break;
							case 'C': case 'c': inputInstrumentHex(w->instrumentindex, iv, 12); redraw(); break;
							case 'D': case 'd': inputInstrumentHex(w->instrumentindex, iv, 13); redraw(); break;
							case 'E': case 'e': inputInstrumentHex(w->instrumentindex, iv, 14); redraw(); break;
							case 'F': case 'f': inputInstrumentHex(w->instrumentindex, iv, 15); redraw(); break;
						}
					} break;
				case I_MODE_ADJUST: case I_MODE_MOUSEADJUST:
					switch (input)
					{
						case '\n': case '\r': w->mode = I_MODE_NORMAL; redraw(); break;
					}
					break;
				case I_MODE_PREVIEW: case I_MODE_INDICES_PREVIEW:
					switch (input)
					{
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
						default:
							previewNote(charToNote(input), w->instrument, w->channel);
							break;
					} break;
			} break;
	}
	if (w->chord) { w->chord = '\0'; redraw(); }
i_afterchordunset:
}
