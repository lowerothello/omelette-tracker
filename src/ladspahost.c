/*
 * a small standalone LADSPA host for plugin testing/livecoding
 * uses omelette's routines for loading/drawing/processing plugins
 */

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <math.h>
#include <limits.h>
#include <ctype.h>
#include <string.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <dlfcn.h>
#include <dirent.h>
#include <jack/jack.h>   /* audio i/o        */
#include <jack/thread.h> /* threading helper */
#include <ladspa.h>      /* LADSPA plugins   */
#include <X11/Xlib.h> /* keysym defs */
#include <X11/keysym.h>

/* omelette libs */
struct winsize ws;
int DEBUG;
jack_nframes_t samplerate, buffersize;
#include "util.c"
#include "config.h"
#include "draw_helpers.c"
#include "button.h"
#include "tooltip.h"
#include "tracker/variant.h" /* for NOTE_MAX */
#include "tooltip.c"
#include "control.h"
#include "effect/autogenui.c"
#include "effect/ladspa.h"
#include "effect/ladspa.c"

typedef struct {
	bool           redraw;
	bool           resize;
	bool           reload;
	const char    *soname;
	unsigned long  index;
	LadspaState   *ladspa;
	void          *dl;
	float         *input[2];
	float         *output[2];
	jack_port_t   *inport[2]; /* TODO: bad disambiguation */
	jack_port_t   *outport[2];
	jack_client_t *client;
	int            unsafe;
} LadspaHostState;
LadspaHostState lhs;

void cleanup(int signal)
{
	jack_deactivate(lhs.client);
	jack_client_close(lhs.client);

	cleanupTerminal();

	if (lhs.ladspa)
	{
		freeLadspaEffect(lhs.ladspa);
		free(lhs.ladspa);
	}
	if (lhs.dl) dlclose(lhs.dl);

	free(lhs.input[0]);
	free(lhs.input[1]);
	free(lhs.output[0]);
	free(lhs.output[1]);

	printf("%d\n", signal);
	exit(signal);
}

#define EFFECT_WIDTH 40

void redraw(void)
{
	lhs.redraw = 0;

	fcntl(0, F_SETFL, 0); /* blocking */

	/* "CSI 2 J"      clears the screen              */
	/* "CSI ? 2 5 h"  ensures the cursor is visible  */
	/* "CSI 2   q"    sets the cursor shape to block */
	printf("\033[2J\033[?25h\033[2 q");

	if (lhs.ladspa)
	{
		clearControls();
		drawLadspaEffect(lhs.ladspa,
				((ws.ws_col - EFFECT_WIDTH)>>1) + 1, EFFECT_WIDTH,
				((ws.ws_row - getLadspaEffectHeight(lhs.ladspa))>>1) + 2, 1, ws.ws_row);

		drawControls();
	} else
	{ /* invalid plugin requested */
#define INDEX_TEXT ", index "
		char buffer[strlen(lhs.soname) + strlen(INDEX_TEXT) + 3];
		sprintf(buffer, "%s%s%02x", lhs.soname, INDEX_TEXT, (uint8_t)lhs.index);
		drawCentreText(1, (ws.ws_row>>1) - 1, ws.ws_col, "Invalid plugin requested:");
		drawCentreText(1, (ws.ws_row>>1), ws.ws_col, buffer);
	}

	fflush(stdout);
	fcntl(0, F_SETFL, O_NONBLOCK); /* non-blocking */
}
void resize(void)
{
	lhs.resize = 0;
	lhs.redraw = 1;

	ioctl(1, TIOCGWINSZ, &ws);
}

static void toggleKeyControlRedraw(void) { toggleKeyControl(); lhs.redraw = 1; }
static void revertKeyControlRedraw(void) { revertKeyControl(); lhs.redraw = 1; }
static void incControlValueRedraw (void) { incControlValue (); lhs.redraw = 1; }
static void decControlValueRedraw (void) { decControlValue (); lhs.redraw = 1; }
static void upArrow  (void) { decControlCursor(1); lhs.redraw = 1; }
static void downArrow(void) { incControlCursor(1); lhs.redraw = 1; }
static void leftArrow (void) { incControlFieldpointer(); lhs.redraw = 1; }
static void rightArrow(void) { decControlFieldpointer(); lhs.redraw = 1; }
static void mouse(enum Button button, int x, int y) { mouseControls(button, x, y); lhs.redraw = 1; }
void resetInput(void)
{
	clearTooltip();
	setTooltipMouseCallback(mouse);
	addTooltipBind("toggle checkmark button" , 0          , XK_Return   , TT_DRAW, (void(*)(void*))toggleKeyControlRedraw, NULL);
	addTooltipBind("reset control to default", 0          , XK_BackSpace, TT_DRAW, (void(*)(void*))revertKeyControlRedraw, NULL);
	addTooltipBind("increment value"         , ControlMask, XK_a        , TT_DRAW, (void(*)(void*))incControlValueRedraw , NULL);
	addTooltipBind("decrement value"         , ControlMask, XK_x        , TT_DRAW, (void(*)(void*))decControlValueRedraw , NULL);
	addTooltipBind("up arrow"                , 0          , XK_Up       , TT_DRAW, (void(*)(void*))upArrow               , NULL);
	addTooltipBind("down arrow"              , 0          , XK_Down     , TT_DRAW, (void(*)(void*))downArrow             , NULL);
	addTooltipBind("left arrow"              , 0          , XK_Left     , TT_DRAW, (void(*)(void*))leftArrow             , NULL);
	addTooltipBind("right arrow"             , 0          , XK_Right    , TT_DRAW, (void(*)(void*))rightArrow            , NULL);
}

