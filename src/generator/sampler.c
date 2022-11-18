/* the threshold where processing is no longer necessary */
/* to avoid denormals and otherwise wasted cycles        */
#define NOISE_GATE 0.00001f

enum {
	SAMPLE_CHANNELS_STEREO,
	SAMPLE_CHANNELS_LEFT,
	SAMPLE_CHANNELS_RIGHT,
	SAMPLE_CHANNELS_MIX,
	SAMPLE_CHANNELS_SWAP,
} SAMPLE_CHANNELS;

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

/* clamps within range and loop, returns output samples */
void trimloop(double ptr, uint32_t length, uint32_t loop, Channel *cv, uint8_t decimate, Instrument *iv, uint8_t stereochannel, short *output)
{
	if (loop)
	{ /* if there is a loop range */
		if (iv->pingpong)
		{ /* ping-pong loop */
			if (ptr > length)
			{
				ptr = fmod(ptr - length, loop<<1);
				if (ptr > loop) /* walking forwards  */ ptr = (length - loop) + (ptr - loop);
				else            /* walking backwards */ ptr = length - ptr;
			}

			if (iv->sample->channels == 1) getSample(ptr+iv->trimstart, decimate, iv->bitdepth, iv->sample, output);
			else                           getSample((ptr+iv->trimstart)*iv->sample->channels + stereochannel, decimate, iv->bitdepth, iv->sample, output);
		} else
		{ /* crossfaded forwards loop */
			uint32_t looprampmax = MIN(samplerate*DIV1000 * LOOP_RAMP_MS, (loop>>1)) * iv->loopramp*DIV255;
			if (ptr > length) ptr = (length - loop) + looprampmax + fmod(ptr - length, loop - looprampmax);

			if (ptr > length - looprampmax)
			{
				float lerp = (ptr - length + looprampmax) / (float)looprampmax;
				float ramppointer = (ptr - (loop - looprampmax));
				if (iv->sample->channels == 1) getSampleLoopRamp( ptr+iv->trimstart, ramppointer, lerp, decimate, iv->bitdepth, iv->sample, output);
				else                           getSampleLoopRamp((ptr+iv->trimstart)*iv->sample->channels + stereochannel,
				                                                         ramppointer*iv->sample->channels + stereochannel, lerp, decimate, iv->bitdepth, iv->sample, output);
			} else
			{
				if (iv->sample->channels == 1) getSample( ptr+iv->trimstart, decimate, iv->bitdepth, iv->sample, output);
				else                           getSample((ptr+iv->trimstart)*iv->sample->channels + stereochannel, decimate, iv->bitdepth, iv->sample, output);
			}
		}
	} else
	{
		if (ptr < length)
		{
			if (iv->sample->channels == 1) getSample( ptr+iv->trimstart, decimate, iv->bitdepth, iv->sample, output);
			else                           getSample((ptr+iv->trimstart)*iv->sample->channels + stereochannel, decimate, iv->bitdepth, iv->sample, output);
		}
	}
}

float semitoneShortToMultiplier(int16_t input)
{
	if (input < 0) return powf(M_12_ROOT_2, -((abs(input)>>12)*12 + (abs(input)&0x0fff)*DIV256));
	else           return powf(M_12_ROOT_2, (input>>12)*12 + (input&0x0fff)*DIV256);
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

#include "minimal.c"
#include "midi.c"
#include "cyclic.c"
#include "tonal.c"
#include "beat.c"
#include "wavetable.c"

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
			processMinimal(iv->sample, pitchedpointer, localsamplerate, iv->bitdepth, cv->samplernote, l, r);
			break;
		case INST_ALG_CYCLIC:    processCyclic   (iv, cv, rp, pointer, pitchedpointer, l, r); break;
		case INST_ALG_WAVETABLE: processWavetable(iv, cv, rp, pointer, pitchedpointer, l, r); break;
	}

	short hold;
	switch (iv->channelmode)
	{
		case SAMPLE_CHANNELS_STEREO: break;
		case SAMPLE_CHANNELS_LEFT:   *r = *l; break;
		case SAMPLE_CHANNELS_RIGHT:  *l = *r; break;
		case SAMPLE_CHANNELS_MIX:    *l = *r = ((*l>>1) + (*r>>1)); break;
		case SAMPLE_CHANNELS_SWAP:   hold = *l; *l = *r; *r = hold; break;
	}

	if (iv->invert) { *l *= -1; *r *= -1; }
	*l *= cv->envgain;
	*r *= cv->envgain;
}
