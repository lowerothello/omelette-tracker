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
#include <assert.h>
#include <dirent.h>
#include <libgen.h>
#include <jack/jack.h>
#include <jack/midiport.h>
#include <jack/thread.h>
#include <sndfile.h>
#include <dlfcn.h>
#include <ladspa.h>
#include <lilv/lilv.h>
#include <lv2.h>
// #include <lv2/port-props/port-props.h>
#include <lv2/urid/urid.h>
#include <lv2/units/units.h>

/* libdrawille */
#include "../lib/libdrawille/src/Canvas.h"

#define MIN(X, Y) ((X)<(Y)?(X):(Y))
#define MAX(X, Y) ((X)>(Y)?(X):(Y))


/* version */
const unsigned char MAJOR = 1;
const unsigned char MINOR = 1;

#define LINENO_COLS 7

#define TRACK_ROW 3 /* rows above the track headers */
#define PROGRAM_TITLE "omelette tracker"

#define INSTRUMENT_INDEX_COLS 18

#define RECORD_LENGTH 600 /* max record length, in seconds */

int DEBUG;

jack_nframes_t samplerate, buffersize;
jack_nframes_t rampmax;

jack_client_t *client;

struct termios origterm;
struct winsize ws;


/* prototypes, TODO: proper header files would be less ugly */
void startPlayback(void);
void stopPlayback (void);
void showTracker   (void);
void showInstrument(void);
void showMaster    (void);

#include "config.h"

#ifndef NO_VALGRIND
#include <valgrind/valgrind.h>
#endif

#include "command.c"

#include "dsp.c"
#include "buttons.h"

#include "event.h"

#include "column.c"
#include "control.c"

#include "types/types.c"

#include "macros.h"

void setBpm(uint16_t *, uint8_t);
void midiNoteOff(jack_nframes_t, uint8_t, uint8_t, uint8_t);

#include "input.c"
#include "draw.c"
ControlState cc;
#include "tooltip.c"
TooltipState tt;

#include "types/file.c"

#include "browser.c"
#include "filebrowser.c"
#include "effect/pluginbrowser.c"

#include "event.c" /* event handlers */

#include "instrument/waveform.c"
#include "generator/sampler.c"

#include "background.c"
#include "process.c"
#include "macros.c"

#include "effect/input.c"
#include "effect/draw.c"

#include "master.c"

#include "instrument/instrument.c"
#include "instrument/input.c"
#include "instrument/draw.c"

#include "tracker/visual.c"
#include "tracker/tracker.c"
#include "tracker/draw.h"
#include "tracker/input.c"

#include "init.c"

void drawRuler(void)
{
	printf("\033[1m\033[0;%ldH%s\033[m", (ws.ws_col - strlen(PROGRAM_TITLE))>>1, PROGRAM_TITLE);
	printf("\033[1m\033[0;%dHv%d.%03d  %d\033[m", ws.ws_col - 15, MAJOR, MINOR, DEBUG);

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

		if (w->follow) printf(">");
		else           printf(" ");

		if (s->playing == PLAYING_STOP) printf("STOP");
		else                            printf("PLAY");

		if (w->follow) printf(">");
		else           printf(" ");

		printf(" &%d +%x  \033[1m%3dBPM\033[m", w->octave, w->step, s->songbpm);
	}
}
void redraw(void)
{
	fcntl(0, F_SETFL, 0); /* blocking */
	/* "CSI 2 J"     clears the screen              */
	/* "CSI ? 2 5 h"  ensures the cursor is visible  */
	/* "CSI 2   q" sets the cursor shape to block */
	puts("\033[2J\033[?25h\033[2 q");

	if (ws.ws_row < 14 + TRACK_ROW || ws.ws_col < 38 + INSTRUMENT_INDEX_COLS - 1)
	{
		printf("\033[%d;%dH%s", w->centre, (ws.ws_col - (unsigned short)strlen("(terminal too small)")) / 2, "(terminal too small)");
	} else
	{
#ifdef ENABLE_BACKGROUND
		drawBackground();
#endif
		drawRuler();
		switch (w->page)
		{
			case PAGE_TRACK_VARIANT: case PAGE_TRACK_EFFECT:      drawTracker();             break;
			case PAGE_INSTRUMENT_SAMPLE: case PAGE_INSTRUMENT_EFFECT: drawInstrument();          break;
			case PAGE_EFFECT_MASTER: case PAGE_EFFECT_SEND:           drawMaster();              break;
			case PAGE_PLUGINBROWSER:                                  drawPluginEffectBrowser(); break;
		}
		drawCommand(&w->command, w->mode);
	}

	drawTooltip(&tt);

	fflush(stdout);
	fcntl(0, F_SETFL, O_NONBLOCK); /* non-blocking */
}

