char ifMacro(jack_nframes_t fptr, Channel *cv, Row r, char m, char (*callback)(jack_nframes_t, int, Channel *, Row))
{
	char ret = 0;
	for (int i = 0; i <= cv->data.macroc; i++)
		if (r.macro[i].c == m)
			ret = callback(fptr, r.macro[i].v, cv, r);
	return ret;
}

void changeBpm(Song *cs, uint8_t newbpm)
{
	cs->bpm = newbpm;
	cs->spr = samplerate * (60.0 / newbpm) / cs->rowhighlight;
}

/* freewheel to fill up the ramp buffer */
void ramp(Channel *cv, uint8_t realinstrument)
{
	if (cv->rampbuffer)
	{
		/* clear the rampbuffer properly so cruft isn't played in edge cases */
		memset(cv->rampbuffer, 0, sizeof(short) * rampmax * 2);

		/* save state */
		cv->rampinst = realinstrument;

		uint32_t pointeroffset = cv->pointer;
		uint32_t pitchedpointeroffset = cv->pitchedpointer;
		if (realinstrument < p->s->instrumentc)
		{
			float multiplier = powf(M_12_ROOT_2, (short)cv->samplernote - NOTE_C5 + cv->finetune);
			pitchedpointeroffset += (int)((pointeroffset+1)*multiplier) - (int)(pointeroffset*multiplier);
			float oldenvgain = cv->envgain;
			if (cv->data.reverse)
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
					samplerProcess(realinstrument, cv, pointeroffset+i, pitchedpointeroffset,
							&cv->rampbuffer[i*2 + 0], &cv->rampbuffer[i*2 + 1]);
				}
			} else
				for (uint16_t i = 0; i < rampmax; i++)
				{
					pitchedpointeroffset += (int)((pointeroffset+i+1)*multiplier) - (int)((pointeroffset+i)*multiplier);
					samplerProcess(realinstrument, cv, pointeroffset+i, pitchedpointeroffset,
							&cv->rampbuffer[i*2 + 0], &cv->rampbuffer[i*2 + 1]);
				}
			cv->envgain = oldenvgain;
		}
	} cv->rampindex = 0;
}

