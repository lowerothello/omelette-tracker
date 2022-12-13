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
	if (ptr <= (s->length-1) * s->tracks) *output += (s->data[ptr]>>shift)<<shift;
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

			if (iv->sample->tracks == 1) getSample(ptr+iv->trimstart, decimate, iv->bitdepth, iv->sample, output);
			else                           getSample((ptr+iv->trimstart)*iv->sample->tracks + stereotrack, decimate, iv->bitdepth, iv->sample, output);
		} else
		{ /* crossfaded forwards loop */
			uint32_t looprampmax = MIN(samplerate*DIV1000 * LOOP_RAMP_MS, (loop>>1)) * iv->loopramp*DIV255;
			if (ptr > length) ptr = (length - loop) + looprampmax + fmod(ptr - length, loop - looprampmax);

			if (ptr > length - looprampmax)
			{
				float lerp = (ptr - length + looprampmax) / (float)looprampmax;
				float ramppointer = (ptr - (loop - looprampmax));
				if (iv->sample->tracks == 1) getSampleLoopRamp( ptr+iv->trimstart, ramppointer, lerp, decimate, iv->bitdepth, iv->sample, output);
				else                           getSampleLoopRamp((ptr+iv->trimstart)*iv->sample->tracks + stereotrack,
				                                                         ramppointer*iv->sample->tracks + stereotrack, lerp, decimate, iv->bitdepth, iv->sample, output);
			} else
			{
				if (iv->sample->tracks == 1) getSample( ptr+iv->trimstart, decimate, iv->bitdepth, iv->sample, output);
				else                           getSample((ptr+iv->trimstart)*iv->sample->tracks + stereotrack, decimate, iv->bitdepth, iv->sample, output);
			}
		}
	} else
	{
		if (ptr < length)
		{
			if (iv->sample->tracks == 1) getSample( ptr+iv->trimstart, decimate, iv->bitdepth, iv->sample, output);
			else                           getSample((ptr+iv->trimstart)*iv->sample->tracks + stereotrack, decimate, iv->bitdepth, iv->sample, output);
		}
	}
}

float semitoneShortToMultiplier(int16_t input)
{
	if (input < 0) return powf(M_12_ROOT_2, -((abs(input)>>12)*12 + (abs(input)&0x0fff)*DIV256));
	else           return powf(M_12_ROOT_2, (input>>12)*12 + (input&0x0fff)*DIV256);
}

