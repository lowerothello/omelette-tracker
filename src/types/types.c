struct _Song;
struct _Window;

#define EVENT_QUEUE_MAX 16
typedef struct {
	struct _Song   *s;
	struct _Window *w;
	struct { jack_port_t *l, *r; } in, out;
	jack_port_t    *midiout;
	bool            redraw; /* request a screen redraw */
	bool            resize; /* request a screen resize */
	Event           eventv[EVENT_QUEUE_MAX];
	uint8_t         eventc; /* the eventv index pushEvent() should populate */
} PlaybackInfo;
PlaybackInfo *p;

#include "effect.h"
#include "variant.h"
#include "channel.h"
#include "instrument.h"

#include "song.h"
#include "song.c"
Song *s;

#include "window.h"
Window *w;

#include "effect.c"
#include "variant.c"
#include "channel.c"
#include "instrument.c"

typedef struct {
	struct { jack_default_audio_sample_t *l, *r; } in, out;
	void *midiout;
} portbuffers;
portbuffers pb;