void triggerNote(Channel *cv, uint8_t note, uint8_t inst)
{
	if (note == NOTE_VOID) return;
	if (note == NOTE_OFF)
	{
		cv->data.release = 1;
		cv->r.inst = inst;
		cv->r.note = note;
	} else
	{
		cv->r.inst = cv->samplerinst = inst;
		cv->r.note = cv->samplernote = note;
		cv->pointer = cv->pitchedpointer = 0;
		cv->data.reverse = 0;
		cv->data.release = 0;
		cv->portamentosamples = 0; cv->portamentosamplepointer = 1;
		cv->startportamentofinetune = cv->targetportamentofinetune = cv->portamentofinetune = 0.0f;
		cv->microtonalfinetune = 0.0f;
		cv->vibrato = 0;
		cv->localenvelope = -1;
		cv->localpitchshift = -1;
		cv->localcyclelength = -1;
		
		/* must stop retriggers cos pointers are no longer guaranteed to be valid */
		cv->rtrigblocksize = 0;
		cv->data.rtrig_rev = 0;
		cv->rtrigsamples = 0;

		if (!cv->data.mute && instrumentSafe(p->s, inst))
		{
			p->s->instrumentv[p->s->instrumenti[inst]].triggerflash = samplerate / buffersize *DIV1000 * INSTRUMENT_TRIGGER_FLASH_MS;
			p->dirty = 1;
		}
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
{ jack_midi_data_t event[2] = {0b11000000 | midichannel, program}; jack_midi_event_write(pb.midiout, fptr, event, 2); }
void midiCC(jack_nframes_t fptr, uint8_t midichannel, uint8_t controller, uint8_t value)
{ jack_midi_data_t event[3] = {0b10110000 | midichannel, controller, value}; jack_midi_event_write(pb.midiout, fptr, event, 3); }

char triggerMidi(jack_nframes_t fptr, Channel *cv, uint8_t oldnote, uint8_t note, uint8_t inst)
{
	if (note != NOTE_VOID && !cv->data.mute && instrumentSafe(p->s, inst))
	{
		Instrument *iv = &p->s->instrumentv[p->s->instrumenti[inst]];
		if (iv->midichannel != -1)
		{
			/* always stop the prev. note */
			midiNoteOff(fptr, iv->midichannel, oldnote, (cv->randgain>>4)<<3);
			midiNoteOn (fptr, iv->midichannel, note,    (cv->randgain>>4)<<3);
			return 1;
		}
	} return 0;
}


void preprocessRow(jack_nframes_t fptr, char midi, Channel *cv, Row r)
{
	char ret;
	uint8_t oldnote = NOTE_UNUSED;

	for (int i = 0; i <= cv->data.macroc; i++)
		cv->r.macro[i] = r.macro[i];

	/* end interpolation */
	if (cv->targetgain != -1)
	{
		if (cv->data.target_rand)
		{
			cv->randgain = cv->targetgain;
			cv->data.target_rand = 0;
		} else cv->gain = cv->randgain = cv->targetgain;
		cv->targetgain = -1;
	}
	if (cv->targetwaveshaperstrength != -1) { cv->waveshaperstrength = cv->targetwaveshaperstrength; cv->targetwaveshaperstrength = -1; }
	if (cv->targetsendgain != -1) { cv->sendgain = cv->targetsendgain; cv->targetsendgain = -1; }
	if (cv->rtrigsamples)
	{
		if (cv->rtrigblocksize > 0 || cv->rtrigblocksize == -1) cv->rtrigblocksize--;
		else
		{
			cv->data.rtrig_rev = 0;
			cv->rtrigsamples = 0;
		}
	}

	/* end interpolation */
	if (cv->targetfiltermode != -1) { cv->filtermode = cv->targetfiltermode; cv->targetfiltermode = -1; }
	if (cv->targetfiltercut != -1) { cv->filtercut = cv->targetfiltercut; cv->targetfiltercut = -1; }
	if (cv->targetfilterres != -1) { cv->filterres = cv->targetfilterres; cv->targetfilterres = -1; }

	ret = ifMacro(fptr, cv, r, 'V', &Vc);
	if (!ret) cv->vibratosamples = 0;

	if (ifMacro(fptr, cv, r, '%', &percentc)) return;
	ifMacro(fptr, cv, r, 'b', &Bc); /* bpm */

	if (cv->pointer
			&&(ifMacro(fptr, cv, r, 'G', &DUMMY)
			|| ifMacro(fptr, cv, r, 'I', &DUMMY)))
		ramp(cv, p->s->instrumenti[cv->samplerinst]);

	ifMacro(fptr, cv, r, 'G', &Gc); /* gain              */
	ifMacro(fptr, cv, r, 'I', &Ic); /* rand gain         */
	ifMacro(fptr, cv, r, 'g', &gc); /* smooth gain       */
	ifMacro(fptr, cv, r, 'i', &ic); /* smooth rand gain  */
	ifMacro(fptr, cv, r, 'S', &Sc); /* send              */
	ifMacro(fptr, cv, r, 's', &sc); /* smooth send       */

	ret = ifMacro(fptr, cv, r, 'C', &Cc); /* cut */
	if (!ret && r.note != NOTE_VOID) { ret = ifMacro(fptr, cv, r, 'P', &Pc); /* portamento */
		if (!ret)                    { ret = ifMacro(fptr, cv, r, 'D', &Dc); /* delay      */
			if (!ret)
			{
				oldnote = cv->r.note;
				/* TODO: not sure what this conditional is here for tbh */
				if (r.inst == INST_VOID)
				{
					if (instrumentSafe(p->s, cv->samplerinst))
						ramp(cv, p->s->instrumenti[cv->samplerinst]);
					triggerNote(cv, r.note, cv->r.inst);
				} else
				{
					if (instrumentSafe(p->s, cv->r.inst))
						ramp(cv, p->s->instrumenti[cv->r.inst]);
					triggerNote(cv, r.note, r.inst);
				}
			}
		}
	}

	if (cv->pointer && instrumentSafe(p->s, cv->samplerinst)
			&&(ifMacro(fptr, cv, r, 'F', &DUMMY)
			|| ifMacro(fptr, cv, r, 'Z', &DUMMY)
			|| ifMacro(fptr, cv, r, 'O', &DUMMY)
			|| ifMacro(fptr, cv, r, 'o', &ocPRERAMP)
			|| ifMacro(fptr, cv, r, 'U', &DUMMY)
			|| ifMacro(fptr, cv, r, 'u', &DUMMY)))
		ramp(cv, p->s->instrumenti[cv->samplerinst]);

	ifMacro(fptr, cv, r, 'F', &Fc); /* cutoff            */
	ifMacro(fptr, cv, r, 'Z', &Zc); /* resonance         */
	ifMacro(fptr, cv, r, 'f', &fc); /* smooth cutoff     */
	ifMacro(fptr, cv, r, 'z', &zc); /* smooth resonance  */
	ifMacro(fptr, cv, r, 'O', &Oc); /* offset            */
	ifMacro(fptr, cv, r, 'o', &oc); /* bw offset         */
	ifMacro(fptr, cv, r, 'U', &Uc); /* rand offset       */
	ifMacro(fptr, cv, r, 'u', &uc); /* rand bw offset    */
	ifMacro(fptr, cv, r, 'E', &Ec); /* local envelope    */
	ifMacro(fptr, cv, r, 'H', &Hc); /* local pitch shift */
	ifMacro(fptr, cv, r, 'L', &Lc); /* local cyclelength */

	/* midi */
	if (oldnote != NOTE_UNUSED && midi) /* trigger midi (needs to be after gain calculation) */
	{
		if (r.inst == INST_VOID) { if (cv->r.inst != INST_VOID) ret = triggerMidi(fptr, cv, oldnote, r.note, cv->r.inst); }
		else                                                    ret = triggerMidi(fptr, cv, oldnote, r.note, r.inst);
	}
	ifMacro(fptr, cv, r, ';', &midicctargetc);
	ifMacro(fptr, cv, r, '@', &midipcc);
	ifMacro(fptr, cv, r, '.', &midiccc);

	ifMacro(fptr, cv, r, 'M', &Mc);             /* microtonal offset  */

	/* retrigger macros (all *4* of them) */
	if (       !ifMacro(fptr, cv, r, 'q', &qc)  /* bw retrigger       */
	        && !ifMacro(fptr, cv, r, 'Q', &Qc)  /* retrigger          */
	        && !ifMacro(fptr, cv, r, 'r', &rc)) /* bw block retrigger */
		ifMacro(fptr, cv, r, 'R', &Rc);         /* block retrigger    */
	if (cv->rtrigsamples && cv->rtrigblocksize < -1)
	{ /* clean up if the last row had a [Q,q]xx and this row doesn't */
		cv->data.rtrig_rev = 0;
		cv->rtrigsamples = 0;
	}

	ifMacro(fptr, cv, r, 'W', &Wc); /* waveshaper         */
	ifMacro(fptr, cv, r, 'w', &wc); /* smooth waveshaper  */
}

void postSampler(jack_nframes_t fptr, Channel *cv, float rp,
		float lf, float rf, SVFilter fl[2], SVFilter fr[2],
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

	lf = hardclip(lf);
	rf = hardclip(rf);

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

	if (cv->outputl && cv->outputr)
	{
		cv->outputl[fptr] = lf;
		cv->outputr[fptr] = rf;
	}

	/* if (cv->targetsendgain != -1)
	{
		ls = lf * cv->sendgain*DIV15 + (cv->targetsendgain - cv->sendgain)*DIV15 * rp;
		rs = rf * cv->sendgain*DIV15 + (cv->targetsendgain - cv->sendgain)*DIV15 * rp;
	} else if (cv->sendgain)
	{
		ls = lf * cv->sendgain*DIV15;
		rs = rf * cv->sendgain*DIV15;
	} */
}

void playChannelLookback(jack_nframes_t fptr, Channel *cv)
{
	float multiplier;
	uint32_t delta;

	uint16_t sprs = 0;
	uint16_t sprp;

	if (cv->delaysamples > cv->cutsamples)
	{ /* delay is set and takes priority over cut */
		sprs = cv->delaysamples;

		triggerNote(cv, cv->delaynote, cv->delayinst);
		cv->delaysamples = 0;

		ifMacro(fptr, cv, cv->r, 'O', &Oc); /* offset         */
		ifMacro(fptr, cv, cv->r, 'o', &oc); /* bw offset      */
		ifMacro(fptr, cv, cv->r, 'U', &Uc); /* rand offset    */
		ifMacro(fptr, cv, cv->r, 'u', &uc); /* rand bw offset */
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

			if (cv->data.reverse)
			{
				delta = (int)((cv->portamentosamplepointer+1)*multiplier) - (int)(cv->portamentosamplepointer*multiplier);
				if (cv->pitchedpointer > delta) cv->pitchedpointer -= delta;
			} else
				cv->pitchedpointer += (int)((cv->portamentosamplepointer+1)*multiplier) - (int)(cv->portamentosamplepointer*multiplier);

			cv->portamentosamplepointer++;
		} else cv->finetune += cv->portamentofinetune;
	}

	/* process the sampler */
	if (instrumentSafe(p->s, cv->samplerinst) && cv->samplernote != NOTE_VOID
			&& cv->samplernote != NOTE_OFF && cv->samplerinst != INST_VOID)
	{
		Instrument *iv = &p->s->instrumentv[p->s->instrumenti[cv->samplerinst]];
		for (sprp = 0; sprp < p->s->spr - sprs; sprp++)
			envelope(iv, cv, cv->pointer+sprp);

		if (cv->portamentosamplepointer >= cv->portamentosamples)
		{ /* only walk pitchedpointer if not pitch sliding */
			multiplier = powf(M_12_ROOT_2, (short)cv->samplernote - NOTE_C5 + cv->finetune);
			if (cv->data.reverse)
			{
				delta = (int)((cv->pointer+(sprp - sprs))*multiplier) - (int)(cv->pointer*multiplier);
				if (cv->pitchedpointer > delta) cv->pitchedpointer -= delta;
				else                            cv->pitchedpointer = 0;
			} else
				cv->pitchedpointer += (int)((cv->pointer+(sprp - sprs))*multiplier) - (int)(cv->pointer*multiplier);
		} cv->pointer += sprp - sprs;
	}
}
void playChannel(jack_nframes_t fptr, uint16_t sprp, Channel *cv)
{
	short li = 0;
	short ri = 0;

	float multiplier;
	uint32_t delta;

	if (cv->cutsamples && sprp > cv->cutsamples)
	{
		ramp(cv, p->s->instrumenti[cv->samplerinst]);
		triggerNote(cv, NOTE_OFF, cv->r.inst);
		triggerMidi(fptr, cv, cv->r.note, NOTE_OFF, cv->r.inst);
		cv->cutsamples = 0;
	}
	if (cv->delaysamples && sprp > cv->delaysamples)
	{
		ramp(cv, p->s->instrumenti[cv->delayinst]);
		triggerNote(cv, cv->delaynote, cv->delayinst);
		triggerMidi(fptr, cv, cv->r.note, cv->delaynote, cv->delayinst);
		cv->delaysamples = 0;

		ifMacro(fptr, cv, cv->r, 'F', &Fc); /* cutoff            */
		ifMacro(fptr, cv, cv->r, 'Z', &Zc); /* resonance         */
		ifMacro(fptr, cv, cv->r, 'f', &fc); /* smooth cutoff     */
		ifMacro(fptr, cv, cv->r, 'z', &zc); /* smooth resonance  */
		ifMacro(fptr, cv, cv->r, 'O', &Oc); /* offset            */
		ifMacro(fptr, cv, cv->r, 'o', &oc); /* bw offset         */
		ifMacro(fptr, cv, cv->r, 'U', &Uc); /* rand offset       */
		ifMacro(fptr, cv, cv->r, 'u', &uc); /* rand bw offset    */
		ifMacro(fptr, cv, cv->r, 'E', &Ec); /* local envelope    */
		ifMacro(fptr, cv, cv->r, 'H', &Hc); /* local pitch shift */
		ifMacro(fptr, cv, cv->r, 'L', &Lc); /* local cyclelength */
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

		if (cv->data.reverse)
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

	if (instrumentSafe(p->s, cv->samplerinst))
	{
		Instrument *iv = &p->s->instrumentv[p->s->instrumenti[cv->samplerinst]];

		if (!cv->data.mute && !(sprp%PITCH_WHEEL_SAMPLES) && iv->midichannel != -1 && cv->finetune != 0.0f)
			midiPitchWheel(fptr, iv->midichannel, MIN(2.0f, MAX(-2.0f, cv->finetune + cv->portamentofinetune)));

		/* process the sampler */
		if (cv->samplerinst != INST_VOID && cv->samplernote != NOTE_VOID && cv->samplernote != NOTE_OFF)
		{
			if (cv->rtrigsamples)
			{
				uint32_t rtrigoffset = (cv->pointer - cv->rtrigpointer) % cv->rtrigsamples;
				if (!rtrigoffset)
				{ /* first sample of any retrigger */
					if (iv->midichannel != -1)
					{
						midiNoteOff(fptr, iv->midichannel, cv->r.note, (cv->randgain>>4)<<3);
						midiNoteOn (fptr, iv->midichannel, cv->r.note, (cv->randgain>>4)<<3);
					}
					if (cv->pointer > cv->rtrigpointer) /* first sample of any retrigger but the first */
						cv->rtrigcurrentpitchedpointer = cv->pitchedpointer;
				}
				if (cv->data.rtrig_rev) samplerProcess(p->s->instrumenti[cv->samplerinst], cv, cv->pointer, cv->rtrigpitchedpointer - (cv->pitchedpointer - cv->rtrigcurrentpitchedpointer), &li, &ri);
				else                    samplerProcess(p->s->instrumenti[cv->samplerinst], cv, cv->pointer, cv->rtrigpitchedpointer + (cv->pitchedpointer - cv->rtrigcurrentpitchedpointer), &li, &ri);
			} else samplerProcess(p->s->instrumenti[cv->samplerinst], cv, cv->pointer, cv->pitchedpointer, &li, &ri);

			if (cv->portamentosamplepointer >= cv->portamentosamples)
			{ /* only walk pitchedpointer if not pitch sliding */
				multiplier = powf(M_12_ROOT_2, (short)cv->samplernote - NOTE_C5 + cv->finetune);
				if (cv->data.reverse)
				{
					delta = (int)((cv->pointer+1)*multiplier) - (int)(cv->pointer*multiplier);
					if (cv->pitchedpointer > delta) cv->pitchedpointer -= delta;
				} else
					cv->pitchedpointer += (int)((cv->pointer+1)*multiplier) - (int)(cv->pointer*multiplier);
			} cv->pointer++;
		}
	}

	float lf = (float)li*DIVSHRT;
	float rf = (float)ri*DIVSHRT;

	float samplegain;
	float rowprogress = (float)sprp / (float)p->s->spr;
	if (cv->rampbuffer && cv->rampindex < rampmax)
	{ // ramping
		if (!cv->data.mute)
		{
			float gain = (float)cv->rampindex / (float)rampmax;

			if (instrumentSafe(p->s, cv->samplerinst))
			{
				samplegain = powf(2, (float)p->s->instrumentv[p->s->instrumenti[cv->samplerinst]].gain*DIV16);
				lf *= samplegain * gain;
				rf *= samplegain * gain;
			}

			if (cv->rampinst < p->s->instrumentc)
			{
				samplegain = powf(2, (float)p->s->instrumentv[p->s->instrumenti[cv->rampinst]].gain*DIV16);
				lf += ((float)cv->rampbuffer[cv->rampindex*2 + 0]*DIVSHRT)*samplegain * (1.0f - gain);
				rf += ((float)cv->rampbuffer[cv->rampindex*2 + 1]*DIVSHRT)*samplegain * (1.0f - gain);
			}

			postSampler(fptr, cv, rowprogress, lf, rf,
					cv->fl, cv->fr, cv->randgain, cv->targetgain);
		}

		cv->rampindex++;
	} else if (!cv->data.mute && instrumentSafe(p->s, cv->samplerinst))
	{
		samplegain = powf(2, (float)p->s->instrumentv[p->s->instrumenti[cv->samplerinst]].gain*DIV16);
		lf *= samplegain;
		rf *= samplegain;

		postSampler(fptr, cv, rowprogress, lf, rf,
				cv->fl, cv->fr, cv->randgain, cv->targetgain);
	}
}

void lookback(jack_nframes_t fptr)
{
	/* for every row before the current one */
	for (uint16_t r = 0; r < p->s->playfy; r++)
		for (uint8_t c = 0; c < p->s->channelc; c++)
		{
			preprocessRow(fptr, 0, &p->s->channelv[c], *getChannelRow(&p->s->channelv[c].data, r));
			playChannelLookback(fptr, &p->s->channelv[c]);
		}
}

typedef struct
{
	jack_nframes_t iterations;
	jack_nframes_t fptroffset;
	Channel *cv;
} ChannelThreadArg;

void *channelThreadRoutine(void *arg)
{
	for (int i = 0; i < ((ChannelThreadArg *)arg)->iterations; i++)
		playChannel(((ChannelThreadArg *)arg)->fptroffset + i,
				p->s->sprp + i, ((ChannelThreadArg *)arg)->cv);
	return NULL;
}

int process(jack_nframes_t nfptr, void *arg)
{
	PlaybackInfo *p = arg;
	Channel *cv;
	Instrument *iv;

	pb.in.l =    jack_port_get_buffer(p->in.l,    nfptr);
	pb.in.r =    jack_port_get_buffer(p->in.r,    nfptr);
	pb.out.l =   jack_port_get_buffer(p->out.l,   nfptr); memset(pb.out.l, 0, nfptr * sizeof(sample_t));
	pb.out.r =   jack_port_get_buffer(p->out.r,   nfptr); memset(pb.out.r, 0, nfptr * sizeof(sample_t));
	pb.midiout = jack_port_get_buffer(p->midiout, nfptr); jack_midi_clear_buffer(pb.midiout);

	if (p->sem == M_SEM_RELOAD_REQ) p->sem++;
	if (p->sem != M_SEM_OK) return 0;


	/* will no longer write to the record buffer */
	if (p->w->instrumentrecv == INST_REC_LOCK_PREP_END)    p->w->instrumentrecv = INST_REC_LOCK_END;
	if (p->w->instrumentrecv == INST_REC_LOCK_PREP_CANCEL) p->w->instrumentrecv = INST_REC_LOCK_CANCEL;
	/* start recording immediately if not cueing */
	if (p->w->instrumentrecv == INST_REC_LOCK_START)
	{
		p->w->instrumentrecv = INST_REC_LOCK_CONT;
		p->dirty = 1;
	}
	/* force stop midi for every channel playing this instrument */
	/* if (p->w->instrumentlockv == INST_GLOBAL_INST_MUTE)
	{
		for (uint8_t c = 0; c < p->s->channelc; c++)
		{
			cv = &p->s->channelv[c];
			if (p->s->instrumenti[cv->r.inst] == p->w->instrumentlocki)
			{
				midiNoteOff(0, p->s->instrumentv[p->s->instrumenti[cv->r.inst]].midichannel, cv->r.note, (cv->randgain>>4)<<3);
				cv->r.note = NOTE_VOID;
			}
		} p->w->instrumentlockv = INST_GLOBAL_LOCK_OK;
	} */

	/* force stop midi data when a channels are muted */
	if (p->w->instrumentlockv == INST_GLOBAL_CHANNEL_MUTE)
	{
		for (uint8_t c = 0; c < p->s->channelc; c++)
		{
			cv = &p->s->channelv[c];
			if (cv->data.mute && instrumentSafe(p->s, cv->r.inst) && p->s->instrumentv[p->s->instrumenti[cv->r.inst]].midichannel != -1)
			{
				midiNoteOff(0, p->s->instrumentv[p->s->instrumenti[cv->r.inst]].midichannel, cv->r.note, (cv->randgain>>4)<<3);
				cv->r.note = NOTE_VOID;
			}
		}

		p->w->instrumentlockv = INST_GLOBAL_LOCK_OK;
	}

	Row holdr;
	uint8_t holdnote, holdinst;
	if (p->w->previewtrigger)
	{ // start instrument preview
		if (!p->s->playing)
		{
			holdr = p->w->previewchannel.r;
			holdnote = p->w->previewchannel.samplernote;
			holdinst = p->w->previewchannel.samplerinst;
			clearChannelRuntime(&p->w->previewchannel);
			p->w->previewchannel.r = holdr; /* don't clear out cv->r, for midi */
			p->w->previewchannel.samplernote = holdnote;
			p->w->previewchannel.samplerinst = holdinst;

			preprocessRow(0, 1, &p->w->previewchannel, p->w->previewrow);
		}
		p->w->previewtrigger = 0;
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
			if (!cv->data.mute && instrumentSafe(p->s, cv->r.inst))
			{
				iv = &p->s->instrumentv[p->s->instrumenti[cv->r.inst]];
				if (iv->midichannel != -1) midiNoteOff(0, iv->midichannel, cv->r.note, (cv->randgain>>4)<<3);
			} clearChannelRuntime(cv);
		}

		lookback(0);

		/* start recording if cueing */
		if (p->w->instrumentrecv == INST_REC_LOCK_CUE_START)
			p->w->instrumentrecv = INST_REC_LOCK_CUE_CONT;
		p->s->playing = PLAYING_CONT;

		p->dirty=1;
	} else if (p->s->playing == PLAYING_PREP_STOP)
	{
		/* stop channels */
		Channel *cv;
		for (uint8_t i = 0; i < p->s->channelc; i++)
		{
			cv = &p->s->channelv[i];
			cv->delaysamples = 0;
			cv->cutsamples = 0;
			ramp(cv, p->s->instrumenti[cv->samplerinst]);
			triggerNote(cv, NOTE_OFF, cv->r.inst);
			triggerMidi(0, cv, cv->r.note, NOTE_OFF, cv->r.inst);
		}

		p->dirty = 1;
		p->s->playing = PLAYING_STOP;
	}

	if (p->w->request == REQ_BPM)
	{
		changeBpm(p->s, p->s->songbpm);
		p->w->request = REQ_OK;
	}


	/*                        MULTITHREADING                         */
	/* isn't strictly realtime-safe, but *should* be ok (maybe)      */
	/* honestly half this file probably isn't strictly realtime-safe */
	jack_native_thread_t thread_ids[p->s->channelc]; /* index 0 is the preview channel */
	ChannelThreadArg     thread_arg[p->s->channelc]; /* index 0 is the preview channel */
	ChannelThreadArg main_arg; /* arg used for channel 0 */
	jack_nframes_t fptr = 0;
	jack_nframes_t iterations = 0;

	/* handle the preview channel first in a thread */
	if (p->w->previewchannel.samplernote != NOTE_VOID
			&& p->w->previewchannel.samplerinst != INST_VOID)
	{
		thread_arg[0].iterations = nfptr;
		thread_arg[0].fptroffset = 0;
		thread_arg[0].cv = &p->w->previewchannel;
#ifdef NO_MULTITHREADING
		channelThreadRoutine(&thread_arg[0]);
#else
#ifndef NO_VALGRIND
		if (RUNNING_ON_VALGRIND)
			channelThreadRoutine(&thread_arg[0]);
		else
#endif
			jack_client_create_thread(client, &thread_ids[0],
					jack_client_real_time_priority(client), jack_is_realtime(client),
					channelThreadRoutine, &thread_arg[0]);
#endif
	}

	while (fptr < nfptr)
	{
		/* next row */
		if (p->s->sprp > p->s->spr)
		{
			p->s->sprp = 0;

			if (p->s->playing == PLAYING_CONT)
			{
				/* preprocess channels */
				for (uint8_t c = 0; c < p->s->channelc; c++)
					preprocessRow(fptr, 1, &p->s->channelv[c], *getChannelRow(&p->s->channelv[c].data, p->s->playfy));

				/* loop at the end of the song */
				if (p->s->playfy++ >= p->s->loop[1])
				{
					p->s->playfy = p->s->loop[0];
					lookback(fptr);
					
					/* handle record cue */
					/* TODO: how to do record cue with the new sequencing approach? */
					/* if (p->w->instrumentrecv == INST_REC_LOCK_CUE_CONT) p->w->instrumentrecv = INST_REC_LOCK_END;
					if (p->w->instrumentrecv == INST_REC_LOCK_CUE_START) p->w->instrumentrecv = INST_REC_LOCK_CUE_CONT; */
				}

				if (p->w->follow)
					p->w->trackerfy = p->s->playfy;
				p->dirty = 1;
			}
		}

		iterations = MIN(nfptr - fptr, p->s->spr - p->s->sprp);

		/* spawn threads for each channel except for channel 0 */
		for (uint8_t i = 1; i < p->s->channelc; i++)
		{
			thread_arg[i].iterations = iterations;
			thread_arg[i].fptroffset = fptr;
			thread_arg[i].cv = &p->s->channelv[i];
#ifdef NO_MULTITHREADING
			channelThreadRoutine(&thread_arg[i]);
#else
#ifndef NO_VALGRIND
			if (RUNNING_ON_VALGRIND)
				channelThreadRoutine(&thread_arg[i]);
			else
#endif
				jack_client_create_thread(client, &thread_ids[i],
						jack_client_real_time_priority(client), jack_is_realtime(client),
						channelThreadRoutine, &thread_arg[i]);
#endif
		}

		/* run channel 0 in this thread */
		main_arg.iterations = iterations;
		main_arg.fptroffset = fptr;
		main_arg.cv = &p->s->channelv[0];
		channelThreadRoutine(&main_arg);

		/* join with the channel threads */
#ifndef NO_MULTITHREADING
#ifndef NO_VALGRIND
		if (!RUNNING_ON_VALGRIND)
#endif
			for (uint8_t i = 1; i < p->s->channelc; i++)
				pthread_join(thread_ids[i], NULL);
#endif


		fptr       += iterations + 1; /* TODO: is this +1 a hack? genuinely can't tell */
		p->s->sprp += iterations + 1; /* TODO: is this +1 a hack? genuinely can't tell */
	}
	/* join with the prevew channel thread */
#ifndef NO_MULTITHREADING
#ifndef NO_VALGRIND
	if (p->w->previewchannel.samplernote != NOTE_VOID
			&& p->w->previewchannel.samplerinst != INST_VOID
			&& !RUNNING_ON_VALGRIND)
#else
	if (p->w->previewchannel.samplernote != NOTE_VOID
			&& p->w->previewchannel.samplerinst != INST_VOID)
#endif
		pthread_join(thread_ids[0], NULL);
#endif


	/* sum the output from each channel thread */
	for (uint8_t c = 0; c < p->s->channelc; c++)
		if (p->s->channelv[c].outputl && p->s->channelv[c].outputr)
			for (fptr = 0; fptr < nfptr; fptr++)
			{
				pb.out.l[fptr] += p->s->channelv[c].outputl[fptr];
				pb.out.r[fptr] += p->s->channelv[c].outputr[fptr];
			}
	if (p->w->previewchannel.samplernote != NOTE_VOID
			&& p->w->previewchannel.samplerinst != INST_VOID)
		for (fptr = 0; fptr < nfptr; fptr++)
		{
			pb.out.l[fptr] += p->w->previewchannel.outputl[fptr];
			pb.out.r[fptr] += p->w->previewchannel.outputr[fptr];
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

	/* background visualization */
#ifdef ENABLE_BACKGROUND
	updateBackground(nfptr, pb.out.l, pb.out.r);
#endif

	/* instrument trigger animation */
	for (int i = 0; i < p->s->instrumentc; i++)
		if (p->s->instrumentv[i].triggerflash)
		{
			if (p->s->instrumentv[i].triggerflash == 1) p->dirty = 1;
			p->s->instrumentv[i].triggerflash--;
		}

	return 0;
}
