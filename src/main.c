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
#include <jack/midiport.h>
typedef jack_default_audio_sample_t sample_t;
#include <sndfile.h>

/* libdrawille */
#include "lib/libdrawille/src/Canvas.h"

#define MIN(X, Y)  ((X) < (Y) ? (X) : (Y))
#define MAX(X, Y)  ((X) > (Y) ? (X) : (Y))

uint32_t pow32(uint32_t a, uint32_t b)
{
	if (!b) return 1;

	uint32_t c = a;
	for (uint32_t i = 1; i < b; i++)
		c = c * a;

	return c;
}


/* version */
const unsigned char MAJOR = 0;
const unsigned char MINOR = 94;


jack_nframes_t samplerate, buffersize;
jack_nframes_t rampmax, stretchrampmax;
jack_client_t *client;
struct winsize ws;
struct termios term, origterm;

#define LINENO_COLS 5
#define SONGLIST_COLS 5

#define CHANNEL_ROW 4 /* rows above the channel headers */

#define INSTRUMENT_INDEX_COLS 20
#define INSTRUMENT_CONTROL_ROW 4
#define INSTRUMENT_CONTROL_COLS 57

#define RECORD_LENGTH 600 /* record length, in seconds */

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
void showTracker(void);
void showInstrument(void);

void changeMacro(int, char *);

#include "config.h"

#include "command.c"
#include "dsp.c"
#include "types.c"

#include "input.c"
#include "sampler.c"
#include "instrument.c"
#include "filebrowser.c"
#include "waveform.c"
#include "tracker.c"
#include "background.c"
#include "process.c"


void drawRuler(void)
{
	/* top ruler */
	printf("\033[0;0H\033[2K\033[1momelette tracker\033[0;%dHv%d.%2d  %d\033[m",
			ws.ws_col - 14, MAJOR, MINOR, DEBUG);
	/* bottom ruler */
	if (w->mode < 255)
	{
		switch (w->instrumentrecv)
		{
			case INST_REC_LOCK_CONT: case INST_REC_LOCK_START:
				if (w->recptr == 0) printf("\033[%d;%dH\033[3m{REC %02x   0s}\033[m", ws.ws_row, ws.ws_col - 50, w->instrumentreci);
				else                printf("\033[%d;%dH\033[3m{REC %02x %3ds}\033[m", ws.ws_row, ws.ws_col - 50, w->instrumentreci, w->recptr / samplerate + 1);
				break;
			case INST_REC_LOCK_CUE_CONT: case INST_REC_LOCK_CUE_START:
				if (w->recptr == 0) printf("\033[%d;%dH\033[3m[cue]{REC %02x   0s}\033[m", ws.ws_row, ws.ws_col - 55, w->instrumentreci);
				else                printf("\033[%d;%dH\033[3m[cue]{REC %02x %3ds}\033[m", ws.ws_row, ws.ws_col - 55, w->instrumentreci, w->recptr / samplerate + 1);
				break;
		}

		if (w->count) printf("\033[%d;%dH%3d", ws.ws_row, ws.ws_col - 29, w->count);
		if (w->chord) printf("\033[%d;%dH%c", ws.ws_row, ws.ws_col - 26, w->chord);

		printf("\033[%d;%dH", ws.ws_row, ws.ws_col - 24);
		if (w->flags&W_FLAG_FOLLOW) printf(">"); else printf(" ");
		if (s->playing == PLAYING_STOP) printf("STOP");
		else                            printf("PLAY");
		if (w->flags&W_FLAG_FOLLOW) printf(">"); else printf(" ");
		printf(" &%d +%x  ", w->octave, w->step);
		if (w->keyboardmacro) printf("%cxx  ", w->keyboardmacro);
		else                  printf("     ");
		printf("\033[1m%3dBPM\033[m", s->songbpm);
	}
}
void redraw(void)
{
	fcntl(0, F_SETFL, 0); /* blocking */
	puts("\033[2J\033[?25h");

	if (ws.ws_row < 20 || ws.ws_col < INSTRUMENT_CONTROL_COLS + INSTRUMENT_INDEX_COLS - 1)
	{
		printf("\033[%d;%dH%s", w->centre, (ws.ws_col - (unsigned short)strlen("(terminal too small)")) / 2, "(terminal too small)");
		fflush(stdout);
		fcntl(0, F_SETFL, O_NONBLOCK);
		return;
	}

	if (ENABLE_BACKGROUND) drawBackground();

	drawRuler();
	switch (w->popup)
	{
		case 0: drawTracker(); break;
		case 1: drawInstrument(); break;
		case 2: drawFilebrowser(); break;
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
/* void commandTabCallback(char *text)
{
	char *buffer = malloc(strlen(text) + 1);
	wordSplit(buffer, text, 0);
	// if      (!strcmp(buffer, "bpm")) snprintf(text, COMMAND_LENGTH + 1, "bpm %d", s->bpm);
	free(buffer);
} */
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
		if (w->flags&W_FLAG_FOLLOW)
			w->trackerfy = 0;
		s->playing = PLAYING_START;
	} else strcpy(w->command.error, "failed to start playback, invalid pattern selected");
	redraw();
}
void stopPlayback(void)
{
	if (s->playing)
	{
		if (w->instrumentrecv == INST_REC_LOCK_CONT || w->instrumentrecv == INST_REC_LOCK_CUE_CONT)
			w->instrumentrecv = INST_REC_LOCK_PREP_END;
		s->playing = PLAYING_PREP_STOP;
	} else w->trackerfy = 0;
	redraw();
}

