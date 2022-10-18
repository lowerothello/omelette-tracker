/* window->dirpath should be set to the path */
int changeDirectory(void)
{
	if (w->dir != NULL) { closedir(w->dir); w->dir = NULL; }

	/* avoid paths that start with "//" cos they're ugly */
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
		return 1;
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
			w->dirmaxwidth = MAX(w->dirmaxwidth, strlen(dirent->d_name)+2);
			w->dirc++;
		}
		dirent = readdir(w->dir);
	}
	rewinddir(w->dir);
	w->dirmaxwidth = MIN(w->dirmaxwidth, w->dirw);
	w->dircols = MAX(MIN(w->dirw / w->dirmaxwidth, (w->dirc - 1)>>2), 1);
	return 0;
}

/* assume swap2 is a (Sample), free it if it is set */
void cb_freeSemargSample(Event *e)
{
	if (e->src)
	{
		if (((Sample *)e->src)->data) free(((Sample *)e->src)->data);
		free(e->src); e->src = NULL;
	}
}

void freePreviewSample(void)
{ /* fully atomic */
	Event e;
	e.sem = M_SEM_SWAP_REQ;
	e.dest = (void **)&w->previewsample;
	e.src = NULL; /* explicitly typed for clarity */
	e.callback = cb_freeSemargSample;
	pushEvent(&e);
}

void drawFilebrowser(void)
{ /* this function can smash the stack sometimes */
	// printf("\033[%d;%dH\033[1mFILEBROWSER\033[m", CHANNEL_ROW-2, (ws.ws_col-11)>>1);

	struct dirent *dirent = readdir(w->dir);
	char testdirpath[NAME_MAX + 1];
	DIR *testdir;
	unsigned short xw = w->dirmaxwidth * w->dircols;
	unsigned short xo = w->dirx + ((w->dirw-xw)>>1) + 1;
	unsigned int yo = w->diry + (w->dirh>>1) - w->filebrowserindex / w->dircols;

	if (strlen(w->dirpath) > w->dirw) printf("\033[%d;%dH%.*s", w->diry, w->dirx, w->dirw, w->dirpath + ((unsigned short)strlen(w->dirpath) - w->dirw));
	else                              printf("\033[%d;%dH%.*s", w->diry, w->dirx + ((w->dirw - (unsigned short)strlen(w->dirpath))>>1), w->dirw, w->dirpath);

	unsigned int dirc = 0; /* gcc makes this var static, even with -O0. thanks stallman very cool (tcc handles this correctly, literally a gcc bug) */
	while (dirent)
	{
		for (unsigned char wcol = 0; wcol < w->dircols; wcol++)
		{
			if (!dirent) break;
			while (!strcmp(dirent->d_name, ".")
				|| !strcmp(dirent->d_name, "..")
				|| !strcmp(dirent->d_name, "lost+found"))
				dirent = readdir(w->dir);
			if (!dirent) break;

			if (yo > w->diry && yo <= w->diry + w->dirh)
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
	if (dirc != w->dirc) changeDirectory(); /* recount the entries if a file got added or removed */
	if (!dirc)
	{
		printf("\033[%d;%dH%.*s", w->diry + (w->dirh>>1), xo + ((xw - (unsigned short)strlen("(empty directory)") + 1)>>1), w->dirw, "(empty directory)");
		printf("\033[%d;%dH",     w->diry + (w->dirh>>1), xo + ((xw - (unsigned short)strlen("(empty directory)") + 1)>>1) + 1);
	} else printf("\033[%d;%dH",  w->diry + (w->dirh>>1) + w->fyoffset, xo + w->dirmaxwidth * (w->filebrowserindex % w->dircols));
}

/* sets newpath to the hovered file, returns 1 if newpath is a file and 2 if newpath is a directory */
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
				p->dirty = 1; break;
			} dirc++;
		} dirent = readdir(w->dir);
	}
	rewinddir(w->dir);
	return 0; /* fail case, reachable only for empty dirs i think? idk */
}

