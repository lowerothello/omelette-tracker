#define S_FLAG_TTEMPO  0b00000001 /* timestretch     */
#define S_FLAG_MONO    0b00000010 /*   deprecated    */
#define S_FLAG_SUSTAIN 0b00000100 /* inverse sustain */
#define S_FLAG_MIDI    0b00001000 /* midi output     */
#define S_FLAG_RPLAY   0b00010000 /*   deprecated    */
#define S_FLAG_PPLOOP  0b00100000 /* ping-pong loop  */
#define S_FLAG_PHASE   0b10000000 /* invert phase    */

void getSample(uint32_t p, instrument *iv, float *l, float *r)
{
	uint8_t shift = 15 - iv->bitdepth;
	/* listchars */       *l += ((iv->sampledata[p*iv->channels+0]>>shift)<<shift)*DIVSHRT;
	if (iv->channels > 1) *r += ((iv->sampledata[p*iv->channels+1]>>shift)<<shift)*DIVSHRT;
	else                  *r += ((iv->sampledata[p*iv->channels+0]>>shift)<<shift)*DIVSHRT;
}

void getSampleLoopRamp(uint32_t p, uint32_t q, float lerp, instrument *iv, float *l, float *r)
{
	uint8_t shift = 15 - iv->bitdepth;
	/* listchars */       *l += ((iv->sampledata[p*iv->channels+0]>>shift)<<shift)*DIVSHRT * (1.0 - lerp) + ((iv->sampledata[q * iv->channels+0]>>shift)<<shift)*DIVSHRT * lerp;
	if (iv->channels > 1) *r += ((iv->sampledata[p*iv->channels+1]>>shift)<<shift)*DIVSHRT * (1.0 - lerp) + ((iv->sampledata[q * iv->channels+1]>>shift)<<shift)*DIVSHRT * lerp;
	else                  *r += ((iv->sampledata[p*iv->channels+0]>>shift)<<shift)*DIVSHRT * (1.0 - lerp) + ((iv->sampledata[q * iv->channels+0]>>shift)<<shift)*DIVSHRT * lerp;
}

/* clamps within range and loop, returns output samples */
void trimloop(uint32_t pitchedpointer, uint32_t pointer,
		channel *cv, instrument *iv, sample_t *l, sample_t *r)
{
	uint32_t p = pitchedpointer + iv->trim[0] + cv->pointeroffset;

	if (iv->loop[1])
	{ /* if there is a loop range */
		if (iv->loop[0] == iv->loop[1] && p > iv->loop[1])
		{
			*l = *r = 0.0f;
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
			uint32_t looprampmax = MIN(samplerate/1000 * LOOP_RAMP_MS, (iv->loop[1] - iv->loop[0])*0.5) * (iv->loopramp*DIV255);
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

	/* trigger the release envelope */
	if (((iv->loop[1] && iv->trim[1] < iv->loop[1]) || !iv->loop[1])
			&& p > iv->trim[1] - (iv->volume.r * ENVELOPE_RELEASE * samplerate)
			&& !cv->releasepointer)
		cv->releasepointer = pointer;

	if (!(iv->loop[0] || iv->loop[1]))
	{
		/* always point to the left channel */
		p -= p % iv->channels;

		if (p < iv->length) getSample(p, iv, l, r);
	}

	/* sample gain */
	*l *= iv->gain*DIV64; *r *= iv->gain*DIV64;
}

uint32_t calcDecimate(instrument *iv, uint8_t decimate, uint32_t pointer)
{
	float d = 1.0f + (1.0f - decimate*DIV255) * 20;

	if (iv->samplerate == 255) return pointer;
	else                       return (uint32_t)(pointer / d) * d;
}

void samplerProcess(instrument *iv, channel *cv, uint32_t pointer, float *l, float *r)
{
	uint32_t ramppos, pointersnap;
	uint16_t cyclelength;

	float gain = adsrEnvelope(iv->volume, 0.0, pointer, cv->releasepointer);
	if (!(pointer > (iv->volume.a+ENVELOPE_ATTACK_MIN) * ENVELOPE_ATTACK * samplerate && gain == 0.0f)
			&& iv->length > 0)
	{
		cyclelength = MAX(iv->cyclelength, 1);
		pointersnap = pointer % cyclelength;

		if (cv->reverse) ramppos = cyclelength;
		else             ramppos = 0;

		if (pointersnap == ramppos)
		{ // first sample of a cycle
			cv->localstretchrampmax = MIN(cyclelength, stretchrampmax);
			if (pointer == 0) cv->stretchrampindex = cv->localstretchrampmax;
			else cv->stretchrampindex = 0;
		}

		float calcpitch;
		float calcrate = (float)iv->c5rate / (float)samplerate;
		if (iv->flags & S_FLAG_TTEMPO)
		{ /* time stretch */
			calcpitch = powf(M_12_ROOT_2, (short)cv->r.note - C5 + cv->finetune) * (float)(iv->pitchshift*DIV128);

			trimloop(calcDecimate(iv, iv->samplerate,
					((pointer - pointersnap) * calcrate) + (pointersnap * calcpitch) * calcrate),
					pointer, cv, iv, l, r);

			if (cv->stretchrampindex < cv->localstretchrampmax)
			{
				float rl = 0.0f; float rr = 0.0f;
				trimloop(calcDecimate(iv, iv->samplerate,
						((pointer - pointersnap - cyclelength) * calcrate) + ((cyclelength + cv->stretchrampindex) * calcpitch * calcrate)),
						pointer, cv, iv, &rl, &rr);

				float gain = (float)cv->stretchrampindex / (float)cv->localstretchrampmax;
				*l = *l * gain + rl * (1.0f - gain); *r = *r * gain + rr * (1.0f - gain);
				cv->stretchrampindex++;
			}
		} else
		{ /* pitch shift */
			calcpitch = powf(M_12_ROOT_2, (short)cv->r.note - C5 + cv->finetune); /* really slow */

			trimloop(calcDecimate(iv, iv->samplerate,
					(float)(pointer - pointersnap + (pointersnap * (float)(iv->pitchshift*DIV128))) * calcpitch * calcrate),
					pointer, cv, iv, l, r);

			if (cv->stretchrampindex < cv->localstretchrampmax)
			{
				float rl = 0.0f; float rr = 0.0f;
				trimloop(calcDecimate(iv, iv->samplerate,
						(float)(pointer - pointersnap - cyclelength + ((cyclelength + cv->stretchrampindex) * (float)(iv->pitchshift*DIV128))) * calcpitch * calcrate),
						pointer, cv, iv, &rl, &rr);

				float gain = (float)cv->stretchrampindex / (float)cv->localstretchrampmax;
				*l = *l * gain + rl * (1.0f - gain); *r = *r * gain + rr * (1.0f - gain);
				cv->stretchrampindex++;
			}
		}
	}

	if (iv->flags & S_FLAG_MONO)  { *l = (*l + *r) * 0.5f; *r = *l; }
	*l *= gain; *r *= gain;
}
