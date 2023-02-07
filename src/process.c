void setBpm(uint16_t *spr, uint8_t newbpm)
{ *spr = samplerate * (60.f / newbpm) / p->s->rowhighlight; }

/* freewheel to fill up the ramp buffer */
void ramp(Track *cv, float rp, uint8_t realinstrument)
{
	if (cv->rampbuffer)
	{
		/* clear the rampbuffer properly so cruft isn't played in edge cases */
		memset(cv->rampbuffer, 0, sizeof(float) * rampmax * 2);

		/* save state */
		float samplegain = powf(2, (float)p->s->instrument->v[realinstrument].gain*DIV16);

		uint32_t pointeroffset = cv->pointer;
		uint32_t pitchedpointeroffset = cv->pitchedpointer;
		if (realinstrument < p->s->instrument->c)
		{
			pitchedpointeroffset++;
			float oldenvgain = cv->envgain;
			float oldmodenvgain = cv->modenvgain;
			short l, r;
			if (cv->data.reverse)
			{
				jack_nframes_t localrampmax;
				if (pointeroffset < rampmax)
					localrampmax = rampmax - pointeroffset;
				else localrampmax = rampmax;

				for (uint16_t i = 0; i < localrampmax; i++)
				{
					if (pitchedpointeroffset) pitchedpointeroffset--;
					samplerProcess(realinstrument, cv, rp, pointeroffset+i, pitchedpointeroffset, &l, &r);
					cv->rampbuffer[i*2 + 0] = (float)l*DIVSHRT * samplegain;
					cv->rampbuffer[i*2 + 1] = (float)r*DIVSHRT * samplegain;
				}
			} else
				for (uint16_t i = 0; i < rampmax; i++)
				{
					pitchedpointeroffset++;
					samplerProcess(realinstrument, cv, rp, pointeroffset+i, pitchedpointeroffset, &l, &r);
					cv->rampbuffer[i*2 + 0] = (float)l*DIVSHRT * samplegain;
					cv->rampbuffer[i*2 + 1] = (float)r*DIVSHRT * samplegain;
				}
			cv->envgain = oldenvgain;
			cv->modenvgain = oldmodenvgain;
		}
	} cv->rampindex = 0;
}

void midiNoteOff(jack_nframes_t fptr, uint8_t miditrack, uint8_t note, uint8_t velocity)
{
#ifndef DEBUG_DISABLE_AUDIO_OUTPUT
	if (note != NOTE_VOID)
	{
		jack_midi_data_t event[3] = {0b10000000 | miditrack, note, velocity};
		jack_midi_event_write(pb.midiout, fptr, event, 3);
	}
#endif
}
void midiNoteOn(jack_nframes_t fptr, uint8_t miditrack, uint8_t note, uint8_t velocity)
{
#ifndef DEBUG_DISABLE_AUDIO_OUTPUT
	if (note != NOTE_VOID)
	{
		jack_midi_data_t event[3] = {0b10010000 | miditrack, note, velocity};
		jack_midi_event_write(pb.midiout, fptr, event, 3);
	}
#endif
}

void midiPC(jack_nframes_t fptr, uint8_t miditrack, uint8_t program)
{
#ifndef DEBUG_DISABLE_AUDIO_OUTPUT
	jack_midi_data_t event[2] = {0b11000000 | miditrack, program};
	jack_midi_event_write(pb.midiout, fptr, event, 2);
#endif
}
void midiCC(jack_nframes_t fptr, uint8_t miditrack, uint8_t controller, uint8_t value)
{
#ifndef DEBUG_DISABLE_AUDIO_OUTPUT
	jack_midi_data_t event[3] = {0b10110000 | miditrack, controller, value};
	jack_midi_event_write(pb.midiout, fptr, event, 3);
#endif
}

