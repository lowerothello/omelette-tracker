/* process macro .m if it's present in the row */
bool ifMacro(jack_nframes_t fptr, uint16_t *spr, Track *cv, Row r, char m)
{
	return ifMacroCallback(fptr, spr, cv, r, m, MACRO_CALLBACK(m));
}

/* ifMacro(), but run .callback instead of guessing */
bool ifMacroCallback(jack_nframes_t fptr, uint16_t *spr, Track *cv, Row r, char m, bool (*callback)(jack_nframes_t, uint16_t*, int, Track*, Row))
{
	char ret = 0;
	for (int i = 0; i <= cv->variant->macroc; i++)
		if (r.macro[i].c == m && MACRO_SET(r.macro[i].c))
			ret = callback(fptr, spr, r.macro[i].v, cv, r);
	return ret;
}

/* if the row needs to be ramped in based on the macros present */
bool ifMacroRamp(Track *cv, Row r)
{
	for (int i = 0; i <= cv->variant->macroc; i++)
		if (MACRO_RAMP(r.macro[i].c)) return 1;

	return 0;
}

static int swapCase(int x)
{
	if (islower(x)) return toupper(x);
	else            return tolower(x);
}

bool changeMacro(int input, char *dest)
{
	/* use the current value if input is '\0' */
	if (!input) input = *dest; /* dest is pre-swapped */
	else        input = swapCase(input);

	if (MACRO_SET(input))
		*dest = input;

	return 0;
}

void addMacroBinds(const char *prettyname, unsigned int state, void (*callback)(void*))
{
	addTooltipPrettyPrint(prettyname, state, "macro");
	for (size_t i = 0; i < 128; i++)
		if (MACRO_SET(i))
			addTooltipBind(MACRO_PRETTYNAME(i), state, swapCase(i), 0, callback, (void*)i);
}

static bool _macroVibrato(jack_nframes_t fptr, uint16_t *spr, int m, Track *cv, Row r)
{
	cv->vibrato = m&0xf;
	if (!cv->vibratosamples) /* reset the phase if starting */
		cv->vibratosamplepointer = 0;
	cv->vibratosamples = *spr / (((m>>4) + 1) / 16.0); /* use floats for slower lfo speeds */
	cv->vibratosamplepointer = MIN(cv->vibratosamplepointer, cv->vibratosamples - 1);
	return 1;
}
bool macroVibrato(jack_nframes_t fptr, uint16_t *spr, int m, Track *cv, Row r)
{
	return _macroVibrato(fptr, spr, m, cv, r);
}

bool macroBpm(jack_nframes_t fptr, uint16_t *spr, int m, Track *cv, Row r)
{
	if (m == 0) setBpm(spr, p->s->songbpm);
	else        setBpm(spr, MAX(32, m));
	return 0;
}

bool macroCut(jack_nframes_t fptr, uint16_t *spr, int m, Track *cv, Row r)
{
	if (!m)
	{ /* cut now */
		ramp(cv, 0.0f, p->s->instrument->i[cv->r.inst]); /* TODO: proper rowprogress */
		triggerNote(fptr, cv, cv->r.note, NOTE_OFF, cv->r.inst);
		cv->cutsamples = 0;
		return 1;
	} else /* cut later */
		cv->cutsamples = *spr * m*DIV256;
	return 0;
}

bool macroPortamento(jack_nframes_t fptr, uint16_t *spr, int m, Track *cv, Row r)
{
	if (cv->portamentosamplepointer > cv->portamentosamples)
	{
		cv->portamentosamples = (*spr * m)/16;
		cv->portamentosamplepointer = 0;
		cv->startportamentofinetune = cv->portamentofinetune;
		cv->targetportamentofinetune = (r.note - (cv->r.note + cv->portamentofinetune));
	} return 1;
}

