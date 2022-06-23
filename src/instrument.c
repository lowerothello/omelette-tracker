#include "types/sampler.c"
#include "types/virtualanalogue.c"

#define MIN_INSTRUMENT_INDEX -3

void initInstrumentTypes(void)
{
	samplerInit(0);
	analogueInit(1);
}

void drawInstrument(void)
{
	switch (w->mode)
	{
		case 1:
			printf("\033[%d;0H\033[1m-- PREVIEW --\033[m", ws.ws_row);
			w->command.error[0] = '\0';
			break;
		case 2:
			printf("\033[%d;0H\033[1m-- ADJUST --\033[m", ws.ws_row);
			w->command.error[0] = '\0';
			break;
		case 3:
			printf("\033[%d;0H\033[1m-- PREVIEW+ADJUST --\033[m", ws.ws_row);
			w->command.error[0] = '\0';
			break;
		case 4:
			printf("\033[%d;0H\033[1m-- MOUSEADJUST --\033[m", ws.ws_row);
			w->command.error[0] = '\0';
			break;
		case 5:
			printf("\033[%d;0H\033[1m-- PREVIEW+MOUSEADJUST --\033[m", ws.ws_row);
			w->command.error[0] = '\0';
			break;
	}

	unsigned short x = w->instrumentcelloffset;
	unsigned short y = w->instrumentrowoffset;
	printf(    "\033[%d;%dH┌──────────────────────────────────────────────────────────────────────────────┐", y+0, x);
	for (int i = 1; i < INSTRUMENT_BODY_ROWS; i++)
		printf("\033[%d;%dH│                                                                              │", y+i, x);
	printf(    "\033[%d;%dH└──────────────────────────────────────────────────────────────────────────────┘", y+INSTRUMENT_BODY_ROWS, x);

	printf("\033[%d;%dH  <- INSTRUMENT (%02x) ->  ", y+0, x+27, w->instrument);
	if (s->instrumenti[w->instrument] == 0)
	{
		printf("\033[%d;%dH(not added)", y+2, x+34);
		printf("\033[%d;%dH", y, x+32);
	} else
	{
		instrument *iv = s->instrumentv[s->instrumenti[w->instrument]];
		printf("\033[%d;%dHtype   [%02x]", y+2, x+4, iv->type);
		printf("\033[%d;%dHfader  [%02x]", y+3, x+4, iv->fader);

		printf(    "\033[%d;%dH┌────────────────────────────────────────────────────────────────────────────┐", y+5, x+1);
		for (int i = 0; i < INSTRUMENT_TYPE_ROWS; i++)
			printf("\033[%d;%dH│                                                                            │", y+6 + i, x+1);
		printf(    "\033[%d;%dH└────────────────────────────────────────────────────────────────────────────┘", y+6 + INSTRUMENT_TYPE_ROWS, x+1);

		if (iv->typefollow == iv->type
				&& iv->type < INSTRUMENT_TYPE_COUNT && t->f[iv->type].draw)
		{
			if (w->mode > 1 && w->mode < 6)
				t->f[iv->type].draw(iv, w->instrument, x+2, y+6, &w->instrumentindex, 1);
			else
				t->f[iv->type].draw(iv, w->instrument, x+2, y+6, &w->instrumentindex, 0);
		}

		switch (w->instrumentindex)
		{
			case -3: printf("\033[%d;%dH", y+0, x+32); break;
			case -2: printf("\033[%d;%dH", y+2, x+13); break;
			case -1: printf("\033[%d;%dH", y+3, x+13); break;
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
				if (w->instrumentlockv > INST_REC_LOCK_OK) break;

				iv->type++;
				w->instrumentlocki = s->instrumenti[w->instrument];
				w->instrumentlockv = INST_GLOBAL_LOCK_PREP_FREE;
			}
			break;
		case MIN_INSTRUMENT_INDEX + 2:
			if (iv)
			{
				if (iv->fader%16 < 15)
					iv->fader++;
				if (iv->fader>>4 < 15)
					iv->fader+=16;
			}
			break;
		default:
			if (iv && iv->type < INSTRUMENT_TYPE_COUNT && t->f[iv->type].adjustUp)
				t->f[iv->type].adjustUp(iv, index, mouse);
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
				if (w->instrumentlockv > INST_REC_LOCK_OK) break;

				iv->type--;
				w->instrumentlocki = s->instrumenti[w->instrument];
				w->instrumentlockv = INST_GLOBAL_LOCK_PREP_FREE;
			}
			break;
		case MIN_INSTRUMENT_INDEX + 2:
			if (iv)
			{
				if (iv->fader%16 > 0)
					iv->fader--;
				if (iv->fader>>4 > 0)
					iv->fader-=16;
			}
			break;
		default:
			if (iv && iv->type < INSTRUMENT_TYPE_COUNT && t->f[iv->type].adjustDown)
				t->f[iv->type].adjustDown(iv, index, mouse);
			break;
	}
}
void instrumentAdjustLeft(instrument *iv, short index, char mouse)
{
	switch (index)
	{
		case MIN_INSTRUMENT_INDEX:
			if (w->instrument > 0)
				w->instrument--;
			break;
		case MIN_INSTRUMENT_INDEX + 1:
			break;
		case MIN_INSTRUMENT_INDEX + 2:
			if (iv)
			{
				if (iv->fader%16 > iv->fader>>4)
					iv->fader+=16;
				else if (iv->fader%16 > 0)
					iv->fader--;
			}
			break;
		default:
			if (iv && iv->type < INSTRUMENT_TYPE_COUNT && t->f[iv->type].adjustLeft)
				t->f[iv->type].adjustLeft(iv, index, mouse);
			break;
	}
}
void instrumentAdjustRight(instrument *iv, short index, char mouse)
{
	switch (index)
	{
		case MIN_INSTRUMENT_INDEX:
			if (w->instrument < 254)
				w->instrument++;
			break;
		case MIN_INSTRUMENT_INDEX + 1:
			break;
		case MIN_INSTRUMENT_INDEX + 2:
			if (iv)
			{
				if (iv->fader>>4 > iv->fader%16)
					iv->fader++;
				else if (iv->fader>>4 > 0)
					iv->fader-=16;
			}
			break;
		default:
			if (iv && iv->type < INSTRUMENT_TYPE_COUNT && t->f[iv->type].adjustRight)
					t->f[iv->type].adjustRight(iv, index, mouse);
			break;
	}
}

