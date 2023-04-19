typedef enum FilterMode
{
	FILTER_MODE_LP12 = 0,
	FILTER_MODE_HP12 = 1,
	FILTER_MODE_BP12 = 2,
	FILTER_MODE_NT12 = 3,
	FILTER_MODE_LP24 = 4,
	FILTER_MODE_HP24 = 5,
	FILTER_MODE_BP24 = 6,
	FILTER_MODE_NT24 = 7,
} FilterMode;


void getSample(double ptr, bool interpolate, uint8_t decimate, int8_t bitdepth, Sample *s, short *output);
void getSampleLoopRamp(double ptr, double rptr, float lerp, bool interpolate, uint8_t decimate, int8_t bitdepth, Sample *s, short *output);

float semitoneShortToMultiplier(int16_t input);

#include "minimal.c" /* TODO: should probably remove tbh */
#include "midi.c"
#include "sampler.c"
