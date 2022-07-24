#define PLAY_LOCK_OK 0    /* p->s and p->w are safe */
#define PLAY_LOCK_START 1 /* p->s and p->w want to be unsafe */
#define PLAY_LOCK_CONT 2  /* p->s and p->w are unsafe */

portbuffers pb;

char ifMacro(jack_nframes_t fptr, channel *cv, row r, char m, char (*callback)(jack_nframes_t, int, channel *, row))
{
	char ret = 0;
	for (int i = 0; i < cv->macroc; i++)
		if (r.macro[i].c == m)
			ret = callback(fptr, r.macro[i].v, cv, r);
	return ret;
}

void changeBpm(song *s, uint8_t newbpm)
{
	s->bpm = newbpm;
	s->spr = samplerate * (60.0 / newbpm) / s->rowhighlight;
}

/* freewheel to fill up the ramp buffer */
void ramp(playbackinfo *p, channel *cv, uint8_t realinstrument, uint32_t pointeroffset)
{
	if (cv->rampbuffer)
	{
		instrument *iv = p->s->instrumentv[realinstrument];

		/* clear the rampbuffer so cruft isn't played in edge cases */
		memset(cv->rampbuffer, 0, sizeof(sample_t) * rampmax * 2);

		if (cv->gain != -1) cv->rampgain = cv->gain;
		else if (iv)        cv->rampgain = iv->defgain;

		SVFilter oldfl = cv->fl;
		SVFilter oldfr = cv->fr;
		if (iv)
		{
			if (cv->reverse)
			{
				jack_nframes_t localrampmax;
				if (pointeroffset < rampmax)
					localrampmax = rampmax - pointeroffset;
				else localrampmax = rampmax;

				for (uint16_t i = 0; i < localrampmax; i++)
					samplerProcess(iv, cv, pointeroffset - i,
							&cv->rampbuffer[i*2 + 0], &cv->rampbuffer[i*2 + 1]);
			} else
				for (uint16_t i = 0; i < rampmax; i++)
					samplerProcess(iv, cv, pointeroffset + i,
							&cv->rampbuffer[i*2 + 0], &cv->rampbuffer[i*2 + 1]);
		}
		cv->fl = oldfl;
		cv->fr = oldfr;
	}
}
jack_midi_data_t midiGain(channel *cv, instrument *iv)
{
	if (cv->targetgain != -1)
	{
		if (cv->gain != -1) return ((cv->targetgain>>4)<<3);
		else if (iv)        return ((iv->defgain>>4)<<3);
	} else
	{
		if (cv->gain != -1) return (cv->gain>>4)<<3;
		else if (iv)        return (iv->defgain>>4)<<3;
	} return 0;
}
void applyGain(playbackinfo *p, float rowprogress, channel *cv, instrument *iv, float *l, float *r)
{
	if (cv->targetgain != -1)
	{
		if (cv->gain != -1)
		{
			*l *= (cv->gain>>4)*DIV16 + ((cv->targetgain>>4) - (cv->gain>>4))*DIV16 * rowprogress;
			*r *= (cv->gain%16)*DIV16 + ((cv->targetgain%16) - (cv->gain%16))*DIV16 * rowprogress;
		} else if (iv)
		{
			*l *= (iv->defgain>>4)*DIV16 + ((cv->targetgain>>4) - (iv->defgain>>4))*DIV16 * rowprogress;
			*r *= (iv->defgain%16)*DIV16 + ((cv->targetgain%16) - (iv->defgain%16))*DIV16 * rowprogress;
		}
	} else
	{
		if (cv->gain != -1) { *l *= (cv->gain>>4)*DIV16; *r *= (cv->gain%16)*DIV16; }
		else if (iv) { *l *= (iv->defgain>>4)*DIV16; *r *= (iv->defgain%16)*DIV16; }
	}
}


