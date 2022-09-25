void getSample(uint32_t p, instrument *iv, short *l, short *r)
{
	/* shift is used here for bitcrushing */
	uint8_t shift = 15 - iv->bitdepth;

	short leftdelta = (iv->sampledata[p*iv->channels+0]>>shift)<<shift;
	*l += leftdelta;

	if (iv->channels == 1) *r += leftdelta;
	else                   *r += (iv->sampledata[p*iv->channels+1]>>shift)<<shift;
}

void getSampleLoopRamp(uint32_t p, uint32_t q, float lerp, instrument *iv, short *l, short *r)
{
	/* shift is used here for bitcrushing */
	uint8_t shift = 15 - iv->bitdepth;

	short leftdelta = ((iv->sampledata[p*iv->channels+0]>>shift)<<shift) * (1.0f - lerp) + ((iv->sampledata[q * iv->channels+0]>>shift)<<shift)*lerp;
	*l += leftdelta;

	if (iv->channels == 1) *r += leftdelta;
	else                   *r += ((iv->sampledata[p*iv->channels+1]>>shift)<<shift) * (1.0f - lerp) + ((iv->sampledata[q * iv->channels+1]>>shift)<<shift)*lerp;
}

/* clamps within range and loop, returns output samples */
void trimloop(uint32_t p, uint8_t localenvelope,
		channel *cv, instrument *iv, short *l, short *r)
{
	p += iv->trim[0];

	if (iv->loop < iv->trim[1])
	{ /* if there is a loop range */
		if (iv->pingpong)
		{ /* ping-pong loop */
			uint32_t looplength = iv->trim[1] - iv->loop;
			if (p > iv->trim[1])
			{
				uint32_t i = (p - iv->trim[1])/looplength;
				if (i % 2 == 0) /* backwards */ p = iv->trim[1] - (p - iv->trim[1])%looplength;
				else            /* forwards  */ p = iv->loop    + (p - iv->trim[1])%looplength;
			}
			/* always point to the left channel */
			p -= p % iv->channels;
			getSample(p, iv, l, r);
		} else
		{ /* crossfaded forwards loop */
			uint32_t looprampmax = MIN(samplerate*DIV1000 * LOOP_RAMP_MS, (iv->trim[1] - iv->loop)*0.5) * (iv->loopramp*DIV255);
			uint32_t looplength = iv->trim[1] - iv->loop - looprampmax;
			if (p > iv->trim[1]) p = iv->loop + looprampmax + (p - iv->trim[1])%looplength;
			/* always point to the left channel */
			p -= p % iv->channels;
			if (p > iv->trim[1] - looprampmax)
			{
				float lerp = (p - iv->trim[1] + looprampmax) / (float)looprampmax;
				uint32_t ramppointer = (p - looplength);
				/* always point to the left channel */
				ramppointer -= ramppointer % iv->channels;
				getSampleLoopRamp(p, ramppointer, lerp, iv, l, r);
			} else getSample(p, iv, l, r);
		}
	} else
	{
		p -= p % iv->channels;
		if (p < iv->trim[1]) getSample(p, iv, l, r);
	}
}

uint32_t calcDecimate(instrument *iv, uint8_t decimate, uint32_t pointer)
{
	/* using a double here so the pointer should't be quantized by float bullshit too badly */
	double d = 1.0f + (1.0f - decimate*DIV255) * 20;
	return (uint32_t)(pointer/d)*d;
}

int envelope(instrument *iv, channel *cv, uint32_t pointer)
{
	uint8_t env = iv->envelope; if (cv->localenvelope != -1) env = cv->localenvelope;

	uint32_t alen = ((env>>4)+ENVELOPE_A_MIN) * ENVELOPE_A_STEP * samplerate;
	uint32_t dlen = ((env%16)+ENVELOPE_D_MIN) * ENVELOPE_D_STEP * samplerate;

	if (cv->data.release || (!iv->sustain && pointer > alen))
	     { if (dlen) cv->envgain = MAX(cv->envgain - (1.0f/dlen), 0.0f); else cv->envgain = 0.0f; }
	else { if (alen) cv->envgain = MIN(cv->envgain + (1.0f/alen), 1.0f); else cv->envgain = 1.0f; }

	if (pointer > alen && cv->envgain < NOISE_GATE) return 0;
	return 1;
}