#define INSTRUMENT_SAMPLER_WIDTH 38
void drawInstrumentSampler(ControlState *cc, Instrument *iv, short x, short width)
{
	short y = ws.ws_row - 12;

	short xx = x + ((width - INSTRUMENT_SAMPLER_WIDTH)>>1);

	if (iv->algorithm == INST_ALG_MIDI) drawMidi(cc, iv, xx);
	else
	{
		if (w->showfilebrowser)
			drawBrowser(fbstate);
		else
		{
			clearControls(cc);
			drawWaveform(iv);
			printf("\033[%d;%dHC5 rate: [        ]  channel: [      ]", y+0, xx);
			printf("\033[%d;%dHquality: [ ][ ][  ]  gain:    [ ][   ]", y+1, xx);
			printf("\033[%d;%dHgain env:    [    ]  [    ]:  [  ][  ]", y+2, xx);

			addControlInt(cc, xx+10, y+0, &iv->sample->rate, 8, 0x0, 0xffffffff, iv->sample->defrate, 0, 0, instrumentSamplerControlCallback, NULL);
			addControlInt(cc, xx+10, y+1, &iv->interpolate, 0, 0,   1,      0,      0, 0, instrumentSamplerControlCallback, NULL);
			addControlInt(cc, xx+13, y+1, &iv->bitdepth,    1, 0x0, 0xf,    0xf,    0, 0, instrumentSamplerControlCallback, NULL);
			addControlInt(cc, xx+16, y+1, &iv->samplerate,  2, 0x0, 0xff,   0xff,   0, 0, instrumentSamplerControlCallback, NULL);
			addControlInt(cc, xx+14, y+2, &iv->envelope,    4, 0x0, 0xffff, 0x00f0, 0, 0, instrumentSamplerControlCallback, NULL);
			addControlInt(cc, xx+31, y+0, &iv->channelmode, 1, 0,   4,      0,      6, 5, instrumentSamplerControlCallback, NULL);
				addScalePointInt(cc, "STEREO", SAMPLE_CHANNELS_STEREO);
				addScalePointInt(cc, "  LEFT", SAMPLE_CHANNELS_LEFT  );
				addScalePointInt(cc, " RIGHT", SAMPLE_CHANNELS_RIGHT );
				addScalePointInt(cc, "   MIX", SAMPLE_CHANNELS_MIX   );
				addScalePointInt(cc, "  SWAP", SAMPLE_CHANNELS_SWAP  );
			addControlInt(cc, xx+31, y+1, &iv->invert,     0, 0,    1,   0, 0, 0, instrumentSamplerControlCallback, NULL);
			addControlInt(cc, xx+34, y+1, &iv->gain,       3, -128, 127, 0, 0, 0, instrumentSamplerControlCallback, NULL);
			addControlInt(cc, xx+22, y+2, &iv->filtermode, 1, 0,    7,   0, 4, 8, instrumentSamplerControlCallback, NULL);
				addScalePointInt(cc, "LP12", FILTER_MODE_LP12);
				addScalePointInt(cc, "HP12", FILTER_MODE_HP12);
				addScalePointInt(cc, "BP12", FILTER_MODE_BP12);
				addScalePointInt(cc, "NT12", FILTER_MODE_NT12);
				addScalePointInt(cc, "LP24", FILTER_MODE_LP24);
				addScalePointInt(cc, "HP24", FILTER_MODE_HP24);
				addScalePointInt(cc, "BP24", FILTER_MODE_BP24);
				addScalePointInt(cc, "NT24", FILTER_MODE_NT24);
			addControlInt(cc, xx+31, y+2, &iv->filtercutoff,    2, 0x0, 0xff, 0xff, 0, 0, instrumentSamplerControlCallback, NULL);
			addControlInt(cc, xx+35, y+2, &iv->filterresonance, 2, 0x0, 0xff, 0x0,  0, 0, instrumentSamplerControlCallback, NULL);

			addControlInt(cc, xx+4, y+4, &iv->algorithm, 1, 0, 4, 1, 10, 5, instrumentSamplerControlCallback, NULL);
				addScalePointInt(cc, "   MINIMAL", INST_ALG_SIMPLE   );
				addScalePointInt(cc, "    CYCLIC", INST_ALG_CYCLIC   );
				addScalePointInt(cc, "     TONAL", INST_ALG_TONAL    );
				addScalePointInt(cc, "      BEAT", INST_ALG_BEAT     );
				addScalePointInt(cc, " WAVETABLE", INST_ALG_WAVETABLE);

			switch (iv->algorithm)
			{
				case INST_ALG_SIMPLE: printf("\033[%d;%dH + [          ] + ", y+4, xx); break;
				case INST_ALG_CYCLIC:    drawCyclic   (cc, iv, xx); break;
				case INST_ALG_TONAL:     drawTonal    (cc, iv, xx); break;
				case INST_ALG_BEAT:      drawBeat     (cc, iv, xx); break;
				case INST_ALG_WAVETABLE: drawWavetable(cc, iv, xx); break;
			}

			drawControls(cc);
		}
	}
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
		case INST_ALG_CYCLIC:    processCyclic   (iv, cv, rp, pointer, pitchedpointer, l, r); break;
		case INST_ALG_WAVETABLE: processWavetable(iv, cv, rp, pointer, pitchedpointer, l, r); break;
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
