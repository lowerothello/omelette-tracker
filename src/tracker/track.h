#define SONG_MAX 65535
#define STATE_ROWS 4

typedef struct MacroState
{
	short    base;            /* unsigned nibble per track                            */
	short    rand;            /* .base override for jitter                            */
	short    target;          /* smoothing target, committed to both .rand and .base  */
	uint8_t  lfospeed;        /* how many lfo cycles per row                          */
	unsigned target_rand : 1; /* .target should be commited to .rand but NOT to .base */
	unsigned lfo_stereo  : 1; /* .lfospeed should be stereo                           */
} MacroState;

#define NAME_LEN 32
typedef struct Track
{
	char    name[NAME_LEN + 1];
	uint8_t volume;
	int8_t  panning;
	int8_t  transpose;
	int8_t  patternlengthscale;

	bool mute;
	VariantChain *variant;
	EffectChain  *effect;


	/* runtime */
	unsigned reverse : 1;
	unsigned release : 1;
	unsigned file    : 1;

	void **macrostate;
	void  *inststate;

	uint32_t pointer; /* clock */

	MacroState gain;
	MacroState send;

	Row r;

	uint16_t rampindex;  /* progress through the ramp buffer, rampmax if not ramping */
	float   *rampbuffer; /* samples to ramp out */

	float *mainmult[2]; /* stereo gain multiplier */
	float *sendmult[2]; /* stereo gain multiplier */

	uint32_t triggerflash;
} Track; /* cv */

#define TRACK_MAX 32
typedef struct TrackChain
{
	uint8_t c; /* track count  */
	Track **v; /* track values */
} TrackChain;


void regenGlobalRowc(struct Song *cs);

/* clears the playback state of a track */
void clearTrackRuntime(Track *cv);

void initTrackData(Track *cv, uint16_t songlen); /* TODO: should be atomic */
void clearTrackData(Track *cv, uint16_t songlen);
void addTrackRuntime(Track *cv);
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

struct json_object *serializeTrackChain(TrackChain*);
TrackChain *deserializeTrackChain(struct json_object*);
