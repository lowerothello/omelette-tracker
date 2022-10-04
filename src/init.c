void common_cleanup(int ret)
{
	clearControls(&cc);
	clearTooltip(&tt);

	if (w) { free(w); w = NULL; }
	if (s) { delSong(s); s = NULL; }
	if (p) { free(p); p = NULL; }
	if (b) { freeBackground(b); b = NULL; }

	fcntl(0, F_SETFL, 0); /* reset to blocking stdin reads */
	tcsetattr(1, TCSANOW, &origterm); /* reset to the original termios */
	puts("\033[?1002l"); /* disable mouse */
	puts("\033[?1049l"); /* reset to the front buffer */

	exit(ret);
}

void cleanup(int ret)
{
#ifndef DEBUG_DISABLE_AUDIO_THREAD
	if (client)
	{
		if (s)
		{
			struct timespec req;

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
#endif

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

		for (short i = 0; i < w->vbchannelc; i++)
			free(w->vbtrig[i]);
		for (short i = 0; i < w->pbchannelc; i++)
			free(w->pbvariantv[i]);
	}

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

	/* jack stuffs */
	p = malloc(sizeof(PlaybackInfo));
	if (!p) { puts("out of memory"); common_cleanup(1); }
	memset(p, 0, sizeof(PlaybackInfo));

#ifndef DEBUG_DISABLE_AUDIO_THREAD
	client = jack_client_open("omelette", JackNullOption, NULL);
	if (!client) { puts("failed to init the jack client"); common_cleanup(1); }

	p->in.l =    jack_port_register(client, "in_l",     JACK_DEFAULT_AUDIO_TYPE, JackPortIsInput |JackPortIsTerminal, 0);
	p->in.r =    jack_port_register(client, "in_r",     JACK_DEFAULT_AUDIO_TYPE, JackPortIsInput |JackPortIsTerminal, 0);
	p->out.l =   jack_port_register(client, "out_l",    JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput|JackPortIsTerminal, 0);
	p->out.r =   jack_port_register(client, "out_r",    JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput|JackPortIsTerminal, 0);
	p->midiout = jack_port_register(client, "out_midi", JACK_DEFAULT_MIDI_TYPE,  JackPortIsOutput|JackPortIsTerminal, 0);

	samplerate = jack_get_sample_rate(client);
	buffersize = jack_get_buffer_size(client);

	rampmax =      samplerate / 1000 * RAMP_MS;
	grainrampmax = samplerate / 1000 * TIMESTRETCH_RAMP_MS;
#endif

	w = calloc(1, sizeof(Window));
	if (!w) { puts("out of memory"); common_cleanup(1); }
	w->octave = 4;
	w->defvariantlength = 0x7;
	w->channelbuffer.macroc = 1;
	w->trackerfy = STATE_ROWS;

	s = addSong();
	if (!s) { puts("out of memory"); common_cleanup(1); }

	regenGlobalRowc(s);

	/* 4 starting channels */
	addChannel(s, s->channelc);
	addChannel(s, s->channelc);
	addChannel(s, s->channelc);
	addChannel(s, s->channelc);

	p->s = s;
	p->w = w;

	/* need to be called before the jack client is activated */
	initBackground();
	__addChannel(&w->previewchannel);


#ifndef DEBUG_DISABLE_AUDIO_THREAD
	jack_set_process_callback(client, process, p);
	jack_activate(client);
#endif


#ifdef SAMPLES_DIR
	strcpy(w->dirpath, SAMPLES_DIR);
#else
	getcwd(w->dirpath, sizeof(w->dirpath));
#endif

	changeDirectory();

	if (argc > 1)
	{
		strcpy(w->newfilename, argv[1]);
		p->sem = M_SEM_RELOAD_REQ;
	}

	resize(0);
}
