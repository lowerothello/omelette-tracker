typedef struct
{
	uint8_t type;
	void   *state;
} Effect;
#define EFFECT_CHAIN_LEN 16
typedef struct
{
	float  *input[2];
	float  *output[2];
	uint8_t c;
	Effect  v[];
} EffectChain;


typedef struct
{
	char    c; /* command  */
	uint8_t v; /* argument */
	bool    alt; /* true to use the altername command */
} Macro;
#define NOTE_VOID 255
#define NOTE_OFF 254
#define NOTE_UNUSED 253 /* explicitly unused */
#define NOTE_C5 60      /* centre note */
#define NOTE_A10 120    /* first out of range note */
#define INST_VOID 255
#define INST_FILEPREVIEW -1 /* signed, be careful with this */
typedef struct
{
	uint8_t note; /* MIDI compatible  | NOTE_* declares */
	uint8_t inst; /* instrument index | INST_* declares */
	Macro   macro[8];
} Row;

#define C_VTRIG_LOOP 0b00000001
typedef struct
{
	uint8_t index;
	uint8_t flags;
} Vtrig;

#define VARIANT_VOID 255
#define VARIANT_OFF 254
#define VARIANT_MAX 254
#define VARIANT_ROWMAX 255
typedef struct
{
	uint16_t rowc;
	Row      rowv[];
} Variant;

#define SONG_MAX 65535
#define STATE_ROWS 4

#define CHANNEL_MAX 32
typedef struct
{
	uint8_t  varianti[VARIANT_MAX]; /* variant index/backref   */
	Variant *variantv[VARIANT_MAX]; /* variant contents        */
	uint8_t  variantc; /*              variant contents length */

	Vtrig   *trig;  /* variant triggers */
	Variant *songv; /* main fallback variant  */

	/* flags */
	bool mute;
	bool reverse;
	bool release;
	bool rtrig_rev;

	uint8_t macroc; /* macro count */

	EffectChain *effect;
} ChannelData; /* raw sequence data */

typedef struct
{
	ChannelData data; /* saved to disk */

	/* runtime state */
	uint32_t pointer;        /* clock */
	uint32_t pitchedpointer; /* sample to play */

	/* gain */
	struct {
		uint8_t base; /* unsigned nibble per channel */
		uint8_t rand; /* base override for the altGxy macro */

		short   target;      /* smoothing target, committed to both rand and base */
		bool    target_rand; /* target should be commited to rand but NOT to base */
	} gain;

	struct {
		uint8_t base;
		uint8_t rand;

		short   target;
		bool    target_rand;
	} send;

	Row      r;
	uint8_t  samplernote;
	short    samplerinst; /* allowed to be signed */
	float    finetune;                 /* calculated fine tune, clamped between -2/+2 for midi */

	float    portamentofinetune;       /* portamento fine tune */
	float    targetportamentofinetune; /* cv->portamentofinetune destination */
	float    startportamentofinetune;  /* cv->portamentofinetune start       */
	uint32_t portamentosamples;        /* portamento length   */
	uint32_t portamentosamplepointer;  /* portamento progress */

	float    microtonalfinetune;         /* used by the local microtonal macro */
	uint16_t rtrigsamples;               /* samples per retrigger */
	uint32_t rtrigpointer;               /* clock reference */
	uint32_t rtrigcurrentpointer;        /* pointer the current rtrig started at */
	uint32_t rtrigpitchedpointer;        /* pitchedpointer to ratchet back to */
	uint32_t rtrigcurrentpitchedpointer; /* pitchedpointer the current retrig started at */
	int8_t   rtrigblocksize;             /* number of rows block extends to */
	uint16_t cutsamples;                 /* samples into the row to cut, 0 for no cut */
	uint16_t delaysamples;               /* samples into the row to delay, 0 for no delay */
	uint8_t  delaynote;
	uint8_t  delayinst;

	/* vibrato */
	uint8_t  vibrato;                    /* vibrato depth, 0-f */
	uint32_t vibratosamples;             /* samples per full phase walk */
	uint32_t vibratosamplepointer;       /* distance through cv->vibratosamples */


	short localenvelope;
	short localsustain;
	short localpitchshift, targetlocalpitchshift;
	short localpitchwidth, targetlocalpitchwidth;
	int   localcyclelength;
	short localsamplerate, targetlocalsamplerate;

	short   midiccindex;
	uint8_t midicc;

	/* filter */
	struct {
		SVFilter fl[2], fr[2];
		int8_t   mode[2], targetmode[2]; /* TODO: jitter variant? */

		/* cutoff */
		uint8_t  cut[2];
		uint8_t  randcut[2];
		short    targetcut[2];
		bool     targetcut_rand;

		/* resonance */
		uint8_t  res[2];
		uint8_t  randres[2];
		short    targetres[2];
		bool     targetres_rand;
	} filter;

	/* ramping */
	uint16_t rampindex;        /* progress through the ramp buffer, rampmax if not ramping */
	short   *rampbuffer;       /* samples to ramp out */
	uint8_t  rampinst;         /* real index, needed to determine the output group */

	/* sampler */
	float envgain;
	float modenvgain; /* wavetable modulation envelope */

	uint16_t grainrampindex; /* progress through the grain ramp buffer, >=cv->grainrampmax if not ramping */
	uint16_t grainrampmax;   /* actual grainrampmax used, to allow for tiny grain sizes */

	float *output[2];
	float *pluginoutput[2]; /* some external plugins need to read and write from separate buffers */
	float *mainmult[2]; /* apply post-effects in parallel with sendmult */
	float *sendmult[2]; /* apply post-effects in parallel with mainmult */

	uint32_t triggerflash;
} Channel;

