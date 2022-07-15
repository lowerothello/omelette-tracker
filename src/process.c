#define PLAY_LOCK_OK 0    /* p->s and p->w are safe */
#define PLAY_LOCK_START 1 /* p->s and p->w want to be unsafe */
#define PLAY_LOCK_CONT 2  /* p->s and p->w are unsafe */
typedef struct
{
	portbufferpair in;
	portbufferpair out;
	portbufferpair cin[MAX_CHANNELS];
	portbufferpair cout[MAX_CHANNELS];
} portbuffers;

char ifMacro(channel *cv, row r, char m, char (*callback)(int, channel *, row))
{
	char ret = 0;
	for (int i = 0; i < cv->macroc; i++)
		if (r.macro[i].c == m)
			ret = callback(r.macro[i].v, cv, r);
	return ret;
}

void changeBpm(song *s, uint8_t newbpm)
{
	s->bpm = newbpm;
	s->spr = samplerate * (60.0 / newbpm) / (s->rowhighlight * 2);
}

/* freewheel to fill up the ramp buffer */
void ramp(playbackinfo *p, channel *cv, uint8_t realinstrument, uint32_t pointeroffset)
{
	instrument *iv = p->s->instrumentv[realinstrument];

	/* clear the rampbuffer so cruft isn't played in edge cases */
	memset(cv->rampbuffer, 0, sizeof(sample_t) * rampmax * 2);

	if (cv->gain != -1) cv->rampgain = cv->gain;
	else if (iv)        cv->rampgain = iv->defgain;

	if (iv)
	{
		if (cv->reverse)
		{
			jack_nframes_t localrampmax;
			if (pointeroffset < rampmax)
				localrampmax = rampmax - pointeroffset;
			else localrampmax = rampmax;

			for (uint16_t i = 0; i < localrampmax; i++)
			{
				if (!cv->r.note) break;
				samplerProcess(iv, cv, pointeroffset-i,
						&cv->rampbuffer[i * 2 + 0], &cv->rampbuffer[i * 2 + 1]);
			}
		} else
			for (uint16_t i = 0; i < rampmax; i++)
			{
				if (!cv->r.note) break;
				samplerProcess(iv, cv, pointeroffset+i,
						&cv->rampbuffer[i * 2 + 0], &cv->rampbuffer[i * 2 + 1]);
			}
	}
}

void _triggerNote(channel *cv, uint8_t note, uint8_t inst)
{
	if (note == 255) /* note off */
	{
		if (!cv->releasepointer) cv->releasepointer = cv->pointer;
	} else
	{
		cv->r.inst = inst;
		cv->r.note = note;
		cv->portamento = 0;
		cv->pointer = 0;
		cv->reverse = 0;
		cv->portamentofinetune = 0.0;
		cv->microtonalfinetune = 0.0;
		cv->pointeroffset = 0;
		cv->releasepointer = 0;
		cv->gain = -1;
		cv->targetgain = -1;
	}
}
/* ramping */
void triggerNote(channel *cv, uint8_t note, uint8_t inst)
{
	if (cv->r.note && note != 255 && p->s->instrumenti[cv->r.inst]) /* old note, ramp it out */
	{
		if (cv->rampbuffer && !(p->w->instrumentlockv == INST_GLOBAL_LOCK_FREE
				&& p->w->instrumentlocki == p->s->instrumenti[cv->r.inst])
				&& cv->r.note)
			ramp(p, cv, p->s->instrumenti[cv->r.inst], cv->pointer);
		cv->rampindex = 0; /* set this even if it's not populated */
	}
	_triggerNote(cv, note, inst);
}


void calcVibrato(channel *cv, int m)
{
	cv->vibrato = m%16;
	if (!cv->vibratosamples) /* reset the phase if starting */
		cv->vibratosamplepointer = 0;
	cv->vibratosamples = p->s->spr / (((m>>4) + 1) / 16.0); /* use floats for slower lfo speeds */
	cv->vibratosamplepointer = MIN(cv->vibratosamplepointer, cv->vibratosamples - 1);
}

