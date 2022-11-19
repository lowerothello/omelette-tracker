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
} INST_REC;

enum { /* pages */
	PAGE_TRACK_VARIANT,
	PAGE_TRACK_EFFECT,
	PAGE_INSTRUMENT_SAMPLE,
	PAGE_INSTRUMENT_EFFECT,
	PAGE_EFFECT_MASTER,
	PAGE_EFFECT_SEND,
	PAGE_PLUGINBROWSER,
} PAGE;

enum { /* tracker modes */
	T_MODE_NORMAL,
	T_MODE_INSERT,
	T_MODE_MOUSEADJUST,
	T_MODE_VISUAL,
	T_MODE_VISUALLINE,
	T_MODE_VISUALREPLACE,
} T_MODE;

enum { /* instrument modes */
	I_MODE_NORMAL,
	I_MODE_INSERT,
} I_MODE;

enum {
	PTRIG_OK,     /* no queued preview            */
	PTRIG_NORMAL, /* queued s->instrument preview */
	PTRIG_FILE,   /* queued filebrowser preview   */
} PTRIG;

#define TRACKERFX_MIN -1
#define TRACKERFX_VISUAL_MIN 0
typedef struct _Window {
	Variant *pbvariantv[TRACK_MAX];
	Vtrig   *vbtrig    [TRACK_MAX];
	uint8_t  pbtrackc; /* how many tracks are in the pattern buffer */
	int8_t   pbfx[2];    /* patternbuffer horizontal clipping region */
	Instrument instrumentbuffer; /* instrument paste buffer */
	uint8_t    defvariantlength;

	TrackData trackbuffer; /* track paste buffer */

	char filepath[COMMAND_LENGTH];

	void         (*filebrowserCallback)(char *); /* arg is the selected path */
	command_t      command;
	unsigned char  page, oldpage;
	unsigned char  mode, oldmode;
	unsigned short centre;
	uint8_t        pattern;    /* focused pattern */
	uint8_t        track;    /* focused track */
	short          instrument; /* focused instrument, TODO: should be a uint8_t */

	uint16_t       trackerfy, visualfy;
	int8_t         trackerfx, visualfx;
	uint8_t        visualtrack;

	short          effectscroll;

	bool          showfilebrowser; /* show the instrument sample browser */
	int           filebrowserindex;
	uint32_t      plugineffectindex;
	bool          pluginbrowserbefore; /* true to place plugins before the cursor, false to place plugins after the cursor */
	EffectChain **pluginbrowserchain;  /* which chain to place plugins into */

	unsigned short mousey, mousex;

	short       fyoffset;
	short       shiftoffset; /* TODO */
	signed char trackoffset;
	signed char fieldpointer;

	Canvas  *waveformcanvas;
	char   **waveformbuffer;
	size_t   waveformw, waveformh;
	uint32_t waveformdrawpointer;

	int8_t wtparam;

	char     chord; /* key chord buffer, vi-style multi-letter commands */
	uint16_t count; /* action repeat count, follows similar rules to w->chord */
	char     octave;
	uint8_t  step;
	char     keyboardmacro;
	bool     keyboardmacroalt;
	bool     follow;

	Row     previewrow;
	Track   previewtrack;
	Sample *previewsample; /* used by the filebrowser to soft load samples */
	char    previewtrigger;

	uint8_t  instrumentreci; /* NOT a realindex */
	uint8_t  instrumentrecv; /* value, set to an INST_REC_LOCK constant */
	short   *recbuffer;      /* disallow removing an instrument while recording to it */
	uint32_t recptr;

	char newfilename[COMMAND_LENGTH]; /* used by readSong */
} Window;
