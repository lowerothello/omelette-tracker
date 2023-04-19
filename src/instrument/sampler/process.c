/* clamps within range and loop, returns output samples */
static void trimloop(double ptr, uint32_t length, uint32_t loop, Track *cv, uint8_t decimate, InstSamplerState *s, uint8_t stereotrack, short *output)
{
	InstSamplerPlaybackState *ps = cv->inststate;

	if (loop)
	{ /* if there is a loop range */
		if ((*s->sample)[ps->sampleslot]->pingpong)
		{ /* ping-pong loop */
			if (ptr > length)
			{
				ptr = ptr - (length % (loop<<1));
				if (ptr > loop) /* walking forwards  */ ptr = (length - loop) + (ptr - loop);
				else            /* walking backwards */ ptr = length - ptr;
			}

			if ((*s->sample)[ps->sampleslot]->channels == 1)
				getSample(ptr+(*s->sample)[ps->sampleslot]->trimstart, s->interpolate, decimate, s->bitredux, (*s->sample)[ps->sampleslot], output);
			else
				getSample((ptr+(*s->sample)[ps->sampleslot]->trimstart)*(*s->sample)[ps->sampleslot]->channels + stereotrack, s->interpolate, decimate, s->bitredux, (*s->sample)[ps->sampleslot], output);
		} else
		{ /* crossfaded forwards loop */
			uint32_t looprampmax = MIN(samplerate/1000 * LOOP_RAMP_MS, (loop>>1)) * ((*s->sample)[ps->sampleslot]->loopramp/255);
			if (ptr > length)
				ptr = (length - loop) + looprampmax + fmodf(ptr - length, loop - looprampmax);

			if (ptr > length - looprampmax)
			{
				float lerp = (ptr - length + looprampmax) / (float)looprampmax;
				float ramppointer = (ptr - (loop - looprampmax));
				if ((*s->sample)[ps->sampleslot]->channels == 1)
					getSampleLoopRamp(ptr+(*s->sample)[ps->sampleslot]->trimstart, ramppointer, lerp, s->interpolate, decimate, s->bitredux, (*s->sample)[ps->sampleslot], output);
				else
					getSampleLoopRamp((ptr+(*s->sample)[ps->sampleslot]->trimstart)*(*s->sample)[ps->sampleslot]->channels + stereotrack, ramppointer*(*s->sample)[ps->sampleslot]->channels + stereotrack, lerp, s->interpolate, decimate, s->bitredux, (*s->sample)[ps->sampleslot], output);
			} else
			{
				if ((*s->sample)[ps->sampleslot]->channels == 1)
					getSample(ptr+(*s->sample)[ps->sampleslot]->trimstart, s->interpolate, decimate, s->bitredux, (*s->sample)[ps->sampleslot], output);
				else
					getSample((ptr+(*s->sample)[ps->sampleslot]->trimstart)*(*s->sample)[ps->sampleslot]->channels + stereotrack, s->interpolate, decimate, s->bitredux, (*s->sample)[ps->sampleslot], output);
			}
		}
	} else
	{
		if (ptr < length)
		{
			if ((*s->sample)[ps->sampleslot]->channels == 1)
				getSample(ptr+(*s->sample)[ps->sampleslot]->trimstart, s->interpolate, decimate, s->bitredux, (*s->sample)[ps->sampleslot], output);
			else
				getSample((ptr+(*s->sample)[ps->sampleslot]->trimstart)*(*s->sample)[ps->sampleslot]->channels + stereotrack, s->interpolate, decimate, s->bitredux, (*s->sample)[ps->sampleslot], output);
		}
	}
}

