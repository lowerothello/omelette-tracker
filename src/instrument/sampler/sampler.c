void samplerInit(Inst *iv)
{
	iv->type = INST_TYPE_SAMPLER;

	InstSamplerState *s = iv->state = calloc(1, sizeof(InstSamplerState));
	s->sample = calloc(1, sizeof(SampleChain));

	s->samplerate = 0xff;
	s->bitdepth = 0xf;
	s->envelope = 0x00f0;

	s->granular.cyclelength = 0x3fff;
	s->granular.rampgrains = 8;
	s->granular.beatsensitivity = 0x80;
	s->granular.beatdecay = 0xff;
}

void samplerFree(Inst *iv)
{
	freeWaveform();

	InstSamplerState *s = iv->state;
	FOR_SAMPLECHAIN(i, s->sample)
		free((*s->sample)[i]);
	free(s->sample);
	free(s);
}

/* dest has already been free'd */
void samplerCopy(Inst *dest, Inst *src)
{
	dest->type = INST_TYPE_SAMPLER;
	InstSamplerState *s = dest->state = calloc(1, sizeof(InstSamplerState));

	memcpy(dest->state, src->state, sizeof(InstSamplerState));

	s->sample = calloc(1, sizeof(SampleChain));
	copySampleChain(s->sample, ((InstSamplerState*)src->state)->sample);
}

void samplerGetIndexInfo(Inst *iv, char *buffer)
{
	uint32_t samplesize = 0;
	InstSamplerState *s = iv->state;
	FOR_SAMPLECHAIN(i, s->sample)
		samplesize +=
				(*s->sample)[i]->length *
				(*s->sample)[i]->channels;
	humanReadableSize(samplesize, buffer);
}

void samplerDraw(Inst *iv, short x, short y, short width, short height, short minx, short maxx)
{
	InstSamplerState *s = iv->state;

	short rows, cols;
	if (!(cols = getMaxInstUICols(&cyclicInstUI, width))) return;
	if (!(rows = getInstUIRows(&cyclicInstUI, cols))) return;

	short wh = MAX(height - rows, INSTUI_WAVEFORM_MIN);

	drawBoundingBox(x, y, width, wh - 1, minx, maxx, 1, ws.ws_row);

	short sample_rows = getInstUIRows(&sampleInstUI, getMaxInstUICols(&sampleInstUI, width - 5));

	short whh = wh - sample_rows;

	/* multisample indices */
	addControlDummy(x + 3, y + (whh>>1));
	char buffer[5];
	short siy;
	for (uint8_t i = 0; i < SAMPLE_MAX; i++)
	{
		siy = y + (whh>>1) - w->sample + i;
		if (siy > y && siy < y + (whh - 1))
		{ /* vertical bounds limiting */
			if (i == w->sample) printf("\033[1;7m");
			if ((*s->sample)[i]) snprintf(buffer, 5, " %01x", i);
			else                 snprintf(buffer, 5, " .");
			printCulling(buffer, x+1, siy, minx, maxx);
			if (i == w->sample) printf("\033[22;27m");
		}
	}

	drawWaveform((*s->sample)[w->sample], x+INSTUI_MULTISAMPLE_WIDTH + 1, y+1, (width-INSTUI_MULTISAMPLE_WIDTH) - 1, whh - 2);
	drawInstUI(&sampleInstUI, iv, x+1, width-2, y + whh - 1, 0, sample_rows);

	y += wh;

	drawInstUI(&cyclicInstUI, iv, x, width, y, 0, rows);
}

void samplerTriggerNote(Inst *iv, Track *cv)
{
	InstSamplerState *s = iv->state;
	cv->sampleslot = s->samplemap[cv->r.note];
}

