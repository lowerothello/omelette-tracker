// #define _POSIX_C_SOURCE 199309L
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <time.h>
#include <limits.h>
#include <math.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <termios.h>
#include <signal.h>
#include <dirent.h>
#include <libgen.h>
#include <jack/jack.h>
typedef jack_default_audio_sample_t sample_t;
#include <sndfile.h>

#define MIN(X, Y)  ((X) < (Y) ? (X) : (Y))
#define MAX(X, Y)  ((X) > (Y) ? (X) : (Y))

uint32_t pow32(uint32_t a, uint32_t b)
{
	if (b == 0) return 1;
	uint32_t c = a;
	for (uint32_t i = 1; i < b; i++)
		c = c * a;
	return c;
}


/* version */
const unsigned char MAJOR = 0;
const unsigned char MINOR = 50;


jack_nframes_t samplerate;
jack_nframes_t rampmax;
jack_nframes_t buffersize;
struct winsize ws;
struct termios term, origterm;
int fl;

#define LINENO_COLS 5
#define ROW_COLS 17
#define ROW_FIELDS 6
#define SONG_COLS 5

/* main ramp buffer */

#define INSTRUMENT_TYPE_COUNT 2
#define MIN_EFFECT_INDEX 0

#define INSTRUMENT_BODY_COLS 80
#define INSTRUMENT_BODY_ROWS 21
#define INSTRUMENT_TYPE_ROWS 14
#define INSTRUMENT_TYPE_COLS 41

#define RECORD_LENGTH 600 /* record length, in seconds */

#define M_12_ROOT_2 1.0594630943592953

/* n=0 is the least significant digit */
char hexDigit32(uint32_t a, uint32_t n)
{ return a / pow32(16, n) % 16; }

int DEBUG;

/* prototypes */
int changeDirectory(void);
void redraw(void);
void startPlayback(void);
void stopPlayback(void);

#include "config.h"

#include "command.c"
#include "dsp.c"
#include "structures.c"

int ifMacro(row, char);

#include "input.c"
#include "instrument.c"
#include "filebrowser.c"
#include "tracker.c"
#include "process.c"

jack_client_t *client;


void redraw(void)
{
	printf("\033[2J");

	if (ws.ws_row < 24 || ws.ws_col < 80)
	{
		printf("\033[%d;%dH%s", w->centre, (ws.ws_col - (unsigned short)strlen("(terminal too small)")) / 2, "(terminal too small)");
		return;
	}

	drawTracker();
	switch (w->popup)
	{
		case 1: drawInstrument();  break;
		case 2: drawFilebrowser();  break;
	}
	drawCommand(&w->command, w->mode);

	fflush(stdout);
}

