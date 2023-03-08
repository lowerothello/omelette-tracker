#include "config.h"
#include "dsp/dsp.c"

#include "button.h"
#include "scrollbar.h"

#include "playback.h"
#include "init.h"

#include "draw_helpers.c" /* helper functions for drawing to the screen */
#include "event.h"        /* event library */

struct _Song;
struct _UI;

#include "playbackinfo.h"

enum WaveShape {
	SHAPE_PULSE,
	SHAPE_LINEAR,
	SHAPE_SINE,
};

#include "tooltip.h" /* like half the input handling code */
#include "control.h" /* cool visual control library */

#include "redraw.h" /* the redraw callback and ruler code */

/* TODO: merge modulation with oscillators? */
#include "modulation/envelope.c"
#include "modulation/lfo.c"

#include "effect/effect.h"         /* audio effects (LADSPA, LV2) */
#include "effect/draw.h"           /* audio effects (LADSPA, LV2) */
#include "tracker/tracker.h"       /* tracker page */
#include "instrument/instrument.h" /* instrument page */
#include "instrument/input.h"
#include "instrument/waveform.h"
#include "instrument/draw.h"

/* TODO: should be in a header */
enum Mode {
	MODE_NORMAL,
	MODE_INSERT,
	MODE_EFFECT,
	MODE_MOUSEADJUST,
	MODE_VISUAL,
	MODE_VISUALLINE,
	MODE_VISUALREPLACE,
	MODE_COMMAND,
};

#include "command.h"

#include "song.h"
#include "window.h"

#include "file/file.h"

#include "browser.h"
#include "filebrowser.h"
#include "pluginbrowser.h"

#include "process.h"
#include "macros/macros.h"

#include "input.h"

#include "song.c"
#include "window.c"

#include "event.c"

#include "generator/sampler.h"

#include "tracker/tracker.c"
#include "effect/effect.c"
#include "effect/draw.c"
#include "instrument/instrument.c"
#include "instrument/input.c"
#include "instrument/draw.c"

#include "filebrowser.c"
#include "pluginbrowser.c"
#include "command.c"

typedef struct {
	struct { jack_default_audio_sample_t *l, *r; } in, out;
	void *midiout;
} portbuffers;
portbuffers pb;

#include "macros/macros.c"
#include "process.c"

#include "redraw.c"
#include "playback.c"
#include "tooltip.c"
#include "init.c"
