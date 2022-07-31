#define S_FLAG_TTEMPO  0b00000001 /* timestretch     */
#define S_FLAG_PHASE   0b00000010 /* invert phase    */
#define S_FLAG_SUSTAIN 0b00000100 /* inverse sustain */
#define S_FLAG_MIDI    0b00001000 /* midi output     */
#define S_FLAG_RPLAY   0b00010000 /*   deprecated    */
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
void trimloop(uint32_t pitchedpointer, uint32_t pointer, uint8_t localenvelope,
		channel *cv, instrument *iv, short *l, short *r)
{
	uint32_t p = pitchedpointer + iv->trim[0] + cv->pointeroffset;

	if (iv->loop[1])
	{ /* if there is a loop range */
		if (iv->loop[0] == iv->loop[1] && p > iv->loop[1])
		{
			*l = *r = 0;
			return;
		}

		if (iv->flags & S_FLAG_PPLOOP)
		{ /* ping-pong loop */
			uint32_t looplength = iv->loop[1] - iv->loop[0];
			if (p > iv->loop[1])
			{
				uint32_t i = (p - iv->loop[1])/looplength;
				if (i % 2 == 0) /* backwards */ p = iv->loop[1] - (p - iv->loop[1])%looplength;
				else            /* forwards  */ p = iv->loop[0] + (p - iv->loop[1])%looplength;
			}

			/* always point to the left channel */
			p -= p % iv->channels;
			getSample(p, iv, l, r);
		} else
		{ /* crossfaded forwards loop */
			uint32_t looprampmax = MIN(samplerate*DIV1000 * LOOP_RAMP_MS, (iv->loop[1] - iv->loop[0])*0.5) * (iv->loopramp*DIV255);
			uint32_t looplength = iv->loop[1] - iv->loop[0] - looprampmax;
			if (p > iv->loop[1]) p = iv->loop[0] + looprampmax + (p - iv->loop[1])%looplength;

			/* always point to the left channel */
			p -= p % iv->channels;

			if (p > iv->loop[1] - looprampmax)
			{
				float lerp = (p - iv->loop[1] + looprampmax) / (float)looprampmax;
				uint32_t ramppointer = (p - looplength);
				/* always point to the left channel */
				ramppointer -= ramppointer % iv->channels;
				getSampleLoopRamp(p, ramppointer, lerp, iv, l, r);
			} else getSample(p, iv, l, r);
		}
	}

	/* auto-trigger the release envelope */
	if (!cv->releasepointer && ((iv->loop[1] && iv->trim[1] < iv->loop[1]) || !iv->loop[1])
			&& p > iv->trim[1] - (((localenvelope%16)+ENVELOPE_DECAY_MIN) * ENVELOPE_DECAY * samplerate))
		cv->releasepointer = pointer;

	if (!(iv->loop[0] || iv->loop[1]))
	{
		p -= p % iv->channels;
		if (p < iv->length) getSample(p, iv, l, r);
	}
}

uint32_t calcDecimate(instrument *iv, uint8_t decimate, uint32_t pointer)
{
	float d = 1.0f + (1.0f - decimate*DIV255) * 20;

	if (iv->samplerate == 255) return pointer;
	else                       return (uint32_t)(pointer / d) * d;
}

void samplerProcess(instrument *iv, channel *cv, uint32_t pointer, short *l, short *r)
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
	if (cv->localcyclelength != -1) localcyclelength = cv->localcyclelength<<8;
	else                            localcyclelength = iv->cyclelength;

	float envgain = envelope(localenvelope, pointer, cv->releasepointer, !(iv->flags&S_FLAG_SUSTAIN));
	if (!(pointer > ((localenvelope>>4)+ENVELOPE_ATTACK_MIN) * ENVELOPE_ATTACK * samplerate && envgain < NOISE_GATE) && iv->length > 0)
	{
		uint16_t cyclelength = MAX(1, samplerate*DIV1000 * TIMESTRETCH_CYCLE_UNIT_MS * MAX(1, localcyclelength));
		pointersnap = pointer % cyclelength;

		if (cv->reverse) ramppos = cyclelength;
		else             ramppos = 0;

		if (pointersnap == ramppos)
		{ // first sample of a cycle
			cv->localstretchrampmax = MIN(cyclelength, stretchrampmax);
			if (pointer == 0) cv->stretchrampindex = cv->localstretchrampmax;
			else cv->stretchrampindex = 0;
		}

		float calcpitch, calcshift;
		float calcrate = (float)iv->c5rate / (float)samplerate;
		if (iv->flags & S_FLAG_TTEMPO)
		{ /* time stretch */
			calcpitch = powf(M_12_ROOT_2, (short)cv->r.note - C5 + cv->finetune + (localpitchshift*DIV64ALT - 2.0f) * 12.0f);

			trimloop(calcDecimate(iv, iv->samplerate,
					((pointer - pointersnap) * calcrate) + (pointersnap * calcpitch) * calcrate),
					pointer, localenvelope, cv, iv, l, r);

			if (cv->stretchrampindex < cv->localstretchrampmax)
			{
				short rl = 0; short rr = 0;
				trimloop(calcDecimate(iv, iv->samplerate,
						((pointer - pointersnap - cyclelength) * calcrate) + ((cyclelength + cv->stretchrampindex) * calcpitch * calcrate)),
						pointer, localenvelope, cv, iv, &rl, &rr);

				gain = (float)cv->stretchrampindex / (float)cv->localstretchrampmax;
				*l = *l * gain + rl * (1.0f - gain);
				*r = *r * gain + rr * (1.0f - gain);
				cv->stretchrampindex++;
			}
		} else
		{ /* pitch shift */
			calcpitch = powf(M_12_ROOT_2, (short)cv->r.note - C5 + cv->finetune); /* really slow */
			calcshift = powf(2, localpitchshift*DIV64ALT - 2.0f);

			trimloop(calcDecimate(iv, iv->samplerate,
					(float)(pointer - pointersnap + (pointersnap * calcshift)) * calcpitch * calcrate),
					pointer, localenvelope, cv, iv, l, r);

			if (cv->stretchrampindex < cv->localstretchrampmax)
			{
				short rl = 0; short rr = 0;
				trimloop(calcDecimate(iv, iv->samplerate,
						(float)(pointer - pointersnap - cyclelength + ((cyclelength + cv->stretchrampindex) * calcshift)) * calcpitch * calcrate),
						pointer, localenvelope, cv, iv, &rl, &rr);

				gain = (float)cv->stretchrampindex / (float)cv->localstretchrampmax;
				*l = *l * gain + rl * (1.0f - gain);
				*r = *r * gain + rr * (1.0f - gain);
				cv->stretchrampindex++;
			}
		}
	}

	switch (iv->channelmode)
	{
		case 1: *r = *l; break;
		case 2: *l = *r; break;
		case 3: *l = *r = ((*l>>1) + (*r>>1)); break;
		case 4: hold = *l; *l = *r; *r = hold; break;
	}

	if (iv->flags & S_FLAG_PHASE) { *l *= -1; *r *= -1; }
	*l *= envgain; *r *= envgain;
}
