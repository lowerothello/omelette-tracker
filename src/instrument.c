#include "types/sampler.c"
#include "types/dummy.c"

void initInstrumentTypes(void)
{
	samplerInit(0);
	dummyInit(1);
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

		unsigned short x, y;
		x = w->instrumentcelloffset;
		y = w->instrumentrowoffset;
		printf("\033[%d;%dH┌───────── <- [INSTRUMENT %02x] -> ─────────┐", y - 1, x, w->instrument);
		for (int i = 0; i < INSTRUMENT_BODY_ROWS; i++)
			printf("\033[%d;%dH│                                         │", y + i, x);
		printf("\033[%d;%dH└─────────────────────────────────────────┘", y + INSTRUMENT_BODY_ROWS, x);

		if (s->instrumenti[w->instrument] == 0)
		{
			printf("\033[%d;%dH               (not added)               ", y + 1, x + 1);
			printf("\033[%d;%dH", y - 1, x + 15 + w->fieldpointer);
		} else
		{
			instrument *iv = s->instrumentv[s->instrumenti[w->instrument]];
			printf("\033[%d;%dH   type: [%02x]                            ", y + 1, x + 1, iv->type);

			printf("\033[%d;%dH┌───────────────────────────────────────┐", y + 3, x + 1);
			for (int i = 0; i < INSTRUMENT_TYPE_ROWS; i++)
				printf("\033[%d;%dH│                                       │", y + 4 + i, x + 1);
			printf("\033[%d;%dH└───────────────────────────────────────┘", y + INSTRUMENT_TYPE_ROWS + 4, x + 1);

			printf("\033[%d;%dH  fader: [%02x%02x]      sends  effect: [%x] ", y + INSTRUMENT_TYPE_ROWS + 5, x + 1, iv->fader[0], iv->fader[1], w->instrumentsend);
			printf(    "\033[%d;%dH                     -----     mix: [%x] ", y + INSTRUMENT_TYPE_ROWS + 6, x + 1, iv->send[w->instrumentsend]);

			if (w->popup == 1 && iv->typefollow == iv->type
					&& iv->type < INSTRUMENT_TYPE_COUNT && t->f[iv->type].draw)
				t->f[iv->type].draw(iv, w->instrument, x + 2, y + 4, &w->instrumentindex, w->fieldpointer);

			switch (w->instrumentindex)
			{
				case -2: printf("\033[%d;%dH", y - 1, x + 15); break;
				case -1: printf("\033[%d;%dH", y + 1, x + 12); break;
				default:
					unsigned short tic = 0; /* type index count */
					if (iv->type < INSTRUMENT_TYPE_COUNT)
						tic = t->f[iv->type].indexc;

					if      (w->instrumentindex == tic + 0)
						printf("\033[%d;%dH", y + INSTRUMENT_TYPE_ROWS + 5, x + 11 + w->fieldpointer);
					else if (w->instrumentindex == tic + 1)
						printf("\033[%d;%dH", y + INSTRUMENT_TYPE_ROWS + 5, x + 38);
					else if (w->instrumentindex == tic + 2)
						printf("\033[%d;%dH", y + INSTRUMENT_TYPE_ROWS + 6, x + 38);
					break;
			}
		}

		if (w->popup == 2) /* draw the file browser overlay */
		{
			printf("\033[%d;%dH┌──────────── [filebrowser] ────────────┐", y + 3, x + 1);

			if (strlen(w->dirpath) > INSTRUMENT_BODY_COLS - 4)
			{
				char buffer[INSTRUMENT_BODY_COLS + 1];
				memcpy(buffer, w->dirpath + ((unsigned short)strlen(w->dirpath - INSTRUMENT_BODY_COLS - 4)), INSTRUMENT_BODY_COLS - 4);
				printf("\033[%d;%dH%.*s", y + 4, x + 3, INSTRUMENT_BODY_COLS - 4, buffer);
			} else
			{
				printf("\033[%d;%dH%.*s", y + 4, x - 1 + (INSTRUMENT_BODY_COLS - (unsigned short)strlen(w->dirpath) + 1) / 2, INSTRUMENT_BODY_COLS - 4, w->dirpath);
			}

			struct dirent *dirent = readdir(w->dir);
			unsigned int dirc = 0;
			unsigned short ya;
			char testdirpath[NAME_MAX + 1];
			DIR *testdir;
			while (dirent)
			{
				if (strcmp(dirent->d_name, ".") && strcmp(dirent->d_name, ".."))
				{
					ya = y + 11 - w->instrumentindex + dirc;
					if (ya > y + 4 && ya < y + INSTRUMENT_TYPE_ROWS + 4)
					{
						strcpy(testdirpath, w->dirpath);
						strcat(testdirpath, "/");
						strcat(testdirpath, dirent->d_name);
						testdir = opendir(testdirpath);
						if (testdir == NULL) /* add a trailing slash if the file is a directory */
							printf("\033[%d;%dH%.*s", ya, x + (INSTRUMENT_BODY_COLS - w->dirmaxwidth) / 2, INSTRUMENT_BODY_COLS - 5, dirent->d_name);
						else
						{
							printf("\033[%d;%dH%.*s/", ya, x + (INSTRUMENT_BODY_COLS - w->dirmaxwidth) / 2, INSTRUMENT_BODY_COLS - 5, dirent->d_name);
							closedir(testdir);
						}
					}
					dirc++;
				}
				dirent = readdir(w->dir);
			}
			rewinddir(w->dir);
			if (dirc != w->dirc) changeDirectory(); /* recount the entries if a file gets added or removed */

			if (dirc == 0)
			{
				w->dirmaxwidth = INSTRUMENT_BODY_COLS - 4;
				printf("\033[%d;%dH%.*s", y + 11, x + (INSTRUMENT_BODY_COLS - (unsigned short)strlen("(empty directory)") + 1) / 2, INSTRUMENT_BODY_COLS - 4, "(empty directory)");
				printf("\033[%d;%dH", y + 11, x + (INSTRUMENT_BODY_COLS - (unsigned short)strlen("(empty directory)") + 1) / 2 + 1);
			} else printf("\033[%d;%dH", y + 11 + w->fyoffset, x + (INSTRUMENT_BODY_COLS - w->dirmaxwidth) / 2);
		}
	}
}

