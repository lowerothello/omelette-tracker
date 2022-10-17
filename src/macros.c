void changeMacro(int input, char *dest)
{
	switch (input)
	{
		case ';': *dest = ';'; break; /* MIDI CC target               */
		case '@': *dest = '@'; break; /* MIDI PC                      */
		case '.': *dest = '.'; break; /* MIDI CC                      */
		case '%': *dest = '%'; break; /* note chance                  */
		case 'b': *dest = 'B'; break; /* bpm                          */
		case 'c': *dest = 'C'; break; /* note cut                     */
		case 'd': *dest = 'D'; break; /* note delay                   */
		case 'e': *dest = 'E'; break; /* local envelope times         */
		case 'E': *dest = 'e'; break; /* local envelope mode          */
		case 'f': *dest = 'F'; break; /* filter                       */
		case 'F': *dest = 'f'; break; /* smooth filter                */
		case 'g': *dest = 'G'; break; /* stereo gain                  */
		case 'G': *dest = 'g'; break; /* smooth stereo gain           */
		case 'h': *dest = 'H'; break; /* local pitch shift            */
		case 'H': *dest = 'h'; break; /* smooth local pitch shift     */
		case 'i': *dest = 'I'; break; /* stereo gain jitter           */
		case 'I': *dest = 'i'; break; /* smooth stereo gain jitter    */
		case 'l': *dest = 'L'; break; /* local cycle length high byte */
		case 'L': *dest = 'l'; break; /* local cycle length low byte  */
		case 'm': *dest = 'M'; break; /* filter mode                  */
		case 'M': *dest = 'm'; break; /* smooth filter mode           */
		case 'o': *dest = 'O'; break; /* offset                       */
		case 'O': *dest = 'o'; break; /* backwards offset             */
		case 'p': *dest = 'P'; break; /* pitch slide                  */
		case 'P': *dest = 'p'; break; /* microtonal offset            */
		case 'q': *dest = 'Q'; break; /* retrigger                    */
		case 'Q': *dest = 'q'; break; /* backwards retrigger          */
		case 'r': *dest = 'R'; break; /* block retrigger              */
		case 'R': *dest = 'r'; break; /* backwards block retrigger    */
		case 's': *dest = 'S'; break; /* send                         */
		case 'S': *dest = 's'; break; /* smooth send                  */
		case 'u': *dest = 'U'; break; /* random offset                */
		case 'U': *dest = 'u'; break; /* random backwards offset      */
		case 'v': *dest = 'V'; break; /* vibrato                      */
		case 'w': *dest = 'W'; break; /* local pitch width            */
		case 'W': *dest = 'w'; break; /* smooth local pitch width     */
		case 'x': *dest = 'X'; break; /* local samplerate             */
		case 'X': *dest = 'x'; break; /* target local samplerate      */
		case 'z': *dest = 'Z'; break; /* filter resonance             */
		case 'Z': *dest = 'z'; break; /* smooth filter resonance      */
	}
}

