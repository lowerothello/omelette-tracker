char ifMacro(jack_nframes_t fptr, channel *cv, row r, char m, char (*callback)(jack_nframes_t, int, channel *, row))
{
	char ret = 0;
	for (int i = 0; i <= cv->macroc; i++)
		if (r.macro[i].c == m)
			ret = callback(fptr, r.macro[i].v, cv, r);
	return ret;
}

void changeBpm(song *s, uint8_t newbpm)
{
	s->bpm = newbpm;
	s->spr = samplerate * (60.0 / newbpm) / s->rowhighlight;
}

int envelope(instrument *iv, channel *cv, uint32_t pointer)
{
	uint8_t env;
	if (cv->localenvelope != -1) env = cv->localenvelope;
	else                         env = iv->envelope;

	uint32_t alen = ((env>>4)+ENVELOPE_A_MIN) * ENVELOPE_A_STEP * samplerate;
	uint32_t dlen = ((env%16)+ENVELOPE_D_MIN) * ENVELOPE_D_STEP * samplerate;

	if (cv->flags&C_FLAG_RELEASE || ((iv->flags&S_FLAG_SUSTAIN) && pointer > alen))
	     { if (dlen) cv->envgain = MAX(cv->envgain - (1.0f/dlen), 0.0f); else cv->envgain = 0.0f; }
	else { if (alen) cv->envgain = MIN(cv->envgain + (1.0f/alen), 1.0f); else cv->envgain = 1.0f; }

	if (pointer > alen && cv->envgain < NOISE_GATE)
		return 0;
	return 1;
}

/* freewheel to fill up the ramp buffer */
void ramp(channel *cv, uint8_t realinstrument, uint32_t pointeroffset, uint32_t pitchedpointeroffset)
{
	if (cv->rampbuffer)
	{
		instrument *iv = p->s->instrumentv[realinstrument];

		/* clear the rampbuffer properly so cruft isn't played in edge cases */
		memset(cv->rampbuffer, 0, sizeof(short) * rampmax * 2);

		/* save state */
		cv->rampgain = cv->randgain;
		cv->rampinst = realinstrument;

		/* consistant filters */ /* TODO: seems to be broken and causing clicks */
		memcpy(&cv->rampfl, &cv->fl, sizeof(SVFilter) * 2);
		memcpy(&cv->rampfr, &cv->fr, sizeof(SVFilter) * 2);

		if (iv)
		{
			float multiplier = powf(M_12_ROOT_2, (short)cv->samplernote - NOTE_C5 + cv->finetune);
			pitchedpointeroffset += (int)((cv->pointer+1)*multiplier) - (int)(cv->pointer*multiplier);
			float oldenvgain = cv->envgain;
			if (cv->flags&C_FLAG_REVERSE)
			{
				jack_nframes_t localrampmax;
				if (pointeroffset < rampmax)
					localrampmax = rampmax - pointeroffset;
				else localrampmax = rampmax;

				uint32_t delta;
				for (uint16_t i = 0; i < localrampmax; i++)
				{
					delta = (int)((pointeroffset+i+1)*multiplier) - (int)((pointeroffset+i)*multiplier);
					if (pitchedpointeroffset > delta) pitchedpointeroffset -= delta;
					else                              pitchedpointeroffset = 0;
					if (envelope(iv, cv, cv->pointer) && pitchedpointeroffset < iv->length)
						samplerProcess(iv, cv, pointeroffset + i, pitchedpointeroffset,
								&cv->rampbuffer[i*2 + 0], &cv->rampbuffer[i*2 + 1]);
				}
			} else
				for (uint16_t i = 0; i < rampmax; i++)
				{
					pitchedpointeroffset += (int)((pointeroffset+i+1)*multiplier) - (int)((pointeroffset+i)*multiplier);
					if (envelope(iv, cv, cv->pointer) && pitchedpointeroffset < iv->length)
						samplerProcess(iv, cv, pointeroffset + i, pitchedpointeroffset,
								&cv->rampbuffer[i*2 + 0], &cv->rampbuffer[i*2 + 1]);
				}
			cv->envgain = oldenvgain;
		}
	} cv->rampindex = 0;
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
{ jack_midi_data_t event[2] = {0b11000000 | midichannel, program}; jack_midi_event_write(pb.midiout, fptr, event, 2); }
void midiCC(jack_nframes_t fptr, uint8_t midichannel, uint8_t controller, uint8_t value)
{ jack_midi_data_t event[3] = {0b10110000 | midichannel, controller, value}; jack_midi_event_write(pb.midiout, fptr, event, 3); }

void triggerNote(channel *cv, uint8_t note, uint8_t inst)
{
	if (note == NOTE_VOID) return;
	if (note == NOTE_OFF)
	{
		if (!(cv->flags&C_FLAG_RELEASE)) cv->flags ^= C_FLAG_RELEASE;
		cv->r.inst = inst;
		cv->r.note = note;
	} else
	{
		cv->r.inst = cv->samplerinst = inst;
		cv->r.note = cv->samplernote = note;
		cv->pointer = cv->pitchedpointer = 0;
		if (cv->flags&C_FLAG_REVERSE) cv->flags ^= C_FLAG_REVERSE;
		if (cv->flags&C_FLAG_RELEASE) cv->flags ^= C_FLAG_RELEASE;
		cv->portamentosamples = 0; cv->portamentosamplepointer = 1;
		cv->startportamentofinetune = cv->targetportamentofinetune = cv->portamentofinetune = 0.0f;
		cv->microtonalfinetune = 0.0f;
		cv->vibrato = 0;
		cv->localenvelope = -1;
		cv->localpitchshift = -1;
		cv->localcyclelength = -1;
		
		/* must stop retriggers cos pointers are no longer guaranteed to be valid */
		cv->rtrigblocksize = 0;
		if (cv->flags&C_FLAG_RTRIG_REV) cv->flags ^= C_FLAG_RTRIG_REV;
		cv->rtrigsamples = 0;

		if (!(cv->flags&C_FLAG_MUTE) && p->s->instrumenti[inst])
		{
			p->s->instrumentv[p->s->instrumenti[inst]]->triggerflash = samplerate / buffersize *DIV1000 * INSTRUMENT_TRIGGER_FLASH_MS;
			p->dirty = 1;
		}
	}
}

char triggerMidi(jack_nframes_t fptr, channel *cv, uint8_t oldnote, uint8_t note, uint8_t inst)
{
	if (note != NOTE_VOID && !(cv->flags&C_FLAG_MUTE) && p->s->instrumenti[inst])
	{
		instrument *iv = p->s->instrumentv[p->s->instrumenti[inst]];
		if (iv->flags&S_FLAG_MIDI)
		{
			/* always stop the prev. note */
			midiNoteOff(fptr, iv->midichannel, oldnote, (cv->randgain>>4)<<3);
			midiNoteOn(fptr, iv->midichannel, note, (cv->randgain>>4)<<3);
			return 1;
		}
	} return 0;
}


char Vc(jack_nframes_t fptr, int m, channel *cv, row r)
{
	cv->vibrato = m%16;
	if (!cv->vibratosamples) /* reset the phase if starting */
		cv->vibratosamplepointer = 0;
	cv->vibratosamples = p->s->spr / (((m>>4) + 1) / 16.0); /* use floats for slower lfo speeds */
	cv->vibratosamplepointer = MIN(cv->vibratosamplepointer, cv->vibratosamples - 1);
	return 1;
}

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
		ramp(cv, p->s->instrumenti[cv->samplerinst], cv->pointer, cv->pitchedpointer);
		triggerMidi(fptr, cv, cv->r.note, NOTE_OFF, cv->r.inst);
		triggerNote(cv, NOTE_OFF, cv->r.inst);
		cv->cutsamples = 0;
		return 1;
	} else if (m%16 != 0) /* cut later */
		cv->cutsamples = p->s->spr * m*DIV256;
	return 0;
}

