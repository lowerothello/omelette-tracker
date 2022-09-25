// #define _POSIX_C_SOURCE 199309L
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <time.h>
#include <limits.h>
#include <stdbool.h>
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

#define MIN(X, Y) ((X)<(Y)?(X):(Y))
#define MAX(X, Y) ((X)>(Y)?(X):(Y))


/* version */
const unsigned char MAJOR = 0;
const unsigned char MINOR = 95;

#define LINENO_COLS 7

#define CHANNEL_ROW 4 /* rows above the channel headers */

#define INSTRUMENT_INDEX_COLS 20
#define INSTRUMENT_CONTROL_ROW 4
#define INSTRUMENT_CONTROL_COLS 57

#define RECORD_LENGTH 600 /* record length, in seconds */

int DEBUG;

jack_nframes_t samplerate, buffersize;
jack_nframes_t rampmax, stretchrampmax;
jack_client_t *client;

struct termios origterm;
struct winsize ws;


/* prototypes */
void redraw(void);
void resize(int);
void startPlayback(void);
void stopPlayback(void);
void showTracker(void);
void showInstrument(void);
void showMaster(void);

void changeMacro(int, char *);

#include "config.h"

#include "command.c"
#include "dsp.c"

#include "types/types.c"
#include "types/channel.c"
#include "types/instrument.c"
#include "types/song.c"
#include "types/file.c"

#include "background.c"
#include "sampler.c"
#include "macros.h"
#include "process.c"
#include "macros.c"

#include "input.c"

#include "control.c"
#include "types/effect.c"
ControlState cc;
#include "tooltip.c"
TooltipState tt;

#include "master.c"
#include "filebrowser.c"
#include "waveform.c"

#include "instrument/instrument.c"
#include "instrument/input.c"
#include "instrument/draw.c"

#include "tracker/visual.c"
#include "tracker/tracker.c"
#include "tracker/input.c"
#include "tracker/draw.c"

#include "init.c"


void drawRuler(void)
{
	/* top ruler */
	printf("\033[0;0H\033[2K\033[1momelette tracker\033[0;%dHv%d.%2d  %d\033[m",
			ws.ws_col - 14, MAJOR, MINOR, DEBUG);
	DEBUG = 0;

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

		printf("\033[%d;%dH", ws.ws_row, ws.ws_col - 19);
		if (w->flags&W_FLAG_FOLLOW) printf(">"); else printf(" ");
		if (s->playing == PLAYING_STOP) printf("STOP");
		else                            printf("PLAY");
		if (w->flags&W_FLAG_FOLLOW) printf(">"); else printf(" ");
		printf(" &%d +%x  \033[1m%3dBPM\033[m", w->octave, w->step, s->songbpm);
	}
}
void redraw(void)
{
	fcntl(0, F_SETFL, 0); /* blocking */
	puts("\033[2J\033[?25h");

	if (ws.ws_row < 20 || ws.ws_col < INSTRUMENT_CONTROL_COLS + INSTRUMENT_INDEX_COLS - 1)
	{
		printf("\033[%d;%dH%s", w->centre, (ws.ws_col - (unsigned short)strlen("(terminal too small)")) / 2, "(terminal too small)");
	} else
	{
#ifdef ENABLE_BACKGROUND
		drawBackground();
#endif

		drawRuler();
		switch (w->popup)
		{
			case 0: drawTracker(); break;
			case 1: drawInstrument(); break;
			case 3: drawMaster(); break;
			case 15: drawFilebrowser(); break;
		}
		drawCommand(&w->command, w->mode);
	}

	drawTooltip(&tt);

	fflush(stdout);
	fcntl(0, F_SETFL, O_NONBLOCK); /* non-blocking */
}

void filebrowserEditCallback(char *path)
{
	strcpy(w->newfilename, path);
	p->lock = PLAY_LOCK_START;
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
			w->popup = 15;
			w->filebrowserCallback = &filebrowserEditCallback;
			redraw();
		} else
		{
			strcpy(w->newfilename, buffer);
			p->lock = PLAY_LOCK_START;
		}
	}

	free(buffer); buffer = NULL;
	redraw();
	return 0;
}

void startPlayback(void)
{
	s->playfy = s->loop[0];
	s->sprp = 0;
	if (w->flags&W_FLAG_FOLLOW)
		w->trackerfy = s->playfy;
	s->playing = PLAYING_START;
}
void stopPlayback(void)
{
	if (s->playing)
	{
		if (w->instrumentrecv == INST_REC_LOCK_CONT || w->instrumentrecv == INST_REC_LOCK_CUE_CONT)
			w->instrumentrecv = INST_REC_LOCK_PREP_END;
		s->playing = PLAYING_PREP_STOP;
	} else w->trackerfy = STATE_ROWS;
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
	if (s->instrumenti[w->instrument] != INSTRUMENT_VOID)
		resetWaveform();
}
void showMaster(void)
{
	w->mode = 0;
	w->popup = 3;
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
					if (w->count) { w->count = 0;  }
					if (w->chord) { w->chord = '\0'; clearTooltip(&tt); }
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
							case T_MODE_VTRIG_INSERT:
							case T_MODE_VTRIG_VISUAL:
							case T_MODE_VTRIG_MOUSEADJUST:
								w->oldmode = T_MODE_VTRIG;
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
						case 3: masterInput(input); break;
						case 15: filebrowserInput(input); break;
					}
					break;
			}
	}
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
	w->waveformdrawpointer = 0;

	resizeBackground(b);
	changeDirectory(); /* recalc the maxwidth/cols */

	redraw();
	signal(SIGWINCH, &resize);
}

int main(int argc, char **argv)
{
	if (argc > 1 && (!strcmp(argv[1], "-v") || !strcmp(argv[1], "--version")))
	{
		printf("omelette tracker, v%d.%2d\n", MAJOR, MINOR);
		return 0;
	}

	init(argc, argv); /* will exit if it fails */

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
			w->trackerfy = STATE_ROWS;
			p->dirty = 1;
			p->lock = PLAY_LOCK_OK;
		}

		running = input();

#ifdef ENABLE_BACKGROUND
		redraw();
#else
		if (p->dirty) { p->dirty = 0; redraw(); }
#endif

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
				instrument *iv = &s->instrumentv[s->instrumenti[w->instrumentreci]];
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
	} cleanup(0);
}
