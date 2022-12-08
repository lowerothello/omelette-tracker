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

#define INSTRUMENT_INDEX_COLS 18

#define RECORD_LENGTH 600 /* max record length, in seconds */

int DEBUG;

jack_nframes_t samplerate, buffersize;
jack_nframes_t rampmax;

struct winsize ws;

/* prototypes, TODO: a proper header file */
void startPlayback(void);
void stopPlayback (void);
void resetInput(void);
void cleanup(int);

#include "config.h"

#ifndef NO_VALGRIND
#include <valgrind/valgrind.h>
#endif


#include "dsp.c"

#include "column.c"

#include "truecolour.h"

#include "file.c"
#include "types/types.c"


#include "init.c"


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
void startPlayback(void)
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
void stopPlayback(void)
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

static void showFileInfo(void)
{
	if (strlen(w->filepath))
		sprintf(w->command.error, "\"%.*s\"", COMMAND_LENGTH - 2, w->filepath);
	else
		strcpy(w->command.error, "No file loaded");
	p->redraw = 1;
}
static void toggleTooltip(void)
{
	w->showtooltip = !w->showtooltip;
	p->redraw = 1;
}
void resetInput(void)
{
	w->count = 0;
	w->chord = 0;
	clearTooltip(&tt);
	addTooltipBind(&tt, "show file info" , ControlMask, XK_G    , 0, (void(*)(void*))showFileInfo  , NULL);
	addTooltipBind(&tt, "show tracker"   , 0          , XK_F1   , 0, (void(*)(void*))showTracker   , NULL);
	addTooltipBind(&tt, "show instrument", 0          , XK_F2   , 0, (void(*)(void*))showInstrument, NULL);
	addTooltipBind(&tt, "show master"    , 0          , XK_F3   , 0, (void(*)(void*))showMaster    , NULL);
	addTooltipBind(&tt, "start playback" , 0          , XK_F5   , 0, (void(*)(void*))startPlayback , NULL);
	addTooltipBind(&tt, "stop playback"  , 0          , XK_F6   , 0, (void(*)(void*))stopPlayback  , NULL);

	switch (w->mode)
	{
		case MODE_COMMAND: initCommandInput(&tt); break;
		default:
			addTooltipBind(&tt, "command mode", 0, XK_colon   , 0, enterCommandMode             , NULL);
			addTooltipBind(&tt, "hide tooltip", 0, XK_question, 0, (void(*)(void*))toggleTooltip, NULL);
			switch (w->page)
			{
				case PAGE_TRACK_VARIANT: case PAGE_TRACK_EFFECT: initTrackerInput            (&tt); break;
				case PAGE_INSTRUMENT:                            initInstrumentInput         (&tt); break;
				case PAGE_EFFECT_MASTER: case PAGE_EFFECT_SEND:  initMasterInput             (&tt); break;
				case PAGE_PLUGINBROWSER:                         initPluginEffectBrowserInput(&tt); break;
			} break;
	}
}

void resize(int signal)
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
		while (nanosleep(&req, &req) < 0);
	} cleanup(0);
}
