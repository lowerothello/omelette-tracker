/* clamps within range and loop, returns output samples */
void trimloop(double ptr, uint32_t length, uint32_t loop, Track *cv, uint8_t decimate, InstSamplerState *s, uint8_t stereotrack, short *output)
{
	InstSamplerPlaybackState *ps = cv->inststate;
	if (loop)
	{ /* if there is a loop range */
		if ((*s->sample)[ps->sampleslot]->pingpong)
		{ /* ping-pong loop */
			if (ptr > length)
			{
				ptr = fmod(ptr - length, loop<<1);
				if (ptr > loop) /* walking forwards  */ ptr = (length - loop) + (ptr - loop);
				else            /* walking backwards */ ptr = length - ptr;
			}

			if ((*s->sample)[ps->sampleslot]->channels == 1) getSample(ptr+(*s->sample)[ps->sampleslot]->trimstart, decimate, s->bitdepth, (*s->sample)[ps->sampleslot], output);
			else                                             getSample((ptr+(*s->sample)[ps->sampleslot]->trimstart)*(*s->sample)[ps->sampleslot]->channels + stereotrack, decimate, s->bitdepth, (*s->sample)[ps->sampleslot], output);
		} else
		{ /* crossfaded forwards loop */
			uint32_t looprampmax = MIN(samplerate*DIV1000 * LOOP_RAMP_MS, (loop>>1)) * (*s->sample)[ps->sampleslot]->loopramp*DIV255;
			if (ptr > length) ptr = (length - loop) + looprampmax + fmod(ptr - length, loop - looprampmax);

			if (ptr > length - looprampmax)
			{
				float lerp = (ptr - length + looprampmax) / (float)looprampmax;
				float ramppointer = (ptr - (loop - looprampmax));
				if ((*s->sample)[ps->sampleslot]->channels == 1) getSampleLoopRamp( ptr+(*s->sample)[ps->sampleslot]->trimstart, ramppointer, lerp, decimate, s->bitdepth, (*s->sample)[ps->sampleslot], output);
				else                                              getSampleLoopRamp((ptr+(*s->sample)[ps->sampleslot]->trimstart)*(*s->sample)[ps->sampleslot]->channels + stereotrack, ramppointer*(*s->sample)[ps->sampleslot]->channels + stereotrack, lerp, decimate, s->bitdepth, (*s->sample)[ps->sampleslot], output);
			} else
			{
				if ((*s->sample)[ps->sampleslot]->channels == 1) getSample( ptr+(*s->sample)[ps->sampleslot]->trimstart, decimate, s->bitdepth, (*s->sample)[ps->sampleslot], output);
				else                                              getSample((ptr+(*s->sample)[ps->sampleslot]->trimstart)*(*s->sample)[ps->sampleslot]->channels + stereotrack, decimate, s->bitdepth, (*s->sample)[ps->sampleslot], output);
			}
		}
	} else
	{
		if (ptr < length)
		{
			if ((*s->sample)[ps->sampleslot]->channels == 1) getSample( ptr+(*s->sample)[ps->sampleslot]->trimstart, decimate, s->bitdepth, (*s->sample)[ps->sampleslot], output);
			else                                              getSample((ptr+(*s->sample)[ps->sampleslot]->trimstart)*(*s->sample)[ps->sampleslot]->channels + stereotrack, decimate, s->bitdepth, (*s->sample)[ps->sampleslot], output);
		}
	}
}