bool macroDelay(jack_nframes_t fptr, uint16_t *spr, int m, Track *cv, Row r)
{
	if (!m) return 0;
	cv->delaysamples = *spr * m*DIV256;
	cv->delaynote = r.note;
	cv->delayinst = r.inst;
	return 1;
}

bool macroGain(jack_nframes_t fptr, uint16_t *spr, int m, Track *cv, Row r)
{
	cv->gain.base = cv->gain.rand = m;
	return 1;
}
bool macroSmoothGain(jack_nframes_t fptr, uint16_t *spr, int m, Track *cv, Row r)
{
	cv->gain.target = m;
	return 1;
}
bool macroGainJitter(jack_nframes_t fptr, uint16_t *spr, int m, Track *cv, Row r)
{
	signed char stereo = rand()%((m>>4)+1);
	cv->gain.rand =
		 (MAX(0, (cv->gain.base>>4)  - stereo - rand()%((m&0x0f)+1))<<4)
		+ MAX(0, (cv->gain.base&0xf) - stereo - rand()%((m&0x0f)+1));
	return 1;
}
bool macroSmoothGainJitter(jack_nframes_t fptr, uint16_t *spr, int m, Track *cv, Row r)
{
	cv->gain.target_rand = 1;
	signed char stereo = rand()%((m>>4)+1);
	cv->gain.target =
		 (MAX(0, (cv->gain.base>>4)  - stereo - rand()%((m&0x0f)+1))<<4)
		+ MAX(0, (cv->gain.base&0xf) - stereo - rand()%((m&0x0f)+1));
	return 1;
}

bool macroSend(jack_nframes_t fptr, uint16_t *spr, int m, Track *cv, Row r)
{
	cv->send.base = cv->send.rand = m;
	return 1;
}
bool macroSmoothSend(jack_nframes_t fptr, uint16_t *spr, int m, Track *cv, Row r)
{
	cv->send.target = m;
	return 1;
}
bool macroSendJitter(jack_nframes_t fptr, uint16_t *spr, int m, Track *cv, Row r)
{
	signed char stereo = rand()%((m>>4)+1);
	cv->send.rand =
		 (MAX(0, (cv->send.base>>4)  - stereo - rand()%((m&0x0f)+1))<<4)
		+ MAX(0, (cv->send.base&0xf) - stereo - rand()%((m&0x0f)+1));
	return 1;
}
bool macroSmoothSendJitter(jack_nframes_t fptr, uint16_t *spr, int m, Track *cv, Row r)
{
	cv->send.target_rand = 1;
	signed char stereo = rand()%((m>>4)+1);
	cv->send.target =
		 (MAX(0, (cv->send.base>>4)  - stereo - rand()%((m&0x0f)+1))<<4)
		+ MAX(0, (cv->send.base&0xf) - stereo - rand()%((m&0x0f)+1));
	return 1;
}

static bool _macroTickRetrig(jack_nframes_t fptr, uint16_t *spr, int m, Track *cv, Row r)
{
	if (m)
	{
		if (cv->rtrigblocksize >= 0)
		{ /* starting a new chain */
			cv->rtrigpointer = cv->rtrigcurrentpointer = cv->pointer;
			cv->rtrigpitchedpointer = cv->rtrigcurrentpitchedpointer = cv->pitchedpointer;
		}
		cv->rtrigblocksize = -1;
		cv->rtrigsamples = *spr*DIV256 * m;
		return 1;
	} return 0;
}
bool macroTickRetrig(jack_nframes_t fptr, uint16_t *spr, int m, Track *cv, Row r)
{
	cv->rtrig_rev = 0;
	return _macroTickRetrig(fptr, spr, m, cv, r);
}
bool macroReverseTickRetrig(jack_nframes_t fptr, uint16_t *spr, int m, Track *cv, Row r)
{
	cv->rtrig_rev = 1;
	return _macroTickRetrig(fptr, spr, m, cv, r);
}
bool macroBlockRetrig(jack_nframes_t fptr, uint16_t *spr, int m, Track *cv, Row r)
{
	cv->rtrigpointer = cv->rtrigcurrentpointer = cv->pointer;
	cv->rtrigpitchedpointer = cv->rtrigcurrentpitchedpointer = cv->pitchedpointer;
	cv->rtrigblocksize = m>>4;
	if (m&0xf) cv->rtrigsamples = *spr / (m&0xf);
	else       cv->rtrigsamples = *spr * (cv->rtrigblocksize+1);
	return 1;
}
bool macroReverseBlockRetrig(jack_nframes_t fptr, uint16_t *spr, int m, Track *cv, Row r)
{
	cv->rtrig_rev = 1;
	return macroBlockRetrig(fptr, spr, m, cv, r);
}

