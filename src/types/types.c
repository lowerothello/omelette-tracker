#define NOTE_VOID 255
#define NOTE_OFF 254
#define NOTE_UNUSED 253 /* explicitly unused */
#define NOTE_C5 60      /* centre note */
#define NOTE_A10 120    /* first out of range note */
#define INST_VOID 255
typedef struct
{
	uint8_t note; /* MIDI compatible  | NOTE_* declares */
	uint8_t inst; /* instrument index | INST_* declares */
	struct {
		char    c; /* command  */
		uint8_t v; /* argument */
	} macro[8]; /* up to 8 macro columns, TODO: dynamically allocate? */
} row;

#define VARIANT_VOID 255
#define VARIANT_OFF 254
#define VARIANT_MAX 254
#define VARIANT_ROWMAX 255
typedef struct
{
	uint16_t rowc;
	row      rowv[];
} variant;

#define C_VTRIG_LOOP 0b00000001
typedef struct
{
	uint8_t index;
	uint8_t flags;
} vtrig;

#define SONG_MAX 65535
#define STATE_ROWS 4

#define CHANNEL_MAX 32
typedef struct
{
	uint8_t  varianti[VARIANT_MAX]; /* variant index/backref   */
	variant *variantv[VARIANT_MAX]; /* variant contents        */
	uint8_t  variantc; /*              variant contents length */

	vtrig   *trig;  /* variant triggers */
	variant *songv; /* main fallback variant  */

	/* flags */
	bool mute;
	bool reverse;
	bool release;
	bool rtrig_rev;
	bool target_rand;

	uint8_t macroc; /* macro count */
} channeldata; /* raw sequence data */
typedef struct
{
	channeldata data; /* saved to disk */

	/* runtime state */
	uint32_t pointer;        /* clock */
	uint32_t pitchedpointer; /* sample to play */
	uint8_t  gain;           /* unsigned nibble per-channel */
	uint8_t  randgain;       /* gain override for the Ixy macro */
	short    targetgain;     /* smooth gain target */
	row      r;
	uint8_t  samplernote, samplerinst;
	float    finetune;                 /* calculated fine tune, clamped between -2/+2 for midi */

	float    portamentofinetune;       /* portamento fine tune */
	float    targetportamentofinetune; /* cv->portamentofinetune destination */
	float    startportamentofinetune;  /* cv->portamentofinetune start       */
	uint32_t portamentosamples;        /* portamento length   */
	uint32_t portamentosamplepointer;  /* portamento progress */

	float    microtonalfinetune;         /* used by the local microtonal macro */
	uint16_t rtrigsamples;               /* samples per retrigger */
	uint32_t rtrigpointer;               /* clock reference */
	uint32_t rtrigpitchedpointer;        /* pitchedpointer to ratchet back to */
	uint32_t rtrigcurrentpitchedpointer; /* pitchedpointer the current retrig started at */
	int8_t   rtrigblocksize;             /* number of rows block extends to */
	uint16_t cutsamples;                 /* samples into the row to cut, 0 for no cut */
	uint16_t delaysamples;               /* samples into the row to delay, 0 for no delay */
	uint8_t  delaynote;
	uint8_t  delayinst;
	uint8_t  vibrato;                    /* vibrato depth, 0-f */
	uint32_t vibratosamples;             /* samples per full phase walk */
	uint32_t vibratosamplepointer;       /* distance through cv->vibratosamples */

	short localenvelope;
	short localpitchshift;
	int   localcyclelength;

	short   midiccindex;
	uint8_t midicc;
	short   targetmidicc;

	int8_t sendgroup;
	int8_t sendgain;
	int8_t targetsendgain;

	float envgain;

	/* waveshaper */
	char    waveshaper;               /* which waveshaper to use */
	uint8_t waveshaperstrength;       /* mix / input gain */
	short   targetwaveshaperstrength; /* mix / input gain target */

	/* filter */
	SVFilter fl[2], fr[2];
	char     filtermode;
	int8_t   targetfiltermode;
	uint8_t  filtercut;
	short    targetfiltercut;
	int8_t   filterres, targetfilterres;

	/* ramping */
	uint16_t rampindex;        /* progress through the ramp buffer, rampmax if not ramping */
	short   *rampbuffer;       /* samples to ramp out */
	uint8_t  rampinst;         /* real index, needed to determine the output group */

	uint16_t stretchrampindex; /* progress through the stretch ramp buffer, >=cv->stretchrampmax if not ramping */
	uint16_t stretchrampmax;   /* actual stretchrampmax used, to allow for tiny buffer sizes */
} channel;

