#define FILEBROWSER_PAGE_HEIGHT 18

/* window->dirpath should be set to the path */
int changeDirectory(void)
{
	if (w->dir != NULL) { closedir(w->dir); w->dir = NULL; }

	/* avoid paths that start with "//" */
	if (w->dirpath[1] == '/')
	{
		char tmp[NAME_MAX];
		memcpy(tmp, w->dirpath+1, NAME_MAX);
		memcpy(w->dirpath, tmp, NAME_MAX);
	}

	w->dir = opendir(w->dirpath);
	if (w->dir == NULL)
	{
		strcpy(w->command.error, "failed to open directory");
		return 1; /* failed, probably because window->dir doesn't point to a dir */
	}

	struct dirent *dirent = readdir(w->dir);
	w->dirc = 0;
	w->dirmaxwidth = 1;
	while (dirent != NULL)
	{
		if (
				   strcmp(dirent->d_name, ".")
				&& strcmp(dirent->d_name, "..")
				&& strcmp(dirent->d_name, "lost+found"))
		{
			if (strlen(dirent->d_name)+2 > w->dirmaxwidth)
				w->dirmaxwidth = strlen(dirent->d_name)+2;
			w->dirc++;
		}
		dirent = readdir(w->dir);
	}
	rewinddir(w->dir);
	w->dirmaxwidth = MIN(w->dirmaxwidth, ws.ws_col - 4);
	w->dircols = MAX(MIN((ws.ws_col - 8) / w->dirmaxwidth, (w->dirc - 1) / 4), 1);
	return 0;
}

void drawFilebrowser(void)
{
	printf("\033[%d;%dH\033[1mFILEBROWSER\033[m", CHANNEL_ROW-2, (ws.ws_col-11) / 2);

	struct dirent *dirent = readdir(w->dir);
	unsigned int dirc, yo;
	unsigned short xo, xw;
	char testdirpath[NAME_MAX + 1];
	DIR *testdir;
	xw = w->dirmaxwidth * w->dircols;
	xo = (ws.ws_col-xw) / 2 + 1;
	yo = ws.ws_row/2 - w->filebrowserindex / w->dircols;

	if (strlen(w->dirpath) > ws.ws_col-4)
		printf("\033[%d;%dH%.*s", CHANNEL_ROW, 2, ws.ws_col-4, w->dirpath + ((unsigned short)strlen(w->dirpath - ws.ws_col-4)));
	else
		printf("\033[%d;%dH%.*s", CHANNEL_ROW, (ws.ws_col - (unsigned short)strlen(w->dirpath)) / 2, ws.ws_col-4, w->dirpath);

	dirc = 0; /* gcc makes this var static, even with -O0. thanks stallman very cool (tcc handles this correctly, literally a gcc bug) */
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

			if (yo > CHANNEL_ROW && yo < ws.ws_row)
			{
				strcpy(testdirpath, w->dirpath);
				strcat(testdirpath, "/");
				strcat(testdirpath, dirent->d_name);

				testdir = opendir(testdirpath);
				if (testdir) /* add a trailing slash if the file is a directory */
				{
					closedir(testdir);
					printf("\033[%d;%dH\033[%dX%.*s/", yo, xo + w->dirmaxwidth * wcol, w->dirmaxwidth, w->dirmaxwidth-1, dirent->d_name);
				} else printf("\033[%d;%dH\033[%dX%.*s", yo, xo + w->dirmaxwidth * wcol, w->dirmaxwidth, w->dirmaxwidth, dirent->d_name);
			}
			dirc++;
			dirent = readdir(w->dir);
		}
		yo++;
	}

	rewinddir(w->dir);
	if (dirc != w->dirc) changeDirectory(); /* recount the entries if a file gets added or removed */
	if (!dirc)
	{
		printf("\033[%d;%dH%.*s", ws.ws_row/2, xo + (xw - (unsigned short)strlen("(empty directory)") + 1) / 2, ws.ws_col - 4, "(empty directory)");
		printf("\033[%d;%dH",     ws.ws_row/2, xo + (xw - (unsigned short)strlen("(empty directory)") + 1) / 2 + 1);
	} else printf("\033[%d;%dH",  ws.ws_row/2 + w->fyoffset, xo + w->dirmaxwidth * (w->filebrowserindex % w->dircols));
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
			if (dirc == w->filebrowserindex)
			{
				strcpy(newpath, w->dirpath);
				strcat(newpath, "/");
				strcat(newpath, dirent->d_name);
				testdir = opendir(newpath);
				rewinddir(w->dir);
				if (testdir == NULL) return 1; /* file */
				else { closedir(testdir); return 2; /* directory */ }
				redraw(); break;
			} dirc++;
		} dirent = readdir(w->dir);
	}
	rewinddir(w->dir);
	return 0; /* fail case, reachable only for empty dirs i think? idk */
}

