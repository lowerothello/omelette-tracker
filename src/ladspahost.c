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

void resetInput(void);

/* omelette libs */
struct winsize ws;
int DEBUG;
jack_nframes_t samplerate, buffersize;
#define MIN(X, Y) ((X)<(Y)?(X):(Y))
#define MAX(X, Y) ((X)>(Y)?(X):(Y))
#include "config.h"
#include "types/draw_helpers.c"
#include "types/tooltip.c"
#include "types/control.c"
#include "types/effect/autogenui.c"
#include "types/effect/ladspa.h"

typedef struct {
	bool           redraw;
	bool           resize;
	bool           reload;
	const char    *soname;
	unsigned long  index;
	LadspaState   *state;
	void          *dl;
	float         *input[2];
	float         *output[2];
	jack_port_t   *inport[2]; /* TODO: bad disambiguation */
	jack_port_t   *outport[2];
	jack_client_t *client;
	int            unsafe;
} P;
P p;

void cleanup(int signal)
{
	jack_deactivate(p.client);
	jack_client_close(p.client);

	cleanupTerminal();

	if (p.state)
	{
		freeLadspaEffect(p.state);
		free(p.state);
	}
	if (p.dl) dlclose(p.dl);

	free(p.input[0]);
	free(p.input[1]);
	free(p.output[0]);
	free(p.output[1]);

	printf("%d\n", signal);
	exit(signal);
}

#define EFFECT_WIDTH 40

void redraw(void)
{
	p.redraw = 0;

	fcntl(0, F_SETFL, 0); /* blocking */

	/* "CSI 2 J"      clears the screen              */
	/* "CSI ? 2 5 h"  ensures the cursor is visible  */
	/* "CSI 2   q"    sets the cursor shape to block */
	printf("\033[2J\033[?25h\033[2 q");

	if (p.state)
	{
		clearControls(&cc);
		drawLadspaEffect(p.state, &cc,
				((ws.ws_col - EFFECT_WIDTH)>>1) + 1, EFFECT_WIDTH,
				((ws.ws_row - getLadspaEffectHeight(p.state))>>1) + 2, 1, ws.ws_row);

		drawControls(&cc);

		/* drawTooltip(&tt); */
	} else
	{ /* invalid plugin requested */
#define INDEX_TEXT ", index "
		char buffer[strlen(p.soname) + strlen(INDEX_TEXT) + 3];
		sprintf(buffer, "%s%s%02x", p.soname, INDEX_TEXT, (uint8_t)p.index);
		drawCentreText(1, (ws.ws_row>>1) - 1, ws.ws_col, "Invalid plugin requested:");
		drawCentreText(1, (ws.ws_row>>1), ws.ws_col, buffer);
	}

	fflush(stdout);
	fcntl(0, F_SETFL, O_NONBLOCK); /* non-blocking */
}
void resize(void)
{
	p.resize = 0;
	p.redraw = 1;

	ioctl(1, TIOCGWINSZ, &ws);
}

static void toggleKeyControlRedraw(ControlState *cc) { toggleKeyControl(cc); p.redraw = 1; }
static void revertKeyControlRedraw(ControlState *cc) { revertKeyControl(cc); p.redraw = 1; }
static void incControlValueRedraw (ControlState *cc) { incControlValue (cc); p.redraw = 1; }
static void decControlValueRedraw (ControlState *cc) { decControlValue (cc); p.redraw = 1; }
static void upArrow  (void) { decControlCursor(&cc, 1); p.redraw = 1; }
static void downArrow(void) { incControlCursor(&cc, 1); p.redraw = 1; }
static void leftArrow (void) { incControlFieldpointer(&cc); p.redraw = 1; }
static void rightArrow(void) { decControlFieldpointer(&cc); p.redraw = 1; }
static void mouse(enum Button button, int x, int y) { mouseControls(&cc, button, x, y); p.redraw = 1; }
void resetInput(void)
{
	clearTooltip(&tt);
	setTooltipMouseCallback(&tt, mouse);
	addTooltipBind(&tt, "toggle checkmark button" , 0          , XK_Return   , TT_DRAW, (void(*)(void*))toggleKeyControlRedraw, &cc );
	addTooltipBind(&tt, "reset control to default", 0          , XK_BackSpace, TT_DRAW, (void(*)(void*))revertKeyControlRedraw, &cc );
	addTooltipBind(&tt, "increment value"         , ControlMask, XK_a        , TT_DRAW, (void(*)(void*))incControlValueRedraw , &cc );
	addTooltipBind(&tt, "decrement value"         , ControlMask, XK_x        , TT_DRAW, (void(*)(void*))decControlValueRedraw , &cc );
	addTooltipBind(&tt, "up arrow"                , 0          , XK_Up       , TT_DRAW, (void(*)(void*))upArrow               , NULL);
	addTooltipBind(&tt, "down arrow"              , 0          , XK_Down     , TT_DRAW, (void(*)(void*))downArrow             , NULL);
	addTooltipBind(&tt, "left arrow"              , 0          , XK_Left     , TT_DRAW, (void(*)(void*))leftArrow             , NULL);
	addTooltipBind(&tt, "right arrow"             , 0          , XK_Right    , TT_DRAW, (void(*)(void*))rightArrow            , NULL);
}

