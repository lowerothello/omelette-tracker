void setBpm(uint16_t *spr, uint8_t newbpm)
{ *spr = samplerate * (60.f / newbpm) / p->s->rowhighlight; }

/* freewheel to fill up the ramp buffer */
void ramp(Channel *cv, float rp, uint8_t realinstrument)
{
	if (cv->rampbuffer)
	{
		/* clear the rampbuffer properly so cruft isn't played in edge cases */
		memset(cv->rampbuffer, 0, sizeof(short) * rampmax * 2);

		/* save state */
		cv->rampinst = realinstrument;

		uint32_t pointeroffset = cv->pointer;
		uint32_t pitchedpointeroffset = cv->pitchedpointer;
		if (realinstrument < p->s->instrument->c)
		{
			pitchedpointeroffset++;
			float oldenvgain = cv->envgain;
			float oldmodenvgain = cv->modenvgain;
			if (cv->data.reverse)
			{
				jack_nframes_t localrampmax;
				if (pointeroffset < rampmax)
					localrampmax = rampmax - pointeroffset;
				else localrampmax = rampmax;

				for (uint16_t i = 0; i < localrampmax; i++)
				{
					if (pitchedpointeroffset) pitchedpointeroffset--;
					samplerProcess(realinstrument, cv, rp, pointeroffset+i, pitchedpointeroffset,
							&cv->rampbuffer[i*2 + 0], &cv->rampbuffer[i*2 + 1]);
				}
			} else
				for (uint16_t i = 0; i < rampmax; i++)
				{
					pitchedpointeroffset++;
					samplerProcess(realinstrument, cv, rp, pointeroffset+i, pitchedpointeroffset,
							&cv->rampbuffer[i*2 + 0], &cv->rampbuffer[i*2 + 1]);
				}
			cv->envgain = oldenvgain;
			cv->modenvgain = oldmodenvgain;
		}
	} cv->rampindex = 0;
}

void triggerNote(Channel *cv, uint8_t note, short inst)
{
	if (note == NOTE_VOID) return;
	if (note == NOTE_OFF)
	{
		cv->data.release = 1;
		cv->r.inst = inst;
		cv->r.note = note;
	} else
	{
		if (inst < 0)
		{
			cv->r.inst = INST_VOID;
			cv->samplerinst = inst;
		} else cv->r.inst = cv->samplerinst = inst;

		cv->r.note = cv->samplernote = note;
		cv->pointer = cv->pitchedpointer = 0;
		cv->data.reverse = 0;
		cv->data.release = 0;
		cv->portamentosamples = 0; cv->portamentosamplepointer = 1;
		cv->startportamentofinetune = cv->targetportamentofinetune = cv->portamentofinetune = 0.0f;
		cv->microtonalfinetune = 0.0f;
		cv->vibrato = 0;

		/* local controls */
		cv->localenvelope = -1;
		cv->localsustain = -1;
		cv->localpitchshift = -1;
		cv->localpitchwidth = -1;
		cv->localcyclelength = -1;
		cv->localsamplerate = -1;
		
		/* must stop retriggers cos pointers are no longer guaranteed to be valid */
		cv->rtrigblocksize = 0;
		cv->data.rtrig_rev = 0;
		cv->rtrigsamples = 0;

		if (inst >= 0 && !cv->data.mute) /* if inst is not special and unmuted */
		{
			if (instrumentSafe(p->s, inst))
			{
				Instrument *iv = &p->s->instrument->v[p->s->instrument->i[inst]];
				cv->filter.mode[0] = cv->filter.mode[1] = iv->filtermode;
				cv->filter.cut[0] =  cv->filter.cut[1] = cv->filter.randcut[0] =  cv->filter.randcut[1] = iv->filtercutoff;
				cv->filter.res[0] =  cv->filter.res[1] = cv->filter.randres[0] =  cv->filter.randres[1] = iv->filterresonance;
				iv->triggerflash = samplerate / buffersize * INSTRUMENT_TRIGGER_FLASH_S; /* instrument trig flash */
			}
			cv->triggerflash = samplerate / buffersize * INSTRUMENT_TRIGGER_FLASH_S; /* channel trig flash */
			p->redraw = 1;
		}
	}
}

void midiNoteOff(jack_nframes_t fptr, uint8_t midichannel, uint8_t note, uint8_t velocity)
{
#ifndef DEBUG_DISABLE_AUDIO_OUTPUT
	if (note != NOTE_VOID && note != NOTE_OFF)
	{
		jack_midi_data_t event[3] = {0b10000000 | midichannel, note, velocity};
		jack_midi_event_write(pb.midiout, fptr, event, 3);
	}
#endif
}
void midiPitchWheel(jack_nframes_t fptr, uint8_t midichannel, float pitch)
{
#ifndef DEBUG_DISABLE_AUDIO_OUTPUT
	unsigned short intpitch = (int)(pitch * 4096.0f) + 8192;
	jack_midi_data_t event[3] = {0b11100000 | midichannel, (unsigned short)(intpitch<<9)>>9, intpitch>>7};
	jack_midi_event_write(pb.midiout, fptr, event, 3);
#endif
}
void midiNoteOn(jack_nframes_t fptr, uint8_t midichannel, uint8_t note, uint8_t velocity)
{
#ifndef DEBUG_DISABLE_AUDIO_OUTPUT
	if (note != NOTE_VOID && note != NOTE_OFF)
	{
		jack_midi_data_t event[3] = {0b10010000 | midichannel, note, velocity};
		jack_midi_event_write(pb.midiout, fptr, event, 3);
	}
#endif
}

void midiPC(jack_nframes_t fptr, uint8_t midichannel, uint8_t program)
{
#ifndef DEBUG_DISABLE_AUDIO_OUTPUT
	jack_midi_data_t event[2] = {0b11000000 | midichannel, program};
	jack_midi_event_write(pb.midiout, fptr, event, 2);
#endif
}
void midiCC(jack_nframes_t fptr, uint8_t midichannel, uint8_t controller, uint8_t value)
{
#ifndef DEBUG_DISABLE_AUDIO_OUTPUT
	jack_midi_data_t event[3] = {0b10110000 | midichannel, controller, value};
	jack_midi_event_write(pb.midiout, fptr, event, 3);
#endif
}