bool triggerMidi(jack_nframes_t fptr, Track *cv, uint8_t oldnote, uint8_t note, uint8_t inst)
{
	if (note != NOTE_VOID && !cv->data.mute && instrumentSafe(p->s->instrument, inst))
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

void triggerNote(jack_nframes_t fptr, Track *cv, uint8_t oldnote, uint8_t note, short inst)
{
	triggerMidi(fptr, cv, oldnote, note, inst);

	/* TODO: note is NEVER set to NOTE_VOID in this file, pretty sure that's a bug */
	switch (note)
	{
		case NOTE_VOID:
		case NOTE_OFF:
			cv->data.release = 1;
			cv->r.inst = inst;
			cv->r.note = NOTE_VOID;
			break;
		case NOTE_CUT:
			cv->data.release = 0;
			cv->envgain = 0.0f;
			cv->r.inst = inst;
			cv->r.note = NOTE_VOID;
			break;
		default:
			if (inst < 0)
			{
				cv->r.inst = INST_VOID;
				cv->file = 1;
			} else
			{
				cv->r.inst = inst;
				cv->file = 0;
			}

			cv->r.note = note;
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
				if (instrumentSafe(p->s->instrument, inst))
				{
					Instrument *iv = &p->s->instrument->v[p->s->instrument->i[inst]];
					iv->triggerflash = samplerate / buffersize * INSTRUMENT_TRIGGER_FLASH_S; /* instrument trig flash */
				}
				cv->triggerflash = samplerate / buffersize * INSTRUMENT_TRIGGER_FLASH_S; /* track trig flash */
				p->redraw = 1;
			}
			break;
	}
}


static void handleLocalMacros(jack_nframes_t fptr, uint16_t *spr, Track *cv, Row r)
{
	ifMacro(fptr, spr, cv, r, 'F');
	ifMacro(fptr, spr, cv, r, 'f');
	ifMacro(fptr, spr, cv, r, 'Z');
	ifMacro(fptr, spr, cv, r, 'z');
	// ifMacro(fptr, spr, cv, r, 'F');
	// ifMacro(fptr, spr, cv, r, 'f');
	// ifMacro(fptr, spr, cv, r, 'Z');
	// ifMacro(fptr, spr, cv, r, 'z');
	ifMacro(fptr, spr, cv, r, 'M');
	ifMacro(fptr, spr, cv, r, 'm');
	ifMacro(fptr, spr, cv, r, 'O');
	ifMacro(fptr, spr, cv, r, 'U');
	ifMacro(fptr, spr, cv, r, 'o');
	ifMacro(fptr, spr, cv, r, 'u');
	ifMacro(fptr, spr, cv, r, 'E');
	ifMacro(fptr, spr, cv, r, 'e');
	ifMacro(fptr, spr, cv, r, 'H');
	ifMacro(fptr, spr, cv, r, 'h');
	ifMacro(fptr, spr, cv, r, 'W');
	ifMacro(fptr, spr, cv, r, 'w');
	ifMacro(fptr, spr, cv, r, 'L');
	ifMacro(fptr, spr, cv, r, 'l');
	ifMacro(fptr, spr, cv, r, 'X');
	ifMacro(fptr, spr, cv, r, 'x');
}

