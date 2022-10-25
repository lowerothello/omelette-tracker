/* downsampling approximation                 */
/* not perfectly accurate cos floats are used */
uint32_t calcDecimate(uint8_t decimate, uint32_t pointer)
{
	/* using a double here so the pointer should't be quantized by float bullshit too badly */
	double d = 1.0f + (1.0f - decimate*DIV255) * 10; /* TODO: scale exponentially? (more precision at the top end) */
	return (uint32_t)(pointer/d)*d;
}

void getSample(uint32_t ptr, uint8_t decimate, int8_t bitdepth, Sample *s, short *output)
{
	ptr = calcDecimate(decimate, ptr);
	uint8_t shift = 0xf - bitdepth;
	// if (ptr <= s->length * s->channels) *output += (s->data[ptr]>>shift)<<shift;
	if (ptr <= (s->length-1) * s->channels) *output += (s->data[ptr]>>shift)<<shift;
}

void getSampleLoopRamp(uint32_t ptr, uint32_t rptr, float lerp, uint8_t decimate, int8_t bitdepth, Sample *s, short *output)
{
	ptr =  calcDecimate(decimate, ptr);
	rptr = calcDecimate(decimate, rptr);
	uint8_t shift = 0xf - bitdepth;
	*output += ((s->data[ptr]>>shift)<<shift)*(1.0f - lerp) + ((s->data[rptr]>>shift)<<shift)*lerp;
}

#define ENVELOPE_A_STEP 0.02f
#define ENVELOPE_A_MIN  0.00f

#define ENVELOPE_D_STEP 0.13f
#define ENVELOPE_D_MIN  0.01f
/* returns true if the envelope has finished, sets cv->envgain to the gain multiplier */
bool envelope(uint16_t env, Channel *cv, uint32_t pointer, float *envgain)
{
	uint32_t a = (((env & 0xf000) >> 12)+ENVELOPE_A_MIN) * ENVELOPE_A_STEP * samplerate;
	uint32_t d = (((env & 0x0f00) >> 8 )+ENVELOPE_D_MIN) * ENVELOPE_D_STEP * samplerate;
	float    s =  ((env & 0x00f0) >> 4 ) * DIV15;
	uint32_t r =  ((env & 0x000f)       +ENVELOPE_D_MIN) * ENVELOPE_D_STEP * samplerate;

	if (cv->data.release)
	{
		if (r) *envgain = MAX(*envgain - (1.0f/r), 0.0f);
		else   *envgain = 0.0f;
	} else
	{
		if (pointer <= a)
		{
			if (a) *envgain = MIN(*envgain + (1.0f/a), 1.0f);
			else   *envgain = 1.0f;
		} else if (pointer < a+d) *envgain -= ((1.0f - s)/d);
		else                      *envgain = s;
	}

	if (pointer > a && *envgain < NOISE_GATE) return 1;
	return 0;
}

#include "granular.c"
#include "wavetable.c" /* needs to be after declaring envelope() */

void minimalSamplerProcess(Sample *sample, uint32_t pointer, uint8_t decimate, int8_t bitdepth, short note, short *l, short *r)
{
	float calcrate = (float)sample->rate / (float)samplerate;
	float calcpitch = powf(M_12_ROOT_2, note - NOTE_C5);

	if (sample->channels == 1)
	{
		getSample(pointer*calcrate*calcpitch, decimate, bitdepth, sample, l);
		getSample(pointer*calcrate*calcpitch, decimate, bitdepth, sample, r);
	} else
	{
		getSample(pointer*calcrate*calcpitch * sample->channels + 0, decimate, bitdepth, sample, l);
		getSample(pointer*calcrate*calcpitch * sample->channels + 1, decimate, bitdepth, sample, r);
	}
}

void samplerProcess(uint8_t realinst, Channel *cv, float rp, uint32_t pointer, uint32_t pitchedpointer, short *l, short *r)
{
	Instrument *iv = &p->s->instrument->v[realinst];
	if (!iv->sample->length || iv->algorithm == INST_ALG_MIDI) return; /* TODO: optimize midi further in process.c */

	uint16_t env = iv->envelope; if (cv->localenvelope != -1) env = cv->localenvelope;
	/* return if the envelope has finished */
	if (envelope(env, cv, pointer, &cv->envgain)) return;

	uint8_t localsamplerate;
	switch (iv->algorithm)
	{
		case INST_ALG_SIMPLE:
			localsamplerate = iv->samplerate; if (cv->localsamplerate != -1) localsamplerate = cv->localsamplerate;
			if (cv->targetlocalsamplerate != -1) localsamplerate += (cv->targetlocalsamplerate - localsamplerate) * rp;
			minimalSamplerProcess(iv->sample, pitchedpointer, localsamplerate, iv->bitdepth, cv->samplernote, l, r);
			break;
		case INST_ALG_GRANULAR: granularProcess(iv, cv, rp, pointer, pitchedpointer, l, r); break;
		case INST_ALG_WAVETABLE: wavetableProcess(iv, cv, rp, pointer, pitchedpointer, l, r); break;
	}

	short hold;
	switch (iv->channelmode)
	{
		case 1: *r = *l; break;                       /* mono left    */
		case 2: *l = *r; break;                       /* mono right   */
		case 3: *l = *r = ((*l>>1) + (*r>>1)); break; /* mono mix     */
		case 4: hold = *l; *l = *r; *r = hold; break; /* channel swap */
	}

	if (iv->invert) { *l *= -1; *r *= -1; }
	*l *= cv->envgain;
	*r *= cv->envgain;
}
