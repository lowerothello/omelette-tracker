#include "config.h"
#include "dsp/dsp.c"

#include "playback.h"
#include "init.h"

#include "file.h"         /* file helper functions */

#include "redraw.h"       /* the redraw callback (lol) */
#include "draw_helpers.c" /* helper functions for drawing to the screen */
#include "event.h"        /* event library */

struct _Song;
struct _UI;
typedef struct {
	struct _Song *s;
	struct _UI   *w;
	struct { jack_port_t *l, *r; } in, out;
	jack_port_t  *midiout;
	bool          redraw; /* request a screen redraw */
	bool          resize; /* request a screen resize */
	Event         eventv[EVENT_QUEUE_MAX];
	uint8_t       eventc; /* the eventv index pushEvent() should populate */
	uint8_t       xeventthreadsem; /* semaphore for the xevent thread, TODO: merge with the event system */
} PlaybackInfo;
PlaybackInfo *p;

enum WAVE_SHAPE {
	SHAPE_PULSE,
	SHAPE_LINEAR,
	SHAPE_SINE,
};

#include "tooltip.h" /* like half the input handling code */
#include "control.c" /* cool visual control library */

/* TODO: merge modulation with oscillators? */
#include "modulation/envelope.c"
#include "modulation/lfo.c"

#include "effect/effect.h"         /* audio effects (LADSPA, LV2) */
#include "tracker/tracker.h"       /* tracker page code */
#include "instrument/instrument.h" /* instrument page code */
#include "master.h"                /* master effects page code TODO: very unfinished */

/* TODO: should be in a header */
enum _Mode {
	T_MODE_NORMAL,
	T_MODE_INSERT,
	T_MODE_MOUSEADJUST,
	T_MODE_VISUAL,
	T_MODE_VISUALLINE,
	T_MODE_VISUALREPLACE,
	I_MODE_NORMAL,
	I_MODE_INSERT,
	MODE_COMMAND,
};

#include "command.h"

#include "song.h"
#include "window.h"

#include "browser.h"
#include "filebrowser.h"
#include "pluginbrowser.h"

#include "macros.h"
#include "process.h"

#include "song.c"
#include "window.c"

#include "input.h"
#include "event.c"

#include "generator/sampler.h"

#include "effect/effect.c"
#include "tracker/tracker.c"
#include "instrument/instrument.c"
#include "master.c"

#include "filebrowser.c"
#include "pluginbrowser.c"
#include "command.c"

typedef struct {
	struct { jack_default_audio_sample_t *l, *r; } in, out;
	void *midiout;
} portbuffers;
portbuffers pb;

#include "macros.c"
#include "process.c"

#include "redraw.c"
#include "playback.c"
#include "init.c"