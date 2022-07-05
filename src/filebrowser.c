void drawFilebrowser(void)
{
	printf("\033[%d;%dH\033[1mFILEBROWSER\033[m", CHANNEL_ROW-2, (ws.ws_col - 10) / 2);

	unsigned short y = w->instrumentrowoffset;

	struct dirent *dirent = readdir(w->dir);
	unsigned int dirc = 0;
	unsigned short yo, xo, xw;
	char testdirpath[NAME_MAX + 1];
	DIR *testdir;
	xw = (w->dirmaxwidth + 2)*w->dircols;
	xo = (ws.ws_col-xw) / 2 + 1;
	yo = y+11 - w->instrumentindex / w->dircols;

	for (unsigned short cy = y; cy < y+7+INSTRUMENT_TYPE_ROWS; cy++)
		printf("\033[%d;%dH\033[%dX", cy, w->instrumentcelloffset, INSTRUMENT_BODY_COLS);


	if (strlen(w->dirpath) > INSTRUMENT_BODY_COLS - 4)
	{
		char buffer[INSTRUMENT_BODY_COLS + 1];
		memcpy(buffer, w->dirpath + ((unsigned short)strlen(w->dirpath - INSTRUMENT_BODY_COLS - 4)), INSTRUMENT_BODY_COLS - 4);
		printf("\033[%d;%dH%.*s", y+1, xo, xw, buffer);
	} else
	{
		printf("\033[%d;%dH%.*s", y+1, xo + (xw - (unsigned short)strlen(w->dirpath) + 1) / 2, INSTRUMENT_BODY_COLS - 4, w->dirpath);
	}

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
		printf("\033[%d;%dH%.*s", y+11, xo + (xw - (unsigned short)strlen("(empty directory)") + 1) / 2, INSTRUMENT_BODY_COLS - 4, "(empty directory)");
		printf("\033[%d;%dH", y+11, xo + (xw - (unsigned short)strlen("(empty directory)") + 1) / 2 + 1);
	} else printf("\033[%d;%dH", y+11 + w->fyoffset, xo + (w->dirmaxwidth + 2) * (w->instrumentindex % w->dircols));
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

void filebrowserInput(int input)
{
	char newpath[NAME_MAX + 1], oldpath[NAME_MAX + 1];
	int button, x, y;
	switch (input)
	{
		case 10: case 13: /* return */
			newpath[NAME_MAX + 1];
			oldpath[NAME_MAX + 1]; strcpy(oldpath, w->dirpath);
			switch (getSubdir(newpath))
			{
				case 1: /* file */
					w->popup = 0;
					w->instrumentindex = 0;
					w->filebrowserCallback(newpath);
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
					handleFKeys(getchar());
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
							if (getchar() == '~')
							{
								if (w->previewsamplestatus == 1) w->previewsamplestatus = 2;
								w->instrumentindex = w->dirc - 1;
								redraw();
							}
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
							} break;
						case 'M': /* mouse */
							if (w->previewsamplestatus == 1) w->previewsamplestatus = 2;
							button = getchar();
							x = getchar() - 32;
							y = getchar() - 32;
							switch (button)
							{
								case WHEEL_UP: case WHEEL_UP_CTRL: /* scroll up */
									w->instrumentindex -= WHEEL_SPEED * w->dircols;
									if (w->instrumentindex < 0)
										w->instrumentindex = 0;
									break;
								case WHEEL_DOWN: case WHEEL_DOWN_CTRL: /* scroll down */
									w->instrumentindex += WHEEL_SPEED * w->dircols;
									if (w->instrumentindex > w->dirc - 1)
										w->instrumentindex = w->dirc - 1;
									break;
								case BUTTON_RELEASE: case BUTTON_RELEASE_CTRL: /* release click */
									w->instrumentindex += w->fyoffset * w->dircols;
									w->fyoffset = 0;
									break;
								case BUTTON1: case BUTTON3: case BUTTON1_CTRL: case BUTTON3_CTRL:
									if (x < w->instrumentcelloffset
											|| x > w->instrumentcelloffset + INSTRUMENT_BODY_COLS - 1
											|| y < w->instrumentrowoffset
											|| y > w->instrumentrowoffset + INSTRUMENT_BODY_ROWS)
									{
										w->popup = 0;
										break;
									}
								case BUTTON1_HOLD: case BUTTON1_HOLD_CTRL:
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

									if (button == BUTTON3 || button == BUTTON3_CTRL)
									{
										w->instrumentindex += w->fyoffset * w->dircols;
										w->fyoffset = 0;
										char newpath[NAME_MAX + 1]; /* TODO: functionize, do on release */
										char oldpath[NAME_MAX + 1]; strcpy(oldpath, w->dirpath);
										switch (getSubdir(newpath))
										{
											case 1: /* file */
												w->popup = 0;
												w->instrumentindex = 0;
												w->filebrowserCallback(newpath);
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
									} break;
							}
							redraw();
							break;
					} break;
				default:
					switch (w->mode)
					{
						case 0: /* leave the popup */
							if (w->previewsamplestatus == 1) w->previewsamplestatus = 2;
							// pushInstrumentHistoryIfNew(s->instrumentv[s->instrumenti[w->instrument]]);
							w->popup = 1;
							w->instrumentindex = 0;
							break;
						case 1: /* leave preview */
							w->mode = 0;
							break;
					}
					redraw();
					break;
			} break;
		default:
			switch (w->mode)
			{
				case 0:
					switch (input)
					{
						case 'i': /* enter preview */
							w->mode = 1;
							redraw();
							break;
					}
					break;
				case 1: /* preview */
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
							/* {
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
								w->previewnote = charToNote(input);
								w->previewchannel = w->channel;
								w->previewinst = 255;
								w->previewtrigger = 3; // trigger the preview sample
							} */
							break;
					} break;
			} break;
	}
}
