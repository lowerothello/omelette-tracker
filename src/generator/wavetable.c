#define LFO_MAX_S 0.1f
#define LFO_MIN_S 10.0f

void wavetableProcess(Instrument *iv, Channel *cv, float rp, uint32_t pointer, uint32_t pitchedpointer, short *l, short *r)
{
	float hold;
	float calcrate = (float)iv->sample.rate / (float)samplerate;

	/* lfo */
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
	uint32_t synclen = framelen * powf(2.0f, (iv->wavetable.syncoffset + cv->envgain * iv->wavetable.env.sync + lfogain * iv->wavetable.lfo.sync)*DIV128);
	synclen = MAX(1, synclen);
	float wtphase = MIN((float)(pitchedpointer % framelen) / framelen, (float)(pitchedpointer % synclen) / synclen);

	/* pwm */
	hold = (iv->wavetable.pulsewidth + 127.0f)*DIV255 + (cv->envgain * iv->wavetable.env.pwm + lfogain * iv->wavetable.lfo.pwm)*DIV128;
	hold = MAX(MIN(hold, 1.0f), 0.0f);
	if (wtphase < hold) wtphase = wtphase / (hold*2.0f);
	else                wtphase = 0.5f + (1.0f - (1.0f - wtphase) / (1.0f - hold)) * 0.5f;

	/* phase dynamics */
	hold = (iv->wavetable.phasedynamics + 127.0f)*DIV255 + (cv->envgain * iv->wavetable.env.pdyn + lfogain * iv->wavetable.lfo.pdyn)*DIV128;
	if (wtphase < 0.5f) wtphase = wtphase * hold * 2.0f;
	else                wtphase = 1.0f - ((1.0f - wtphase) * hold * 2.0f);

	/* phase modulation */
	hold = (cv->envgain * iv->wavetable.env.phase + lfogain * iv->wavetable.lfo.phase)*DIV512;
	wtphase = fmodf(wtphase + hold, 1.0f);

	/* wavetable pos */
	uint32_t pointersnap = iv->trimstart + MIN((iv->sample.length - iv->trimstart) / framelen - 1, MAX(0, (short)iv->wavetable.wtpos + (short)((cv->envgain * iv->wavetable.env.wtpos + lfogain * iv->wavetable.lfo.wtpos) * 2.0f))) * framelen;
	uint8_t localsamplerate = iv->samplerate; if (cv->localsamplerate != -1) localsamplerate = cv->localsamplerate;
	if (cv->targetlocalsamplerate != -1) localsamplerate += (cv->targetlocalsamplerate - localsamplerate) * rp;

	if (iv->sample.channels == 1)
	{
		getSample( pointersnap + (uint32_t)(wtphase*framelen) * calcrate, localsamplerate, iv, l);
		getSample( pointersnap + (uint32_t)(wtphase*framelen) * calcrate, localsamplerate, iv, r);
	} else
	{
		getSample((pointersnap + (uint32_t)(wtphase*framelen)) * calcrate * iv->sample.channels + 0, localsamplerate, iv, l);
		getSample((pointersnap + (uint32_t)(wtphase*framelen)) * calcrate * iv->sample.channels + 1, localsamplerate, iv, r);
	}

	hold = powf(2.0f, lfogain * iv->wavetable.lfo.gain*DIV256 * -1.0f);
	*l *= hold;
	*r *= hold;
}