char Vc(int m, channel *cv, row r)
{
	if (m >= 0)
		calcVibrato(cv, m);
	else cv->vibratosamples = 0;
	return 0;
}
char bc(int m, channel *cv, row r)
{
	if (m == 0) changeBpm(p->s, p->s->songbpm);
	else        changeBpm(p->s, MAX(32, m));
	return 0;
}
char Cc(int m, channel *cv, row r)
{
	if (m != -1 && m>>4 == 0)
	{
		if (!(p->w->instrumentlockv == INST_GLOBAL_LOCK_FREE
				&& p->w->instrumentlocki == p->s->instrumenti[cv->r.inst]))
			ramp(p, cv, p->s->instrumenti[cv->r.inst], cv->pointer);
		cv->rampindex = 0;
		cv->r.note = 0;
		cv->vibrato = 0;
		return 1;
	} else if (m != -1 && m%16 != 0) /* cut, don't divide by 0 */
	{
		cv->cutsamples = p->s->spr * (float)(m>>4) / (float)(m%16);
		return 1;
	}
	return 0;
}
char Pc(int m, channel *cv, row r)
{
	if (m != -1)
	{
		cv->portamento = r.note;
		cv->portamentospeed = m;
		return 1;
	}
	return 0;
}
char Dc(int m, channel *cv, row r)
{
	if (m != -1 && m%16 != 0)
	{
		cv->delaysamples = p->s->spr * (float)(m>>4) / (float)(m%16);
		cv->delaynote = r.note;
		cv->delayinst = r.inst;
		return 1;
	}
	return 0;
}
char Gc(int m, channel *cv, row r)
{
	if (m != -1)
	{
		if (cv->pointer)
		{
			ramp(p, cv, p->s->instrumenti[cv->r.inst], cv->pointer);
			cv->rampindex = 0;
		}
		cv->gain = m;
		return 1;
	}
	return 0;
}
char gc(int m, channel *cv, row r)
{
	if (m != -1)
	{
		if (cv->pointer) /* only slide if a note is already playing */
			cv->targetgain = m;
		else
			cv->gain = m;
	}
	return 0;
}
char Rc(int m, channel *cv, row r)
{
	if (m%16 != 0)
	{
		cv->rtrigpointer = cv->pointer;
		cv->rtrigsamples = (p->s->spr / (m%16)) * ((m>>4) + 1);
		cv->rtrigblocksize = m>>4;
	}
	return 0;
}
char Wc(int m, channel *cv, row r)
{
	cv->waveshaper = m>>4;
	cv->waveshaperstrength = m%16;
	return 0;
}
char tc(int m, channel *cv, row r)
{ cv->gate = m; return 0; }
char Oc(int m, channel *cv, row r)
{
	instrument *iv = p->s->instrumentv[p->s->instrumenti[cv->r.inst]];
	if (cv->r.note) /* if playing a note */
	{
		if (!r.note) /* if not changing note: ramping needed */
		{
			/* clear the rampbuffer so cruft isn't played in edge cases */
			memset(cv->rampbuffer, 0, sizeof(sample_t) * rampmax * 2);
			for (uint16_t i = 0; i < rampmax; i++)
			{
				if (!cv->r.note) break;
				samplerProcess(iv, cv, cv->pointer + i,
						&cv->rampbuffer[i * 2 + 0], &cv->rampbuffer[i * 2 + 1]);
			}
			cv->rampindex = 0;
		}
		cv->pointeroffset = (m*DIV255) * (iv->trim[1] - iv->trim[0]);
		cv->pointer = 0;
	}

	return 0;
}
char oc(int m, channel *cv, row r)
{
	cv->reverse = !cv->reverse;
	if (m) return Oc(m, cv, r);
	return 0;
}
char Mc(int m, channel *cv, row r)
{ cv->microtonalfinetune = m*DIV255; return 0; }
char PERCENTc(int m, channel *cv, row r) /* returns true to NOT play */
{
	if (rand() % 256 > m) return 1;
	else                  return 0;
}
char Lc(int m, channel *cv, row r)
{
	cv->filtertype = 0;
	cv->filterres = (m>>4)*DIV15;
	cv->filtercut = (m%16)*DIV15;
	return 0;
}
char Hc(int m, channel *cv, row r)
{
	cv->filtertype = 1;
	cv->filterres = (m>>4)*DIV15;
	cv->filtercut = (m%16)*DIV15;
	return 0;
}
char Bc(int m, channel *cv, row r)
{
	cv->filtertype = 2;
	cv->filterres = (m>>4)*DIV15;
	cv->filtercut = (m%16)*DIV15;
	return 0;
}
char Nc(int m, channel *cv, row r)
{
	cv->filtertype = 3;
	cv->filterres = (m>>4)*DIV15;
	cv->filtercut = (m%16)*DIV15;
	return 0;
}

