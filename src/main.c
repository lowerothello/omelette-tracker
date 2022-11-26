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
#include <ctype.h>
#include <jack/jack.h>     /* audio i/o        */
#include <jack/midiport.h> /* midi i/o         */
#include <jack/thread.h>   /* threading helper */
#include <sndfile.h> /* audio file read/write */
#include <dlfcn.h>
#include <ladspa.h> /* LADSPA audio plugins */
#include <lilv/lilv.h> /* LV2 audio plugins */
#include <lv2.h>       /* LV2 audio plugins */
#include <lv2/urid/urid.h>   /* LV2 urids */
#include <lv2/units/units.h> /* LV2 units */
#include <X11/Xlib.h> /* X11 hack for key release events */
#include <X11/keysym.h>

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


/* prototypes, TODO: a proper header file */
void startPlayback(void *_);
void stopPlayback (void *_);
void showTracker   (void *_);
void showInstrument(void *_);
void showMaster    (void *);
void resetInput(void);
void cleanup(int);

#include "config.h"

#ifndef NO_VALGRIND
#include <valgrind/valgrind.h>
#endif


#include "dsp.c"
#include "buttons.h"

#include "event.h"

#include "column.c"

#include "control.c"

#include "types/types.c"
#include "file/file.c"


#include "macros.h"

void setBpm(uint16_t *, uint8_t);
void midiNoteOff(jack_nframes_t, uint8_t, uint8_t, uint8_t);

#include "draw.c"
ControlState cc;

#include "input.c"

#include "browser.c"
#include "filebrowser.c"
#include "effect/pluginbrowser.c"

#include "event.c" /* event handlers */

#include "instrument/waveform.c"
#include "generator/sampler.c"

#include "process.c"
#include "macros.c"

#include "effect/input.c"
#include "effect/draw.c"

#include "master.c"

void trackerDownArrow(size_t count); /* ugly prototype */
void trackerHome(void); /* ugly prototype */
#include "tracker/visual.c"
#include "tracker/tracker.c"
#include "tracker/draw.h"
#include "tracker/input.c"

#include "instrument/instrument.c"
#include "instrument/input.c"
#include "instrument/draw.c"

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
		drawRuler();
		switch (w->page)
		{
			case PAGE_TRACK_VARIANT: case PAGE_TRACK_EFFECT: drawTracker();             break;
			case PAGE_INSTRUMENT:                            drawInstrument();          break;
			case PAGE_EFFECT_MASTER: case PAGE_EFFECT_SEND:  drawMaster();              break;
			case PAGE_PLUGINBROWSER:                         drawPluginEffectBrowser(); break;
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
void leaveSpecialModes(void)
{
	switch (w->page)
	{
		case PAGE_TRACK_VARIANT: case PAGE_TRACK_EFFECT:
			switch (w->mode)
			{
				case T_MODE_INSERT: break;
				default: w->mode = T_MODE_NORMAL; break;
			} break;
		default: break;
	}
}
void startPlayback(void *_)
{
	leaveSpecialModes();
	if (s->loop[1]) s->playfy = s->loop[0];
	else            s->playfy = STATE_ROWS;
	s->sprp = 0;
	if (w->follow)
		w->trackerfy = s->playfy;
	s->playing = PLAYING_START;
	p->redraw = 1;
}
void stopPlayback(void *_)
{
	leaveSpecialModes();
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

void showTracker(void *_)
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
	p->redraw = 1;
}
void showInstrument(void *_)
{
	w->showfilebrowser = 0;
	w->page = PAGE_INSTRUMENT;
	w->mode = I_MODE_NORMAL;

	if (s->instrument->i[w->instrument] != INSTRUMENT_VOID)
		resetWaveform();
	freePreviewSample();
	p->redraw = 1;
}
void showMaster(void *_)
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
	p->redraw = 1;
}


