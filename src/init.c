void common_cleanup(int ret)
{
	if (w) { free(w); w = NULL; }
	if (s) { delSong(s); s = NULL; }
	if (p) { free(p); p = NULL; }
	if (b) { freeBackground(b); b = NULL; }

	clearControls(&cc);
	clearTooltip(&tt);

	freeLadspaDB();

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

		if (w->dir) closedir(w->dir);
		if (w->recbuffer) free(w->recbuffer);
		if (w->waveformcanvas) free_canvas(w->waveformcanvas);
		if (w->waveformbuffer) free_buffer(w->waveformbuffer);
		__delChannel(&w->previewchannel);
		if (w->previewchannel.data.trig) free(w->previewchannel.data.trig);
		if (w->previewchannel.data.songv) free(w->previewchannel.data.songv);
		if (w->previewsample)
		{
			if (w->previewsample->data) free(w->previewsample->data);
			free(w->previewsample);
		}

		for (short i = 0; i < w->vbchannelc; i++)
			free(w->vbtrig[i]);
		for (short i = 0; i < w->pbchannelc; i++)
			free(w->pbvariantv[i]);
	}
	printf("\033[%d;0H\033[2K", ws.ws_row);

	common_cleanup(ret);
}

void init(int argc, char **argv)
{
	/* seed rand */
	srand(time(NULL));
	puts("\033[?1049h"); /* switch to the back buffer */
	puts("\033[?1002h"); /* enable mouse events */

	struct termios term;
	tcgetattr(1, &term);
	origterm = term;
	term.c_lflag &= (~ECHO & ~ICANON);
	tcsetattr(1, TCSANOW, &term); /* disable ECHO and ICANON */

	fcntl(0, F_SETFL, O_NONBLOCK); /* non-blocking reads */

	/* trap signals */
	signal(SIGINT, SIG_IGN);
	signal(SIGTERM, &cleanup);

	initLadspaDB();

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
	w->channelbuffer.macroc = 1;
	w->trackerfy = STATE_ROWS;

	s = addSong();
	if (!s) { puts("out of memory"); common_cleanup(1); }

	p->s = s;
	p->w = w;

	/* need to be called before the jack client is activated */
	initBackground();
	__addChannel(&w->previewchannel);


#ifndef DEBUG_DISABLE_AUDIO_OUTPUT
	jack_set_process_callback(client, process, p);
	jack_activate(client);
#else
	pthread_create(&dummyprocessthread, NULL, process, p);
#endif


#ifdef SAMPLES_DIR
	strcpy(w->dirpath, SAMPLES_DIR);
#else
	getcwd(w->dirpath, sizeof(w->dirpath));
#endif

	ioctl(1, TIOCGWINSZ, &ws);
	w->dirx = w->diry = 0;
	w->dirh = ws.ws_row - 1;
	w->dirw = ws.ws_col;
	changeDirectory();

	if (argc > 1)
	{
		strcpy(w->newfilename, argv[1]);
		Event e;
		e.sem = M_SEM_RELOAD_REQ;
		e.callback = cb_reloadFile;
		pushEvent(&e);
	} else reapplyBpm(); /* implied by the other branch */

	resize(0);
}