void descMacro(char c, uint8_t v)
{
	char text[64];
	memset(&text, '\0', sizeof(char) * 64);
	switch (c)
	{
		case ';': strcpy(text, "MIDI CC TARGET"); break;
		case '@': strcpy(text, "MIDI PC"); break;
		case '.': strcpy(text, "MIDI CC"); break;
		case '%': strcpy(text, "ROW CHANCE"); break;
		case 'B': strcpy(text, "BPM"); break;
		case 'C': strcpy(text, "NOTE CUT"); break;
		case 'D': strcpy(text, "NOTE DELAY"); break;
		case 'E': strcpy(text, "LOCAL ENVELOPE TIMES"); break;
		case 'e': strcpy(text, "LOCAL ENVELOPE MODE"); break;
		case 'F': strcpy(text, "FILTER CUTOFF"); break;
		case 'f': strcpy(text, "SMOOTH FILTER CUTOFF"); break;
		case 'G': strcpy(text, "STEREO GAIN"); break;
		case 'g': strcpy(text, "SMOOTH STEREO GAIN"); break;
		case 'H': strcpy(text, "LOCAL PITCH SHIFT"); break;
		case 'h': strcpy(text, "SMOOTH LOCAL PITCH SHIFT"); break;
		case 'I': strcpy(text, "STEREO GAIN JITTER"); break;
		case 'i': strcpy(text, "SMOOTH STEREO GAIN JITTER"); break;
		case 'L': strcpy(text, "LOCAL CYCLELENGTH HIGH BYTE"); break;
		case 'l': strcpy(text, "LOCAL CYCLELENGTH LOW BYTE"); break;
		case 'M': strcpy(text, "FILTER MODE"); break;
		case 'm': strcpy(text, "SMOOTH FILTER MODE"); break;
		case 'O': strcpy(text, "SAMPLE OFFSET / WT POS"); break;
		case 'o': strcpy(text, "REVERSE SAMPLE OFFSET / SMOOTH WT POS"); break;
		case 'P': strcpy(text, "PORTAMENTO"); break;
		case 'p': strcpy(text, "FINE PITCH OFFSET"); break;
		case 'Q': strcpy(text, "ALTERNATE RETRIGGER"); break;
		case 'q': strcpy(text, "REVERSE ALTERNATE RETRIGGER"); break;
		case 'R': strcpy(text, "BLOCK RETRIGGER"); break;
		case 'r': strcpy(text, "REVERSE BLOCK RETRIGGER"); break;
		case 'S': strcpy(text, "STEREO SEND GAIN"); break;
		case 's': strcpy(text, "SMOOTH STEREO SEND GAIN"); break;
		case 'U': strcpy(text, "RANDOM SAMPLE OFFSET / WT POS JITTER"); break;
		case 'u': strcpy(text, "RANDOM REVERSE SAMPLE OFFSET / SMOOTH WT POS JITTER"); break;
		case 'V': strcpy(text, "VIBRATO"); break;
		case 'W': strcpy(text, "LOCAL PITCH WIDTH"); break;
		case 'w': strcpy(text, "SMOOTH LOCAL PITCH WIDTH"); break;
		case 'X': strcpy(text, "LOCAL SAMPLERATE"); break;
		case 'x': strcpy(text, "SMOOTH LOCAL SAMPLERATE"); break;
		case 'Z': strcpy(text, "FILTER RESONANCE"); break;
		case 'z': strcpy(text, "SMOOTH FILTER RESONANCE"); break;
	}
	if (text[0] != '\0')
		printf("\033[%d;%ldH%s", ws.ws_row, (ws.ws_col - strlen(text)) / 2, text);
}


char _Vc(jack_nframes_t fptr, uint16_t *spr, int m, Channel *cv, Row r)
{
	cv->vibrato = m&0xf;
	if (!cv->vibratosamples) /* reset the phase if starting */
		cv->vibratosamplepointer = 0;
	cv->vibratosamples = *spr / (((m>>4) + 1) / 16.0); /* use floats for slower lfo speeds */
	cv->vibratosamplepointer = MIN(cv->vibratosamplepointer, cv->vibratosamples - 1);
	return 1;
}
char Vc(jack_nframes_t fptr, uint16_t *spr, int m, Channel *cv, Row r)
{ return _Vc(fptr, spr, m, cv, r); }

char Bc(jack_nframes_t fptr, uint16_t *spr, int m, Channel *cv, Row r)
{
	if (m == 0) setBpm(spr, p->s->songbpm);
	else        setBpm(spr, MAX(32, m));
	return 0;
}

char Cc(jack_nframes_t fptr, uint16_t *spr, int m, Channel *cv, Row r)
{
	if (!m>>4)
	{ /* cut now */
		ramp(cv, 0.0f, p->s->instrument->i[cv->samplerinst]); /* TODO: proper rowprogress */
		triggerNote(cv, NOTE_OFF, cv->r.inst);
		triggerMidi(fptr, cv, cv->r.note, NOTE_OFF, cv->r.inst);
		cv->cutsamples = 0;
		return 1;
	} else if ((m&0xf) != 0) /* cut later */
		cv->cutsamples = *spr * m*DIV256;
	return 0;
}