void samplerProcess(uint8_t realinst, channel *cv, uint32_t pointer, uint32_t pitchedpointer, short *l, short *r)
{
	instrument *iv = &p->s->instrumentv[realinst];
	if (!iv->sampledata) return;

	/* return if the sample has finished */
	if (!envelope(iv, cv, pointer)) return;

	uint32_t pointersnap;
	short hold;
	float gain;

	uint8_t localenvelope = iv->envelope; if (cv->localenvelope != -1) localenvelope = cv->localenvelope;
	uint8_t localpitchshift = iv->pitchshift; if (cv->localpitchshift != -1) localpitchshift = cv->localpitchshift;
	uint16_t localcyclelength = iv->cyclelength; if (cv->localcyclelength != -1) localcyclelength = cv->localcyclelength;

	uint16_t cyclelength = MAX(1, samplerate*DIV1000 * TIMESTRETCH_CYCLE_UNIT_MS * MAX(1, localcyclelength));
	pointersnap = pointer % cyclelength;

	uint32_t ramppos = 0; if (cv->data.reverse) ramppos = cyclelength;

	if (pointersnap == ramppos)
	{ // first sample of a cycle
		cv->stretchrampmax = MIN(cyclelength, stretchrampmax);
		if (pointer == 0) cv->stretchrampindex = cv->stretchrampmax;
		else cv->stretchrampindex = 0;
	}

	if (iv->timestretch)
	{ /* time stretch */
		float calcrate = (float)iv->c5rate / (float)samplerate;
		float calcpitchshift = (float)pitchedpointer / (float)pointer * powf(2, localpitchshift*DIV64 - 2.0f);

		trimloop(calcDecimate(iv, iv->samplerate,
				((pointer - pointersnap) * calcrate) + (pointersnap * calcpitchshift) * calcrate),
				localenvelope, cv, iv, l, r);

		if (cv->stretchrampindex < cv->stretchrampmax)
		{
			short rl = 0; short rr = 0;
			trimloop(calcDecimate(iv, iv->samplerate,
					((pointer - pointersnap - cyclelength) * calcrate) + ((cyclelength + cv->stretchrampindex) * calcpitchshift * calcrate)),
					localenvelope, cv, iv, &rl, &rr);

			gain = (float)cv->stretchrampindex / (float)cv->stretchrampmax;
			*l = *l * gain + rl * (1.0f - gain);
			*r = *r * gain + rr * (1.0f - gain);
			cv->stretchrampindex++;
		}
	} else
	{ /* pitch shift */
		float calcshift = powf(2, localpitchshift*DIV64 - 2.0f);
		float calcpitchrate = (float)pitchedpointer / (float)pointer * (float)iv->c5rate / (float)samplerate;

		trimloop(calcDecimate(iv, iv->samplerate,
				(pointer - pointersnap + (pointersnap * calcshift)) * calcpitchrate),
				localenvelope, cv, iv, l, r);

		if (cv->stretchrampindex < cv->stretchrampmax)
		{
			short rl = 0; short rr = 0;
			trimloop(calcDecimate(iv, iv->samplerate,
					(pointer - pointersnap - cyclelength + ((cyclelength + cv->stretchrampindex) * calcshift)) * calcpitchrate),
					localenvelope, cv, iv, &rl, &rr);

			gain = (float)cv->stretchrampindex / (float)cv->stretchrampmax;
			*l = *l * gain + rl * (1.0f - gain);
			*r = *r * gain + rr * (1.0f - gain);
			cv->stretchrampindex++;
		}
	}

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