typedef struct
{
	uint32_t length;
	uint8_t  channels;
	uint32_t rate; /* rate to play C5 at */
	uint32_t defrate; /* rate to return to when the rate control is reset */
	short    data[]; /* alloc(sizeof(short) * length * channels) */
} Sample;

#define INSTRUMENT_VOID 255
#define INSTRUMENT_MAX 255
enum {
	INST_ALG_SIMPLE,
	INST_ALG_GRANULAR,
	INST_ALG_WAVETABLE,
	INST_ALG_MIDI
} INST_ALG;

#define WT_PARAM_MAX 9
enum {
	WT_PARAM_WTPOS,
	WT_PARAM_GAIN,
	WT_PARAM_SYNC,
	WT_PARAM_FILTER, /* cutoff and resonance */
	WT_PARAM_PHASE,
	WT_PARAM_FREQUENCY,
	WT_PARAM_PULSEWIDTH,
	WT_PARAM_PHASEDYNAMICS,
	WT_PARAM_NOISE,
} WT_PARAM;
typedef struct
{
	Sample *sample;

	int8_t channelmode;

	/* quality */
	uint8_t  samplerate;  /* percent of c5rate to actually use */
	int8_t   bitdepth;
	bool     interpolate; /* lerp between samples */

	uint32_t trimstart;
	uint32_t trimlength;
	uint32_t looplength;
	uint16_t envelope;
	int8_t   gain;
	bool     invert;
	bool     pingpong;
	uint8_t  loopramp;

	int8_t   filtermode;
	uint8_t  filtercutoff;
	uint8_t  filterresonance;

	int8_t   algorithm;

	/* midi */
	struct {
		int8_t channel;
	} midi;

	/* granular */
	struct {
		uint16_t cyclelength;
		bool     reversegrains;
		int8_t   rampgrains;
		int16_t  timestretch;
		bool     notestretch;
		int16_t  pitchshift;
		int8_t   pitchstereo;
	} granular;

	/* wavetable */
	struct {
		uint32_t framelength;
		uint8_t  wtpos;
		int8_t   syncoffset;
		int8_t   pulsewidth;
		int8_t   phasedynamics;
		uint16_t envelope;
		uint8_t  lfospeed;
		int8_t   lfoduty;
		bool     lfoshape;
		struct {
			int8_t wtpos;
			int8_t sync;
			int8_t cutoff;
			int8_t phase;
			int8_t pwm;
			int8_t pdyn;
		} env;
		struct {
			uint8_t gain;
			int8_t wtpos;
			int8_t sync;
			int8_t cutoff;
			int8_t phase;
			int8_t pwm;
			int8_t pdyn;
		} lfo;
	} wavetable;

	/* effects */
	EffectChain *effect;
	float *output[2];       /* used by effects */
	float *pluginoutput[2]; /* some external plugins need to read and write from separate buffers */

	uint32_t triggerflash;
} Instrument;

typedef struct
{
	uint8_t    c;                 /* instrument count   */
	uint8_t    i[INSTRUMENT_MAX]; /* instrument backref */
	Instrument v[];               /* instrument values  */
} InstrumentChain;

typedef struct
{
	uint8_t c;   /* channel count  */
	Channel v[]; /* channel values */
} ChannelChain;

