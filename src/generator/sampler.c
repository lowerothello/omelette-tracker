/* downsampling approximation                 */
/* not perfectly accurate cos floats are used */
uint32_t calcDecimate(uint8_t decimate, uint32_t pointer)
{
	/* using a double here so the pointer should't be quantized by float bullshit too badly */
	double d = 1.0f + (1.0f - decimate*DIV255) * 10; /* TODO: use pow()? */
	return (uint32_t)(pointer/d)*d;
}

void getSample(uint32_t ptr, uint8_t decimate, Instrument *iv, short *output)
{
	ptr = calcDecimate(decimate, ptr);
	uint8_t shift = 15 - iv->bitdepth;
	if (ptr <= iv->sample.length * iv->sample.channels) *output += (iv->sample.data[ptr]>>shift)<<shift;
}

void getSampleLoopRamp(uint32_t ptr, uint32_t rptr, float lerp, uint8_t decimate, Instrument *iv, short *output)
{
	ptr =  calcDecimate(decimate, ptr);
	rptr = calcDecimate(decimate, rptr);
	uint8_t shift = 15 - iv->bitdepth;
	*output += ((iv->sample.data[ptr]>>shift)<<shift)*(1.0f - lerp) + ((iv->sample.data[rptr]>>shift)<<shift)*lerp;
}

#include "granular.c"
#include "wavetable.c"

#define ENVELOPE_A_STEP 0.02f
#define ENVELOPE_A_MIN  0.00f

#define ENVELOPE_D_STEP 0.13f
#define ENVELOPE_D_MIN  0.01f
/* returns true if the envelope has finished, sets cv->envgain to the gain multiplier */
bool envelope(Instrument *iv, Channel *cv, uint32_t pointer)
{
	uint16_t env = iv->envelope; if (cv->localenvelope != -1) env = cv->localenvelope;

	uint32_t a = (((env & 0xf000) >> 12)+ENVELOPE_A_MIN) * ENVELOPE_A_STEP * samplerate;
	uint32_t d = (((env & 0x0f00) >> 8 )+ENVELOPE_D_MIN) * ENVELOPE_D_STEP * samplerate;
	float    s =  ((env & 0x00f0) >> 4 ) * DIV15;
	uint32_t r =  ((env & 0x000f)       +ENVELOPE_D_MIN) * ENVELOPE_D_STEP * samplerate;

	if (cv->data.release)
	{
		if (r) cv->envgain = MAX(cv->envgain - (1.0f/r), 0.0f);
		else   cv->envgain = 0.0f;
	} else
	{
		if (pointer <= a)
		{
			if (a) cv->envgain = MIN(cv->envgain + (1.0f/a), 1.0f);
			else   cv->envgain = 1.0f;
		} else if (pointer < a+d) cv->envgain -= ((1.0f - s)/d);
		else                      cv->envgain = s;
	}

	if (pointer > a && cv->envgain < NOISE_GATE) return 1;
	return 0;
}

void minimalSamplerProcess(Sample *sample, uint32_t pointer, uint32_t pitchedpointer, short *l, short *r)
{
	float calcrate = (float)sample->rate / (float)samplerate;
	/* TODO: loop range, samplerate, bitdepth */
	if (pitchedpointer < sample->length * calcrate)
	{
		if (sample->channels == 1)
		{
			*l += sample->data[(uint32_t)(pitchedpointer*calcrate)];
			*r += sample->data[(uint32_t)(pitchedpointer*calcrate)];
		} else
		{
			*l += sample->data[(uint32_t)(pitchedpointer*calcrate) * sample->channels + 0];
			*r += sample->data[(uint32_t)(pitchedpointer*calcrate) * sample->channels + 1];
		}
	}
}

void samplerProcess(uint8_t realinst, Channel *cv, float rp, uint32_t pointer, uint32_t pitchedpointer, short *l, short *r)
{
	Instrument *iv = &p->s->instrument->v[realinst];
	if (!iv->sample.data || iv->algorithm == INST_ALG_MIDI) return; /* TODO: optimize midi further in process.c */

	/* return if the envelope has finished */
	if (envelope(iv, cv, pointer)) return;

	switch (iv->algorithm)
	{
		case INST_ALG_SIMPLE: minimalSamplerProcess(&iv->sample, pointer, pitchedpointer, l, r); break;
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
