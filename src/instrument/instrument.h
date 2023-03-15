typedef struct Sample
{ /* alloc(sizeof(Sample) + sizeof(short) * .length * .channels) */
	uint32_t length;
	uint8_t  channels;
	uint32_t rate;    /* rate to play C5 at */
	uint32_t defrate; /* rate to return to when the rate control is reset */
	bool     invert; /* TODO: implement */
	short    data[];
} Sample;

#define INSTRUMENT_VOID 255
#define INSTRUMENT_MAX 255

enum InstAlg
{
	INST_ALG_NULL,
	INST_ALG_SAMPLER,
	INST_ALG_MIDI,
};

struct InstAlgSampler
{
	enum InstAlg alg;
};

struct InstAlgMidi
{
	enum InstAlg alg;
	int8_t       channel;
};

union InstrumentUnion
{
	enum InstAlg          alg;
	struct InstAlgSampler sampler;
	struct InstAlgMidi    midi;
};

typedef struct Instrument
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
	uint8_t  frame;
	uint16_t envelope;
	int8_t   gain;
	bool     invert; /* DEPRECATED */
	bool     pingpong;
	uint8_t  loopramp;

	enum InstAlg algorithm;

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

int copyInstrument(uint8_t index, Instrument *src);

/* frees the contents of an instrument */
void delInstrumentForce(Instrument*);
int delInstrument(uint8_t index);

/* checks to see if an instrument is allocated and safe to use */
bool instrumentSafe(InstrumentChain*, short index);

/* take a Sample* and reparent it under Instrument .iv */
void reparentSample(Instrument*, Sample *sample);

void toggleRecording(uint8_t inst, char cue);

/* __ layer of abstraction for initializing instrumentbuffer */
void __addInstrument(Instrument*, int8_t algorithm);
InstrumentChain *_addInstrument(uint8_t index, int8_t algorithm);
int addInstrument(uint8_t index, int8_t algorithm, void (*cb)(Event*));

int addReparentInstrument(uint8_t index, int8_t algorithm, Sample *buffer);

/* returns -1 if no instrument slots are free */
short emptyInstrument(uint8_t min);

void yankInstrument(uint8_t index);
void putInstrument(size_t index);

Sample *_loadSample(char *path);
void loadSample(uint8_t index, char *path); /* TODO: atomicity */

/* TODO: sample could already be loaded into p->semarg, should reparent if so */
void sampleLoadCallback(char *path); /* TODO: atomicity */

void instrumentControlCallback(void); /* TODO: should be part of the tracker */

#include "input.h"
#include "waveform.h"
#include "draw.h"