void processCyclic(InstSamplerState *s, Track *cv, float rp, uint32_t pointer, uint32_t pitchedpointer, float finetune, short *l, short *r)
{
	InstSamplerPlaybackState *ps = cv->inststate;
	uint32_t length = MIN((*s->sample)[ps->sampleslot]->trimlength, (*s->sample)[ps->sampleslot]->length-1 - (*s->sample)[ps->sampleslot]->trimstart);
	uint32_t loop   = MIN((*s->sample)[ps->sampleslot]->looplength, length);
	uint16_t localcyclelength = s->granular.cyclelength; if (ps->localcyclelength != -1) localcyclelength = ps->localcyclelength;

	float f;
	uint16_t localpitchshift;
	f = macroStateGetMono(&ps->pitchshift, rp);
	if (f != NAN)
		localpitchshift = (f - 0.5f) * (127<<9);
	else
		localpitchshift = s->granular.pitchshift;

	uint16_t localpitchwidth;
	f = macroStateGetMono(&ps->pitchwidth, rp);
	if (f != NAN)
		localpitchwidth = (f - 0.5f) * (127<<1);
	else
		localpitchwidth = s->granular.pitchstereo;

	uint8_t localsamplerate;
	f = macroStateGetMono(&ps->samplerate, rp);
	if (f != NAN)
		localsamplerate = f*256;
	else
		localsamplerate = s->samplerate;

	uint16_t cyclelength = MAX(1, samplerate*DIV1000 * TIMESTRETCH_CYCLE_UNIT_MS * MAX(1, localcyclelength));
	uint32_t pointersnap = pointer % cyclelength;

	uint16_t grainrampmax = cyclelength*DIV32 * s->granular.rampgrains;
	/* calculate the new grain start and trigger ramping */
	if (!pointersnap)
	{
		if (!pointer) /* don't ramp in the very first grain */
			ps->grainrampindex = grainrampmax;
		else ps->grainrampindex = 0;
	}

	short rl = 0;
	short rr = 0;

	float gain;

	/* use doubles to try to avoid the worst of float approximation bullshit */
	double calcshiftstereol = powf(2.0f, (float)(-localpitchwidth)*DIV1024);
	double calcshiftstereor = powf(2.0f, (float)(+localpitchwidth)*DIV1024);
	double calcshift = semitoneShortToMultiplier(localpitchshift) / semitoneShortToMultiplier(s->granular.timestretch);
	double calcrate = (float)(*s->sample)[ps->sampleslot]->rate / (float)samplerate * semitoneShortToMultiplier(s->granular.timestretch);
	double calcpitch = powf(M_12_ROOT_2, (short)cv->r.note - NOTE_C5 + finetune);
	if (s->granular.notestretch)
	{ /* note stretch */
		if (s->granular.reversegrains)
		{
			trimloop((pitchedpointer+cyclelength - pointersnap - (pointersnap * calcpitch*calcshift*calcshiftstereol)) * calcrate, length, loop, cv, localsamplerate, s, 0, l);
			trimloop((pitchedpointer+cyclelength - pointersnap - (pointersnap * calcpitch*calcshift*calcshiftstereor)) * calcrate, length, loop, cv, localsamplerate, s, 1, r);
		} else
		{
			trimloop((pitchedpointer - pointersnap + (pointersnap * calcpitch*calcshift*calcshiftstereol)) * calcrate, length, loop, cv, localsamplerate, s, 0, l);
			trimloop((pitchedpointer - pointersnap + (pointersnap * calcpitch*calcshift*calcshiftstereor)) * calcrate, length, loop, cv, localsamplerate, s, 1, r);
		}

		if (ps->grainrampindex < grainrampmax)
		{
			if (s->granular.reversegrains)
			{
				trimloop((pitchedpointer - pointersnap - ((cyclelength + ps->grainrampindex) * calcpitch*calcshift*calcshiftstereol)) * calcrate, length, loop, cv, localsamplerate, s, 0, &rl);
				trimloop((pitchedpointer - pointersnap - ((cyclelength + ps->grainrampindex) * calcpitch*calcshift*calcshiftstereor)) * calcrate, length, loop, cv, localsamplerate, s, 1, &rr);
			} else
			{
				trimloop((pitchedpointer - pointersnap - cyclelength + ((cyclelength + ps->grainrampindex) * calcpitch*calcshift*calcshiftstereol)) * calcrate, length, loop, cv, localsamplerate, s, 0, &rl);
				trimloop((pitchedpointer - pointersnap - cyclelength + ((cyclelength + ps->grainrampindex) * calcpitch*calcshift*calcshiftstereor)) * calcrate, length, loop, cv, localsamplerate, s, 1, &rr);
			}

			gain = (float)ps->grainrampindex / (float)grainrampmax;
			*l = *l * gain + rl * (1.0f - gain);
			*r = *r * gain + rr * (1.0f - gain);
			ps->grainrampindex++;
		}
	} else
	{ /* no note stretch */
		if (s->granular.reversegrains)
		{
			trimloop((pitchedpointer+cyclelength - pointersnap - (pointersnap * calcshift*calcshiftstereol)) * calcpitch*calcrate, length, loop, cv, localsamplerate, s, 0, l);
			trimloop((pitchedpointer+cyclelength - pointersnap - (pointersnap * calcshift*calcshiftstereor)) * calcpitch*calcrate, length, loop, cv, localsamplerate, s, 1, r);
		} else
		{
			trimloop((pitchedpointer - pointersnap + (pointersnap * calcshift*calcshiftstereol)) * calcpitch*calcrate, length, loop, cv, localsamplerate, s, 0, l);
			trimloop((pitchedpointer - pointersnap + (pointersnap * calcshift*calcshiftstereor)) * calcpitch*calcrate, length, loop, cv, localsamplerate, s, 1, r);
		}

		if (ps->grainrampindex < grainrampmax)
		{
			if (s->granular.reversegrains)
			{
				trimloop((pitchedpointer - pointersnap - ((cyclelength + ps->grainrampindex) * calcshift*calcshiftstereol)) * calcpitch*calcrate, length, loop, cv, localsamplerate, s, 0, &rl);
				trimloop((pitchedpointer - pointersnap - ((cyclelength + ps->grainrampindex) * calcshift*calcshiftstereor)) * calcpitch*calcrate, length, loop, cv, localsamplerate, s, 1, &rr);
			} else
			{
				trimloop((pitchedpointer - pointersnap - cyclelength + ((cyclelength + ps->grainrampindex) * calcshift*calcshiftstereol)) * calcpitch*calcrate, length, loop, cv, localsamplerate, s, 0, &rl);
				trimloop((pitchedpointer - pointersnap - cyclelength + ((cyclelength + ps->grainrampindex) * calcshift*calcshiftstereor)) * calcpitch*calcrate, length, loop, cv, localsamplerate, s, 1, &rr);
			}

			gain = (float)ps->grainrampindex / (float)grainrampmax;
			*l = *l * gain + rl * (1.0f - gain);
			*r = *r * gain + rr * (1.0f - gain);
			ps->grainrampindex++;
		}
	}
}

