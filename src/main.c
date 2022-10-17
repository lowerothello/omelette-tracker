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

/* libdrawille */
#include "lib/libdrawille/src/Canvas.h"

#define MIN(X, Y) ((X)<(Y)?(X):(Y))
#define MAX(X, Y) ((X)>(Y)?(X):(Y))


/* version */
const unsigned char MAJOR = 0;
const unsigned char MINOR = 101;

#define LINENO_COLS 7

#define CHANNEL_ROW 5 /* rows above the channel headers */

#define INSTRUMENT_INDEX_COLS 18

#define RECORD_LENGTH 600 /* record length, in seconds */

int DEBUG;

jack_nframes_t samplerate, buffersize;
jack_nframes_t rampmax;

jack_client_t *client;
pthread_t dummyprocessthread;

struct termios origterm;
struct winsize ws;


/* prototypes, TODO: proper header files would be less ugly */
void resize(int); /* TODO: remove hooks to this, should only be hooked asynchronously */
void startPlayback(void);
void stopPlayback(void);
void showTracker(void);
void showInstrument(void);
void showMaster(void);
void changeMacro(int, char *);

#include "config.h"

#ifndef NO_VALGRIND
#include <valgrind/valgrind.h>
#endif

#include "command.c"
#include "dsp.c"

#include "signal.h" /* signal declares */

#include "buttons.h"
#include "control.c"
#include "types/types.c"

char ifMacro(jack_nframes_t, uint16_t *, Channel *, Row, char, char (*)(jack_nframes_t, uint16_t *, int, Channel *, Row));
void setBpm(uint16_t *, uint8_t);
void midiNoteOff(jack_nframes_t, uint8_t, uint8_t, uint8_t);

#include "input.c"
ControlState cc;
#include "tooltip.c"
TooltipState tt;
#include "column.c"

#include "types/effect.c"
#include "effect/pluginbrowser.c"
#include "types/channel.c"
#include "types/instrument.c"
#include "types/song.c"
#include "types/file.c"

#include "signal.c" /* signal handlers */

#include "background.c"
#include "generator/sampler.c"
#include "macros.h"
#include "process.c"
#include "macros.c"

// #include "master.c"
#include "filebrowser.c"

#include "effect/effectinput.c"
#include "effect/effectdraw.c"

#include "instrument/instrument.c"
#include "instrument/input.c"
#include "instrument/draw.c"

#include "tracker/visual.c"
#include "tracker/tracker.c"
#include "tracker/draw.c"
#include "tracker/input.c"

#include "init.c"


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
	puts("\033[2J\033[?25h");

	if (ws.ws_row < 14 + CHANNEL_ROW || ws.ws_col < 38 + INSTRUMENT_INDEX_COLS - 1)
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
			case PAGE_CHANNEL_VARIANT:
			case PAGE_CHANNEL_EFFECT:
				drawTracker(); break;
			case PAGE_INSTRUMENT_SAMPLE:
			case PAGE_INSTRUMENT_EFFECT:
				drawInstrument(); break;
			/* case PAGE_MASTER:
				drawMaster(); break; */
			case PAGE_FILEBROWSER:
				drawFilebrowser(); break;
			case PAGE_CHANNEL_EFFECT_PLUGINBROWSER:
			case PAGE_INSTRUMENT_EFFECT_PLUGINBROWSER:
				drawPluginEffectBrowser();
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
	w->page = PAGE_CHANNEL_VARIANT;
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
			w->page = PAGE_FILEBROWSER;
			w->filebrowserCallback = &filebrowserEditCallback;
		} else
		{
			strcpy(w->newfilename, buffer);
			Event e;
			e.sem = M_SEM_RELOAD_REQ;
			e.callback = cb_reloadFile;
			pushEvent(&e);
		}
	}

	free(buffer); buffer = NULL;
	p->dirty = 1;
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
	} else w->trackerfy = STATE_ROWS;
	p->dirty = 1;
}