void filebrowserUpArrow(int count)
{
	w->filebrowserindex -= w->dircols * count;
	if (w->filebrowserindex < 0)
		w->filebrowserindex = 0;
	freePreviewSample();
}
void filebrowserDownArrow(int count)
{
	w->filebrowserindex += w->dircols * count;
	if (w->filebrowserindex > w->dirc - 1)
		w->filebrowserindex = w->dirc - 1;
	freePreviewSample();
}
void filebrowserLeftArrow(int count)
{
	w->filebrowserindex -= count;
	if (w->filebrowserindex < 0)
		w->filebrowserindex = 0;
	freePreviewSample();
}
void filebrowserRightArrow(int count)
{
	w->filebrowserindex += count;
	if (w->filebrowserindex > w->dirc - 1)
		w->filebrowserindex = w->dirc - 1;
	freePreviewSample();
}
void filebrowserHome(void)
{
	w->filebrowserindex = 0;
	freePreviewSample();
}
void filebrowserEnd(void)
{
	w->filebrowserindex = w->dirc - 1;
	freePreviewSample();
}
void filebrowserMouse(int button, int x, int y)
{
	unsigned short xo;
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
			if (y <= w->diry || y >= w->diry + w->dirh) break; /* ignore clicking out of range */

			xo = w->dirx + ((w->dirw - w->dirmaxwidth*w->dircols)>>1) + 1;
			w->filebrowserindex -= w->filebrowserindex % w->dircols; /* pull to the first column */

			if (x >= xo + w->dirmaxwidth * w->dircols)
				w->filebrowserindex += w->dircols - 1;
			else if (x >= xo)
				w->filebrowserindex += (x - xo) / w->dirmaxwidth;

			w->fyoffset = y - (w->diry + (w->dirh>>1));
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
						w->page = 0;
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
	}
	freePreviewSample();
}
void filebrowserReturn(void)
{
	char newpath[NAME_MAX + 1], oldpath[NAME_MAX + 1];

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
			freePreviewSample(); break;
	}
}
void filebrowserBackspace(void)
{
	dirname(w->dirpath);
	changeDirectory();
	w->filebrowserindex = 0;
	freePreviewSample();
}
void filebrowserPreview(int input)
{
	char path[NAME_MAX + 1];
	if (getSubdir(path) == 1) /* semaphore is free and hovered file is not a directory */
	{
		if (!w->previewsample)
		{
			Sample *newpreviewsample = malloc(sizeof(Sample));
			SF_INFO sfinfo;
			newpreviewsample->data = _loadSample(path, &sfinfo);
			if (newpreviewsample->data)
			{
				newpreviewsample->length = sfinfo.frames;
				newpreviewsample->channels = sfinfo.channels;
				newpreviewsample->rate = sfinfo.samplerate;

				Event e;
				e.sem = M_SEM_SWAP_PREVIEWSAMPLE_PREVIEW_REQ;
				*e.dest = w->previewsample;
				e.src = newpreviewsample;
				e.callback = cb_freeSemargSample;
				e.callbackarg = (void *)((size_t)input);
				pushEvent(&e);
			} else
			{
				strcpy(w->command.error, "failed to preview sample, out of memory");
				free(newpreviewsample);
			}
		} else previewFileNote(input);
	}
}

void filebrowserInput(int input)
{
	switch (input)
	{
		case '\n': case '\r': /* return    */ filebrowserReturn   (); p->dirty = 1; break;
		case 127: case '\b':  /* backspace */ filebrowserBackspace(); p->dirty = 1; break;
		case '\033':
			switch (getchar())
			{
				case 'O':
					switch (getchar())
					{
						case 'P': /* xterm f1 */ showTracker   (); break;
						case 'Q': /* xterm f2 */ showInstrument(); break;
					} freePreviewSample(); p->dirty = 1; break;
				case '[': /* CSI */
					switch (getchar())
					{
						case '[':
							switch (getchar())
							{
								case 'A': /* linux f1 */ showTracker   (); break;
								case 'B': /* linux f2 */ showInstrument(); break;
								case 'E': /* linux f5 */ startPlayback(); break;
							} p->dirty = 1; break;
						case 'A': /* up arrow    */ filebrowserUpArrow   (1); p->dirty = 1; break;
						case 'B': /* down arrow  */ filebrowserDownArrow (1); p->dirty = 1; break;
						case 'D': /* left arrow  */ filebrowserLeftArrow (1); p->dirty = 1; break;
						case 'C': /* right arrow */ filebrowserRightArrow(1); p->dirty = 1; break;
						case 'H': /* xterm home  */ filebrowserHome(); p->dirty = 1; break;
						case '4': /* end */ if (getchar() == '~') { filebrowserEnd(); p->dirty = 1; } break;
						case '5': /* page up / shift+scrollup */ getchar(); filebrowserUpArrow  (w->dirh>>1); p->dirty = 1; break;
						case '6': /* page dn / shift+scrolldn */ getchar(); filebrowserDownArrow(w->dirh>>1); p->dirty = 1; break;
						case '1': /* mod+arrow / f5 - f8 */
							switch (getchar())
							{
								case '5': /* xterm f5   */ getchar(); startPlayback(); break;
								case '7': /*       f6   */ getchar(); stopPlayback (); break;
								case ';': /* mod+arrow  */ getchar(); break;
								case '~': /* linux home */ filebrowserHome(); p->dirty = 1; break;
							} break;
						case 'M': /* mouse */ filebrowserMouse(getchar(), getchar() - 32, getchar() - 32); p->dirty = 1; break;
					} break;
				default: /* leave the filebrowser */
					w->filebrowserCallback(NULL);
					p->dirty = 1; freePreviewSample(); break;
			} break;
		case '0': w->octave = 0; p->dirty = 1; break;
		case '1': w->octave = 1; p->dirty = 1; break;
		case '2': w->octave = 2; p->dirty = 1; break;
		case '3': w->octave = 3; p->dirty = 1; break;
		case '4': w->octave = 4; p->dirty = 1; break;
		case '5': w->octave = 5; p->dirty = 1; break;
		case '6': w->octave = 6; p->dirty = 1; break;
		case '7': w->octave = 7; p->dirty = 1; break;
		case '8': w->octave = 8; p->dirty = 1; break;
		case '9': w->octave = 9; p->dirty = 1; break;
		default: filebrowserPreview(input); break;
	}
}
