#define SONG_MAX 65535
#define STATE_ROWS 4

#define CHANNEL_MAX 32
typedef struct {
	VariantChain *variant;

	/* flags */
	bool mute;
	bool reverse;
	bool release;
	bool rtrig_rev;

	EffectChain *effect;
} ChannelData; /* raw sequence data */

typedef struct {
	ChannelData data; /* saved to disk */

	/* runtime state */
	uint32_t pointer;        /* clock */
	uint32_t pitchedpointer; /* tuned clock */

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
	short    samplerinst; /* signed for special instruments */
	float    finetune;    /* calculated fine tune */

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
	uint8_t  vibrato;              /* vibrato depth, 0-f */
	uint32_t vibratosamples;       /* samples per full phase walk */
	uint32_t vibratosamplepointer; /* distance through cv->vibratosamples */


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

typedef struct {
	uint8_t c;   /* channel count  */
	Channel v[]; /* channel values */
} ChannelChain;


/* TODO: holy shit this api is ugly, refine and document */

void regenGlobalRowc(struct _Song *cs);

/* clears the playback state of a channel */
void clearChannelRuntime(Channel *cv);

void initChannelData(struct _Song *cs, ChannelData *cd); /* TODO: should be atomic */
void clearChanneldata(struct _Song *cs, ChannelData *cd); /* TODO: should be atomic */
void __addChannel(Channel *cv); /* __ layer of abstraction for initializing previewchannel */
void  _addChannel(struct _Song *cs, Channel *cv);
void debug_dumpChannelState(struct _Song *cs);
int addChannel(struct _Song *cs, uint8_t index, uint16_t count);
void _delChannel(struct _Song *cs, Channel *cv);
void delChannel(uint8_t index, uint16_t count);
void copyChanneldata(ChannelData *dest, ChannelData *src); /* TODO: atomicity */
Row *getChannelRow(ChannelData *cd, uint16_t index);
char checkBpmCache(jack_nframes_t fptr, uint16_t *spr, int m, Channel *cv, Row r);
void regenBpmCache(struct _Song *cs);
void regenGlobalRowc(struct _Song *cs);
void cycleVariantUp(Variant *v, uint16_t bound);
void cycleVariantDown(Variant *v, uint16_t bound);
void serializeChannel(struct _Song *cs, Channel *cv, FILE *fp);
void deserializeChannel(struct _Song *cs, Channel *cv, FILE *fp, uint8_t major, uint8_t minor);

/* TODO: should be declared in macros.h */
char ifMacro(jack_nframes_t fptr, uint16_t *spr, Channel *cv, Row r, char m, bool alt, char (*callback)(jack_nframes_t, uint16_t *, int, Channel *, Row));