char Pc(jack_nframes_t fptr, int m, channel *cv, row r)
{
	if (cv->portamentosamplepointer > cv->portamentosamples)
	{
		cv->portamentosamples = (p->s->spr * m)/16;
		cv->portamentosamplepointer = 0;
		cv->startportamentofinetune = cv->portamentofinetune;
		cv->targetportamentofinetune = (r.note - (cv->r.note + cv->portamentofinetune));
	} return 1;
}

char Dc(jack_nframes_t fptr, int m, channel *cv, row r)
{
	if (!(m%16)) return 0;
	cv->delaysamples = p->s->spr * m*DIV256;
	cv->delaynote = r.note;
	if (r.inst == INST_VOID) cv->delayinst = cv->r.inst;
	else                     cv->delayinst = r.inst;
	return 1;
}

char DUMMY(jack_nframes_t fptr, int m, channel *cv, row r) { return 1; }

char GcPOSTRAMP(jack_nframes_t fptr, int m, channel *cv, row r)
{
	cv->gain = cv->randgain = m;
	return 1;
}
char gc(jack_nframes_t fptr, int m, channel *cv, row r)
{
	cv->targetgain = m;
	return 1;
}
char IcPOSTRAMP(jack_nframes_t fptr, int m, channel *cv, row r)
{
	signed char stereo = rand()%((m>>4)+1);
	cv->randgain =
		 (MAX(0, (cv->gain>>4) - stereo - rand()%((m%16)+1))<<4)
		+ MAX(0, (cv->gain%16) - stereo - rand()%((m%16)+1));
	return 1;
}
char ic(jack_nframes_t fptr, int m, channel *cv, row r)
{
	if (!(cv->flags&C_FLAG_TARGET_RAND)) cv->flags ^= C_FLAG_TARGET_RAND;
	signed char stereo = rand()%((m>>4)+1);
	cv->targetgain =
		 (MAX(0, (cv->gain>>4) - stereo - rand()%((m%16)+1))<<4)
		+ MAX(0, (cv->gain%16) - stereo - rand()%((m%16)+1));
	return 1;
}

char Qc(jack_nframes_t fptr, int m, channel *cv, row r)
{
	if (m)
	{
		if (cv->rtrigblocksize >= 0)
		{ /* starting a new chain */
			cv->rtrigpointer = cv->pointer;
			cv->rtrigpitchedpointer = cv->rtrigcurrentpitchedpointer = cv->pitchedpointer;
		}
		cv->rtrigblocksize = -1;
		cv->rtrigsamples = p->s->spr*DIV256 * m;
		return 1;
	} return 0;
}
char qc(jack_nframes_t fptr, int m, channel *cv, row r)
{
	if (m)
	{
		if (!(cv->flags&C_FLAG_RTRIG_REV)) cv->flags ^= C_FLAG_RTRIG_REV;
		return Qc(fptr, m, cv, r);
	} return 0;
}
char Rc(jack_nframes_t fptr, int m, channel *cv, row r)
{
	cv->rtrigpointer = cv->pointer;
	cv->rtrigpitchedpointer = cv->rtrigcurrentpitchedpointer = cv->pitchedpointer;
	cv->rtrigblocksize = m>>4;
	if (m%16) cv->rtrigsamples = p->s->spr / (m%16);
	else      cv->rtrigsamples = p->s->spr * (cv->rtrigblocksize+1);
	return 1;
}
char rc(jack_nframes_t fptr, int m, channel *cv, row r)
{
	if (!(cv->flags&C_FLAG_RTRIG_REV)) cv->flags ^= C_FLAG_RTRIG_REV;
	return Rc(fptr, m, cv, r);
}

char Wc(jack_nframes_t fptr, int m, channel *cv, row r) { cv->waveshaper = m>>4; cv->waveshaperstrength = m%16; return 1; }
char wc(jack_nframes_t fptr, int m, channel *cv, row r) { cv->waveshaper = m>>4; cv->targetwaveshaperstrength = m%16; return 1; }

char OcPRERAMP(jack_nframes_t fptr, int m, channel *cv, row r)
{ if (r.note == NOTE_VOID) return 1; return 0; }
char OcPOSTRAMP(jack_nframes_t fptr, int m, channel *cv, row r)
{
	instrument *iv = p->s->instrumentv[p->s->instrumenti[cv->r.inst]];
	if (iv && cv->r.note != NOTE_VOID) /* if playing a note */
	{
		if (r.note == NOTE_VOID) /* if not changing note, explicit ramping needed */
			ramp(cv, p->s->instrumenti[cv->samplerinst], cv->pointer, cv->pitchedpointer);
		cv->pitchedpointer = (m*DIV255) * (iv->trim[1] - iv->trim[0]);
	} return 0;
}
char ocPOSTRAMP(jack_nframes_t fptr, int m, channel *cv, row r)
{
	cv->flags ^= C_FLAG_REVERSE;
	if (m) return OcPOSTRAMP(fptr, m, cv, r);
	return 0;
}
char UcPOSTRAMP(jack_nframes_t fptr, int m, channel *cv, row r)
{
	instrument *iv = p->s->instrumentv[p->s->instrumenti[cv->r.inst]];
	if (iv && cv->r.note != NOTE_VOID) /* if playing a note */
	{
		if (r.note == NOTE_VOID) /* if not changing note, explicit ramping needed */
			ramp(cv, p->s->instrumenti[cv->samplerinst], cv->pointer, cv->pitchedpointer);
		if (m>>4 == m%16) /* both nibbles are the same */
			cv->pitchedpointer = ((((m>>4)<<4) + rand()%16)*DIV255) * (iv->trim[1] - iv->trim[0]);
		else
		{
			int min = MIN(m>>4, m%16);
			int max = MAX(m>>4, m%16);
			cv->pitchedpointer = (((min + rand()%(max - min +1))<<4)*DIV255) * (iv->trim[1] - iv->trim[0]);
		}
	} return 0;
}
char ucPOSTRAMP(jack_nframes_t fptr, int m, channel *cv, row r)
{
	cv->flags ^= C_FLAG_REVERSE;
	return UcPOSTRAMP(fptr, m, cv, r);
}