void filebrowserEditCallback(char *path)
{
	strcpy(w->newfilename, path);
	p->lock = PLAY_LOCK_START;
	w->songfx = 0;
}
void commandTabCallback(char *text)
{
	char *buffer = malloc(strlen(text) + 1);
	wordSplit(buffer, text, 0);
	if      (!strcmp(buffer, "bpm")) snprintf(text, COMMAND_LENGTH + 1, "bpm %d", s->bpm);
	else if (!strcmp(buffer, "rows")) snprintf(text, COMMAND_LENGTH + 1, "rows 0x%02x", s->patternv[s->patterni[s->songi[w->songfx]]]->rowc);
	else if (!strcmp(buffer, "highlight")) snprintf(text, COMMAND_LENGTH + 1, "highlight 0x%02x", s->rowhighlight);
	else if (!strcmp(buffer, "step")) snprintf(text, COMMAND_LENGTH + 1, "step 0x%x", w->step);
	else if (!strcmp(buffer, "octave")) snprintf(text, COMMAND_LENGTH + 1, "octave %d", w->octave);
	free(buffer);
}
int commandCallback(char *command, unsigned char *mode)
{
	char *buffer = malloc(strlen(command) + 1);
	wordSplit(buffer, command, 0);
	if      (!strcmp(buffer, "q"))   { free(buffer); buffer = NULL; return 1; }
	else if (!strcmp(buffer, "q!"))  { free(buffer); buffer = NULL; return 1; }
	else if (!strcmp(buffer, "w"))   { wordSplit(buffer, command, 1); writeSong(buffer); }
	else if (!strcmp(buffer, "wq"))
	{
		wordSplit(buffer, command, 1);
		if (!writeSong(buffer)) { free(buffer); buffer = NULL; return 1; } /* exit if writing the file succeeded */
	} else if (!strcmp(buffer, "e"))
	{
		wordSplit(buffer, command, 1);
		if (!strcmp(buffer, ""))
		{
			w->popup = 2;
			w->filebrowserCallback = &filebrowserEditCallback;
			redraw();
		} else
		{
			strcpy(w->newfilename, buffer);
			p->lock = PLAY_LOCK_START;
			w->songfx = 0;
		}
	} else if (!strcmp(buffer, "bpm"))
	{
		wordSplit(buffer, command, 1);
		char update = 0;
		if (s->songbpm == s->bpm) update = 1;
		s->songbpm = MIN(MAX(strtol(buffer, NULL, 0), 32), 255);
		if (update) w->request = REQ_BPM;
	} else if (!strcmp(buffer, "rows")) /* pattern length */
	{
		wordSplit(buffer, command, 1);
		pattern *pattern = s->patternv[s->patterni[s->songi[w->songfx]]];
		pattern->rowc = strtol(buffer, NULL, 16);
		w->defpatternlength = pattern->rowc;
		if (w->trackerfy > pattern->rowc)
			w->trackerfy = pattern->rowc;
	} else if (!strcmp(buffer, "highlight")) /* row highlight */
	{
		wordSplit(buffer, command, 1);
		s->rowhighlight = strtol(buffer, NULL, 16);
	} else if (!strcmp(buffer, "step"))
	{
		wordSplit(buffer, command, 1);
		w->step = MIN(strtol(buffer, NULL, 16), 15);
	} else if (!strcmp(buffer, "octave"))
	{
		wordSplit(buffer, command, 1);
		w->octave = MIN(MAX(strtol(buffer, NULL, 0), 0), 9);
	}

	free(buffer); buffer = NULL;
	redraw();
	return 0;
}

void startPlayback(void)
{
	s->songp = w->songfx;
	s->songr = 0;
	if (!(w->popup == 0 && w->mode != 0))
		w->trackerfy = 0;
	s->playing = PLAYING_START;
	redraw();
}
void stopPlayback(void)
{
	if (s->playing)
	{
		if (w->instrumentrecv == INST_REC_LOCK_CONT)
			w->instrumentrecv = INST_REC_LOCK_PREP_END;
		s->playing = PLAYING_PREP_STOP;
	} else w->trackerfy = 0;
	redraw();
}

int input(void)
{
	int input;
	while (1)
	{
		input = getchar(); /* read a byte from stdin if it's available */
		if (input < 0) break;
		DEBUG = input;

		if (w->mode == 255) /* command */
		{
			if (commandInput(&w->command, input, &w->mode)) return 1;
			redraw();
		} else if (input == ':') /* enter command mode */
		{
			setCommand(&w->command, &commandCallback, NULL, &commandTabCallback, 1, ":", "");
			w->mode = 255;
			redraw();
		} else if (input == 7) /* ^G, show file info */
		{
			if (strlen(w->filepath))
				sprintf(w->command.error, "\"%.*s\"", COMMAND_LENGTH - 2, w->filepath);
			else
				strcpy(w->command.error, "No file loaded");
			redraw();
		} else switch (w->popup)
		{
			case 0: trackerInput(input);     break;
			case 1: instrumentInput(input);  break;
			case 2: filebrowserInput(input);  break;
		}
	}
	return 0;
}