/* clamps within range and loop, returns output samples */
void trimloop(double ptr, uint32_t length, uint32_t loop, Track *cv, uint8_t decimate, InstSamplerState *s, uint8_t stereotrack, short *output)
{
	if (loop)
	{ /* if there is a loop range */
		if ((*s->sample)[cv->sampleslot]->pingpong)
		{ /* ping-pong loop */
			if (ptr > length)
			{
				ptr = fmod(ptr - length, loop<<1);
				if (ptr > loop) /* walking forwards  */ ptr = (length - loop) + (ptr - loop);
				else            /* walking backwards */ ptr = length - ptr;
			}

			if ((*s->sample)[cv->sampleslot]->channels == 1) getSample(ptr+(*s->sample)[cv->sampleslot]->trimstart, decimate, s->bitdepth, (*s->sample)[cv->sampleslot], output);
			else                                             getSample((ptr+(*s->sample)[cv->sampleslot]->trimstart)*(*s->sample)[cv->sampleslot]->channels + stereotrack, decimate, s->bitdepth, (*s->sample)[cv->sampleslot], output);
		} else
		{ /* crossfaded forwards loop */
			uint32_t looprampmax = MIN(samplerate*DIV1000 * LOOP_RAMP_MS, (loop>>1)) * (*s->sample)[cv->sampleslot]->loopramp*DIV255;
			if (ptr > length) ptr = (length - loop) + looprampmax + fmod(ptr - length, loop - looprampmax);

			if (ptr > length - looprampmax)
			{
				float lerp = (ptr - length + looprampmax) / (float)looprampmax;
				float ramppointer = (ptr - (loop - looprampmax));
				if ((*s->sample)[cv->sampleslot]->channels == 1) getSampleLoopRamp( ptr+(*s->sample)[cv->sampleslot]->trimstart, ramppointer, lerp, decimate, s->bitdepth, (*s->sample)[cv->sampleslot], output);
				else                                              getSampleLoopRamp((ptr+(*s->sample)[cv->sampleslot]->trimstart)*(*s->sample)[cv->sampleslot]->channels + stereotrack, ramppointer*(*s->sample)[cv->sampleslot]->channels + stereotrack, lerp, decimate, s->bitdepth, (*s->sample)[cv->sampleslot], output);
			} else
			{
				if ((*s->sample)[cv->sampleslot]->channels == 1) getSample( ptr+(*s->sample)[cv->sampleslot]->trimstart, decimate, s->bitdepth, (*s->sample)[cv->sampleslot], output);
				else                                              getSample((ptr+(*s->sample)[cv->sampleslot]->trimstart)*(*s->sample)[cv->sampleslot]->channels + stereotrack, decimate, s->bitdepth, (*s->sample)[cv->sampleslot], output);
			}
		}
	} else
	{
		if (ptr < length)
		{
			if ((*s->sample)[cv->sampleslot]->channels == 1) getSample( ptr+(*s->sample)[cv->sampleslot]->trimstart, decimate, s->bitdepth, (*s->sample)[cv->sampleslot], output);
			else                                              getSample((ptr+(*s->sample)[cv->sampleslot]->trimstart)*(*s->sample)[cv->sampleslot]->channels + stereotrack, decimate, s->bitdepth, (*s->sample)[cv->sampleslot], output);
		}
	}
}