#define INSTRUMENT_VOID 255
#define INSTRUMENT_MAX 255
typedef struct
{
	short    *sampledata;   /* variable size, persists between types */
	uint32_t  samplelength; /* raw samples allocated for sampledata */

	uint32_t length;
	uint8_t  channels;
	int8_t   channelmode;
	uint32_t c5rate;
	uint8_t  samplerate;  /* percent of c5rate to actually use */
	int8_t   bitdepth;
	uint16_t cyclelength;
	uint8_t  pitchshift;
	bool     timestretch;
	uint32_t trim[2];
	uint32_t loop;
	uint8_t  envelope;
	bool     sustain;
	uint8_t  gain;
	bool     invert;
	bool     pingpong;
	uint8_t  loopramp;
	int8_t   midichannel;

	uint32_t triggerflash;
} instrument;


#define EFFECT_CHAIN_LEN 16
typedef struct
{
	uint8_t type;
	void   *state;
} Effect;


#define PLAYING_STOP 0
#define PLAYING_START 1
#define PLAYING_CONT 2
#define PLAYING_PREP_STOP 3
typedef struct
{
	/* instruments */
	uint8_t     instrumenti[INSTRUMENT_MAX]; /* instrument backref */
	uint8_t     instrumentc; /*                 instrument count */
	instrument *instrumentv; /*                 instrument values */

	/* channels */
	uint8_t  channelc; /* channel count */
	channel *channelv; /* channel values */

	/* song pointers */
	uint16_t playfy;  /* analogous to window->trackerfy */
	uint16_t songlen; /* how long the global variant is */
	uint16_t loop[2]; /* loop range pointers */

	/* mastering chain */
	uint8_t effectc;
	Effect *effectv;

	/* misc. state */
	uint8_t rowhighlight;
	uint8_t          bpm;
	uint8_t      songbpm; /* to store the song's bpm through bpm change macros */
	uint16_t         spr; /* samples per row (samplerate * (60 / bpm) / (rowhighlight * 2)) */
	uint16_t        sprp; /* samples per row progress */
	char         playing;
} song;
song *s;


#define INST_GLOBAL_LOCK_OK 0        /* playback and most memory ops are safe */
#define INST_GLOBAL_INST_MUTE 7      /* force stop midi for instrument locki */
#define INST_GLOBAL_CHANNEL_MUTE 8   /* one or more channels have changed mute state */

#define INST_REC_LOCK_OK 0          /* playback and most memory ops are safe */
#define INST_REC_LOCK_START 1       /* want to be unsafe                     */
#define INST_REC_LOCK_CUE_START 2   /* want to be unsafe                     */
#define INST_REC_LOCK_CONT 3        /* recording                             */
#define INST_REC_LOCK_CUE_CONT 4    /* recording                             */
#define INST_REC_LOCK_PREP_END 5    /* start stopping recording              */
#define INST_REC_LOCK_END 6         /* stopping recording has finished       */
#define INST_REC_LOCK_PREP_CANCEL 7 /* start cancelling recording            */
#define INST_REC_LOCK_CANCEL 8      /* cancelling recording has finished     */

#define W_FLAG_FOLLOW 0b00000001