void midiNoteOff(jack_nframes_t fptr, uint8_t midichannel, uint8_t note, uint8_t velocity)
{
	if (note != NOTE_VOID && note != NOTE_OFF)
	{
		jack_midi_data_t event[3] = {0b10000000 | midichannel, note, velocity};
		jack_midi_event_write(pb.midiout, fptr, event, 3);
	}
}
void midiPitchWheel(jack_nframes_t fptr, uint8_t midichannel, float pitch)
{
	unsigned short intpitch = (int)(pitch * 4096.0f) + 8192;
	jack_midi_data_t event[3] = {0b11100000 | midichannel, (unsigned short)(intpitch<<9)>>9, intpitch>>7};
	jack_midi_event_write(pb.midiout, fptr, event, 3);
}
void midiNoteOn(jack_nframes_t fptr, uint8_t midichannel, uint8_t note, uint8_t velocity)
{
	if (note != NOTE_VOID && note != NOTE_OFF)
	{
		jack_midi_data_t event[3] = {0b10010000 | midichannel, note, velocity};
		jack_midi_event_write(pb.midiout, fptr, event, 3);
	}
}
void _triggerNote(channel *cv, uint8_t note, uint8_t inst)
{
	if (note == NOTE_VOID) return;
	if (note == NOTE_OFF)
	{
		if (!cv->releasepointer) cv->releasepointer = cv->pointer;
	} else
	{
		cv->r.inst = inst;
		cv->r.note = note;
		cv->portamento = NOTE_VOID;
		cv->pointer = 0;
		cv->reverse = 0;
		cv->portamentofinetune = 0.0;
		cv->microtonalfinetune = 0.0;
		cv->pointeroffset = 0;
		cv->releasepointer = 0;
		cv->gain = -1;
		cv->targetgain = -1;
		cv->vibrato = 0;
	}
}
void triggerNote(playbackinfo *p, jack_nframes_t fptr, channel *cv, uint8_t note, uint8_t inst)
{
	if (p->s->instrumenti[cv->r.inst] && !(p->w->instrumentlockv == INST_GLOBAL_LOCK_FREE
			&& p->w->instrumentlocki == p->s->instrumenti[cv->r.inst]))
		ramp(p, cv, p->s->instrumenti[cv->r.inst], cv->pointer);
	cv->rampindex = 0; /* set this even if it's not populated so it always ramps out */
}
/* separate from triggerNote() cos gain needs to be calculated in between */
char triggerMidi(playbackinfo *p, jack_nframes_t fptr, channel *cv, uint8_t oldnote, uint8_t note, uint8_t inst)
{
	if (note != NOTE_VOID && !cv->mute && p->s->instrumenti[inst])
	{
		instrument *iv = p->s->instrumentv[p->s->instrumenti[inst]];
		if (iv->flags&S_FLAG_MIDI)
		{
			/* always stop the prev. note */
			midiNoteOff(fptr, iv->midichannel, oldnote, midiGain(cv, iv));
			midiNoteOn(fptr, iv->midichannel, note, midiGain(cv, iv));
			return 1;
		}
	}
	return 0;
}


void calcVibrato(channel *cv, int m)
{
	cv->vibrato = m%16;
	if (!cv->vibratosamples) /* reset the phase if starting */
		cv->vibratosamplepointer = 0;
	cv->vibratosamples = p->s->spr / (((m>>4) + 1) / 16.0); /* use floats for slower lfo speeds */
	cv->vibratosamplepointer = MIN(cv->vibratosamplepointer, cv->vibratosamples - 1);
}

