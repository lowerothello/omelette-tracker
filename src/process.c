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
		memset(cv->rampbuffer, 0, sizeof(short) * rampmax * 2);

		/* set the gain */
		if (cv->gain != -1) cv->rampgain = cv->gain;
		else cv->rampgain = 0x88;

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
	}
}
jack_midi_data_t midiGain(channel *cv, instrument *iv)
{
	if (cv->targetgain != -1)
	{
		if (cv->gain != -1) return ((cv->targetgain>>4)<<3);
		else                return 64;
	} else
	{
		if (cv->gain != -1) return (cv->gain>>4)<<3;
		else                return 64;
	} return 0;
}
void applyGain(playbackinfo *p, float rowprogress, channel *cv, float *l, float *r)
{
	if (cv->targetgain != -1)
	{
		if (cv->gain != -1)
		{
			*l *= (cv->gain>>4)*DIV16 + ((cv->targetgain>>4) - (cv->gain>>4))*DIV16 * rowprogress;
			*r *= (cv->gain%16)*DIV16 + ((cv->targetgain%16) - (cv->gain%16))*DIV16 * rowprogress;
		} else
		{
			*l *= 0.5f + ((cv->targetgain>>4) - 8)*DIV16 * rowprogress;
			*r *= 0.5f + ((cv->targetgain%16) - 8)*DIV16 * rowprogress;
		}
	} else
	{
		if (cv->gain != -1) { *l *= (cv->gain>>4)*DIV16; *r *= (cv->gain%16)*DIV16; }
		else                { *l *= 0.5f; *r *= 0.5f; }
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
void midiPC(jack_nframes_t fptr, uint8_t midichannel, uint8_t program)
{
	jack_midi_data_t event[2] = {0b11000000 | midichannel, program};
	jack_midi_event_write(pb.midiout, fptr, event, 2);
}
void midiCC(jack_nframes_t fptr, uint8_t midichannel, uint8_t controller, uint8_t value)
{
	jack_midi_data_t event[3] = {0b10110000 | midichannel, controller, value};
	jack_midi_event_write(pb.midiout, fptr, event, 3);
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
		cv->localenvelope = -1;
		cv->localpitchshift = -1;
		cv->localcyclelength = -1;

		if (!cv->mute && p->s->instrumenti[inst])
		{
			p->s->instrumentv[p->s->instrumenti[inst]]->triggerflash = samplerate / buffersize *DIV1000 * INSTRUMENT_TRIGGER_FLASH_MS;
			p->dirty = 1;
		}
	}
}
void triggerNote(jack_nframes_t fptr, channel *cv, uint8_t note, uint8_t inst)
{
	if (p->s->instrumenti[cv->r.inst] && !(p->w->instrumentlockv == INST_GLOBAL_LOCK_FREE
			&& p->w->instrumentlocki == p->s->instrumenti[cv->r.inst]))
		ramp(p, cv, p->s->instrumenti[cv->r.inst], cv->pointer);
	cv->rampindex = 0; /* set this even if it's not populated so it always ramps out */
}
/* separate from triggerNote() cos gain needs to be calculated in between */
char triggerMidi(jack_nframes_t fptr, channel *cv, uint8_t oldnote, uint8_t note, uint8_t inst)
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
	} return 0;
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
{ calcVibrato(cv, m); return 1; }

char Bc(jack_nframes_t fptr, int m, channel *cv, row r)
{
	if (m == 0) changeBpm(p->s, p->s->songbpm);
	else        changeBpm(p->s, MAX(32, m));
	return 0;
}

