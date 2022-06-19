#include "types/sampler.c"
#include "types/virtualanalogue.c"

#define MIN_INSTRUMENT_INDEX -3

void initInstrumentTypes(void)
{
	samplerInit(0);
	analogueInit(1);
}

void instrumentRedraw(void)
{
	if (w->popup && w->popup < 3)
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

		if (w->popup == 2) /* file browser */
		{
			printf("\033[%d;%dH  FILEBROWSER  ", y+0, x+32);

			if (strlen(w->dirpath) > INSTRUMENT_BODY_COLS - 4)
			{
				char buffer[INSTRUMENT_BODY_COLS + 1];
				memcpy(buffer, w->dirpath + ((unsigned short)strlen(w->dirpath - INSTRUMENT_BODY_COLS - 4)), INSTRUMENT_BODY_COLS - 4);
				printf("\033[%d;%dH%.*s", y+1, x+3, INSTRUMENT_BODY_COLS - 4, buffer);
			} else
			{
				printf("\033[%d;%dH%.*s", y+1, x-1 + (INSTRUMENT_BODY_COLS - (unsigned short)strlen(w->dirpath) + 1) / 2, INSTRUMENT_BODY_COLS - 4, w->dirpath);
			}

			struct dirent *dirent = readdir(w->dir);
			unsigned int dirc = 0;
			short yo, xo;
			char testdirpath[NAME_MAX + 1];
			DIR *testdir;
			xo = x + (INSTRUMENT_BODY_COLS - (w->dirmaxwidth + 2) * w->dircols) / 2;
			yo = y+11 - w->instrumentindex / w->dircols;
			while (dirent)
			{
				for (unsigned char wcol = 0; wcol < w->dircols; wcol++)
				{
					if (!dirent) break;
					while (
							   !strcmp(dirent->d_name, ".")
							|| !strcmp(dirent->d_name, "..")
							|| !strcmp(dirent->d_name, "lost+found"))
						dirent = readdir(w->dir);
					if (!dirent) break;

					if (yo > y+1 && yo < y + INSTRUMENT_BODY_ROWS)
					{
						strcpy(testdirpath, w->dirpath);
						strcat(testdirpath, "/");
						strcat(testdirpath, dirent->d_name);

						testdir = opendir(testdirpath);
						if (testdir) /* add a trailing slash if the file is a directory */
						{ closedir(testdir);
							printf(   "\033[%d;%dH%.*s/", yo, xo + (w->dirmaxwidth + 2) * wcol, INSTRUMENT_BODY_COLS - 4, dirent->d_name);
						} else printf("\033[%d;%dH%.*s",  yo, xo + (w->dirmaxwidth + 2) * wcol, INSTRUMENT_BODY_COLS - 4, dirent->d_name);
					}
					dirent = readdir(w->dir);
				}
				yo++;
				dirc += w->dircols;
			}

			rewinddir(w->dir);
			if (dirc != w->dirc) changeDirectory(); /* recount the entries if a file gets added or removed */

			if (dirc == 0)
			{
				w->dirmaxwidth = INSTRUMENT_BODY_COLS - 4;
				printf("\033[%d;%dH%.*s", y+11, x + (INSTRUMENT_BODY_COLS - (unsigned short)strlen("(empty directory)") + 1) / 2, INSTRUMENT_BODY_COLS - 4, "(empty directory)");
				printf("\033[%d;%dH", y+11, x + (INSTRUMENT_BODY_COLS - (unsigned short)strlen("(empty directory)") + 1) / 2 + 1);
			} else printf("\033[%d;%dH", y+11 + w->fyoffset, xo + (w->dirmaxwidth + 2) * (w->instrumentindex % w->dircols));
		} else /* instrument */
		{
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

				if (w->popup == 1 && iv->typefollow == iv->type
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
	}
}

void instrumentAdjustUp(instrument *iv, short index)
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
				t->f[iv->type].adjustUp(iv, index);
			break;
	}
}
void instrumentAdjustDown(instrument *iv, short index)
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
				t->f[iv->type].adjustDown(iv, index);
			break;
	}
}
void instrumentAdjustLeft(instrument *iv, short index)
{
	switch (index)
	{
		case MIN_INSTRUMENT_INDEX:
			if (w->instrument > 0)
			{
				w->instrumentsend = 0;
				w->instrument--;
			}
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
				t->f[iv->type].adjustLeft(iv, index);
			break;
	}
}
void instrumentAdjustRight(instrument *iv, short index)
{
	switch (index)
	{
		case MIN_INSTRUMENT_INDEX:
			if (w->instrument < 254)
			{
				w->instrumentsend = 0;
				w->instrument++;
			}
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
					t->f[iv->type].adjustRight(iv, index);
			break;
	}
}