static bool commandCallback(char *command, enum _Mode *mode)
{
	char *buffer = malloc(strlen(command) + 1);
	wordSplit(buffer, command, 0);
	if      (!strcmp(buffer, "q"))   { free(buffer); buffer = NULL; return 1; }
	else if (!strcmp(buffer, "q!"))  { free(buffer); buffer = NULL; return 1; }
	else if (!strcmp(buffer, "w"))   { wordSplit(buffer, command, 1); writeSong(s, buffer); }
	else if (!strcmp(buffer, "wq"))
	{
		wordSplit(buffer, command, 1);
		if (!writeSong(s, buffer)) { free(buffer); buffer = NULL; return 1; } /* exit if writing the file succeeded */
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
static void enterCommandMode(void *arg)
{
	setCommand(&w->command, &commandCallback, NULL, NULL, 1, ":", "");
	w->oldmode = w->mode;
	switch (w->mode)
	{
		case T_MODE_VISUAL:
		case T_MODE_VISUALLINE:
		case T_MODE_VISUALREPLACE:
		case T_MODE_INSERT:
		case T_MODE_MOUSEADJUST:
			w->oldmode = T_MODE_NORMAL;
			break;
		default: break;
	}
	w->mode = MODE_COMMAND;
	p->redraw = 1;
}

static void showFileInfo(void *_)
{
	if (strlen(w->filepath))
		sprintf(w->command.error, "\"%.*s\"", COMMAND_LENGTH - 2, w->filepath);
	else
		strcpy(w->command.error, "No file loaded");
	p->redraw = 1;
}
void resetInput(void)
{
	w->count = 0;
	w->chord = 0;
	clearTooltip(&tt);
	addTooltipBind(&tt, "show file info" , ControlMask, XK_G    , 0, showFileInfo    , NULL);
	addTooltipBind(&tt, "command mode"   , 0          , XK_colon, 0, enterCommandMode, NULL);
	addTooltipBind(&tt, "show tracker"   , 0          , XK_F1   , 0, showTracker     , NULL);
	addTooltipBind(&tt, "show instrument", 0          , XK_F2   , 0, showInstrument  , NULL);
	addTooltipBind(&tt, "show master"    , 0          , XK_F3   , 0, showMaster      , NULL);
	addTooltipBind(&tt, "start playback" , 0          , XK_F5   , 0, startPlayback   , NULL);
	addTooltipBind(&tt, "stop playback"  , 0          , XK_F6   , 0, stopPlayback    , NULL);

	switch (w->mode)
	{
		case MODE_COMMAND: initCommandInput(&tt); break;
		default:
			switch (w->page)
			{
				case PAGE_TRACK_VARIANT: case PAGE_TRACK_EFFECT: initTrackerInput            (&tt); break;
				case PAGE_INSTRUMENT:                            initInstrumentInput         (&tt); break;
				case PAGE_EFFECT_MASTER: case PAGE_EFFECT_SEND:  initMasterInput             (&tt); break;
				case PAGE_PLUGINBROWSER:                         initPluginEffectBrowserInput(&tt); break;
			} break;
	}
}

void resize(int _)
{
	ioctl(1, TIOCGWINSZ, &ws);
	w->centre = (ws.ws_row>>1) + 1;

	resizeWaveform();
	resizeBrowser(fbstate,
			INSTRUMENT_INDEX_COLS + 2,         /* x */
			TRACK_ROW + 1,                     /* y */
			ws.ws_col - INSTRUMENT_INDEX_COLS, /* w */
			ws.ws_row - TRACK_ROW - 1);        /* h */
	resizeBrowser(pbstate,
			1,                          /* x */
			TRACK_ROW + 1,              /* y */
			ws.ws_col,                  /* w */
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
	while (1)
	{
		if (!mainM_SEM())
		{
			if (p->resize) { p->resize = 0; resize(0); }
			if (p->redraw) { p->redraw = 0; redraw();  }

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
			handleStdin(&tt); /* ensure that semaphores are handled between input and draw */
		}

		req.tv_sec  = 0; /* nanosleep can set this higher sometimes, so set every cycle */
		req.tv_nsec = UPDATE_DELAY;
		while(nanosleep(&req, &req) < 0);
	} cleanup(0);
}
