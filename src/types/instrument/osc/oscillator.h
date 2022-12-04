#include "phase.h"

enum OSC_TYPE {
	OSC_TYPE_SAMPLE,    /* sampler   */
	OSC_TYPE_WAVETABLE, /* wavetable */
	OSC_TYPE_UNION,     /* union     */
	OSC_TYPE_SUPER,     /* super     */
	OSC_TYPE_FMBANK,    /* fmbank    */
};

#include "sample.h"
#include "wavetable.h"
#include "union.h"
#include "super.h"
#include "fmbank.h"

/* using a union to determine the size */
union Oscillator {
	WavetableOsc wavetable;
	UnionOsc     unionosc; /* "union" is a reserved word */
	SuperOsc     super;
	FmbankOsc    fmbank;
};


/* should be copiable with a single memcpy() */
typedef struct {
	Sample *sample;
	uint32_t clock;

	enum OSC_TYPE osc_type[2];
	union Oscillator osc  [2];

	/* TODO: modulation: lfos, envelopes, noise source, matrix, etc */
	Lfo lfo[4];
	Envelope envelope[4];

	uint8_t f_cutoff;
	uint8_t f_resonance;
	int8_t  f_mode; /* TODO: should be an enum */

	uint8_t gain;
	bool release; /* sound is releasing */

	uint32_t triggerflash;
} Voice;
