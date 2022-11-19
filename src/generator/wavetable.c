void drawWavetable(ControlState *cc, Instrument *iv)
{
	short x = INSTRUMENT_INDEX_COLS + ((ws.ws_col - INSTRUMENT_INDEX_COLS - 38)>>1) +1;
	short y = ws.ws_row - 12;

	drawWaveform(iv, x, y);
	printf("\033[%d;%dH + [          ] +   [                ]", y+4,  x);
	printf("\033[%d;%dHunison:  [ ][ ][ ]                    ", y+5,  x);
	printf("\033[%d;%dHoffset: [        ]                    ", y+6,  x);
	printf("\033[%d;%dHframe:  [        ]                    ", y+7,  x);
	printf("\033[%d;%dH                                      ", y+8,  x);
	printf("\033[%d;%dHenvelope:   [    ]                    ", y+9,  x);
	printf("\033[%d;%dHlfo speed:    [  ]                    ", y+10, x);
	printf("\033[%d;%dHlfo shape [   ][ ]                    ", y+11, x);

	iv->wavetable.framelength = MIN(iv->wavetable.framelength, iv->sample->length-1);
	addControlInt(cc, x+9,  y+6,  &iv->trimstart, 8, 0x0, iv->sample->length-1, 0, 0, 0, instrumentSamplerControlCallback, NULL);
	addControlInt(cc, x+9,  y+7,  &iv->wavetable.framelength, 8, 0x0, iv->sample->length-1, iv->sample->length-1, 0, 0, instrumentSamplerControlCallback, NULL);

	addControlInt(cc, x+13, y+9,  &iv->wavetable.envelope, 4, 0x0,  0xffff, 0x00f0, 0, 0, instrumentSamplerControlCallback, NULL);
	addControlInt(cc, x+15, y+10, &iv->wavetable.lfospeed, 2, 0x0,  0xff,   0,      0, 0, instrumentSamplerControlCallback, NULL);
	addControlInt(cc, x+11, y+11, &iv->wavetable.lfoduty,  3, -128, 127,    0,      0, 0, instrumentSamplerControlCallback, NULL);
	addControlInt(cc, x+16, y+11, &iv->wavetable.lfoshape, 0, 0,    1,      0,      0, 0, instrumentSamplerControlCallback, NULL);

	addControlInt(cc, x+21, y+4, &w->wtparam, 1, 0, 8, 0, 16, 9, instrumentSamplerControlCallback, NULL);
		addScalePointInt(cc, "   WAVETABLE POS", 0);
		addScalePointInt(cc, "            GAIN", 1);
		addScalePointInt(cc, " OSCILLATOR SYNC", 2);
		addScalePointInt(cc, "          FILTER", 3);
		addScalePointInt(cc, "PHASE MODULATION", 4);
		addScalePointInt(cc, "FREQ. MODULATION", 5);
		addScalePointInt(cc, "     PULSE WIDTH", 6);
		addScalePointInt(cc, "  PHASE DYNAMICS", 7);
		addScalePointInt(cc, "           MIXER", 8);

	/* switch (w->wtparam)
	{
		case WT_PARAM_WTPOS:
			printf("\033[%d;%dHoffset:      [   ]", y+5,  x+21);
			printf("\033[%d;%dHlfo mod:     [   ]", y+6,  x+21);
			printf("\033[%d;%dHenv mod:     [   ]", y+7,  x+21);
			printf("\033[%d;%dHgainenv mod: [   ]", y+8,  x+21);
			printf("\033[%d;%dH                  ", y+9,  x+21);
			printf("\033[%d;%dH                  ", y+10, x+21);
			printf("\033[%d;%dH                  ", y+11, x+21);

			addControlInt(cc, x+35, y+5, &iv->wavetable.wtpos.offset,  3, -128, 127, 0, 0, instrumentSamplerControlCallback, NULL);
			addControlInt(cc, x+35, y+6, &iv->wavetable.wtpos.lfo,     3, -128, 127, 0, 0, instrumentSamplerControlCallback, NULL);
			addControlInt(cc, x+35, y+7, &iv->wavetable.wtpos.env,     3, -128, 127, 0, 0, instrumentSamplerControlCallback, NULL);
			addControlInt(cc, x+35, y+8, &iv->wavetable.wtpos.gainenv, 3, -128, 127, 0, 0, instrumentSamplerControlCallback, NULL);
			break;
	} */
}

#define LFO_MAX_S 0.1f
#define LFO_MIN_S 10.0f