void applyGain(playbackinfo *p, channel *cv, instrument *iv, float *l, float *r)
{
	if (cv->targetgain != -1)
	{
		float rowprogress = (float)p->s->sprp / (float)p->s->spr;
		if (cv->gain != -1)
		{
			*l *= (cv->gain>>4)*DIV15 + ((cv->targetgain>>4) - (cv->gain>>4))*DIV15 * rowprogress;
			*r *= (cv->gain%16)*DIV15 + ((cv->targetgain%16) - (cv->gain%16))*DIV15 * rowprogress;
		} else if (iv)
		{
			*l *= (iv->defgain>>4)*DIV15 + ((cv->targetgain>>4) - (iv->defgain>>4))*DIV15 * rowprogress;
			*r *= (iv->defgain%16)*DIV15 + ((cv->targetgain%16) - (iv->defgain%16))*DIV15 * rowprogress;
		}
	} else
	{
		if (cv->gain != -1) { *l *= (cv->gain>>4)*DIV15; *r *= (cv->gain%16)*DIV15; }
		else if (iv) { *l *= (iv->defgain>>4)*DIV15; *r *= (iv->defgain%16)*DIV15; }
	}
}
void playChannel(jack_nframes_t fptr, playbackinfo *p, portbuffers pb, channel *cv)
{
	instrument *iv = p->s->instrumentv[p->s->instrumenti[cv->r.inst]];
	float lo = 0.0f;  float ro = 0.0f;  /* non-ramp */
	float rlo = 0.0f; float rro = 0.0f; /* ramp */

	cv->finetune = cv->portamentofinetune + cv->microtonalfinetune;
	if (cv->vibratosamples)
	{
		cv->finetune += oscillator(0, (float)cv->vibratosamplepointer / (float)cv->vibratosamples, 0.5)
			* cv->vibrato*DIV16;

		/* re-read the macro once phase is about to overflow */
		cv->vibratosamplepointer++;
		if (cv->vibratosamplepointer > cv->vibratosamples)
		{
			cv->vibratosamplepointer = 0;
			ifMacro(cv, cv->r, 'V', &Vc); // vibrato
		}
	}

	if (cv->cutsamples && p->s->sprp > cv->cutsamples)
	{
		if (cv->rampbuffer) ramp(p, cv, p->s->instrumenti[cv->r.inst], cv->pointer);
		cv->rampindex = 0;
		cv->r.note = 0;
		cv->vibrato = 0;
		cv->cutsamples = 0;
	}
	if (cv->delaysamples && p->s->sprp > cv->delaysamples)
	{
		triggerNote(cv, cv->delaynote, cv->delayinst);
		ifMacro(cv, cv->r, 'O', &Oc); /* offset            */
		ifMacro(cv, cv->r, 'b', &bc); /* bw offset         */
		ifMacro(cv, cv->r, 'M', &Mc); /* microtonal offset */
		cv->delaysamples = 0;
	}

	/* process the type */
	if (iv && cv->r.note)
	{
		if (!(p->w->instrumentlockv == INST_GLOBAL_LOCK_FREE
				&& p->w->instrumentlocki == p->s->instrumenti[cv->r.inst])
				&& cv->r.note != 255)
		{
			if (cv->rtrigsamples)
			{
				uint32_t rtrigoffset = (cv->pointer - cv->rtrigpointer) % cv->rtrigsamples;
				if (cv->reverse)
				{
					if (cv->rampbuffer && !rtrigoffset && cv->pointer < cv->rtrigpointer && cv->pointer > cv->rtrigsamples)
					{ // first sample of any retrigger but the first
						uint8_t oldnote = cv->r.note; // in case the ramping freewheel cuts the note
						ramp(p, cv, p->s->instrumenti[cv->r.inst], cv->pointer - cv->rtrigsamples);
						cv->rampindex = 0;
						cv->r.note = oldnote;
					}
					samplerProcess(iv, cv, cv->rtrigpointer + rtrigoffset, &lo, &ro);
				} else
				{
					if (cv->rampbuffer && !rtrigoffset && cv->pointer > cv->rtrigpointer)
					{ // first sample of any retrigger but the first
						uint8_t oldnote = cv->r.note; // in case the ramping freewheel cuts the note
						ramp(p, cv, p->s->instrumenti[cv->r.inst], cv->pointer + cv->rtrigsamples);
						cv->rampindex = 0;
						cv->r.note = oldnote;
					}
					samplerProcess(iv, cv, cv->rtrigpointer + rtrigoffset, &lo, &ro);
				}
			} else samplerProcess(iv, cv, cv->pointer, &lo, &ro);

			if (cv->reverse)
			{
				cv->pointer--;
				if (cv->pointer <= 1) cv->r.note = 0;
			} else cv->pointer++;
		} else cv->vibrato = 0;
	} else cv->vibrato = 0;

	if (cv->rampbuffer && cv->rampindex < rampmax)
	{ /* ramping */
		/* set the ramp cache */
		rlo = cv->rampbuffer[cv->rampindex * 2 + 0];
		rro = cv->rampbuffer[cv->rampindex * 2 + 1];

		/* denormals */
		if (fabsf(lo) < NOISE_GATE && fabsf(ro) < NOISE_GATE
				&& fabsf(rlo) < NOISE_GATE && fabsf(rro) < NOISE_GATE)
			return;

		/* waveshapers, intentionally pre-gain */ /* TODO: ramping */
		float strength;
		if (cv->waveshaperstrength)
		{
			switch (cv->waveshaper)
			{
				case 0: /* hard clipper */
					strength = 1.0f + cv->waveshaperstrength*1.5f;
					lo = hardclip(lo * strength); ro = hardclip(ro * strength);
					rlo = hardclip(rlo * strength); rro = hardclip(rro * strength);
					break;
				case 1: /* soft clipper */
					strength = 1.0f + cv->waveshaperstrength*0.4f;
					lo = thirddegreepolynomial(lo * strength); ro = thirddegreepolynomial(ro * strength);
					rlo = thirddegreepolynomial(rlo * strength); rro = thirddegreepolynomial(rro * strength);
					break;
				case 2: /* rectifier */
					strength = cv->waveshaperstrength*DIV15;
					lo = rectify(0, lo) * (MIN(strength, 0.5f) * 2) + lo * (1.0f - (MAX(strength, 0.5f) - 0.5f) * 2);
					ro = rectify(0, ro) * (MIN(strength, 0.5f) * 2) + ro * (1.0f - (MAX(strength, 0.5f) - 0.5f) * 2);
					rlo = rectify(0, rlo) * (MIN(strength, 0.5f) * 2) + rlo * (1.0f - (MAX(strength, 0.5f) - 0.5f) * 2);
					rro = rectify(0, rro) * (MIN(strength, 0.5f) * 2) + rro * (1.0f - (MAX(strength, 0.5f) - 0.5f) * 2);
					break;
				case 3: /* rectifier x2 */
					strength = cv->waveshaperstrength*DIV15;
					lo = rectify(1, lo) * (MIN(strength, 0.5f) * 2) + lo * (1.0f - (MAX(strength, 0.5f) - 0.5f) * 2);
					ro = rectify(1, ro) * (MIN(strength, 0.5f) * 2) + ro * (1.0f - (MAX(strength, 0.5f) - 0.5f) * 2);
					rlo = rectify(1, rlo) * (MIN(strength, 0.5f) * 2) + rlo * (1.0f - (MAX(strength, 0.5f) - 0.5f) * 2);
					rro = rectify(1, rro) * (MIN(strength, 0.5f) * 2) + rro * (1.0f - (MAX(strength, 0.5f) - 0.5f) * 2);
					break;
				case 4: /* wavefolder */
					strength = 1.0f + cv->waveshaperstrength*0.3f;
					lo = wavefolder(lo * strength); ro = wavefolder(ro * strength);
					rlo = wavefolder(rlo * strength); rro = wavefolder(rro * strength);
					break;
				case 5: /* wavewrapper */
					strength = 1.0f + cv->waveshaperstrength*0.2f;
					lo = wavewrapper(lo * strength); ro = wavewrapper(ro * strength);
					rlo = wavewrapper(rlo * strength); rro = wavewrapper(rro * strength);
					break;
				case 6: /* signed unsigned conversion */
					strength = cv->waveshaperstrength*DIV15;
					lo = signedunsigned(lo) * (MIN(strength, 0.5f) * 2) + lo * (1.0f - (MAX(strength, 0.5f) - 0.5f) * 2);
					ro = signedunsigned(ro) * (MIN(strength, 0.5f) * 2) + ro * (1.0f - (MAX(strength, 0.5f) - 0.5f) * 2);
					rlo = signedunsigned(rlo) * (MIN(strength, 0.5f) * 2) + rlo * (1.0f - (MAX(strength, 0.5f) - 0.5f) * 2);
					rro = signedunsigned(rro) * (MIN(strength, 0.5f) * 2) + rro * (1.0f - (MAX(strength, 0.5f) - 0.5f) * 2);
					break;
			}
		}

		applyGain(p, cv, iv, &lo, &ro);

		/* mix in ramp data */
		float gain = (float)cv->rampindex / (float)rampmax;
		lo = lo * gain + rlo * (1.0f - gain) * (cv->rampgain>>4)*DIV15;
		ro = ro * gain + rro * (1.0f - gain) * (cv->rampgain%16)*DIV15;
		cv->rampindex++;
	} else
	{ /* not ramping */
		/* denormals */
		if (fabsf(lo) < NOISE_GATE && fabsf(ro) < NOISE_GATE)
			return;

		/* waveshapers, intentionally pre-gain */ /* TODO: ramping */
		float strength;
		if (cv->waveshaperstrength)
		{
			switch (cv->waveshaper)
			{
				case 0: /* hard clipper */
					strength = 1.0f + cv->waveshaperstrength*1.5f;
					lo = hardclip(lo * strength); ro = hardclip(ro * strength);
					break;
				case 1: /* soft clipper */
					strength = 1.0f + cv->waveshaperstrength*0.4f;
					lo = thirddegreepolynomial(lo * strength); ro = thirddegreepolynomial(ro * strength);
					break;
				case 2: /* rectifier */
					strength = cv->waveshaperstrength*DIV15;
					lo = rectify(0, lo) * (MIN(strength, 0.5f) * 2) + lo * (1.0f - (MAX(strength, 0.5f) - 0.5f) * 2);
					ro = rectify(0, ro) * (MIN(strength, 0.5f) * 2) + ro * (1.0f - (MAX(strength, 0.5f) - 0.5f) * 2);
					break;
				case 3: /* rectifier x2 */
					strength = cv->waveshaperstrength*DIV15;
					lo = rectify(1, lo) * (MIN(strength, 0.5f) * 2) + lo * (1.0f - (MAX(strength, 0.5f) - 0.5f) * 2);
					ro = rectify(1, ro) * (MIN(strength, 0.5f) * 2) + ro * (1.0f - (MAX(strength, 0.5f) - 0.5f) * 2);
					break;
				case 4: /* wavefolder */
					strength = 1.0f + cv->waveshaperstrength*0.3f;
					lo = wavefolder(lo * strength); ro = wavefolder(ro * strength);
					break;
				case 5: /* wavewrapper */
					strength = 1.0f + cv->waveshaperstrength*0.2f;
					lo = wavewrapper(lo * strength); ro = wavewrapper(ro * strength);
					break;
				case 6: /* signed unsigned conversion */
					strength = cv->waveshaperstrength*DIV15;
					lo = signedunsigned(lo) * (MIN(strength, 0.5f) * 2) + lo * (1.0f - (MAX(strength, 0.5f) - 0.5f) * 2);
					ro = signedunsigned(ro) * (MIN(strength, 0.5f) * 2) + ro * (1.0f - (MAX(strength, 0.5f) - 0.5f) * 2);
					break;
			}
		}

		applyGain(p, cv, iv, &lo, &ro);
	}

	/* gate */
	if (MAX(fabsf(lo), fabsf(ro)) < (cv->gate%16)*0.025f)
		cv->gateopen = MAX(0.5f - ((cv->gate>>4)*DIV15) * 0.5f, cv->gateopen - MIN_GATE_SPEED_SEC/samplerate - (cv->gate>>4)*DIV128);
	else
		cv->gateopen = MIN(1.0f, cv->gateopen + MIN_GATE_SPEED_SEC/samplerate + (cv->gate>>4)*DIV128);
	lo *= cv->gateopen; ro *= cv->gateopen;

	/* filter */
	switch (cv->filtertype)
	{
		case 0: /* low-pass  */
			if (cv->filtercut < 1.0f)
			{
				runSVFilter(&cv->fl, thirddegreepolynomial(lo), cv->filtercut, cv->filterres);
				runSVFilter(&cv->fr, thirddegreepolynomial(ro), cv->filtercut, cv->filterres);
				lo = hardclip(cv->fl.l);
				ro = hardclip(cv->fr.l);
			}
			break;
		case 1: /* high-pass */
			if (cv->filtercut > 0.0f)
			{
				runSVFilter(&cv->fl, thirddegreepolynomial(lo), cv->filtercut, cv->filterres);
				runSVFilter(&cv->fr, thirddegreepolynomial(ro), cv->filtercut, cv->filterres);
				lo = hardclip(cv->fl.h);
				ro = hardclip(cv->fr.h);
			}
			break;
		case 2: /* band-pass */
			runSVFilter(&cv->fl, thirddegreepolynomial(lo), cv->filtercut, cv->filterres);
			runSVFilter(&cv->fr, thirddegreepolynomial(ro), cv->filtercut, cv->filterres);
			lo = hardclip(cv->fl.b);
			ro = hardclip(cv->fr.b);
			break;
		case 3: /* notch     */
			runSVFilter(&cv->fl, thirddegreepolynomial(lo), cv->filtercut, cv->filterres);
			runSVFilter(&cv->fr, thirddegreepolynomial(ro), cv->filtercut, cv->filterres);
			lo = hardclip(cv->fl.n);
			ro = hardclip(cv->fr.n);
			break;
	}

	if (!cv->mute)
	{
		pb.out.l[fptr] += lo;
		pb.out.r[fptr] += ro;
	}
	if (p->w->instrumentrecv == INST_REC_LOCK_CONT
			&& p->w->recordsource == 1
			&& p->w->recordflags & 0b1)
	{
		p->w->recchannelbuffer[fptr * 2 + 0] = lo;
		p->w->recchannelbuffer[fptr * 2 + 1] = ro;
	}
}