char triggerMidi(jack_nframes_t fptr, Channel *cv, uint8_t oldnote, uint8_t note, uint8_t inst)
{
	if (note != NOTE_VOID && !cv->data.mute && instrumentSafe(p->s, inst))
	{
		Instrument *iv = &p->s->instrument->v[p->s->instrument->i[inst]];
		if (iv->algorithm == INST_ALG_MIDI)
		{
			/* always stop the prev. note */
			midiNoteOff(fptr, iv->midi.channel, oldnote, (cv->gain.rand>>4)<<3);
			midiNoteOn (fptr, iv->midi.channel, note,    (cv->gain.rand>>4)<<3);
			return 1;
		}
	} return 0;
}

void handleLocalMacros(jack_nframes_t fptr, uint16_t *spr, Channel *cv, Row r)
{
	ifMacro(fptr, spr, cv, r, 'F', 0, &Fc);    /* filter cutoff            */
	ifMacro(fptr, spr, cv, r, 'f', 0, &fc);    /* smooth filter cutoff     */
	ifMacro(fptr, spr, cv, r, 'Z', 0, &Zc);    /* filter resonance         */
	ifMacro(fptr, spr, cv, r, 'z', 0, &zc);    /* smooth filter resonance  */
	ifMacro(fptr, spr, cv, r, 'F', 1, &altFc); /* filter cutoff jitter     */
	ifMacro(fptr, spr, cv, r, 'f', 1, &altfc); /* smooth filter cut jitter */
	ifMacro(fptr, spr, cv, r, 'Z', 1, &altZc); /* filter resonance jitter  */
	ifMacro(fptr, spr, cv, r, 'z', 1, &altzc); /* smooth filter res jitter */
	ifMacro(fptr, spr, cv, r, 'M', 0, &Mc);    /* filter mode              */
	ifMacro(fptr, spr, cv, r, 'm', 0, &mc);    /* smooth filter mode       */
	ifMacro(fptr, spr, cv, r, 'O', 0, &Oc);    /* offset                   */
	ifMacro(fptr, spr, cv, r, 'O', 1, &altOc); /* rand offset              */
	ifMacro(fptr, spr, cv, r, 'o', 0, &oc);    /* bw offset                */
	ifMacro(fptr, spr, cv, r, 'o', 1, &altoc); /* rand bw offset           */
	ifMacro(fptr, spr, cv, r, 'E', 0, &Ec);    /* local envelope           */
	ifMacro(fptr, spr, cv, r, 'e', 0, &ec);    /* local sustain            */
	ifMacro(fptr, spr, cv, r, 'H', 0, &Hc);    /* local pitch shift        */
	ifMacro(fptr, spr, cv, r, 'h', 0, &hc);    /* smooth local pitch shift */
	ifMacro(fptr, spr, cv, r, 'W', 0, &Wc);    /* local pitch width        */
	ifMacro(fptr, spr, cv, r, 'w', 0, &wc);    /* smooth local pitch width */
	ifMacro(fptr, spr, cv, r, 'L', 0, &Lc);    /* local cyclelength        */
	ifMacro(fptr, spr, cv, r, 'X', 0, &Xc);    /* local samplerate         */
	ifMacro(fptr, spr, cv, r, 'x', 0, &xc);    /* smooth local samplerate  */
}

