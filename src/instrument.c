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
		printf("\033[%d;%dH┌───────── <- [INSTRUMENT %02X] -> ─────────┐", y - 1, x, w->instrument);
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
			printf("\033[%d;%dH   type: [%02X]                            ", y + 1, x + 1, iv->type);

			printf("\033[%d;%dH┌───────────────────────────────────────┐", y + 3, x + 1);
			for (int i = 0; i < INSTRUMENT_TYPE_ROWS; i++)
				printf("\033[%d;%dH│                                       │", y + 4 + i, x + 1);
			printf("\033[%d;%dH└───────────────────────────────────────┘", y + INSTRUMENT_TYPE_ROWS + 4, x + 1);

			printf("\033[%d;%dH  fader: [%02X][%02X]    sends  effect: [%X] ", y + INSTRUMENT_TYPE_ROWS + 5, x + 1, iv->fader[0], iv->fader[1], w->instrumentsend);
			printf("\033[%d;%dH                     -----     mix: [%X] ", y + INSTRUMENT_TYPE_ROWS + 6, x + 1, iv->send[w->instrumentsend]);
			/* if (w->instrumentrecv > INST_REC_LOCK_OK && w->instrumentreci == s->instrumenti[w->instrument])
				printf("\033[%d;%dH\033[6mREC\033[m", y + 0, x + 2); */

			if (w->popup == 1 && iv->type < INSTRUMENT_TYPE_COUNT && t->f[iv->type].draw != NULL)
				t->f[iv->type].draw(iv, w->instrument, x + 2, y + 4, &w->instrumentindex, w->fieldpointer);

			switch (w->instrumentindex)
			{
				case -2: printf("\033[%d;%dH", y - 1, x + 15 + w->fieldpointer); break;
				case -1: printf("\033[%d;%dH", y + 1, x + 11 + w->fieldpointer); break;
				default:
					unsigned short tic = 0; /* type index count */
					if (iv->type < INSTRUMENT_TYPE_COUNT)
						tic = t->f[iv->type].indexc;

					if      (w->instrumentindex == tic + 0)
						printf("\033[%d;%dH", y + INSTRUMENT_TYPE_ROWS + 5, x + 11 + w->fieldpointer);
					else if (w->instrumentindex == tic + 1)
						printf("\033[%d;%dH", y + INSTRUMENT_TYPE_ROWS + 5, x + 15 + w->fieldpointer);
					else if (w->instrumentindex == tic + 2)
						printf("\033[%d;%dH", y + INSTRUMENT_TYPE_ROWS + 5, x + 38);
					else if (w->instrumentindex == tic + 3)
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
			while (dirent != NULL)
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
		case MIN_INSTRUMENT_INDEX: break;
		case MIN_INSTRUMENT_INDEX + 1:
			if (iv->lock == INST_REC_LOCK_OK) iv->type+=0x10;
			break;
		default:
			unsigned short tic = 0; /* type index count */
			if (iv->type < INSTRUMENT_TYPE_COUNT)
				tic = t->f[iv->type].indexc;

			if      (index == tic + 0)
				iv->fader[0]+=0x10;
			else if (index == tic + 1)
				iv->fader[1]+=0x10;
			else if (index == tic + 2)
			{
				w->instrumentsend++;
				if (w->instrumentsend > 15)
					w->instrumentsend = 0;
			}
			else if (index == tic + 3)
			{
				iv->send[w->instrumentsend]++;
				if (iv->send[w->instrumentsend] > 15)
					iv->send[w->instrumentsend] = 0;
			} else if (iv->type < INSTRUMENT_TYPE_COUNT && t->f[iv->type].adjustUp != NULL)
				t->f[iv->type].adjustUp(iv, index);
			break;
	}
}
void instrumentAdjustDown(instrument *iv, short index)
{
	switch (index)
	{
		case MIN_INSTRUMENT_INDEX: break;
		case MIN_INSTRUMENT_INDEX + 1:
			if (iv->lock == INST_REC_LOCK_OK) iv->type-=16;
			break;
		default:
			unsigned short tic = 0; /* type index count */
			if (iv->type < INSTRUMENT_TYPE_COUNT)
				tic = t->f[iv->type].indexc;

			if      (index == tic + 0)
				iv->fader[0]-=0x10;
			else if (index == tic + 1)
				iv->fader[1]-=0x10;
			else if (index == tic + 2)
			{
				w->instrumentsend--;
				if (w->instrumentsend < 0)
					w->instrumentsend = 15;
			}
			else if (index == tic + 3)
			{
				iv->send[w->instrumentsend]--;
				if (iv->send[w->instrumentsend] < 0)
					iv->send[w->instrumentsend] = 15;
			} else if (iv->type < INSTRUMENT_TYPE_COUNT && t->f[iv->type].adjustDown != NULL)
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
			if (iv->lock == INST_REC_LOCK_OK) iv->type--;
			break;
		default:
			unsigned short tic = 0; /* type index count */
			if (iv->type < INSTRUMENT_TYPE_COUNT)
				tic = t->f[iv->type].indexc;

			if      (index == tic + 0)
				iv->fader[0]-=0x01;
			else if (index == tic + 1)
				iv->fader[1]-=0x01;
			else if (index == tic + 2)
			{
				w->instrumentsend--;
				if (w->instrumentsend < 0)
					w->instrumentsend = 15;
			}
			else if (index == tic + 3)
			{
				iv->send[w->instrumentsend]--;
				if (iv->send[w->instrumentsend] < 0)
					iv->send[w->instrumentsend] = 15;
			} else if (iv->type < INSTRUMENT_TYPE_COUNT && t->f[iv->type].adjustLeft != NULL)
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
			if (iv->lock == INST_REC_LOCK_OK) iv->type++;
			break;
		default:
			unsigned short tic = 0; /* type index count */
			if (iv->type < INSTRUMENT_TYPE_COUNT)
				tic = t->f[iv->type].indexc;

			if      (index == tic + 0)
				iv->fader[0]+=0x01;
			else if (index == tic + 1)
				iv->fader[1]+=0x01;
			else if (index == tic + 2)
			{
				w->instrumentsend++;
				if (w->instrumentsend > 15)
					w->instrumentsend = 0;
			}
			else if (index == tic + 3)
			{
				iv->send[w->instrumentsend]++;
				if (iv->send[w->instrumentsend] > 15)
					iv->send[w->instrumentsend] = 0;
			} else if (iv->type < INSTRUMENT_TYPE_COUNT && t->f[iv->type].adjustRight != NULL)
				t->f[iv->type].adjustRight(iv, index);
			break;
	}
}
short instrumentMouseToIndex(instrument *iv, int y, int x)
{
	if (iv->type < INSTRUMENT_TYPE_COUNT && t->f[iv->type].mouseToIndex != NULL)
		return t->f[iv->type].mouseToIndex(y, x);
	return 0; /* failsafe */
}