void processCyclic(InstSamplerState *s, Track *cv, float rp, uint32_t pointer, uint32_t pitchedpointer, float finetune, short *l, short *r)
{
	uint32_t length = MIN((*s->sample)[cv->sampleslot]->trimlength, (*s->sample)[cv->sampleslot]->length-1 - (*s->sample)[cv->sampleslot]->trimstart);
	uint32_t loop   = MIN((*s->sample)[cv->sampleslot]->looplength, length);
	uint16_t localcyclelength = s->granular.cyclelength; if (cv->localcyclelength != -1) localcyclelength = cv->localcyclelength;

	float f;
	uint16_t localpitchshift;
	f = macroStateGetMono(&cv->pitchshift, rp);
	if (f != NAN)
		localpitchshift = (f - 0.5f) * (127<<9);
	else
		localpitchshift = s->granular.pitchshift;

	uint16_t localpitchwidth;
	f = macroStateGetMono(&cv->pitchwidth, rp);
	if (f != NAN)
		localpitchwidth = (f - 0.5f) * (127<<1);
	else
		localpitchwidth = s->granular.pitchstereo;

	uint8_t localsamplerate;
	f = macroStateGetMono(&cv->samplerate, rp);
	if (f != NAN)
		localsamplerate = f*256;
	else
		localsamplerate = s->samplerate;

	uint16_t cyclelength = MAX(1, samplerate*DIV1000 * TIMESTRETCH_CYCLE_UNIT_MS * MAX(1, localcyclelength));
	uint32_t pointersnap = pointer % cyclelength;

	/* calculate the new grain start and trigger ramping */
	if (!pointersnap)
	{
		cv->grainrampmax = cyclelength*DIV32 * s->granular.rampgrains;
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
	double calcshift = semitoneShortToMultiplier(localpitchshift) / semitoneShortToMultiplier(s->granular.timestretch);
	double calcrate = (float)(*s->sample)[cv->sampleslot]->rate / (float)samplerate * semitoneShortToMultiplier(s->granular.timestretch);
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

		if (cv->grainrampindex < cv->grainrampmax)
		{
			if (s->granular.reversegrains)
			{
				trimloop((pitchedpointer - pointersnap - ((cyclelength + cv->grainrampindex) * calcpitch*calcshift*calcshiftstereol)) * calcrate, length, loop, cv, localsamplerate, s, 0, &rl);
				trimloop((pitchedpointer - pointersnap - ((cyclelength + cv->grainrampindex) * calcpitch*calcshift*calcshiftstereor)) * calcrate, length, loop, cv, localsamplerate, s, 1, &rr);
			} else
			{
				trimloop((pitchedpointer - pointersnap - cyclelength + ((cyclelength + cv->grainrampindex) * calcpitch*calcshift*calcshiftstereol)) * calcrate, length, loop, cv, localsamplerate, s, 0, &rl);
				trimloop((pitchedpointer - pointersnap - cyclelength + ((cyclelength + cv->grainrampindex) * calcpitch*calcshift*calcshiftstereor)) * calcrate, length, loop, cv, localsamplerate, s, 1, &rr);
			}

			gain = (float)cv->grainrampindex / (float)cv->grainrampmax;
			*l = *l * gain + rl * (1.0f - gain);
			*r = *r * gain + rr * (1.0f - gain);
			cv->grainrampindex++;
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

		if (cv->grainrampindex < cv->grainrampmax)
		{
			if (s->granular.reversegrains)
			{
				trimloop((pitchedpointer - pointersnap - ((cyclelength + cv->grainrampindex) * calcshift*calcshiftstereol)) * calcpitch*calcrate, length, loop, cv, localsamplerate, s, 0, &rl);
				trimloop((pitchedpointer - pointersnap - ((cyclelength + cv->grainrampindex) * calcshift*calcshiftstereor)) * calcpitch*calcrate, length, loop, cv, localsamplerate, s, 1, &rr);
			} else
			{
				trimloop((pitchedpointer - pointersnap - cyclelength + ((cyclelength + cv->grainrampindex) * calcshift*calcshiftstereol)) * calcpitch*calcrate, length, loop, cv, localsamplerate, s, 0, &rl);
				trimloop((pitchedpointer - pointersnap - cyclelength + ((cyclelength + cv->grainrampindex) * calcshift*calcshiftstereor)) * calcpitch*calcrate, length, loop, cv, localsamplerate, s, 1, &rr);
			}

			gain = (float)cv->grainrampindex / (float)cv->grainrampmax;
			*l = *l * gain + rl * (1.0f - gain);
			*r = *r * gain + rr * (1.0f - gain);
			cv->grainrampindex++;
		}
	}
}

void samplerProcess(Inst *iv, Track *cv, float rp, uint32_t pointer, uint32_t pitchedpointer, float finetune, short *l, short *r)
{
	InstSamplerState *s = iv->state;

	if (!(*s->sample)[cv->sampleslot]->length) return;

	Envelope env;
	env.adsr = s->envelope; if (cv->localenvelope != -1) env.adsr = cv->localenvelope;
	applyEnvelopeControlChanges(&env);
	env.output = cv->envgain;
	env.release = cv->release;
	env.pointer = pointer;
	/* return if the envelope has finished */
	if (envelope(&env)) return;
	cv->envgain = env.output;

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

	if ((*s->sample)[cv->sampleslot]->invert) { *l *= -1; *r *= -1; }
	*l *= cv->envgain;
	*r *= cv->envgain;

	float fgain = powf(2, (float)s->gain*DIV16);
	*l *= fgain;
	*r *= fgain;
}

void samplerLookback(Inst *iv, Track *cv, uint16_t *spr)
{
	InstSamplerState *s = iv->state;

	Envelope env, wtenv;
	env.adsr = s->envelope; if (cv->localenvelope != -1) env.adsr = cv->localenvelope;
	applyEnvelopeControlChanges(&env);
	wtenv.adsr = s->wavetable.envelope;
	applyEnvelopeControlChanges(&wtenv);
	env.pointer = wtenv.pointer = cv->pointer;
	env.output = cv->envgain;
	wtenv.output = cv->modenvgain;
	env.release = wtenv.release = cv->release;
	for (uint16_t sprp = 0; sprp < *spr; sprp++)
	{
		envelope(&env);
		envelope(&wtenv);
	}
	cv->envgain = env.output;
	cv->modenvgain = wtenv.output;
}

#include "macros.c"
#include "input.c"
