#ifdef DEBUG_DISABLE_AUDIO_OUTPUT
pthread_t dummyprocessthread;
#endif

void common_cleanup(int ret)
{
	if (w) { free(w); w = NULL; }
	if (s) { delSong(s); s = NULL; }
	if (p) { free(p); p = NULL; }

	clearControls(&cc);
	clearTooltip(&tt);

	freeFileBrowser(fbstate);
	free(fbstate);
	freePluginBrowser(pbstate);
	free(pbstate);

	freeNativeDB();
	freeLadspaDB();
	freeLV2DB();

	fcntl(0, F_SETFL, 0); /* reset to blocking stdin reads */
	tcsetattr(1, TCSANOW, &origterm); /* reset to the original termios */
	puts("\033[?1002l"); /* disable mouse */
	puts("\033[?1049l"); /* reset to the front buffer */

	exit(ret);
}

void cleanup(int ret)
{
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

#ifndef DEBUG_DISABLE_AUDIO_OUTPUT
		jack_deactivate(client);
		jack_client_close(client);
#else
		pthread_cancel(dummyprocessthread);
#endif
	}

	if (w)
	{
		_delInstrument(&w->instrumentbuffer);

		if (w->recbuffer) free(w->recbuffer);

		freeWaveform();

		clearTrackdata(s, &w->trackbuffer);

		_delTrack(s, &w->previewtrack);
		if (w->previewsample) free(w->previewsample);

		for (short i = 0; i < w->pbtrackc; i++)
		{
			free(w->pbvariantv[i]);
			free(w->vbtrig[i]);
		}
	}
	printf("\033[%d;0H\033[2K", ws.ws_row);

	common_cleanup(ret);
}

void sigwinch(int _) { p->resize = 1; }

void init(int argc, char **argv)
{
	srand(time(NULL)); /* seed rand */
	puts("\033[?1049h"); /* switch to the back buffer */
	puts("\033[?1002h"); /* enable mouse events */
	fcntl(0, F_SETFL, O_NONBLOCK); /* non-blocking stdin reads */

	struct termios term;
	tcgetattr(1, &term);
	origterm = term;
	term.c_lflag &= (~ECHO & ~ICANON);
	tcsetattr(1, TCSANOW, &term); /* disable ECHO and ICANON */

	initNativeDB();
	initLadspaDB();
	initLV2DB();

	/* jack stuffs */
	p = malloc(sizeof(PlaybackInfo));
	if (!p) { puts("out of memory"); common_cleanup(1); }
	memset(p, 0, sizeof(PlaybackInfo));

#ifndef DEBUG_DISABLE_AUDIO_OUTPUT
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

	w = calloc(1, sizeof(Window));
	if (!w) { puts("out of memory"); common_cleanup(1); }
	w->octave = 4;
	w->defvariantlength = 0x7;
	// w->trackbuffer.variant->macroc = 1;
	w->trackbuffer.effect = newEffectChain(NULL, NULL);
	w->trackerfy = STATE_ROWS;

	s = addSong();
	if (!s) { puts("out of memory"); common_cleanup(1); }

	p->s = s;
	p->w = w;

	/* need to be called before the jack client is activated */
	__addInstrument(&w->instrumentbuffer, INST_ALG_SIMPLE);
	__addTrack(&w->previewtrack);
	initTrackData(s, &w->previewtrack.data);


#ifndef DEBUG_DISABLE_AUDIO_OUTPUT
	jack_set_process_callback(client, process, p);
	jack_activate(client);
#else
	pthread_create(&dummyprocessthread, NULL, process, p);
#endif

	fbstate = initFileBrowser(SAMPLES_DIR, sampleLoadCallback);
	pbstate = initPluginBrowser();

	if (argc > 1)
	{
		strcpy(w->newfilename, argv[1]);
		Event e;
		e.sem = M_SEM_RELOAD_REQ;
		e.callback = cb_reloadFile;
		pushEvent(&e);
	} else reapplyBpm(); /* implied by the other branch */

	/* trap signals */
	signal(SIGINT, SIG_IGN);
	signal(SIGTERM, &cleanup);
	signal(SIGWINCH, &sigwinch);

	p->resize = 1;
}