int instrumentInput(int input)
{
	if (!s->instrumenti[w->instrument])
		w->instrumentindex = MIN_INSTRUMENT_INDEX;
	switch (input)
	{
		case '\033':
			switch (getchar())
			{
				case 'O':
					previewNote(0, 255, w->channel, 1);
					switch (getchar())
					{
						case 'P':
							pushInstrumentHistoryIfNew(s->instrumentv[s->instrumenti[w->instrument]]);
							w->popup = 0;
							w->mode = 0;
							break;
					}
					redraw();
					break;
				case '[':
					instrument *iv = s->instrumentv[s->instrumenti[w->instrument]];
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
										w->fieldpointer = 0;
										break;
									case MIN_INSTRUMENT_INDEX + 2:
										if (w->fieldpointer > 3)
											w->fieldpointer = 3;
										break;
									default:
										if (w->instrumentindex >= 0
												&& iv && iv->type < INSTRUMENT_TYPE_COUNT
												&& t->f[iv->type].endFieldPointer)
											t->f[iv->type].endFieldPointer(w->instrumentindex);
										break;
								}
							}
							redraw();
							break;
						case 'B': /* down arrow */
							if (w->mode > 1) /* adjust */
								instrumentAdjustDown(iv, w->instrumentindex, 0);
							else
							{
								pushInstrumentHistoryIfNew(s->instrumentv[s->instrumenti[w->instrument]]);
								if (s->instrumenti[w->instrument])
									w->instrumentindex++;
								unsigned short tic = 0; /* type index count */
								iv = s->instrumentv[s->instrumenti[w->instrument]];
								if (iv && iv->type < INSTRUMENT_TYPE_COUNT)
									tic = t->f[iv->type].indexc;
								if (w->instrumentindex > tic)
									w->instrumentindex = tic;

								/* ensure fieldpointer is still in range */
								switch (w->instrumentindex)
								{
									case MIN_INSTRUMENT_INDEX: break;
									case MIN_INSTRUMENT_INDEX + 1:
										w->fieldpointer = 0;
										break;
									case MIN_INSTRUMENT_INDEX + 2:
										if (w->fieldpointer > 3)
											w->fieldpointer = 3;
										break;
									default:
										if (w->instrumentindex >= 0
												&& iv && iv->type < INSTRUMENT_TYPE_COUNT
												&& t->f[iv->type].endFieldPointer)
											t->f[iv->type].endFieldPointer(w->instrumentindex);
										break;
								}
							}
							redraw();
							break;
						case 'D': /* left arrow */
							if (w->instrumentindex == MIN_INSTRUMENT_INDEX || w->mode > 1) /* adjust */
								instrumentAdjustLeft(iv, w->instrumentindex, 0);
							else if (w->instrumentindex == MIN_INSTRUMENT_INDEX + 2)
							{
								if (w->fieldpointer == 0)
									w->fieldpointer = 3;
								else w->fieldpointer--;
							} else if (w->instrumentindex <= MIN_INSTRUMENT_INDEX + 2)
								instrumentAdjustLeft(iv, w->instrumentindex, 0);
							else switch (w->instrumentindex)
							{
								case MIN_INSTRUMENT_INDEX:
								case MIN_INSTRUMENT_INDEX + 1:
									break;
								default:
									if (iv && iv->type < INSTRUMENT_TYPE_COUNT
											&& t->f[iv->type].decFieldPointer)
										t->f[iv->type].decFieldPointer(w->instrumentindex);
									break;
							}
							redraw();
							break;
						case 'C': /* right arrow */
							if (w->instrumentindex == MIN_INSTRUMENT_INDEX || w->mode > 1) /* adjust */
								instrumentAdjustRight(iv, w->instrumentindex, 0);
							else if (w->instrumentindex == MIN_INSTRUMENT_INDEX + 2)
							{
								w->fieldpointer++;
								if (w->fieldpointer > 3)
									w->fieldpointer = 0;
							} else if (w->instrumentindex <= MIN_INSTRUMENT_INDEX + 2)
								instrumentAdjustRight(iv, w->instrumentindex, 0);
							else switch (w->instrumentindex)
							{
								case MIN_INSTRUMENT_INDEX:
								case MIN_INSTRUMENT_INDEX + 1:
									break;
								default:
									if (iv && iv->type < INSTRUMENT_TYPE_COUNT
											&& t->f[iv->type].incFieldPointer)
										t->f[iv->type].incFieldPointer(w->instrumentindex);
									break;
							}
							redraw();
							break;
						case '1': /* mod+arrow / f5 - f8 */
							switch (getchar())
							{
								case '5': /* f5, play */
									getchar(); /* extraneous tilde */
									startPlayback();
									break;
								case '7': /* f6 (yes, f6 is '7'), stop */
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
											}
											break;
									}
									break;
							}
							break;
						case 'H': /* home */
							pushInstrumentHistoryIfNew(s->instrumentv[s->instrumenti[w->instrument]]);
							w->fieldpointer = 0;
							redraw();
							break;
						case '4': /* end */
							getchar(); /* burn through the tilde */
							pushInstrumentHistoryIfNew(s->instrumentv[s->instrumenti[w->instrument]]);
							switch (w->instrumentindex)
							{
								case MIN_INSTRUMENT_INDEX:     w->fieldpointer = 0; break;
								case MIN_INSTRUMENT_INDEX + 1: w->fieldpointer = 1; break;
								case MIN_INSTRUMENT_INDEX + 2: w->fieldpointer = 3; break;
								default:
									if (iv && iv->type < INSTRUMENT_TYPE_COUNT
											&& t->f[iv->type].endFieldPointer)
										t->f[iv->type].endFieldPointer(w->instrumentindex);
									break;
							}
							redraw();
							break;
						case '5': /* page up */
							getchar();
							w->instrumentindex = MIN_INSTRUMENT_INDEX;
							redraw();
							break;
						case '6': /* page down */
							getchar();
							w->instrumentindex = 0;
							redraw();
							break;
						case 'M': /* mouse */
							int button = getchar();
							int x = getchar() - 32;
							int y = getchar() - 32;
							switch (button)
							{
								case WHEEL_UP: /* scroll up   */
								case WHEEL_DOWN: /* scroll down */
									break;
								case BUTTON_RELEASE: /* release click */
									/* leave adjust mode */
									if (w->mode > 3) w->mode = w->mode - 4;
									if (w->mode > 1) w->mode = w->mode - 2;
									break;
								case BUTTON1: case BUTTON3:
									pushInstrumentHistoryIfNew(s->instrumentv[s->instrumenti[w->instrument]]);
									if (x < w->instrumentcelloffset
											|| x > w->instrumentcelloffset + INSTRUMENT_BODY_COLS - 1
											|| y < w->instrumentrowoffset
											|| y > w->instrumentrowoffset + INSTRUMENT_BODY_ROWS)
									{
										previewNote(0, 255, w->channel, 1);
										w->popup = 0;
										break;
									}

									switch (y - w->instrumentrowoffset)
									{
										case 0: /* index */
											w->fieldpointer = 0;
											w->instrumentindex = MIN_INSTRUMENT_INDEX;
											break;
										case 1: case 2: /* type */
											w->fieldpointer = 0;
											w->instrumentindex = MIN_INSTRUMENT_INDEX + 1;
											break;
										case 3: case 4: /* fader */
											w->fieldpointer = 0;
											w->instrumentindex = MIN_INSTRUMENT_INDEX + 2;
											break;
										default:
											if (!s->instrumenti[w->instrument]) break;
											if (iv->type < INSTRUMENT_TYPE_COUNT && t->f[iv->type].mouseToIndex)
												t->f[iv->type].mouseToIndex(
														y - w->instrumentrowoffset - 5,
														x - w->instrumentcelloffset - 2,
														button,
														&w->instrumentindex);
											break;
									}

									/* enter mouse adjust mode */
									if (w->mode < 2) w->mode = w->mode + 4;
									if (w->mode < 4) w->mode = w->mode + 2;
									w->mousey = y;
									w->mousex = x;
									break;
								case BUTTON1_HOLD:
									if (w->mode > 3) /* mouse adjust */
									{
										if      (x > w->mousex) instrumentAdjustRight(iv, w->instrumentindex, 1);
										else if (x < w->mousex) instrumentAdjustLeft(iv, w->instrumentindex, 1);

										if      (y > w->mousey) instrumentAdjustDown(iv, w->instrumentindex, 1);
										else if (y < w->mousey) instrumentAdjustUp(iv, w->instrumentindex, 1);

										w->mousey = y;
										w->mousex = x;
									}
									break;
							}
							redraw();
							break;
					}
					break;
				default:
					previewNote(0, 255, w->channel, 1);
					switch (w->mode)
					{
						case 0: case 2: case 4: /* leave the popup */
							pushInstrumentHistoryIfNew(s->instrumentv[s->instrumenti[w->instrument]]);
							w->fieldpointer = 0;
							w->popup = 0;
							w->instrumentindex = 0;
							break;
						case 1: case 3: case 5: /* leave preview */
							w->mode--;
							break;
					}
					redraw();
					break;
			}
			break;
		default:
			switch (w->mode)
			{
				case 0: case 2: case 4:
					instrument *iv = s->instrumentv[s->instrumenti[w->instrument]];
					switch (input)
					{
						case 'i': /* enter preview */
							w->mode++;
							redraw();
							break;
						case 'u': /* undo */
							popInstrumentHistory(s->instrumenti[w->instrument]);
							redraw();
							break;
						case 18: /* ^R, redo */
							unpopInstrumentHistory(s->instrumenti[w->instrument]);
							redraw();
							break;
						case 127: case 8: /* backspace */
							/* TODO: REALLY slow branch */
							if (w->instrumentindex >= 0 && iv && iv->type < INSTRUMENT_TYPE_COUNT
									&& t->f[iv->type].decFieldPointer)
								t->f[iv->type].decFieldPointer(w->instrumentindex);
							redraw();
							break;
						case ' ': /* space */
							/* TODO: REALLY slow branch */
							if (w->instrumentindex >= 0 && iv && iv->type < INSTRUMENT_TYPE_COUNT
									&& t->f[iv->type].incFieldPointer)
								t->f[iv->type].incFieldPointer(w->instrumentindex);
							redraw();
							break;
						default:
							instrument *iv = s->instrumentv[s->instrumenti[w->instrument]];

							switch (w->instrumentindex)
							{
								case MIN_INSTRUMENT_INDEX:
									switch (input)
									{
										case 'a': /* add */
											if (!s->instrumenti[w->instrument])
												addInstrument(w->instrument);
											redraw();
											break;
										case 'd': /* delete */
											if (s->instrumenti[w->instrument])
											{
												yankInstrument(w->instrument);
												delInstrument(w->instrument);
											}
											redraw();
											break;
										case 'y': /* yank */
											yankInstrument(w->instrument);
											redraw();
											break;
										case 'p': /* put */
											if (!(w->instrumentreci == s->instrumenti[w->instrument]
													&& w->instrumentrecv != INST_REC_LOCK_OK))
											{
												if (!s->instrumenti[w->instrument])
													addInstrument(w->instrument);
												w->instrumentlocki = s->instrumenti[w->instrument];
												w->instrumentlockv = INST_GLOBAL_LOCK_PREP_PUT;
											} else
											{
												strcpy(w->command.error, "failed to put instrument, try again");
												redraw();
											}
											break;
										case 'e': /* empty */
											w->instrument = newInstrument(0);
											redraw();
											break;
									}
									break;
								case MIN_INSTRUMENT_INDEX + 1:
									if (w->instrumentlockv > INST_REC_LOCK_OK) break;
									if (!s->instrumenti[w->instrument]) break;
									iv = s->instrumentv[s->instrumenti[w->instrument]];
									switch (input)
									{
										case 1: /* ^a */
											iv->type++;
											w->instrumentlocki = s->instrumenti[w->instrument];
											w->instrumentlockv = INST_GLOBAL_LOCK_PREP_FREE;
											break;
										case 24: /* ^x */
											iv->type--;
											w->instrumentlocki = s->instrumenti[w->instrument];
											w->instrumentlockv = INST_GLOBAL_LOCK_PREP_FREE;
											break;
										case '0':           updateFieldPush(&iv->type, 0);  w->instrumentlocki = s->instrumenti[w->instrument]; w->instrumentlockv = INST_GLOBAL_LOCK_PREP_FREE; break;
										case '1':           updateFieldPush(&iv->type, 1);  w->instrumentlocki = s->instrumenti[w->instrument]; w->instrumentlockv = INST_GLOBAL_LOCK_PREP_FREE; break;
										case '2':           updateFieldPush(&iv->type, 2);  w->instrumentlocki = s->instrumenti[w->instrument]; w->instrumentlockv = INST_GLOBAL_LOCK_PREP_FREE; break;
										case '3':           updateFieldPush(&iv->type, 3);  w->instrumentlocki = s->instrumenti[w->instrument]; w->instrumentlockv = INST_GLOBAL_LOCK_PREP_FREE; break;
										case '4':           updateFieldPush(&iv->type, 4);  w->instrumentlocki = s->instrumenti[w->instrument]; w->instrumentlockv = INST_GLOBAL_LOCK_PREP_FREE; break;
										case '5':           updateFieldPush(&iv->type, 5);  w->instrumentlocki = s->instrumenti[w->instrument]; w->instrumentlockv = INST_GLOBAL_LOCK_PREP_FREE; break;
										case '6':           updateFieldPush(&iv->type, 6);  w->instrumentlocki = s->instrumenti[w->instrument]; w->instrumentlockv = INST_GLOBAL_LOCK_PREP_FREE; break;
										case '7':           updateFieldPush(&iv->type, 7);  w->instrumentlocki = s->instrumenti[w->instrument]; w->instrumentlockv = INST_GLOBAL_LOCK_PREP_FREE; break;
										case '8':           updateFieldPush(&iv->type, 8);  w->instrumentlocki = s->instrumenti[w->instrument]; w->instrumentlockv = INST_GLOBAL_LOCK_PREP_FREE; break;
										case '9':           updateFieldPush(&iv->type, 9);  w->instrumentlocki = s->instrumenti[w->instrument]; w->instrumentlockv = INST_GLOBAL_LOCK_PREP_FREE; break;
										case 'A': case 'a': updateFieldPush(&iv->type, 10); w->instrumentlocki = s->instrumenti[w->instrument]; w->instrumentlockv = INST_GLOBAL_LOCK_PREP_FREE; break;
										case 'B': case 'b': updateFieldPush(&iv->type, 11); w->instrumentlocki = s->instrumenti[w->instrument]; w->instrumentlockv = INST_GLOBAL_LOCK_PREP_FREE; break;
										case 'C': case 'c': updateFieldPush(&iv->type, 12); w->instrumentlocki = s->instrumenti[w->instrument]; w->instrumentlockv = INST_GLOBAL_LOCK_PREP_FREE; break;
										case 'D': case 'd': updateFieldPush(&iv->type, 13); w->instrumentlocki = s->instrumenti[w->instrument]; w->instrumentlockv = INST_GLOBAL_LOCK_PREP_FREE; break;
										case 'E': case 'e': updateFieldPush(&iv->type, 14); w->instrumentlocki = s->instrumenti[w->instrument]; w->instrumentlockv = INST_GLOBAL_LOCK_PREP_FREE; break;
										case 'F': case 'f': updateFieldPush(&iv->type, 15); w->instrumentlocki = s->instrumenti[w->instrument]; w->instrumentlockv = INST_GLOBAL_LOCK_PREP_FREE; break;
									}
									redraw();
									break;
								case MIN_INSTRUMENT_INDEX + 2:
									switch (input)
									{
										case 1: /* ^a */
											iv->fader++;
											break;
										case 24: /* ^x */
											iv->fader--;
											break;
										case '0':           updateFieldPush(&iv->fader, 0);  break;
										case '1':           updateFieldPush(&iv->fader, 1);  break;
										case '2':           updateFieldPush(&iv->fader, 2);  break;
										case '3':           updateFieldPush(&iv->fader, 3);  break;
										case '4':           updateFieldPush(&iv->fader, 4);  break;
										case '5':           updateFieldPush(&iv->fader, 5);  break;
										case '6':           updateFieldPush(&iv->fader, 6);  break;
										case '7':           updateFieldPush(&iv->fader, 7);  break;
										case '8':           updateFieldPush(&iv->fader, 8);  break;
										case '9':           updateFieldPush(&iv->fader, 9);  break;
										case 'A': case 'a': updateFieldPush(&iv->fader, 10); break;
										case 'B': case 'b': updateFieldPush(&iv->fader, 11); break;
										case 'C': case 'c': updateFieldPush(&iv->fader, 12); break;
										case 'D': case 'd': updateFieldPush(&iv->fader, 13); break;
										case 'E': case 'e': updateFieldPush(&iv->fader, 14); break;
										case 'F': case 'f': updateFieldPush(&iv->fader, 15); break;
									}
									redraw();
									break;
								default:
									if (!s->instrumenti[w->instrument]) break;
									iv = s->instrumentv[s->instrumenti[w->instrument]];
									if (iv && iv->type < INSTRUMENT_TYPE_COUNT && t->f[iv->type].input)
										t->f[iv->type].input(&input);
									break;
							}
							break;
					}
					break;
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
							previewNote(charToNote(input, w->octave), w->instrument, w->channel, 0);
							break;
					}
					break;
			}
			break;
	}
	switch (input) /* post type */
	{
		case 10: case 13: /* return */
			pushInstrumentHistoryIfNew(s->instrumentv[s->instrumenti[w->instrument]]);
			if (w->instrumentindex > MIN_INSTRUMENT_INDEX)
			{
				switch (w->mode)
				{
					case 0: w->mode = 2; break;
					case 1: w->mode = 3; break;
					case 2: w->mode = 0; break;
					case 3: w->mode = 1; break;
				}
				redraw();
			}
			break;
	}
	return 0;
}