char Vc(jack_nframes_t fptr, int m, channel *cv, row r)
{
	calcVibrato(cv, m);
	return 1;
}
char bc(jack_nframes_t fptr, int m, channel *cv, row r)
{
	if (m == 0) changeBpm(p->s, p->s->songbpm);
	else        changeBpm(p->s, MAX(32, m));
	return 0;
}
char Cc(jack_nframes_t fptr, int m, channel *cv, row r)
{
	if (!m>>4)
	{ /* cut now */
		triggerNote(p, fptr, cv, NOTE_OFF, cv->r.inst);
		triggerMidi(p, fptr, cv, cv->r.note, NOTE_OFF, cv->r.inst);
		cv->r.note = NOTE_OFF;
		_triggerNote(cv, NOTE_OFF, cv->r.inst);
		cv->cutsamples = 0;
		return 1;
	} else if (m%16 != 0) /* cut later */
		cv->cutsamples = p->s->spr * (float)(m>>4) / (float)(m%16);
	return 0;
}
char Pc(jack_nframes_t fptr, int m, channel *cv, row r)
{
	cv->portamento = r.note;
	cv->portamentospeed = m;
	return 1;
}
char Dc(jack_nframes_t fptr, int m, channel *cv, row r)
{
	if (m%16 != 0)
	{
		cv->delaysamples = p->s->spr * (float)(m>>4) / (float)(m%16);
		cv->delaynote = r.note;
		if (r.inst == INST_VOID) cv->delayinst = cv->r.inst;
		else                     cv->delayinst = r.inst;
		return 1;
	} return 0;
}
char PRERAMP(jack_nframes_t fptr, int m, channel *cv, row r)
{
	if (cv->pointer) return 1;
	return 0;
}
char GcPOSTRAMP(jack_nframes_t fptr, int m, channel *cv, row r)
{
	cv->gain = m;
	return 0;
}
char gc(jack_nframes_t fptr, int m, channel *cv, row r)
{
	/* only slide if a note is already playing */
	if (cv->pointer) cv->targetgain = m;
	else             cv->gain = m;
	return 0;
}
char Rc(jack_nframes_t fptr, int m, channel *cv, row r)
{
	if (m%16 != 0)
	{
		cv->rtrigpointer = cv->pointer;
		cv->rtrigsamples = p->s->spr / (m%16);
		cv->rtrigblocksize = m>>4;
	} return 0;
}
char Wc(jack_nframes_t fptr, int m, channel *cv, row r)
{
	cv->waveshaper = m>>4;
	cv->waveshaperstrength = m%16;
	return 0;
}
char tc(jack_nframes_t fptr, int m, channel *cv, row r)
{ cv->gate = m; return 0; }
char Oc(jack_nframes_t fptr, int m, channel *cv, row r)
{
	instrument *iv = p->s->instrumentv[p->s->instrumenti[cv->r.inst]];
	if (cv->r.note != NOTE_VOID) /* if playing a note */
	{
		if (r.note == NOTE_VOID) /* if not changing note, explicit ramping needed */
		{
			ramp(p, cv, p->s->instrumenti[cv->r.inst], cv->pointer);
			cv->rampindex = 0;
		}
		cv->pointeroffset = (m*DIV255) * (iv->trim[1] - iv->trim[0]);
		cv->pointer = 0;
	} return 0;
}
char oc(jack_nframes_t fptr, int m, channel *cv, row r)
{
	cv->reverse = !cv->reverse;
	if (m) return Oc(fptr, m, cv, r);
	return 0;
}
char Mc(jack_nframes_t fptr, int m, channel *cv, row r)
{ cv->microtonalfinetune = m*DIV255; return 0; }
char PERCENTc(jack_nframes_t fptr, int m, channel *cv, row r) /* returns true to NOT play */
{
	if (rand()%256 > m) return 1;
	else                return 0;
}

char FcPOSTRAMP(jack_nframes_t fptr, int m, channel *cv, row r)
{
	cv->filtercut = m;
	return 0;
}
char fc(jack_nframes_t fptr, int m, channel *cv, row r)
{
	/* only slide if a note is already playing */
	if (cv->pointer) cv->targetfiltercut = m;
	else             cv->filtercut = m;
	return 0;
}
char ZcPOSTRAMP(jack_nframes_t fptr, int m, channel *cv, row r)
{
	cv->filterres = (uint8_t)((uint8_t)m<<4)>>4;
	return 0;
}
/* char Lc(jack_nframes_t fptr, int m, channel *cv, row r)
{
	cv->filtermode = 0;
	cv->filterres = (m>>4)*DIV15;
	cv->filtercut = ((uint8_t)((uint8_t)m<<4)>>4)*DIV15;
	return 0;
}
char Hc(jack_nframes_t fptr, int m, channel *cv, row r)
{
	cv->filtermode = 1;
	cv->filterres = (m>>4)*DIV15;
	cv->filtercut = ((uint8_t)((uint8_t)m<<4)>>4)*DIV15;
	return 0;
}
char Bc(jack_nframes_t fptr, int m, channel *cv, row r)
{
	cv->filtermode = 2;
	cv->filterres = (m>>4)*DIV15;
	cv->filtercut = ((uint8_t)((uint8_t)m<<4)>>4)*DIV15;
	return 0;
}
char Nc(jack_nframes_t fptr, int m, channel *cv, row r)
{
	cv->filtermode = 3;
	cv->filterres = (m>>4)*DIV15;
	cv->filtercut = ((uint8_t)((uint8_t)m<<4)>>4)*DIV15;
	return 0;
} */