void reload(void)
{
	lhs.redraw = 1;
	lhs.reload = 0;
	lhs.unsafe = 1;

	struct timespec req;
	while (lhs.unsafe == 1)
	{
		req.tv_sec = 0;
		req.tv_nsec = UPDATE_DELAY;
		while (nanosleep(&req, &req) < 0);
	}

	if (lhs.ladspa)
	{
		freeLadspaEffect(lhs.ladspa);
		free(lhs.ladspa);
	}
	if (lhs.dl) dlclose(lhs.dl);

	const LADSPA_Descriptor *desc = NULL;
	lhs.dl = getSpecificLadspaDescriptor(&desc, lhs.soname, lhs.index);
	if (desc)
	{
		initLadspaEffect(lhs.ladspa, lhs.input, lhs.output, desc);
	}

	lhs.unsafe = 0;
}

void sigwinch(int signal) { lhs.resize = 1; }
void sigusr1(int signal) { lhs.reload = 1; }

int process(jack_nframes_t bufsize, void *arg)
{
	if (lhs.unsafe == 1)
		lhs.unsafe++;

	if (lhs.unsafe || !lhs.ladspa) return 0;

	jack_default_audio_sample_t *inl = jack_port_get_buffer(lhs.inport[0], bufsize);
	jack_default_audio_sample_t *inr = jack_port_get_buffer(lhs.inport[1], bufsize);
	jack_default_audio_sample_t *outl = jack_port_get_buffer(lhs.outport[0], bufsize);
	jack_default_audio_sample_t *outr = jack_port_get_buffer(lhs.outport[1], bufsize);

	memcpy(lhs.input[0], inl, sizeof(float) * bufsize);
	memcpy(lhs.input[1], inr, sizeof(float) * bufsize);
	runLadspaEffect(bufsize, lhs.ladspa, lhs.input, lhs.output);
	memcpy(outl, lhs.output[0], sizeof(float) * bufsize);
	memcpy(outr, lhs.output[1], sizeof(float) * bufsize);

	return 0;
}

int main(int argc, char *argv[])
{
	if (argc < 3)
	{
		printf("usage: %s {soname} {index}\n", argv[0]);
		return 1;
	}

	lhs.soname = argv[1];
	lhs.index = atoi(argv[2]);

	lhs.client = jack_client_open("omuLADSPA", JackNullOption, NULL);
	lhs.inport[0] = jack_port_register(lhs.client, "in_l", JACK_DEFAULT_AUDIO_TYPE, JackPortIsInput, 0);
	lhs.inport[1] = jack_port_register(lhs.client, "in_r", JACK_DEFAULT_AUDIO_TYPE, JackPortIsInput, 0);
	lhs.outport[0] = jack_port_register(lhs.client, "out_l", JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0);
	lhs.outport[1] = jack_port_register(lhs.client, "out_r", JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0);

	samplerate = jack_get_sample_rate(lhs.client);
	buffersize = jack_get_buffer_size(lhs.client);

	lhs.input[0] = calloc(buffersize, sizeof(float));
	lhs.input[1] = calloc(buffersize, sizeof(float));
	lhs.output[0] = calloc(buffersize, sizeof(float));
	lhs.output[1] = calloc(buffersize, sizeof(float));

	signal(SIGINT,   &cleanup);
	signal(SIGTERM,  &cleanup);
	signal(SIGSEGV,  &cleanup);
	signal(SIGFPE,   &cleanup);
	signal(SIGWINCH, &sigwinch);
	signal(SIGUSR1,  &sigusr1);

	lhs.unsafe = 2; /* hang the process thread until the plugin has been read */

	jack_set_process_callback(lhs.client, process, NULL);
	jack_activate(lhs.client);

	initTerminal();

	resetInput();

	lhs.resize = 1;
	lhs.reload = 1;

	struct timespec req;
	while (1)
	{
		if (lhs.reload) reload();
		if (lhs.resize) resize();
		if (lhs.redraw) redraw();
		handleStdin();

		req.tv_sec = 0;
		req.tv_nsec = UPDATE_DELAY;
		while (nanosleep(&req, &req) < 0);
	}

	cleanup(0);
}