bool macroPitchOffset(jack_nframes_t fptr, uint16_t *spr, int m, Track *cv, Row r)
{ cv->microtonalfinetune = m*DIV255; return 0; }

bool macroRowChance(jack_nframes_t fptr, uint16_t *spr, int m, Track *cv, Row r) /* returns true to NOT play */
{
	if (rand()%256 > m) return 1;
	else                return 0;
}

// bool midicctargetc(jack_nframes_t fptr, uint16_t *spr, int m, Track *cv, Row r) { cv->midiccindex = m%128; return 1; }
// bool midipcc(jack_nframes_t fptr, uint16_t *spr, int m, Track *cv, Row r)
// {
// 	if (!cv->mute && p->s->instrument->i[(r.inst != INST_VOID) ? r.inst : cv->r.inst] < p->s->instrument->c)
// 	{
// 		Instrument *iv = &p->s->instrument->v[p->s->instrument->i[(r.inst != INST_VOID) ? r.inst : cv->r.inst]];
// 		if (iv->algorithm == INST_ALG_MIDI) midiPC(fptr, iv->midi.channel, m%128);
// 	} return 1;
// }
// bool midiccc(jack_nframes_t fptr, uint16_t *spr, int m, Track *cv, Row r)
// {
// 	cv->midicc = m%128;
// 	if (cv->midiccindex != -1 && !cv->mute && p->s->instrument->i[(r.inst != INST_VOID) ? r.inst : cv->r.inst] < p->s->instrument->c)
// 	{
// 		Instrument *iv = &p->s->instrument->v[p->s->instrument->i[(r.inst != INST_VOID) ? r.inst : cv->r.inst]];
// 		if (iv->algorithm == INST_ALG_MIDI) midiCC(fptr, iv->midi.channel, cv->midiccindex, cv->midicc);
// 	} return 1;
// }

static bool _macroOffset(jack_nframes_t fptr, uint16_t *spr, int m, Track *cv, Row r)
{
	if (p->s->instrument->i[cv->r.inst] < p->s->instrument->c)
	{
		Instrument *iv = &p->s->instrument->v[p->s->instrument->i[cv->r.inst]];
		if (cv->r.note != NOTE_VOID) /* if playing a note */
		{
			if (r.note == NOTE_VOID) /* if not changing note, explicit ramping needed */
				ramp(cv, 0.0f, p->s->instrument->i[cv->r.inst]);
			cv->pitchedpointer = (m*DIV256) * iv->trimlength * (float)samplerate / (float)iv->sample->rate;
		}
	} return 0;
}
bool macroOffset(jack_nframes_t fptr, uint16_t *spr, int m, Track *cv, Row r)
{
	cv->reverse = 0;
	return _macroOffset(fptr, spr, m, cv, r);
}
bool macroReverseOffset(jack_nframes_t fptr, uint16_t *spr, int m, Track *cv, Row r)
{
	cv->reverse = 1;
	if (m) return _macroOffset(fptr, spr, m, cv, r);
	return 0;
}
bool macroOffsetJitter(jack_nframes_t fptr, uint16_t *spr, int m, Track *cv, Row r)
{
	if (p->s->instrument->i[cv->r.inst] < p->s->instrument->c)
	{
		Instrument *iv = &p->s->instrument->v[p->s->instrument->i[cv->r.inst]];
		if (cv->r.note != NOTE_VOID) /* if playing a note */
		{
			if (r.note == NOTE_VOID) /* if not changing note, explicit ramping needed */
				ramp(cv, 0.0f, p->s->instrument->i[cv->r.inst]);
			if (m>>4 == (m&0xf)) /* both nibbles are the same */
				cv->pitchedpointer = (((m&0xf) + (rand()&0xf))*DIV256) * iv->trimlength * (float)samplerate / (float)iv->sample->rate;
			else
			{
				int min = MIN(m>>4, m&0xf);
				int max = MAX(m>>4, m&0xf);
				cv->pitchedpointer = (((min + rand()%(max - min +1))<<4)*DIV256) * iv->trimlength * (float)samplerate / (float)iv->sample->rate;
			}
		}
	} return 0;
}
/* TODO: should never reverse in place, kinda important cos this case ramps wrongly */
bool macroReverseOffsetJitter(jack_nframes_t fptr, uint16_t *spr, int m, Track *cv, Row r)
{
	cv->reverse = !cv->reverse;
	return macroOffsetJitter(fptr, spr, m, cv, r);
}

