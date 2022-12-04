enum {
	SAMPLE_CHANNELS_STEREO,
	SAMPLE_CHANNELS_LEFT,
	SAMPLE_CHANNELS_RIGHT,
	SAMPLE_CHANNELS_MIX,
	SAMPLE_CHANNELS_SWAP,
} SAMPLE_CHANNELS;


/* downsampling approximation                 */
/* not perfectly accurate cos floats are used */
uint32_t calcDecimate(uint8_t decimate, uint32_t pointer);

void getSample(uint32_t ptr, uint8_t decimate, int8_t bitdepth, Sample *s, short *output);
void getSampleLoopRamp(uint32_t ptr, uint32_t rptr, float lerp, uint8_t decimate, int8_t bitdepth, Sample *s, short *output);

/* clamps within range and loop, returns output samples */
void trimloop(double ptr, uint32_t length, uint32_t loop, Track *cv, uint8_t decimate, Instrument *iv, uint8_t stereotrack, short *output);

float semitoneShortToMultiplier(int16_t input);

void drawInstrumentSampler(ControlState *cc, Instrument *iv, short x);

void samplerProcess(uint8_t realinst, Track *cv, float rp, uint32_t pointer, uint32_t pitchedpointer, short *l, short *r);

#include "minimal.c"
#include "midi.c"
#include "cyclic.c"
#include "tonal.c"
#include "beat.c"
#include "wavetable.c"

#include "sampler.c"