void reload(void)
{
	p.redraw = 1;
	p.reload = 0;
	p.unsafe = 1;

	struct timespec req;
	while (p.unsafe == 1)
	{
		req.tv_sec = 0;
		req.tv_nsec = UPDATE_DELAY;
		while (nanosleep(&req, &req) < 0);
	}

	if (p.state)
	{
		freeLadspaEffect(p.state);
		free(p.state);
	}
	if (p.dl) dlclose(p.dl);

	const LADSPA_Descriptor *desc = NULL;
	p.dl = getSpecificLadspaDescriptor(&desc, p.soname, p.index);
	if (desc)
	{
		initLadspaEffect(&p.state, p.input, p.output, desc);
	}

	p.unsafe = 0;
}

void sigwinch(int signal) { p.resize = 1; }
void sigusr1(int signal) { p.reload = 1; }

int process(jack_nframes_t bufsize, void *arg)
{
	if (p.unsafe == 1)
		p.unsafe++;

	if (p.unsafe || !p.state) return 0;

	jack_default_audio_sample_t *inl = jack_port_get_buffer(p.inport[0], bufsize);
	jack_default_audio_sample_t *inr = jack_port_get_buffer(p.inport[1], bufsize);
	jack_default_audio_sample_t *outl = jack_port_get_buffer(p.outport[0], bufsize);
	jack_default_audio_sample_t *outr = jack_port_get_buffer(p.outport[1], bufsize);

	memcpy(p.input[0], inl, sizeof(float) * bufsize);
	memcpy(p.input[1], inr, sizeof(float) * bufsize);
	runLadspaEffect(bufsize, p.state, p.input, p.output);
	memcpy(outl, p.output[0], sizeof(float) * bufsize);
	memcpy(outr, p.output[1], sizeof(float) * bufsize);

	return 0;
}

int main(int argc, char *argv[])
{
	if (argc < 3)
	{
		printf("usage: %s {soname} {index}\n", argv[0]);
		return 1;
	}

	p.soname = argv[1];
	p.index = atoi(argv[2]);

	p.client = jack_client_open("omuLADSPA", JackNullOption, NULL);
	p.inport[0] = jack_port_register(p.client, "in_l", JACK_DEFAULT_AUDIO_TYPE, JackPortIsInput, 0);
	p.inport[1] = jack_port_register(p.client, "in_r", JACK_DEFAULT_AUDIO_TYPE, JackPortIsInput, 0);
	p.outport[0] = jack_port_register(p.client, "out_l", JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0);
	p.outport[1] = jack_port_register(p.client, "out_r", JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0);

	samplerate = jack_get_sample_rate(p.client);
	buffersize = jack_get_buffer_size(p.client);

	p.input[0] = calloc(buffersize, sizeof(float));
	p.input[1] = calloc(buffersize, sizeof(float));
	p.output[0] = calloc(buffersize, sizeof(float));
	p.output[1] = calloc(buffersize, sizeof(float));

	signal(SIGINT,   &cleanup);
	signal(SIGTERM,  &cleanup);
	signal(SIGSEGV,  &cleanup);
	signal(SIGFPE,   &cleanup);
	signal(SIGWINCH, &sigwinch);
	signal(SIGUSR1,  &sigusr1);

	p.unsafe = 2; /* hang the process thread until the plugin has been read */

	jack_set_process_callback(p.client, process, NULL);
	jack_activate(p.client);

	initTerminal();

	resetInput();

	p.resize = 1;
	p.reload = 1;

	struct timespec req;
	while (1)
	{
		if (p.reload) reload();
		if (p.resize) resize();
		if (p.redraw) redraw();
		handleStdin(&tt);

		req.tv_sec = 0;
		req.tv_nsec = UPDATE_DELAY;
		while (nanosleep(&req, &req) < 0);
	}

	cleanup(0);
}