void showTracker(void)
{
	switch (w->page)
	{
		case PAGE_CHANNEL_VARIANT:
			w->effectscroll = 0;
			w->mode = T_MODE_NORMAL;
			w->page = PAGE_CHANNEL_EFFECT;
			break;
		case PAGE_CHANNEL_EFFECT:
			w->page = PAGE_CHANNEL_VARIANT;
			break;
		default:
			w->page = PAGE_CHANNEL_VARIANT;
			w->mode = T_MODE_NORMAL;
			break;
	}
	freePreviewSample();
}
void showInstrument(void)
{
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
			w->mode = I_MODE_INDICES;
			break;
	}
	if (s->instrument->i[w->instrument] != INSTRUMENT_VOID)
		resetWaveform();
	freePreviewSample();
}
void showMaster(void)
{
	w->mode = 0;
	w->page = PAGE_MASTER;
	freePreviewSample();
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
			p->dirty = 1;
		} else switch (input)
			{
				case ':': /* enter command mode */
					if (w->count) { w->count = 0;  }
					if (w->chord) { w->chord = '\0'; clearTooltip(&tt); }
					setCommand(&w->command, &commandCallback, NULL, NULL, 1, ":", "");
					// setCommand(&w->command, &commandCallback, NULL, &commandTabCallback, 1, ":", "");
					w->oldmode = w->mode;
					if (w->page == PAGE_CHANNEL_VARIANT)
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
					p->dirty = 1; break;
				case 7: /* ^G, show file info */
					if (strlen(w->filepath))
						sprintf(w->command.error, "\"%.*s\"", COMMAND_LENGTH - 2, w->filepath);
					else
						strcpy(w->command.error, "No file loaded");
					p->dirty = 1; break;
				default:
					switch (w->page)
					{
						case PAGE_CHANNEL_VARIANT:
						case PAGE_CHANNEL_EFFECT:
							trackerInput(input); break;
						case PAGE_INSTRUMENT_SAMPLE:
						case PAGE_INSTRUMENT_EFFECT:
							instrumentInput(input); break;
						/* case PAGE_MASTER:
							masterInput(input); break; */
						case PAGE_FILEBROWSER:
							filebrowserInput(input); break;
						case PAGE_CHANNEL_EFFECT_PLUGINBROWSER:
						case PAGE_INSTRUMENT_EFFECT_PLUGINBROWSER:
							pluginEffectBrowserInput(input); break;
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
	w->centre = (ws.ws_row>>1) + 1;

	w->waveformw = (ws.ws_col - INSTRUMENT_INDEX_COLS +1)<<1;
	// if (ws.ws_col - INSTRUMENT_INDEX_COLS < 57)
		w->waveformh = (ws.ws_row - CHANNEL_ROW - 12)<<2;
	/* else
		w->waveformh = (ws.ws_row - CHANNEL_ROW - 6)<<2; */

	if (w->waveformcanvas) { free_canvas(w->waveformcanvas); w->waveformcanvas = NULL; }
	if (w->waveformbuffer) { free_buffer(w->waveformbuffer); w->waveformbuffer = NULL; }

	if (w->waveformw > 0 && w->waveformh > 0)
		w->waveformcanvas = new_canvas(w->waveformw, w->waveformh);

	if (w->waveformcanvas)
		w->waveformbuffer = new_buffer(w->waveformcanvas);

	w->waveformdrawpointer = 0;

	resizeBackground(b);
	changeDirectory(); /* recalc the maxwidth/cols */

	signal(SIGWINCH, &resize);
	p->dirty = 1;
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
		if (!mainM_SEM())
		{
			running = input();

			/* imply p->dirty if background is on */
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
					w->recbuffer = realloc(w->recbuffer, (w->recptr<<1) * sizeof(short));
					reparentSample(&s->instrument->v[s->instrument->i[w->instrumentreci]], w->recbuffer, w->recptr, samplerate);
					w->recbuffer = 0; resetWaveform();
				} else { free(w->recbuffer); w->recbuffer = NULL; }

				w->instrumentrecv = INST_REC_LOCK_OK;
				p->dirty = 1;
			}
		}

		req.tv_sec  = 0; /* nanosleep can set this higher sometimes, so set every cycle */
		req.tv_nsec = UPDATE_DELAY;
		while(nanosleep(&req, &req) < 0);
	} cleanup(0);
}
