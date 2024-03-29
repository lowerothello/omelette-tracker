enum InstRecLock
{
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

enum Page
{
	PAGE_VARIANT,
	PAGE_PATTERN,
	PAGE_EFFECT,
	PAGE_INSTRUMENT,
	PAGE_PLUGINBROWSER,
};

enum Mode {
	MODE_NORMAL,
	MODE_INSERT,
	MODE_MOUSEADJUST,
	MODE_VISUAL,
	MODE_VISUALLINE,
	MODE_VISUALREPLACE,
	MODE_REPL,
};


#define MAX_OCTAVE 7 /* max centre octave, this +2 is accessible with the keyboard */
#define MIN_OCTAVE 0
#define MAX_STEP 15
#define MIN_STEP 0
#define TRACKERFX_MIN -1
#define TRACKERFX_VISUAL_MIN 0
#define DEF_OCTAVE 4
#define DEF_STEP 0

/* runtime state, NOT serialized */
/* more than is needed for just playback, TODO: should be split into a few separate structures */
typedef struct UI
{
	// Variant *pbvariantv[TRACK_MAX];
	// Vtrig   *pbindex   [TRACK_MAX];
	uint8_t  pbtrackc; /* how many tracks are in the pattern data buffer */
	int8_t   pbfx[2];  /* patternbuffer horizontal clipping region       */
	uint8_t  defvariantlength;

	Inst   instbuffer;   /* instrument paste buffer */
	Track  trackbuffer;  /* track paste buffer      */
	Effect effectbuffer; /* effect paste buffer     */

	char filepath[REPL_LENGTH];

	void         (*filebrowserCallback)(char *); /* arg is the selected path */
	Repl           repl;
	enum Page      page, oldpage;
	enum Mode      mode, oldmode;
	unsigned short centre;
	uint8_t        pattern;    /* focused pattern */
	uint8_t        track;      /* focused track */
	short          instrument; /* focused instrument, TODO: should be a uint8_t */
	uint8_t        sample;     /* focused instrument sample slot */

	int            trackerfy, visualfy;
	int8_t         trackerfx, visualfx;
	uint8_t        visualtrack;

	int           filebrowserindex;
	uint32_t      plugineffectindex;
	bool          pluginbrowserbefore; /* true to place plugins before the cursor, false to place plugins after the cursor */

	unsigned short mousey, mousex;

	short       fyoffset, fxoffset;
	signed char shiftoffset;
	signed char trackoffset;
	signed char fieldpointer;

	int8_t wtparam;

	bool        showtooltip;
	char        chord; /* key chord buffer, vi-style multi-letter commands */
	uint8_t     count; /* action repeat count, follows similar rules to w->chord */
	signed char octave;
	signed char step;
	bool        follow;

	Track  *previewtrack[PREVIEW_TRACKS];
	Sample *previewsample; /* used by the filebrowser to soft load samples */

	uint8_t          instreci;  /* NOT a realindex */
	enum InstRecLock instrecv;
	short           *recbuffer; /* disallow removing an instrument while recording to it */
	uint32_t         recptr;

	uint16_t  playfy;  /* analogous to window->trackerfy                                 */
	uint16_t  spr;     /* samples per row (samplerate * (60 / bpm) / (rowhighlight * 2)) */
	uint16_t  sprp;    /* samples per row progress                                       */
	bool      playing; /* true if the sequencer is running                               */
	bool      loop;    /* true if the current order index is looping                     */
	short     queue;   /* next order index to play, -1 for next index                    */
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
void showEffect(void);

UI *allocWindow(void);
void freeWindow(UI*);
