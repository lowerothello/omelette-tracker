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

/* libdrawille */
#include "lib/libdrawille/src/Canvas.h"

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
const unsigned char MINOR = 84;


jack_nframes_t samplerate;
jack_nframes_t rampmax, stretchrampmax;
jack_nframes_t buffersize;
jack_client_t *client;
struct winsize ws;
struct termios term, origterm;

#define LINENO_COLS 5
#define SONG_COLS 5


#define INSTRUMENT_TYPE_COUNT 2

#define CHANNEL_ROW 4
#define BORDER 2

#define RECORD_LENGTH 600 /* record length, in seconds */

#define C5 61 /* note value that represents C-5 */

/* n=0 is the least significant digit */
char hexDigit32(uint32_t a, uint32_t n)
{ return a / pow32(16, n) % 16; }

int DEBUG;

/* prototypes */
int changeDirectory(void);
void redraw(void);
void resize(int);
void startPlayback(void);
void stopPlayback(void);
void handleFKeys(int);

void resizeWaveform(void);
void changeMacro(int, char *);

#include "config.h"

#include "command.c"
#include "dsp.c"
#include "structures.c"

#include "input.c"
#include "instrument.c"
#include "filebrowser.c"
#include "waveform.c"
#include "tracker.c"
#include "song.c"
#include "background.c"
#include "process.c"


void drawRuler(void)
{
	/* top ruler */
	printf("\033[0;0H\033[2K\033[1momelette tracker\033[0;%dHv%d.%2d      %d\033[m",
			ws.ws_col - 14, MAJOR, MINOR, DEBUG);
	/* bottom ruler */
	if (w->mode < 255)
	{
		if (w->instrumentrecv == INST_REC_LOCK_CONT)
		{
			if (w->recptr == 0)
				printf("\033[%d;%dH\033[3m{REC   0s}\033[m", ws.ws_row, ws.ws_col - 50);
			else
				printf("\033[%d;%dH\033[3m{REC %3ds}\033[m", ws.ws_row, ws.ws_col - 50, w->recptr / samplerate + 1);
		}
		if (w->chord)
			printf("\033[%d;%dH%c", ws.ws_row, ws.ws_col - 23, w->chord);
		printf("\033[%d;%dH", ws.ws_row, ws.ws_col - 20);
		if (s->playing == PLAYING_STOP)
			printf("STOP  ");
		else
			printf("PLAY  ");
		printf("&%d +%x  ", w->octave, w->step);
		if (w->keyboardmacro) printf("%cxx  ", w->keyboardmacro);
		else                  printf("     ");
		printf("B%02x", s->songbpm);
	}
}
void redraw(void)
{
	fcntl(0, F_SETFL, 0); /* blocking */
	puts("\033[2J\033[?25h");

	if (ws.ws_row < 24 || ws.ws_col < 68)
	{
		printf("\033[%d;%dH%s", w->centre, (ws.ws_col - (unsigned short)strlen("(terminal too small)")) / 2, "(terminal too small)");
		fcntl(0, F_SETFL, O_NONBLOCK); /* non-blocking */
		return;
	}

	drawBackground();
	drawRuler();
	switch (w->popup)
	{
		case 0: drawTracker(); break;
		case 1: drawInstrument(); break;
		case 2: drawFilebrowser(); break;
		case 3: drawSong(); break;
		case 4: drawWaveform(); break;
	}
	drawCommand(&w->command, w->mode);

	fflush(stdout);
	fcntl(0, F_SETFL, O_NONBLOCK); /* non-blocking */
}

