static void common_cleanup(int ret)
{
	if (input_api.free)
		input_api.free();

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
		while (w->playing)
		{ /* wait until stopPlayback() finishes fully */
			req.tv_sec = 0;
			req.tv_nsec = UPDATE_DELAY;
			nanosleep(&req, &req);
		}
	}

	VALGRIND_PRINTF("audio backend pre-close\n");
	if (audio_api.clean)
		audio_api.clean();
	killProcThreads();
	VALGRIND_PRINTF("audio backend post-close\n");

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
	initEffectDB();

	if (audioInitAPI()) common_cleanup(1);
	inputInitAPI();

	rampmax = samplerate / 1000 * RAMP_MS;

	p = calloc(1, sizeof(PlaybackInfo));
	if (!p) { puts("out of memory"); common_cleanup(1); }

	w = allocWindow();
	if (!w) { puts("out of memory"); common_cleanup(1); }
	p->w = w;

	s = addSong();
	if (!s) { puts("out of memory"); common_cleanup(1); }
	p->s = s;

	setBpm(&w->spr, s->songbpm); /* ensure that spr is set before starting the audio thread, for sanity reasons */

	fbstate = initFileBrowser(SAMPLES_DIR);
	pbstate = initPluginBrowser();

	spawnProcThreads();
	audio_api.start();

	if (argc > 1)
	{
		strcpy(w->filepath, argv[1]);
		Event e;
		e.sem = M_SEM_RELOAD_REQ;
		e.callback = cb_reloadFile;
		pushEvent(&e);
	} else reapplyBpm(); /* implied by the other branch */

	/* trap signals */
	signal(SIGINT, cleanup);
	signal(SIGTERM, cleanup);
	signal(SIGWINCH, sigwinch);


	resetInput();

	p->resize = 1;
}
