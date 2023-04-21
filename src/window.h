enum { /* TODO: port to the event system */
	INST_REC_LOCK_OK,           /* playback and most memory ops are safe */
	INST_REC_LOCK_START,        /* want to be unsafe                     */
	INST_REC_LOCK_CUE_START,    /* want to be unsafe                     */
	INST_REC_LOCK_CONT,         /* recording                             */
	INST_REC_LOCK_CUE_CONT,     /* recording                             */
	INST_REC_LOCK_PREP_END,     /* start stopping recording              */
	INST_REC_LOCK_END,          /* stopping recording has finished       */
	INST_REC_LOCK_PREP_CANCEL,  /* start cancelling recording            */
	INST_REC_LOCK_CANCEL,       /* cancelling recording has finished     */
};

enum Page {
	PAGE_VARIANT,
	PAGE_INSTRUMENT,
	PAGE_PLUGINBROWSER,
};

// enum Ptrig {
// 	PTRIG_OK,     /* no queued preview            */
// 	PTRIG_NORMAL, /* queued s->instrument preview */
// 	PTRIG_FILE,   /* queued filebrowser preview   */
// };

#define MAX_OCTAVE 7 /* this +2 is accessible with the keyboard */
#define MIN_OCTAVE 0
#define MAX_STEP 15
#define MIN_STEP 0
#define TRACKERFX_MIN -1
#define TRACKERFX_VISUAL_MIN 0
#define DEF_OCTAVE 4
#define DEF_STEP 0

/* runtime state, NOT serialized */
typedef struct UI
{
	Variant *pbvariantv[TRACK_MAX];
	Vtrig   *vbtrig    [TRACK_MAX];
	uint8_t  pbtrackc; /* how many tracks are in the pattern buffer */
	int8_t   pbfx[2];  /* patternbuffer horizontal clipping region */
	uint8_t  defvariantlength;

	Inst   instbuffer;   /* instrument paste buffer */
	Track  trackbuffer;  /* track paste buffer      */
	Effect effectbuffer; /* effect paste buffer     */

	char filepath[COMMAND_LENGTH];

	void         (*filebrowserCallback)(char *); /* arg is the selected path */
	Command        command;
	enum Page      page, oldpage;
	enum Mode      mode, oldmode;
	unsigned short centre;
	uint8_t        pattern;    /* focused pattern */
	uint8_t        track;      /* focused track */
	short          instrument; /* focused instrument, TODO: should be a uint8_t */
	uint8_t        sample;     /* focused instrument sample slot */

	uint16_t       trackerfy, visualfy;
	int8_t         trackerfx, visualfx;
	uint8_t        visualtrack;

	int           filebrowserindex;
	uint32_t      plugineffectindex;
	bool          pluginbrowserbefore; /* true to place plugins before the cursor, false to place plugins after the cursor */

	unsigned short mousey, mousex;

	short       fyoffset, fxoffset;
	short       shiftoffset; /* TODO */
	signed char trackoffset;
	signed char fieldpointer;

	int8_t wtparam;

	bool        showtooltip;
	char        chord; /* key chord buffer, vi-style multi-letter commands */
	uint8_t     count; /* action repeat count, follows similar rules to w->chord */
	signed char octave;
	signed char step;
	bool        follow;

	Track *previewtrack[PREVIEW_TRACKS];
	Sample *previewsample; /* used by the filebrowser to soft load samples */

	uint8_t  instreci;  /* NOT a realindex */
	uint8_t  instrecv;  /* value, set to an INST_REC_LOCK constant */
	short   *recbuffer; /* disallow removing an instrument while recording to it */
	uint32_t recptr;

	short    *bpmcache;    /* bpm change caching so multithreading isn't hell                */
	uint16_t  bpmcachelen; /* how far into bpmcache it's safe to index                       */
	uint16_t  playfy;      /* analogous to window->trackerfy                                 */
	uint16_t  spr;         /* samples per row (samplerate * (60 / bpm) / (rowhighlight * 2)) */
	uint16_t  sprp;        /* samples per row progress                                       */
	bool      playing;     /* true if the sequencer is running                               */
} UI;
UI *w;

void addOctave(int delta);
void incOctave(void) { addOctave( 1); }
void decOctave(void) { addOctave(-1); }

void addStep(int delta);
void incStep(void) { addStep( 1); }
void decStep(void) { addStep(-1); }

void showTracker(void);
void showInstrument(void);

UI *allocWindow(void);
void freeWindow(UI*);
