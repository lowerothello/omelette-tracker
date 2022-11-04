BrowserState *fbstate;

typedef struct
{
	struct dirent *dirent;
	DIR           *dir;
	char          *path;
	uint32_t       lineptr;
	uint32_t       size;
	void         (*callback)(char *path);
} FileBrowserData;

char *fileBrowserGetTitle(void *data)
{
	FileBrowserData *fbd = data;
	fbd->lineptr = 0;
	rewinddir(fbd->dir);
	return strdup(fbd->path);
}

bool showpath(char *path)
{
	if (!strcmp(path, ".")
	 || !strcmp(path, "..")
	 || !strcmp(path, "lost+found"))
		return 0;
	return 1;
}

/* sets *newpath to the hovered file, returns 1 if newpath is a file and 2 if newpath is a directory */
int getSubdir(FileBrowserData *fbd, uint32_t cursor, char **newpath)
{
	rewinddir(fbd->dir);
	struct dirent *dirent = readdir(fbd->dir);
	unsigned int dirc = 0;
	DIR *testdir;
	while (dirent)
	{
		if (showpath(dirent->d_name))
		{
			if (dirc == cursor)
			{
				*newpath = malloc(strlen(fbd->path) + strlen(dirent->d_name) + 2); /* 1 for '/', one for '\0' */
				strcpy(*newpath, fbd->path);
				strcat(*newpath, "/");
				strcat(*newpath, dirent->d_name);

				rewinddir(fbd->dir);

				testdir = opendir(*newpath);
				if     (testdir == NULL)  return 1; /* file */
				else { closedir(testdir); return 2; /* directory */ }
				break;
			} dirc++;
		} dirent = readdir(fbd->dir);
	}
	rewinddir(fbd->dir);
	return 0; /* fail case, reachable only for empty dirs i think? idk */
}

bool fileBrowserGetNext(void *data)
{
	FileBrowserData *fbd = data;

	fbd->dirent = readdir(fbd->dir);

	while (fbd->dirent && !showpath(fbd->dirent->d_name))
		fbd->dirent = readdir(fbd->dir);

	return (fbd->dirent);
}
void fileBrowserDrawLine(BrowserState *b, int y)
{
	FileBrowserData *fbd = b->data;

	char  *testdirpath = malloc(strlen(fbd->path) + strlen(fbd->dirent->d_name) + 2); /* 1 for '/', one for '\0' */
	strcpy(testdirpath, fbd->path);
	strcat(testdirpath, "/");
	strcat(testdirpath, fbd->dirent->d_name);

	DIR *testdir = opendir(testdirpath);
	free(testdirpath);

	if (testdir)
	{ /* directory */
		closedir(testdir);

		/* the CSI X here is to clear the background */
		printf("\033[%d;%dH\033[%dX%.*s/", y, b->x, b->w, b->w - 1, fbd->dirent->d_name);
	} else
	{ /* file */
		/* the CSI X here is to clear the background */
		printf("\033[%d;%dH\033[%dX%.*s", y, b->x, b->w, b->w, fbd->dirent->d_name);
	}
}

uint32_t fileBrowserGetLineCount(void *data) { return ((FileBrowserData *)data)->size; }

/* assume swap2 is a (Sample), free it if it is set */
void cb_freeSemargSample(Event *e)
{
	size_t key = (size_t)e->callbackarg;
	if (key) previewFileNote(key);
	if (e->src) free(e->src);
	e->src = NULL;
}
void freePreviewSample(void)
{ /* fully atomic */
	if (w->previewsample)
	{
		Event e;
		e.sem = M_SEM_SWAP_REQ;
		e.dest = (void **)&w->previewsample;
		e.src = NULL; /* explicitly typed for clarity */
		e.callback = cb_freeSemargSample;
		pushEvent(&e);
	}
}
void fileBrowserCursorCB(void *data) { freePreviewSample(); }

/* tries to load path into fbd */
void changeDirectory(FileBrowserData *fbd, char *path)
{
	DIR *testdir = opendir(path);
	if (testdir) /* path is valid */
	{
		if (fbd->path != path) /* TODO: dirname is janky */
		{
			if (fbd->path) free(fbd->path);
			fbd->path = strdup(path);
		}
		if (fbd->dir) { closedir(fbd->dir); } fbd->dir = testdir;

		fbd->size = 0;
		while (fileBrowserGetNext(fbd))
			fbd->size++;

		rewinddir(fbd->dir);
	}
}

void fileBrowserCommit(BrowserState *b)
{
	FileBrowserData *fbd = b->data;

	char *newpath;
	int ret = getSubdir(fbd, b->cursor, &newpath);
	DEBUG=ret; p->redraw=1;
	switch (ret)
	{
		case 1: /* file */
			fbd->callback(newpath);
			break;
		case 2: /* directory */
			changeDirectory(fbd, newpath);
			b->cursor = 0;
			fileBrowserCursorCB(fbd);
			break;
	}
	if (newpath) free(newpath);
}

BrowserState *initFileBrowser(char *path, void (*callback)(char *path))
{
	BrowserState *ret = calloc(1, sizeof(BrowserState));

	ret->getTitle     = fileBrowserGetTitle;
	ret->getNext      = fileBrowserGetNext;
	ret->drawLine     = fileBrowserDrawLine;
	ret->getLineCount = fileBrowserGetLineCount;
	ret->cursorCB     = fileBrowserCursorCB;
	ret->commit       = fileBrowserCommit;

	ret->data = calloc(1, sizeof(FileBrowserData));
	((FileBrowserData *)ret->data)->callback = callback;

	changeDirectory(ret->data, path);

	return ret;
}
void freeFileBrowser(BrowserState *b)
{
	closedir(((FileBrowserData *)b->data)->dir );
	free    (((FileBrowserData *)b->data)->path);
	free(b->data);
}


void filebrowserBackspace(BrowserState *b)
{
	changeDirectory(b->data, dirname(((FileBrowserData *)b->data)->path));
	b->cursor = 0;
	freePreviewSample();
}
void filebrowserPreview(FileBrowserData *fbd, uint32_t cursor, int input)
{
	char *path;
	if (getSubdir(fbd, cursor, &path) == 1) /* hovered file is not a directory */
	{
		if (!w->previewsample)
		{
			Sample *newpreviewsample = _loadSample(path);
			if (newpreviewsample)
			{
				Event e;
				e.sem = M_SEM_SWAP_PREVIEWSAMPLE_PREVIEW_REQ;
				e.dest = (void **)&w->previewsample;
				e.src = newpreviewsample;
				e.callback = cb_freeSemargSample;
				e.callbackarg = (void *)((size_t)input);
				pushEvent(&e);
			} else strcpy(w->command.error, "failed to preview sample, out of memory");
		} else previewFileNote(input);
	} free(path);
}
