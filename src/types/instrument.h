typedef struct { /* alloc(sizeof(Sample) + sizeof(short) * .length * .tracks) */
	uint32_t length;
	uint8_t  tracks;
	uint32_t rate; /* rate to play C5 at */
	uint32_t defrate; /* rate to return to when the rate control is reset */
	short    data[];
} Sample;

#define INSTRUMENT_VOID 255
#define INSTRUMENT_MAX 255
enum {
	INST_ALG_SIMPLE,
	INST_ALG_CYCLIC,
	INST_ALG_TONAL,
	INST_ALG_BEAT,
	INST_ALG_WAVETABLE,
	INST_ALG_MIDI,
} INST_ALG;

enum {
	WT_PARAM_WTPOS,
	WT_PARAM_GAIN,
	WT_PARAM_SYNC,
	WT_PARAM_FILTER, /* cutoff and resonance */
	WT_PARAM_PHASE,
	WT_PARAM_FREQUENCY,
	WT_PARAM_PULSEWIDTH,
	WT_PARAM_PHASEDYNAMICS,
	WT_PARAM_MIXER,
} WT_PARAM;
enum {
	SCALE_MAJ,
	SCALE_MIN,
	SCALE_CHR,
} SCALE;
typedef struct {
	Sample *sample;

	int8_t trackmode;

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
		uint8_t  cyclelengthjitter;
		uint8_t  reversegrains;
		int8_t   rampgrains;
		int16_t  timestretch;
		bool     notestretch;
		int16_t  pitchshift;
		int16_t  formantshift;
		int8_t   pitchstereo;
		uint8_t  pitchjitter;
		uint8_t  formantjitter;
		int8_t   pitchoctaverange;
		int8_t   pitchoctavechance;
		uint8_t  panjitter;
		uint8_t  ptrjitter;
		int16_t  autotune;
		int8_t   autoscale;
		uint8_t  autospeed;
		uint8_t  autostrength;
		uint8_t  beatsensitivity;
		uint8_t  beatdecay;
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

	uint32_t triggerflash;
} Instrument;

typedef struct {
	uint8_t    c;                 /* instrument count   */
	uint8_t    i[INSTRUMENT_MAX]; /* instrument backref */
	Instrument v[];               /* instrument values  */
} InstrumentChain;

void copyInstrument(Instrument *dest, Instrument *src); /* TODO: should be atomic */

/* frees the contents of an instrument */
void _delInstrument(Instrument *iv);
bool instrumentSafe(struct _Song *cs, short index);

/* take a Sample* and reparent it under instrument iv */
void reparentSample(Instrument *iv, Sample *sample);

/* TODO: use this function */
Sample *applySampleEffects(Instrument *iv, Sample *sample);

void toggleRecording(uint8_t inst, char cue);

/* __ layer of abstraction for initializing instrumentbuffer */
void __addInstrument(Instrument *iv, int8_t algorithm);
InstrumentChain *_addInstrument(uint8_t index, int8_t algorithm);
int addInstrument(uint8_t index, int8_t algorithm, void (*cb)(Event *));

int addReparentInstrument(uint8_t index, int8_t algorithm, Sample *buffer);

/* returns -1 if no instrument slots are free */
short emptyInstrument(uint8_t min);

void yankInstrument(uint8_t index);
void putInstrument (uint8_t index);

int delInstrument(uint8_t index);

Sample *_loadSample(char *path);
void loadSample(uint8_t index, char *path); /* TODO: atomicity */

void serializeInstrument  (Instrument *iv, FILE *fp);
void deserializeInstrument(Instrument *iv, FILE *fp, double ratemultiplier, uint8_t major, uint8_t minor);