void processWavetable(Instrument *iv, Track *cv, float rp, uint32_t pointer, uint32_t pitchedpointer, short *l, short *r)
{
	float hold;
	float calcrate = (float)iv->sample->rate / (float)samplerate;
	float calcpitch = powf(M_12_ROOT_2, (short)cv->samplernote - NOTE_C5 + cv->finetune);

	// uint16_t env = iv->envelope; if (cv->localenvelope != -1) env = cv->localenvelope;
	/* mod envelope */
	envelope(iv->wavetable.envelope, cv, pointer, &cv->modenvgain);
	/* mod lfo */
	uint32_t lfolen = MAX(1, (uint32_t)(LFO_MAX_S * samplerate + (1.0f - (iv->wavetable.lfospeed*DIV255)) * samplerate * LFO_MIN_S));
	float lfophase = (float)(pointer % lfolen) / lfolen;
	hold = (iv->wavetable.lfoduty + 127.0f)*DIV255;
	float lfogain;
	if (iv->wavetable.lfoshape)
	{ /* pulse wave */
		if (lfophase < hold) lfogain = 1.0f;
		else                 lfogain = 0.0f;
	} else
	{ /* linear wave */
		if (lfophase < hold) lfogain = lfophase / hold;
		else                 lfogain = (1.0f-lfophase) / (1.0f-hold);
	}

	/* sync */
	uint32_t framelen = iv->wavetable.framelength / calcrate;
	framelen = MAX(1, framelen);

	uint32_t synclen = framelen * powf(2.0f, (iv->wavetable.syncoffset + cv->modenvgain * iv->wavetable.env.sync + lfogain * iv->wavetable.lfo.sync)*DIV128);
	synclen = MAX(1, synclen);

	float wtphase = MIN(
			fmodf(pitchedpointer*calcpitch, framelen) / framelen,
			fmodf(pitchedpointer*calcpitch, synclen) / synclen);

	/* pwm */
	hold = (iv->wavetable.pulsewidth + 127.0f)*DIV255 + (cv->modenvgain * iv->wavetable.env.pwm + lfogain * iv->wavetable.lfo.pwm)*DIV128;
	hold = MAX(MIN(hold, 1.0f), 0.0f);
	if (wtphase < hold) wtphase = wtphase / (hold*2.0f);
	else                wtphase = 0.5f + (1.0f - (1.0f - wtphase) / (1.0f - hold)) * 0.5f;

	/* phase dynamics */
	hold = (iv->wavetable.phasedynamics + 127.0f)*DIV255 + (cv->modenvgain * iv->wavetable.env.pdyn + lfogain * iv->wavetable.lfo.pdyn)*DIV128;
	if (wtphase < 0.5f) wtphase = wtphase * hold * 2.0f;
	else                wtphase = 1.0f - ((1.0f - wtphase) * hold * 2.0f);

	/* phase modulation */
	hold = (cv->modenvgain * iv->wavetable.env.phase + lfogain * iv->wavetable.lfo.phase)*DIV512;
	wtphase = fmodf(wtphase + hold, 1.0f);

	/* wavetable pos */
	uint32_t pointersnap = iv->trimstart + MIN((iv->sample->length - iv->trimstart) / framelen - 1, MAX(0, (short)iv->wavetable.wtpos + (short)((cv->modenvgain * iv->wavetable.env.wtpos + lfogain * iv->wavetable.lfo.wtpos) * 2.0f))) * framelen;
	uint8_t localsamplerate = iv->samplerate; if (cv->localsamplerate != -1) localsamplerate = cv->localsamplerate;
	if (cv->targetlocalsamplerate != -1) localsamplerate += (cv->targetlocalsamplerate - localsamplerate) * rp;

	if (iv->sample->tracks == 1)
	{
		getSample(pointersnap + (uint32_t)(wtphase*framelen) *calcrate, localsamplerate, iv->bitdepth, iv->sample, l);
		getSample(pointersnap + (uint32_t)(wtphase*framelen) *calcrate, localsamplerate, iv->bitdepth, iv->sample, r);
	} else
	{
		getSample((pointersnap + (uint32_t)(wtphase*framelen)) *calcrate * iv->sample->tracks + 0, localsamplerate, iv->bitdepth, iv->sample, l);
		getSample((pointersnap + (uint32_t)(wtphase*framelen)) *calcrate * iv->sample->tracks + 1, localsamplerate, iv->bitdepth, iv->sample, r);
	}

	hold = powf(2.0f, lfogain * iv->wavetable.lfo.gain*DIV256 * -1.0f);
	*l *= hold;
	*r *= hold;
}