int getSubdir(char *newpath)
{
	struct dirent *dirent = readdir(w->dir);
	unsigned int dirc = 0;
	DIR *testdir;
	while (dirent != NULL)
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
									w->popup = 0;
									w->mode = 0;
									break;
								case 'Q':
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
							switch (getchar())
							{
								case 'A': /* up arrow */
									if (w->mode > 1) /* adjust */
										instrumentAdjustUp(iv, w->instrumentindex);
									else
									{
										w->fieldpointer = 0;
										w->instrumentindex--;
										if (w->instrumentindex < MIN_INSTRUMENT_INDEX)
											w->instrumentindex = MIN_INSTRUMENT_INDEX;
									}
									redraw();
									break;
								case 'B': /* down arrow */
									if (w->mode > 1) /* adjust */
										instrumentAdjustDown(iv, w->instrumentindex);
									else
									{
										w->fieldpointer = 0;
										if (s->instrumenti[w->instrument] != 0)
											w->instrumentindex++;
										unsigned short tic = 0; /* type index count */
										if (iv && iv->type < INSTRUMENT_TYPE_COUNT)
											tic = t->f[iv->type].indexc;
										if (w->instrumentindex > tic + 3)
											w->instrumentindex = tic + 3;
									}
									redraw();
									break;
								case 'D': /* left arrow */
									instrumentAdjustLeft(iv, w->instrumentindex);
									redraw();
									break;
								case 'C': /* right arrow */
									instrumentAdjustRight(iv, w->instrumentindex);
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
											getchar();
											break;
									}
									break;
								case 'H': /* home */
									if (w->instrumentindex == MIN_INSTRUMENT_INDEX)
										w->instrument = 0;
									else w->instrumentindex = MIN_INSTRUMENT_INDEX;
									break;
								case '4': /* end */
									getchar(); /* burn through the tilde */
									unsigned short tic = 0; /* type index count */
									if (iv->type < INSTRUMENT_TYPE_COUNT)
										tic = t->f[iv->type].indexc;
									w->instrumentindex = tic + 3;
									break;
								case '5': /* page up */
									getchar(); /* burn through the tilde */
									break;
								case '6': /* page down */
									getchar(); /* burn through the tilde */
									break;
								case 'M': /* mouse */
									w->fieldpointer = 0;
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
											if (x < w->instrumentcelloffset
													|| x > w->instrumentcelloffset + INSTRUMENT_BODY_COLS - 1
													|| y < w->instrumentrowoffset - 1
													|| y > w->instrumentrowoffset + INSTRUMENT_BODY_ROWS)
											{
												w->popup = 0;
												break;
											}

											if (y < w->instrumentrowoffset)
												w->instrumentindex = MIN_INSTRUMENT_INDEX;
											else if (y < w->instrumentrowoffset + 3)
												w->instrumentindex = MIN_INSTRUMENT_INDEX + 1;
											else if (y >= w->instrumentrowoffset + INSTRUMENT_TYPE_ROWS + 5)
											{
												unsigned short tic = 0; /* type index count */
												iv = s->instrumentv[s->instrumenti[w->instrument]];
												if (iv->type < INSTRUMENT_TYPE_COUNT)
													tic = t->f[iv->type].indexc;

												if (x - w->instrumentcelloffset - 2 < 12)
													w->instrumentindex = tic + 0;
												else if (x - w->instrumentcelloffset - 2 < 20)
													w->instrumentindex = tic + 1;
												else
												{
													if (y > w->instrumentrowoffset + INSTRUMENT_TYPE_ROWS + 5)
														w->instrumentindex = tic + 3;
													else
														w->instrumentindex = tic + 2;
												}
											} else
											{
												if (!s->instrumenti[w->instrument]) break;

												w->instrumentindex =
													instrumentMouseToIndex(iv,
															y - w->instrumentrowoffset + MIN_INSTRUMENT_INDEX - 1,
															x - w->instrumentcelloffset - 2);
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
								default:
									instrument *iv;
									switch (w->instrumentindex)
									{
										case MIN_INSTRUMENT_INDEX:
											switch (input)
											{
												case 'a': /* add */
													if (!s->instrumenti[w->instrument])
														addInstrument(s, w, t, w->instrument);
													redraw();
													break;
												case 'd': /* delete */
													if (s->instrumenti[w->instrument])
													{
														yankInstrument(s, w, w->instrument);
														delInstrument(s, w->instrument);
													}
													redraw();
													break;
												case 'y': /* yank */
													yankInstrument(s, w, w->instrument);
													redraw();
													break;
												case 'p': /* put */
													putInstrument(s, w, t, w->instrument);
													redraw();
													break;
												case 'e': /* empty */
													w->instrument = newInstrument(s, 0);
													redraw();
													break;
											}
											break;
										case MIN_INSTRUMENT_INDEX + 1:
											if (!s->instrumenti[w->instrument]) break;
											iv = s->instrumentv[s->instrumenti[w->instrument]];
											switch (input)
											{
												case 1: /* ^a */
													iv->type++;
													break;
												case 24: /* ^x */
													iv->type--;
													break;
												case '0':           updateField(&w->fieldpointer, 2, (uint32_t *)&iv->type, 0);  break;
												case '1':           updateField(&w->fieldpointer, 2, (uint32_t *)&iv->type, 1);  break;
												case '2':           updateField(&w->fieldpointer, 2, (uint32_t *)&iv->type, 2);  break;
												case '3':           updateField(&w->fieldpointer, 2, (uint32_t *)&iv->type, 3);  break;
												case '4':           updateField(&w->fieldpointer, 2, (uint32_t *)&iv->type, 4);  break;
												case '5':           updateField(&w->fieldpointer, 2, (uint32_t *)&iv->type, 5);  break;
												case '6':           updateField(&w->fieldpointer, 2, (uint32_t *)&iv->type, 6);  break;
												case '7':           updateField(&w->fieldpointer, 2, (uint32_t *)&iv->type, 7);  break;
												case '8':           updateField(&w->fieldpointer, 2, (uint32_t *)&iv->type, 8);  break;
												case '9':           updateField(&w->fieldpointer, 2, (uint32_t *)&iv->type, 9);  break;
												case 'A': case 'a': updateField(&w->fieldpointer, 2, (uint32_t *)&iv->type, 10); break;
												case 'B': case 'b': updateField(&w->fieldpointer, 2, (uint32_t *)&iv->type, 11); break;
												case 'C': case 'c': updateField(&w->fieldpointer, 2, (uint32_t *)&iv->type, 12); break;
												case 'D': case 'd': updateField(&w->fieldpointer, 2, (uint32_t *)&iv->type, 13); break;
												case 'E': case 'e': updateField(&w->fieldpointer, 2, (uint32_t *)&iv->type, 14); break;
												case 'F': case 'f': updateField(&w->fieldpointer, 2, (uint32_t *)&iv->type, 15); break;
											}
											redraw();
											break;
										default:
											if (!s->instrumenti[w->instrument]) break;
											iv = s->instrumentv[s->instrumenti[w->instrument]];
											if (iv->type < INSTRUMENT_TYPE_COUNT && t->f[iv->type].input != NULL)
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
					if (w->instrumentindex > MIN_INSTRUMENT_INDEX)
					{
						switch (w->mode)
						{
							case 0: w->mode = 2; break;
							case 1: w->mode = 3; break;
							case 2: w->mode = 0; break;
							case 3: w->mode = 1; break;
							/* don't leave mouse adjust */
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
							loadSample(s, w, t, w->instrument, newpath);
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
									w->popup = 0;
									w->instrumentindex = 0;
									w->mode = 0;
									break;
								case 'Q':
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
											loadSample(s, w, t, w->instrument, newpath);
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
											if (w->previewinstrument.sampledata != NULL)
											{
												/* iv should be valid if this branch is reached */
												sampler_state *ss = w->previewinstrument.state;
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