char Mc(jack_nframes_t fptr, int m, channel *cv, row r) { cv->microtonalfinetune = m*DIV255; return 0; }

char PERCENTc(jack_nframes_t fptr, int m, channel *cv, row r) /* returns true to NOT play */
{
	if (rand()%256 > m) return 1;
	else                return 0;
}

char FcPOSTRAMP(jack_nframes_t fptr, int m, channel *cv, row r) { cv->filtercut = m; return 1; }
char fc(jack_nframes_t fptr, int m, channel *cv, row r)   { cv->targetfiltercut = m; return 1; }
char ZcPOSTRAMP(jack_nframes_t fptr, int m, channel *cv, row r)
{
	if ((m>>4) < 8) cv->filtermode = m>>4;
	else            cv->targetfiltermode = (m>>4) - 8;
	cv->filterres = m%16;
	return 1;
}
char zc(jack_nframes_t fptr, int m, channel *cv, row r)
{
	if ((m>>4) < 8) cv->filtermode = m>>4;
	else            cv->targetfiltermode = (m>>4) - 8;
	cv->targetfilterres = m%16;
	return 1;
}

char Sc(jack_nframes_t fptr, int m, channel *cv, row r)
{
	cv->sendgroup = m>>4;
	cv->sendgain = m%16;
	return 1;
}
char sc(jack_nframes_t fptr, int m, channel *cv, row r)
{
	cv->sendgroup = m>>4;
	cv->targetsendgain = m%16;
	return 1;
}
char Ec(jack_nframes_t fptr, int m, channel *cv, row r) { cv->localenvelope = m; return 1; }
char Hc(jack_nframes_t fptr, int m, channel *cv, row r) { cv->localpitchshift = m; return 1; }
char Lc(jack_nframes_t fptr, int m, channel *cv, row r)
{
	instrument *iv = p->s->instrumentv[p->s->instrumenti[cv->r.inst]];
	if (cv->localcyclelength == -1) cv->localcyclelength = iv->cyclelength;
	cv->localcyclelength = (((uint16_t)cv->localcyclelength<<8)>>8) + (m<<8);
	return 1;
}
char lc(jack_nframes_t fptr, int m, channel *cv, row r)
{
	instrument *iv = p->s->instrumentv[p->s->instrumenti[cv->r.inst]];
	if (cv->localcyclelength == -1) cv->localcyclelength = iv->cyclelength;
	cv->localcyclelength = (((uint16_t)cv->localcyclelength>>8)<<8)+m;
	return 1;
}
char midicctargetc(jack_nframes_t fptr, int m, channel *cv, row r) { cv->midiccindex = m%128; return 1; }
char midipcc(jack_nframes_t fptr, int m, channel *cv, row r)
{
	if (!(cv->flags&C_FLAG_MUTE) && p->s->instrumenti[(r.inst != INST_VOID) ? r.inst : cv->r.inst])
	{
		instrument *iv = p->s->instrumentv[p->s->instrumenti[(r.inst != INST_VOID) ? r.inst : cv->r.inst]];
		if (iv->flags&S_FLAG_MIDI) midiPC(fptr, iv->midichannel, m%128);
	} return 1;
}
char midiccc(jack_nframes_t fptr, int m, channel *cv, row r)
{
	cv->midicc = m%128;
	if (cv->midiccindex != -1 && !(cv->flags&C_FLAG_MUTE) && p->s->instrumenti[(r.inst != INST_VOID) ? r.inst : cv->r.inst])
	{
		instrument *iv = p->s->instrumentv[p->s->instrumenti[(r.inst != INST_VOID) ? r.inst : cv->r.inst]];
		if (iv->flags&S_FLAG_MIDI) midiCC(fptr, iv->midichannel, cv->midiccindex, cv->midicc);
	} return 1;
}
char smoothmidiccc(jack_nframes_t fptr, int m, channel *cv, row r)
{ cv->targetmidicc = m%128; return 1; }

