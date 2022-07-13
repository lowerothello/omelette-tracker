#include "types/sampler.c"

/* mode makes more sense as raw numbers than as declares in this file, imo */

#define MIN_INSTRUMENT_INDEX -5

#define INSTRUMENT_BODY_COLS 70
#define INSTRUMENT_BODY_ROWS 20
#define INSTRUMENT_TYPE_ROWS 14


void drawRecordSource(uint8_t source, unsigned short y, unsigned short x, char adjust)
{
	switch (source)
	{
		case 0:
			if (adjust)
			{
				printf(   "\033[%d;%dH [ line in] ", y+0, x);
				printf(   "\033[%d;%dH  loopback  ", y+1, x);
			} else printf("\033[%d;%dH [ line in] ", y+0, x);
			break;
		case 1:
			if (adjust)
			{
				printf(   "\033[%d;%dH   line in  ", y-1, x);
				printf(   "\033[%d;%dH [loopback] ", y+0, x);
			} else printf("\033[%d;%dH [loopback] ", y+0, x);
			break;
	}
}

void drawInstrument(void)
{
	printf("\033[%d;%dH\033[2mPATTERN\033[m \033[1mINSTRUMENT\033[m", CHANNEL_ROW-2, (ws.ws_col-18) / 2);
	switch (w->mode)
	{
		case 1: printf("\033[%d;0H\033[1m-- PREVIEW --\033[m\033[3 q", ws.ws_row); w->command.error[0] = '\0'; break;
		case 2: printf("\033[%d;0H\033[1m-- ADJUST --\033[m\033[0 q", ws.ws_row); w->command.error[0] = '\0'; break;
		case 3: printf("\033[%d;0H\033[1m-- PREVIEW ADJUST --\033[m\033[0 q", ws.ws_row); w->command.error[0] = '\0'; break;
		case 4: printf("\033[%d;0H\033[1m-- MOUSE ADJUST --\033[m\033[0 q", ws.ws_row); w->command.error[0] = '\0'; break;
		case 5: printf("\033[%d;0H\033[1m-- PREVIEW MOUSE ADJUST --\033[m\033[0 q", ws.ws_row); w->command.error[0] = '\0'; break;
		default: printf("\033[0 q"); break;
	}

	unsigned short x = w->instrumentcelloffset; unsigned short y = w->instrumentrowoffset;

	for (unsigned short cy = y; cy < y+7+INSTRUMENT_TYPE_ROWS; cy++)
		printf("\033[%d;%dH\033[%dX", cy, x, INSTRUMENT_BODY_COLS);

	printf("\033[%d;%dH  <- INSTRUMENT (%02x) ->  ", y+0, x+(INSTRUMENT_BODY_COLS-26) / 2, w->instrument);
	printf("\033[%d;%dH┌\033[%d;%dH┐", y+5,x+1, y+5,x+INSTRUMENT_BODY_COLS-2);
	printf("\033[%d;%dH└\033[%d;%dH┘", y+6+INSTRUMENT_TYPE_ROWS,x+1, y+6+INSTRUMENT_TYPE_ROWS,x+INSTRUMENT_BODY_COLS-2);
	for (int i = 0; i < INSTRUMENT_BODY_COLS-4; i++) printf("\033[%d;%dH─\033[%d;%dH─", y+5,x+2+i, y+6+INSTRUMENT_TYPE_ROWS, x+2+i);
	for (int i = 0; i < INSTRUMENT_TYPE_ROWS; i++)   printf("\033[%d;%dH│\033[%d;%dH│", y+6+i,x+1, y+6+i,x+INSTRUMENT_BODY_COLS-2);

	if (!s->instrumenti[w->instrument])
	{
		printf("\033[%d;%dH(not added)", y+2, x+(INSTRUMENT_BODY_COLS-12) / 2);
		printf("\033[%d;%dH", y+0, x+(INSTRUMENT_BODY_COLS-16) / 2);
	} else
	{
		instrument *iv = s->instrumentv[s->instrumenti[w->instrument]];
		printf("\033[%d;%dHvolume [%02x]", y+3, x+2, iv->defgain);
		printf("\033[%d;%dH\033[1mRECORD\033[m", y+1, x+INSTRUMENT_BODY_COLS-18);
		printf("\033[%d;%dHrecord source   ", y+2, x+INSTRUMENT_BODY_COLS-28);
		printf("\033[%d;%dHonly loop back cursor  ", y+3, x+INSTRUMENT_BODY_COLS-28);
		drawBit(w->recordflags & 0b1);
		printf("\033[%d;%dHgate the next pattern  ", y+4, x+INSTRUMENT_BODY_COLS-28);
		drawBit(w->recordflags & 0b10);
		drawRecordSource(w->recordsource, y+2, x+INSTRUMENT_BODY_COLS-13, w->instrumentindex == MIN_INSTRUMENT_INDEX + 2 && w->mode > 1 && w->mode < 255);

		if (w->mode > 1 && w->mode < 6) drawSampler(iv, w->instrument, x+(INSTRUMENT_BODY_COLS - samplercellwidth) / 2, y+6, &w->instrumentindex, 1);
		else                            drawSampler(iv, w->instrument, x+(INSTRUMENT_BODY_COLS - samplercellwidth) / 2, y+6, &w->instrumentindex, 0);

		switch (w->instrumentindex)
		{
			case MIN_INSTRUMENT_INDEX + 0: printf("\033[%d;%dH", y+0, x+(INSTRUMENT_BODY_COLS-16) / 2); break;
			case MIN_INSTRUMENT_INDEX + 1: printf("\033[%d;%dH", y+3, x+11); break;
			case MIN_INSTRUMENT_INDEX + 2: printf("\033[%d;%dH", y+2, x+INSTRUMENT_BODY_COLS-4); break;
			case MIN_INSTRUMENT_INDEX + 3: printf("\033[%d;%dH", y+3, x+INSTRUMENT_BODY_COLS-4); break;
			case MIN_INSTRUMENT_INDEX + 4: printf("\033[%d;%dH", y+4, x+INSTRUMENT_BODY_COLS-4); break;
		}
	}
}