void filebrowserEditCallback(char *path)
{
	if (path)
	{
		strcpy(w->newfilename, path);
		Event e;
		e.sem = M_SEM_RELOAD_REQ;
		e.callback = cb_reloadFile;
		pushEvent(&e);
	}
	w->page = PAGE_TRACK_VARIANT;
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
		if (strcmp(buffer, ""))
		{
			strcpy(w->newfilename, buffer);
			Event e;
			e.sem = M_SEM_RELOAD_REQ;
			e.callback = cb_reloadFile;
			pushEvent(&e);
		}
	}

	free(buffer); buffer = NULL;
	p->redraw = 1;
	return 0;
}

void startPlayback(void)
{
	if (s->loop[1]) s->playfy = s->loop[0];
	else            s->playfy = STATE_ROWS;
	s->sprp = 0;
	if (w->follow)
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
	} else
	{
		if (s->loop[1]) w->trackerfy = s->loop[0];
		else            w->trackerfy = STATE_ROWS;
	}
	w->mode = 0; /* always go to mode 0 on stop */
	p->redraw = 1;
}

void showTracker(void)
{
	switch (w->page)
	{
		case PAGE_TRACK_VARIANT:
			w->effectscroll = 0;
			w->mode = T_MODE_NORMAL;
			w->page = PAGE_TRACK_EFFECT;
			break;
		case PAGE_TRACK_EFFECT:
			w->page = PAGE_TRACK_VARIANT;
			break;
		default:
			w->page = PAGE_TRACK_VARIANT;
			w->mode = T_MODE_NORMAL;
			break;
	}
	freePreviewSample();
}
void showInstrument(void)
{
	w->showfilebrowser = 0;
	switch (w->page)
	{
		case PAGE_INSTRUMENT_SAMPLE:
			w->effectscroll = 0;
			w->page = PAGE_INSTRUMENT_EFFECT;
			break;
		case PAGE_INSTRUMENT_EFFECT:
			w->page = PAGE_INSTRUMENT_SAMPLE;
			break;
		default:
			w->page = PAGE_INSTRUMENT_SAMPLE;
			w->mode = I_MODE_NORMAL;
			break;
	}
	if (s->instrument->i[w->instrument] != INSTRUMENT_VOID)
		resetWaveform();
	freePreviewSample();
}
void showMaster(void)
{
	switch (w->page)
	{
		case PAGE_EFFECT_MASTER: w->page = PAGE_EFFECT_SEND; break;
		case PAGE_EFFECT_SEND: w->page = PAGE_EFFECT_MASTER; break;
		default:
			w->page = PAGE_EFFECT_MASTER;
			w->mode = 0;
			break;
	}
	freePreviewSample();
}