void samplerProcess(Inst *iv, Track *cv, float rp, uint32_t pointer, uint32_t pitchedpointer, float finetune, short *l, short *r)
{
	InstSamplerState *s = iv->state;
	InstSamplerPlaybackState *ps = cv->inststate;

	if (!(*s->sample)[ps->sampleslot]->length) return;

	Envelope env;
	env.adsr = s->envelope; if (ps->localenvelope != -1) env.adsr = ps->localenvelope;
	applyEnvelopeControlChanges(&env);
	env.output = ps->envgain;
	env.release = cv->release;
	env.pointer = pointer;
	/* return if the envelope has finished */
	if (envelope(&env)) return;
	ps->envgain = env.output;

	processCyclic(s, cv, rp, pointer, pitchedpointer, finetune, l, r);

	short hold;
	switch (s->channelmode)
	{
		case SAMPLE_CHANNELS_STEREO: break;
		case SAMPLE_CHANNELS_LEFT:   *r = *l; break;
		case SAMPLE_CHANNELS_RIGHT:  *l = *r; break;
		case SAMPLE_CHANNELS_MIX:    *l = *r = ((*l>>1) + (*r>>1)); break;
		case SAMPLE_CHANNELS_SWAP:   hold = *l; *l = *r; *r = hold; break;
	}

	if ((*s->sample)[ps->sampleslot]->invert)
	{
		*l *= -1;
		*r *= -1;
	}

	*l *= ps->envgain;
	*r *= ps->envgain;

	float fgain = powf(2, (float)s->gain*DIV16);
	*l *= fgain;
	*r *= fgain;
}

void samplerLookback(Inst *iv, Track *cv, uint16_t *spr)
{
	InstSamplerState *s = iv->state;
	InstSamplerPlaybackState *ps = cv->inststate;

	Envelope env, wtenv;
	env.adsr = s->envelope; if (ps->localenvelope != -1) env.adsr = ps->localenvelope;
	applyEnvelopeControlChanges(&env);
	wtenv.adsr = s->wavetable.envelope;
	applyEnvelopeControlChanges(&wtenv);
	env.pointer = wtenv.pointer = cv->pointer;
	env.output = ps->envgain;
	wtenv.output = ps->modenvgain;
	env.release = wtenv.release = cv->release;
	for (uint16_t sprp = 0; sprp < *spr; sprp++)
	{
		envelope(&env);
		envelope(&wtenv);
	}
	ps->envgain = env.output;
	ps->modenvgain = wtenv.output;
}
