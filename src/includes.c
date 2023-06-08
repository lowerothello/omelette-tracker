#include "config.h"
#include "audiobackend/audiobackend.h"

#include "dsp/dsp.c"

#include "button.h"
#include "scrollbar.h"

#include "playback.h"
#include "init.h"

#include "draw_helpers.c" /* helper functions for drawing to the screen */
#include "event.h" /* event library */

struct Song;
struct UI;

#include "playbackinfo.h"

enum WaveShape {
	SHAPE_PULSE,
	SHAPE_LINEAR,
	SHAPE_SINE,
};

#include "tooltip.h" /* like half the input handling code */
#include "control.h" /* cool visual control library */

#include "redraw.h" /* the redraw callback and ruler code */

#include "modulation/envelope.c"
#include "modulation/lfo.c"

#include "sample.h"
#include "effect/effect.h" /* audio effects (LADSPA, LV2) */
#include "tracker/tracker.h"
#include "instrument/instrument.h"

#include "command.h"

#include "song.h"
#include "window.h"

#include "file/file.h"

#include "browser.h"
#include "filebrowser.h"
#include "pluginbrowser.h"

#include "process.h"
#include "macros/macros.h"

#include "input/input.h"

#include "song.c"
#include "window.c"

#include "event.c"

#include "generator/sampler.h"

#include "sample.c"
#include "tracker/tracker.c"
#include "effect/effect.c"
#include "instrument/instrument.c"

#include "filebrowser.c"
#include "pluginbrowser.c"
#include "command.c"

#include "macros/macros.c"
#include "process.c"

#include "redraw.c"
#include "playback.c"
#include "tooltip.c"
#include "audiobackend/audiobackend.c"
#include "init.c"