enum {
	PLAYING_STOP, /* TODO: some old code relies on this being 0, fix */
	PLAYING_START,
	PLAYING_CONT,
	PLAYING_PREP_STOP
} PLAYING;
typedef struct
{
	/* instruments */
	InstrumentChain *instrument;

	/* channels */
	ChannelChain *channel;
	short        *bpmcache; /* bpm change caching so multithreading isn't hell */
	uint16_t      bpmcachelen; /* how far into bpmcache it's safe to index */

	/* song pointers */
	uint16_t playfy;  /* analogous to window->trackerfy */
	uint16_t songlen; /* how long the global variant is */
	uint16_t loop[3]; /* loop range pointers, [2] is the staging loop end */

	/* mastering chain */
	/* uint8_t effectc;
	Effect *effectv; */

	/* misc. state */
	uint8_t  rowhighlight;
	uint8_t  songbpm;
	uint16_t spr;     /* samples per row (samplerate * (60 / bpm) / (rowhighlight * 2)) */
	uint16_t sprp;    /* samples per row progress */
	char     playing;
} Song;
Song *s;


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
	PAGE_CHANNEL_VARIANT,
	PAGE_CHANNEL_EFFECT,
	PAGE_CHANNEL_EFFECT_PLUGINBROWSER,
	PAGE_INSTRUMENT_SAMPLE,
	PAGE_INSTRUMENT_SAMPLE_PLUGINBROWSER,
	PAGE_INSTRUMENT_EFFECT,
	PAGE_INSTRUMENT_EFFECT_PLUGINBROWSER,
	PAGE_MASTER,
	PAGE_FILEBROWSER
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
	I_MODE_INDICES,
	I_MODE_NORMAL
} I_MODE;

enum {
	PTRIG_OK,     /* no queued preview            */
	PTRIG_NORMAL, /* queued s->instrument preview */
	PTRIG_FILE,   /* queued filebrowser preview   */
} PTRIG;

#define TRACKERFX_MIN -1
#define TRACKERFX_VISUAL_MIN 0
typedef struct
{
	Variant *pbvariantv[CHANNEL_MAX];
	Vtrig   *vbtrig    [CHANNEL_MAX];
	uint8_t  pbchannelc; /* how many channels are in the pattern buffer */
	int8_t   pbfx[2];    /* patternbuffer horizontal clipping region */
	Instrument instrumentbuffer; /* instrument paste buffer */
	uint8_t    defvariantlength;

	ChannelData channelbuffer; /* channel paste buffer */

	char filepath[COMMAND_LENGTH];

	void         (*filebrowserCallback)(char *); /* arg is the selected path */
	command_t      command;
	unsigned char  page;
	unsigned char  mode, oldmode;
	unsigned short centre;
	uint8_t        pattern;    /* focused pattern */
	uint8_t        channel;    /* focused channel */
	short          instrument; /* focused instrument, TODO: should be a uint8_t */

	uint16_t       trackerfy, visualfy;
	int8_t         trackerfx, visualfx;
	uint8_t        visualchannel;

	short          effectscroll;

	int           filebrowserindex;
	size_t        plugineffectindex;
	bool          pluginbrowserbefore; /* true to place plugins before the cursor, false to place plugins after the cursor */
	EffectChain **pluginbrowserchain;  /* which chain to place plugins into */

	unsigned short mousey, mousex;

	short       fyoffset;
	signed char channeloffset;
	signed char fieldpointer;

	char           dirpath[NAME_MAX+1];
	unsigned int   dirc;
	unsigned short dirmaxwidth;
	unsigned char  dircols;
	short dirx, diry, dirw, dirh; /* file browser screen region */
	DIR           *dir;

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
	Channel previewchannel;
	Sample *previewsample; /* used by the filebrowser to soft load samples */
	char    previewtrigger;

	uint8_t  instrumentreci; /* NOT a realindex */
	uint8_t  instrumentrecv; /* value, set to an INST_REC_LOCK constant */
	short   *recbuffer;      /* disallow removing an instrument while recording to it */
	uint32_t recptr;

	char newfilename[COMMAND_LENGTH]; /* used by readSong */
} Window;
Window *w;


typedef struct
{
	struct { jack_default_audio_sample_t *l, *r; } in, out;
	void *midiout;
} portbuffers;
portbuffers pb;

#define EVENT_QUEUE_MAX 16
typedef struct
{
	Song        *s;
	Window      *w;
	struct { jack_port_t *l, *r; } in, out;
	jack_port_t *midiout;
	bool         redraw; /* request a screen redraw */
	bool         resize; /* request a screen resize */
	Event        eventv[EVENT_QUEUE_MAX];
	uint8_t      eventc; /* the eventv index pushEvent() should populate */
} PlaybackInfo;
PlaybackInfo *p;