int getSubdir(char *newpath)
{
	struct dirent *dirent = readdir(w->dir);
	unsigned int dirc = 0;
	DIR *testdir;
	while (dirent)
	{
		if (
				   strcmp(dirent->d_name, ".")
				&& strcmp(dirent->d_name, "..")
				&& strcmp(dirent->d_name, "lost+found"))
		{
			if (dirc == w->instrumentindex)
			{
				strcpy(newpath, w->dirpath);
				strcat(newpath, "/");
				strcat(newpath, dirent->d_name);
				testdir = opendir(newpath);
				rewinddir(w->dir);
				if (testdir == NULL)
					return 1; /* file */
				else
				{
					closedir(testdir);
					return 2; /* directory */
				}
				redraw();
				break;
			}
			dirc++;
		}
		dirent = readdir(w->dir);
	}
	rewinddir(w->dir);
	return 0; /* fail case, reachable only for empty dirs i think? idk */
}

int instrumentInput(int input)
{
	switch (w->popup)
	{
		case 1: /* instrument */
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
										instrumentAdjustUp(iv, w->instrumentindex);
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
										instrumentAdjustDown(iv, w->instrumentindex);
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
										instrumentAdjustLeft(iv, w->instrumentindex);
									else if (w->instrumentindex == MIN_INSTRUMENT_INDEX + 2)
									{
										if (w->fieldpointer == 0)
											w->fieldpointer = 3;
										else w->fieldpointer--;
									} else if (w->instrumentindex <= MIN_INSTRUMENT_INDEX + 2)
										instrumentAdjustLeft(iv, w->instrumentindex);
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
										instrumentAdjustRight(iv, w->instrumentindex);
									else if (w->instrumentindex == MIN_INSTRUMENT_INDEX + 2)
									{
										w->fieldpointer++;
										if (w->fieldpointer > 3)
											w->fieldpointer = 0;
									} else if (w->instrumentindex <= MIN_INSTRUMENT_INDEX + 2)
										instrumentAdjustRight(iv, w->instrumentindex);
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
															instrumentAdjustLeft(iv, MIN_INSTRUMENT_INDEX);
															redraw();
															break;
														case 'C': /* right */
															instrumentAdjustRight(iv, MIN_INSTRUMENT_INDEX);
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
												if      (x > w->mousex) instrumentAdjustRight(iv, w->instrumentindex);
												else if (x < w->mousex) instrumentAdjustLeft(iv, w->instrumentindex);

												if      (y > w->mousey) instrumentAdjustDown(iv, w->instrumentindex);
												else if (y < w->mousey) instrumentAdjustUp(iv, w->instrumentindex);

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
									w->instrumentsend = 0;
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
							switch (input)
							{
								case 'i': /* enter preview */
									w->mode++;
									redraw();
									break;
								case 'u': /* undo */
									pushInstrumentHistoryIfNew(s->instrumentv[s->instrumenti[w->instrument]]);
									popInstrumentHistory(s->instrumenti[w->instrument]);
									redraw();
									break;
								case 18: /* ^R, redo */
									pushInstrumentHistoryIfNew(s->instrumentv[s->instrumenti[w->instrument]]);
									unpopInstrumentHistory(s->instrumenti[w->instrument]);
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
													putInstrument(w->instrument);
													redraw();
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
			break;
		case 2: /* file browser */
			switch (input)
			{
				case 10: case 13: /* return */
					char newpath[NAME_MAX + 1];
					char oldpath[NAME_MAX + 1]; strcpy(oldpath, w->dirpath);
					switch (getSubdir(newpath))
					{
						case 1: /* file */
							loadSample(w->instrument, newpath);
							pushInstrumentHistory(s->instrumentv[s->instrumenti[w->instrument]]);
							w->popup = 1;
							w->instrumentindex = 0;
							break;
						case 2: /* directory */
							strcpy(w->dirpath, newpath);
							if (changeDirectory())
							{
								strcpy(w->dirpath, oldpath);
								changeDirectory();
							} else w->instrumentindex = 0;
							break;
					}
					redraw();
					break;
				case 127: case 8: /* backspace */
					if (w->previewsamplestatus == 1) w->previewsamplestatus = 2;
					dirname(w->dirpath);
					changeDirectory();
					w->instrumentindex = 0;
					redraw();
					break;
				case '\033':
					switch (getchar())
					{
						case 'O':
							switch (getchar())
							{
								case 'P':
									pushInstrumentHistoryIfNew(s->instrumentv[s->instrumenti[w->instrument]]);
									w->popup = 0;
									w->instrumentindex = 0;
									w->mode = 0;
									break;
							}
							redraw();
							break;
						case '[':
							switch (getchar())
							{
								case 'A': /* up arrow */
									if (w->previewsamplestatus == 1) w->previewsamplestatus = 2;
									w->instrumentindex -= w->dircols;
									if (w->instrumentindex < 0) w->instrumentindex = 0;
									redraw();
									break;
								case 'B': /* down arrow */
									if (w->previewsamplestatus == 1) w->previewsamplestatus = 2;
									w->instrumentindex += w->dircols;
									if (w->instrumentindex > w->dirc - 1) w->instrumentindex = w->dirc - 1;
									redraw();
									break;
								case 'D': /* left arrow */
									if (w->previewsamplestatus == 1) w->previewsamplestatus = 2;
									w->instrumentindex--;
									if (w->instrumentindex < 0) w->instrumentindex = 0;
									redraw();
									break;
								case 'C': /* right arrow */
									if (w->previewsamplestatus == 1) w->previewsamplestatus = 2;
									w->instrumentindex++;
									if (w->instrumentindex > w->dirc - 1) w->instrumentindex = w->dirc - 1;
									redraw();
									break;
								case 'H': /* home */
									if (w->previewsamplestatus == 1) w->previewsamplestatus = 2;
									w->instrumentindex = 0;
									redraw();
									break;
								case '4': /* end */
									getchar();
									if (w->previewsamplestatus == 1) w->previewsamplestatus = 2;
									w->instrumentindex = w->dirc - 1;
									redraw();
									break;
								case '5': /* page up */
									getchar();
									if (w->previewsamplestatus == 1) w->previewsamplestatus = 2;
									w->instrumentindex -= w->dircols * (INSTRUMENT_BODY_ROWS - 2);
									if (w->instrumentindex < 0) w->instrumentindex = 0;
									redraw();
									break;
								case '6': /* page down */
									getchar();
									if (w->previewsamplestatus == 1) w->previewsamplestatus = 2;
									w->instrumentindex += w->dircols * (INSTRUMENT_BODY_ROWS - 2);
									if (w->instrumentindex > w->dirc - 1) w->instrumentindex = w->dirc - 1;
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
											getchar();
											break;
									}
									break;
								case 'M': /* mouse */
									if (w->previewsamplestatus == 1) w->previewsamplestatus = 2;
									int button = getchar();
									int x = getchar() - 32;
									int y = getchar() - 32;
									switch (button)
									{
										case WHEEL_UP: /* scroll up */
											w->instrumentindex -= WHEEL_SPEED * w->dircols;
											if (w->instrumentindex < 0)
												w->instrumentindex = 0;
											break;
										case WHEEL_DOWN: /* scroll down */
											w->instrumentindex += WHEEL_SPEED * w->dircols;
											if (w->instrumentindex > w->dirc - 1)
												w->instrumentindex = w->dirc - 1;
											break;
										case BUTTON_RELEASE: /* release click */
											w->instrumentindex += w->fyoffset * w->dircols;
											w->fyoffset = 0;
											/* leave adjust mode */
											if (w->mode > 3) w->mode = w->mode - 4;
											if (w->mode > 1) w->mode = w->mode - 2;
											break;
										case BUTTON1: case BUTTON3:
											if (x < w->instrumentcelloffset
													|| x > w->instrumentcelloffset + INSTRUMENT_BODY_COLS - 1
													|| y < w->instrumentrowoffset
													|| y > w->instrumentrowoffset + INSTRUMENT_BODY_ROWS)
											{
												w->popup = 0;
												break;
											}
										case BUTTON1_HOLD:
											if (y < w->instrumentrowoffset + 2 || y > w->instrumentrowoffset + INSTRUMENT_BODY_ROWS - 1)
												break; /* ignore clicking out of range */

											short xo = w->instrumentcelloffset + (INSTRUMENT_BODY_COLS - (w->dirmaxwidth + 2) * w->dircols) / 2;
											w->instrumentindex -= w->instrumentindex % w->dircols; /* pull to the first column */

											if (x >= xo + (w->dirmaxwidth + 2) * w->dircols)
												w->instrumentindex += w->dircols - 1;
											else if (x >= xo)
												w->instrumentindex += (x - xo) / (w->dirmaxwidth + 2);

											w->fyoffset = y - (w->instrumentrowoffset + 11); /* magic number, visual centre */
											if (w->fyoffset + w->instrumentindex / w->dircols < 0)
												w->fyoffset -= w->instrumentindex / w->dircols + w->fyoffset;
											if (w->fyoffset + w->instrumentindex / w->dircols > (w->dirc - 1) / w->dircols)
												w->fyoffset -= w->instrumentindex / w->dircols + w->fyoffset - (w->dirc - 1) / w->dircols;
											if (w->fyoffset * w->dircols + w->instrumentindex > w->dirc - 1)
												w->fyoffset--;

											if (button == BUTTON3)
											{
												w->instrumentindex += w->fyoffset * w->dircols;
												w->fyoffset = 0;
												char newpath[NAME_MAX + 1]; /* TODO: functionize, do on release */
												char oldpath[NAME_MAX + 1]; strcpy(oldpath, w->dirpath);
												switch (getSubdir(newpath))
												{
													case 1: /* file */
														loadSample(w->instrument, newpath);
														pushInstrumentHistory(s->instrumentv[s->instrumenti[w->instrument]]);
														w->popup = 1;
														w->instrumentindex = 0;
														break;
													case 2: /* directory */
														strcpy(w->dirpath, newpath);
														if (changeDirectory())
														{
															strcpy(w->dirpath, oldpath);
															changeDirectory();
														} else w->instrumentindex = 0;
														break;
												}
											}
											break;
									}
									redraw();
									break;
							}
							break;
						default:
							switch (w->mode)
							{
								case 0: case 2: case 4: /* leave the popup */
									if (w->previewsamplestatus == 1) w->previewsamplestatus = 2;
									pushInstrumentHistoryIfNew(s->instrumentv[s->instrumenti[w->instrument]]);
									w->popup = 1;
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
							switch (input)
							{
								case 'i': /* enter preview */
									w->mode++;
									redraw();
									break;
							}
							break;
						case 1: case 3: case 5: /* preview */
							switch (input)
							{
								case '0': w->octave = 0; break;
								case '1': w->octave = 1; break;
								case '2': w->octave = 2; break;
								case '3': w->octave = 3; break;
								case '4': w->octave = 4; break;
								case '5': w->octave = 5; break;
								case '6': w->octave = 6; break;
								case '7': w->octave = 7; break;
								case '8': w->octave = 8; break;
								case '9': w->octave = 9; break;
								default:
									if (w->previewsamplestatus == 0)
									{
										SF_INFO sfinfo;
										char newpath[NAME_MAX + 1];
										if (getSubdir(newpath) == 1)
										{
											w->previewinstrument.sampledata = _loadSample(newpath, &sfinfo);
											if (w->previewinstrument.sampledata)
											{
												sampler_state *ss = w->previewinstrument.state[w->previewinstrument.type];
												ss->length = sfinfo.frames;
												ss->channels = sfinfo.channels;
												ss->c5rate = sfinfo.samplerate;
												ss->trim[1] = sfinfo.frames;

												w->previewsamplestatus = 1;
											}
										}
									}
									if (w->previewsamplestatus == 1)
									{
										w->previewnote = charToNote(input, w->octave);
										w->previewchannel = w->channel;
										w->previewinst = 255;
										w->previewtrigger = 3; // trigger the preview sample
									}
									break;
							}
							break;
					}
					break;
			}
			break;
	}
	return 0;
}
