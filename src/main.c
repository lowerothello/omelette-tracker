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
#include <pthread.h>
#include <sys/param.h> /* MIN/MAX (pulseaudio wants this) */
#include <dlfcn.h>
#include <errno.h>
#include <sndfile.h>           /* audio file read/write           */
// #ifdef OML_X11
#include <X11/Xlib.h>          /* X11 hack for key release events */
#include <X11/keysym.h>
// #endif
#include <valgrind/valgrind.h> /* valgrind hooks                  */


/* libdrawille */
#include "../lib/libdrawille/src/Canvas.h"

#include "util.c"

/* version */
const uint16_t version = 0x0001;

#define LINENO_COLS 7

#define TRACK_ROW 3 /* rows above the track headers */

#define RECORD_LENGTH 600 /* max record length, in seconds */

int DEBUG;

uint32_t samplerate, buffersize;
uint32_t rampmax;

struct winsize ws;

/* yes, there's a whole ass file just for includes */
#include "includes.c"

void filebrowserEditCallback(char *path)
{
	if (path)
	{
		strcpy(w->filepath, path);
		Event e;
		e.sem = M_SEM_RELOAD_REQ;
		e.callback = cb_reloadFile;
		pushEvent(&e);
	}
	w->page = PAGE_VARIANT;
}
/* void commandTabCallback(char *text)
{
	char *buffer = malloc(strlen(text) + 1);
	wordSplit(buffer, text, 0);
	// if      (!strcmp(buffer, "bpm")) snprintf(text, COMMAND_LENGTH + 1, "bpm %d", s->bpm);
	free(buffer);
} */

static bool commandCallback(char *command, enum Mode *mode)
{
	char *buffer = malloc(strlen(command) + 1);
	wordSplit(buffer, command, 0);
	if      (!strcmp(buffer, "q"))   { free(buffer); buffer = NULL; return 1; }
	else if (!strcmp(buffer, "q!"))  { free(buffer); buffer = NULL; return 1; }
	else if (!strcmp(buffer, "w"))   { wordSplit(buffer, command, 1); writeSongNew(s, buffer); }
	else if (!strcmp(buffer, "wq"))
	{
		wordSplit(buffer, command, 1);
		if (!writeSongNew(s, buffer)) { free(buffer); buffer = NULL; return 1; } /* exit if writing the file succeeded */
	} else if (!strcmp(buffer, "e"))
	{
		wordSplit(buffer, command, 1);
		if (strcmp(buffer, ""))
		{
			strcpy(w->filepath, buffer);
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
	setAutoRepeatOn();
	setCommand(&w->command, &commandCallback, NULL, NULL, 1, ":", "");
	w->oldmode = w->mode;
	switch (w->mode)
	{
		case MODE_VISUAL:
		case MODE_VISUALLINE:
		case MODE_VISUALREPLACE:
		case MODE_INSERT:
		case MODE_MOUSEADJUST:
			w->oldmode = MODE_NORMAL;
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
	clearTooltip();
	addTooltipBind("SIGINT"         , ControlMask, XK_C    , 0, (void(*)(void*))cleanup       , NULL);
	addTooltipBind("show file info" , ControlMask, XK_G    , 0, (void(*)(void*))showFileInfo  , NULL);

	addTooltipBind("show tracker"   , 0          , XK_F1   , 0, (void(*)(void*))showTracker   , NULL);
	addTooltipBind("show instrument", 0          , XK_F2   , 0, (void(*)(void*))showInstrument, NULL);

	addTooltipBind("start playback" , 0          , XK_F5   , 0, (void(*)(void*))startPlayback , NULL);
	addTooltipBind("stop playback"  , 0          , XK_F6   , 0, (void(*)(void*))stopPlayback  , NULL);

	switch (w->mode)
	{
		case MODE_COMMAND: initCommandInput(); break;
		default:
			addTooltipBind("command mode", 0, XK_colon   , 0, enterCommandMode             , NULL);
			addTooltipBind("hide tooltip", 0, XK_question, 0, (void(*)(void*))toggleTooltip, NULL);
			switch (w->page)
			{
				case PAGE_VARIANT: initTrackerInput(); break;
				case PAGE_INSTRUMENT: initInstrumentInput(); break;
				case PAGE_PLUGINBROWSER: initPluginEffectBrowserInput(); break;
			} break;
	}
}

void resize(int signal)
{
	ioctl(1, TIOCGWINSZ, &ws);
	w->centre = (ws.ws_row>>1) + 1;

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

int main(int argc, char *argv[])
{
	if (argc > 1 && (!strcmp(argv[1], "-v") || !strcmp(argv[1], "--version")))
	{
		printf("%s, v$%04x\n", PROGRAM_TITLE, version);
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
			if (w->instrecv == INST_REC_LOCK_CANCEL)
			{
				free(w->recbuffer); w->recbuffer = NULL;
				w->instrecv = INST_REC_LOCK_OK;
				p->redraw = 1;
			} else if (w->instrecv == INST_REC_LOCK_END)
			{
				if (w->recptr > 0)
				{
					Sample *sample = malloc(sizeof(Sample) + (w->recptr<<1)*sizeof(short));
					sample->length = w->recptr;
					sample->channels = 2;
					sample->rate = sample->defrate = samplerate;
					memcpy(&sample->data, w->recbuffer, (w->recptr<<1)*sizeof(short));
					free(w->recbuffer); w->recbuffer = NULL;
					reparentSample(&s->inst->v[s->inst->i[w->instreci]], sample);
					w->recptr = 0;
				} else { free(w->recbuffer); w->recbuffer = NULL; }

				w->instrecv = INST_REC_LOCK_OK;
				p->redraw = 1;
			}
			handleStdin(); /* ensure that semaphores are handled between input and draw */
		}

		req.tv_sec = 0;
		req.tv_nsec = UPDATE_DELAY;
		while (nanosleep(&req, &req) < 0);
	}
}