void showTracker(void)
{
	w->mode = 0;
	w->popup = 0;
}
void showInstrument(void)
{
	w->instrumentindex = 0;
	w->mode = 0;
	w->popup = 1;
	if (s->instrumenti[w->instrument])
		resetWaveform();
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
			if (commandInput(&w->command, input, &w->mode, w->oldmode)) return 1;
			redraw();
		} else switch (input)
			{
				case ':': /* enter command mode */
					setCommand(&w->command, &commandCallback, NULL, NULL, 1, ":", "");
					// setCommand(&w->command, &commandCallback, NULL, &commandTabCallback, 1, ":", "");
					w->oldmode = w->mode;
					if (w->popup == 0)
						switch (w->mode)
						{
							case T_MODE_VISUAL:
							case T_MODE_VISUALLINE:
							case T_MODE_VISUALREPLACE:
							case T_MODE_INSERT:
							case T_MODE_MOUSEADJUST:
								w->oldmode = T_MODE_NORMAL;
								break;
							case T_MODE_SONG_INSERT:
							case T_MODE_SONG_VISUAL:
							case T_MODE_SONG_MOUSEADJUST:
								w->oldmode = T_MODE_SONG;
								break;
						}
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
					}
					break;
			}
	}
	return 0;
}

/* window->dirpath should be set to the path */
int changeDirectory(void)
{
	if (w->dir != NULL) { closedir(w->dir); w->dir = NULL; }

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
			if (strlen(dirent->d_name)+2 > w->dirmaxwidth)
				w->dirmaxwidth = strlen(dirent->d_name)+2;
			w->dirc++;
		}
		dirent = readdir(w->dir);
	}
	rewinddir(w->dir);
	w->dirmaxwidth = MIN(w->dirmaxwidth, ws.ws_col - 4);
	w->dircols = MAX(MIN((ws.ws_col - 8) / w->dirmaxwidth, (w->dirc - 1) / 4), 1);
	return 0;
}

void resize(int _)
{
	signal(SIGWINCH, SIG_IGN); /* ignore the signal until it's finished */
	ioctl(1, TIOCGWINSZ, &ws);
	w->instrumentcelloffset = (ws.ws_col - INSTRUMENT_BODY_COLS) / 2 + 1;
	w->instrumentrowoffset =  (ws.ws_row - INSTRUMENT_BODY_ROWS) / 2 + 1;
	w->centre =                ws.ws_row / 2 + 1;

	w->waveformw = (ws.ws_col - INSTRUMENT_INDEX_COLS +1) * 2;
	w->waveformh = (ws.ws_row - CHANNEL_ROW - INSTRUMENT_CONTROL_ROW -1) * 4;
	if (w->waveformcanvas) { free_canvas(w->waveformcanvas); w->waveformcanvas = NULL; } w->waveformcanvas = new_canvas(w->waveformw, w->waveformh);
	if (w->waveformbuffer) free_buffer(w->waveformbuffer);
	w->waveformbuffer = NULL;
	w->waveformbuffer = new_buffer(w->waveformcanvas);
	if (w->popup == 1 && s->instrumenti[w->instrument])
		w->waveformdrawpointer = 0;

	resizeBackground();
	changeDirectory(); /* recalc the maxwidth/cols */

	redraw();
	signal(SIGWINCH, &resize);
}

void common_cleanup(int ret)
{
	fcntl(0, F_SETFL, 0); /* reset to blocking stdin reads */
	tcsetattr(1, TCSANOW, &origterm); /* reset to the original termios */
	puts("\033[?1002l"); /* disable mouse */
	puts("\033[?1049l"); /* reset to the front buffer */

	exit(ret);
}
void cleanup(int ret)
{
	struct timespec req;

	if (w->dir) closedir(w->dir);
	stopPlayback();
	while (s->playing)
	{ /* wait until stopPlayback() finishes fully */
		req.tv_sec = 0;
		req.tv_nsec = UPDATE_DELAY;
		nanosleep(&req, &req);
	}

	jack_deactivate(client);
	jack_client_close(client);

	_delInstrument(&w->instrumentbuffer);

	if (w->recbuffer) free(w->recbuffer);
	if (w->waveformcanvas) free_canvas(w->waveformcanvas);
	if (w->waveformbuffer) free_buffer(w->waveformbuffer);


	free(w);
	delSong(s);
	free(p);
	freeBackground();

	common_cleanup(ret);
}

