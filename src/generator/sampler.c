/* downsampling approximation                 */
/* not perfectly accurate cos floats are used */
uint32_t calcDecimate(uint8_t decimate, uint32_t pointer)
{
	/* using a double here so the pointer should't be quantized by float bullshit too badly */
	/* TODO: using integer maths would be far better to avoid precision issues */
	double d = 1.0 + (1.0 - decimate*DIV255) * 10.0; /* TODO: scale exponentially? (more precision at the top end) */
	return (uint32_t)(pointer/d)*d;
}

void getSample(uint32_t ptr, uint8_t decimate, int8_t bitdepth, Sample *s, short *output)
{
	ptr = calcDecimate(decimate, ptr);
	uint8_t shift = 0xf - bitdepth;
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
void trimloop(double ptr, uint32_t length, uint32_t loop, Track *cv, uint8_t decimate, Instrument *iv, uint8_t stereotrack, short *output)
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
			else                           getSample((ptr+iv->trimstart)*iv->sample->channels + stereotrack, decimate, iv->bitdepth, iv->sample, output);
		} else
		{ /* crossfaded forwards loop */
			uint32_t looprampmax = MIN(samplerate*DIV1000 * LOOP_RAMP_MS, (loop>>1)) * iv->loopramp*DIV255;
			if (ptr > length) ptr = (length - loop) + looprampmax + fmod(ptr - length, loop - looprampmax);

			if (ptr > length - looprampmax)
			{
				float lerp = (ptr - length + looprampmax) / (float)looprampmax;
				float ramppointer = (ptr - (loop - looprampmax));
				if (iv->sample->channels == 1) getSampleLoopRamp( ptr+iv->trimstart, ramppointer, lerp, decimate, iv->bitdepth, iv->sample, output);
				else                           getSampleLoopRamp((ptr+iv->trimstart)*iv->sample->channels + stereotrack, ramppointer*iv->sample->channels + stereotrack, lerp, decimate, iv->bitdepth, iv->sample, output);
			} else
			{
				if (iv->sample->channels == 1) getSample( ptr+iv->trimstart, decimate, iv->bitdepth, iv->sample, output);
				else                           getSample((ptr+iv->trimstart)*iv->sample->channels + stereotrack, decimate, iv->bitdepth, iv->sample, output);
			}
		}
	} else
	{
		if (ptr < length)
		{
			if (iv->sample->channels == 1) getSample( ptr+iv->trimstart, decimate, iv->bitdepth, iv->sample, output);
			else                           getSample((ptr+iv->trimstart)*iv->sample->channels + stereotrack, decimate, iv->bitdepth, iv->sample, output);
		}
	}
}

float semitoneShortToMultiplier(int16_t input)
{
	if (input < 0) return powf(M_12_ROOT_2, -((abs(input)>>12)*12 + (abs(input)&0x0fff)*DIV256));
	else           return powf(M_12_ROOT_2, (input>>12)*12 + (input&0x0fff)*DIV256);
}

void samplerProcess(uint8_t realinst, Track *cv, float rp, uint32_t pointer, uint32_t pitchedpointer, short *l, short *r)
{
	Instrument *iv = &p->s->instrument->v[realinst];
	if (!iv->sample->length || iv->algorithm == INST_ALG_MIDI) return; /* TODO: optimize midi further in process.c */

	Envelope env;
	env.adsr = iv->envelope; if (cv->localenvelope != -1) env.adsr = cv->localenvelope;
	applyEnvelopeControlChanges(&env);
	env.output = cv->envgain;
	env.release = cv->data.release;
	env.pointer = pointer;
	/* return if the envelope has finished */
	if (envelope(&env)) return;
	cv->envgain = env.output;

	uint8_t localsamplerate;
	switch (iv->algorithm)
	{
		case INST_ALG_SIMPLE:
			localsamplerate = iv->samplerate; if (cv->localsamplerate != -1) localsamplerate = cv->localsamplerate;
			if (cv->targetlocalsamplerate != -1) localsamplerate += (cv->targetlocalsamplerate - localsamplerate) * rp;
			processMinimal(iv->sample, pitchedpointer, localsamplerate, iv->bitdepth, cv->samplernote, l, r);
			break;
		case INST_ALG_CYCLIC: processCyclic(iv, cv, rp, pointer, pitchedpointer, l, r); break;
		// case INST_ALG_WAVETABLE: processWavetable(iv, cv, rp, pointer, pitchedpointer, l, r); break;
		case INST_ALG_MIDI: break;
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
