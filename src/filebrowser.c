typedef struct FileBrowserData
{
	struct dirent *dirent;
	DIR           *dir;
	char          *path;
	uint32_t       lineptr;
	uint32_t       size;
	void         (*callback)(char *path);
} FileBrowserData;

static char *fileBrowserGetTitle(void *data)
{
	FileBrowserData *fbd = data;
	fbd->lineptr = 0;
	rewinddir(fbd->dir);
	return strdup(fbd->path);
}

static bool showpath(char *path)
{
	if (!strcmp(path, ".")
	 || !strcmp(path, "..")
	 || !strcmp(path, "lost+found"))
		return 0;
	return 1;
}

/* sets *newpath to the hovered file, returns 1 if newpath is a file and 2 if newpath is a directory */
static int getSubdir(FileBrowserData *fbd, uint32_t cursor, char **newpath)
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

static bool fileBrowserGetNext(void *data)
{
	FileBrowserData *fbd = data;

	fbd->dirent = readdir(fbd->dir);

	while (fbd->dirent && !showpath(fbd->dirent->d_name))
		fbd->dirent = readdir(fbd->dir);

	return (fbd->dirent);
}

/* buffer should be at least 8 bytes big */
char *humanReadableSize(double bytes, char *buffer)
{
	const char units[] = { '\0', 'K', 'M', 'G', 'T', 'P', 'E', 'Z', 'Y' };

	int i = 0;
	while (bytes > 1024)
	{
		bytes /= 1024;
		i++;
	}

	sprintf(buffer, "%.1f%c", bytes, units[i]);
	return buffer;
}

static void fileBrowserDrawLine(BrowserState *b, int y)
{
	FileBrowserData *fbd = b->data;

	char *testdirpath = malloc(strlen(fbd->path) + strlen(fbd->dirent->d_name) + 2); /* 1 for '/', one for '\0' */
	strcpy(testdirpath, fbd->path);
	strcat(testdirpath, "/");
	strcat(testdirpath, fbd->dirent->d_name);

	DIR *testdir = opendir(testdirpath);
	struct stat filestat;

	if (testdir)
	{ /* directory */
		closedir(testdir);

		char dirname[strlen(fbd->dirent->d_name) + 2];
		strcpy(dirname, fbd->dirent->d_name);
		strcat(dirname, "/");
		printCulling(dirname, b->x, y, b->x, b->x + (b->w-3) - 34);
	} else /* file */
		printCulling(fbd->dirent->d_name, b->x, y, b->x, b->x + (b->w-3) - 35);

	if (stat(testdirpath, &filestat) == -1)
		printCulling(strerror(errno), b->x + (b->w-3) - strlen(strerror(errno)), y, b->x, b->x + (b->w-3));
	else
	{
		char buffer[8];
		humanReadableSize(filestat.st_size, buffer);
		printCulling(buffer, b->x + (b->w-3) - 26 - strlen(buffer), y, b->x, b->x + (b->w-3));
		printCulling(ctime(&filestat.st_mtim.tv_sec), b->x + (b->w-3) - 24, y, b->x, b->x + (b->w-3));
	}

	free(testdirpath);
}

static uint32_t fileBrowserGetLineCount(void *data) { return ((FileBrowserData *)data)->size; }

/* assume swap2 is a (Sample), free it if it is set */
static void cb_freeSemargSample(Event *e)
{
	size_t key = (size_t)e->callbackarg;
	if (key) previewFileNote(key, 0);
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
		e.callbackarg = NULL; /* nothing to preview */
		pushEvent(&e);
	}
}
static void cb_fileBrowserCursor(void *data) { freePreviewSample(); }

/* tries to load path into fbd */
static void changeDirectory(FileBrowserData *fbd, char *path)
{
	/* pop off any leading slashes */
	while (path[1] == '/')
		memcpy(path, path+1, strlen(path));

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

static void fileBrowserCommit(BrowserState *b)
{
	FileBrowserData *fbd = b->data;

	char *newpath;
	int ret = getSubdir(fbd, b->cursor, &newpath);
	switch (ret)
	{
		case 1: /* file */
			fbd->callback(newpath);
			break;
		case 2: /* directory */
			changeDirectory(fbd, newpath);
			b->cursor = 0;
			cb_fileBrowserCursor(fbd);
			break;
	}
	if (newpath) free(newpath);
}

static void sampleLoadCallbackOverload(char *path)
{
	Inst *iv = &s->inst->v[s->inst->i[w->instrument]];
	Sample *newsample = loadSample(path);
	if (newsample)
		attachSample(&((InstSamplerState*)iv->state)->sample, newsample, w->sample);
	else
		strcpy(w->command.error, "failed to load sample, out of memory");

	w->page = PAGE_INSTRUMENT;
	w->mode = MODE_NORMAL;
	w->showfilebrowser = 0;
	free(path);
}

static void sampleLoadCallbackAddCallback(Event *e)
{
	cb_addInst(e);
	sampleLoadCallbackOverload(e->callbackarg);
}

/* TODO: sample could already be loaded into p->semarg, reparent if so */
static void sampleLoadCallback(char *path)
{
	if (!path) return;

	freeWaveform();
	if (!instSafe(s->inst, w->instrument))
		addInst(w->instrument, INST_TYPE_SAMPLER, sampleLoadCallbackAddCallback, strdup(path));
	else
		sampleLoadCallbackOverload(strdup(path));
}

BrowserState *initFileBrowser(char *path)
{
	BrowserState *ret = calloc(1, sizeof(BrowserState));

	ret->getTitle     = fileBrowserGetTitle;
	ret->getNext      = fileBrowserGetNext;
	ret->drawLine     = fileBrowserDrawLine;
	ret->getLineCount = fileBrowserGetLineCount;
	ret->cursorCB     = cb_fileBrowserCursor;
	ret->commit       = fileBrowserCommit;

	ret->data = calloc(1, sizeof(FileBrowserData));
	((FileBrowserData *)ret->data)->callback = sampleLoadCallback;

	changeDirectory(ret->data, path);

	return ret;
}
void freeFileBrowser(BrowserState *b)
{
	closedir(((FileBrowserData *)b->data)->dir );
	free    (((FileBrowserData *)b->data)->path);
	free(b->data);
	free(b);
}


void fileBrowserBackspace(BrowserState *b)
{
	changeDirectory(b->data, dirname(((FileBrowserData *)b->data)->path));
	b->cursor = 0;
	freePreviewSample();
}
void fileBrowserPreview(BrowserState *b, size_t note, bool release)
{
	char *path;
	if (getSubdir(b->data, b->cursor, &path) == 1) /* hovered file is not a directory */
	{
		if (!w->previewsample)
		{
			Sample *newpreviewsample = loadSample(path);
			if (newpreviewsample)
			{
				Event e;
				e.sem = M_SEM_SWAP_REQ;
				e.dest = (void **)&w->previewsample;
				e.src = newpreviewsample;
				e.callback = cb_freeSemargSample;
				e.callbackarg = (void *)((size_t)note);
				pushEvent(&e);
			} else strcpy(w->command.error, "failed to preview sample, out of memory");
		} else previewFileNote(note, release);
	} free(path);
}