void instrumentAdjustUp(instrument *iv, short index, char mouse)
{
	switch (index)
	{
		case MIN_INSTRUMENT_INDEX:
			break;
		case MIN_INSTRUMENT_INDEX + 1:
			if (iv)
			{
				if (iv->defgain%16 < 15)
					iv->defgain++;
				if (iv->defgain>>4 < 15)
					iv->defgain+=16;
			} break;
		case MIN_INSTRUMENT_INDEX + 2:
			if (w->recordsource < 1) w->recordsource++;
			break;
		case MIN_INSTRUMENT_INDEX + 3:
		case MIN_INSTRUMENT_INDEX + 4:
			break;
		default:
			samplerAdjustUp(iv, index, mouse);
			break;
	}
}
void instrumentAdjustDown(instrument *iv, short index, char mouse)
{
	switch (index)
	{
		case MIN_INSTRUMENT_INDEX:
			break;
		case MIN_INSTRUMENT_INDEX + 1:
			if (iv)
			{
				if (iv->defgain%16 > 0)
					iv->defgain--;
				if (iv->defgain>>4 > 0)
					iv->defgain-=16;
			} break;
		case MIN_INSTRUMENT_INDEX + 2:
			if (w->recordsource > 0) w->recordsource--;
			break;
		case MIN_INSTRUMENT_INDEX + 3:
		case MIN_INSTRUMENT_INDEX + 4:
			break;
		default:
			samplerAdjustDown(iv, index, mouse);
			break;
	}
}
void instrumentAdjustLeft(instrument *iv, short index, char mouse)
{
	switch (index)
	{
		case MIN_INSTRUMENT_INDEX:
			if (w->instrumentrecv == INST_REC_LOCK_OK)
				if (w->instrument > 0)
					w->instrument--;
			break;
		case MIN_INSTRUMENT_INDEX + 1:
			if (iv)
			{
				if (iv->defgain%16 > iv->defgain>>4)
					iv->defgain+=16;
				else if (iv->defgain%16 > 0)
					iv->defgain--;
			} break;
		case MIN_INSTRUMENT_INDEX + 2:
		case MIN_INSTRUMENT_INDEX + 3:
		case MIN_INSTRUMENT_INDEX + 4:
			break;
		default:
			samplerAdjustLeft(iv, index, mouse);
			break;
	}
}
void instrumentAdjustRight(instrument *iv, short index, char mouse)
{
	switch (index)
	{
		case MIN_INSTRUMENT_INDEX:
			if (w->instrumentrecv == INST_REC_LOCK_OK)
				if (w->instrument < 254)
					w->instrument++;
			break;
		case MIN_INSTRUMENT_INDEX + 1:
			if (iv)
			{
				if (iv->defgain>>4 > iv->defgain%16)
					iv->defgain++;
				else if (iv->defgain>>4 > 0)
					iv->defgain-=16;
			} break;
		case MIN_INSTRUMENT_INDEX + 2:
		case MIN_INSTRUMENT_INDEX + 3:
		case MIN_INSTRUMENT_INDEX + 4:
			break;
		default:
			samplerAdjustRight(iv, index, mouse);
			break;
	}
}

