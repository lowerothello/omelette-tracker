/* downsampling approximation                 */
/* not perfectly accurate cos floats are used */
uint32_t calcDecimate(Instrument *iv, uint8_t decimate, uint32_t pointer)
{
	/* using a double here so the pointer should't be quantized by float bullshit too badly */
	double d = 1.0f + (1.0f - decimate*DIV255) * 10; /* TODO: use pow()? */
	return (uint32_t)(pointer/d)*d;
}

void getSample(uint32_t ptr, Instrument *iv, short *output)
{
	ptr = calcDecimate(iv, iv->samplerate, ptr);
	uint8_t shift = 15 - iv->bitdepth;
	*output += (iv->sampledata[ptr]>>shift)<<shift;
}

void getSampleLoopRamp(uint32_t ptr, uint32_t rptr, float lerp, Instrument *iv, short *output)
{
	ptr = calcDecimate(iv, iv->samplerate, ptr);
	rptr = calcDecimate(iv, iv->samplerate, rptr);
	uint8_t shift = 15 - iv->bitdepth;
	*output += ((iv->sampledata[ptr]>>shift)<<shift)*(1.0f - lerp) + ((iv->sampledata[rptr]>>shift)<<shift)*lerp;
}

/* clamps within range and loop, returns output samples */
void trimloop(uint32_t ptr, Channel *cv, Instrument *iv, uint8_t stereochannel, short *output)
{
	if (iv->looplength)
	{ /* if there is a loop range */
		if (iv->pingpong)
		{ /* ping-pong loop */
			if (ptr > iv->trimlength)
			{
				ptr = (ptr - iv->trimlength)%(iv->looplength<<1);
				if (ptr > iv->looplength) /* walking forwards  */ ptr = (iv->trimlength - iv->looplength) + (ptr - iv->looplength);
				else                      /* walking backwards */ ptr = iv->trimlength - ptr;
			}

			if (iv->channels == 1) getSample(ptr+iv->trimstart, iv, output);
			else                   getSample((ptr+iv->trimstart)*iv->channels + stereochannel, iv, output);
		} else
		{ /* crossfaded forwards loop */
			uint32_t looprampmax = MIN(samplerate*DIV1000 * LOOP_RAMP_MS, (iv->looplength>>1)) * (iv->loopramp*DIV255);
			if (ptr > iv->trimlength) ptr = (iv->trimlength - iv->looplength) + looprampmax + (ptr - iv->trimlength)%(iv->looplength - looprampmax);

			if (ptr > iv->trimlength - looprampmax)
			{
				float lerp = (ptr - iv->trimlength + looprampmax) / (float)looprampmax;
				float ramppointer = (ptr - (iv->looplength - looprampmax));
				if (iv->channels == 1) getSampleLoopRamp( ptr+iv->trimstart, ramppointer, lerp, iv, output);
				else                   getSampleLoopRamp((ptr+iv->trimstart)*iv->channels + stereochannel,
				                                                 ramppointer*iv->channels + stereochannel, lerp, iv, output);
			} else
			{
				if (iv->channels == 1) getSample( ptr+iv->trimstart, iv, output);
				else                   getSample((ptr+iv->trimstart)*iv->channels + stereochannel, iv, output);
			}
		}
	} else
	{
		if (ptr < iv->trimlength)
		{
			if (iv->channels == 1) getSample( ptr+iv->trimstart, iv, output);
			else                   getSample((ptr+iv->trimstart)*iv->channels + stereochannel, iv, output);
		}
	}
}

/* returns true if the envelope has finished, sets cv->envgain to the gain multiplier */
bool envelope(Instrument *iv, Channel *cv, uint32_t pointer)
{
	uint8_t env = iv->envelope; if (cv->localenvelope != -1) env = cv->localenvelope;

	uint32_t alen = ((env>>4)+ENVELOPE_A_MIN) * ENVELOPE_A_STEP * samplerate;
	uint32_t dlen = ((env%16)+ENVELOPE_D_MIN) * ENVELOPE_D_STEP * samplerate;

	if (cv->data.release || (!iv->sustain && pointer > alen))
	     { if (dlen) cv->envgain = MAX(cv->envgain - (1.0f/dlen), 0.0f); else cv->envgain = 0.0f; }
	else { if (alen) cv->envgain = MIN(cv->envgain + (1.0f/alen), 1.0f); else cv->envgain = 1.0f; }

	if (pointer > alen && cv->envgain < NOISE_GATE) return 1;
	return 0;
}