char Pc(jack_nframes_t fptr, uint16_t *spr, int m, Channel *cv, Row r)
{
	if (cv->portamentosamplepointer > cv->portamentosamples)
	{
		cv->portamentosamples = (*spr * m)/16;
		cv->portamentosamplepointer = 0;
		cv->startportamentofinetune = cv->portamentofinetune;
		cv->targetportamentofinetune = (r.note - (cv->r.note + cv->portamentofinetune));
	} return 1;
}

char Dc(jack_nframes_t fptr, uint16_t *spr, int m, Channel *cv, Row r)
{
	if (!(m&0xf)) return 0;
	cv->delaysamples = *spr * m*DIV256;
	cv->delaynote = r.note;
	if (r.inst == INST_VOID) cv->delayinst = cv->r.inst;
	else                     cv->delayinst = r.inst;
	return 1;
}

char Gc(jack_nframes_t fptr, uint16_t *spr, int m, Channel *cv, Row r) { cv->gain = cv->randgain = m; return 1; }
char gc(jack_nframes_t fptr, uint16_t *spr, int m, Channel *cv, Row r) { cv->targetgain = m; return 1; }

char Sc(jack_nframes_t fptr, uint16_t *spr, int m, Channel *cv, Row r) { cv->sendgain = cv->sendrandgain = m; return 1; }
char sc(jack_nframes_t fptr, uint16_t *spr, int m, Channel *cv, Row r) { cv->targetsendgain = m; return 1; }


char Ic(jack_nframes_t fptr, uint16_t *spr, int m, Channel *cv, Row r)
{
	signed char stereo = rand()%((m>>4)+1);
	cv->randgain =
		 (MAX(0, (cv->gain>>4)  - stereo - rand()%((m&0xf)+1))<<4)
		+ MAX(0, (cv->gain&0xf) - stereo - rand()%((m&0xf)+1));
	return 1;
}
char ic(jack_nframes_t fptr, uint16_t *spr, int m, Channel *cv, Row r)
{
	cv->data.target_rand = 1;
	signed char stereo = rand()%((m>>4)+1);
	cv->targetgain =
		 (MAX(0, (cv->gain>>4)  - stereo - rand()%((m&0xf)+1))<<4)
		+ MAX(0, (cv->gain&0xf) - stereo - rand()%((m&0xf)+1));
	return 1;
}

char _Qc(jack_nframes_t fptr, uint16_t *spr, int m, Channel *cv, Row r)
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
char Qc(jack_nframes_t fptr, uint16_t *spr, int m, Channel *cv, Row r)
{
	cv->data.rtrig_rev = 0;
	return _Qc(fptr, spr, m, cv, r);
}
char qc(jack_nframes_t fptr, uint16_t *spr, int m, Channel *cv, Row r)
{
	cv->data.rtrig_rev = 1;
	return _Qc(fptr, spr, m, cv, r);
}
char Rc(jack_nframes_t fptr, uint16_t *spr, int m, Channel *cv, Row r)
{
	cv->rtrigpointer = cv->rtrigcurrentpointer = cv->pointer;
	cv->rtrigpitchedpointer = cv->rtrigcurrentpitchedpointer = cv->pitchedpointer;
	cv->rtrigblocksize = m>>4;
	if (m&0xf) cv->rtrigsamples = *spr / (m&0xf);
	else      cv->rtrigsamples = *spr * (cv->rtrigblocksize+1);
	return 1;
}
char rc(jack_nframes_t fptr, uint16_t *spr, int m, Channel *cv, Row r)
{
	cv->data.rtrig_rev = 1;
	return Rc(fptr, spr, m, cv, r);
}

char pc(jack_nframes_t fptr, uint16_t *spr, int m, Channel *cv, Row r)
{ cv->microtonalfinetune = m*DIV255; return 0; }

char percentc(jack_nframes_t fptr, uint16_t *spr, int m, Channel *cv, Row r) /* returns true to NOT play */
{
	if (rand()%256 > m) return 1;
	else                return 0;
}

