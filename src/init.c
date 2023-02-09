#ifdef DEBUG_DISABLE_AUDIO_OUTPUT
pthread_t dummyprocessthread;
#endif

static void common_cleanup(int ret)
{
	freeRawInput();
	freeEvents();
	if (p) free(p);

	clearControls();
	clearTooltip();

	if (fbstate) freeFileBrowser(fbstate);
	if (pbstate) freePluginBrowser(pbstate);

	freeLadspaDB();
	freeLV2DB();

	cleanupTerminal();

	exit(ret);
}

void cleanup(int ret)
{
#ifndef DEBUG_DISABLE_AUDIO_OUTPUT
	struct timespec req;

	if (client)
	{
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
		jack_deactivate(client);
		jack_client_close(client);
	}
#else
	pthread_cancel(dummyprocessthread);
	pthread_join(dummyprocessthread, NULL);
#endif

	for (int i = 0; i < PREVIEW_TRACKS; i++)
		_delTrack(s, &w->previewtrack[i]);

	freeWaveform();

	freeSong(s);
	freeWindow(w);

	common_cleanup(ret);
}

static void sigwinch(int signal) { p->resize = 1; }

void jackError(const char *message)
{ /* stub jack error callback to hide errors, TODO: do something more useful */
	return;
}

void init(int argc, char *argv[])
{
	srand(time(NULL)); /* seed rand */

	initTerminal();
	initRawInput();
	initLadspaDB();
	initLV2DB();

	p = malloc(sizeof(PlaybackInfo));
	if (!p) { puts("out of memory"); common_cleanup(1); }
	memset(p, 0, sizeof(PlaybackInfo));

#ifndef DEBUG_DISABLE_AUDIO_OUTPUT
	jack_set_error_function(jackError);
	client = jack_client_open("omelette", JackNullOption, NULL);
	if (!client) { puts("failed to init the jack client"); common_cleanup(1); }

	p->in.l =    jack_port_register(client, "in_l",     JACK_DEFAULT_AUDIO_TYPE, JackPortIsInput |JackPortIsTerminal, 0);
	p->in.r =    jack_port_register(client, "in_r",     JACK_DEFAULT_AUDIO_TYPE, JackPortIsInput |JackPortIsTerminal, 0);
	p->out.l =   jack_port_register(client, "out_l",    JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput|JackPortIsTerminal, 0);
	p->out.r =   jack_port_register(client, "out_r",    JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput|JackPortIsTerminal, 0);
	p->midiout = jack_port_register(client, "out_midi", JACK_DEFAULT_MIDI_TYPE,  JackPortIsOutput|JackPortIsTerminal, 0);

	samplerate = jack_get_sample_rate(client);
	buffersize = jack_get_buffer_size(client);

	rampmax = samplerate / 1000 * RAMP_MS;
#else
	samplerate = DEBUG_DUMMY_SAMPLERATE;
	buffersize = DEBUG_DUMMY_BUFFERSIZE;
#endif

	w = allocWindow();
	if (!w) { puts("out of memory"); common_cleanup(1); }
	p->w = w;

	s = allocSong();
	if (!s) { puts("out of memory"); common_cleanup(1); }
	p->s = s;

#ifndef DEBUG_DISABLE_AUDIO_OUTPUT
	jack_set_process_callback(client, (int(*)(jack_nframes_t, void*))process, p);
	jack_activate(client);
#else
	pthread_create(&dummyprocessthread, NULL, (void*(*)(void*))dummyProcess, p);
#endif

	fbstate = initFileBrowser(SAMPLES_DIR, sampleLoadCallback);
	pbstate = initPluginBrowser();

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
}