void preprocessRow(jack_nframes_t fptr, char midi, channel *cv, row r)
{
	char ret;
	uint8_t oldnote = NOTE_UNUSED;

	for (int i = 0; i <= cv->macroc; i++)
		cv->r.macro[i] = r.macro[i];

	/* end interpolation */
	if (cv->targetgain != -1)
	{
		if (cv->flags&C_FLAG_TARGET_RAND)
		{
			cv->randgain = cv->targetgain;
			cv->flags ^= C_FLAG_TARGET_RAND;
		} else cv->gain = cv->randgain = cv->targetgain;
		cv->targetgain = -1;
	}
	if (cv->targetfiltermode != -1) { cv->filtermode = cv->targetfiltermode; cv->targetfiltermode = -1; }
	if (cv->targetfiltercut != -1) { cv->filtercut = cv->targetfiltercut; cv->targetfiltercut = -1; }
	if (cv->targetfilterres != -1) { cv->filterres = cv->targetfilterres; cv->targetfilterres = -1; }
	if (cv->targetwaveshaperstrength != -1) { cv->waveshaperstrength = cv->targetwaveshaperstrength; cv->targetwaveshaperstrength = -1; }
	if (cv->targetmidicc != -1) { cv->midicc = cv->targetmidicc; cv->targetmidicc = -1; }
	if (cv->targetsendgain != -1) { cv->sendgain = cv->targetsendgain; cv->targetsendgain = -1; }
	if (cv->rtrigsamples)
	{
		if (cv->rtrigblocksize > 0 || cv->rtrigblocksize == -1) cv->rtrigblocksize--;
		else
		{
			if (cv->flags&C_FLAG_RTRIG_REV) cv->flags ^= C_FLAG_RTRIG_REV;
			cv->rtrigsamples = 0;
		}
	}

	ret = ifMacro(fptr, cv, r, 'V', &Vc);
	if (!ret) cv->vibratosamples = 0;

	if (ifMacro(fptr, cv, r, '%', &PERCENTc)) return;

	ifMacro(fptr, cv, r, 'b', &Bc); /* bpm */

	if (cv->pointer
			&& (ifMacro(fptr, cv, r, 'G', &DUMMY)
			||  ifMacro(fptr, cv, r, 'I', &DUMMY)
			||  ifMacro(fptr, cv, r, 'F', &DUMMY)
			||  ifMacro(fptr, cv, r, 'Z', &DUMMY)
			||  ifMacro(fptr, cv, r, 'O', &OcPRERAMP)
			||  ifMacro(fptr, cv, r, 'o', &OcPRERAMP)
			||  ifMacro(fptr, cv, r, 'U', &OcPRERAMP)
			||  ifMacro(fptr, cv, r, 'u', &OcPRERAMP)))
		ramp(cv, p->s->instrumenti[cv->samplerinst], cv->pointer, cv->pitchedpointer);

	ifMacro(fptr, cv, r, 'G', &GcPOSTRAMP); /* gain      */
	ifMacro(fptr, cv, r, 'I', &IcPOSTRAMP); /* rand gain */
	ifMacro(fptr, cv, r, 'F', &FcPOSTRAMP); /* cutoff    */
	ifMacro(fptr, cv, r, 'Z', &ZcPOSTRAMP); /* resonance */
	ifMacro(fptr, cv, r, 'g', &gc); /* smooth gain       */
	ifMacro(fptr, cv, r, 'i', &ic); /* smooth rand gain  */
	ifMacro(fptr, cv, r, 'f', &fc); /* smooth cutoff     */
	ifMacro(fptr, cv, r, 'z', &zc); /* smooth resonance  */

	ifMacro(fptr, cv, r, 'S', &Sc); /* send        */
	ifMacro(fptr, cv, r, 's', &sc); /* smooth send */

	ret = ifMacro(fptr, cv, r, 'C', &Cc); /* cut */
	if (!ret && r.note != NOTE_VOID) { ret = ifMacro(fptr, cv, r, 'P', &Pc); /* portamento */
		if (!ret)                    { ret = ifMacro(fptr, cv, r, 'D', &Dc); /* delay      */
			if (!ret)
			{
				oldnote = cv->r.note;
				if (r.inst == INST_VOID)
				{
					ramp(cv, p->s->instrumenti[cv->samplerinst], cv->pointer, cv->pitchedpointer);
					triggerNote(cv, r.note, cv->r.inst);
				} else
				{
					ramp(cv, p->s->instrumenti[cv->r.inst], cv->pointer, cv->pitchedpointer);
					triggerNote(cv, r.note, r.inst);
				}
			}
		}
	}

	ifMacro(fptr, cv, r, 'O', &OcPOSTRAMP); /* offset         */
	ifMacro(fptr, cv, r, 'o', &ocPOSTRAMP); /* bw offset      */
	ifMacro(fptr, cv, r, 'U', &UcPOSTRAMP); /* rand offset    */
	ifMacro(fptr, cv, r, 'u', &ucPOSTRAMP); /* rand bw offset */

	/* midi */
	if (oldnote != NOTE_UNUSED && midi) /* trigger midi (needs to be after gain calculation) */
	{
		if (r.inst == INST_VOID) ret = triggerMidi(fptr, cv, oldnote, r.note, cv->r.inst);
		else                     ret = triggerMidi(fptr, cv, oldnote, r.note, r.inst);
	}
	ifMacro(fptr, cv, r, ';', &midicctargetc);
	ifMacro(fptr, cv, r, '@', &midipcc);
	ifMacro(fptr, cv, r, '.', &midiccc);
	ifMacro(fptr, cv, r, ',', &smoothmidiccc);

	ifMacro(fptr, cv, r, 'M', &Mc);             /* microtonal offset  */

	/* retrigger macros (all *4* of them) */
	if (       !ifMacro(fptr, cv, r, 'q', &qc)  /* bw retrigger       */
	        && !ifMacro(fptr, cv, r, 'Q', &Qc)  /* retrigger          */
	        && !ifMacro(fptr, cv, r, 'r', &rc)) /* bw block retrigger */
		ifMacro(fptr, cv, r, 'R', &Rc);         /* block retrigger    */
	if (cv->rtrigsamples && cv->rtrigblocksize < -1)
	{ /* clean up if the last row had a [Q,q]xx and this row doesn't */
		if (cv->flags&C_FLAG_RTRIG_REV) cv->flags ^= C_FLAG_RTRIG_REV;
		cv->rtrigsamples = 0;
	}

	ifMacro(fptr, cv, r, 'W', &Wc);             /* waveshaper         */
	ifMacro(fptr, cv, r, 'w', &wc);             /* smooth waveshaper  */
	ifMacro(fptr, cv, r, 'E', &Ec);             /* local envelope     */
	ifMacro(fptr, cv, r, 'H', &Hc);             /* local pitch shift  */
	ifMacro(fptr, cv, r, 'L', &Lc);             /* local cyclelength  */
}

