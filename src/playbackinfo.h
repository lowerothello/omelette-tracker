typedef struct PlaybackInfo
{
	struct _Song  *s;
	struct _UI    *w;
	struct { jack_port_t *l, *r; } in, out;
	jack_port_t   *midiout;
	bool           redraw; /* request a screen redraw */
	bool           resize; /* request a screen resize */
	Event          event[EVENT_QUEUE_MAX];
	uint8_t        eventc; /* the event index pushEvent() should populate */
	uint8_t        xeventthreadsem; /* semaphore for the xevent thread, TODO: merge with the event system */
} PlaybackInfo;
PlaybackInfo *p;