void samplerProcess(uint8_t realinst, Channel *cv, uint32_t pointer, uint32_t pitchedpointer, short *l, short *r)
{
	Instrument *iv = &p->s->instrument->v[realinst];
	if (!iv->sampledata) return;

	/* return if the envelope has finished */
	if (envelope(iv, cv, pointer)) return;

	float gain;

	uint16_t localcyclelength = iv->cyclelength; if (cv->localcyclelength != -1) localcyclelength = cv->localcyclelength;
	int16_t localpitchshift = iv->pitchshift; if (cv->localpitchshift != -1) localpitchshift = (cv->localpitchshift - 0x80)<<8;

	uint16_t cyclelength = MAX(1, samplerate*DIV1000 * TIMESTRETCH_CYCLE_UNIT_MS * MAX(1, localcyclelength));
	uint32_t pointersnap = pointer % cyclelength;

	/* calculate the new grain start and trigger ramping */
	if (!pointersnap)
	{
		/* cv->oldgrainstart = cv->grainstart;
		cv->oldgrainpitchedpointer = cv->grainpitchedpointer;
		cv->grainstart = pitchedpointer;
		cv->grainpitchedpointer = pitchedpointer; */

		cv->grainrampmax = MIN(cyclelength, grainrampmax);
		if (!pointer) /* don't ramp in the very first grain */
			cv->grainrampindex = cv->grainrampmax;
		else cv->grainrampindex = 0;
	}

	short rl = 0;
	short rr = 0;

	float calcshiftstereol = powf(2.0f, (float)(-iv->pitchstereo)*DIV1024);
	float calcshiftstereor = powf(2.0f, (float)(+iv->pitchstereo)*DIV1024);
	float calcshift =        powf(2.0f, (float)(localpitchshift - iv->timestretch)*DIV4096);
	float calcrate =  (float)iv->c5rate / (float)samplerate * powf(2.0f, (float)iv->timestretch*DIV4096);
	float calcpitch = (float)pitchedpointer / (float)pointer;

	if (iv->notestretch)
	{ /* note stretch */
		if (iv->reversegrains)
		{
			trimloop((pointer+cyclelength - pointersnap - (pointersnap * calcpitch*calcshift*calcshiftstereol)) * calcrate, cv, iv, 0, l);
			trimloop((pointer+cyclelength - pointersnap - (pointersnap * calcpitch*calcshift*calcshiftstereor)) * calcrate, cv, iv, 1, r);
		} else
		{
			trimloop(((pointer - pointersnap) + (pointersnap * calcpitch*calcshift*calcshiftstereol)) * calcrate, cv, iv, 0, l);
			trimloop(((pointer - pointersnap) + (pointersnap * calcpitch*calcshift*calcshiftstereor)) * calcrate, cv, iv, 1, r);
		}

		if (cv->grainrampindex < cv->grainrampmax)
		{
			if (iv->reversegrains)
			{
				trimloop((pointer - pointersnap - ((cyclelength + cv->grainrampindex) * calcpitch*calcshift*calcshiftstereol)) * calcrate, cv, iv, 0, &rl);
				trimloop((pointer - pointersnap - ((cyclelength + cv->grainrampindex) * calcpitch*calcshift*calcshiftstereor)) * calcrate, cv, iv, 1, &rr);
			} else
			{
				trimloop((pointer - pointersnap - cyclelength + ((cyclelength + cv->grainrampindex) * calcpitch*calcshift*calcshiftstereol)) * calcrate, cv, iv, 0, &rl);
				trimloop((pointer - pointersnap - cyclelength + ((cyclelength + cv->grainrampindex) * calcpitch*calcshift*calcshiftstereor)) * calcrate, cv, iv, 1, &rr);
			}

			gain = (float)cv->grainrampindex / (float)cv->grainrampmax;
			*l = *l * gain + rl * (1.0f - gain);
			*r = *r * gain + rr * (1.0f - gain);
			cv->grainrampindex++;
		}
	} else
	{ /* no note stretch */
		if (iv->reversegrains)
		{
			trimloop((pointer+cyclelength - pointersnap - (pointersnap * calcshift*calcshiftstereol)) * calcpitch*calcrate, cv, iv, 0, l);
			trimloop((pointer+cyclelength - pointersnap - (pointersnap * calcshift*calcshiftstereor)) * calcpitch*calcrate, cv, iv, 1, r);
		} else
		{
			trimloop((pointer - pointersnap + (pointersnap * calcshift*calcshiftstereol)) * calcpitch*calcrate, cv, iv, 0, l);
			trimloop((pointer - pointersnap + (pointersnap * calcshift*calcshiftstereor)) * calcpitch*calcrate, cv, iv, 1, r);
		}

		if (cv->grainrampindex < cv->grainrampmax)
		{
			if (iv->reversegrains)
			{
				trimloop((pointer - pointersnap - ((cyclelength + cv->grainrampindex) * calcshift*calcshiftstereol)) * calcpitch*calcrate, cv, iv, 0, &rl);
				trimloop((pointer - pointersnap - ((cyclelength + cv->grainrampindex) * calcshift*calcshiftstereor)) * calcpitch*calcrate, cv, iv, 1, &rr);
			} else
			{
				trimloop((pointer - pointersnap - cyclelength + ((cyclelength + cv->grainrampindex) * calcshift*calcshiftstereol)) * calcpitch*calcrate, cv, iv, 0, &rl);
				trimloop((pointer - pointersnap - cyclelength + ((cyclelength + cv->grainrampindex) * calcshift*calcshiftstereor)) * calcpitch*calcrate, cv, iv, 1, &rr);
			}

			gain = (float)cv->grainrampindex / (float)cv->grainrampmax;
			*l = *l * gain + rl * (1.0f - gain);
			*r = *r * gain + rr * (1.0f - gain);
			cv->grainrampindex++;
		}
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