char Cc(jack_nframes_t fptr, int m, channel *cv, row r)
{
	if (!m>>4)
	{ /* cut now */
		triggerNote(fptr, cv, NOTE_OFF, cv->r.inst);
		triggerMidi(fptr, cv, cv->r.note, NOTE_OFF, cv->r.inst);
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
	cv->portamentospeed = m+1; /* avoid P00 doing nothing */
	return 1;
}

char dc(jack_nframes_t fptr, int m, channel *cv, row r)
{
	if (m%16 == 0) return 0;
	cv->delaysamples = p->s->spr * m*DIV256;
	cv->delaynote = r.note;
	if (r.inst == INST_VOID) cv->delayinst = cv->r.inst;
	else                     cv->delayinst = r.inst;
	return 1;
}
char Dc(jack_nframes_t fptr, int m, channel *cv, row r)
{
	if (m%16 == 0) return 0;
	cv->delaysamples = p->s->spr * (float)(m>>4) / (float)(m%16);
	cv->delaynote = r.note;
	if (r.inst == INST_VOID) cv->delayinst = cv->r.inst;
	else                     cv->delayinst = r.inst;
	return 1;
}

char PRERAMP(jack_nframes_t fptr, int m, channel *cv, row r)
{ if (cv->pointer) return 1; return 0; }

char GcPOSTRAMP(jack_nframes_t fptr, int m, channel *cv, row r)
{ cv->gain = m; return 1; }
char gc(jack_nframes_t fptr, int m, channel *cv, row r)
{ cv->targetgain = m; return 1; }

char Rc(jack_nframes_t fptr, int m, channel *cv, row r)
{
	if (m%16 != 0)
	{
		cv->rtrigpointer = cv->pointer;
		cv->rtrigsamples = p->s->spr / (m%16);
		cv->rtrigblocksize = m>>4;
		return 1;
	} return 0;
}

char Wc(jack_nframes_t fptr, int m, channel *cv, row r)
{ cv->waveshaper = m>>4; cv->waveshaperstrength = m%16; return 1; }
char wc(jack_nframes_t fptr, int m, channel *cv, row r)
{ cv->waveshaper = m>>4; cv->targetwaveshaperstrength = m%16; return 1; }

char OCPRERAMP(jack_nframes_t fptr, int m, channel *cv, row r)
{ if (cv->pointer && r.note == NOTE_VOID) return 1; return 0; }

char OcPOSTRAMP(jack_nframes_t fptr, int m, channel *cv, row r)
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

char ocPOSTRAMP(jack_nframes_t fptr, int m, channel *cv, row r)
{
	cv->reverse = !cv->reverse;
	if (m) return OcPOSTRAMP(fptr, m, cv, r);
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
{ cv->filtercut = m; return 1; }
char fc(jack_nframes_t fptr, int m, channel *cv, row r)
{ cv->targetfiltercut = m; return 1; }

char ZcPOSTRAMP(jack_nframes_t fptr, int m, channel *cv, row r)
{
	cv->filtermode = (m>>4)%4; /* TODO: smooth filter mode changes, 24dB/oct modes */
	cv->filterres = m%16;
	return 1;
}
char zc(jack_nframes_t fptr, int m, channel *cv, row r)
{
	cv->filtermode = (m>>4)%4; /* TODO: smooth filter mode changes, 24dB/oct modes */
	cv->targetfilterres = m%16;
	return 1;
}
char Ec(jack_nframes_t fptr, int m, channel *cv, row r)
{ cv->localenvelope = m; return 1; }
char Hc(jack_nframes_t fptr, int m, channel *cv, row r)
{ cv->localpitchshift = m; return 1; }
char Lc(jack_nframes_t fptr, int m, channel *cv, row r)
{ cv->localcyclelength = m; return 1; }
char midicctargetc(jack_nframes_t fptr, int m, channel *cv, row r)
{ cv->midiccindex = m%128; return 1; }
char midipcc(jack_nframes_t fptr, int m, channel *cv, row r)
{
	if (!cv->mute && p->s->instrumenti[r.inst != INST_VOID ? r.inst : cv->r.inst])
	{
		instrument *iv = p->s->instrumentv[p->s->instrumenti[r.inst != INST_VOID ? r.inst : cv->r.inst]];
		if (iv->flags&S_FLAG_MIDI) midiPC(fptr, iv->midichannel, m%128);
	} return 1;
}
char midiccc(jack_nframes_t fptr, int m, channel *cv, row r)
{
	cv->midicc = m%128;
	if (cv->midiccindex != -1 && !cv->mute && p->s->instrumenti[r.inst != INST_VOID ? r.inst : cv->r.inst])
	{
		instrument *iv = p->s->instrumentv[p->s->instrumenti[r.inst != INST_VOID ? r.inst : cv->r.inst]];
		if (iv->flags&S_FLAG_MIDI) midiCC(fptr, iv->midichannel, cv->midiccindex, cv->midicc);
	} return 1;
}
char smoothmidiccc(jack_nframes_t fptr, int m, channel *cv, row r)
{ cv->targetmidicc = m%128; return 1; }

void playChannel(jack_nframes_t fptr, playbackinfo *p, channel *cv)
{
	instrument *iv = p->s->instrumentv[p->s->instrumenti[cv->r.inst]];
	short li = 0; short ri = 0; /* the compiler REALLY wants to make these static */

	float rowprogress = (float)p->s->sprp / (float)p->s->spr;

	cv->finetune = cv->portamentofinetune + cv->microtonalfinetune;
	if (cv->vibratosamples)
	{
		cv->finetune += triosc((float)cv->vibratosamplepointer / (float)cv->vibratosamples) * cv->vibrato*DIV16;

		cv->vibratosamplepointer++;
		/* re-read the macro once phase is about to overflow */
		if (cv->vibratosamplepointer > cv->vibratosamples)
		{
			cv->vibratosamplepointer = 0;
			if (!ifMacro(fptr, cv, cv->r, 'V', &Vc))
				cv->vibratosamples = 0;
		}
	}

	if (cv->cutsamples && p->s->sprp > cv->cutsamples)
	{
		triggerNote(fptr, cv, NOTE_OFF, cv->r.inst);
		triggerMidi(fptr, cv, cv->r.note, NOTE_OFF, cv->r.inst);
		cv->r.note = NOTE_OFF;
		_triggerNote(cv, NOTE_OFF, cv->r.inst);
		cv->cutsamples = 0;
	}
	if (cv->delaysamples && p->s->sprp > cv->delaysamples)
	{
		triggerNote(fptr, cv, cv->delaynote, cv->delayinst);
		triggerMidi(fptr, cv, cv->r.note, cv->delaynote, cv->delayinst);
		if (cv->delaynote == NOTE_OFF) cv->r.note = NOTE_OFF;

		short oldgain = cv->gain;
		short oldtargetgain = cv->targetgain;
		_triggerNote(cv, cv->delaynote, cv->delayinst);
		if (ifMacro(fptr, cv, cv->r, 'G', &GcPOSTRAMP))
		{
			cv->gain = oldgain;
			cv->targetgain = oldtargetgain;
		}

		cv->delaysamples = 0;

		if (ifMacro(fptr, cv, cv->r, 'O', &OCPRERAMP) || ifMacro(fptr, cv, cv->r, 'o', &OCPRERAMP))
		{ ramp(p, cv, p->s->instrumenti[cv->r.inst], cv->pointer); cv->rampindex = 0; }

		ifMacro(fptr, cv, cv->r, 'O', &OcPOSTRAMP); /* offset    */
		ifMacro(fptr, cv, cv->r, 'o', &ocPOSTRAMP); /* bw offset */
		ifMacro(fptr, cv, cv->r, 'M', &Mc); /* microtonal offset */
		ifMacro(fptr, cv, cv->r, 'E', &Ec); /* local envelope    */
	}

	if (!cv->mute && cv->finetune != 0.0f && !(p->s->sprp % PITCH_WHEEL_SAMPLES) && iv->flags&S_FLAG_MIDI)
		midiPitchWheel(fptr, iv->midichannel, MIN(2.0f, MAX(-2.0f, cv->finetune)));
	if (!cv->mute && cv->targetmidicc != -1 && !(p->s->sprp % PITCH_WHEEL_SAMPLES) && iv->flags&S_FLAG_MIDI)
		midiCC(fptr, iv->midichannel, cv->midiccindex, cv->midicc + (cv->targetmidicc - cv->midicc) * rowprogress);

	/* process the sampler */
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
			} samplerProcess(iv, cv, cv->rtrigpointer + rtrigoffset, &li, &ri);
		} else samplerProcess(iv, cv, cv->pointer, &li, &ri);

		if (cv->reverse) { if (cv->pointer) cv->pointer--; }
		else                                cv->pointer++;
	}

	float lf = (float)li*DIVSHRT;
	float rf = (float)ri*DIVSHRT;

	applyGain(p, rowprogress, cv, &lf, &rf);

	if (cv->rampbuffer && cv->rampindex < rampmax)
	{ /* ramping */
		float gain = (float)cv->rampindex / (float)rampmax;
		lf = lf * gain + ((float)cv->rampbuffer[cv->rampindex*2 + 0]*DIVSHRT) * (1.0f - gain) * (cv->rampgain>>4)*DIV16;
		rf = rf * gain + ((float)cv->rampbuffer[cv->rampindex*2 + 1]*DIVSHRT) * (1.0f - gain) * (cv->rampgain%16)*DIV16;
		cv->rampindex++;
	}

	/* denormals */
	if (fabsf(lf) < NOISE_GATE && fabsf(rf) < NOISE_GATE) { lf = 0.0f; rf = 0.0f; }

	/* waveshapers */
	if (cv->targetwaveshaperstrength != -1 || cv->waveshaperstrength)
	{
		float strength, hold;
		switch (cv->waveshaper)
		{
			case 0: /* hard clip */
				if (cv->targetwaveshaperstrength != -1)
					strength = 1.0f + ((cv->waveshaperstrength<<1) + ((cv->targetwaveshaperstrength - cv->waveshaperstrength)<<1) * rowprogress);
				else
					strength = 1.0f + (cv->waveshaperstrength<<1);
				hold = 1.0f / sqrtf(strength);
				lf = hardclip(lf*strength)*hold;
				rf = hardclip(rf*strength)*hold;
				break;
			case 1: /* soft clip */
				if (cv->targetwaveshaperstrength != -1)
					strength = 1.0f + cv->waveshaperstrength + (cv->targetwaveshaperstrength - cv->waveshaperstrength) * rowprogress;
				else
					strength = 1.0f + cv->waveshaperstrength;
				hold = 1.0f / sqrtf(strength);
				lf = thirddegreepolynomial(lf*strength)*hold;
				rf = thirddegreepolynomial(rf*strength)*hold;
				break;
			case 2: /* rectify */
				if (cv->targetwaveshaperstrength != -1)
					strength = (cv->waveshaperstrength + (cv->targetwaveshaperstrength - cv->waveshaperstrength) * rowprogress)*DIV15;
				else
					strength = cv->waveshaperstrength*DIV15;
				lf = rectify(lf) * (MIN(strength, 0.5f) * 2) + lf * (1.0f - (MAX(strength, 0.5f) - 0.5f) * 2);
				rf = rectify(rf) * (MIN(strength, 0.5f) * 2) + rf * (1.0f - (MAX(strength, 0.5f) - 0.5f) * 2);
				break;
			case 3: /* rectify x2 */
				if (cv->targetwaveshaperstrength != -1)
					strength = (cv->waveshaperstrength + (cv->targetwaveshaperstrength - cv->waveshaperstrength) * rowprogress)*DIV15;
				else
					strength = cv->waveshaperstrength*DIV15;
				lf = rectify(lf) * (MIN(strength, 0.5f) * 2) + lf * (1.0f - (MAX(strength, 0.5f) - 0.5f) * 2);
				rf = rectify(rf) * (MIN(strength, 0.5f) * 2) + rf * (1.0f - (MAX(strength, 0.5f) - 0.5f) * 2);
				lf = rectify(lf) * (MIN(strength, 0.5f) * 2) + lf * (1.0f - (MAX(strength, 0.5f) - 0.5f) * 2);
				rf = rectify(rf) * (MIN(strength, 0.5f) * 2) + rf * (1.0f - (MAX(strength, 0.5f) - 0.5f) * 2);
				break;
			case 4: /* wavefold */
				if (cv->targetwaveshaperstrength != -1)
					strength = 1.0f + cv->waveshaperstrength + (cv->targetwaveshaperstrength - cv->waveshaperstrength) * rowprogress;
				else
					strength = 1.0f + cv->waveshaperstrength;
				hold = 1.0f / sqrtf(strength);
				lf = wavefolder(lf*strength)*hold;
				rf = wavefolder(rf*strength)*hold;
				break;
			case 5: /* wavewrap */
				if (cv->targetwaveshaperstrength != -1)
					strength = 1.0f + cv->waveshaperstrength + (cv->targetwaveshaperstrength - cv->waveshaperstrength) * rowprogress;
				else
					strength = 1.0f + cv->waveshaperstrength;
				hold = 1.0f / sqrtf(strength);
				lf = wavewrapper(lf*strength)*hold;
				rf = wavewrapper(rf*strength)*hold;
				break;
			case 6: /* sign conversion */
				if (cv->targetwaveshaperstrength != -1)
					strength = (cv->waveshaperstrength + (cv->targetwaveshaperstrength - cv->waveshaperstrength) * rowprogress)*DIV15;
				else
					strength = cv->waveshaperstrength*DIV15;
				lf = signedunsigned(lf) * (MIN(strength, 0.5f) * 2) + lf * (1.0f - (MAX(strength, 0.5f) - 0.5f) * 2);
				rf = signedunsigned(rf) * (MIN(strength, 0.5f) * 2) + rf * (1.0f - (MAX(strength, 0.5f) - 0.5f) * 2);
				break;
			case 7: /* hard gate */
				if (cv->targetwaveshaperstrength != -1)
					strength = (cv->waveshaperstrength + (cv->targetwaveshaperstrength - cv->waveshaperstrength) * rowprogress)*DIV64;
				else
					strength = cv->waveshaperstrength*DIV64;
				if (fabsf(lf) < strength) lf = 0.0f;
				if (fabsf(rf) < strength) rf = 0.0f;
				break;
		}
	}

	/* filter */
	float cutoff = 1.0f;
	float resonance = 0.0f;
	switch (cv->filtermode)
	{
		case 0: /* low-pass */
			if (cv->targetfiltercut != -1) cutoff = cv->filtercut*DIV255 + (cv->targetfiltercut - (short)cv->filtercut)*DIV255 * rowprogress;
			else                           cutoff = cv->filtercut*DIV255;
			break;
		case 1: /* high-pass */
			if (cv->targetfiltercut != -1) cutoff = cv->filtercut*DIV255 + (cv->targetfiltercut - (short)cv->filtercut)*DIV255 * rowprogress;
			else                           cutoff = cv->filtercut*DIV255;
			break;
		case 2: case 3: /* band-pass / notch */
			if (cv->targetfiltercut != -1) cutoff = cv->filtercut*DIV255 + (cv->targetfiltercut - (short)cv->filtercut)*DIV255 * rowprogress;
			else                           cutoff = cv->filtercut*DIV255;
			break;
	}
	if (cv->targetfilterres != -1) resonance = cv->filterres*DIV15 + (cv->targetfilterres - cv->filterres)*DIV15 * rowprogress;
	else                           resonance = cv->filterres*DIV15;
	runSVFilter(&cv->fl, lf, cutoff, resonance);
	runSVFilter(&cv->fr, rf, cutoff, resonance);
	switch (cv->filtermode)
	{
		case 0: /* low-pass  */ if (!(cv->targetfiltercut == -1 && cv->filtercut == 255)) { lf = hardclip(cv->fl.l); rf = hardclip(cv->fr.l); } break;
		case 1: /* high-pass */ if (!(cv->targetfiltercut == -1 && cv->filtercut == 0)) { lf = hardclip(cv->fl.h); rf = hardclip(cv->fr.h); } break;
		case 2: /* band-pass */ lf = hardclip(cv->fl.b); rf = hardclip(cv->fr.b); break;
		case 3: /* notch     */ lf = hardclip(cv->fl.n); rf = hardclip(cv->fr.n); break;
	}

	if (!cv->mute) { pb.out.l[fptr] += lf; pb.out.r[fptr] += rf; }
}