int input(void)
{
	int input;
	while (1)
	{
		input = getchar(); /* pop a byte from stdin */
		if (input < 0) break;
		DEBUG = input;

		if (w->mode == 255) /* command */
		{
			if (commandInput(&w->command, input, &w->mode, w->oldmode)) return 1;
			p->redraw = 1;
		} else switch (input)
			{
				case ':': /* enter command mode */
					if (w->count) { w->count = 0;  }
					if (w->chord) { w->chord = '\0'; clearTooltip(&tt); }
					setCommand(&w->command, &commandCallback, NULL, NULL, 1, ":", "");
					// setCommand(&w->command, &commandCallback, NULL, &commandTabCallback, 1, ":", "");
					w->oldmode = w->mode;
					if (w->page == PAGE_TRACK_VARIANT)
						switch (w->mode)
						{
							case T_MODE_VISUAL:
							case T_MODE_VISUALLINE:
							case T_MODE_VISUALREPLACE:
							case T_MODE_INSERT:
							case T_MODE_MOUSEADJUST:
								w->oldmode = T_MODE_NORMAL;
								break;
						}
					w->mode = 255;
					p->redraw = 1; break;
				case 7: /* ^G, show file info */
					if (strlen(w->filepath))
						sprintf(w->command.error, "\"%.*s\"", COMMAND_LENGTH - 2, w->filepath);
					else
						strcpy(w->command.error, "No file loaded");
					p->redraw = 1; break;
				default:
					switch (w->page)
					{
						case PAGE_TRACK_VARIANT: case PAGE_TRACK_EFFECT:      trackerInput(input);             break;
						case PAGE_INSTRUMENT_SAMPLE: case PAGE_INSTRUMENT_EFFECT: instrumentInput(input);          break;
						case PAGE_EFFECT_MASTER: case PAGE_EFFECT_SEND:           masterInput(input);              break;
						case PAGE_PLUGINBROWSER:                                  pluginEffectBrowserInput(input); break;
					} break;
			}
	} return 0;
}

void resize(int _)
{
	ioctl(1, TIOCGWINSZ, &ws);
	w->centre = (ws.ws_row>>1) + 1;

	resizeWaveform();
	resizeBackground(b);
	resizeBrowser(fbstate,
			INSTRUMENT_INDEX_COLS + 2,         /* x */
			TRACK_ROW + 1,                   /* y */
			ws.ws_col - INSTRUMENT_INDEX_COLS, /* w */
			ws.ws_row - TRACK_ROW - 1);      /* h */
	resizeBrowser(pbstate,
			1,                            /* x */
			TRACK_ROW + 1,              /* y */
			ws.ws_col,                    /* w */
			ws.ws_row - TRACK_ROW - 1); /* h */

	p->redraw = 1;
}

int main(int argc, char **argv)
{
	if (argc > 1 && (!strcmp(argv[1], "-v") || !strcmp(argv[1], "--version")))
	{
		printf("omelette tracker, v%d.%03d\n", MAJOR, MINOR);
		return 0;
	}

	init(argc, argv); /* will exit if it fails */

	/* loop over input */
	struct timespec req;
	int running = 0;
	while (!running)
	{
		if (!mainM_SEM())
		{
			if (p->resize) { p->resize = 0; resize(0); }
#ifdef ENABLE_BACKGROUND
			/* imply p->redraw if background is on */
			redraw();
#else
			if (p->redraw) { p->redraw = 0; redraw(); }
#endif

			/* finish freeing the record buffer */
			if (w->instrumentrecv == INST_REC_LOCK_CANCEL)
			{
				free(w->recbuffer); w->recbuffer = NULL;
				w->instrumentrecv = INST_REC_LOCK_OK;
				p->redraw = 1;
			} else if (w->instrumentrecv == INST_REC_LOCK_END)
			{
				if (w->recptr > 0)
				{
					Sample *sample = malloc(sizeof(Sample) + (w->recptr<<1)*sizeof(short));
					sample->length = w->recptr;
					sample->tracks = 2;
					sample->rate = sample->defrate = samplerate;
					memcpy(&sample->data, w->recbuffer, (w->recptr<<1)*sizeof(short));
					free(w->recbuffer); w->recbuffer = NULL;
					reparentSample(&s->instrument->v[s->instrument->i[w->instrumentreci]], sample);
					w->recptr = 0; resetWaveform();
				} else { free(w->recbuffer); w->recbuffer = NULL; }

				w->instrumentrecv = INST_REC_LOCK_OK;
				p->redraw = 1;
			}
			running = input(); /* ensure that semaphores are handled between input and draw */
		}

		req.tv_sec  = 0; /* nanosleep can set this higher sometimes, so set every cycle */
		req.tv_nsec = UPDATE_DELAY;
		while(nanosleep(&req, &req) < 0);
	} cleanup(0);
}