#define REQ_OK 0  /* do nothing / done */
#define REQ_BPM 1 /* re-apply the song bpm */
typedef struct
{
	variant *pbvariantv[CHANNEL_MAX];
	uint8_t               pbchannelc; /* how many channels are in the pattern buffer */
	short                    pbfx[2]; /* patternbuffer clipping region */
	vtrig       *vbtrig[CHANNEL_MAX];
	uint8_t               vbchannelc; /* how many channels are in the vtrig buffer */
	uint16_t                  vbrowc; /* how many rows are in the vtrig buffer */
	instrument instrumentbuffer; /* instrument paste buffer */
	uint8_t    defvariantlength;

	channeldata channelbuffer; /* channel paste buffer */

	// uint8_t songibuffer[VARIANT_MAX]; /* song list paste buffer */
	// uint8_t songfbuffer[VARIANT_MAX]; /* song list flags paste buffer */
	// uint8_t songbufferlen;            /* how much of song[i,f]buffer has meaningful data */

	char filepath[COMMAND_LENGTH];

	void         (*filebrowserCallback)(char *); /* arg is the selected path */
	command_t      command;
	unsigned char  popup;
	unsigned char  mode, oldmode;
	unsigned short centre;
	uint8_t        pattern; /* focused pattern */
	uint8_t        channel; /* focused channel */

	uint16_t       trackerfy, visualfy;
	short          trackerfx, visualfx;
	uint8_t        visualchannel;
	short          instrumentindex;
	short          instrument;           /* focused instrument */
	unsigned short instrumentcelloffset;
	unsigned short instrumentrowoffset;

	int filebrowserindex;

	unsigned short mousey, mousex;

	short       fyoffset;
	signed char channeloffset;
	signed char fieldpointer;

	char           dirpath[NAME_MAX+1];
	unsigned int   dirc;
	unsigned short dirmaxwidth;
	unsigned char  dircols;
	DIR           *dir;

	Canvas  *waveformcanvas;
	char   **waveformbuffer;
	size_t   waveformw, waveformh;
	uint32_t waveformwidth;
	uint32_t waveformcursor;
	uint32_t waveformvisual;
	uint32_t waveformdrawpointer;

	char     chord; /* key chord buffer, vi-style multi-letter commands */
	uint16_t count; /* action repeat count, follows similar rules to w->chord */
	char     octave;
	uint8_t  step;
	char     keyboardmacro;
	uint8_t  flags; /* %1:follow */

	row     previewrow;
	uint8_t previewchannelsrc;
	channel previewchannel;
	char    previewtrigger; /* 0:cut
	                           1:start inst
	                           2:still inst */

	uint8_t instrumentlocki; /* realindex */
	uint8_t instrumentlockv; /* value, set to an INST_GLOBAL_LOCK constant */
	char    request;         /* ask the playback function to do something */

	uint8_t  instrumentreci; /* NOT a realindex */
	uint8_t  instrumentrecv; /* value, set to an INST_REC_LOCK constant */
	short   *recbuffer;      /* disallow changing the type or removing while recording */
	uint32_t recptr;

	char newfilename[COMMAND_LENGTH]; /* used by readSong */
} window;
window *w;


typedef struct
{
	jack_port_t *l;
	jack_port_t *r;
} portpair;

typedef struct
{
	sample_t *l;
	sample_t *r;
} portbufferpair;

typedef struct
{
	portbufferpair in;
	portbufferpair out;
	void *midiout;
} portbuffers;
portbuffers pb;

#define PLAY_LOCK_OK 0    /* p->s and p->w are safe */
#define PLAY_LOCK_START 1 /* p->s and p->w want to be unsafe */
#define PLAY_LOCK_CONT 2  /* p->s and p->w are unsafe */
typedef struct
{
	song        *s;
	window      *w;
	portpair     in, out;
	jack_port_t *midiout;
	char         dirty;
	char         lock; /* PLAY_LOCK_* defines */
} playbackinfo;
playbackinfo *p;