void bendUp(channel *cv, uint32_t spr, uint32_t count) { cv->portamentofinetune = MIN((float)(cv->portamento - cv->r.note), cv->portamentofinetune + (12.0f / spr) * (cv->portamentospeed*DIV256) * count); }
void bendDown(channel *cv, uint32_t spr, uint32_t count) { cv->portamentofinetune = MAX((float)(cv->portamento - cv->r.note), cv->portamentofinetune - (12.0f / spr) * (cv->portamentospeed*DIV256) * count); }

void preprocessRow(jack_nframes_t fptr, char midi, channel *cv, row r)
{
	char ret;
	uint8_t oldnote = NOTE_UNUSED;

	for (int i = 0; i < cv->macroc; i++)
		cv->r.macro[i] = r.macro[i];

	/* end interpolation */
	if (cv->targetgain != -1) { cv->gain = cv->targetgain; cv->targetgain = -1; }
	if (cv->targetfiltercut != -1) { cv->filtercut = cv->targetfiltercut; cv->targetfiltercut = -1; }
	if (cv->targetfilterres != -1) { cv->filterres = cv->targetfilterres; cv->targetfilterres = -1; }
	if (cv->targetwaveshaperstrength != -1) { cv->waveshaperstrength = cv->targetwaveshaperstrength; cv->targetwaveshaperstrength = -1; }
	if (cv->targetmidicc != -1) { cv->midicc = cv->targetmidicc; cv->targetmidicc = -1; }
	if (cv->rtrigsamples) { if (cv->rtrigblocksize) cv->rtrigblocksize--; else cv->rtrigsamples = 0; }

	ret = ifMacro(fptr, cv, r, 'V', &Vc);
	if (!ret) cv->vibratosamples = 0;

	if (ifMacro(fptr, cv, r, '%', &PERCENTc)) return;

	ifMacro(fptr, cv, r, 'b', &Bc); /* bpm */

	ret = ifMacro(fptr, cv, r, 'C', &Cc); /* cut */
	if (!ret && r.note != NOTE_VOID)
	{
		ret = ifMacro(fptr, cv, r, 'P', &Pc); /* pitch slide */
		if (!ret)
		{
			ret = (ifMacro(fptr, cv, r, 'D', &Dc) || ifMacro(fptr, cv, r, 'd', &dc)); /* delay */
			if (!ret)
			{
				oldnote = cv->r.note;
				if (r.inst == INST_VOID) { triggerNote(fptr, cv, r.note, cv->r.inst); _triggerNote(cv, r.note, cv->r.inst); }
				else                     { triggerNote(fptr, cv, r.note, r.inst); _triggerNote(cv, r.note, r.inst);         }
			}
		}
	}

	/* ramping */
	if (ifMacro(fptr, cv, r, 'G', &PRERAMP)
			|| ifMacro(fptr, cv, r, 'F', &PRERAMP)
			|| ifMacro(fptr, cv, r, 'Z', &PRERAMP)
			|| ifMacro(fptr, cv, r, 'O', &OCPRERAMP)
			|| ifMacro(fptr, cv, r, 'o', &OCPRERAMP))
	{ ramp(p, cv, p->s->instrumenti[cv->r.inst], cv->pointer); cv->rampindex = 0; }

	ifMacro(fptr, cv, r, 'O', &OcPOSTRAMP); /* offset    */
	ifMacro(fptr, cv, r, 'o', &ocPOSTRAMP); /* bw offset */

	ifMacro(fptr, cv, r, 'G', &GcPOSTRAMP);
	ifMacro(fptr, cv, r, 'F', &FcPOSTRAMP);
	ifMacro(fptr, cv, r, 'Z', &ZcPOSTRAMP);

	ifMacro(fptr, cv, r, 'g', &gc);
	ifMacro(fptr, cv, r, 'f', &fc);
	ifMacro(fptr, cv, r, 'Z', &zc);

	if (oldnote != NOTE_UNUSED && midi) /* trigger midi (needs to be after gain calculation) */
	{
		if (r.inst == INST_VOID) ret = triggerMidi(fptr, cv, oldnote, r.note, cv->r.inst);
		else                     ret = triggerMidi(fptr, cv, oldnote, r.note, r.inst);
		if (r.note == NOTE_OFF) cv->r.note = NOTE_OFF;
	}
	ifMacro(fptr, cv, r, ';', &midicctargetc);
	ifMacro(fptr, cv, r, '@', &midipcc);
	ifMacro(fptr, cv, r, '.', &midiccc);
	ifMacro(fptr, cv, r, ',', &smoothmidiccc);


	ifMacro(fptr, cv, r, 'M', &Mc); /* microtonal offset */
	ifMacro(fptr, cv, r, 'R', &Rc); /* retrigger         */

	ifMacro(fptr, cv, r, 'W', &Wc); /* waveshaper        */
	ifMacro(fptr, cv, r, 'w', &wc); /* smooth waveshaper */

	ifMacro(fptr, cv, r, 'E', &Ec); /* local envelope    */
	ifMacro(fptr, cv, r, 'H', &Hc); /* local pitch shift */
	ifMacro(fptr, cv, r, 'L', &Lc); /* local cyclelength */
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
	/* start recording immediately if not cueing */
	if (p->w->instrumentrecv == INST_REC_LOCK_START)
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
				p->w->previewchannel.waveshaperstrength = 0; p->w->previewchannel.targetwaveshaperstrength = -1;
				p->w->previewchannel.filtermode = 0;
				p->w->previewchannel.filtercut = 255; p->w->previewchannel.targetfiltercut = -1;
				p->w->previewchannel.filterres = 0; p->w->previewchannel.targetfilterres = -1;
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
			cv->waveshaperstrength = 0; cv->targetwaveshaperstrength = -1;
			cv->filtermode = 0;
			cv->filtercut = 255; cv->targetfiltercut = -1;
			cv->filterres = 0; cv->targetfilterres = -1;
			cv->midiccindex = -1; cv->midicc = 0; cv->targetmidicc = -1;
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
							ifMacro(0, cv, cv->r, 'O', &OcPOSTRAMP); /* offset    */
							ifMacro(0, cv, cv->r, 'b', &ocPOSTRAMP); /* bw offset */
							ifMacro(0, cv, cv->r, 'M', &Mc); /* microtonal offset */
							ifMacro(0, cv, cv->r, 'E', &Ec); /* local envelope    */
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
						ifMacro(0, cv, cv->r, 'O', &OcPOSTRAMP); /* offset    */
						ifMacro(0, cv, cv->r, 'b', &ocPOSTRAMP); /* bw offset */
						ifMacro(0, cv, cv->r, 'M', &Mc); /* microtonal offset */
						ifMacro(0, cv, cv->r, 'E', &Ec); /* local envelope    */
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
		/* start recording if cueing */
		if (p->w->instrumentrecv == INST_REC_LOCK_CUE_START)
			p->w->instrumentrecv = INST_REC_LOCK_CUE_CONT;
		p->s->playing = PLAYING_CONT;
	} else if (p->s->playing == PLAYING_PREP_STOP)
	{
		/* stop channels */
		channel *cv;
		for (uint8_t i = 0; i < p->s->channelc; i++)
		{
			cv = &p->s->channelv[i];
			cv->delaysamples = 0;
			cv->cutsamples = 0;
			triggerNote(0, cv, NOTE_OFF, cv->r.inst);
			triggerMidi(0, cv, cv->r.note, NOTE_OFF, cv->r.inst);
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
					
					/* stop recording if cueing */
					if (p->w->instrumentrecv == INST_REC_LOCK_CUE_CONT)
						p->w->instrumentrecv = INST_REC_LOCK_END;
					/* start recording if cueing */
					if (p->w->instrumentrecv == INST_REC_LOCK_CUE_START)
						p->w->instrumentrecv = INST_REC_LOCK_CUE_CONT;

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
	if (p->w->instrumentrecv == INST_REC_LOCK_CONT
			|| p->w->instrumentrecv == INST_REC_LOCK_CUE_CONT)
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
				if (p->w->recptr % samplerate == 0) p->dirty = 1;
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
	for (int i = 1; i < p->s->instrumentc; i++)
		if (p->s->instrumentv[i]->triggerflash)
		{
			if (p->s->instrumentv[i]->triggerflash == 1) p->dirty = 1;
			p->s->instrumentv[i]->triggerflash--;
		}

	return 0;
}
