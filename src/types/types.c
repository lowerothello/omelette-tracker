struct _Song;
struct _UI;

#define EVENT_QUEUE_MAX 16
typedef struct {
	struct _Song *s;
	struct _UI   *w;
	struct { jack_port_t *l, *r; } in, out;
	jack_port_t  *midiout;
	bool          redraw; /* request a screen redraw */
	bool          resize; /* request a screen resize */
	Event         eventv[EVENT_QUEUE_MAX];
	uint8_t       eventc; /* the eventv index pushEvent() should populate */
} PlaybackInfo;
PlaybackInfo *p;

#include "tooltip.c"
TooltipState tt;


#include "effect.h"
#include "variant.h"
#include "track.h"
#include "instrument.h"

#include "song.h"
#include "song.c"
Song *s;

#define COMMAND_LENGTH 512
#define COMMAND_HISTORY_LENGTH 32

#include "window.h"
UI *w;

#include "effect.c"
#include "variant.c"
#include "track.c"
#include "instrument.c"

#include "command.c"

typedef struct {
	struct { jack_default_audio_sample_t *l, *r; } in, out;
	void *midiout;
} portbuffers;
portbuffers pb;