void instrumentAdjustUp(instrument *iv, short index)
{
	switch (index)
	{
		case MIN_INSTRUMENT_INDEX:      break;
		case MIN_INSTRUMENT_INDEX + 1:  break;
		default:
			unsigned short tic = 0; /* type index count */
			if (iv->type < INSTRUMENT_TYPE_COUNT)
				tic = t->f[iv->type].indexc;

			if (index == tic + 0)
			{
				iv->fader[0]++;
				iv->fader[1]++;
			} else if (iv->type < INSTRUMENT_TYPE_COUNT && t->f[iv->type].adjustUp)
				t->f[iv->type].adjustUp(iv, index);
			break;
	}
}
void instrumentAdjustDown(instrument *iv, short index)
{
	switch (index)
	{
		case MIN_INSTRUMENT_INDEX:      break;
		case MIN_INSTRUMENT_INDEX + 1:  break;
		default:
			unsigned short tic = 0; /* type index count */
			if (iv->type < INSTRUMENT_TYPE_COUNT)
				tic = t->f[iv->type].indexc;

			if (index == tic + 0)
			{
				iv->fader[0]--;
				iv->fader[1]--;
			} else if (iv->type < INSTRUMENT_TYPE_COUNT && t->f[iv->type].adjustDown)
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
			if (w->instrumentlockv > INST_REC_LOCK_OK) break;

			iv->type--;
			w->instrumentlocki = s->instrumenti[w->instrument];
			w->instrumentlockv = INST_GLOBAL_LOCK_PREP_FREE;
			break;
		default:
			unsigned short tic = 0; /* type index count */
			if (iv->type < INSTRUMENT_TYPE_COUNT)
				tic = t->f[iv->type].indexc;

			if (index == tic + 0)
			{
				if (iv->fader[1] > iv->fader[0])
					iv->fader[0]++;
				else
					iv->fader[1]--;
			} else if (index == tic + 1)
			{
				w->instrumentsend--;
				if (w->instrumentsend < 0)
					w->instrumentsend = 15;
			} else if (index == tic + 2)
			{
				iv->send[w->instrumentsend]--;
				if (iv->send[w->instrumentsend] < 0)
					iv->send[w->instrumentsend] = 15;
				if (w->instrumentlockv == INST_GLOBAL_LOCK_OK)
				{
					w->instrumentlocki = s->instrumenti[w->instrument];
					w->instrumentlockv = w->instrumentsend + 16;
				}
			} else if (iv->type < INSTRUMENT_TYPE_COUNT && t->f[iv->type].adjustLeft)
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
			if (w->instrumentlockv > INST_REC_LOCK_OK) break;

			iv->type++;
			w->instrumentlocki = s->instrumenti[w->instrument];
			w->instrumentlockv = INST_GLOBAL_LOCK_PREP_FREE;
			break;
		default:
			unsigned short tic = 0; /* type index count */
			if (iv->type < INSTRUMENT_TYPE_COUNT)
				tic = t->f[iv->type].indexc;

			if (index == tic + 0)
			{
				if (iv->fader[0] > iv->fader[1])
					iv->fader[1]++;
				else
					iv->fader[0]--;
			} else if (index == tic + 1)
			{
				w->instrumentsend++;
				if (w->instrumentsend > 15)
					w->instrumentsend = 0;
			} else if (index == tic + 2)
			{
				iv->send[w->instrumentsend]++;
				if (iv->send[w->instrumentsend] > 15)
					iv->send[w->instrumentsend] = 0;
				if (w->instrumentlockv == INST_GLOBAL_LOCK_OK)
				{
					w->instrumentlocki = s->instrumenti[w->instrument];
					w->instrumentlockv = w->instrumentsend + 16;
				}
			} else if (iv->type < INSTRUMENT_TYPE_COUNT && t->f[iv->type].adjustRight)
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
		if (strcmp(dirent->d_name, ".") && strcmp(dirent->d_name, ".."))
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
					w->previewchanneltrigger = 0;
					switch (getchar())
					{
						case 'O':
							switch (getchar())
							{
								case 'P':
									pushInstrumentHistoryIfNew(s->instrumentv[s->instrumenti[w->instrument]]);
									w->popup = 0;
									w->mode = 0;
									break;
								case 'Q':
									pushInstrumentHistoryIfNew(s->instrumentv[s->instrumenti[w->instrument]]);
									w->popup = 3;
									w->mode = 0;
									w->effectindex = MIN_EFFECT_INDEX;
									w->effectoffset = 0;
									break;
								// case 'R': w->tab = 3; break;
								// case 'S': w->tab = 4; break;
							}
							redraw();
							break;
						case '[':
							instrument *iv = s->instrumentv[s->instrumenti[w->instrument]];
							unsigned short tic = 0; /* type index count */
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
										if (iv && iv->type < INSTRUMENT_TYPE_COUNT)
											tic = t->f[iv->type].indexc;

										/* ensure fieldpointer is still in range */
										if (w->instrumentindex <= MIN_INSTRUMENT_INDEX + 1)
											w->fieldpointer = 0;
										else if (w->instrumentindex == tic)
										{ if (w->fieldpointer > 3) w->fieldpointer = 3; }
										else if (w->instrumentindex > tic)
											w->fieldpointer = 0;
										else if (w->instrumentindex > MIN_INSTRUMENT_INDEX + 1
												&& iv && iv->type < INSTRUMENT_TYPE_COUNT
												&& t->f[iv->type].endFieldPointer)
											t->f[iv->type].endFieldPointer(&w->fieldpointer, w->instrumentindex);
									}
									redraw();
									break;
								case 'B': /* down arrow */
									if (w->mode > 1) /* adjust */
										instrumentAdjustDown(iv, w->instrumentindex);
									else
									{
										pushInstrumentHistoryIfNew(s->instrumentv[s->instrumenti[w->instrument]]);
										if (s->instrumenti[w->instrument] != 0)
											w->instrumentindex++;
										if (iv && iv->type < INSTRUMENT_TYPE_COUNT)
											tic = t->f[iv->type].indexc;
										if (w->instrumentindex > tic + 2)
											w->instrumentindex = tic + 2;

										/* ensure fieldpointer is still in range */
										if (w->instrumentindex <= MIN_INSTRUMENT_INDEX + 1)
											w->fieldpointer = 0;
										else if (w->instrumentindex == tic)
										{ if (w->fieldpointer > 3) w->fieldpointer = 3; }
										else if (w->instrumentindex > tic)
											w->fieldpointer = 0;
										else if (w->instrumentindex > MIN_INSTRUMENT_INDEX + 1
												&& iv && iv->type < INSTRUMENT_TYPE_COUNT
												&& t->f[iv->type].endFieldPointer)
											t->f[iv->type].endFieldPointer(&w->fieldpointer, w->instrumentindex);
									}
									redraw();
									break;
								case 'D': /* left arrow */
									if (iv && iv->type < INSTRUMENT_TYPE_COUNT)
										tic = t->f[iv->type].indexc;

									if (w->instrumentindex == MIN_INSTRUMENT_INDEX || w->mode > 1) /* adjust */
										instrumentAdjustLeft(iv, w->instrumentindex);
									else if (w->instrumentindex == tic)
									{
										if (w->fieldpointer == 0)
											w->fieldpointer = 3;
										else w->fieldpointer--;
									} else if (w->instrumentindex > tic)
										instrumentAdjustLeft(iv, w->instrumentindex);
									else switch (w->instrumentindex)
									{
										case MIN_INSTRUMENT_INDEX:
										case MIN_INSTRUMENT_INDEX + 1:
											break;
										default:
											if (iv && iv->type < INSTRUMENT_TYPE_COUNT
													&& t->f[iv->type].decFieldPointer)
												t->f[iv->type].decFieldPointer(&w->fieldpointer, w->instrumentindex);
											break;
									}
									redraw();
									break;
								case 'C': /* right arrow */
									if (iv && iv->type < INSTRUMENT_TYPE_COUNT)
										tic = t->f[iv->type].indexc;

									if (w->instrumentindex == MIN_INSTRUMENT_INDEX || w->mode > 1) /* adjust */
										instrumentAdjustRight(iv, w->instrumentindex);
									else if (w->instrumentindex == tic)
									{
										w->fieldpointer++;
										if (w->fieldpointer > 3)
											w->fieldpointer = 0;
									} else if (w->instrumentindex > tic)
										instrumentAdjustRight(iv, w->instrumentindex);
									else switch (w->instrumentindex)
									{
										case MIN_INSTRUMENT_INDEX:
										case MIN_INSTRUMENT_INDEX + 1:
											break;
										default:
											if (iv && iv->type < INSTRUMENT_TYPE_COUNT
													&& t->f[iv->type].incFieldPointer)
												t->f[iv->type].incFieldPointer(&w->fieldpointer, w->instrumentindex);
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
									unsigned short tic = 0; /* type index count */
									if (iv && iv->type < INSTRUMENT_TYPE_COUNT)
										tic = t->f[iv->type].indexc;

									if (w->instrumentindex == tic)
										w->fieldpointer = 3;
									else if (w->instrumentindex > tic)
										w->fieldpointer = 1;
									else switch (w->instrumentindex)
									{
										case MIN_INSTRUMENT_INDEX:     w->fieldpointer = 0; break;
										case MIN_INSTRUMENT_INDEX + 1: w->fieldpointer = 1; break;
										default:
											if (iv && iv->type < INSTRUMENT_TYPE_COUNT
													&& t->f[iv->type].endFieldPointer)
												t->f[iv->type].endFieldPointer(&w->fieldpointer, w->instrumentindex);
											break;
									}
									redraw();
									break;
								case '5': /* page up */
									getchar(); /* burn through the tilde */
									break;
								case '6': /* page down */
									getchar(); /* burn through the tilde */
									break;
								case 'M': /* mouse */
									int button = getchar();
									int x = getchar() - 32;
									int y = getchar() - 32;
									switch (button)
									{
										case 32 + 64: /* scroll up */
											break;
										case 33 + 64: /* scroll down */
											break;
										case 35: /* release click */
											/* leave adjust mode */
											if (w->mode > 3) w->mode = w->mode - 4;
											if (w->mode > 1) w->mode = w->mode - 2;
											break;
										case BUTTON1:
											pushInstrumentHistoryIfNew(s->instrumentv[s->instrumenti[w->instrument]]);
											if (x < w->instrumentcelloffset
													|| x > w->instrumentcelloffset + INSTRUMENT_BODY_COLS - 1
													|| y < w->instrumentrowoffset - 1
													|| y > w->instrumentrowoffset + INSTRUMENT_BODY_ROWS)
											{
												w->popup = 0;
												break;
											}

											if (y < w->instrumentrowoffset)
											{ /* header */
												w->fieldpointer = 0;
												w->instrumentindex = MIN_INSTRUMENT_INDEX;
											} else if (y < w->instrumentrowoffset + 3)
											{ /* type selection */
												if (x - w->instrumentcelloffset < 12) w->fieldpointer = 0;
												else                                  w->fieldpointer = 1;
												w->instrumentindex = MIN_INSTRUMENT_INDEX + 1;
											} else if (y >= w->instrumentrowoffset + INSTRUMENT_TYPE_ROWS + 5)
											{ /* type contents */
												unsigned short tic = 0; /* type index count */
												iv = s->instrumentv[s->instrumenti[w->instrument]];
												if (iv && iv->type < INSTRUMENT_TYPE_COUNT)
													tic = t->f[iv->type].indexc;

												if (x - w->instrumentcelloffset < 22)
												{ /* fader */
													if (x - w->instrumentcelloffset < 11)      w->fieldpointer = 0;
													else if (x - w->instrumentcelloffset > 14) w->fieldpointer = 3;
													else w->fieldpointer = x - w->instrumentcelloffset - 11;
													w->instrumentindex = tic + 0;
												} else
												{ /* sends */
													w->fieldpointer = 0;
													if (y > w->instrumentrowoffset + INSTRUMENT_TYPE_ROWS + 5)
														w->instrumentindex = tic + 2;
													else
														w->instrumentindex = tic + 1;
												}
											} else
											{
												if (!s->instrumenti[w->instrument]) break;
												if (iv->type < INSTRUMENT_TYPE_COUNT && t->f[iv->type].mouseToIndex)
													t->f[iv->type].mouseToIndex(
															y - w->instrumentrowoffset + MIN_INSTRUMENT_INDEX - 1,
															x - w->instrumentcelloffset - 2,
															&w->instrumentindex, &w->fieldpointer);
											}

											/* enter mouse adjust mode */
											if (w->mode < 2) w->mode = w->mode + 4;
											if (w->mode < 4) w->mode = w->mode + 2;
											w->mousey = y;
											w->mousex = x;
											break;
										case BUTTON1 + 32:
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
									unsigned short tic = 0; /* type index count */
									if (iv && iv->type < INSTRUMENT_TYPE_COUNT)
										tic = t->f[iv->type].indexc;

									if (w->instrumentindex == tic)
									{
										switch (input)
										{
											case 1: /* ^a */
												iv->fader[0]++;
												iv->fader[1]++;
												break;
											case 24: /* ^x */
												iv->fader[0]--;
												iv->fader[1]--;
												break;
											default: /* 2 stages cos each half of the field points to a different memory address */
												if (w->fieldpointer < 2)
													switch (input)
													{
														case '0':           updateField(w->fieldpointer, 2, (uint32_t *)&iv->fader[0], 0);  w->fieldpointer++; if (w->fieldpointer > 3) w->fieldpointer = 0; break;
														case '1':           updateField(w->fieldpointer, 2, (uint32_t *)&iv->fader[0], 1);  w->fieldpointer++; if (w->fieldpointer > 3) w->fieldpointer = 0; break;
														case '2':           updateField(w->fieldpointer, 2, (uint32_t *)&iv->fader[0], 2);  w->fieldpointer++; if (w->fieldpointer > 3) w->fieldpointer = 0; break;
														case '3':           updateField(w->fieldpointer, 2, (uint32_t *)&iv->fader[0], 3);  w->fieldpointer++; if (w->fieldpointer > 3) w->fieldpointer = 0; break;
														case '4':           updateField(w->fieldpointer, 2, (uint32_t *)&iv->fader[0], 4);  w->fieldpointer++; if (w->fieldpointer > 3) w->fieldpointer = 0; break;
														case '5':           updateField(w->fieldpointer, 2, (uint32_t *)&iv->fader[0], 5);  w->fieldpointer++; if (w->fieldpointer > 3) w->fieldpointer = 0; break;
														case '6':           updateField(w->fieldpointer, 2, (uint32_t *)&iv->fader[0], 6);  w->fieldpointer++; if (w->fieldpointer > 3) w->fieldpointer = 0; break;
														case '7':           updateField(w->fieldpointer, 2, (uint32_t *)&iv->fader[0], 7);  w->fieldpointer++; if (w->fieldpointer > 3) w->fieldpointer = 0; break;
														case '8':           updateField(w->fieldpointer, 2, (uint32_t *)&iv->fader[0], 8);  w->fieldpointer++; if (w->fieldpointer > 3) w->fieldpointer = 0; break;
														case '9':           updateField(w->fieldpointer, 2, (uint32_t *)&iv->fader[0], 9);  w->fieldpointer++; if (w->fieldpointer > 3) w->fieldpointer = 0; break;
														case 'A': case 'a': updateField(w->fieldpointer, 2, (uint32_t *)&iv->fader[0], 10); w->fieldpointer++; if (w->fieldpointer > 3) w->fieldpointer = 0; break;
														case 'B': case 'b': updateField(w->fieldpointer, 2, (uint32_t *)&iv->fader[0], 11); w->fieldpointer++; if (w->fieldpointer > 3) w->fieldpointer = 0; break;
														case 'C': case 'c': updateField(w->fieldpointer, 2, (uint32_t *)&iv->fader[0], 12); w->fieldpointer++; if (w->fieldpointer > 3) w->fieldpointer = 0; break;
														case 'D': case 'd': updateField(w->fieldpointer, 2, (uint32_t *)&iv->fader[0], 13); w->fieldpointer++; if (w->fieldpointer > 3) w->fieldpointer = 0; break;
														case 'E': case 'e': updateField(w->fieldpointer, 2, (uint32_t *)&iv->fader[0], 14); w->fieldpointer++; if (w->fieldpointer > 3) w->fieldpointer = 0; break;
														case 'F': case 'f': updateField(w->fieldpointer, 2, (uint32_t *)&iv->fader[0], 15); w->fieldpointer++; if (w->fieldpointer > 3) w->fieldpointer = 0; break;
													}
												else
													switch (input)
													{
														case '0':           updateField(w->fieldpointer - 2, 2, (uint32_t *)&iv->fader[1], 0);  w->fieldpointer++; if (w->fieldpointer > 3) w->fieldpointer = 0; break;
														case '1':           updateField(w->fieldpointer - 2, 2, (uint32_t *)&iv->fader[1], 1);  w->fieldpointer++; if (w->fieldpointer > 3) w->fieldpointer = 0; break;
														case '2':           updateField(w->fieldpointer - 2, 2, (uint32_t *)&iv->fader[1], 2);  w->fieldpointer++; if (w->fieldpointer > 3) w->fieldpointer = 0; break;
														case '3':           updateField(w->fieldpointer - 2, 2, (uint32_t *)&iv->fader[1], 3);  w->fieldpointer++; if (w->fieldpointer > 3) w->fieldpointer = 0; break;
														case '4':           updateField(w->fieldpointer - 2, 2, (uint32_t *)&iv->fader[1], 4);  w->fieldpointer++; if (w->fieldpointer > 3) w->fieldpointer = 0; break;
														case '5':           updateField(w->fieldpointer - 2, 2, (uint32_t *)&iv->fader[1], 5);  w->fieldpointer++; if (w->fieldpointer > 3) w->fieldpointer = 0; break;
														case '6':           updateField(w->fieldpointer - 2, 2, (uint32_t *)&iv->fader[1], 6);  w->fieldpointer++; if (w->fieldpointer > 3) w->fieldpointer = 0; break;
														case '7':           updateField(w->fieldpointer - 2, 2, (uint32_t *)&iv->fader[1], 7);  w->fieldpointer++; if (w->fieldpointer > 3) w->fieldpointer = 0; break;
														case '8':           updateField(w->fieldpointer - 2, 2, (uint32_t *)&iv->fader[1], 8);  w->fieldpointer++; if (w->fieldpointer > 3) w->fieldpointer = 0; break;
														case '9':           updateField(w->fieldpointer - 2, 2, (uint32_t *)&iv->fader[1], 9);  w->fieldpointer++; if (w->fieldpointer > 3) w->fieldpointer = 0; break;
														case 'A': case 'a': updateField(w->fieldpointer - 2, 2, (uint32_t *)&iv->fader[1], 10); w->fieldpointer++; if (w->fieldpointer > 3) w->fieldpointer = 0; break;
														case 'B': case 'b': updateField(w->fieldpointer - 2, 2, (uint32_t *)&iv->fader[1], 11); w->fieldpointer++; if (w->fieldpointer > 3) w->fieldpointer = 0; break;
														case 'C': case 'c': updateField(w->fieldpointer - 2, 2, (uint32_t *)&iv->fader[1], 12); w->fieldpointer++; if (w->fieldpointer > 3) w->fieldpointer = 0; break;
														case 'D': case 'd': updateField(w->fieldpointer - 2, 2, (uint32_t *)&iv->fader[1], 13); w->fieldpointer++; if (w->fieldpointer > 3) w->fieldpointer = 0; break;
														case 'E': case 'e': updateField(w->fieldpointer - 2, 2, (uint32_t *)&iv->fader[1], 14); w->fieldpointer++; if (w->fieldpointer > 3) w->fieldpointer = 0; break;
														case 'F': case 'f': updateField(w->fieldpointer - 2, 2, (uint32_t *)&iv->fader[1], 15); w->fieldpointer++; if (w->fieldpointer > 3) w->fieldpointer = 0; break;
													}
												break;
										}
										redraw();
									} else if (w->instrumentindex == tic + 1)
									{
										switch (input)
										{
											case 1: /* ^a */
												w->instrumentsend++;
												if (w->instrumentsend > 15)
													w->instrumentsend = 0;
												break;
											case 24: /* ^x */
												w->instrumentsend--;
												if (w->instrumentsend < 0)
													w->instrumentsend = 15;
												break;
											case '0':           w->instrumentsend = 0;  break;
											case '1':           w->instrumentsend = 1;  break;
											case '2':           w->instrumentsend = 2;  break;
											case '3':           w->instrumentsend = 3;  break;
											case '4':           w->instrumentsend = 4;  break;
											case '5':           w->instrumentsend = 5;  break;
											case '6':           w->instrumentsend = 6;  break;
											case '7':           w->instrumentsend = 7;  break;
											case '8':           w->instrumentsend = 8;  break;
											case '9':           w->instrumentsend = 9;  break;
											case 'A': case 'a': w->instrumentsend = 10; break;
											case 'B': case 'b': w->instrumentsend = 11; break;
											case 'C': case 'c': w->instrumentsend = 12; break;
											case 'D': case 'd': w->instrumentsend = 13; break;
											case 'E': case 'e': w->instrumentsend = 14; break;
											case 'F': case 'f': w->instrumentsend = 15; break;
										}
										redraw();
									} else if (w->instrumentindex == tic + 2)
									{
										if (!s->instrumenti[w->instrument]) break;
										iv = s->instrumentv[s->instrumenti[w->instrument]];

										switch (input)
										{
											case 1: /* ^a */
												iv->send[w->instrumentsend]++;
												if (iv->send[w->instrumentsend] > 15)
													iv->send[w->instrumentsend] = 0;
												break;
											case 24: /* ^x */
												iv->send[w->instrumentsend]--;
												if (iv->send[w->instrumentsend] < 0)
													iv->send[w->instrumentsend] = 15;
												break;
											case '0':           iv->send[w->instrumentsend] = 0;  break;
											case '1':           iv->send[w->instrumentsend] = 1;  break;
											case '2':           iv->send[w->instrumentsend] = 2;  break;
											case '3':           iv->send[w->instrumentsend] = 3;  break;
											case '4':           iv->send[w->instrumentsend] = 4;  break;
											case '5':           iv->send[w->instrumentsend] = 5;  break;
											case '6':           iv->send[w->instrumentsend] = 6;  break;
											case '7':           iv->send[w->instrumentsend] = 7;  break;
											case '8':           iv->send[w->instrumentsend] = 8;  break;
											case '9':           iv->send[w->instrumentsend] = 9;  break;
											case 'A': case 'a': iv->send[w->instrumentsend] = 10; break;
											case 'B': case 'b': iv->send[w->instrumentsend] = 11; break;
											case 'C': case 'c': iv->send[w->instrumentsend] = 12; break;
											case 'D': case 'd': iv->send[w->instrumentsend] = 13; break;
											case 'E': case 'e': iv->send[w->instrumentsend] = 14; break;
											case 'F': case 'f': iv->send[w->instrumentsend] = 15; break;
										}
										if (w->instrumentlockv == INST_GLOBAL_LOCK_OK)
										{
											w->instrumentlocki = s->instrumenti[w->instrument];
											w->instrumentlockv = w->instrumentsend + 16;
										}
										redraw();
									} else switch (w->instrumentindex)
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
									previewNote(charToNote(input, w->octave), w->instrument);
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
								case 'Q':
									pushInstrumentHistoryIfNew(s->instrumentv[s->instrumenti[w->instrument]]);
									w->popup = 3;
									w->mode = 0;
									w->effectindex = MIN_EFFECT_INDEX;
									w->effectoffset = 0;
									break;
								// case 'R': w->tab = 3; break;
								// case 'S': w->tab = 4; break;
							}
							redraw();
							break;
						case '[':
							switch (getchar())
							{
								case 'A': /* up arrow */
									if (w->previewsamplestatus == 1) w->previewsamplestatus = 2;
									if (w->instrumentindex > 0)
										w->instrumentindex--;
									redraw();
									break;
								case 'B': /* down arrow */
									if (w->previewsamplestatus == 1) w->previewsamplestatus = 2;
									if (w->instrumentindex < w->dirc - 1)
										w->instrumentindex++;
									redraw();
									break;
								case 'D': /* left arrow */
									if (w->previewsamplestatus == 1) w->previewsamplestatus = 2;
									dirname(w->dirpath);
									changeDirectory();
									w->instrumentindex = 0;
									redraw();
									break;
								case 'C': /* right arrow */
									if (w->previewsamplestatus == 1) w->previewsamplestatus = 2;
									char newpath[NAME_MAX + 1];
									char oldpath[NAME_MAX + 1]; strcpy(oldpath, w->dirpath);
									switch (getSubdir(newpath))
									{
										case 1: /* file */
											loadSample(w->instrument, newpath);
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
								case 'H': /* home */
									if (w->previewsamplestatus == 1) w->previewsamplestatus = 2;
									w->instrumentindex = 0;
									redraw();
									break;
								case '4': /* end */
									if (w->previewsamplestatus == 1) w->previewsamplestatus = 2;
									getchar(); /* burn ~ */
									w->instrumentindex = w->dirc - 1;
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
										case 32 + 64: /* scroll up */
											w->instrumentindex -= 3;
											if (w->instrumentindex < 0)
												w->instrumentindex = 0;
											break;
										case 33 + 64: /* scroll down */
											w->instrumentindex += 3;
											if (w->instrumentindex > w->dirc - 1)
												w->instrumentindex = w->dirc - 1;
											break;
										case 35: /* release click */
											w->instrumentindex += w->fyoffset;
											w->fyoffset = 0;
											/* leave adjust mode */
											if (w->mode > 3) w->mode = w->mode - 4;
											if (w->mode > 1) w->mode = w->mode - 2;
											break;
										case BUTTON1:
											if (y < w->instrumentrowoffset + 3
													|| y > w->instrumentrowoffset + INSTRUMENT_TYPE_ROWS + 4)
											{
												w->popup = 1;
												w->instrumentindex = 0;
											}
											if (x < w->instrumentcelloffset
													|| x > w->instrumentcelloffset + INSTRUMENT_BODY_COLS
													|| y < w->instrumentrowoffset - 1
													|| y > w->instrumentrowoffset + INSTRUMENT_BODY_ROWS)
											{
												pushInstrumentHistoryIfNew(s->instrumentv[s->instrumenti[w->instrument]]);
												w->popup = 0;
												break;
											}
										case BUTTON1 + 32:
											if (y > ws.ws_row - 1 || y < 3) break; /* ignore clicking out of range */
											w->fyoffset = y - (w->instrumentrowoffset + 11); /* magic number */
											if (w->fyoffset + w->instrumentindex < 0)
												w->fyoffset -= w->instrumentindex + w->fyoffset;
											if (w->fyoffset + w->instrumentindex > w->dirc - 1)
												w->fyoffset -= w->instrumentindex + w->fyoffset - w->dirc + 1;
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
										w->previewchannel.r.note = charToNote(input, w->octave);
										w->previewchanneltrigger = 3; // trigger the preview sample
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