/* window->dirpath should be set to the path */
int changeDirectory(void)
{
	if (w->dir != NULL) closedir(w->dir);

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
	w->dirmaxwidth = 0;
	while (dirent != NULL)
	{
		if (
				   strcmp(dirent->d_name, ".")
				&& strcmp(dirent->d_name, "..")
				&& strcmp(dirent->d_name, "lost+found"))
		{
			if (strlen(dirent->d_name) > w->dirmaxwidth)
				w->dirmaxwidth = strlen(dirent->d_name);
			w->dirc++;
		}
		dirent = readdir(w->dir);
	}
	rewinddir(w->dir);
	w->dirmaxwidth = MIN(w->dirmaxwidth, INSTRUMENT_BODY_COLS - 4);
	w->dircols = MAX(MIN((INSTRUMENT_BODY_COLS - 8) / w->dirmaxwidth, (w->dirc - 1) / 4), 1);
	return 0;
}

void resize(int)
{
	ioctl(1, TIOCGWINSZ, &ws);
	w->visiblechannels =          (ws.ws_col - LINENO_COLS - 2) / ROW_COLS;
	w->trackercelloffset =       ((ws.ws_col - LINENO_COLS - 2) % ROW_COLS) / 2 + 1;
	w->instrumentcelloffset =     (ws.ws_col - INSTRUMENT_BODY_COLS) / 2 + 1;
	w->instrumentrowoffset =      (ws.ws_row - INSTRUMENT_BODY_ROWS) / 2 + 1;
	w->songvisible =               ws.ws_col / SONG_COLS;
	w->songcelloffset =           (ws.ws_col % SONG_COLS) / 2 + 1;
	w->centre =                    ws.ws_row / 2;
	redraw();
	signal(SIGWINCH, &resize); /* not sure why this needs to be redefined every time */
}

void common_cleanup(int ret)
{
	fcntl(0, F_SETFL, fl & ~O_NONBLOCK); /* reset to blocking stdin reads */
	tcsetattr(1, TCSANOW, &origterm); /* reset to the original termios */
	printf("\033[?1002l"); /* disable mouse */
	printf("\033[?1049l"); /* reset to the front buffer */
	printf("\033[0 q"); /* reset the cursor shape */

	exit(ret);
}
void cleanup(int ret)
{
	if (w->dir) closedir(w->dir);
	jack_deactivate(client);
	jack_client_close(client);

	free(w->previewinstrument.state[0]);
	for (int i = 0; i < INSTRUMENT_TYPE_COUNT; i++)
		if (w->instrumentbuffer.state[i])
		{
			t->f[i].delType(&w->instrumentbuffer.state[i]);
			w->instrumentbuffer.state[i] = NULL;
		}


	free(w);
	delSong(s);
	free(p);
	free(t);

	common_cleanup(ret);
}