/* TODO: temp name */
#define AA(TYPE, I) \
	if (cv->filter.target##TYPE[I] != -1) \
	{ \
		if (cv->filter.target##TYPE##_rand) cv->filter.rand##TYPE[I] = cv->filter.target##TYPE[I]; \
		else                                cv->filter.TYPE[I] = cv->filter.rand##TYPE[I] = cv->filter.target##TYPE[I]; \
		cv->filter.target##TYPE[I] = -1; \
	}

static void handleLerpMacros(jack_nframes_t fptr, uint16_t *spr, Track *cv, Row r)
{
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

	if (cv->filter.targetmode[0] != -1) { cv->filter.mode[0] = cv->filter.targetmode[0]; cv->filter.targetmode[0] = -1; }
	if (cv->filter.targetmode[1] != -1) { cv->filter.mode[1] = cv->filter.targetmode[1]; cv->filter.targetmode[1] = -1; }

	AA(cut, 0); AA(cut, 1); cv->filter.targetcut_rand = 0;
	AA(res, 0); AA(res, 1); cv->filter.targetres_rand = 0;

	if (cv->targetlocalsamplerate != -1) { cv->localsamplerate = cv->targetlocalsamplerate; cv->targetlocalsamplerate = -1; }
	if (cv->targetlocalpitchshift != -1) { cv->localpitchshift = cv->targetlocalpitchshift; cv->targetlocalpitchshift = -1; }
	if (cv->targetlocalpitchwidth != -1) { cv->localpitchwidth = cv->targetlocalpitchwidth; cv->targetlocalpitchwidth = -1; }

	ifMacro(fptr, spr, cv, r, 'g');
	ifMacro(fptr, spr, cv, r, 'j');
	ifMacro(fptr, spr, cv, r, 's');
	ifMacro(fptr, spr, cv, r, 'k');
}

void processRow(jack_nframes_t fptr, uint16_t *spr, bool midi, Track *cv, Row r)
{
	bool triggerramp = 0;
	Track oldcv;
	memcpy(&oldcv, cv, sizeof(Track)); /* rampbuffer is still accesible from this copy */

	for (int i = 0; i <= cv->data.variant->macroc; i++)
		cv->r.macro[i] = r.macro[i];

	handleLerpMacros(fptr, spr, cv, r);

	if (cv->rtrigsamples)
	{
		if (cv->rtrigblocksize > 0 || cv->rtrigblocksize == -1)
			cv->rtrigblocksize--;
		else
		{
			cv->data.rtrig_rev = 0;
			cv->rtrigsamples = 0;
		}
	}

	if (!ifMacro(fptr, spr, cv, r, 'V')) cv->vibratosamples = 0;
	if ( ifMacro(fptr, spr, cv, r, '%')) return;

	ifMacro(fptr, spr, cv, r, 'G');
	ifMacro(fptr, spr, cv, r, 'I');
	ifMacro(fptr, spr, cv, r, 'S');
	ifMacro(fptr, spr, cv, r, 'K');

	/* try to persist old state a little bit */
	if      (r.note != NOTE_VOID && r.inst == INST_VOID) r.inst = cv->r.inst;
	else if (r.note == NOTE_VOID && r.inst != INST_VOID) r.note = cv->r.note;

	if ((!ifMacro(fptr, spr, cv, r, 'C') && r.note != NOTE_VOID)
			&& !ifMacro(fptr, spr, cv, r, 'P')
			&& !ifMacro(fptr, spr, cv, r, 'D'))
	{
		triggerNote(fptr, cv, cv->r.note, r.note, r.inst);
		triggerramp = 1;
	}

	handleLocalMacros(fptr, spr, cv, r);

	ifMacro(fptr, spr, cv, r, 'p'); /* microtonal offset  */

	/* retrigger macros */
	if (!ifMacro(fptr, spr, cv, r, 'q')
			&& !ifMacro(fptr, spr, cv, r, 'Q')
			&& !ifMacro(fptr, spr, cv, r, 'r'))
		ifMacro(fptr, spr, cv, r, 'R');

	if (cv->rtrigsamples && cv->rtrigblocksize == -2)
	{ /* clean up if the last row had an altRxx and this row doesn't */
		cv->data.rtrig_rev = 0;
		cv->rtrigsamples = 0;
	}


	if (cv->pointer && ifMacroRamp(cv, r))
		triggerramp = 1;

	if (triggerramp && instrumentSafe(p->s->instrument, cv->r.inst))
		ramp(&oldcv, 0.0f, p->s->instrument->i[cv->r.inst]);
}

void postSampler(jack_nframes_t fptr, Track *cv, float rp, float lf, float rf)
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

void playTrackLookback(jack_nframes_t fptr, uint16_t *spr, Track *cv)
{
	uint16_t sprs = 0; /* spr start    */
	uint16_t sprp;     /* spr progress */

	if (cv->delaysamples > cv->cutsamples)
	{ /* delay is set and takes priority over cut */
		sprs = cv->delaysamples;

		triggerNote(fptr, cv, cv->r.note, cv->delaynote, cv->delayinst);
		cv->delaysamples = 0;

		handleLocalMacros(fptr, spr, cv, cv->r);
		cv->envgain = cv->modenvgain = 0.0f; /* TODO: necessary? */ /* not perfect but a safe enough assumption */
	} else if (cv->cutsamples)
	{ /* cut is set and takes priority over delay */
		triggerNote(fptr, cv, cv->r.note, NOTE_OFF, cv->r.inst);
		cv->cutsamples = 0;
		cv->envgain = cv->modenvgain = 0.0f; /* immediately cut the envelopes, not perfect but a safe enough assumption */
		return;
	}

	for (sprp = sprs; sprp < *spr; sprp++)
	{
		if (cv->portamentosamplepointer == cv->portamentosamples)
		{
			cv->portamentofinetune = cv->targetportamentofinetune;
			cv->portamentosamplepointer++;
		} else if (cv->portamentosamplepointer < cv->portamentosamples)
		{
			cv->portamentofinetune = cv->startportamentofinetune +
				(cv->targetportamentofinetune - cv->startportamentofinetune) * (float)cv->portamentosamplepointer/(float)cv->portamentosamples;

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
		}
	}

	/* process the sampler */
	if (instrumentSafe(p->s->instrument, cv->r.inst)
			&& cv->r.note != NOTE_VOID && cv->r.inst != INST_VOID)
	{
		Instrument *iv = &p->s->instrument->v[p->s->instrument->i[cv->r.inst]];
		Envelope env, wtenv;
		env.adsr = iv->envelope; if (cv->localenvelope != -1) env.adsr = cv->localenvelope;
		applyEnvelopeControlChanges(&env);
		wtenv.adsr = iv->wavetable.envelope;
		applyEnvelopeControlChanges(&wtenv);
		env.pointer = wtenv.pointer = cv->pointer;
		env.output = cv->envgain;
		wtenv.output = cv->modenvgain;
		env.release = wtenv.release = cv->data.release;
		for (sprp = 0; sprp < *spr - sprs; sprp++)
		{
			envelope(&env);
			envelope(&wtenv);
		}
		cv->envgain = env.output;
		cv->modenvgain = wtenv.output;

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
void playTrack(jack_nframes_t fptr, uint16_t *spr, jack_nframes_t sprp, Track *cv)
{
	short li = 0;
	short ri = 0;

	uint32_t delta;
	float multiplier;

	float rowprogress = (float)sprp / (float)(*spr);

	if (cv->cutsamples && sprp > cv->cutsamples)
	{
		if (instrumentSafe(p->s->instrument, cv->r.inst))
			ramp(cv, rowprogress, p->s->instrument->i[cv->r.inst]);
		triggerNote(fptr, cv, cv->r.note, NOTE_OFF, cv->r.inst);
		cv->cutsamples = 0;
	}
	if (cv->delaysamples && sprp > cv->delaysamples)
	{
		if (instrumentSafe(p->s->instrument, cv->delayinst))
			ramp(cv, rowprogress, p->s->instrument->i[cv->delayinst]);
		triggerNote(fptr, cv, cv->r.note, cv->delaynote, cv->delayinst);
		cv->delaysamples = 0;

		handleLocalMacros(fptr, spr, cv, cv->r);
	}

	cv->finetune = cv->microtonalfinetune;
	if (cv->portamentosamplepointer == cv->portamentosamples)
	{
		cv->portamentofinetune = cv->targetportamentofinetune;
		cv->portamentosamplepointer++;
		cv->r.note += cv->targetportamentofinetune;
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
			if (!ifMacro(fptr, spr, cv, cv->r, 'V'))
				cv->vibratosamples = 0;
		}
	}

	float lf = 0.0f;
	float rf = 0.0f;
	float samplegain;
	if (cv->file)
	{
		if (!cv->data.mute && p->w->previewsample && cv->r.note != NOTE_VOID)
		{
			processMinimal(p->w->previewsample, cv->pointer, 0xff, 0xf, cv->r.note, &li, &ri);
			cv->pointer++;
			cv->output[0][fptr] = li*DIVSHRT;
			cv->output[1][fptr] = ri*DIVSHRT;
		} else
		{
			cv->output[0][fptr] = 0.0f;
			cv->output[1][fptr] = 0.0f;
		}
	} else
	{
		if (instrumentSafe(p->s->instrument, cv->r.inst))
		{
			/* process the sampler */
			if (cv->r.inst != INST_VOID && cv->r.note != NOTE_VOID)
			{
				if (cv->rtrigsamples)
				{
					uint32_t rtrigoffset = (cv->pointer - cv->rtrigpointer) % cv->rtrigsamples;
					if (!rtrigoffset)
					{ /* first sample of any retrigger */
						if (cv->pointer > cv->rtrigpointer) /* first sample of any retrigger but the first */
						{
							triggerMidi(fptr, cv, cv->r.note, cv->r.note, cv->r.inst);
							cv->rtrigcurrentpitchedpointer = cv->pitchedpointer;
							cv->rtrigcurrentpointer = cv->pointer;
						}
					}
					if (cv->data.rtrig_rev)
					{
						uint32_t pointer, pitchedpointer;
						if (cv->rtrigpointer > cv->pointer - cv->rtrigcurrentpointer)
							pointer = cv->rtrigpointer - (cv->pointer - cv->rtrigcurrentpointer);
						else pointer = 0;
						if (cv->rtrigpitchedpointer > cv->pitchedpointer - cv->rtrigcurrentpitchedpointer)
							pitchedpointer = cv->rtrigcurrentpitchedpointer - (cv->pitchedpointer - cv->rtrigcurrentpitchedpointer);
						else pitchedpointer = 0;
						samplerProcess(p->s->instrument->i[cv->r.inst], cv, rowprogress, pointer, pitchedpointer, &li, &ri);
					} else samplerProcess(p->s->instrument->i[cv->r.inst], cv, rowprogress, cv->rtrigpointer + (cv->pointer - cv->rtrigcurrentpointer), cv->rtrigpitchedpointer + (cv->pitchedpointer - cv->rtrigcurrentpitchedpointer), &li, &ri);
				} else samplerProcess(p->s->instrument->i[cv->r.inst], cv, rowprogress, cv->pointer, cv->pitchedpointer, &li, &ri);

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

				if (instrumentSafe(p->s->instrument, cv->r.inst))
				{
					samplegain = powf(2, (float)p->s->instrument->v[p->s->instrument->i[cv->r.inst]].gain*DIV16);
					lf *= samplegain * gain;
					rf *= samplegain * gain;
				}

				lf += cv->rampbuffer[cv->rampindex*2 + 0] * (1.0f - gain);
				rf += cv->rampbuffer[cv->rampindex*2 + 1] * (1.0f - gain);

				postSampler(fptr, cv, rowprogress, lf, rf);
			} else cv->output[0][fptr] = cv->output[1][fptr] = 0.0f;

			cv->rampindex++;
		} else if (!cv->data.mute && instrumentSafe(p->s->instrument, cv->r.inst))
		{
			samplegain = powf(2, (float)p->s->instrument->v[p->s->instrument->i[cv->r.inst]].gain*DIV16);
			lf *= samplegain;
			rf *= samplegain;

			postSampler(fptr, cv, rowprogress, lf, rf);
		} else cv->output[0][fptr] = cv->output[1][fptr] = 0.0f;
	}
}

void lookback(jack_nframes_t fptr, uint16_t *spr, uint16_t playfy, Track *cv)
{
	clearTrackRuntime(cv);

	/* for every row before the current one */
	Row *r;
	for (uint16_t i = 0; i < playfy; i++)
	{
		/* scope lookback notes within the most recent vtrig */
		if (cv->data.variant->trig[i].index != VARIANT_VOID) { cv->r.note = NOTE_VOID; cv->envgain = 0.0f; }

		r = getTrackRow(&cv->data, i);
		if (p->s->bpmcachelen > i && p->s->bpmcache[i] != -1) macroBpm(fptr, spr, p->s->bpmcache[i], cv, *r);
		processRow(fptr, spr, 0, cv, *r);
		playTrackLookback(fptr, spr, cv);
	}
}

static void _trackThreadRoutine(Track *cv, uint16_t *spr, uint16_t *sprp, uint16_t *playfy, bool readrows)
{
	Row *r;
	for (jack_nframes_t fptr = 0; fptr < buffersize; fptr++) /* TODO: probably shouldn't rely on buffersize here */
	{
		playTrack(fptr, spr, *sprp, cv);

		/* next row */
		if ((*sprp)++ > *spr)
		{
			*sprp = 0;
			if (readrows && p->s->playing)
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
					if (p->w->instrumentrecv == INST_REC_LOCK_CUE_CONT) p->w->instrumentrecv = INST_REC_LOCK_END;
					if (p->w->instrumentrecv == INST_REC_LOCK_CUE_START) p->w->instrumentrecv = INST_REC_LOCK_CUE_CONT;
				} else if (!p->s->loop[1] && *playfy >= p->s->songlen)
				{
					*playfy = STATE_ROWS;
					lookback(fptr, spr, *playfy, cv);
					if (p->w->instrumentrecv == INST_REC_LOCK_CUE_CONT) p->w->instrumentrecv = INST_REC_LOCK_END;
					if (p->w->instrumentrecv == INST_REC_LOCK_CUE_START) p->w->instrumentrecv = INST_REC_LOCK_CUE_CONT;
				}

				/* preprocess track */
				r = getTrackRow(&cv->data, *playfy);
				if (p->s->bpmcachelen > *playfy && p->s->bpmcache[*playfy] != -1) macroBpm(fptr, spr, p->s->bpmcache[*playfy], cv, *r);
				processRow(fptr, spr, 1, cv, *r);
			}
		}
	}

	/* track insert effects */
	if (cv->data.effect) /* previewtrack doesn't have any effects so this check is required */
		for (uint8_t i = 0; i < cv->data.effect->c; i++)
			runEffect(buffersize, cv->data.effect, &cv->data.effect->v[i]);
}

static void *trackThreadRoutine(void *arg) /* wrapper for some temp variables */
{
	uint16_t spr = p->s->spr;
	uint16_t sprp = p->s->sprp;
	uint16_t playfy = p->s->playfy;
	_trackThreadRoutine(arg, &spr, &sprp, &playfy, 1);
	return NULL;
}
static void *previewTrackThreadRoutine(void *arg) /* don't try to read rows that don't exist */
{
	uint16_t spr = p->s->spr;
	uint16_t sprp = p->s->sprp;
	uint16_t playfy = p->s->playfy;
	_trackThreadRoutine(arg, &spr, &sprp, &playfy, 0);
	return NULL;
}

static void triggerFlash(PlaybackInfo *p)
{
	/* triggerflash animations */
	for (int i = 0; i < p->s->instrument->c; i++)
		if (p->s->instrument->v[i].triggerflash)
		{
			if (p->s->instrument->v[i].triggerflash == 1) p->redraw = 1;
			p->s->instrument->v[i].triggerflash--;
		}

	for (int i = 0; i < p->s->track->c; i++)
		if (p->s->track->v[i].triggerflash)
		{
			if (p->s->track->v[i].triggerflash == 1) p->redraw = 1;
			p->s->track->v[i].triggerflash--;
		}
}

void *dummyProcess(PlaybackInfo *p)
{
	struct timespec req;

	while (1)
	{
		if (processM_SEM()) return 0;

		triggerFlash(p);

		req.tv_sec  = 0; /* nanosleep can set this higher sometimes, so set every cycle */
		req.tv_nsec = UPDATE_DELAY;
		while(nanosleep(&req, &req) < 0);
	}
}

int process(jack_nframes_t nfptr, PlaybackInfo *p)
{
	uint8_t i;

	pb.in.l =    jack_port_get_buffer(p->in.l,    nfptr);
	pb.in.r =    jack_port_get_buffer(p->in.r,    nfptr);
	pb.out.l =   jack_port_get_buffer(p->out.l,   nfptr);
	pb.out.r =   jack_port_get_buffer(p->out.r,   nfptr);
	pb.midiout = jack_port_get_buffer(p->midiout, nfptr); jack_midi_clear_buffer(pb.midiout);

	if (processM_SEM()) return 0;

	/* TODO: should be events */
	/* will no longer write to the record buffer */
	if (p->w->instrumentrecv == INST_REC_LOCK_PREP_END)    p->w->instrumentrecv = INST_REC_LOCK_END;
	if (p->w->instrumentrecv == INST_REC_LOCK_PREP_CANCEL) p->w->instrumentrecv = INST_REC_LOCK_CANCEL;
	/* start recording immediately if not cueing */
	if (p->w->instrumentrecv == INST_REC_LOCK_START)
	{
		p->w->instrumentrecv = INST_REC_LOCK_CONT;
		p->redraw = 1;
	}

	/*                        MULTITHREADING                         */
	/* isn't strictly realtime-safe, but *should* be ok (maybe)      */
	/* honestly half this file probably isn't strictly realtime-safe */
	bool preview_thread_failed[PREVIEW_TRACKS]; /* TODO: more descriptive name */
	memset(preview_thread_failed, 1, PREVIEW_TRACKS); /* set high by default, gcc probably makes this static or smth */
#ifndef NO_MULTITHREADING
	jack_native_thread_t preview_thread_ids   [PREVIEW_TRACKS];
	jack_native_thread_t thread_ids   [p->s->track->c-1]; /* index 0 is the preview track */
#endif

	/* handle the preview tracks first */
	for (i = 0; i < PREVIEW_TRACKS; i++)
	{
		if (p->w->previewtrack[i].r.note != NOTE_VOID
				&& p->w->previewtrack[i].r.inst != INST_VOID)
		{
#ifdef NO_MULTITHREADING
			previewTrackThreadRoutine(&p->w->previewtrack[i]);
#else
			preview_thread_failed[i] = 0; /* set low if the thread needs to be joined with */
			if (RUNNING_ON_VALGRIND)
				previewTrackThreadRoutine(&p->w->previewtrack[i]);
			else
				/* try spawning a thread, if it fails then try again */
				while (jack_client_create_thread(client, &preview_thread_ids[i],
						jack_client_real_time_priority(client), jack_is_realtime(client),
						previewTrackThreadRoutine, &p->w->previewtrack[i]));
#endif
		}
	}

	/* spawn threads for each track except for track 0 */
	for (i = 1; i < p->s->track->c; i++)
	{
#ifdef NO_MULTITHREADING
		trackThreadRoutine(&p->s->track->v[i]);
#else
		if (RUNNING_ON_VALGRIND)
			trackThreadRoutine(&p->s->track->v[i]);
		else
			/* try spawning a thread, if it fails then try again */
			while (jack_client_create_thread(client, &thread_ids[i-1],
					jack_client_real_time_priority(client), jack_is_realtime(client),
					trackThreadRoutine, &p->s->track->v[i]));
#endif
	}

	/* run track 0 in this thread */
	uint16_t c0spr = p->s->spr;
	uint16_t c0sprp = p->s->sprp;
	uint16_t c0playfy = p->s->playfy;
	if (p->s->track->c)
		_trackThreadRoutine(&p->s->track->v[0], &c0spr, &c0sprp, &c0playfy, 1);

#ifndef NO_MULTITHREADING
	if (!RUNNING_ON_VALGRIND)
	{
		/* join with the track threads */
		for (i = 1; i < p->s->track->c; i++)
			pthread_join(thread_ids[i-1], NULL);
		/* join with the prevew threads */
		for (i = 1; i < p->s->track->c; i++)
			if (!preview_thread_failed[i])
				pthread_join(preview_thread_ids[i], NULL);
	}
#endif

	/* apply the new sprp and playfy track0 calculated */
	if (c0playfy != p->s->playfy)
	{
		if (p->w->follow) p->w->trackerfy = c0playfy;
		p->redraw = 1;
	}
	p->s->playfy = c0playfy;
	p->s->sprp = c0sprp;
	p->s->spr = c0spr;

	/* clear the master and send chain ports */
	memset(p->s->masteroutput[0], 0, nfptr * sizeof(float));
	memset(p->s->masteroutput[1], 0, nfptr * sizeof(float));
	memset(p->s->sendoutput  [0], 0, nfptr * sizeof(float));
	memset(p->s->sendoutput  [1], 0, nfptr * sizeof(float));

	/* sum the output from each track thread */
	for (uint8_t c = 0; c < p->s->track->c; c++)
		if (p->s->track->v[c].output[0] && p->s->track->v[c].output[1])
			for (jack_nframes_t fptr = 0; fptr < nfptr; fptr++)
			{
				p->s->masteroutput[0][fptr] += p->s->track->v[c].output[0][fptr] * p->s->track->v[c].mainmult[0][fptr];
				p->s->masteroutput[1][fptr] += p->s->track->v[c].output[1][fptr] * p->s->track->v[c].mainmult[1][fptr];
				p->s->sendoutput  [0][fptr] += p->s->track->v[c].output[0][fptr] * p->s->track->v[c].sendmult[0][fptr];
				p->s->sendoutput  [1][fptr] += p->s->track->v[c].output[1][fptr] * p->s->track->v[c].sendmult[1][fptr];
			}

	/* send chain */
	for (i = 0; i < p->s->send->c; i++)
		runEffect(nfptr, p->s->send, &p->s->send->v[i]);
	/* mix the send chain output into the master input */
	for (jack_nframes_t fptr = 0; fptr < nfptr; fptr++)
	{
		p->s->masteroutput[0][fptr] += p->s->sendoutput[0][fptr];
		p->s->masteroutput[1][fptr] += p->s->sendoutput[1][fptr];
	}

	for (i = 0; i < PREVIEW_TRACKS; i++)
	{
		if (!preview_thread_failed[i])
			for (jack_nframes_t fptr = 0; fptr < nfptr; fptr++)
			{
				p->s->masteroutput[0][fptr] += p->w->previewtrack[i].output[0][fptr] * p->w->previewtrack[i].mainmult[0][fptr];
				p->s->masteroutput[1][fptr] += p->w->previewtrack[i].output[1][fptr] * p->w->previewtrack[i].mainmult[1][fptr];
			}
	}

	/* master chain */
	for (i = 0; i < p->s->master->c; i++)
		runEffect(nfptr, p->s->master, &p->s->master->v[i]);

	/* output */
	for (jack_nframes_t fptr = 0; fptr < nfptr; fptr++)
	{
		pb.out.l[fptr] = p->s->masteroutput[0][fptr];
		pb.out.r[fptr] = p->s->masteroutput[1][fptr];
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

	triggerFlash(p);

	return 0;
}