void preprocessRow(jack_nframes_t fptr, uint16_t *spr, bool midi, Channel *cv, Row r)
{
	char ret;
	uint8_t oldnote = NOTE_UNUSED;

	for (int i = 0; i <= cv->data.macroc; i++)
		cv->r.macro[i] = r.macro[i];

	/* end interpolation */
	if (cv->gain.target != -1)
	{
		if (cv->gain.target_rand) /* only apply the new gain to rand, not to base */
		{
			cv->gain.rand = cv->gain.target;
			cv->gain.target_rand = 0;
		} else cv->gain.base = cv->gain.rand = cv->gain.target;
		cv->gain.target = -1;
	}
	if (cv->send.target != -1)
	{
		if (cv->send.target_rand) /* only apply the new gain to rand, not to base */
		{
			cv->send.rand = cv->send.target;
			cv->send.target_rand = 0;
		} else cv->send.base = cv->send.rand = cv->send.target;
		cv->send.target = -1;
	}

	if (cv->rtrigsamples)
	{
		if (cv->rtrigblocksize > 0 || cv->rtrigblocksize == -1) cv->rtrigblocksize--;
		else
		{
			cv->data.rtrig_rev = 0;
			cv->rtrigsamples = 0;
		}
	}

	if (cv->filter.targetmode[0] != -1) { cv->filter.mode[0] = cv->filter.targetmode[0]; cv->filter.targetmode[0] = -1; }
	if (cv->filter.targetmode[1] != -1) { cv->filter.mode[1] = cv->filter.targetmode[1]; cv->filter.targetmode[1] = -1; }

	if (cv->filter.targetcut[0] != -1)
	{
		if (cv->filter.targetcut_rand) cv->filter.randcut[0] = cv->filter.targetcut[0];
		else                           cv->filter.cut[0] = cv->filter.randcut[0] = cv->filter.targetcut[0];
		cv->filter.targetcut[0] = -1;
	}
	if (cv->filter.targetcut[1] != -1)
	{
		if (cv->filter.targetcut_rand) cv->filter.randcut[1] = cv->filter.targetcut[1];
		else                           cv->filter.cut[1] = cv->filter.randcut[1] = cv->filter.targetcut[1];
		cv->filter.targetcut[1] = -1;
	}
	cv->filter.targetcut_rand = 0;

	if (cv->filter.targetcut[0] != -1)
	{
		if (cv->filter.targetres_rand) cv->filter.randres[0] = cv->filter.targetres[0];
		else                           cv->filter.res[0] = cv->filter.randres[0] = cv->filter.targetres[0];
		cv->filter.targetres[0] = -1;
	}
	if (cv->filter.targetcut[1] != -1)
	{
		if (cv->filter.targetres_rand) cv->filter.randres[1] = cv->filter.targetres[1];
		else                           cv->filter.res[1] = cv->filter.randres[1] = cv->filter.targetres[1];
		cv->filter.targetres[1] = -1;
	}
	cv->filter.targetres_rand = 0;

	if (cv->targetlocalsamplerate != -1) { cv->localsamplerate = cv->targetlocalsamplerate; cv->targetlocalsamplerate = -1; }
	if (cv->targetlocalpitchshift != -1) { cv->localpitchshift = cv->targetlocalpitchshift; cv->targetlocalpitchshift = -1; }
	if (cv->targetlocalpitchwidth != -1) { cv->localpitchwidth = cv->targetlocalpitchwidth; cv->targetlocalpitchwidth = -1; }

	ret = ifMacro(fptr, spr, cv, r, 'V', 0, &Vc);
	if (!ret) cv->vibratosamples = 0;

	if (ifMacro(fptr, spr, cv, r, '%', 0, &percentc)) return;

	if (cv->pointer && instrumentSafe(p->s, cv->samplerinst)
			&&(ifMacro(fptr, spr, cv, r, 'G', 0, &DUMMY)
			|| ifMacro(fptr, spr, cv, r, 'I', 0, &DUMMY)))
		ramp(cv, 0.0f, p->s->instrument->i[cv->samplerinst]);

	ifMacro(fptr, spr, cv, r, 'G', 0, &Gc);    /* gain               */
	ifMacro(fptr, spr, cv, r, 'g', 0, &gc);    /* smooth gain        */
	ifMacro(fptr, spr, cv, r, 'G', 1, &altGc); /* gain jitter        */
	ifMacro(fptr, spr, cv, r, 'g', 1, &altgc); /* smooth gain jitter */
	ifMacro(fptr, spr, cv, r, 'S', 0, &Sc);    /* send               */
	ifMacro(fptr, spr, cv, r, 's', 0, &sc);    /* smooth send        */
	ifMacro(fptr, spr, cv, r, 'S', 1, &altSc); /* send jitter        */
	ifMacro(fptr, spr, cv, r, 's', 1, &altsc); /* smooth send jitter */

	ret = ifMacro(fptr, spr, cv, r, 'C', 0, &Cc); /* cut */

	/* try to persist old state a little bit */
	if      (r.note != NOTE_VOID && r.inst == INST_VOID) r.inst = cv->r.inst;
	else if (r.inst != INST_VOID && r.note == NOTE_VOID) r.note = cv->r.note;


	if (!ret && r.note != NOTE_VOID) { ret = ifMacro(fptr, spr, cv, r, 'P', 0, &Pc); /* portamento */
		if (!ret)                    { ret = ifMacro(fptr, spr, cv, r, 'D', 0, &Dc); /* delay      */
			if (!ret)
			{
				oldnote = cv->r.note; /* used for midi TODO: jank */
				if (instrumentSafe(p->s, cv->r.inst))
					ramp(cv, 0.0f, p->s->instrument->i[cv->r.inst]);
				triggerNote(cv, r.note, r.inst);
			}
		}
	}

	if (cv->pointer && instrumentSafe(p->s, cv->samplerinst)
			&&(ifMacro(fptr, spr, cv, r, 'F', 0, &DUMMY)
			|| ifMacro(fptr, spr, cv, r, 'Z', 0, &DUMMY)
			|| ifMacro(fptr, spr, cv, r, 'O', 0, &DUMMY)
			|| ifMacro(fptr, spr, cv, r, 'o', 0, &ocPRERAMP)
			|| ifMacro(fptr, spr, cv, r, 'U', 0, &DUMMY)
			|| ifMacro(fptr, spr, cv, r, 'u', 0, &DUMMY)))
		ramp(cv, 0.0f, p->s->instrument->i[cv->samplerinst]);

	handleLocalMacros(fptr, spr, cv, r);

	/* midi */
	if (midi && oldnote != NOTE_UNUSED) /* trigger midi (needs to be after gain calculation) */
	{
		if (r.inst == INST_VOID) { if (cv->r.inst != INST_VOID) ret = triggerMidi(fptr, cv, oldnote, r.note, cv->r.inst); }
		else                                                    ret = triggerMidi(fptr, cv, oldnote, r.note, r.inst);
	}
	ifMacro(fptr, spr, cv, r, ';', 0, &midicctargetc);
	ifMacro(fptr, spr, cv, r, '@', 0, &midipcc);
	ifMacro(fptr, spr, cv, r, '.', 0, &midiccc);

	ifMacro(fptr, spr, cv, r, 'p', 0, &pc); /* microtonal offset  */

	/* retrigger macros */
	if (       !ifMacro(fptr, spr, cv, r, 'r', 1, &altrc)  /* bw retrigger       */
	        && !ifMacro(fptr, spr, cv, r, 'R', 1, &altRc)  /* retrigger          */
	        && !ifMacro(fptr, spr, cv, r, 'r', 0, &rc))    /* bw block retrigger */
		ifMacro(fptr, spr, cv, r, 'R', 0, &Rc);            /* block retrigger    */
	if (cv->rtrigsamples && cv->rtrigblocksize < -1)
	{ /* clean up if the last row had an altRxx and this row doesn't */
		cv->data.rtrig_rev = 0;
		cv->rtrigsamples = 0;
	}
}