int main(int argc, char **argv)
{
	printf("\033[?1049h"); /* switch to the back buffer */
	printf("\033[?1002h"); /* enable mouse events */

	tcgetattr(1, &term);
	origterm = term;
	term.c_lflag &= (~ECHO & ~ICANON);
	tcsetattr(1, TCSANOW, &term); /* disable ECHO and ICANON */

	fl = fcntl(0, F_SETFL, O_NONBLOCK); /* non-blocking stdin reads */

	/* ignore sigint to prevent habitual <C-c> breaking stuff */
	signal(SIGINT, SIG_IGN);

	signal(SIGTERM, &cleanup);


	/* jack stuffs */
	p = malloc(sizeof(playbackinfo));
	if (!p)
	{
		printf("out of memory");
		common_cleanup(1);
	}
	memset(p, 0, sizeof(playbackinfo));
	client = jack_client_open("omutrack", JackNullOption, NULL);
	if (client == NULL)
	{
		printf("failed to init the jack client");
		free(p);
		common_cleanup(1);
	}

	p->inl = jack_port_register(client, "in_l", JACK_DEFAULT_AUDIO_TYPE, JackPortIsInput, 0);
	p->inr = jack_port_register(client, "in_r", JACK_DEFAULT_AUDIO_TYPE, JackPortIsInput, 0);

	p->outl = jack_port_register(client, "out_l", JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0);
	p->outr = jack_port_register(client, "out_r", JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0);

	samplerate = jack_get_sample_rate(client);
	rampmax = samplerate / 1000 * RAMP_MS;
	buffersize = jack_get_buffer_size(client);

	w = calloc(1, sizeof(window));
	if (w == NULL)
	{
		printf("out of memory");
		free(p);
		common_cleanup(1);
	}
	w->octave = 4;
	w->defpatternlength = 0x1f;

	s = addSong();
	if (s == NULL)
	{
		printf("out of memory");
		free(w);
		free(p);
		common_cleanup(1);
	}

	p->s = s;
	p->w = w;


	jack_set_process_callback(client, process, p);
	jack_activate(client);


	w->previewinstrument.type = 0;
	w->previewinstrument.state[0] = malloc(sizeof(sampler_state));
	if (!w->previewinstrument.state)
	{
		printf("out of memory");
		jack_deactivate(client);
		jack_client_close(client);

		free(w);
		delSong(s);
		free(p);

		common_cleanup(1);
	}
	sampler_state *ss = w->previewinstrument.state[0];
	ss->volume.s = 255;


	addPattern(0, 0);
	s->songi[0] = 0;


#ifdef SAMPLES_DIR
	strcpy(w->dirpath, SAMPLES_DIR);
#else
	getcwd(w->dirpath, sizeof(w->dirpath));
#endif
	changeDirectory();


	t = calloc(1, sizeof(typetable));
	if (t == NULL)
	{
		printf("out of memory");

		if (w->dir != NULL) closedir(w->dir);
		jack_deactivate(client);
		jack_client_close(client);

		free(w->previewinstrument.state[0]);

		free(w);
		delSong(s);
		free(p);

		common_cleanup(1);
	}
	initInstrumentTypes();


	if (argc > 1)
	{
		strcpy(w->newfilename, argv[1]);
		p->lock = PLAY_LOCK_START;
	}

	resize(0);

	/* loop over input */
	struct timespec req;
	int running = 0;
	while(!running)
	{
		if (p->lock == PLAY_LOCK_CONT)
		{
			song *cs = readSong(w->newfilename);
			if (cs)
			{
				delSong(s);
				s = cs;
			}
			p->s = s;
			p->dirty = 1;
			p->lock = PLAY_LOCK_OK;
		}

		running = input();

		if (p->dirty)
		{
			p->dirty = 0;
			redraw();
		}

		/* if (w->previewsamplestatus == 3)
		{
			free(w->previewinstrument.sampledata);
			w->previewinstrument.sampledata = NULL;
			w->previewsamplestatus = 0;
		} */

		/* perform any pending instrument actions */
		changeInstrumentType(s, 0);

		/* finish freeing the record buffer */
		if (w->instrumentrecv == INST_REC_LOCK_END)
		{
			if (w->recptr > 0)
			{
				instrument *iv = s->instrumentv[w->instrumentreci];
				sampler_state *ss = iv->state[0]; /* assume 0 is the sampler type, and that it is loaded TODO: bad idea */
				if (ss->sampledata)
				{ free(ss->sampledata); ss->sampledata = NULL; }
				ss->sampledata = malloc(w->recptr * 2 * sizeof(short)); /* *2 for stereo */
				if (ss->sampledata == NULL)
				{
					strcpy(w->command.error, "saving recording failed, out of memory");
				} else
				{
					memcpy(ss->sampledata, w->recbuffer, w->recptr * 2 * sizeof(short));
					ss->samplelength = w->recptr * 2;
					ss->channels = 2;
					ss->length = w->recptr;
					ss->c5rate = samplerate;
					ss->trim[0] = 0;
					ss->trim[1] = w->recptr;
					ss->loop[0] = 0;
					ss->loop[1] = 0;
				}
			}

			free(w->recbuffer); w->recbuffer = NULL;
			w->instrumentrecv = INST_REC_LOCK_OK;
			redraw();
		}

		req.tv_sec  = 0; /* nanosleep can set this higher sometimes, so set every cycle */
		req.tv_nsec = 16666666; // wait ~>16.8ms (60hz)
		while(nanosleep(&req, &req) < 0);
	}

	cleanup(0);
}
