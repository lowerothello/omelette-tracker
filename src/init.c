static void common_cleanup(int ret)
{
	freeRawInput();
	freeEvents();
	if (p) free(p);

	clearControls();
	clearTooltip();

	if (fbstate) freeFileBrowser(fbstate);
	if (pbstate) freePluginBrowser(pbstate);

	freeEffectDB();

	cleanupTerminal();

	exit(ret);
}

void cleanup(int ret)
{
	struct timespec req;

	if (s)
	{
		stopPlayback();
		while (s->playing)
		{ /* wait until stopPlayback() finishes fully */
			req.tv_sec = 0;
			req.tv_nsec = UPDATE_DELAY;
			nanosleep(&req, &req);
		}
	}
	cleanAudio();

	for (int i = 0; i < PREVIEW_TRACKS; i++)
		_delTrack(s, &w->previewtrack[i]);

	freeWaveform();

	freeSong(s);
	freeWindow(w);

	common_cleanup(ret);
}

static void sigwinch(int signal) { p->resize = 1; }

void init(int argc, char *argv[])
{
	srand(time(NULL)); /* seed rand */

	initTerminal();
	initRawInput();
	initEffectDB();

	if (initAudio()) common_cleanup(1);

	samplerate = getSampleRate();
	buffersize = getBufferSize();
	rampmax = samplerate / 1000 * RAMP_MS;

	p = malloc(sizeof(PlaybackInfo));
	if (!p) { puts("out of memory"); common_cleanup(1); }
	memset(p, 0, sizeof(PlaybackInfo));

	w = allocWindow();
	if (!w) { puts("out of memory"); common_cleanup(1); }
	p->w = w;

	s = allocSong();
	if (!s) { puts("out of memory"); common_cleanup(1); }
	p->s = s;

	fbstate = initFileBrowser(SAMPLES_DIR, sampleLoadCallback);
	pbstate = initPluginBrowser();

	startAudio();

	if (argc > 1)
	{
		strcpy(w->filepath, argv[1]);
		Event e;
		e.sem = M_SEM_RELOAD_REQ;
		e.callback = cb_reloadFile;
		pushEvent(&e);
	} else reapplyBpm(); /* implied by the other branch */

	/* trap signals */
	signal(SIGINT  , cleanup );
	signal(SIGTERM , cleanup );
	// signal(SIGSEGV , cleanup ); /* TODO: might be unsafe? */
	// signal(SIGFPE  , cleanup ); /* TODO: might be unsafe? */
	// signal(SIGABRT , cleanup ); /* TODO: might be unsafe? */
	signal(SIGWINCH, sigwinch);


	resetInput();

	p->resize = 1;
puts("finished initializing");
}