void postSampler(jack_nframes_t fptr, Channel *cv, float rp, float lf, float rf)
{
	/* filter */
	float cutoff_l = cv->filter.randcut[0]*DIV255;
	float cutoff_r = cv->filter.randcut[1]*DIV255;
	float resonance_l = cv->filter.randres[0]*DIV255;
	float resonance_r = cv->filter.randres[1]*DIV255;
	if (cv->filter.targetcut[0] != -1) cutoff_l += (cv->filter.targetcut[0] - (short)cv->filter.randcut[0])*DIV255 * rp;
	if (cv->filter.targetcut[1] != -1) cutoff_r += (cv->filter.targetcut[1] - (short)cv->filter.randcut[1])*DIV255 * rp;
	if (cv->filter.targetres[0] != -1) resonance_l += (cv->filter.targetres[0] - cv->filter.randres[0])*DIV255 * rp;
	if (cv->filter.targetres[1] != -1) resonance_r += (cv->filter.targetres[1] - cv->filter.randres[1])*DIV255 * rp;

	/* first pass (12dB/oct) */
	runSVFilter(&cv->filter.fl[0], lf, cutoff_l, resonance_l);
	runSVFilter(&cv->filter.fr[0], rf, cutoff_r, resonance_r);
	switch (cv->filter.mode[0]&0x3)
	{
		case 0: /* low-pass  */ if (!(cv->filter.targetcut[0] == -1 && cv->filter.randcut[0] == 0xff)) lf = cv->filter.fl[0].l; break;
		case 1: /* high-pass */ if (!(cv->filter.targetcut[0] == -1 && cv->filter.randcut[0] == 0x00)) lf = cv->filter.fl[0].h; break;
		case 2: /* band-pass */ lf = cv->filter.fl[0].b; break;
		case 3: /* notch     */ lf = cv->filter.fl[0].n; break;
	}
	if (cv->filter.targetmode[0] != -1)
		switch (cv->filter.targetmode[0]&0x3)
		{
			case 0: /* low-pass  */ if (!(cv->filter.targetcut[0] == -1 && cv->filter.randcut[0] == 0xff)) lf += (cv->filter.fl[0].l - lf) * rp; break;
			case 1: /* high-pass */ if (!(cv->filter.targetcut[0] == -1 && cv->filter.randcut[0] == 0x00)) lf += (cv->filter.fl[0].h - lf) * rp; break;
			case 2: /* band-pass */ lf += (cv->filter.fl[0].b - lf) * rp; break;
			case 3: /* notch     */ lf += (cv->filter.fl[0].n - lf) * rp; break;
		}
	switch (cv->filter.mode[1]&0x3)
	{
		case 0: /* low-pass  */ if (!(cv->filter.targetcut[1] == -1 && cv->filter.randcut[1] == 0xff)) rf = cv->filter.fr[0].l; break;
		case 1: /* high-pass */ if (!(cv->filter.targetcut[1] == -1 && cv->filter.randcut[1] == 0x00)) rf = cv->filter.fr[0].h; break;
		case 2: /* band-pass */ rf = cv->filter.fr[0].b; break;
		case 3: /* notch     */ rf = cv->filter.fr[0].n; break;
	}
	if (cv->filter.targetmode[1] != -1)
		switch (cv->filter.targetmode[1]&0x3)
		{
			case 0: /* low-pass  */ if (!(cv->filter.targetcut[1] == -1 && cv->filter.randcut[1] == 0xff)) rf += (cv->filter.fr[0].l - rf) * rp; break;
			case 1: /* high-pass */ if (!(cv->filter.targetcut[1] == -1 && cv->filter.randcut[1] == 0x00)) rf += (cv->filter.fr[0].h - rf) * rp; break;
			case 2: /* band-pass */ rf += (cv->filter.fr[0].b - rf) * rp; break;
			case 3: /* notch     */ rf += (cv->filter.fr[0].n - rf) * rp; break;
		}

	/* second pass (24dB/oct) */
	runSVFilter(&cv->filter.fl[1], lf, cutoff_l, resonance_l);
	runSVFilter(&cv->filter.fr[1], rf, cutoff_r, resonance_r);
	if (cv->filter.mode[0]&0b100) /* if the '4' bit is set */
		switch (cv->filter.mode[0]&0x3)
		{
			case 0: /* low-pass  */ if (!(cv->filter.targetcut[0] == -1 && cv->filter.randcut[0] == 0xff)) lf = cv->filter.fl[1].l; break;
			case 1: /* high-pass */ if (!(cv->filter.targetcut[0] == -1 && cv->filter.randcut[0] == 0x00)) lf = cv->filter.fl[1].h; break;
			case 2: /* band-pass */ lf = cv->filter.fl[1].b; break;
			case 3: /* notch     */ lf = cv->filter.fl[1].n; break;
		}
	if (cv->filter.targetmode[0] != -1 && cv->filter.targetmode[0]&0b100) /* if the '4' bit is set */
		switch (cv->filter.targetmode[0]&0x3)
		{
			case 0: /* low-pass  */ if (!(cv->filter.targetcut[0] == -1 && cv->filter.randcut[0] == 0xff)) { lf += (cv->filter.fl[1].l - lf) * rp; } break;
			case 1: /* high-pass */ if (!(cv->filter.targetcut[0] == -1 && cv->filter.randcut[0] == 0x00)) { lf += (cv->filter.fl[1].h - lf) * rp; } break;
			case 2: /* band-pass */ lf += (cv->filter.fl[1].b - lf) * rp; break;
			case 3: /* notch     */ lf += (cv->filter.fl[1].n - lf) * rp; break;
		}
	if (cv->filter.mode[1]&0b100) /* if the '4' bit is set */
		switch (cv->filter.mode[1]&0x3)
		{
			case 0: /* low-pass  */ if (!(cv->filter.targetcut[1] == -1 && cv->filter.randcut[1] == 0xff)) rf = cv->filter.fr[1].l; break;
			case 1: /* high-pass */ if (!(cv->filter.targetcut[1] == -1 && cv->filter.randcut[1] == 0x00)) rf = cv->filter.fr[1].h; break;
			case 2: /* band-pass */ rf = cv->filter.fr[1].b; break;
			case 3: /* notch     */ rf = cv->filter.fr[1].n; break;
		}
	if (cv->filter.targetmode[1] != -1 && cv->filter.targetmode[1]&0b100) /* if the '4' bit is set */
		switch (cv->filter.targetmode[1]&0x3)
		{
			case 0: /* low-pass  */ if (!(cv->filter.targetcut[1] == -1 && cv->filter.randcut[1] == 0xff)) { rf += (cv->filter.fr[1].l - rf) * rp; } break;
			case 1: /* high-pass */ if (!(cv->filter.targetcut[1] == -1 && cv->filter.randcut[1] == 0x00)) { rf += (cv->filter.fr[1].h - rf) * rp; } break;
			case 2: /* band-pass */ rf += (cv->filter.fr[1].b - rf) * rp; break;
			case 3: /* notch     */ rf += (cv->filter.fr[1].n - rf) * rp; break;
		}
	lf = hardclip(lf);
	rf = hardclip(rf);

	/* gain multipliers */
	if (cv->gain.target != -1)
	{
		cv->mainmult[0][fptr] = (cv->gain.rand>>4)*DIV16 + ((cv->gain.target>>4) - (cv->gain.rand>>4))*DIV16 * rp;
		cv->mainmult[1][fptr] = (cv->gain.rand&15)*DIV16 + ((cv->gain.target&15) - (cv->gain.rand&15))*DIV16 * rp;
	} else
	{
		cv->mainmult[0][fptr] = (cv->gain.rand>>4)*DIV16;
		cv->mainmult[1][fptr] = (cv->gain.rand&15)*DIV16;
	}

	cv->output[0][fptr] = lf;
	cv->output[1][fptr] = rf;

	if (cv->send.target != -1)
	{
		cv->sendmult[0][fptr] = (cv->send.rand>>4)*DIV16 + ((cv->send.target>>4) - (cv->send.rand>>4))*DIV16 * rp;
		cv->sendmult[1][fptr] = (cv->send.rand&15)*DIV16 + ((cv->send.target&15) - (cv->send.rand>>4))*DIV16 * rp;
	} else if (cv->send.rand)
	{
		cv->sendmult[0][fptr] = (cv->send.rand>>4)*DIV16;
		cv->sendmult[1][fptr] = (cv->send.rand&15)*DIV16;
	}
}