void postSampler(jack_nframes_t fptr, int outputgroup, channel *cv, float rp,
		float lf, float rf,
		SVFilter fl[2], SVFilter fr[2],
		uint8_t gain, short targetgain)
{
	/* waveshapers */
	if (cv->targetwaveshaperstrength != -1 || cv->waveshaperstrength)
	{
		float strength, hold;
		switch (cv->waveshaper)
		{
			case 0: /* hard clip */
				if (cv->targetwaveshaperstrength != -1)
					 strength = 1.0f + (cv->waveshaperstrength<<1) + ((cv->targetwaveshaperstrength - cv->waveshaperstrength)<<1) * rp;
				else strength = 1.0f + (cv->waveshaperstrength<<1);
				hold = 1.0f / sqrtf(strength);
				lf = hardclip(lf*strength)*hold;
				rf = hardclip(rf*strength)*hold;
				break;
			case 1: /* soft clip */
				if (cv->targetwaveshaperstrength != -1)
					 strength = 1.0f + cv->waveshaperstrength + (cv->targetwaveshaperstrength - cv->waveshaperstrength) * rp;
				else strength = 1.0f + cv->waveshaperstrength;
				hold = 1.0f / sqrtf(strength);
				lf = thirddegreepolynomial(lf*strength)*hold;
				rf = thirddegreepolynomial(rf*strength)*hold;
				break;
			case 2: /* rectify */
				if (cv->targetwaveshaperstrength != -1)
					 strength = (cv->waveshaperstrength + (cv->targetwaveshaperstrength - cv->waveshaperstrength) * rp)*DIV15;
				else strength = cv->waveshaperstrength*DIV15;
				lf = rectify(lf) * (MIN(strength, 0.5f) * 2) + lf * (1.0f - (MAX(strength, 0.5f) - 0.5f) * 2);
				rf = rectify(rf) * (MIN(strength, 0.5f) * 2) + rf * (1.0f - (MAX(strength, 0.5f) - 0.5f) * 2);
				break;
			case 3: /* rectify x2 */
				if (cv->targetwaveshaperstrength != -1)
					 strength = (cv->waveshaperstrength + (cv->targetwaveshaperstrength - cv->waveshaperstrength) * rp)*DIV15;
				else strength = cv->waveshaperstrength*DIV15;
				lf = rectify(lf) * (MIN(strength, 0.5f) * 2) + lf * (1.0f - (MAX(strength, 0.5f) - 0.5f) * 2);
				rf = rectify(rf) * (MIN(strength, 0.5f) * 2) + rf * (1.0f - (MAX(strength, 0.5f) - 0.5f) * 2);
				lf = rectify(lf) * (MIN(strength, 0.5f) * 2) + lf * (1.0f - (MAX(strength, 0.5f) - 0.5f) * 2);
				rf = rectify(rf) * (MIN(strength, 0.5f) * 2) + rf * (1.0f - (MAX(strength, 0.5f) - 0.5f) * 2);
				break;
			case 4: /* wavefold */
				if (cv->targetwaveshaperstrength != -1)
					 strength = 1.0f + cv->waveshaperstrength + (cv->targetwaveshaperstrength - cv->waveshaperstrength) * rp;
				else strength = 1.0f + cv->waveshaperstrength;
				hold = 1.0f / sqrtf(strength);
				lf = wavefolder(lf*strength)*hold;
				rf = wavefolder(rf*strength)*hold;
				break;
			case 5: /* wavewrap */
				if (cv->targetwaveshaperstrength != -1)
					 strength = 1.0f + cv->waveshaperstrength + (cv->targetwaveshaperstrength - cv->waveshaperstrength) * rp;
				else strength = 1.0f + cv->waveshaperstrength;
				hold = 1.0f / sqrtf(strength);
				lf = wavewrapper(lf*strength)*hold;
				rf = wavewrapper(rf*strength)*hold;
				break;
			case 6: /* sign conversion */
				if (cv->targetwaveshaperstrength != -1)
					 strength = (cv->waveshaperstrength + (cv->targetwaveshaperstrength - cv->waveshaperstrength) * rp)*DIV15;
				else strength = cv->waveshaperstrength*DIV15;
				lf = signedunsigned(lf) * (MIN(strength, 0.5f) * 2) + lf * (1.0f - (MAX(strength, 0.5f) - 0.5f) * 2);
				rf = signedunsigned(rf) * (MIN(strength, 0.5f) * 2) + rf * (1.0f - (MAX(strength, 0.5f) - 0.5f) * 2);
				break;
			case 7: /* hard gate */
				if (cv->targetwaveshaperstrength != -1)
					 strength = (cv->waveshaperstrength + (cv->targetwaveshaperstrength - cv->waveshaperstrength) * rp)*DIV64;
				else strength = cv->waveshaperstrength*DIV64;
				if (fabsf(lf) < strength) lf = 0.0f;
				if (fabsf(rf) < strength) rf = 0.0f;
				break;
		}
	}

	/* filter */
	float cutoff = cv->filtercut*DIV255;
	float resonance = cv->filterres*DIV15;
	if (cv->targetfiltercut != -1) cutoff += (cv->targetfiltercut - (short)cv->filtercut)*DIV255 * rp;
	if (cv->targetfilterres != -1) resonance += (cv->targetfilterres - cv->filterres)*DIV15 * rp;

	/* first pass (12dB/oct) */
	runSVFilter(&fl[0], lf, cutoff, resonance);
	runSVFilter(&fr[0], rf, cutoff, resonance);
	switch (cv->filtermode%4)
	{
		case 0: /* low-pass  */ if (!(cv->targetfiltercut == -1 && cv->filtercut == 255)) { lf = fl[0].l; rf = fr[0].l; } break;
		case 1: /* high-pass */ if (!(cv->targetfiltercut == -1 && cv->filtercut == 0))   { lf = fl[0].h; rf = fr[0].h; } break;
		case 2: /* band-pass */ lf = fl[0].b; rf = fr[0].b; break;
		case 3: /* notch     */ lf = fl[0].n; rf = fr[0].n; break;
	}
	if (cv->targetfiltermode != -1)
		switch (cv->targetfiltermode%4)
		{
			case 0: /* low-pass  */ if (!(cv->targetfiltercut == -1 && cv->filtercut == 255)) { lf += (fl[0].l - lf) * rp; rf += (fr[0].l - rf) * rp; } break;
			case 1: /* high-pass */ if (!(cv->targetfiltercut == -1 && cv->filtercut == 0))   { lf += (fl[0].h - lf) * rp; rf += (fr[0].h - rf) * rp; } break;
			case 2: /* band-pass */ lf += (fl[0].b - lf) * rp; rf += (fr[0].b - rf) * rp; break;
			case 3: /* notch     */ lf += (fl[0].n - lf) * rp; rf += (fr[0].n - rf) * rp; break;
		}
	/* second pass (24dB/oct) */
	runSVFilter(&fl[1], lf, cutoff, resonance);
	runSVFilter(&fr[1], rf, cutoff, resonance);
	switch (cv->filtermode+4)
	{
		case 0: /* low-pass  */ if (!(cv->targetfiltercut == -1 && cv->filtercut == 255)) { lf = fl[1].l; rf = fr[1].l; } break;
		case 1: /* high-pass */ if (!(cv->targetfiltercut == -1 && cv->filtercut == 0))   { lf = fl[1].h; rf = fr[1].h; } break;
		case 2: /* band-pass */ lf = fl[1].b; rf = fr[1].b; break;
		case 3: /* notch     */ lf = fl[1].n; rf = fr[1].n; break;
	}
	if (cv->targetfiltermode != -1)
		switch (cv->targetfiltermode+4)
		{
			case 0: /* low-pass  */ if (!(cv->targetfiltercut == -1 && cv->filtercut == 255)) { lf += (fl[1].l - lf) * rp; rf += (fr[1].l - rf) * rp; } break;
			case 1: /* high-pass */ if (!(cv->targetfiltercut == -1 && cv->filtercut == 0))   { lf += (fl[1].h - lf) * rp; rf += (fr[1].h - rf) * rp; } break;
			case 2: /* band-pass */ lf += (fl[1].b - lf) * rp; rf += (fr[1].b - rf) * rp; break;
			case 3: /* notch     */ lf += (fl[1].n - lf) * rp; rf += (fr[1].n - rf) * rp; break;
		}

	lf = hardclip(lf); rf = hardclip(rf);

	/* apply gain */
	if (targetgain != -1)
	{
		lf *= (gain>>4)*DIV16 + ((targetgain>>4) - (gain>>4))*DIV16 * rp;
		rf *= (gain%16)*DIV16 + ((targetgain%16) - (gain%16))*DIV16 * rp;
	} else
	{
		lf *= (gain>>4)*DIV16;
		rf *= (gain%16)*DIV16;
	}

	if (!(cv->flags&C_FLAG_MUTE))
	{
		pb.out[outputgroup].l[fptr] += lf;
		pb.out[outputgroup].r[fptr] += rf;
		if (cv->targetsendgain != -1)
		{
			pb.out[cv->sendgroup].l[fptr] += lf * cv->sendgain*DIV15 + (cv->targetsendgain - cv->sendgain)*DIV15 * rp;
			pb.out[cv->sendgroup].r[fptr] += rf * cv->sendgain*DIV15 + (cv->targetsendgain - cv->sendgain)*DIV15 * rp;
		} else if (cv->sendgain)
		{
			pb.out[cv->sendgroup].l[fptr] += lf * cv->sendgain*DIV15;
			pb.out[cv->sendgroup].r[fptr] += rf * cv->sendgain*DIV15;
		}
	}
}