void bendUp(channel *cv, uint32_t spr, uint32_t count)
{
	cv->portamentofinetune += (12.0f / spr) * (cv->portamentospeed*DIV255) * count;
	while (cv->portamentofinetune > 0.5f)
	{
		cv->portamentofinetune -= 1.0f;
		cv->r.note++;
	}
	if (cv->r.note > cv->portamento || (cv->r.note == cv->portamento && cv->portamentofinetune >= 0.0f))
	{
		cv->portamentofinetune = 0.0f;
		cv->r.note = cv->portamento;
		cv->portamento = 0;
	}
}

void bendDown(channel *cv, uint32_t spr, uint32_t count)
{
	cv->portamentofinetune -= (12.0f / spr) * (cv->portamentospeed*DIV255);
	while (cv->portamentofinetune < 0.5f)
	{
		cv->portamentofinetune += 1.0f;
		cv->r.note--;
	}
	if (cv->r.note <= cv->portamento && cv->portamentofinetune <= 0.0f)
	{
		cv->portamentofinetune = 0.0f;
		cv->r.note = cv->portamento;
		cv->portamento = 0;
	}
}

void preprocessRow(channel *cv, row r)
{
	char ret;

	ifMacro(cv, r, 'b', &bc); /* bpm */

	ret = ifMacro(cv, r, 'C', &Cc); /* cut */
	if (!ret && r.note)
	{
		ret = ifMacro(cv, r, '%', &PERCENTc); /* chance */
		if (!ret)
		{
			ret = ifMacro(cv, r, 'P', &Pc); /* pitch slide */
			if (!ret)
			{
				ret = ifMacro(cv, r, 'D', &Dc); /* delay */
				if (!ret) triggerNote(cv, r.note, r.inst);
			}
		}
	}

	ifMacro(cv, r, 'M', &Mc); /* microtonal offset */

	/* gain */
	if (cv->targetgain != -1)
	{
		cv->gain = cv->targetgain;
		cv->targetgain = -1;
	}
	ret = ifMacro(cv, r, 'G', &Gc);
	if (!ret)
		ifMacro(cv, r, 'g', &gc);

	ifMacro(cv, r, 'O', &Oc); /* offset    */
	ifMacro(cv, r, 'o', &oc); /* bw offset */

	if (cv->rtrigsamples)
	{
		if (cv->rtrigblocksize)
			cv->rtrigblocksize--;
		else
			cv->rtrigsamples = 0;
	}
	ifMacro(cv, r, 'R', &Rc); /* retrigger */

	ifMacro(cv, r, 'V', &Vc); /* vibrato   */

	ifMacro(cv, r, 'L', &Lc); /* low-pass   */
	ifMacro(cv, r, 'H', &Hc); /* high-pass  */
	ifMacro(cv, r, 'B', &Bc); /* band-pass  */
	ifMacro(cv, r, 'N', &Nc); /* notch      */
	ifMacro(cv, r, 'W', &Wc); /* waveshaper */
	ifMacro(cv, r, 't', &tc); /* gate       */

	for (int i = 0; i < cv->macroc; i++)
		cv->r.macro[i] = r.macro[i];
}