void playChannelLookback(jack_nframes_t fptr, uint16_t *spr, Channel *cv)
{
	uint16_t sprs = 0; /* spr start    */
	uint16_t sprp;     /* spr progress */

	if (cv->delaysamples > cv->cutsamples)
	{ /* delay is set and takes priority over cut */
		sprs = cv->delaysamples;

		triggerNote(cv, cv->delaynote, cv->delayinst);
		cv->delaysamples = 0;

		handleLocalMacros(fptr, spr, cv, cv->r);
		cv->envgain = cv->modenvgain = 0.0f; /* TODO: necessary? */ /* not perfect but a safe enough assumption */
	} else if (cv->cutsamples)
	{ /* cut is set and takes priority over delay */
		triggerNote(cv, NOTE_OFF, cv->r.inst);
		cv->cutsamples = 0;
		cv->envgain = cv->modenvgain = 0.0f; /* immediately cut the envelopes, not perfect but a safe enough assumption */
		return;
	}

	for (sprp = sprs; sprp < *spr; sprp++)
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

			if (cv->data.reverse)
			{
				if (cv->pitchedpointer > *spr - sprs) cv->pitchedpointer -= *spr - sprs;
				else                                  cv->pitchedpointer = 0;
				if (cv->pointer > *spr - sprs) cv->pointer -= *spr - sprs;
				else                           cv->pointer = 0;
			} else
			{
				cv->pitchedpointer += *spr - sprs;
				cv->pointer        += *spr - sprs;
			}

			cv->portamentosamplepointer++;
		} else cv->finetune += cv->portamentofinetune;
	}

	/* process the sampler */
	if (instrumentSafe(p->s, cv->samplerinst) && cv->samplernote != NOTE_VOID
			&& cv->samplernote != NOTE_OFF && cv->samplerinst != INST_VOID)
	{
		Instrument *iv = &p->s->instrument->v[p->s->instrument->i[cv->samplerinst]];
		uint16_t env;
		for (sprp = 0; sprp < *spr - sprs; sprp++)
		{
			env = iv->envelope; if (cv->localenvelope != -1) env = cv->localenvelope;
			envelope(env, cv, cv->pointer+sprp, &cv->envgain);
			envelope(iv->wavetable.envelope, cv, cv->pointer+sprp, &cv->modenvgain);
		}

		if (cv->portamentosamplepointer >= cv->portamentosamples)
		{ /* only walk pitchedpointer if not pitch sliding */
			if (cv->data.reverse)
			{
				if (cv->pitchedpointer > *spr - sprs) cv->pitchedpointer -= *spr - sprs;
				else                                  cv->pitchedpointer = 0;
				if (cv->pointer > *spr - sprs) cv->pointer -= *spr - sprs;
				else                           cv->pointer = 0;
			} else
			{
				cv->pitchedpointer += *spr - sprs;
				cv->pointer        += *spr - sprs;
			}
		}
	}
}
void playChannel(bool midi, jack_nframes_t fptr, uint16_t *spr, jack_nframes_t sprp, Channel *cv)
{
	short li = 0;
	short ri = 0;

	uint32_t delta;
	float multiplier;

	float rowprogress = (float)sprp / (float)(*spr);

	if (cv->cutsamples && sprp > cv->cutsamples)
	{
		if (instrumentSafe(p->s, cv->samplerinst))
			ramp(cv, rowprogress, p->s->instrument->i[cv->samplerinst]);
		triggerNote(cv, NOTE_OFF, cv->r.inst);
		if (midi) triggerMidi(fptr, cv, cv->r.note, NOTE_OFF, cv->r.inst);
		cv->cutsamples = 0;
	}
	if (cv->delaysamples && sprp > cv->delaysamples)
	{
		if (instrumentSafe(p->s, cv->delayinst))
			ramp(cv, rowprogress, p->s->instrument->i[cv->delayinst]);
		triggerNote(cv, cv->delaynote, cv->delayinst);
		if (midi) triggerMidi(fptr, cv, cv->r.note, cv->delaynote, cv->delayinst);
		cv->delaysamples = 0;

		handleLocalMacros(fptr, spr, cv, cv->r);
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

		multiplier = powf(M_12_ROOT_2, cv->portamentofinetune);
		delta = (int)((cv->portamentosamplepointer+1)*multiplier) - (int)(cv->portamentosamplepointer*multiplier);

		if (cv->data.reverse)
		{
			if (cv->pitchedpointer > delta) cv->pitchedpointer -= delta;
			else                            cv->pitchedpointer = 0;
		} else cv->pitchedpointer += delta;

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
			if (!ifMacro(fptr, spr, cv, cv->r, 'V', 0, &Vc))
				cv->vibratosamples = 0;
		}
	}

	float lf, rf;
	float samplegain;
	switch (cv->samplerinst)
	{
		case INST_FILEPREVIEW:
			if (!cv->data.mute && p->w->previewsample && cv->samplernote != NOTE_VOID && cv->samplernote != NOTE_OFF)
			{
				minimalSamplerProcess(p->w->previewsample, cv->pitchedpointer, 0xff, 0xf, cv->samplernote, &li, &ri);

				if (cv->data.reverse) { if (cv->pitchedpointer) cv->pitchedpointer--; }
				else                                            cv->pitchedpointer++;
				cv->pointer++;

				postSampler(fptr, cv, rowprogress, li*DIVSHRT, ri*DIVSHRT);
			} else cv->output[0][fptr] = cv->output[1][fptr] = 0.0f;
			break;
		default:
			if (instrumentSafe(p->s, cv->samplerinst))
			{
				Instrument *iv = &p->s->instrument->v[p->s->instrument->i[cv->samplerinst]];

				if (midi && !cv->data.mute && !(sprp%PITCH_WHEEL_SAMPLES) && iv->algorithm == INST_ALG_MIDI && cv->finetune != 0.0f)
					midiPitchWheel(fptr, iv->midi.channel, MIN(2.0f, MAX(-2.0f, cv->finetune + cv->portamentofinetune)));

				/* process the sampler */
				if (cv->samplerinst != INST_VOID && cv->samplernote != NOTE_VOID && cv->samplernote != NOTE_OFF)
				{
					if (cv->rtrigsamples)
					{
						uint32_t rtrigoffset = (cv->pointer - cv->rtrigpointer) % cv->rtrigsamples;
						if (!rtrigoffset)
						{ /* first sample of any retrigger */
							if (midi && iv->algorithm == INST_ALG_MIDI)
							{
								midiNoteOff(fptr, iv->midi.channel, cv->r.note, (cv->gain.rand>>4)<<3);
								midiNoteOn (fptr, iv->midi.channel, cv->r.note, (cv->gain.rand>>4)<<3);
							}
							if (cv->pointer > cv->rtrigpointer) /* first sample of any retrigger but the first */
							{
								cv->rtrigcurrentpitchedpointer = cv->pitchedpointer;
								cv->rtrigcurrentpointer = cv->pointer;
							}
						}
						if (cv->data.rtrig_rev) samplerProcess(p->s->instrument->i[cv->samplerinst], cv, rowprogress, cv->rtrigpointer - (cv->pointer - cv->rtrigcurrentpointer), cv->rtrigpitchedpointer - (cv->pitchedpointer - cv->rtrigcurrentpitchedpointer), &li, &ri);
						else                    samplerProcess(p->s->instrument->i[cv->samplerinst], cv, rowprogress, cv->rtrigpointer + (cv->pointer - cv->rtrigcurrentpointer), cv->rtrigpitchedpointer + (cv->pitchedpointer - cv->rtrigcurrentpitchedpointer), &li, &ri);
					} else samplerProcess(p->s->instrument->i[cv->samplerinst], cv, rowprogress, cv->pointer, cv->pitchedpointer, &li, &ri);

					if (cv->portamentosamplepointer >= cv->portamentosamples)
					{ /* only walk pitchedpointer if not pitch sliding */
						if (cv->data.reverse) { if (cv->pitchedpointer) cv->pitchedpointer--; }
						else                                            cv->pitchedpointer++;
					} cv->pointer++;
				}
			}

			lf = li*DIVSHRT;
			rf = ri*DIVSHRT;

			if (cv->rampbuffer && cv->rampindex < rampmax)
			{ // ramping
				if (!cv->data.mute)
				{
					float gain = (float)cv->rampindex / (float)rampmax;

					if (instrumentSafe(p->s, cv->samplerinst))
					{
						samplegain = powf(2, (float)p->s->instrument->v[p->s->instrument->i[cv->samplerinst]].gain*DIV16);
						lf *= samplegain * gain;
						rf *= samplegain * gain;
					}

					if (cv->rampinst < p->s->instrument->c)
					{
						samplegain = powf(2, (float)p->s->instrument->v[p->s->instrument->i[cv->rampinst]].gain*DIV16);
						lf += ((float)cv->rampbuffer[cv->rampindex*2 + 0]*DIVSHRT)*samplegain * (1.0f - gain);
						rf += ((float)cv->rampbuffer[cv->rampindex*2 + 1]*DIVSHRT)*samplegain * (1.0f - gain);
					}

					postSampler(fptr, cv, rowprogress, lf, rf);
				} else cv->output[0][fptr] = cv->output[1][fptr] = 0.0f;

				cv->rampindex++;
			} else if (!cv->data.mute && instrumentSafe(p->s, cv->samplerinst))
			{
				samplegain = powf(2, (float)p->s->instrument->v[p->s->instrument->i[cv->samplerinst]].gain*DIV16);
				lf *= samplegain;
				rf *= samplegain;

				postSampler(fptr, cv, rowprogress, lf, rf);
			} else cv->output[0][fptr] = cv->output[1][fptr] = 0.0f;
			break;
	}
}