void filebrowserInput(int input)
{
	char newpath[NAME_MAX + 1], oldpath[NAME_MAX + 1];
	int button, x, y;
	unsigned short xo;
	switch (input)
	{
		case 10: case 13: /* return */
			strcpy(oldpath, w->dirpath);
			switch (getSubdir(newpath))
			{
				case 1: /* file */
					w->popup = 0;
					w->filebrowserCallback(newpath);
					break;
				case 2: /* directory */
					strcpy(w->dirpath, newpath);
					if (changeDirectory())
					{
						strcpy(w->dirpath, oldpath);
						changeDirectory();
					} else w->filebrowserindex = 0;
					break;
			}
			redraw();
			break;
		case 127: case '\b': /* backspace */
			dirname(w->dirpath);
			changeDirectory();
			w->filebrowserindex = 0;
			redraw(); break;
		case '\033':
			switch (getchar())
			{
				case 10: case 13: /* alt+return */
					strcpy(oldpath, w->dirpath);
					switch (getSubdir(newpath))
					{
						case 1: /* file */
							w->filebrowserCallback(newpath);
							break;
						case 2: /* directory */
							strcpy(w->dirpath, newpath);
							if (changeDirectory())
							{
								strcpy(w->dirpath, oldpath);
								changeDirectory();
							} else w->filebrowserindex = 0;
							break;
					} redraw(); break;
				case 'O':
					switch (getchar())
					{
						case 'P': /* xterm f1 */ showTracker(); break;
						case 'Q': /* xterm f2 */ showInstrument(); break;
					} redraw(); break;
				case '[':
					switch (getchar())
					{
						case '[':
							switch (getchar())
							{
								case 'A': /* linux f1 */ showTracker(); break;
								case 'B': /* linux f2 */ showInstrument(); break;
								case 'E': /* linux f5 */ startPlayback(); break;
							} redraw(); break;
						case 'A': /* up arrow */
							w->filebrowserindex -= w->dircols;
							if (w->filebrowserindex < 0) w->filebrowserindex = 0;
							redraw(); break;
						case 'B': /* down arrow */
							w->filebrowserindex += w->dircols;
							if (w->filebrowserindex > w->dirc - 1) w->filebrowserindex = w->dirc - 1;
							redraw(); break;
						case 'D': /* left arrow */
							w->filebrowserindex--;
							if (w->filebrowserindex < 0) w->filebrowserindex = 0;
							redraw(); break;
						case 'C': /* right arrow */
							w->filebrowserindex++;
							if (w->filebrowserindex > w->dirc - 1) w->filebrowserindex = w->dirc - 1;
							redraw(); break;
						case 'H': /* xterm home */
							w->filebrowserindex = 0;
							redraw(); break;
						case '4': /* end */
							if (getchar() == '~')
							{
								w->filebrowserindex = w->dirc - 1;
								redraw();
							} break;
						case '5': /* page up */
							getchar();
							w->filebrowserindex -= w->dircols * FILEBROWSER_PAGE_HEIGHT;
							if (w->filebrowserindex < 0) w->filebrowserindex = 0;
							redraw(); break;
						case '6': /* page down */
							getchar();
							w->filebrowserindex += w->dircols * FILEBROWSER_PAGE_HEIGHT;
							if (w->filebrowserindex > w->dirc - 1) w->filebrowserindex = w->dirc - 1;
							redraw(); break;
						case '1': /* mod+arrow / f5 - f8 */
							switch (getchar())
							{
								case '5': /* xterm f5, play  */ getchar(); startPlayback(); break;
								case '7': /* f6, stop        */ getchar(); stopPlayback(); break;
								case ';': /* mod+arrow       */ getchar(); break;
								case '~': /* linux home      */ w->filebrowserindex = 0; redraw(); break;
							} break;
						case 'M': /* mouse */
							button = getchar();
							x = getchar() - 32;
							y = getchar() - 32;
							switch (button)
							{
								case WHEEL_UP: case WHEEL_UP_CTRL: /* scroll up */
									w->filebrowserindex -= WHEEL_SPEED * w->dircols;
									if (w->filebrowserindex < 0)
										w->filebrowserindex = 0;
									break;
								case WHEEL_DOWN: case WHEEL_DOWN_CTRL: /* scroll down */
									w->filebrowserindex += WHEEL_SPEED * w->dircols;
									if (w->filebrowserindex > w->dirc - 1)
										w->filebrowserindex = w->dirc - 1;
									break;
								case BUTTON_RELEASE: case BUTTON_RELEASE_CTRL: /* release click */
									w->filebrowserindex += w->fyoffset * w->dircols;
									w->fyoffset = 0;
									break;
								case BUTTON1: case BUTTON3:
								case BUTTON1_CTRL: case BUTTON3_CTRL:
								case BUTTON1_HOLD: case BUTTON1_HOLD_CTRL:
									if (y <= CHANNEL_ROW || y == ws.ws_row) break; /* ignore clicking out of range */

									// xw = w->dirmaxwidth * w->dircols;
									xo = (ws.ws_col - w->dirmaxwidth*w->dircols) / 2 + 1;
									w->filebrowserindex -= w->filebrowserindex % w->dircols; /* pull to the first column */

									if (x >= xo + w->dirmaxwidth * w->dircols)
										w->filebrowserindex += w->dircols - 1;
									else if (x >= xo)
										w->filebrowserindex += (x - xo) / w->dirmaxwidth;

									w->fyoffset = y - ws.ws_row/2;
									if (w->fyoffset + w->filebrowserindex / w->dircols < 0)
										w->fyoffset -= w->filebrowserindex / w->dircols + w->fyoffset;
									if (w->fyoffset + w->filebrowserindex / w->dircols > (w->dirc - 1) / w->dircols)
										w->fyoffset -= w->filebrowserindex / w->dircols + w->fyoffset - (w->dirc - 1) / w->dircols;
									if (w->fyoffset * w->dircols + w->filebrowserindex > w->dirc - 1)
										w->fyoffset--;

									if (button == BUTTON3 || button == BUTTON3_CTRL)
									{
										w->filebrowserindex += w->fyoffset * w->dircols;
										w->fyoffset = 0;
										char newpath[NAME_MAX + 1]; /* TODO: functionize, do on release */
										char oldpath[NAME_MAX + 1]; strcpy(oldpath, w->dirpath);
										switch (getSubdir(newpath))
										{
											case 1: /* file */
												w->popup = 0;
												w->filebrowserindex = 0;
												w->filebrowserCallback(newpath);
												break;
											case 2: /* directory */
												strcpy(w->dirpath, newpath);
												if (changeDirectory())
												{
													strcpy(w->dirpath, oldpath);
													changeDirectory();
												} else w->filebrowserindex = 0;
												break;
										}
									} break;
							} redraw(); break;
					} break;
				default:
					switch (w->mode)
					{
						case 0: /* leave the popup */
							w->popup = 1;
							break;
					} redraw(); break;
			} break;
	}
}