char midicctargetc(jack_nframes_t fptr, uint16_t *spr, int m, Channel *cv, Row r) { cv->midiccindex = m%128; return 1; }
char midipcc(jack_nframes_t fptr, uint16_t *spr, int m, Channel *cv, Row r)
{
	if (!cv->data.mute && p->s->instrument->i[(r.inst != INST_VOID) ? r.inst : cv->r.inst] < p->s->instrument->c)
	{
		Instrument *iv = &p->s->instrument->v[p->s->instrument->i[(r.inst != INST_VOID) ? r.inst : cv->r.inst]];
		if (iv->algorithm == INST_ALG_MIDI) midiPC(fptr, iv->midi.channel, m%128);
	} return 1;
}
char midiccc(jack_nframes_t fptr, uint16_t *spr, int m, Channel *cv, Row r)
{
	cv->midicc = m%128;
	if (cv->midiccindex != -1 && !cv->data.mute && p->s->instrument->i[(r.inst != INST_VOID) ? r.inst : cv->r.inst] < p->s->instrument->c)
	{
		Instrument *iv = &p->s->instrument->v[p->s->instrument->i[(r.inst != INST_VOID) ? r.inst : cv->r.inst]];
		if (iv->algorithm == INST_ALG_MIDI) midiCC(fptr, iv->midi.channel, cv->midiccindex, cv->midicc);
	} return 1;
}

char _Oc(jack_nframes_t fptr, uint16_t *spr, int m, Channel *cv, Row r)
{
	if (p->s->instrument->i[cv->r.inst] < p->s->instrument->c)
	{
		Instrument *iv = &p->s->instrument->v[p->s->instrument->i[cv->r.inst]];
		if (cv->r.note != NOTE_VOID) /* if playing a note */
		{
			if (r.note == NOTE_VOID) /* if not changing note, explicit ramping needed */
				ramp(cv, 0.0f, p->s->instrument->i[cv->samplerinst]);
			if (iv->granular.notestretch) cv->pitchedpointer = ((m*DIV256) * iv->trimlength * (float)samplerate / (float)iv->sample.rate) * powf(M_12_ROOT_2, (short)cv->samplernote - NOTE_C5);
			else                          cv->pitchedpointer = (m*DIV256) * iv->trimlength * (float)samplerate / (float)iv->sample.rate;
			cv->pointer = cv->pitchedpointer / powf(M_12_ROOT_2, (short)cv->samplernote - NOTE_C5);
		}
	} return 0;
}
char Oc(jack_nframes_t fptr, uint16_t *spr, int m, Channel *cv, Row r)
{
	cv->data.reverse = 0;
	return _Oc(fptr, spr, m, cv, r);
}
char oc(jack_nframes_t fptr, uint16_t *spr, int m, Channel *cv, Row r)
{
	cv->data.reverse = 1;
	if (m) return _Oc(fptr, spr, m, cv, r);
	return 0;
}
char Uc(jack_nframes_t fptr, uint16_t *spr, int m, Channel *cv, Row r)
{
	if (p->s->instrument->i[cv->r.inst] < p->s->instrument->c)
	{
		Instrument *iv = &p->s->instrument->v[p->s->instrument->i[cv->r.inst]];
		if (cv->r.note != NOTE_VOID) /* if playing a note */
		{
			if (r.note == NOTE_VOID) /* if not changing note, explicit ramping needed */
				ramp(cv, 0.0f, p->s->instrument->i[cv->samplerinst]);
			if (m>>4 == (m&0xf)) /* both nibbles are the same */
			{
				cv->pitchedpointer = (((m&0xf) + (rand()&0xf))*DIV256) * iv->trimlength * (float)samplerate / (float)iv->sample.rate;
				cv->pointer = cv->pitchedpointer / powf(M_12_ROOT_2, (short)cv->samplernote - NOTE_C5);
			} else
			{
				int min = MIN(m>>4, m&0xf);
				int max = MAX(m>>4, m&0xf);
				cv->pitchedpointer = (((min + rand()%(max - min +1))<<4)*DIV256) * iv->trimlength * (float)samplerate / (float)iv->sample.rate;
				cv->pointer = cv->pitchedpointer / powf(M_12_ROOT_2, (short)cv->samplernote - NOTE_C5);
			}
		}
	} return 0;
}
/* TODO: should never reverse in place, kinda important cos this case ramps wrongly */
char uc(jack_nframes_t fptr, uint16_t *spr, int m, Channel *cv, Row r)
{
	cv->data.reverse = !cv->data.reverse;
	return Uc(fptr, spr, m, cv, r);
}