void lookback(jack_nframes_t fptr, uint16_t *spr, uint16_t playfy, Channel *cv)
{
	clearChannelRuntime(cv);

	/* for every row before the current one */
	Row *r;
	for (uint16_t i = 0; i < playfy; i++)
	{
		/* scope lookback notes within the most recent vtrig */
		if (cv->data.trig[i].index != VARIANT_VOID) { cv->r.note = NOTE_OFF; cv->envgain = 0.0f; }

		r = getChannelRow(&cv->data, i);
		if (p->s->bpmcachelen > i && p->s->bpmcache[i] != -1) Bc(fptr, spr, p->s->bpmcache[i], cv, *r);
		preprocessRow(fptr, spr, 0, cv, *r);
		playChannelLookback(fptr, spr, cv);
	}
}

void _channelThreadRoutine(Channel *cv, uint16_t *spr, uint16_t *sprp, uint16_t *playfy, bool readrows)
{
	Row *r;
	for (jack_nframes_t fptr = 0; fptr < buffersize; fptr++) /* TODO: probably shouldn't rely on buffersize here */
	{
		playChannel(1, fptr, spr, *sprp, cv);

		/* next row */
		if ((*sprp)++ > *spr)
		{
			*sprp = 0;
			if (readrows && p->s->playing == PLAYING_CONT)
			{
				/* walk the pointer and loop */
				(*playfy)++;
				if (p->s->loop[1] && *playfy > p->s->loop[1])
				{
					*playfy = p->s->loop[0];
					if (p->s->loop[2])
					{
						p->s->loop[1] = p->s->loop[2];
						p->s->loop[2] = 0;
					}
					lookback(fptr, spr, *playfy, cv);
				} else if (!p->s->loop[1] && *playfy >= p->s->songlen)
				{
					*playfy = STATE_ROWS;
					lookback(fptr, spr, *playfy, cv);
				}
					/* TODO: record cue */
					/* if (p->w->instrumentrecv == INST_REC_LOCK_CUE_CONT) p->w->instrumentrecv = INST_REC_LOCK_END;
					if (p->w->instrumentrecv == INST_REC_LOCK_CUE_START) p->w->instrumentrecv = INST_REC_LOCK_CUE_CONT; */

				/* preprocess channel */
				r = getChannelRow(&cv->data, *playfy);
				if (p->s->bpmcachelen > *playfy && p->s->bpmcache[*playfy] != -1) Bc(fptr, spr, p->s->bpmcache[*playfy], cv, *r);
				preprocessRow(fptr, spr, 1, cv, *r);
			}
		}
	}

	/* channel insert effects */
	if (cv->data.effect) /* previewchannel doesn't have any effects so this check is required */
		for (uint8_t i = 0; i < cv->data.effect->c; i++)
			runEffect(buffersize, cv->data.effect, &cv->data.effect->v[i]);
}