static void samplerProcess(Inst *iv, Track *cv, float rp, uint32_t pointer, float note, short *l, short *r)
{
	InstSamplerState *s = iv->state;
	InstSamplerPlaybackState *ps = cv->inststate;

	if (!(*s->sample)[ps->sampleslot] || !(*s->sample)[ps->sampleslot]->length) return;

	Envelope env;
	env.adsr = s->envelope; if (ps->localenvelope != -1) env.adsr = ps->localenvelope;
	applyEnvelopeControlChanges(&env);
	env.output = ps->envgain;
	env.release = cv->release;
	env.pointer = pointer;
	/* return if the envelope has finished */
	if (envelope(&env)) return;
	ps->envgain = env.output;

	uint32_t length = MIN((*s->sample)[ps->sampleslot]->trimlength, (*s->sample)[ps->sampleslot]->length-1 - (*s->sample)[ps->sampleslot]->trimstart);
	uint32_t loop   = MIN((*s->sample)[ps->sampleslot]->looplength, length);

	float f;
	int localpitchshift;
	f = macroStateGetMono(&ps->pitchshift, rp);
	if (!isnan(f)) localpitchshift = (f - 0.5f) * (127<<9);
	else           localpitchshift = s->pitchshift;
	float pitchshift = semitoneShortToMultiplier(localpitchshift);

	uint8_t localrateredux;
	f = macroStateGetMono(&ps->rateredux, rp);
	if (!isnan(f)) localrateredux = f*256;
	else           localrateredux = s->rateredux;

	uint16_t localcyclelength = s->cyclelength;
	if (ps->localcyclelength != -1)
		localcyclelength = ps->localcyclelength;

	uint16_t cyclelength = (samplerate/1000 * TIMESTRETCH_CYCLE_UNIT_MS * localcyclelength) / pitchshift;
	uint16_t grainrampmax = (cyclelength>>4) * s->ramp;

	float pitchmod = powf(M_12_ROOT_2, note - NOTE_C5);

	float calcshift = semitoneShortToMultiplier(s->formantshift) * pitchshift;
	float calcshiftl = powf(2.0f, -(s->formantstereo*DIV1024))*calcshift;
	float calcshiftr = powf(2.0f, +(s->formantstereo*DIV1024))*calcshift;

	float calctime = semitoneShortToMultiplier(s->timestretch);
	float calcrate = (float)(*s->sample)[ps->sampleslot]->rate / (float)samplerate;

	float framegrainoffset;
	double frameptr, nextframeptr;
	if (s->notestretch)
	{
		framegrainoffset = ps->pitchedpointer / pitchmod; /* undo the speed distortion to get pointer affected by any drift that may have happened */
		frameptr = framegrainoffset*calctime*calcrate;
		nextframeptr = ((ps->pitchedpointer+pitchmod) / pitchmod)*calctime*calcrate;
	} else
	{
		framegrainoffset = ps->pitchedpointer;
		frameptr = framegrainoffset*calctime*calcrate;
		nextframeptr = (ps->pitchedpointer+pitchmod)*calctime*calcrate;
	}

	short el, er;
	float envtarget;
	bool attack = 0;
	if (s->transientsensitivity)
		/* only process pointer int crossings */
		for (uint32_t i = (uint32_t)frameptr; i < (uint32_t)nextframeptr; i++)
		{
			trimloop(i, length, loop, cv, 0xff, s, 0, &el);
			trimloop(i, length, loop, cv, 0xff, s, 1, &er);
			envtarget = fabsf((el>>1) + (er>>1)) / SHRT_MAX;

			if (ps->transattackfollower < envtarget) ps->transattackfollower = MIN(ps->transattackfollower + TRANS_ENV_FOLLOWER_SPEED, 1.0f);
			else                                     ps->transattackfollower = MAX(ps->transattackfollower - TRANS_ENV_FOLLOWER_SPEED, 0.0f);

			if (ps->transattackhold) /* conditional is unnecessary but it guards against running even more unnecessary conditionals :3 */
				ps->transattackhighest = MAX(ps->transattackhighest, ps->transattackfollower);

			if (ps->transattackfollower < ps->transattackhighest - TRANS_ENV_FOLLOWER_RELTHRES)
				ps->transattackhold = 0;

			if (!ps->transattackhold && envtarget - (1.0f - ((float)s->transientsensitivity)*DIV255)*TRANS_ENV_FOLLOWER_MAX > ps->transattackfollower)
			{
				attack = 1;
				ps->transattackhold = 1;
				ps->transattackhighest = ps->transattackfollower;
			}
		}

	/* avoid getting stuck in an infinite cycle if cycles are wanted */
	if (cyclelength && ps->nextcyclestart < ps->pitchedpointer)
		attack = 1;

	if (!pointer || ps->pitchedpointer == ps->nextcyclestart || attack)
	{
		ps->nextcyclestart = ps->pitchedpointer + cyclelength + cyclelength*(rand()%(s->cyclelengthjitter+1)*DIV256);

		ps->oldrawgrainoffset = ps->rawgrainoffset;
		ps->oldgrainoffset = ps->grainoffset;

		ps->rawgrainoffset = ps->pitchedpointer;
		ps->grainoffset = framegrainoffset;

		if (!((uint32_t)ps->pitchedpointer)) /* don't ramp in the very first grain */
			ps->grainrampindex = grainrampmax;
		else
			ps->grainrampindex = 0;
	}

	trimloop(((ps->grainoffset * calctime) + ((ps->pitchedpointer - ps->rawgrainoffset) * calcshiftl)) * calcrate, length, loop, cv, localrateredux, s, 0, l);
	trimloop(((ps->grainoffset * calctime) + ((ps->pitchedpointer - ps->rawgrainoffset) * calcshiftr)) * calcrate, length, loop, cv, localrateredux, s, 1, r);
	if (ps->grainrampindex < grainrampmax)
	{
		short rl = 0, rr = 0;
		trimloop(((ps->oldgrainoffset * calctime) + ((ps->pitchedpointer - ps->oldrawgrainoffset) * calcshiftl)) * calcrate, length, loop, cv, localrateredux, s, 0, &rl);
		trimloop(((ps->oldgrainoffset * calctime) + ((ps->pitchedpointer - ps->oldrawgrainoffset) * calcshiftr)) * calcrate, length, loop, cv, localrateredux, s, 1, &rr);

		float gain = (float)ps->grainrampindex / (float)grainrampmax;
		*l = *l*gain + rl*(1.0f - gain);
		*r = *r*gain + rr*(1.0f - gain);
		ps->grainrampindex++;
	}

	short hold;
	switch (s->channelmode)
	{
		case SAMPLE_CHANNELS_STEREO: break;
		case SAMPLE_CHANNELS_LEFT:   *r = *l; break;
		case SAMPLE_CHANNELS_RIGHT:  *l = *r; break;
		case SAMPLE_CHANNELS_MIX:    *l = *r = ((*l>>1) + (*r>>1)); break;
		case SAMPLE_CHANNELS_SWAP:   hold = *l; *l = *r; *r = hold; break;
		default: break;
	}

	if ((*s->sample)[ps->sampleslot]->invert)
	{
		*l *= -1;
		*r *= -1;
	}

	*l *= ps->envgain;
	*r *= ps->envgain;

	float fgain = powf(2, s->gain>>4);
	*l *= fgain;
	*r *= fgain;

	if (cv->reverse) ps->pitchedpointer -= pitchmod;
	else             ps->pitchedpointer += pitchmod;
}

static void samplerLookback(Inst *iv, Track *cv, uint16_t *spr)
{
	InstSamplerState *s = iv->state;
	InstSamplerPlaybackState *ps = cv->inststate;

	Envelope env;
	env.adsr = s->envelope; if (ps->localenvelope != -1) env.adsr = ps->localenvelope;
	applyEnvelopeControlChanges(&env);
	env.pointer = cv->pointer;
	env.output = ps->envgain;
	env.release = cv->release;
	for (uint16_t sprp = 0; sprp < *spr; sprp++)
		envelope(&env);

	ps->envgain = env.output;

	if (cv->reverse)
	{
		if (ps->pitchedpointer > *spr) ps->pitchedpointer -= *spr;
		else                           ps->pitchedpointer = 0;
	} else ps->pitchedpointer += *spr;
}