void instrumentInput(int input)
{
	instrument *iv;
	int button, x, y;
	if (!s->instrumenti[w->instrument])
		w->instrumentindex = MIN_INSTRUMENT_INDEX;
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
					iv = s->instrumentv[s->instrumenti[w->instrument]];
					switch (getchar())
					{
						case 'A': /* up arrow */
							if (w->mode > 1) /* adjust */
								instrumentAdjustUp(iv, w->instrumentindex, 0);
							else
							{
								pushInstrumentHistoryIfNew(s->instrumentv[s->instrumenti[w->instrument]]);
								w->instrumentindex--;
								if (w->instrumentindex < MIN_INSTRUMENT_INDEX)
									w->instrumentindex = MIN_INSTRUMENT_INDEX;
								/* ensure fieldpointer is still in range */
								switch (w->instrumentindex)
								{
									case MIN_INSTRUMENT_INDEX: break;
									case MIN_INSTRUMENT_INDEX + 1:
										if (w->fieldpointer > 3)
											w->fieldpointer = 3;
										break;
									case MIN_INSTRUMENT_INDEX + 2:
									case MIN_INSTRUMENT_INDEX + 3:
									case MIN_INSTRUMENT_INDEX + 4:
										break;
									default:
										if (w->instrumentindex >= 0 && iv)
											samplerEndFieldPointer(w->instrumentindex);
										break;
								}
							} redraw(); break;
						case 'B': /* down arrow */
							if (w->mode > 1) /* adjust */ instrumentAdjustDown(iv, w->instrumentindex, 0);
							else
							{
								pushInstrumentHistoryIfNew(s->instrumentv[s->instrumenti[w->instrument]]);
								if (s->instrumenti[w->instrument])
									w->instrumentindex++;
								unsigned short tic = 0; /* type index count */
								iv = s->instrumentv[s->instrumenti[w->instrument]];
								if (iv) tic = samplerindexc;
								if (w->instrumentindex > tic)
									w->instrumentindex = tic;
								/* ensure fieldpointer is still in range */
								switch (w->instrumentindex)
								{
									case MIN_INSTRUMENT_INDEX: break;
									case MIN_INSTRUMENT_INDEX + 1:
										if (w->fieldpointer > 3)
											w->fieldpointer = 3;
										break;
									case MIN_INSTRUMENT_INDEX + 2:
									case MIN_INSTRUMENT_INDEX + 3:
									case MIN_INSTRUMENT_INDEX + 4:
										break;
									default:
										if (w->instrumentindex >= 0 && iv)
											samplerEndFieldPointer(w->instrumentindex);
										break;
								}
							} redraw(); break;
						case 'D': /* left arrow */
							if (w->instrumentindex == MIN_INSTRUMENT_INDEX + 1)
							{
								if (w->fieldpointer == 0)
									w->fieldpointer = 3;
								else w->fieldpointer--;
							} else if (w->instrumentindex < 0)
								instrumentAdjustLeft(iv, w->instrumentindex, 0);
							else
							{
								if (iv)
									samplerDecFieldPointer(w->instrumentindex);
							} redraw(); break;
						case 'C': /* right arrow */
							if (w->instrumentindex == MIN_INSTRUMENT_INDEX + 1)
							{
								w->fieldpointer++;
								if (w->fieldpointer > 3)
									w->fieldpointer = 0;
							} else if (w->instrumentindex < 0)
								instrumentAdjustRight(iv, w->instrumentindex, 0);
							else
							{
								if (iv)
									samplerIncFieldPointer(w->instrumentindex);
							} redraw(); break;
						case '1': /* mod+arrow / f5 - f8 */
							switch (getchar())
							{
								case '5': /* f5, play */
									getchar(); /* extraneous tilde */
									startPlayback();
									break;
								case '7': /* f6, stop */
									getchar(); /* extraneous tilde */
									stopPlayback();
									break;
								case ';': /* mod+arrow */
									switch (getchar())
									{
										case '5': /* ctrl+arrow */
											switch (getchar())
											{
												case 'D': /* left */
													instrumentAdjustLeft(iv, MIN_INSTRUMENT_INDEX, 0);
													redraw();
													break;
												case 'C': /* right */
													instrumentAdjustRight(iv, MIN_INSTRUMENT_INDEX, 0);
													redraw();
													break;
											} break;
										default: getchar(); break;
									} break;
							} break;
						case 'H': /* home */
							pushInstrumentHistoryIfNew(s->instrumentv[s->instrumenti[w->instrument]]);
							w->fieldpointer = 0;
							redraw(); break;
						case '4': /* end */
							if (getchar() == '~')
							{
								pushInstrumentHistoryIfNew(s->instrumentv[s->instrumenti[w->instrument]]);
								switch (w->instrumentindex)
								{
									case MIN_INSTRUMENT_INDEX:
									case MIN_INSTRUMENT_INDEX + 2:
									case MIN_INSTRUMENT_INDEX + 3:
									case MIN_INSTRUMENT_INDEX + 4:
										w->fieldpointer = 0; break;
									case MIN_INSTRUMENT_INDEX + 1: w->fieldpointer = 3; break;
									default:
										if (iv)
											samplerEndFieldPointer(w->instrumentindex);
										break;
								} redraw();
							} break;
						case '5': /* page up */
							getchar();
							w->instrumentindex = MIN_INSTRUMENT_INDEX;
							redraw(); break;
						case '6': /* page down */
							getchar();
							w->instrumentindex = 0;
							redraw(); break;
						case 'M': /* mouse */
							button = getchar();
							x = getchar() - 32;
							y = getchar() - 32;
							switch (button)
							{
								case WHEEL_UP:   case WHEEL_UP_CTRL:   /* scroll up   */
								case WHEEL_DOWN: case WHEEL_DOWN_CTRL: /* scroll down */
									break;
								case BUTTON_RELEASE: case BUTTON_RELEASE_CTRL: /* release click */
									/* leave adjust mode */
									if (w->mode > 3) w->mode -= 4;
									if (w->mode > 1) w->mode -= 2;
									break;
								case BUTTON1_HOLD: case BUTTON1_HOLD_CTRL:
									if (w->mode > 3) /* mouse adjust */
									{
										if      (x > w->mousex) instrumentAdjustRight(iv, w->instrumentindex, 1);
										else if (x < w->mousex) instrumentAdjustLeft(iv, w->instrumentindex, 1);
										if      (y > w->mousey) instrumentAdjustDown(iv, w->instrumentindex, 1);
										else if (y < w->mousey) instrumentAdjustUp(iv, w->instrumentindex, 1);
										w->mousey = y; w->mousex = x;
									} break;
								case BUTTON1: case BUTTON3: case BUTTON1_CTRL: case BUTTON3_CTRL:
									if (y == CHANNEL_ROW-2)
									{
										if (x < (ws.ws_col-17) / 2 + 7) handleFKeys('P');
										else                            handleFKeys('Q');
										break;
									}
									pushInstrumentHistoryIfNew(s->instrumentv[s->instrumenti[w->instrument]]);
									switch (MAX(0, y - w->instrumentrowoffset))
									{
										case 0: /* index */
											w->fieldpointer = 0;
											w->instrumentindex = MIN_INSTRUMENT_INDEX;
											break;
										case 1: case 2: case 3:
											if (x - w->instrumentcelloffset < INSTRUMENT_BODY_COLS / 2)
												w->instrumentindex = MIN_INSTRUMENT_INDEX + 1;
											else
											{
												w->instrumentindex = MIN_INSTRUMENT_INDEX + 3;
												w->recordflags ^= 0b1;  break;
											}
											w->fieldpointer = 0;
											break;
										case 4:
											if (x - w->instrumentcelloffset < INSTRUMENT_BODY_COLS / 2)
												w->instrumentindex = MIN_INSTRUMENT_INDEX + 1;
											else
											{
												w->instrumentindex = MIN_INSTRUMENT_INDEX + 4;
												w->recordflags ^= 0b10; break;
											}
											w->fieldpointer = 0;
											break;
										default:
											if (!s->instrumenti[w->instrument]) break;
											samplerMouseToIndex(
													y - w->instrumentrowoffset - 5,
													x - w->instrumentcelloffset - (INSTRUMENT_BODY_COLS - samplercellwidth) / 2,
													button,
													&w->instrumentindex);
											break;
									}
									/* enter mouse adjust mode */
									if (w->mode < 2) w->mode = w->mode + 4;
									if (w->mode < 4) w->mode = w->mode + 2;
									w->mousey = y; w->mousex = x;
									break;
							} redraw(); break;
					} break;
				/* alt+numbers, change step if in insert mode */
				case '0': if (w->mode == 1 || w->mode == 3 || w->mode == 5) w->step = 0; redraw(); break;
				case '1': if (w->mode == 1 || w->mode == 3 || w->mode == 5) w->step = 1; redraw(); break;
				case '2': if (w->mode == 1 || w->mode == 3 || w->mode == 5) w->step = 2; redraw(); break;
				case '3': if (w->mode == 1 || w->mode == 3 || w->mode == 5) w->step = 3; redraw(); break;
				case '4': if (w->mode == 1 || w->mode == 3 || w->mode == 5) w->step = 4; redraw(); break;
				case '5': if (w->mode == 1 || w->mode == 3 || w->mode == 5) w->step = 5; redraw(); break;
				case '6': if (w->mode == 1 || w->mode == 3 || w->mode == 5) w->step = 6; redraw(); break;
				case '7': if (w->mode == 1 || w->mode == 3 || w->mode == 5) w->step = 7; redraw(); break;
				case '8': if (w->mode == 1 || w->mode == 3 || w->mode == 5) w->step = 8; redraw(); break;
				case '9': if (w->mode == 1 || w->mode == 3 || w->mode == 5) w->step = 9; redraw(); break;
				default:
					switch (w->mode)
					{
						case 1: case 3: case 5: /* leave preview */ w->mode--;  break;
						case 2: case 4:         /* leave adjust  */ w->mode-=2; break;
					} redraw(); break;
			} break;
		default:
			switch (w->mode)
			{
				case 0: case 2: case 4:
					if (w->chord)
						switch (w->chord)
						{
							case 'r':
								switch (input)
								{
									case 's': /* start/stop */
										toggleRecording(w->instrument);
										break;
									case 'c': /* cancel */
										if (w->instrumentrecv != INST_REC_LOCK_OK)
											w->instrumentrecv = INST_REC_LOCK_PREP_CANCEL;
										break;
								}
							case 'k': /* keyboard macro */
								w->keyboardmacro = '\0';
								if (input != 'k') // kk just resets
									changeMacro(input, &w->keyboardmacro);
								redraw(); break;
						}
					else
					{
						instrument *iv = s->instrumentv[s->instrumenti[w->instrument]];
						switch (input)
						{
							case 'i': /* enter preview */ w->mode++; redraw(); break;
							case 'r': /* record         */ w->chord = 'r'; redraw(); goto i_afterchordunset;
							case 'k': /* keyboard macro */ w->chord = 'k'; redraw(); goto i_afterchordunset;
							case 'u': /* undo    */   popInstrumentHistory(s->instrumenti[w->instrument]); redraw(); break;
							case 18: /* ^R, redo */ unpopInstrumentHistory(s->instrumenti[w->instrument]); redraw(); break;
							case 127: case 8: /* backspace */
								/* TODO: REALLY slow branch */
								if (w->instrumentindex >= 0 && iv)
									samplerDecFieldPointer(w->instrumentindex);
								redraw(); break;
							case ' ': /* space */
								/* TODO: REALLY slow branch */
								if (w->instrumentindex >= 0 && iv)
									samplerIncFieldPointer(w->instrumentindex);
								redraw(); break;
							default:
								iv = s->instrumentv[s->instrumenti[w->instrument]];
								switch (w->instrumentindex)
								{
									case MIN_INSTRUMENT_INDEX:
										switch (input)
										{
											case 'a': /* add */
												if (!s->instrumenti[w->instrument])
													addInstrument(w->instrument);
												redraw(); break;
											case 'd': /* delete */
												if (w->instrumentrecv == INST_REC_LOCK_OK && s->instrumenti[w->instrument])
												{
													yankInstrument(w->instrument);
													delInstrument(w->instrument);
												} redraw(); break;
											case 'y': /* yank */
												yankInstrument(w->instrument);
												redraw(); break;
											case 'p': /* put */
												if (w->instrumentrecv == INST_REC_LOCK_OK)
												{
													if (!s->instrumenti[w->instrument])
														addInstrument(w->instrument);
													w->instrumentlocki = s->instrumenti[w->instrument];
													w->instrumentlockv = INST_GLOBAL_LOCK_PREP_PUT;
												} else
												{
													strcpy(w->command.error, "failed to put instrument, try again");
													redraw();
												} break;
											case 'e': /* empty */
												if (w->instrumentrecv == INST_REC_LOCK_OK)
													w->instrument = newInstrument(0);
												redraw();
												break;
										} break;
									case MIN_INSTRUMENT_INDEX + 1:
										switch (input)
										{
											case 1:  /* ^a */ iv->defgain++; break;
											case 24: /* ^x */ iv->defgain--; break;
											case '0':           updateFieldPush(&iv->defgain, 0);  break;
											case '1':           updateFieldPush(&iv->defgain, 1);  break;
											case '2':           updateFieldPush(&iv->defgain, 2);  break;
											case '3':           updateFieldPush(&iv->defgain, 3);  break;
											case '4':           updateFieldPush(&iv->defgain, 4);  break;
											case '5':           updateFieldPush(&iv->defgain, 5);  break;
											case '6':           updateFieldPush(&iv->defgain, 6);  break;
											case '7':           updateFieldPush(&iv->defgain, 7);  break;
											case '8':           updateFieldPush(&iv->defgain, 8);  break;
											case '9':           updateFieldPush(&iv->defgain, 9);  break;
											case 'A': case 'a': updateFieldPush(&iv->defgain, 10); break;
											case 'B': case 'b': updateFieldPush(&iv->defgain, 11); break;
											case 'C': case 'c': updateFieldPush(&iv->defgain, 12); break;
											case 'D': case 'd': updateFieldPush(&iv->defgain, 13); break;
											case 'E': case 'e': updateFieldPush(&iv->defgain, 14); break;
											case 'F': case 'f': updateFieldPush(&iv->defgain, 15); break;
										} redraw(); break;
									case MIN_INSTRUMENT_INDEX + 2:
									case MIN_INSTRUMENT_INDEX + 3:
									case MIN_INSTRUMENT_INDEX + 4:
										break;
									default: samplerInput(&input); break;
								} break;
						}
					} break;
				case 1: case 3: case 5: /* preview */
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
	switch (input) /* post type */
	{
		case 10: case 13: /* return */
			pushInstrumentHistoryIfNew(s->instrumentv[s->instrumenti[w->instrument]]);
			switch (w->instrumentindex)
			{
				case MIN_INSTRUMENT_INDEX: break;
				case MIN_INSTRUMENT_INDEX + 3: w->recordflags ^= 0b1;  break;
				case MIN_INSTRUMENT_INDEX + 4: w->recordflags ^= 0b10; break;
				default:
					switch (w->mode)
					{
						case 0: w->mode = 2; break;
						case 1: w->mode = 3; break;
						case 2: w->mode = 0; break;
						case 3: w->mode = 1; break;
					} break;
			} redraw(); break;
	}
	if (w->chord) { w->chord = '\0'; redraw(); }
i_afterchordunset:
}