int process(jack_nframes_t nfptr, void *arg)
{
	playbackinfo *p = arg;
	channel *cv;
	portbuffers pb;

	pb.in.l =  jack_port_get_buffer(p->in.l, nfptr);
	pb.in.r =  jack_port_get_buffer(p->in.r, nfptr);
	pb.out.l = jack_port_get_buffer(p->out.l, nfptr); memset(pb.out.l, 0, nfptr * sizeof(sample_t));
	pb.out.r = jack_port_get_buffer(p->out.r, nfptr); memset(pb.out.r, 0, nfptr * sizeof(sample_t));

	if (p->lock == PLAY_LOCK_START)
		p->lock = PLAY_LOCK_CONT;

	if (p->lock == PLAY_LOCK_CONT) return 0;


	/* just need to know these have been seen */
	/* will no longer write to the record buffer */
	if (p->w->instrumentrecv == INST_REC_LOCK_PREP_END)
		p->w->instrumentrecv = INST_REC_LOCK_END;
	if (p->w->instrumentrecv == INST_REC_LOCK_PREP_CANCEL)
		p->w->instrumentrecv = INST_REC_LOCK_CANCEL;
	/* start recording if gate pattern is off */
	if (p->w->instrumentrecv == INST_REC_LOCK_START && !(p->w->recordflags & 0b10))
	{
		p->w->instrumentrecv = INST_REC_LOCK_CONT;
		p->dirty = 1;
	}
	/* will no longer access the instrument state */
	if (p->w->instrumentlockv == INST_GLOBAL_LOCK_PREP_FREE
			|| p->w->instrumentlockv == INST_GLOBAL_LOCK_PREP_HIST
			|| p->w->instrumentlockv == INST_GLOBAL_LOCK_PREP_PUT)
		p->w->instrumentlockv++;


	if (p->w->previewtrigger)
		switch (p->w->previewtrigger)
		{
			case 1: // start instrument preview
				memcpy(&p->w->previewchannel, &p->s->channelv[p->w->previewchannelsrc], sizeof(channel));
				p->w->previewchannel.gain = -1;
				p->w->previewchannel.macroc = 1;
				p->w->previewchannel.rtrigsamples = 0;
				p->w->previewchannel.rampindex = rampmax;
				p->w->previewchannel.rampbuffer = NULL;

				preprocessRow(&p->w->previewchannel, p->w->previewrow);

				p->w->previewtrigger++;
				break;
		}


	/* start/stop playback */
	if (p->s->playing == PLAYING_START)
	{
		changeBpm(p->s, p->s->songbpm);
		p->s->sprp = 0;

		/* clear the channels */
		for (uint8_t c = 0; c < p->s->channelc; c++)
		{
			cv = &p->s->channelv[c];
			cv->r.note = 0;
			cv->rtrigsamples = 0;
			cv->waveshaperstrength = 0;
			cv->gate = 0;
			cv->gateopen = 1.0f;
			cv->filtertype = 0.0f;
			cv->filtercut = 1.0f;
			cv->filterres = 0.0f;
		}

		/* non-volatile state */
		/* start at the beginning of the block */
		uint8_t blockstart = 0;
		for (uint8_t i = p->s->songp; i >= 0; i--)
			if (p->s->songi[i] == 255)
			{
				blockstart = i + 1;
				break;
			}

		/* for each pattern in the block */
		pattern *pt;
		channel *cv;
		for (uint8_t b = blockstart; b < p->s->songp; b++)
		{
			pt = p->s->patternv[p->s->patterni[p->s->songi[b]]];
			/* for each row */
			for (uint8_t r = 0; r < pt->rowc; r++)
			{
				for (uint8_t c = 0; c < p->s->channelc; c++)
				{
					cv = &p->s->channelv[c];
					preprocessRow(cv, pt->rowv[c][r%pt->rowcc[c]+1]);
					if (cv->cutsamples && cv->delaysamples)
					{
						if (cv->cutsamples > cv->delaysamples)
						{
							cv->r.note = 0;
							cv->cutsamples = 0;
						} else
						{
							_triggerNote(cv, cv->delaynote, cv->delayinst);
							ifMacro(cv, cv->r, 'O', &Oc); /* offset            */
							ifMacro(cv, cv->r, 'b', &bc); /* bw offset         */
							ifMacro(cv, cv->r, 'M', &Mc); /* microtonal offset */
							if (cv->reverse)
							{
								if (cv->pointer > p->s->spr - cv->delaysamples)
									cv->pointer -= p->s->spr - cv->delaysamples;
								else cv->r.note = 0;
							} else cv->pointer += p->s->spr - cv->delaysamples;
							cv->delaysamples = 0;
						}
					}
					else if (cv->cutsamples)
					{
						cv->r.note = 0;
						cv->cutsamples = 0;
					}
					else if (cv->delaysamples)
					{
						_triggerNote(cv, cv->delaynote, cv->delayinst);
						ifMacro(cv, cv->r, 'O', &Oc); /* offset            */
						ifMacro(cv, cv->r, 'b', &bc); /* bw offset         */
						ifMacro(cv, cv->r, 'M', &Mc); /* microtonal offset */
						if (cv->reverse)
						{
							if (cv->pointer > p->s->spr - cv->delaysamples)
								cv->pointer -= p->s->spr - cv->delaysamples;
							else cv->r.note = 0;
						} else cv->pointer += p->s->spr - cv->delaysamples;
						cv->delaysamples = 0;
					} else if (cv->r.note)
					{
						if (cv->reverse)
						{
							if (cv->pointer > p->s->spr) cv->pointer -= p->s->spr;
							else cv->r.note = 0;
						} else cv->pointer += p->s->spr;
					}

					if (cv->r.note && cv->portamento)
					{
						if (cv->r.note == cv->portamento)
						{ /* fine bend */
							if (cv->portamentofinetune < 0.0)
								bendUp(cv, p->s->spr, p->s->spr);
							else
								bendDown(cv, p->s->spr, p->s->spr);
						} else if (cv->r.note < cv->portamento)
						{ /* bend up */
							bendUp(cv, p->s->spr, p->s->spr);
						} else
						{ /* bend down */
							bendDown(cv, p->s->spr, p->s->spr);
						}
					}
				}
			}
		}
		/* start recording if gate pattern is on */
		if (p->w->instrumentrecv == INST_REC_LOCK_START && p->w->recordflags & 0b10)
			p->w->instrumentrecv = INST_REC_LOCK_CONT;

		p->s->playing = PLAYING_CONT;
	} else if (p->s->playing == PLAYING_PREP_STOP)
	{
		/* stop channels */
		for (uint8_t i = 0; i < p->s->channelc; i++)
		{
			if (!(p->w->instrumentlockv == INST_GLOBAL_LOCK_FREE
					&& p->w->instrumentlocki == p->s->instrumenti[p->s->channelv[i].r.inst])
					&& p->s->channelv[i].r.inst)
				ramp(p, &p->s->channelv[i],
						p->s->instrumenti[p->s->channelv[i].r.inst],
						p->s->channelv[i].pointer);
			p->s->channelv[i].r.note = 0;
		}

		p->dirty = 1;
		p->s->playing = PLAYING_STOP;
	}

	if (p->w->request == REQ_BPM)
	{
		changeBpm(p->s, p->s->songbpm);
		p->w->request = REQ_OK;
	}


	/* loop over samples */
	for (jack_nframes_t fptr = 0; fptr < nfptr; fptr++)
	{
		/* preprocess the channel */
		if (p->s->playing == PLAYING_CONT && p->s->sprp == 0)
			for (uint8_t c = 0; c < p->s->channelc; c++)
				preprocessRow(&p->s->channelv[c],
						p->s->patternv[p->s->patterni[p->s->songi[p->s->songp]]]->rowv[c]
						[p->s->songr % (p->s->patternv[p->s->patterni[p->s->songi[p->s->songp]]]->rowcc[c]+1)]);

		for (uint8_t c = 0; c < p->s->channelc; c++)
		{
			cv = &p->s->channelv[c];
			if (p->s->playing == PLAYING_CONT && cv->portamento)
			{
				if (cv->r.note == cv->portamento)
				{ /* fine bend */
					if (cv->portamentofinetune < 0.0)
						bendUp(cv, p->s->spr, 1);
					else
						bendDown(cv, p->s->spr, 1);
				} else if (cv->r.note < cv->portamento)
				{ /* coarse bend up */
					bendUp(cv, p->s->spr, 1);
				} else
				{ /* coarse bend down */
					bendDown(cv, p->s->spr, 1);
				}
			}
			playChannel(fptr, p, pb, cv);
		}
		/* play the preview */
		if (p->w->previewchannel.r.note)
			playChannel(fptr, p, pb, &p->w->previewchannel);

		/* next row */
		if (p->s->sprp++ > p->s->spr)
		{
			p->s->sprp = 0;
			if (p->s->playing == PLAYING_CONT)
			{
				/* next pattern */
				if (p->s->songr++ >= p->s->patternv[p->s->patterni[p->s->songi[p->s->songp]]]->rowc)
				{
					p->s->songr = 0;
					
					/* stop recording if gate pattern is on */
					if (p->w->instrumentrecv == INST_REC_LOCK_CONT && p->w->recordflags & 0b10)
						p->w->instrumentrecv = INST_REC_LOCK_END;
					/* start recording if gate pattern is on */
					if (p->w->instrumentrecv == INST_REC_LOCK_START && p->w->recordflags & 0b10)
						p->w->instrumentrecv = INST_REC_LOCK_CONT;

					if (p->w->songnext)
					{
						p->s->songp = p->w->songnext - 1;
						p->w->songnext = 0;
					} else if (p->s->songf[p->s->songp]) /* loop */
					{} /* do nothing, don't inc the song pointer */
					else if (p->s->songi[p->s->songp + 1] == 255)
					{ /* no next pattern, go to the beginning of the block */
						uint8_t blockstart = 0;
						for (uint8_t i = p->s->songp; i >= 0; i--)
							if (p->s->songi[i] == 255)
							{
								blockstart = i + 1;
								break;
							}
						p->s->songp = blockstart;
					} else p->s->songp++;
				}

				if (w->flags & 0b1)
				{
					p->w->songfy = p->s->songp;
					p->w->trackerfy = p->s->songr;
				}
				p->dirty = 1;
			}
		}
	}

	/* record */
	if (p->s->playing == PLAYING_CONT && p->w->instrumentrecv == INST_REC_LOCK_CONT)
	{
		if (p->w->recptr + nfptr > RECORD_LENGTH * samplerate)
		{
			strcpy(p->w->command.error, "record buffer full");
			p->w->instrumentrecv = INST_REC_LOCK_END;
		} else
		{
			int c;
			for (jack_nframes_t fptr = 0; fptr < nfptr; fptr++)
			{
				switch (p->w->recordsource)
				{
					case 0:
						c = (float)pb.in.l[fptr] * (float)SHRT_MAX;
						if      (c > SHRT_MAX) p->w->recbuffer[p->w->recptr * 2 + 0] = SHRT_MAX;
						else if (c < SHRT_MIN) p->w->recbuffer[p->w->recptr * 2 + 0] = SHRT_MIN;
						else                   p->w->recbuffer[p->w->recptr * 2 + 0] = c;
						c = (float)pb.in.r[fptr] * (float)SHRT_MAX;
						if      (c > SHRT_MAX) p->w->recbuffer[p->w->recptr * 2 + 1] = SHRT_MAX;
						else if (c < SHRT_MIN) p->w->recbuffer[p->w->recptr * 2 + 1] = SHRT_MIN;
						else                   p->w->recbuffer[p->w->recptr * 2 + 1] = c;
						break;
					case 1:
						if (p->w->recordflags & 0b1)
						{
							c = (float)p->w->recchannelbuffer[fptr * 2 + 0] * (float)SHRT_MAX;
							if      (c > SHRT_MAX) p->w->recbuffer[p->w->recptr * 2 + 0] = SHRT_MAX;
							else if (c < SHRT_MIN) p->w->recbuffer[p->w->recptr * 2 + 0] = SHRT_MIN;
							else                   p->w->recbuffer[p->w->recptr * 2 + 0] = c;
							c = (float)p->w->recchannelbuffer[fptr * 2 + 1] * (float)SHRT_MAX;
							if      (c > SHRT_MAX) p->w->recbuffer[p->w->recptr * 2 + 1] = SHRT_MAX;
							else if (c < SHRT_MIN) p->w->recbuffer[p->w->recptr * 2 + 1] = SHRT_MIN;
							else                   p->w->recbuffer[p->w->recptr * 2 + 1] = c;
						} else
						{
							c = (float)pb.out.l[fptr] * (float)SHRT_MAX;
							if      (c > SHRT_MAX) p->w->recbuffer[p->w->recptr * 2 + 0] = SHRT_MAX;
							else if (c < SHRT_MIN) p->w->recbuffer[p->w->recptr * 2 + 0] = SHRT_MIN;
							else                   p->w->recbuffer[p->w->recptr * 2 + 0] = c;
							c = (float)pb.out.r[fptr] * (float)SHRT_MAX;
							if      (c > SHRT_MAX) p->w->recbuffer[p->w->recptr * 2 + 1] = SHRT_MAX;
							else if (c < SHRT_MIN) p->w->recbuffer[p->w->recptr * 2 + 1] = SHRT_MIN;
							else                   p->w->recbuffer[p->w->recptr * 2 + 1] = c;
						}
						break;
				}
				p->w->recptr++;
			}
		}
	}

	updateBackground(nfptr, pb.out);

	return 0;
}