void playChannelLookback(jack_nframes_t fptr, channel *cv)
{
	instrument *iv = p->s->instrumentv[p->s->instrumenti[cv->samplerinst]];

	float multiplier;
	uint32_t delta;

	uint16_t sprs = 0;
	uint16_t sprp;

	if (cv->delaysamples > cv->cutsamples)
	{ /* delay is set and takes priority over cut */
		sprs = cv->delaysamples;

		triggerNote(cv, cv->delaynote, cv->delayinst);
		cv->delaysamples = 0;

		ifMacro(fptr, cv, cv->r, 'O', &OcPOSTRAMP); /* offset         */
		ifMacro(fptr, cv, cv->r, 'o', &ocPOSTRAMP); /* bw offset      */
		ifMacro(fptr, cv, cv->r, 'U', &UcPOSTRAMP); /* rand offset    */
		ifMacro(fptr, cv, cv->r, 'u', &ucPOSTRAMP); /* rand bw offset */
		ifMacro(fptr, cv, cv->r, 'M', &Mc); /* microtonal offset */
		ifMacro(fptr, cv, cv->r, 'E', &Ec); /* local envelope    */
		cv->envgain = 0; /* TODO: necessary? */ /* not perfect, but a good enough assumption */
	} else if (cv->cutsamples)
	{ /* cut is set and takes priority over delay */
		triggerNote(cv, NOTE_OFF, cv->r.inst);
		cv->cutsamples = 0;
		cv->envgain = 0; /* not perfect, but a good enough assumption */
		return;
	}

	for (sprp = sprs; sprp < p->s->spr; sprp++)
	{
		cv->finetune = cv->microtonalfinetune;
		if (cv->portamentosamplepointer == cv->portamentosamples)
		{
			cv->portamentofinetune = cv->targetportamentofinetune;
			cv->finetune += cv->portamentofinetune;
			cv->portamentosamplepointer++;
		} else if (cv->portamentosamplepointer < cv->portamentosamples)
		{
			cv->portamentofinetune = cv->startportamentofinetune +
				(cv->targetportamentofinetune - cv->startportamentofinetune) * (float)cv->portamentosamplepointer/(float)cv->portamentosamples;
			cv->finetune += cv->portamentofinetune;

			multiplier = powf(M_12_ROOT_2, (short)cv->samplernote - NOTE_C5 + cv->finetune);

			if (cv->flags&C_FLAG_REVERSE)
			{
				delta = (int)((cv->portamentosamplepointer+1)*multiplier) - (int)(cv->portamentosamplepointer*multiplier);
				if (cv->pitchedpointer > delta) cv->pitchedpointer -= delta;
			} else
				cv->pitchedpointer += (int)((cv->portamentosamplepointer+1)*multiplier) - (int)(cv->portamentosamplepointer*multiplier);

			cv->portamentosamplepointer++;
		} else cv->finetune += cv->portamentofinetune;
	}

	/* process the sampler */
	if (iv && cv->samplernote != NOTE_VOID && cv->samplernote != NOTE_OFF && cv->samplerinst != INST_VOID
			&& !(p->w->instrumentlockv == INST_GLOBAL_LOCK_FREE
			&& p->w->instrumentlocki == p->s->instrumenti[cv->samplerinst]))
	{
		for (sprp = 0; sprp < p->s->spr - sprs; sprp++)
			if (!(envelope(iv, cv, cv->pointer+sprp) && cv->pitchedpointer < iv->length))
				cv->samplernote = NOTE_OFF;

		if (cv->portamentosamplepointer >= cv->portamentosamples)
		{ /* only walk pitchedpointer if not pitch sliding */
			multiplier = powf(M_12_ROOT_2, (short)cv->samplernote - NOTE_C5 + cv->finetune);
			if (cv->flags&C_FLAG_REVERSE)
			{
				delta = (int)((cv->pointer+(sprp - sprs))*multiplier) - (int)(cv->pointer*multiplier);
				if (cv->pitchedpointer > delta) cv->pitchedpointer -= delta;
				else                            cv->pitchedpointer = 0;
			} else
				cv->pitchedpointer += (int)((cv->pointer+(sprp - sprs))*multiplier) - (int)(cv->pointer*multiplier);
		} cv->pointer += sprp - sprs;
	}
}
void playChannel(jack_nframes_t fptr, uint16_t sprp, channel *cv)
{
	instrument *iv = p->s->instrumentv[p->s->instrumenti[cv->samplerinst]];
	short li = 0; short ri = 0; /* gcc REALLY wants these to be static, thank you stallman very cool */

	float multiplier;
	uint32_t delta;

	if (cv->cutsamples && sprp > cv->cutsamples)
	{
		ramp(cv, p->s->instrumenti[cv->samplerinst], cv->pointer, cv->pitchedpointer);
		triggerMidi(fptr, cv, cv->r.note, NOTE_OFF, cv->r.inst);
		triggerNote(cv, NOTE_OFF, cv->r.inst);
		cv->cutsamples = 0;
	}
	if (cv->delaysamples && sprp > cv->delaysamples)
	{
		ramp(cv, p->s->instrumenti[cv->delayinst], cv->pointer, cv->pitchedpointer);
		triggerMidi(fptr, cv, cv->r.note, cv->delaynote, cv->delayinst);
		triggerNote(cv, cv->delaynote, cv->delayinst);
		cv->delaysamples = 0;

		if (cv->pointer
				&& (ifMacro(fptr, cv, cv->r, 'O', &OcPRERAMP)
				||  ifMacro(fptr, cv, cv->r, 'o', &OcPRERAMP)
				||  ifMacro(fptr, cv, cv->r, 'U', &OcPRERAMP)
				||  ifMacro(fptr, cv, cv->r, 'u', &OcPRERAMP)))
			ramp(cv, p->s->instrumenti[cv->samplerinst], cv->pointer, cv->pitchedpointer);

		ifMacro(fptr, cv, cv->r, 'O', &OcPOSTRAMP); /* offset         */
		ifMacro(fptr, cv, cv->r, 'o', &ocPOSTRAMP); /* bw offset      */
		ifMacro(fptr, cv, cv->r, 'U', &UcPOSTRAMP); /* rand offset    */
		ifMacro(fptr, cv, cv->r, 'u', &ucPOSTRAMP); /* rand bw offset */
		ifMacro(fptr, cv, cv->r, 'M', &Mc); /* microtonal offset      */
		ifMacro(fptr, cv, cv->r, 'E', &Ec); /* local envelope         */
	}

	cv->finetune = cv->microtonalfinetune;
	if (cv->portamentosamplepointer == cv->portamentosamples)
	{
		cv->portamentofinetune = cv->targetportamentofinetune;
		cv->portamentosamplepointer++;
		cv->samplernote += cv->portamentofinetune;
	} else if (cv->portamentosamplepointer < cv->portamentosamples)
	{
		cv->portamentofinetune = cv->startportamentofinetune +
			(cv->targetportamentofinetune - cv->startportamentofinetune) * (float)cv->portamentosamplepointer/(float)cv->portamentosamples;

		multiplier = powf(M_12_ROOT_2, (short)cv->samplernote - NOTE_C5 + cv->finetune + cv->portamentofinetune);

		if (cv->flags&C_FLAG_REVERSE)
		{
			delta = (int)((cv->portamentosamplepointer+1)*multiplier) - (int)(cv->portamentosamplepointer*multiplier);
			if (cv->pitchedpointer > delta) cv->pitchedpointer -= delta;
			else                            cv->pitchedpointer = 0;
		} else
			cv->pitchedpointer += (int)((cv->portamentosamplepointer+1)*multiplier) - (int)(cv->portamentosamplepointer*multiplier);

		cv->portamentosamplepointer++;
	}

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

	if (iv && !(cv->flags&C_FLAG_MUTE) && !(sprp%PITCH_WHEEL_SAMPLES) && iv->flags&S_FLAG_MIDI)
	{
		if (cv->finetune != 0.0f) midiPitchWheel(fptr, iv->midichannel, MIN(2.0f, MAX(-2.0f, cv->finetune + cv->portamentofinetune)));
		if (cv->targetmidicc != -1) midiCC(fptr, iv->midichannel, cv->midiccindex, cv->midicc + (cv->targetmidicc - cv->midicc) * (float)sprp/(float)p->s->spr);
	}

	/* process the sampler */
	if (iv && cv->samplernote != NOTE_VOID && cv->samplernote != NOTE_OFF && cv->samplerinst != INST_VOID
			&& !(p->w->instrumentlockv == INST_GLOBAL_LOCK_FREE
			&& p->w->instrumentlocki == p->s->instrumenti[cv->samplerinst]))
	{
		if (cv->rtrigsamples)
		{
			uint32_t rtrigoffset = (cv->pointer - cv->rtrigpointer) % cv->rtrigsamples;
			if (!rtrigoffset)
			{ /* first sample of any retrigger */
				if (iv->flags&S_FLAG_MIDI)
				{
					midiNoteOff(fptr, iv->midichannel, cv->r.note, (cv->randgain>>4)<<3);
					midiNoteOn(fptr, iv->midichannel, cv->r.note, (cv->randgain>>4)<<3);
				}
				if (cv->pointer > cv->rtrigpointer) /* first sample of any retrigger but the first */
					cv->rtrigcurrentpitchedpointer = cv->pitchedpointer;
			}
			if (envelope(iv, cv, cv->pointer))
			{
				if (cv->flags&C_FLAG_RTRIG_REV) samplerProcess(iv, cv, cv->pointer, cv->rtrigpitchedpointer - (cv->pitchedpointer - cv->rtrigcurrentpitchedpointer), &li, &ri);
				else                            samplerProcess(iv, cv, cv->pointer, cv->rtrigpitchedpointer + (cv->pitchedpointer - cv->rtrigcurrentpitchedpointer), &li, &ri);
			}
		} else { if (envelope(iv, cv, cv->pointer)) samplerProcess(iv, cv, cv->pointer, cv->pitchedpointer, &li, &ri); }

		if (cv->portamentosamplepointer >= cv->portamentosamples)
		{ /* only walk pitchedpointer if not pitch sliding */
			multiplier = powf(M_12_ROOT_2, (short)cv->samplernote - NOTE_C5 + cv->finetune);
			if (cv->flags&C_FLAG_REVERSE)
			{
				delta = (int)((cv->pointer+1)*multiplier) - (int)(cv->pointer*multiplier);
				if (cv->pitchedpointer > delta) cv->pitchedpointer -= delta;
			} else
				cv->pitchedpointer += (int)((cv->pointer+1)*multiplier) - (int)(cv->pointer*multiplier);
		} cv->pointer++;
	}

	float lf = (float)li*DIVSHRT;
	float rf = (float)ri*DIVSHRT;

	float rowprogress = (float)sprp / (float)p->s->spr;
	if (cv->rampbuffer && cv->rampindex < rampmax)
	{ /* ramping */
		float gain = (float)cv->rampindex / (float)rampmax;

		if (iv)
			postSampler(fptr, iv->outputgroup, cv, rowprogress,
					hardclip(lf * iv->gain*DIV32) * gain, hardclip(rf * iv->gain*DIV32) * gain,
					cv->fl, cv->fr, cv->randgain, cv->targetgain);

		if (p->s->instrumentv[cv->rampinst])
			postSampler(fptr, p->s->instrumentv[cv->rampinst]->outputgroup, cv, rowprogress,
					hardclip(((float)cv->rampbuffer[cv->rampindex*2 + 0]*DIVSHRT) * p->s->instrumentv[cv->rampinst]->gain*DIV32) * (1.0f - gain),
					hardclip(((float)cv->rampbuffer[cv->rampindex*2 + 1]*DIVSHRT) * p->s->instrumentv[cv->rampinst]->gain*DIV32) * (1.0f - gain),
					cv->rampfl, cv->rampfr, cv->rampgain, -1);

		cv->rampindex++;
	} else if (iv)
		postSampler(fptr, iv->outputgroup, cv, rowprogress,
				hardclip(lf * iv->gain*DIV32), hardclip(rf * iv->gain*DIV32),
				cv->fl, cv->fr, cv->randgain, cv->targetgain);
}

