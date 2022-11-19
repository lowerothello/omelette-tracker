void drawCyclic(ControlState *cc, Instrument *iv)
{
	short x = INSTRUMENT_INDEX_COLS + ((ws.ws_col - INSTRUMENT_INDEX_COLS - 38)>>1) +1;
	short y = ws.ws_row - 12;

	drawWaveform(iv, x, y);
	printf("\033[%d;%dH + [          ] +   offset: [        ]", y+4, x);
	printf("\033[%d;%dHunison:  [ ][ ][ ]  length: [        ]", y+5, x);
	printf("\033[%d;%dHrev/ramp:  [  ][ ]  loop:   [        ]", y+6, x);
	printf("\033[%d;%dHcycle:  [    ][  ]  pp/ramp:   [ ][  ]", y+7, x);
	printf("\033[%d;%dHtime:   [ ][     ]                    ", y+8, x);
	printf("\033[%d;%dH                     -    PITCH     - ", y+9, x);
	printf("\033[%d;%dHpan jitter:   [  ]  root: [   ][     ]", y+10, x);
	printf("\033[%d;%dHptr jitter:   [  ]  jitter: [  ][ ][ ]", y+11, x);

	/* granular */
	addControlInt(cc, x+12, y+6, &iv->granular.reversegrains,     2, 0x0,    0xff,   0,      0, 0, instrumentSamplerControlCallback, NULL);
	addControlInt(cc, x+16, y+6, &iv->granular.rampgrains,        1, 0x0,    0xf,    8,      0, 0, instrumentSamplerControlCallback, NULL);
	addControlInt(cc, x+9,  y+7, &iv->granular.cyclelength,       4, 0x0,    0xffff, 0x3fff, 0, 0, instrumentSamplerControlCallback, NULL);
	addControlInt(cc, x+15, y+7, &iv->granular.cyclelengthjitter, 2, 0x0,    0xff,   0x0,    0, 0, instrumentSamplerControlCallback, NULL);
	addControlInt(cc, x+9,  y+8, &iv->granular.notestretch,       0, 0,      1,      0,      0, 0, instrumentSamplerControlCallback, NULL);
	addControlInt(cc, x+12, y+8, &iv->granular.timestretch,       5, 0x8bff, 0x7bff, 0,      0, 0, instrumentSamplerControlCallback, NULL);

	/* range */
	iv->trimlength = MIN(iv->trimlength, iv->sample->length-1);
	iv->looplength = MIN(iv->looplength, iv->sample->length-1);
	addControlInt(cc, x+29, y+4, &iv->trimstart,  8, 0x0, iv->sample->length-1,                    0, 0, 0, instrumentSamplerControlCallback, NULL);
	addControlInt(cc, x+29, y+5, &iv->trimlength, 8, 0x0, iv->sample->length-1, iv->sample->length-1, 0, 0, instrumentSamplerControlCallback, NULL);
	addControlInt(cc, x+29, y+6, &iv->looplength, 8, 0x0, iv->trimlength,                          0, 0, 0, instrumentSamplerControlCallback, NULL);
	addControlInt(cc, x+32, y+7, &iv->pingpong,   0, 0,   1,    0, 0, 0, instrumentSamplerControlCallback, NULL);
	addControlInt(cc, x+35, y+7, &iv->loopramp,   2, 0x0, 0xff, 0, 0, 0, instrumentSamplerControlCallback, NULL);

	/* jitter */
	addControlInt(cc, x+15, y+10, &iv->granular.panjitter, 2, 0x00, 0xff, 0, 0, 0, instrumentSamplerControlCallback, NULL);
	addControlInt(cc, x+15, y+11, &iv->granular.panjitter, 2, 0x00, 0xff, 0, 0, 0, instrumentSamplerControlCallback, NULL);

	/* pitch */
	addControlInt(cc, x+27, y+10, &iv->granular.pitchstereo,       3, -128,   127,    0, 0, 0, instrumentSamplerControlCallback, NULL);
	addControlInt(cc, x+32, y+10, &iv->granular.pitchshift,        5, 0x8bff, 0x7bff, 0, 0, 0, instrumentSamplerControlCallback, NULL);
	addControlInt(cc, x+29, y+11, &iv->granular.pitchjitter,       2, 0x00,   0xff,   0, 0, 0, instrumentSamplerControlCallback, NULL);
	addControlInt(cc, x+33, y+11, &iv->granular.pitchoctaverange,  1, 0x0,    0xf,    0, 0, 0, instrumentSamplerControlCallback, NULL);
	addControlInt(cc, x+36, y+11, &iv->granular.pitchoctavechance, 1, 0x0,    0xf,    0, 0, 0, instrumentSamplerControlCallback, NULL);
}

void processCyclic(Instrument *iv, Track *cv, float rp, uint32_t pointer, uint32_t pitchedpointer, short *l, short *r)
{
	uint32_t length = MIN(iv->trimlength, iv->sample->length-1 - iv->trimstart);
	uint32_t loop   = MIN(iv->looplength, length);
	uint16_t localcyclelength = iv->granular.cyclelength; if (cv->localcyclelength != -1) localcyclelength = cv->localcyclelength;
	int16_t  localpitchshift =  iv->granular.pitchshift;  if (cv->localpitchshift  != -1) localpitchshift  = (cv->localpitchshift - 0x80)<<8;
	int16_t  localpitchwidth =  iv->granular.pitchstereo; if (cv->localpitchwidth  != -1) localpitchwidth  = cv->localpitchwidth - 0x80;
	if (cv->targetlocalpitchshift != -1) localpitchshift += (((cv->targetlocalpitchshift - 0x80)<<8) - localpitchshift) * rp;
	if (cv->targetlocalpitchwidth != -1) localpitchwidth +=  ((cv->targetlocalpitchwidth - 0x80)     - localpitchwidth) * rp;

	uint8_t localsamplerate = iv->samplerate; if (cv->localsamplerate != -1) localsamplerate = cv->localsamplerate;
	if (cv->targetlocalsamplerate != -1) localsamplerate += (cv->targetlocalsamplerate - localsamplerate) * rp;

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
	double calcrate = (float)iv->sample->rate / (float)samplerate * semitoneShortToMultiplier(iv->granular.timestretch);
	double calcpitch = powf(M_12_ROOT_2, (short)cv->samplernote - NOTE_C5 + cv->finetune);
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