void filebrowserEditCallback(char *path)
{
	strcpy(w->newfilename, path);
	p->lock = PLAY_LOCK_START;
	w->songfy = 0;
}
void commandTabCallback(char *text)
{
	char *buffer = malloc(strlen(text) + 1);
	wordSplit(buffer, text, 0);
	if      (!strcmp(buffer, "bpm")) snprintf(text, COMMAND_LENGTH + 1, "bpm %d", s->bpm);
	else if (!strcmp(buffer, "rows")) snprintf(text, COMMAND_LENGTH + 1, "rows 0x%02x", s->patternv[s->patterni[s->songi[w->songfy]]]->rowc);
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
			w->songfy = 0;
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
		pattern *pattern = s->patternv[s->patterni[s->songi[w->songfy]]];
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
	if (s->patterni[s->songi[w->songfy]])
	{
		s->songp = w->songfy;
		s->songr = 0;
		if (!(w->popup == 0 && w->mode != 0))
			w->trackerfy = 0;
		s->playing = PLAYING_START;
	} else strcpy(w->command.error, "failed to start playback, invalid pattern selected");
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

/* should be called after recieving \033O */
void handleFKeys(int input)
{
	switch (input)
	{
		case 'P': /* tracker */
			if (w->popup == 1)
				switch (w->mode)
				{
					case 1: case 3: case 5: w->mode = T_MODE_INSERT; break;
					default:                w->mode = 0; break;
				}
			w->popup = 0;
			break;
		case 'Q': /* instrument */
			w->instrumentindex = MIN_INSTRUMENT_INDEX;
			if (w->popup == 0 && w->mode == T_MODE_INSERT)
				w->mode = 1;
			else
				w->mode = 0;
			w->popup = 1;
			break;
		case 'R': /* song */
			w->popup = 3;
			w->mode = 0;
			w->songfx = 0;
			break;
	}
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
		} else switch (input)
			{
				case ':': /* enter command mode */
					setCommand(&w->command, &commandCallback, NULL, &commandTabCallback, 1, ":", "");
					w->mode = 255;
					redraw();
					break;
				case 7: /* ^G, show file info */
					if (strlen(w->filepath))
						sprintf(w->command.error, "\"%.*s\"", COMMAND_LENGTH - 2, w->filepath);
					else
						strcpy(w->command.error, "No file loaded");
					redraw();
					break;
				default:
					switch (w->popup)
					{
						case 0: trackerInput(input); break;
						case 1: instrumentInput(input); break;
						case 2: filebrowserInput(input); break;
						case 3: songInput(input); break;
						case 4: waveformInput(input); break;
					}
					break;
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
	w->dirmaxwidth = 1;
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

void resize(int _)
{
	ioctl(1, TIOCGWINSZ, &ws);
	w->instrumentcelloffset = (ws.ws_col - INSTRUMENT_BODY_COLS) / 2 + 1;
	w->instrumentrowoffset =  (ws.ws_row - INSTRUMENT_BODY_ROWS) / 2 + 1;
	w->centre =                ws.ws_row / 2;

	w->waveformw = ws.ws_col * 2;
	w->waveformh = (ws.ws_row - 2) * 4;
	if (w->waveformcanvas) free_canvas(w->waveformcanvas);
	if (w->waveformbuffer) free_buffer(w->waveformbuffer);
	w->waveformcanvas = new_canvas(w->waveformw, w->waveformh);
	w->waveformbuffer = new_buffer(w->waveformcanvas);
	if (w->popup == 4)
		resizeWaveform();

	resizeBackground();

	redraw();
	signal(SIGWINCH, &resize); /* not sure why this needs to be redefined every time sometimes */
}

void common_cleanup(int ret)
{
	fcntl(0, F_SETFL, 0); /* reset to blocking stdin reads */
	tcsetattr(1, TCSANOW, &origterm); /* reset to the original termios */
	puts("\033[?1002l"); /* disable mouse */
	puts("\033[?1049l"); /* reset to the front buffer */
	puts("\033[0 q"); /* reset the cursor shape */

	exit(ret);
}
void cleanup(int ret)
{
	if (w->dir) closedir(w->dir);
	jack_deactivate(client);

	for (int i = 0; i < INSTRUMENT_TYPE_COUNT; i++)
		if (w->instrumentbuffer.state[i])
			t->f[i].delType(&w->instrumentbuffer.state[i]);

	if (w->recbuffer) free(w->recbuffer);
	if (w->recchannelbuffer) free(w->recchannelbuffer);
	if (w->waveformcanvas) free_canvas(w->waveformcanvas);
	if (w->waveformbuffer) free_buffer(w->waveformbuffer);


	free(w);
	delSong(s);
	free(p);
	free(t);
	freeBackground();
	freeOscillator();
	jack_client_close(client);

	common_cleanup(ret);
}

int main(int argc, char **argv)
{
	/* seed rand */
	srand(time(NULL));
	puts("\033[?1049h"); /* switch to the back buffer */
	puts("\033[?1002h"); /* enable mouse events */

	tcgetattr(1, &term);
	origterm = term;
	term.c_lflag &= (~ECHO & ~ICANON);
	tcsetattr(1, TCSANOW, &term); /* disable ECHO and ICANON */

	fcntl(0, F_SETFL, O_NONBLOCK); /* non-blocking reads */

	/* ignore sigint to prevent habitual <C-c> breaking stuff */
	signal(SIGINT, SIG_IGN);

	signal(SIGTERM, &cleanup);


	/* jack stuffs */
	p = malloc(sizeof(playbackinfo));
	if (!p)
	{
		puts("out of memory");
		common_cleanup(1);
	}
	memset(p, 0, sizeof(playbackinfo));
	client = jack_client_open("omutrack", JackNullOption, NULL);
	if (client == NULL)
	{
		puts("failed to init the jack client");
		free(p);
		common_cleanup(1);
	}

	p->in.l = jack_port_register(client, "in_l", JACK_DEFAULT_AUDIO_TYPE, JackPortIsInput, 0);
	p->in.r = jack_port_register(client, "in_r", JACK_DEFAULT_AUDIO_TYPE, JackPortIsInput, 0);
	p->out.l = jack_port_register(client, "out_l", JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0);
	p->out.r = jack_port_register(client, "out_r", JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0);

	samplerate = jack_get_sample_rate(client);
	rampmax = samplerate / 1000 * RAMP_MS;
	stretchrampmax = samplerate / 1000 * TIMESTRETCH_RAMP_MS;
	buffersize = jack_get_buffer_size(client);

	w = calloc(1, sizeof(window));
	if (w == NULL)
	{
		puts("out of memory");
		free(p);
		common_cleanup(1);
	}
	w->octave = 4;
	w->defpatternlength = 0x1f;

	s = addSong();
	if (s == NULL)
	{
		puts("out of memory");
		free(w);
		free(p);
		common_cleanup(1);
	}

	p->s = s;
	p->w = w;


	jack_set_process_callback(client, process, p);

	initBackground(); /* needs to be before jack_activate */
	genOscillator();

	jack_activate(client);

	addPattern(0, 0);
	s->songi[0] = 0;


#ifdef SAMPLES_DIR
	strcpy(w->dirpath, SAMPLES_DIR);
#else
	getcwd(w->dirpath, sizeof(w->dirpath));
#endif
	changeDirectory();


	t = calloc(1, sizeof(typetable));
	if (!t)
	{
		puts("out of memory");

		if (w->dir != NULL) closedir(w->dir);
		jack_deactivate(client);
		jack_client_close(client);

		free(w);
		delSong(s);
		free(p);
		freeBackground();
		freeOscillator();

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

		// if (p->dirty)
		{ p->dirty = 0; redraw(); }

		/* perform any pending instrument actions */
		changeInstrumentType(s, 0);

		/* finish freeing the record buffer */
		if (w->instrumentrecv == INST_REC_LOCK_CANCEL)
		{
			free(w->recbuffer); w->recbuffer = NULL;
			free(w->recchannelbuffer); w->recchannelbuffer = NULL;
			w->instrumentrecv = INST_REC_LOCK_OK;
			p->dirty = 1;
		} else if (w->instrumentrecv == INST_REC_LOCK_END)
		{
			if (w->recptr > 0)
			{
				instrument *iv = s->instrumentv[w->instrumentreci];
				sampler_state *ss = iv->state[0]; /* assume 0 is the sampler type, and that it is loaded TODO: bad idea? */
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
			free(w->recchannelbuffer); w->recchannelbuffer = NULL;
			w->instrumentrecv = INST_REC_LOCK_OK;
			p->dirty = 1;
		}

		req.tv_sec  = 0; /* nanosleep can set this higher sometimes, so set every cycle */
		req.tv_nsec = 16666666; // wait ~>16.7ms (60hz)
		while(nanosleep(&req, &req) < 0);
	}

	cleanup(0);
}