void lookback(jack_nframes_t fptr)
{
	pattern *pt;

	/* non-volatile state */
	/* start at the beginning of the current playlist block */
	uint8_t blockstart = 0;
	for (uint8_t i = p->s->songp; i >= 0; i--)
		if (p->s->songi[i] == PATTERN_VOID)
		{
			blockstart = i + 1;
			break;
		}

	/* for each pattern in the block */
	for (uint8_t b = blockstart; b < p->s->songp; b++)
	{
		pt = p->s->patternv[p->s->patterni[p->s->songi[b]]];
		/* for each row */
		for (uint8_t r = 0; r <= pt->rowc; r++)
			for (uint8_t c = 0; c < p->s->channelc; c++)
			{
				preprocessRow(fptr, 0, &p->s->channelv[c], pt->rowv[c][r%pt->rowcc[c]+1]);
				playChannelLookback(fptr, &p->s->channelv[c]);
			}
	}
}

void clearChannel(channel *cv)
{
	cv->r.note = cv->samplernote = NOTE_VOID;
	cv->r.inst = cv->samplerinst = INST_VOID;
	cv->rtrigsamples = 0;
	if (cv->flags&C_FLAG_RTRIG_REV) cv->flags ^= C_FLAG_RTRIG_REV;
	cv->waveshaperstrength = 0; cv->targetwaveshaperstrength = -1;
	cv->gain = cv->randgain = 0x88; cv->targetgain = -1;
	if (cv->flags&C_FLAG_TARGET_RAND) cv->flags ^= C_FLAG_TARGET_RAND;
	cv->filtermode = 0; cv->targetfiltermode = -1;
	cv->filtercut = 255; cv->targetfiltercut = -1;
	cv->filterres = 0; cv->targetfilterres = -1;
	cv->midiccindex = -1; cv->midicc = 0; cv->targetmidicc = -1;
	cv->sendgroup = 0; cv->sendgain = 0; cv->targetsendgain = -1;
}

