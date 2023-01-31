enum SAMPLE_CHANNELS {
	SAMPLE_CHANNELS_STEREO,
	SAMPLE_CHANNELS_LEFT,
	SAMPLE_CHANNELS_RIGHT,
	SAMPLE_CHANNELS_MIX,
	SAMPLE_CHANNELS_SWAP,
};

enum FILTER_MODE {
	FILTER_MODE_LP12 = 0,
	FILTER_MODE_HP12 = 1,
	FILTER_MODE_BP12 = 2,
	FILTER_MODE_NT12 = 3,
	FILTER_MODE_LP24 = 4,
	FILTER_MODE_HP24 = 5,
	FILTER_MODE_BP24 = 6,
	FILTER_MODE_NT24 = 7,
};


/* downsampling approximation                 */
/* not perfectly accurate cos floats are used */
uint32_t calcDecimate(uint8_t decimate, uint32_t pointer);

void getSample(uint32_t ptr, uint8_t decimate, int8_t bitdepth, Sample *s, short *output);
void getSampleLoopRamp(uint32_t ptr, uint32_t rptr, float lerp, uint8_t decimate, int8_t bitdepth, Sample *s, short *output);

/* clamps within range and loop, returns output samples */
void trimloop(double ptr, uint32_t length, uint32_t loop, Track *cv, uint8_t decimate, Instrument *iv, uint8_t stereotrack, short *output);

float semitoneShortToMultiplier(int16_t input);

void drawInstrumentSampler(ControlState *cc, Instrument *iv, short x, short w);

void samplerProcess(uint8_t realinst, Track *cv, float rp, uint32_t pointer, uint32_t pitchedpointer, short *l, short *r);

#include "minimal.c"
#include "midi.c"
#include "cyclic.c"
#include "wavetable.c"

#include "sampler.c"
