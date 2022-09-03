#define S_FLAG_TTEMPO  0b00000001 /* timestretch     */
#define S_FLAG_PHASE   0b00000010 /* invert phase    */
#define S_FLAG_SUSTAIN 0b00000100 /* inverse sustain */
#define S_FLAG_MIDI    0b00001000 /* midi output     */
// #define S_FLAG_RPLAY   0b00010000 /*   deprecated    */
#define S_FLAG_PPLOOP  0b00100000 /* ping-pong loop  */

void getSample(uint32_t p, instrument *iv, short *l, short *r)
{
	uint8_t shift = 15 - iv->bitdepth;
	/* listchars */       *l += (iv->sampledata[p*iv->channels+0]>>shift)<<shift;
	if (iv->channels > 1) *r += (iv->sampledata[p*iv->channels+1]>>shift)<<shift;
	else                  *r += (iv->sampledata[p*iv->channels+0]>>shift)<<shift;
}

void getSampleLoopRamp(uint32_t p, uint32_t q, float lerp, instrument *iv, short *l, short *r)
{
	uint8_t shift = 15 - iv->bitdepth;
	/* listchars */       *l += ((iv->sampledata[p*iv->channels+0]>>shift)<<shift) * (1.0f - lerp) + ((iv->sampledata[q * iv->channels+0]>>shift)<<shift) * lerp;
	if (iv->channels > 1) *r += ((iv->sampledata[p*iv->channels+1]>>shift)<<shift) * (1.0f - lerp) + ((iv->sampledata[q * iv->channels+1]>>shift)<<shift) * lerp;
	else                  *r += ((iv->sampledata[p*iv->channels+0]>>shift)<<shift) * (1.0f - lerp) + ((iv->sampledata[q * iv->channels+0]>>shift)<<shift) * lerp;
}

/* clamps within range and loop, returns output samples */
void trimloop(uint32_t p, uint8_t localenvelope,
		channel *cv, instrument *iv, short *l, short *r)
{
	p += iv->trim[0];

	if (iv->loop < iv->trim[1])
	{ /* if there is a loop range */
		if (iv->flags&S_FLAG_PPLOOP)
		{ /* ping-pong loop */
			uint32_t looplength = iv->trim[1] - iv->loop;
			if (p > iv->trim[1])
			{
				uint32_t i = (p - iv->trim[1])/looplength;
				if (i % 2 == 0) /* backwards */ p = iv->trim[1] - (p - iv->trim[1])%looplength;
				else            /* forwards  */ p = iv->loop + (p - iv->trim[1])%looplength;
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
	} else if (p < iv->trim[1])
	{
		p -= p % iv->channels;
		if (p < iv->length) getSample(p, iv, l, r);
	}
}

uint32_t calcDecimate(instrument *iv, uint8_t decimate, uint32_t pointer)
{
	float d = 1.0f + (1.0f - decimate*DIV255) * 20;

	if (iv->samplerate == 255) return pointer;
	else                       return (uint32_t)(pointer/d)*d;
}

void samplerProcess(instrument *iv, channel *cv, uint32_t pointer, uint32_t pitchedpointer, short *l, short *r)
{
	uint32_t ramppos, pointersnap;
	short hold;
	float gain;

	uint8_t localenvelope;
	if (cv->localenvelope != -1) localenvelope = cv->localenvelope;
	else                         localenvelope = iv->envelope;
	uint8_t localpitchshift;
	if (cv->localpitchshift != -1) localpitchshift = cv->localpitchshift;
	else                           localpitchshift = iv->pitchshift;
	uint16_t localcyclelength;
	if (cv->localcyclelength != -1) localcyclelength = cv->localcyclelength;
	else                            localcyclelength = iv->cyclelength;

	uint16_t cyclelength = MAX(1, samplerate*DIV1000 * TIMESTRETCH_CYCLE_UNIT_MS * MAX(1, localcyclelength));
	pointersnap = pointer % cyclelength;

	if (cv->flags&C_FLAG_REVERSE) ramppos = cyclelength;
	else                          ramppos = 0;

	if (pointersnap == ramppos)
	{ // first sample of a cycle
		cv->localstretchrampmax = MIN(cyclelength, stretchrampmax);
		if (pointer == 0) cv->stretchrampindex = cv->localstretchrampmax;
		else cv->stretchrampindex = 0;
	}

	float calcshift = powf(2, localpitchshift*DIV64 - 2.0f);
	float calcrate = (float)iv->c5rate / (float)samplerate;
	float calcpitch = (float)pitchedpointer / (float)pointer;
	if (iv->flags&S_FLAG_TTEMPO)
	{ /* time stretch */
		trimloop(calcDecimate(iv, iv->samplerate,
				((pointer - pointersnap) * calcrate) + (pointersnap * calcpitch * calcshift) * calcrate),
				localenvelope, cv, iv, l, r);

		if (cv->stretchrampindex < cv->localstretchrampmax)
		{
			short rl = 0; short rr = 0;
			trimloop(calcDecimate(iv, iv->samplerate,
					((pointer - pointersnap - cyclelength) * calcrate) + ((cyclelength + cv->stretchrampindex) * calcpitch * calcshift * calcrate)),
					localenvelope, cv, iv, &rl, &rr);

			gain = (float)cv->stretchrampindex / (float)cv->localstretchrampmax;
			*l = *l * gain + rl * (1.0f - gain);
			*r = *r * gain + rr * (1.0f - gain);
			cv->stretchrampindex++;
		}
	} else
	{ /* pitch shift */
		trimloop(calcDecimate(iv, iv->samplerate,
				(pointer - pointersnap + (pointersnap * calcshift)) * calcpitch * calcrate),
				localenvelope, cv, iv, l, r);

		if (cv->stretchrampindex < cv->localstretchrampmax)
		{
			short rl = 0; short rr = 0;
			trimloop(calcDecimate(iv, iv->samplerate,
					(pointer - pointersnap - cyclelength + ((cyclelength + cv->stretchrampindex) * calcshift)) * calcpitch * calcrate),
					localenvelope, cv, iv, &rl, &rr);

			gain = (float)cv->stretchrampindex / (float)cv->localstretchrampmax;
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

	if (iv->flags&S_FLAG_PHASE) { *l *= -1; *r *= -1; }
	*l *= cv->envgain; *r *= cv->envgain;
}