char Fc(jack_nframes_t fptr, uint16_t *spr, int m, Channel *cv, Row r)
{
	cv->filtercut[0] =  m&0xf0;
	cv->filtercut[1] = (m&0x0f)<<4;
	return 1;
}
char fc(jack_nframes_t fptr, uint16_t *spr, int m, Channel *cv, Row r)
{
	cv->targetfiltercut[0] =  m&0xf0;
	cv->targetfiltercut[1] = (m&0x0f)<<4;
	return 1;
}
char Zc(jack_nframes_t fptr, uint16_t *spr, int m, Channel *cv, Row r)
{
	cv->filterres[0] =  m&0xf0;
	cv->filterres[1] = (m&0x0f)<<4;
	return 1;
}
char zc(jack_nframes_t fptr, uint16_t *spr, int m, Channel *cv, Row r)
{
	cv->targetfilterres[0] =  m&0xf0;
	cv->targetfilterres[1] = (m&0x0f)<<4;
	return 1;
}
char Mc(jack_nframes_t fptr, uint16_t *spr, int m, Channel *cv, Row r)
{
	cv->filtermode[0] = (m&0x70)>>4; /* ignore the '8' bit */
	cv->filtermode[1] =  m&0x07;     /* ignore the '8' bit */
	return 1;
}
char mc(jack_nframes_t fptr, uint16_t *spr, int m, Channel *cv, Row r)
{
	cv->targetfiltermode[0] = (m&0x70)>>4; /* ignore the '8' bit */
	cv->targetfiltermode[1] =  m&0x07;     /* ignore the '8' bit */
	return 1;
}

char Xc(jack_nframes_t fptr, uint16_t *spr, int m, Channel *cv, Row r) { cv->localsamplerate = m; return 1; }
char xc(jack_nframes_t fptr, uint16_t *spr, int m, Channel *cv, Row r) { cv->targetlocalsamplerate = m; return 1; }

char Ec(jack_nframes_t fptr, uint16_t *spr, int m, Channel *cv, Row r) { cv->localenvelope = m;         return 1; }
char ec(jack_nframes_t fptr, uint16_t *spr, int m, Channel *cv, Row r) { cv->localsustain = m;          return 1; }
char Hc(jack_nframes_t fptr, uint16_t *spr, int m, Channel *cv, Row r) { cv->localpitchshift = m;       return 1; }
char hc(jack_nframes_t fptr, uint16_t *spr, int m, Channel *cv, Row r) { cv->targetlocalpitchshift = m; return 1; }
char Wc(jack_nframes_t fptr, uint16_t *spr, int m, Channel *cv, Row r) { cv->localpitchwidth = m;       return 1; }
char wc(jack_nframes_t fptr, uint16_t *spr, int m, Channel *cv, Row r) { cv->targetlocalpitchwidth = m; return 1; }
char Lc(jack_nframes_t fptr, uint16_t *spr, int m, Channel *cv, Row r)
{
	if (p->s->instrument->i[cv->r.inst] < p->s->instrument->c)
	{
		Instrument *iv = &p->s->instrument->v[p->s->instrument->i[cv->r.inst]];
		if (cv->localcyclelength == -1) cv->localcyclelength = iv->granular.cyclelength;
		cv->localcyclelength = (((uint16_t)cv->localcyclelength<<8)>>8) + (m<<8);
	}
	return 1;
}
char lc(jack_nframes_t fptr, uint16_t *spr, int m, Channel *cv, Row r)
{
	if (p->s->instrument->i[cv->r.inst] < p->s->instrument->c)
	{
		Instrument *iv = &p->s->instrument->v[p->s->instrument->i[cv->r.inst]];
		if (cv->localcyclelength == -1) cv->localcyclelength = iv->granular.cyclelength;
		cv->localcyclelength = (((uint16_t)cv->localcyclelength>>8)<<8)+m;
	}
	return 1;
}