bool macroCutoff(jack_nframes_t fptr, uint16_t *spr, int m, Track *cv, Row r)
{
	cv->filter.cut[0] = cv->filter.randcut[0] =  m&0xf0;
	cv->filter.cut[1] = cv->filter.randcut[1] = (m&0x0f)<<4;
	return 1;
}
bool macroSmoothCutoff(jack_nframes_t fptr, uint16_t *spr, int m, Track *cv, Row r)
{
	cv->filter.targetcut[0] =  m&0xf0;
	cv->filter.targetcut[1] = (m&0x0f)<<4;
	return 1;
}
bool macroCutoffJitter(jack_nframes_t fptr, uint16_t *spr, int m, Track *cv, Row r)
{
	short stereo = rand()%((m&0xf0)+1);
	cv->filter.randcut[0] = MAX(0, cv->filter.cut[0] - stereo - rand()%(((m&0x0f)<<4)+1));
	cv->filter.randcut[1] = MAX(0, cv->filter.cut[1] - stereo - rand()%(((m&0x0f)<<4)+1));
	return 1;
}
bool macroSmoothCutoffJitter(jack_nframes_t fptr, uint16_t *spr, int m, Track *cv, Row r)
{
	cv->filter.targetcut_rand = 1;
	short stereo = rand()%((m&0xf0)+1);
	cv->filter.targetcut[0] = MAX(0, cv->filter.cut[0] - stereo - rand()%(((m&0x0f)<<4)+1));
	cv->filter.targetcut[1] = MAX(0, cv->filter.cut[1] - stereo - rand()%(((m&0x0f)<<4)+1));
	return 1;
}

bool macroResonance(jack_nframes_t fptr, uint16_t *spr, int m, Track *cv, Row r)
{
	cv->filter.res[0] = cv->filter.randres[0] =  m&0xf0;
	cv->filter.res[1] = cv->filter.randres[1] = (m&0x0f)<<4;
	return 1;
}
bool macroSmoothResonance(jack_nframes_t fptr, uint16_t *spr, int m, Track *cv, Row r)
{
	cv->filter.targetres[0] =  m&0xf0;
	cv->filter.targetres[1] = (m&0x0f)<<4;
	return 1;
}
bool macroResonanceJitter(jack_nframes_t fptr, uint16_t *spr, int m, Track *cv, Row r)
{
	short stereo = rand()%((m&0xf0)+1);
	cv->filter.randres[0] = MAX(0, cv->filter.res[0] - stereo - rand()%(((m&0x0f)<<4)+1));
	cv->filter.randres[1] = MAX(0, cv->filter.res[1] - stereo - rand()%(((m&0x0f)<<4)+1));
	return 1;
}
bool macroSmoothResonanceJitter(jack_nframes_t fptr, uint16_t *spr, int m, Track *cv, Row r)
{
	cv->filter.targetres_rand = 1;
	short stereo = rand()%((m&0xf0)+1);
	cv->filter.targetres[0] = MAX(0, cv->filter.res[0] - stereo - rand()%(((m&0x0f)<<4)+1));
	cv->filter.targetres[1] = MAX(0, cv->filter.res[1] - stereo - rand()%(((m&0x0f)<<4)+1));
	return 1;
}

