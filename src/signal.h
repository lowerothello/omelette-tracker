/* communication between the main and process threads */

/* main semaphore */
/* communication is always initiated by the main thread */
enum {
	M_SEM_OK,                             /* allow processing */
	M_SEM_DONE,                           /* pop the event    */
	M_SEM_RELOAD_REQ,                     /* trigger downtime for a file reload */
	M_SEM_SWAP_REQ,                       /* swap e->swap1 and e->swap2 in the process thread */
	M_SEM_CALLBACK,                       /* call p->semcallback() in the main thread */
	M_SEM_BLOCK_CALLBACK,                 /* call p->semcallback() and block the main thread */
	M_SEM_SWAP_PREVIEWSAMPLE_PREVIEW_REQ, /* swap e->swap1 and e->swap2 then preview note e->callbackarg */ /* TODO: kinda jank */
	M_SEM_BPM,          /* reapply the song bpm             */
	M_SEM_CHANNEL_MUTE, /* apply channel mutes to midi data */
} M_SEM;

typedef struct event
{
	uint8_t sem; /* M_SEM_* defines */
	void  **dest; /* swapping sets *dest to src, leaving what *dest used to be in src */
	void   *src;  /* note that only dest is a double pointer */
	void  (*callback)(struct event *);
	void   *callbackarg;
} Event;

void pushEvent(Event *e);