void playChannel(jack_nframes_t fptr, playbackinfo *p, channel *cv)
{
	char ret;
	instrument *iv = p->s->instrumentv[p->s->instrumenti[cv->r.inst]];
	float lo = 0.0f;
	float ro = 0.0f;

	float rowprogress = (float)p->s->sprp / (float)p->s->spr;

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
			ret = ifMacro(fptr, cv, cv->r, 'V', &Vc); // vibrato
			if (!ret) cv->vibratosamples = 0;
		}
	}

	if (cv->cutsamples && p->s->sprp > cv->cutsamples)
	{
		triggerNote(p, fptr, cv, NOTE_OFF, cv->r.inst);
		ret = triggerMidi(p, fptr, cv, cv->r.note, NOTE_OFF, cv->r.inst);
		cv->r.note = NOTE_OFF;
		_triggerNote(cv, NOTE_OFF, cv->r.inst);
		cv->cutsamples = 0;
	}
	if (cv->delaysamples && p->s->sprp > cv->delaysamples)
	{
		triggerNote(p, fptr, cv, cv->delaynote, cv->delayinst);
		ret = triggerMidi(p, fptr, cv, cv->r.note, cv->delaynote, cv->delayinst);
		if (cv->delaynote == NOTE_OFF) cv->r.note = NOTE_OFF;

		/* dumb hack */
		short oldgain = cv->gain;
		short oldtargetgain = cv->targetgain;
		_triggerNote(cv, cv->delaynote, cv->delayinst);
		cv->gain = oldgain;
		cv->targetgain = oldtargetgain;

		cv->delaysamples = 0;
		ifMacro(fptr, cv, cv->r, 'O', &Oc); /* offset            */
		ifMacro(fptr, cv, cv->r, 'b', &bc); /* bw offset         */
		ifMacro(fptr, cv, cv->r, 'M', &Mc); /* microtonal offset */
	}

	if (cv->finetune != 0.0f && !(p->s->sprp % PITCH_WHEEL_SAMPLES) && iv->flags&S_FLAG_MIDI)
		midiPitchWheel(fptr, iv->midichannel, MIN(2.0f, MAX(-2.0f, cv->finetune)));

	/* process the type */
	if (iv && cv->r.note != NOTE_VOID && cv->r.note != NOTE_OFF
			&& !(p->w->instrumentlockv == INST_GLOBAL_LOCK_FREE
			&& p->w->instrumentlocki == p->s->instrumenti[cv->r.inst]))
	{
		if (cv->rtrigsamples)
		{
			uint32_t rtrigoffset = (cv->pointer - cv->rtrigpointer) % cv->rtrigsamples;

			if (!rtrigoffset
					&& ((cv->reverse && cv->pointer < cv->rtrigpointer && cv->pointer > cv->rtrigsamples)
					|| (!cv->reverse && cv->pointer > cv->rtrigpointer)))
			{ // first sample of any retrigger but the first
				if (iv->flags&S_FLAG_MIDI)
				{
					midiNoteOff(fptr, iv->midichannel, cv->r.note, midiGain(cv, iv));
					midiNoteOn(fptr, iv->midichannel, cv->r.note, midiGain(cv, iv));
				}
				ramp(p, cv, p->s->instrumenti[cv->r.inst], cv->pointer - cv->rtrigsamples);
				cv->rampindex = 0;
			}

			samplerProcess(iv, cv, cv->rtrigpointer + rtrigoffset, &lo, &ro);
		} else samplerProcess(iv, cv, cv->pointer, &lo, &ro);

		if (cv->reverse) { if (cv->pointer) cv->pointer--; }
		else                                cv->pointer++;
	}

	applyGain(p, rowprogress, cv, iv, &lo, &ro);

	if (cv->rampbuffer && cv->rampindex < rampmax)
	{ /* ramping */
		float gain = (float)cv->rampindex / (float)rampmax;
		lo = lo * gain + cv->rampbuffer[cv->rampindex*2 + 0] * (1.0f - gain) * (cv->rampgain>>4)*DIV16;
		ro = ro * gain + cv->rampbuffer[cv->rampindex*2 + 1] * (1.0f - gain) * (cv->rampgain%16)*DIV16;
		cv->rampindex++;
	}

	/* denormals */
	if (fabsf(lo) < NOISE_GATE && fabsf(ro) < NOISE_GATE) return;

	/* waveshapers */ /* TODO: ramping */
	if (cv->waveshaperstrength)
	{
		float strength;
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
				strength = cv->waveshaperstrength*DIV16;
				lo = rectify(0, lo) * (MIN(strength, 0.5f) * 2) + lo * (1.0f - (MAX(strength, 0.5f) - 0.5f) * 2);
				ro = rectify(0, ro) * (MIN(strength, 0.5f) * 2) + ro * (1.0f - (MAX(strength, 0.5f) - 0.5f) * 2);
				break;
			case 3: /* rectifier x2 */
				strength = cv->waveshaperstrength*DIV16;
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
				strength = cv->waveshaperstrength*DIV16;
				lo = signedunsigned(lo) * (MIN(strength, 0.5f) * 2) + lo * (1.0f - (MAX(strength, 0.5f) - 0.5f) * 2);
				ro = signedunsigned(ro) * (MIN(strength, 0.5f) * 2) + ro * (1.0f - (MAX(strength, 0.5f) - 0.5f) * 2);
				break;
		}
	}

	/* gate */
	if (MAX(fabsf(lo), fabsf(ro)) < (cv->gate%16)*0.025f)
		cv->gateopen = MAX(0.5f - ((cv->gate>>4)*DIV16) * 0.5f, cv->gateopen - MIN_GATE_SPEED_SEC/samplerate - (cv->gate>>4)*DIV128);
	else
		cv->gateopen = MIN(1.0f, cv->gateopen + MIN_GATE_SPEED_SEC/samplerate + (cv->gate>>4)*DIV128);
	lo *= cv->gateopen; ro *= cv->gateopen;

	/* filter */
	float cutoff;
	switch (cv->filtermode)
	{
		case 0: /* low-pass  */
			if (cv->targetfiltercut != -1) cutoff = cv->filtercut*DIV255 + (cv->targetfiltercut - (short)cv->filtercut)*DIV255 * rowprogress;
			else if (cv->filtercut != 255) cutoff = cv->filtercut*DIV255;
			break;
		case 1: /* high-pass */
			if (cv->targetfiltercut != -1) cutoff = cv->filtercut*DIV255 + (cv->targetfiltercut - (short)cv->filtercut)*DIV255 * rowprogress;
			else if   (cv->filtercut != 0) cutoff = cv->filtercut*DIV255;
			break;
		case 2: case 3: /* band-pass / notch */
			if (cv->targetfiltercut != -1) cutoff = cv->filtercut*DIV255 + (cv->targetfiltercut - (short)cv->filtercut)*DIV255 * rowprogress;
			else                           cutoff = cv->filtercut*DIV255;
			break;
	}
	runSVFilter(&cv->fl, thirddegreepolynomial(lo), cutoff, cv->filterres);
	runSVFilter(&cv->fr, thirddegreepolynomial(ro), cutoff, cv->filterres);
	switch (cv->filtermode)
	{
		case 0: /* low-pass  */ lo = hardclip(cv->fl.l); ro = hardclip(cv->fr.l); break;
		case 1: /* high-pass */ lo = hardclip(cv->fl.h); ro = hardclip(cv->fr.h); break;
		case 2: /* band-pass */ lo = hardclip(cv->fl.b); ro = hardclip(cv->fr.b); break;
		case 3: /* notch     */ lo = hardclip(cv->fl.n); ro = hardclip(cv->fr.n); break;
	}

	if (!cv->mute)
	{ pb.out.l[fptr] += lo; pb.out.r[fptr] += ro; }
}

