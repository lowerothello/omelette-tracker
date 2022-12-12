#include "redraw.h"
#include "event.h"

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

#include "tooltip.c"
#include "control.c"

enum WAVE_SHAPE {
	SHAPE_PULSE,
	SHAPE_LINEAR,
	SHAPE_SINE,
};

#include "modulation/envelope.c"
#include "modulation/lfo.c"

#include "effect/effect.h"
#include "tracker/tracker.h"
#include "instrument/instrument.h"
#include "master.h"

#define COMMAND_LENGTH 512
#define COMMAND_HISTORY_LENGTH 32

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

#include "../generator/sampler.h"

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