void *channelThreadRoutine(void *arg)
{
	uint16_t spr = p->s->spr;
	uint16_t sprp = p->s->sprp;
	uint16_t playfy = p->s->playfy;
	_channelThreadRoutine(arg, &spr, &sprp, &playfy, 1);
	return NULL;
}
void *previewChannelThreadRoutine(void *arg) /* don't try to read rows that don't exist */
{
	uint16_t spr = p->s->spr;
	uint16_t sprp = p->s->sprp;
	uint16_t playfy = p->s->playfy;
	_channelThreadRoutine(arg, &spr, &sprp, &playfy, 0);
	return NULL;
}

#ifndef DEBUG_DISABLE_AUDIO_OUTPUT
int process(jack_nframes_t nfptr, void *arg)
{
#else
void *process(void *arg)
{
	struct timespec req;
	while (1)
	{
#endif
	PlaybackInfo *p = arg;
	Channel *cv;
	Instrument *iv;

#ifndef DEBUG_DISABLE_AUDIO_OUTPUT
	pb.in.l =    jack_port_get_buffer(p->in.l,    nfptr);
	pb.in.r =    jack_port_get_buffer(p->in.r,    nfptr);
	pb.out.l =   jack_port_get_buffer(p->out.l,   nfptr);
	pb.out.r =   jack_port_get_buffer(p->out.r,   nfptr);
	pb.midiout = jack_port_get_buffer(p->midiout, nfptr); jack_midi_clear_buffer(pb.midiout);
#endif

	if (processM_SEM()) return 0;

	/* will no longer write to the record buffer */
	if (p->w->instrumentrecv == INST_REC_LOCK_PREP_END)    p->w->instrumentrecv = INST_REC_LOCK_END;
	if (p->w->instrumentrecv == INST_REC_LOCK_PREP_CANCEL) p->w->instrumentrecv = INST_REC_LOCK_CANCEL;
	/* start recording immediately if not cueing */
	if (p->w->instrumentrecv == INST_REC_LOCK_START)
	{
		p->w->instrumentrecv = INST_REC_LOCK_CONT;
		p->redraw = 1;
	}

	Row holdr;
	uint8_t holdnote, holdinst;
	switch (p->w->previewtrigger)
	{
		case PTRIG_NORMAL:
			/* start instrument preview */
			/* TODO: this holding should all be unnecessary */
			holdr = p->w->previewchannel.r;
			holdnote = p->w->previewchannel.samplernote;
			holdinst = p->w->previewchannel.samplerinst;
			clearChannelRuntime(&p->w->previewchannel);
			p->w->previewchannel.r = holdr; /* don't clear out cv->r, for midi */
			p->w->previewchannel.samplernote = holdnote;
			p->w->previewchannel.samplerinst = holdinst;

			preprocessRow(0, &p->s->spr, 1, &p->w->previewchannel, p->w->previewrow);
			p->w->previewtrigger = PTRIG_OK;
			break;
		case PTRIG_FILE:
			// ramp(cv, 0.0f, p->s->instrument->i[cv->r.inst]);
			triggerNote(&p->w->previewchannel, p->w->previewrow.note, INST_FILEPREVIEW);
			p->w->previewtrigger = PTRIG_OK;
			break;
	}

	/* start/stop playback */
	if (p->s->playing == PLAYING_START)
	{
		setBpm(&p->s->spr, p->s->songbpm);
		p->s->sprp = 0;

		/* stop preview */
		p->w->previewchannel.r.note = p->w->previewchannel.samplernote = NOTE_VOID;
		p->w->previewchannel.r.inst = p->w->previewchannel.samplerinst = INST_VOID;

		/* TODO: also stop the sampler's follower note */
		/* TODO: is it worth it to multithread this?   */
		/* clear the channels */
		for (uint8_t c = 0; c < p->s->channel->c; c++)
		{
			cv = &p->s->channel->v[c];
			if (!cv->data.mute && instrumentSafe(p->s, cv->r.inst))
			{
				iv = &p->s->instrument->v[p->s->instrument->i[cv->r.inst]];
				if (iv->algorithm == INST_ALG_MIDI) midiNoteOff(0, iv->midi.channel, cv->r.note, (cv->gain.rand>>4)<<3);
			}

			lookback(0, &p->s->spr, p->s->playfy, cv);
			preprocessRow(0, &p->s->spr, 1, &p->s->channel->v[c], *getChannelRow(&p->s->channel->v[c].data, p->s->playfy));
		}


		/* start recording if cueing */
		if (p->w->instrumentrecv == INST_REC_LOCK_CUE_START)
			p->w->instrumentrecv = INST_REC_LOCK_CUE_CONT;
		p->s->playing = PLAYING_CONT;

		p->redraw=1;
	} else if (p->s->playing == PLAYING_PREP_STOP)
	{
		/* stop channels */
		Channel *cv;
		for (uint8_t i = 0; i < p->s->channel->c; i++)
		{
			cv = &p->s->channel->v[i];
			cv->delaysamples = 0;
			cv->cutsamples = 0;
			if (instrumentSafe(p->s, cv->samplerinst))
				ramp(cv, (float)p->s->sprp / (float)p->s->spr, p->s->instrument->i[cv->samplerinst]);
			triggerNote(cv, NOTE_OFF, cv->r.inst);
			triggerMidi(0, cv, cv->r.note, NOTE_OFF, cv->r.inst);
		}

		if (p->s->loop[2])
		{
			p->s->loop[1] = p->s->loop[2];
			p->s->loop[2] = 0;
		}

		p->redraw = 1;
		p->s->playing = PLAYING_STOP;
	}

#ifdef DEBUG_DISABLE_AUDIO_OUTPUT
	req.tv_sec  = 0; /* nanosleep can set this higher sometimes, so set every cycle */
	req.tv_nsec = UPDATE_DELAY;
	while(nanosleep(&req, &req) < 0);
	continue;
#endif
	/*                        MULTITHREADING                         */
	/* isn't strictly realtime-safe, but *should* be ok (maybe)      */
	/* honestly half this file probably isn't strictly realtime-safe */
#ifndef NO_MULTITHREADING
	jack_native_thread_t thread_ids   [p->s->channel->c]; /* index 0 is the preview channel */
	bool                 thread_failed[p->s->channel->c]; /* if the thread failed to initialize, try REALLY hard not to segfault */
	memset(thread_failed, 0, sizeof(bool) * p->s->channel->c); /* gcc probably makes this static or smth */
#endif

	/* handle the preview channel first in a thread */
	if (p->w->previewchannel.samplernote != NOTE_VOID
			&& p->w->previewchannel.samplerinst != INST_VOID)
	{
#ifdef NO_MULTITHREADING
		previewChannelThreadRoutine(&p->w->previewchannel);
#else
#ifndef NO_VALGRIND
		if (RUNNING_ON_VALGRIND)
			previewChannelThreadRoutine(&p->w->previewchannel);
		else
#endif
			/* try spawning a thread, if it fails then do the work in this thread */
			if (jack_client_create_thread(client, &thread_ids[0],
					jack_client_real_time_priority(client), jack_is_realtime(client),
					previewChannelThreadRoutine, &p->w->previewchannel))
			{
				thread_failed[0] = 1;
				previewChannelThreadRoutine(&p->w->previewchannel);
			}
#endif
	}

	/* spawn threads for each channel except for channel 0 */
	for (uint8_t i = 1; i < p->s->channel->c; i++)
	{
#ifdef NO_MULTITHREADING
		channelThreadRoutine(&p->s->channel->v[i]);
#else
#ifndef NO_VALGRIND
		if (RUNNING_ON_VALGRIND)
			channelThreadRoutine(&p->s->channel->v[i]);
		else
#endif
			/* try spawning a thread, if it fails then do the work in this thread */
			if (jack_client_create_thread(client, &thread_ids[i],
					jack_client_real_time_priority(client), jack_is_realtime(client),
					channelThreadRoutine, &p->s->channel->v[i]))
			{
				thread_failed[i] = 1;
				channelThreadRoutine(&p->s->channel->v[i]);
			}
#endif
	}

	/* run channel 0 in this thread */
	uint16_t c0spr = p->s->spr;
	uint16_t c0sprp = p->s->sprp;
	uint16_t c0playfy = p->s->playfy;
	if (p->s->channel->c)
		_channelThreadRoutine(&p->s->channel->v[0], &c0spr, &c0sprp, &c0playfy, 1);

	/* join with the channel threads */
#ifndef NO_MULTITHREADING
#ifndef NO_VALGRIND
	if (!RUNNING_ON_VALGRIND)
#endif
		for (uint8_t i = 1; i < p->s->channel->c; i++)
			if (!thread_failed[i])
				pthread_join(thread_ids[i], NULL);
#endif


	/* join with the prevew channel thread */
#ifndef NO_MULTITHREADING
#ifndef NO_VALGRIND
	if (!RUNNING_ON_VALGRIND && !thread_failed[0]
			&& p->w->previewchannel.samplernote != NOTE_VOID
			&& p->w->previewchannel.samplerinst != INST_VOID)
#else
	if (!thread_failed[0]
			&& p->w->previewchannel.samplernote != NOTE_VOID
			&& p->w->previewchannel.samplerinst != INST_VOID)
#endif
		pthread_join(thread_ids[0], NULL);
#endif


	/* apply the new sprp and playfy channel0 calculated */
	if (c0playfy != p->s->playfy)
	{
		if (p->w->follow) p->w->trackerfy = c0playfy;
		p->redraw = 1;
	}
	p->s->playfy = c0playfy;
	p->s->sprp = c0sprp;
	p->s->spr = c0spr;


#ifndef DEBUG_DISABLE_AUDIO_OUTPUT
	if (p->w->previewchannel.samplernote != NOTE_VOID
			&& p->w->previewchannel.samplerinst != INST_VOID)
		for (jack_nframes_t fptr = 0; fptr < nfptr; fptr++)
		{
			pb.out.l[fptr] = p->w->previewchannel.output[0][fptr] * p->w->previewchannel.mainmult[0][fptr];
			pb.out.r[fptr] = p->w->previewchannel.output[1][fptr] * p->w->previewchannel.mainmult[1][fptr];
		}
	else
	{
		memset(pb.out.l, 0, nfptr * sizeof(jack_default_audio_sample_t));
		memset(pb.out.r, 0, nfptr * sizeof(jack_default_audio_sample_t));
	}

	/* sum the output from each channel thread */
	for (uint8_t c = 0; c < p->s->channel->c; c++)
		if (p->s->channel->v[c].output[0] && p->s->channel->v[c].output[1])
			for (jack_nframes_t fptr = 0; fptr < nfptr; fptr++)
			{
				pb.out.l[fptr] += p->s->channel->v[c].output[0][fptr] * p->s->channel->v[c].mainmult[0][fptr];
				pb.out.r[fptr] += p->s->channel->v[c].output[1][fptr] * p->s->channel->v[c].mainmult[1][fptr];
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
				if (p->w->recptr % samplerate == 0) p->redraw = 1;
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
#endif /* ENABLE_BACKGROUND */
#endif /* DEBUG_DISABLE_AUDIO_OUTPUT */

	/* triggerflash animations */
	for (int i = 0; i < p->s->instrument->c; i++)
		if (p->s->instrument->v[i].triggerflash)
		{
			if (p->s->instrument->v[i].triggerflash == 1) p->redraw = 1;
			p->s->instrument->v[i].triggerflash--;
		}

	for (int i = 0; i < p->s->channel->c; i++)
		if (p->s->channel->v[i].triggerflash)
		{
			if (p->s->channel->v[i].triggerflash == 1) p->redraw = 1;
			p->s->channel->v[i].triggerflash--;
		}

#ifdef DEBUG_DISABLE_AUDIO_OUTPUT
	req.tv_sec  = 0; /* nanosleep can set this higher sometimes, so set every cycle */
	req.tv_nsec = UPDATE_DELAY;
	while(nanosleep(&req, &req) < 0);
	}
#endif

	return 0;
}