int main(int argc, char **argv)
{
	if (argc > 1 && (!strcmp(argv[1], "-v") || !strcmp(argv[1], "--version")))
	{
		printf("omelette tracker, v%d.%2d\n", MAJOR, MINOR);
		return 0;
	}

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
	} memset(p, 0, sizeof(playbackinfo));

	client = jack_client_open("omelette", JackNullOption, NULL);
	if (!client)
	{
		puts("failed to init the jack client");
		free(p);
		common_cleanup(1);
	}

	p->in.l = jack_port_register(client, "in_l", JACK_DEFAULT_AUDIO_TYPE, JackPortIsInput|JackPortIsTerminal, 0);
	p->in.r = jack_port_register(client, "in_r", JACK_DEFAULT_AUDIO_TYPE, JackPortIsInput|JackPortIsTerminal, 0);
	char *buffer = malloc(9);
	for (short i = 0; i < OUTPUT_GROUPS; i++)
	{
		snprintf(buffer, 9, "out_%d_l", i); p->out[i].l = jack_port_register(client, buffer, JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput|JackPortIsTerminal, 0);
		snprintf(buffer, 9, "out_%d_r", i); p->out[i].r = jack_port_register(client, buffer, JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput|JackPortIsTerminal, 0);
	} free(buffer);
	p->midiout = jack_port_register(client, "out_midi", JACK_DEFAULT_MIDI_TYPE, JackPortIsOutput|JackPortIsTerminal, 0);

	samplerate = jack_get_sample_rate(client);
	buffersize = jack_get_buffer_size(client);
	rampmax = samplerate / 1000 * RAMP_MS;
	stretchrampmax = samplerate / 1000 * TIMESTRETCH_RAMP_MS;

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

	initBackground(); /* needs to be before jack_activate */

	w->previewchannel.r.note = w->previewchannel.samplernote = NOTE_VOID;
	w->previewchannel.r.inst = w->previewchannel.samplerinst = INST_VOID;
	w->previewchannel.filtercut = 1.0f;


	jack_set_process_callback(client, process, p);
	jack_activate(client);

	addPattern(0);
	s->songi[0] = 0;


#ifdef SAMPLES_DIR
	strcpy(w->dirpath, SAMPLES_DIR);
#else
	getcwd(w->dirpath, sizeof(w->dirpath));
#endif
	changeDirectory();

	if (argc > 1)
	{
		strcpy(w->newfilename, argv[1]);
		p->lock = PLAY_LOCK_START;
	}

	resize(0);

	/* loop over input */
	struct timespec req;
	int running = 0;
	while (!running)
	{
		if (p->lock == PLAY_LOCK_CONT)
		{
			song *cs = readSong(w->newfilename);
			if (cs) { delSong(s); s = cs; }
			p->s = s;
			p->dirty = 1;
			p->lock = PLAY_LOCK_OK;
		}

		running = input();

		if (ENABLE_BACKGROUND || p->dirty)
		{ p->dirty = 0; redraw(); }

		/* perform any pending instrument actions */
		asyncInstrumentUpdate(s);

		/* finish freeing the record buffer */
		if (w->instrumentrecv == INST_REC_LOCK_CANCEL)
		{
			free(w->recbuffer); w->recbuffer = NULL;
			w->instrumentrecv = INST_REC_LOCK_OK;
			p->dirty = 1;
		} else if (w->instrumentrecv == INST_REC_LOCK_END)
		{
			if (w->recptr > 0)
			{
				instrument *iv = s->instrumentv[s->instrumenti[w->instrumentreci]];
				if (iv->sampledata)
				{ free(iv->sampledata); iv->sampledata = NULL; }
				iv->sampledata = malloc(w->recptr * 2 * sizeof(short)); /* *2 for stereo */
				if (!iv->sampledata) strcpy(w->command.error, "saving recording failed, out of memory");
				else
				{
					memcpy(iv->sampledata, w->recbuffer, w->recptr * 2 * sizeof(short));
					iv->samplelength = w->recptr * 2;
					iv->channels = 2;
					iv->length = w->recptr;
					iv->c5rate = samplerate;
					iv->trim[0] = 0;
					iv->trim[1] = w->recptr-1;
					iv->loop = w->recptr-1;
				}
				resetWaveform();
			}

			free(w->recbuffer); w->recbuffer = NULL;
			w->instrumentrecv = INST_REC_LOCK_OK;
			p->dirty = 1;
		}

		req.tv_sec  = 0; /* nanosleep can set this higher sometimes, so set every cycle */
		req.tv_nsec = UPDATE_DELAY;
		while(nanosleep(&req, &req) < 0);
	}

	cleanup(0);
}
