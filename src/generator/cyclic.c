void processCyclic(Instrument *iv, Track *cv, float rp, uint32_t pointer, uint32_t pitchedpointer, float finetune, short *l, short *r)
{
	uint32_t length = MIN(iv->sample[cv->sampleslot]->trimlength, iv->sample[cv->sampleslot]->length-1 - iv->sample[cv->sampleslot]->trimstart);
	uint32_t loop   = MIN(iv->sample[cv->sampleslot]->looplength, length);
	uint16_t localcyclelength = iv->granular.cyclelength; if (cv->localcyclelength != -1) localcyclelength = cv->localcyclelength;

	float f;
	uint16_t localpitchshift;
	f = macroStateGetMono(&cv->pitchshift, rp);
	if (f != NAN)
		localpitchshift = (f - 0.5f) * (127<<9);
	else
		localpitchshift = iv->granular.pitchshift;

	uint16_t localpitchwidth;
	f = macroStateGetMono(&cv->pitchwidth, rp);
	if (f != NAN)
		localpitchwidth = (f - 0.5f) * (127<<1);
	else
		localpitchwidth = iv->granular.pitchstereo;

	uint8_t localsamplerate;
	f = macroStateGetMono(&cv->samplerate, rp);
	if (f != NAN)
		localsamplerate = f*256;
	else
		localsamplerate = iv->samplerate;

	uint16_t cyclelength = MAX(1, samplerate*DIV1000 * TIMESTRETCH_CYCLE_UNIT_MS * MAX(1, localcyclelength));
	uint32_t pointersnap = pointer % cyclelength;

	/* calculate the new grain start and trigger ramping */
	if (!pointersnap)
	{
		cv->grainrampmax = cyclelength*DIV32 * iv->granular.rampgrains;
		if (!pointer) /* don't ramp in the very first grain */
			cv->grainrampindex = cv->grainrampmax;
		else cv->grainrampindex = 0;
	}

	short rl = 0;
	short rr = 0;

	float gain;

	/* use doubles to try to avoid the worst of float approximation bullshit */
	double calcshiftstereol = powf(2.0f, (float)(-localpitchwidth)*DIV1024);
	double calcshiftstereor = powf(2.0f, (float)(+localpitchwidth)*DIV1024);
	double calcshift = semitoneShortToMultiplier(localpitchshift) / semitoneShortToMultiplier(iv->granular.timestretch);
	double calcrate = (float)iv->sample[cv->sampleslot]->rate / (float)samplerate * semitoneShortToMultiplier(iv->granular.timestretch);
	double calcpitch = powf(M_12_ROOT_2, (short)cv->r.note - NOTE_C5 + finetune);
	if (iv->granular.notestretch)
	{ /* note stretch */
		if (iv->granular.reversegrains)
		{
			trimloop((pitchedpointer+cyclelength - pointersnap - (pointersnap * calcpitch*calcshift*calcshiftstereol)) * calcrate, length, loop, cv, localsamplerate, iv, 0, l);
			trimloop((pitchedpointer+cyclelength - pointersnap - (pointersnap * calcpitch*calcshift*calcshiftstereor)) * calcrate, length, loop, cv, localsamplerate, iv, 1, r);
		} else
		{
			trimloop((pitchedpointer - pointersnap + (pointersnap * calcpitch*calcshift*calcshiftstereol)) * calcrate, length, loop, cv, localsamplerate, iv, 0, l);
			trimloop((pitchedpointer - pointersnap + (pointersnap * calcpitch*calcshift*calcshiftstereor)) * calcrate, length, loop, cv, localsamplerate, iv, 1, r);
		}

		if (cv->grainrampindex < cv->grainrampmax)
		{
			if (iv->granular.reversegrains)
			{
				trimloop((pitchedpointer - pointersnap - ((cyclelength + cv->grainrampindex) * calcpitch*calcshift*calcshiftstereol)) * calcrate, length, loop, cv, localsamplerate, iv, 0, &rl);
				trimloop((pitchedpointer - pointersnap - ((cyclelength + cv->grainrampindex) * calcpitch*calcshift*calcshiftstereor)) * calcrate, length, loop, cv, localsamplerate, iv, 1, &rr);
			} else
			{
				trimloop((pitchedpointer - pointersnap - cyclelength + ((cyclelength + cv->grainrampindex) * calcpitch*calcshift*calcshiftstereol)) * calcrate, length, loop, cv, localsamplerate, iv, 0, &rl);
				trimloop((pitchedpointer - pointersnap - cyclelength + ((cyclelength + cv->grainrampindex) * calcpitch*calcshift*calcshiftstereor)) * calcrate, length, loop, cv, localsamplerate, iv, 1, &rr);
			}

			gain = (float)cv->grainrampindex / (float)cv->grainrampmax;
			*l = *l * gain + rl * (1.0f - gain);
			*r = *r * gain + rr * (1.0f - gain);
			cv->grainrampindex++;
		}
	} else
	{ /* no note stretch */
		if (iv->granular.reversegrains)
		{
			trimloop((pitchedpointer+cyclelength - pointersnap - (pointersnap * calcshift*calcshiftstereol)) * calcpitch*calcrate, length, loop, cv, localsamplerate, iv, 0, l);
			trimloop((pitchedpointer+cyclelength - pointersnap - (pointersnap * calcshift*calcshiftstereor)) * calcpitch*calcrate, length, loop, cv, localsamplerate, iv, 1, r);
		} else
		{
			trimloop((pitchedpointer - pointersnap + (pointersnap * calcshift*calcshiftstereol)) * calcpitch*calcrate, length, loop, cv, localsamplerate, iv, 0, l);
			trimloop((pitchedpointer - pointersnap + (pointersnap * calcshift*calcshiftstereor)) * calcpitch*calcrate, length, loop, cv, localsamplerate, iv, 1, r);
		}

		if (cv->grainrampindex < cv->grainrampmax)
		{
			if (iv->granular.reversegrains)
			{
				trimloop((pitchedpointer - pointersnap - ((cyclelength + cv->grainrampindex) * calcshift*calcshiftstereol)) * calcpitch*calcrate, length, loop, cv, localsamplerate, iv, 0, &rl);
				trimloop((pitchedpointer - pointersnap - ((cyclelength + cv->grainrampindex) * calcshift*calcshiftstereor)) * calcpitch*calcrate, length, loop, cv, localsamplerate, iv, 1, &rr);
			} else
			{
				trimloop((pitchedpointer - pointersnap - cyclelength + ((cyclelength + cv->grainrampindex) * calcshift*calcshiftstereol)) * calcpitch*calcrate, length, loop, cv, localsamplerate, iv, 0, &rl);
				trimloop((pitchedpointer - pointersnap - cyclelength + ((cyclelength + cv->grainrampindex) * calcshift*calcshiftstereor)) * calcpitch*calcrate, length, loop, cv, localsamplerate, iv, 1, &rr);
			}

			gain = (float)cv->grainrampindex / (float)cv->grainrampmax;
			*l = *l * gain + rl * (1.0f - gain);
			*r = *r * gain + rr * (1.0f - gain);
			cv->grainrampindex++;
		}
	}
}

InstUI *initInstUICyclic(void)
{
	InstUI *iui = allocInstUI(4);
	iui->width = INSTUI_SAMPLER_WIDTH;
	initInstUICommonSamplerBlock(&iui->block[0]);
	initInstUIGranularSamplerBlock(&iui->block[1]);
	initInstUIRangeSamplerBlock(&iui->block[2]);
	initInstUIPitchSamplerBlock(&iui->block[3]);
	iui->flags |= INSTUI_DRAWWAVEFORM;
	return iui;
}
