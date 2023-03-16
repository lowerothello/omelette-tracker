#define SONG_MAX 65535
#define STATE_ROWS 4

typedef struct MacroState
{
	short   base;             /* unsigned nibble per track                            */
	short   rand;             /* .base override for jitter                            */
	short   target;           /* smoothing target, committed to both .rand and .base  */
	uint8_t lfospeed;         /* how many lfo cycles per row                          */
	unsigned target_rand : 1; /* .target should be commited to .rand but NOT to .base */
	unsigned lfo_stereo  : 1; /* .lfospeed should be stereo                           */
} MacroState;

typedef struct Track
{
	/* flags */
	bool mute;
	unsigned reverse : 1;
	unsigned release : 1;
	unsigned file    : 1;

	VariantChain *variant;
	EffectChain *effect;

	/* runtime state */
	uint32_t pointer;        /* clock */
	uint32_t pitchedpointer; /* tuned clock */

	/* gain */
	MacroState gain;
	MacroState send;

	Row      r;
	float    finetune; /* calculated fine tune, TODO: remove */

	short localenvelope;
	short localsustain;
	MacroState pitchshift;
	MacroState pitchwidth;
	MacroState samplerate;
	int localcyclelength;

	short   midiccindex;
	uint8_t midicc;

	/* ramping */
	uint16_t rampindex;  /* progress through the ramp buffer, rampmax if not ramping */
	float   *rampbuffer; /* samples to ramp out */

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
} Track; /* cv */

#define TRACK_MAX 32
typedef struct {
	uint8_t c; /* track count  */
	Track **v; /* track values */
} TrackChain;


void regenGlobalRowc(struct Song *cs);

/* clears the playback state of a track */
void clearTrackRuntime(Track *cv);

void initTrackData(Track *cv, uint16_t songlen); /* TODO: should be atomic */
void clearTrackData(Track *cv, uint16_t songlen);
void addTrackRuntime(Track *cv);
void addTrackData(Track *cv, uint16_t songlen);
void debug_dumpTrackState(struct Song *cs);

/* copyfrom can be NULL */
/* .cs can be NULL */
Track *allocTrack(struct Song *cs, Track *copyfrom);

/* copyfrom can be NULL */
void addTrack(struct Song *cs, uint8_t index, uint16_t count, Track *copyfrom);

/* .cs can be NULL */
void _delTrack(struct Song *cs, Track *cv);

void delTrack(uint8_t index, uint16_t count);
void copyTrack(Track *dest, Track *src); /* NOT atomic */
Row *getTrackRow(Track *cv, uint16_t index);
void regenBpmCache(struct Song *cs);
void regenGlobalRowc(struct Song *cs);
void cycleVariantUp(Variant *v, uint16_t bound);
void cycleVariantDown(Variant *v, uint16_t bound);

void applyTrackMutes(void);
void toggleTrackMute(uint8_t track);
void toggleTrackSolo(uint8_t track);