void bendUp(channel *cv, uint32_t spr, uint32_t count)
{
	cv->portamentofinetune = MIN((float)(cv->portamento - cv->r.note),
			cv->portamentofinetune + (12.0f / spr) * (cv->portamentospeed*DIV255) * count);
}

void bendDown(channel *cv, uint32_t spr, uint32_t count)
{
	cv->portamentofinetune = MAX((float)(cv->portamento - cv->r.note),
			cv->portamentofinetune - (12.0f / spr) * (cv->portamentospeed*DIV255) * count);
}

void preprocessRow(jack_nframes_t fptr, char midi, channel *cv, row r)
{
	char ret;
	uint8_t oldnote = NOTE_UNUSED;

	for (int i = 0; i < cv->macroc; i++)
		cv->r.macro[i] = r.macro[i];

	/* gain */
	if (cv->targetgain != -1)
	{
		cv->gain = cv->targetgain;
		cv->targetgain = -1;
	}
	/* filter */
	if (cv->targetfiltercut != -1)
	{
		cv->filtercut = cv->targetfiltercut;
		cv->targetfiltercut = -1;
	}
	/* retrigger */
	if (cv->rtrigsamples)
	{
		if (cv->rtrigblocksize) cv->rtrigblocksize--;
		else                    cv->rtrigsamples = 0;
	}
	/* vibrato */
	ret = ifMacro(fptr, cv, r, 'V', &Vc);
	if (!ret) cv->vibratosamples = 0;

	if (ifMacro(fptr, cv, r, '%', &PERCENTc)) return;

	ifMacro(fptr, cv, r, 'b', &bc); /* bpm */

	ret = ifMacro(fptr, cv, r, 'C', &Cc); /* cut */
	if (!ret && r.note != NOTE_VOID)
	{
		ret = ifMacro(fptr, cv, r, 'P', &Pc); /* pitch slide */
		if (!ret)
		{
			ret = ifMacro(fptr, cv, r, 'D', &Dc); /* delay */
			if (!ret)
			{
				oldnote = cv->r.note;
				if (r.inst == INST_VOID) { triggerNote(p, fptr, cv, r.note, cv->r.inst); _triggerNote(cv, r.note, cv->r.inst); }
				else                     { triggerNote(p, fptr, cv, r.note, r.inst); _triggerNote(cv, r.note, r.inst);         }
			}
		}
	}

	/* ramping */
	if (ifMacro(fptr, cv, r, 'G', &PRERAMP)
			|| ifMacro(fptr, cv, r, 'F', &PRERAMP)
			|| ifMacro(fptr, cv, r, 'Z', &PRERAMP))
	{
		ramp(p, cv, p->s->instrumenti[cv->r.inst], cv->pointer);
		cv->rampindex = 0;
	}
	ifMacro(fptr, cv, r, 'G', &GcPOSTRAMP);
	ifMacro(fptr, cv, r, 'F', &FcPOSTRAMP);
	ifMacro(fptr, cv, r, 'Z', &ZcPOSTRAMP);

	ifMacro(fptr, cv, r, 'g', &gc);
	ifMacro(fptr, cv, r, 'f', &fc);

	if (oldnote != NOTE_UNUSED && midi) /* trigger midi (needs to be after gain calculation) */
	{
		if (r.inst == INST_VOID) ret = triggerMidi(p, fptr, cv, oldnote, r.note, cv->r.inst);
		else                     ret = triggerMidi(p, fptr, cv, oldnote, r.note, r.inst);
		if (r.note == NOTE_OFF) cv->r.note = NOTE_OFF;
	}


	ifMacro(fptr, cv, r, 'M', &Mc); /* microtonal offset */
	ifMacro(fptr, cv, r, 'O', &Oc); /* offset            */
	ifMacro(fptr, cv, r, 'o', &oc); /* bw offset         */
	ifMacro(fptr, cv, r, 'R', &Rc); /* retrigger         */

	// ifMacro(fptr, cv, r, 'L', &Lc); /* low-pass          */
	// ifMacro(fptr, cv, r, 'H', &Hc); /* high-pass         */
	// ifMacro(fptr, cv, r, 'B', &Bc); /* band-pass         */
	// ifMacro(fptr, cv, r, 'N', &Nc); /* notch             */

	ifMacro(fptr, cv, r, 'W', &Wc); /* waveshaper        */
	ifMacro(fptr, cv, r, 't', &tc); /* gate              */
}