bool macroFilterMode(jack_nframes_t fptr, uint16_t *spr, int m, Track *cv, Row r)
{
	cv->filter.mode[0] = (m&0x70)>>4; /* ignore the '8' bit */
	cv->filter.mode[1] =  m&0x07;     /* ignore the '8' bit */
	return 1;
}
bool macroSmoothFilterMode(jack_nframes_t fptr, uint16_t *spr, int m, Track *cv, Row r)
{
	cv->filter.targetmode[0] = (m&0x70)>>4; /* ignore the '8' bit */
	cv->filter.targetmode[1] =  m&0x07;     /* ignore the '8' bit */
	return 1;
}

bool macroLocalSamplerate(jack_nframes_t fptr, uint16_t *spr, int m, Track *cv, Row r)
{
	cv->localsamplerate = m;
	return 1;
}
bool macroSmoothLocalSamplerate(jack_nframes_t fptr, uint16_t *spr, int m, Track *cv, Row r)
{
	cv->targetlocalsamplerate = m;
	return 1;
}

bool macroLocalAttDec(jack_nframes_t fptr, uint16_t *spr, int m, Track *cv, Row r)
{
	cv->localenvelope = m;
	return 1;
}
bool macroLocalSusRel(jack_nframes_t fptr, uint16_t *spr, int m, Track *cv, Row r)
{
	cv->localsustain = m;
	return 1;
}
bool macroLocalPitchShift(jack_nframes_t fptr, uint16_t *spr, int m, Track *cv, Row r)
{
	cv->localpitchshift = m;
	return 1;
}
bool macroSmoothLocalPitchShift(jack_nframes_t fptr, uint16_t *spr, int m, Track *cv, Row r)
{
	cv->targetlocalpitchshift = m;
	return 1;
}
bool macroLocalPitchWidth(jack_nframes_t fptr, uint16_t *spr, int m, Track *cv, Row r)
{
	cv->localpitchwidth = m;
	return 1;
}
bool macroSmoothLocalPitchWidth(jack_nframes_t fptr, uint16_t *spr, int m, Track *cv, Row r)
{
	cv->targetlocalpitchwidth = m;
	return 1;
}
bool macroLocalCycleLengthHighByte(jack_nframes_t fptr, uint16_t *spr, int m, Track *cv, Row r)
{
	if (p->s->instrument->i[cv->r.inst] < p->s->instrument->c)
	{
		Instrument *iv = &p->s->instrument->v[p->s->instrument->i[cv->r.inst]];
		if (cv->localcyclelength == -1) cv->localcyclelength = iv->granular.cyclelength;
		cv->localcyclelength = (((uint16_t)cv->localcyclelength<<8)>>8) + (m<<8);
	}
	return 1;
}
bool macroLocalCycleLengthLowByte(jack_nframes_t fptr, uint16_t *spr, int m, Track *cv, Row r)
{
	if (p->s->instrument->i[cv->r.inst] < p->s->instrument->c)
	{
		Instrument *iv = &p->s->instrument->v[p->s->instrument->i[cv->r.inst]];
		if (cv->localcyclelength == -1) cv->localcyclelength = iv->granular.cyclelength;
		cv->localcyclelength = (((uint16_t)cv->localcyclelength>>8)<<8)+m;
	}
	return 1;
}
