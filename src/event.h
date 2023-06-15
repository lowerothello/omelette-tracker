/* communication between the main and process threads */
#define EVENT_QUEUE_MAX 16

/* main semaphore */
/* communication is always initiated by the main thread */
enum M_SEM {
	M_SEM_OK,             /* allow processing                                            */
	M_SEM_DONE,           /* pop the event                                               */
	M_SEM_RELOAD_REQ,     /* trigger downtime for a file reload on the main thread       */
	M_SEM_QUEUE_SWAP_REQ, /* queue an e->swap1 and e->swap2 swap in the proc thread      */
	M_SEM_SWAP_REQ,       /* swap e->swap1 and e->swap2 in the proc thread               */
	M_SEM_CALLBACK,       /* call p->semcallback() in the main thread                    */
	M_SEM_BLOCK_CALLBACK, /* call p->semcallback() and block the main thread             */
	M_SEM_BPM,            /* reapply the song bpm                                        */
	M_SEM_TRACK_MUTE,     /* apply track mutes to midi data                              */
	M_SEM_INPUT,          /* queued X server input events                                */
	M_SEM_PREVIEW,        /* arg1=note, arg2=inst, arg3=release                          */
	M_SEM_PLAYING_START,  /* start the sequencer                                         */
	M_SEM_PLAYING_STOP,   /* stop the sequencer                                          */
};

typedef struct event
{
	enum M_SEM sem;
	int arg1, arg2, arg3;
	void     **dest; /* swapping sets *.dest to .src, leaving what *.dest used to be in .src */
	void      *src;  /* note that only .dest is a double pointer */
	void     (*callback)(struct event*);
	void      *callbackarg;
} Event;

void pushEvent(Event*);