int process(jack_nframes_t nfptr, void *arg)
{
	playbackinfo *p = arg;
	channel *cv; instrument *iv;

	pb.in.l =    jack_port_get_buffer(p->in.l, nfptr);
	pb.in.r =    jack_port_get_buffer(p->in.r, nfptr);
	pb.out.l =   jack_port_get_buffer(p->out.l, nfptr); memset(pb.out.l, 0, nfptr * sizeof(sample_t));
	pb.out.r =   jack_port_get_buffer(p->out.r, nfptr); memset(pb.out.r, 0, nfptr * sizeof(sample_t));
	pb.midiout = jack_port_get_buffer(p->midiout, nfptr); jack_midi_clear_buffer(pb.midiout);

	if (p->lock == PLAY_LOCK_START) p->lock = PLAY_LOCK_CONT;
	if (p->lock == PLAY_LOCK_CONT) return 0;


	/* will no longer write to the record buffer */
	if (p->w->instrumentrecv == INST_REC_LOCK_PREP_END)    p->w->instrumentrecv = INST_REC_LOCK_END;
	if (p->w->instrumentrecv == INST_REC_LOCK_PREP_CANCEL) p->w->instrumentrecv = INST_REC_LOCK_CANCEL;
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
	/* force stop midi for every channel playing this instrument */
	if (p->w->instrumentlockv == INST_GLOBAL_INST_MUTE)
	{
		for (uint8_t c = 0; c < p->s->channelc; c++)
		{
			cv = &p->s->channelv[c];
			if (p->s->instrumenti[cv->r.inst] == p->w->instrumentlocki)
			{
				iv = p->s->instrumentv[p->s->instrumenti[cv->r.inst]];
				midiNoteOff(0, iv->midichannel, cv->r.note, midiGain(cv, iv));
				cv->r.note = NOTE_VOID;
			}
		} p->w->instrumentlockv = INST_GLOBAL_LOCK_OK;
	}
	/* force stop midi data when a channel is muted */
	if (p->w->instrumentlockv == INST_GLOBAL_CHANNEL_MUTE)
	{
		cv = &p->s->channelv[p->w->instrumentlocki];
		iv = p->s->instrumentv[p->s->instrumenti[cv->r.inst]];
		if (iv && iv->flags&S_FLAG_MIDI)
		{
			midiNoteOff(0, iv->midichannel, cv->r.note, midiGain(cv, iv));
			cv->r.note = NOTE_VOID;
		} p->w->instrumentlockv = INST_GLOBAL_LOCK_OK;
	}


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
				preprocessRow(0, 0, &p->w->previewchannel, p->w->previewrow);
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

			if (!cv->mute && p->s->instrumenti[cv->r.inst])
			{
				iv = p->s->instrumentv[p->s->instrumenti[cv->r.inst]];
				if (iv->flags&S_FLAG_MIDI) midiNoteOff(0, iv->midichannel, cv->r.note, midiGain(cv, iv));
			}

			cv->r.note = NOTE_VOID;
			cv->rtrigsamples = 0;
			cv->waveshaperstrength = 0;
			cv->gate = 0;
			cv->gateopen = 1.0f;
			cv->filtermode = 0;
			cv->filtercut = 255; cv->targetfiltercut = -1;
			cv->filterres = 0; cv->targetfilterres = -1;
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
		pattern *pt; channel *cv;
		for (uint8_t b = blockstart; b < p->s->songp; b++)
		{
			pt = p->s->patternv[p->s->patterni[p->s->songi[b]]];
			/* for each row */
			for (uint8_t r = 0; r < pt->rowc; r++)
				for (uint8_t c = 0; c < p->s->channelc; c++)
				{
					cv = &p->s->channelv[c];
					preprocessRow(nfptr, 0, cv, pt->rowv[c][r%pt->rowcc[c]+1]);
					if (cv->cutsamples && cv->delaysamples)
					{
						if (cv->cutsamples > cv->delaysamples)
						{
							cv->r.note = NOTE_VOID;
							cv->cutsamples = 0;
						} else
						{
							_triggerNote(cv, cv->delaynote, cv->delayinst);
							ifMacro(0, cv, cv->r, 'O', &Oc); /* offset            */
							ifMacro(0, cv, cv->r, 'b', &bc); /* bw offset         */
							ifMacro(0, cv, cv->r, 'M', &Mc); /* microtonal offset */
							if (cv->reverse)
							{
								if (cv->pointer > p->s->spr - cv->delaysamples)
									cv->pointer -= p->s->spr - cv->delaysamples;
								else cv->pointer = 0;
							} else cv->pointer += p->s->spr - cv->delaysamples;
							cv->delaysamples = 0;
						}
					} else if (cv->cutsamples)
					{
						cv->r.note = NOTE_VOID;
						cv->cutsamples = 0;
					} else if (cv->delaysamples)
					{
						_triggerNote(cv, cv->delaynote, cv->delayinst);
						ifMacro(0, cv, cv->r, 'O', &Oc); /* offset            */
						ifMacro(0, cv, cv->r, 'b', &bc); /* bw offset         */
						ifMacro(0, cv, cv->r, 'M', &Mc); /* microtonal offset */
						if (cv->reverse)
						{
							if (cv->pointer > p->s->spr - cv->delaysamples)
								cv->pointer -= p->s->spr - cv->delaysamples;
							else cv->pointer = 0;
						} else cv->pointer += p->s->spr - cv->delaysamples;
						cv->delaysamples = 0;
					} else if (cv->r.note != NOTE_VOID)
					{
						if (cv->reverse)
						{
							if (cv->pointer > p->s->spr) cv->pointer -= p->s->spr;
							else cv->pointer = 0;
						} else cv->pointer += p->s->spr;
					}

					if (cv->r.note != NOTE_VOID && cv->portamento != NOTE_VOID
							&& cv->portamentofinetune != (float)(cv->portamento - cv->r.note))
					{
						if (cv->portamentofinetune < (float)(cv->portamento - cv->r.note))
							bendUp(cv, p->s->spr, p->s->spr);
						else if (cv->portamentofinetune > (float)(cv->portamento - cv->r.note))
							bendDown(cv, p->s->spr, p->s->spr);
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
		channel *cv;
		for (uint8_t i = 0; i < p->s->channelc; i++)
		{
			cv = &p->s->channelv[i];
			triggerNote(p, 0, cv, NOTE_OFF, cv->r.inst);
			triggerMidi(p, 0, cv, cv->r.note, NOTE_OFF, cv->r.inst);
			cv->r.note = NOTE_OFF;
			_triggerNote(cv, NOTE_OFF, cv->r.inst);
			cv->r.note = NOTE_VOID;
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
				preprocessRow(fptr, 1, &p->s->channelv[c],
						p->s->patternv[p->s->patterni[p->s->songi[p->s->songp]]]->rowv[c]
						[p->s->songr % (p->s->patternv[p->s->patterni[p->s->songi[p->s->songp]]]->rowcc[c]+1)]);

		for (uint8_t c = 0; c < p->s->channelc; c++)
		{
			cv = &p->s->channelv[c];
			if (p->s->playing == PLAYING_CONT && cv->r.note != NOTE_VOID && cv->portamento != NOTE_VOID
					&& cv->portamentofinetune != (float)(cv->portamento - cv->r.note))
			{
				if (cv->portamentofinetune < (float)(cv->portamento - cv->r.note))
					bendUp(cv, p->s->spr, 1);
				else if (cv->portamentofinetune > (float)(cv->portamento - cv->r.note))
					bendDown(cv, p->s->spr, 1);
			} playChannel(fptr, p, cv);
		}
		/* play the preview */
		if (p->w->previewchannel.r.note != NOTE_VOID)
			playChannel(fptr, p, &p->w->previewchannel);

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
					} else if (p->s->songf[p->s->songp]) {} /* loop, ignore */
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
				} p->dirty = 1;
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
				c = (float)pb.in.l[fptr] * (float)SHRT_MAX;
				if      (c > SHRT_MAX) p->w->recbuffer[p->w->recptr * 2 + 0] = SHRT_MAX;
				else if (c < SHRT_MIN) p->w->recbuffer[p->w->recptr * 2 + 0] = SHRT_MIN;
				else                   p->w->recbuffer[p->w->recptr * 2 + 0] = c;
				c = (float)pb.in.r[fptr] * (float)SHRT_MAX;
				if      (c > SHRT_MAX) p->w->recbuffer[p->w->recptr * 2 + 1] = SHRT_MAX;
				else if (c < SHRT_MIN) p->w->recbuffer[p->w->recptr * 2 + 1] = SHRT_MIN;
				else                   p->w->recbuffer[p->w->recptr * 2 + 1] = c;
				p->w->recptr++;
			}
		}
	}

	if (ENABLE_BACKGROUND) updateBackground(nfptr, pb.out);
	return 0;
}