int process(jack_nframes_t nfptr, void *arg)
{
	playbackinfo *p = arg;
	channel *cv; instrument *iv;

	pb.in.l = jack_port_get_buffer(p->in.l, nfptr);
	pb.in.r = jack_port_get_buffer(p->in.r, nfptr);
	for (short i = 0; i < OUTPUT_GROUPS; i++)
	{
		pb.out[i].l = jack_port_get_buffer(p->out[i].l, nfptr); memset(pb.out[i].l, 0, nfptr * sizeof(sample_t));
		pb.out[i].r = jack_port_get_buffer(p->out[i].r, nfptr); memset(pb.out[i].r, 0, nfptr * sizeof(sample_t));
	} pb.midiout = jack_port_get_buffer(p->midiout, nfptr); jack_midi_clear_buffer(pb.midiout);

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
				midiNoteOff(0, iv->midichannel, cv->r.note, (cv->randgain>>4)<<3);
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
			midiNoteOff(0, iv->midichannel, cv->r.note, (cv->randgain>>4)<<3);
			cv->r.note = NOTE_VOID;
		} p->w->instrumentlockv = INST_GLOBAL_LOCK_OK;
	}

	row holdr;
	uint32_t holdptr, holdpitchedptr, holdflags;
	uint8_t holdnote, holdinst;
	if (p->w->previewtrigger)
		switch (p->w->previewtrigger)
		{
			case 1: // start instrument preview
				p->w->previewtrigger++;
				if (!p->s->playing)
				{
					holdr = p->w->previewchannel.r;
					holdflags = p->w->previewchannel.flags;
					holdptr = p->w->previewchannel.pointer;
					holdpitchedptr = p->w->previewchannel.pitchedpointer;
					holdnote = p->w->previewchannel.samplernote;
					holdinst = p->w->previewchannel.samplerinst;
					// memset(&p->w->previewchannel, 0, sizeof(channel));
					clearChannel(&p->w->previewchannel);
					p->w->previewchannel.r = holdr; /* don't clear out cv->r, for midi */
					p->w->previewchannel.flags = holdflags;
					p->w->previewchannel.samplernote = holdnote;
					p->w->previewchannel.samplerinst = holdinst;
					p->w->previewchannel.pointer = holdptr;
					p->w->previewchannel.pitchedpointer = holdpitchedptr;
					p->w->previewchannel.macroc = 0;

					preprocessRow(0, 1, &p->w->previewchannel, p->w->previewrow);
					// p->w->previewchannel.r = p->w->previewrow; /* TODO: sus, not needed for other channels */
				} break;
		}

	/* start/stop playback */
	if (p->s->playing == PLAYING_START)
	{
		changeBpm(p->s, p->s->songbpm);
		p->s->sprp = 0;

		/* stop preview */
		p->w->previewchannel.r.note = p->w->previewchannel.samplernote = NOTE_VOID;
		p->w->previewchannel.r.inst = p->w->previewchannel.samplerinst = INST_VOID;
		/* TODO: also stop the sampler's follower note */
		/* clear the channels */
		for (uint8_t c = 0; c < p->s->channelc; c++)
		{
			cv = &p->s->channelv[c];

			if (!(cv->flags&C_FLAG_MUTE) && p->s->instrumenti[cv->r.inst])
			{
				iv = p->s->instrumentv[p->s->instrumenti[cv->r.inst]];
				if (iv->flags&S_FLAG_MIDI) midiNoteOff(0, iv->midichannel, cv->r.note, (cv->randgain>>4)<<3);
			}

			clearChannel(cv);
		}

		lookback(0);

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
			ramp(cv, p->s->instrumenti[cv->samplerinst], cv->pointer, cv->pitchedpointer);
			triggerMidi(0, cv, cv->r.note, NOTE_OFF, cv->r.inst);
			triggerNote(cv, NOTE_OFF, cv->r.inst);
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
			playChannel(fptr, p->s->sprp, &p->s->channelv[c]);

		/* play the preview */
		if (p->w->previewchannel.samplernote != NOTE_VOID
				&& p->w->previewchannel.samplerinst != INST_VOID)
			playChannel(fptr, p->s->sprp, &p->w->previewchannel);

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
					else if (p->s->songi[p->s->songp + 1] == PATTERN_VOID)
					{ /* no next pattern, go to the beginning of the block */
						uint8_t blockstart = 0;
						for (uint8_t i = p->s->songp; i >= 0; i--)
							if (p->s->songi[i] == PATTERN_VOID)
							{
								blockstart = i + 1;
								break;
							}
						p->s->songp = blockstart;
					} else p->s->songp++;
				}

				if (w->flags&0b1)
				{
					p->w->songfy = p->s->songp;
					p->w->trackerfy = p->s->songr;
				} p->dirty = 1;
			}
			// lookback(fptr);
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

	if (ENABLE_BACKGROUND) updateBackground(nfptr, pb.out[0]);
	for (int i = 1; i < p->s->instrumentc; i++)
		if (p->s->instrumentv[i]->triggerflash)
		{
			if (p->s->instrumentv[i]->triggerflash == 1) p->dirty = 1;
			p->s->instrumentv[i]->triggerflash--;
		}

	return 0;
}
