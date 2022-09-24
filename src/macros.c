void changeMacro(int input, char *dest)
{
	if (isdigit(input)) *dest = input;
	else switch (input)
	{
		case ';': *dest = ';'; break; /* MIDI CC target               */
		case '@': *dest = '@'; break; /* MIDI PC                      */
		case '.': *dest = '.'; break; /* MIDI CC                      */
		case ',': *dest = ','; break; /* smooth MIDI CC               */
		case '%': *dest = '%'; break; /* note chance                  */
		case 'b': *dest = 'B'; break; /* bpm                          */
		case 'c': *dest = 'C'; break; /* note cut                     */
		case 'd': *dest = 'D'; break; /* note delay                   */
		case 'e': *dest = 'E'; break; /* local envelope               */
		case 'f': *dest = 'F'; break; /* filter                       */
		case 'F': *dest = 'f'; break; /* smooth filter                */
		case 'g': *dest = 'G'; break; /* gain                         */
		case 'G': *dest = 'g'; break; /* smooth gain                  */
		case 'h': *dest = 'H'; break; /* local pitch shift            */
		case 'i': *dest = 'I'; break; /* random gain                  */
		case 'I': *dest = 'i'; break; /* smooth random gain           */
		case 'l': *dest = 'L'; break; /* local cycle length high byte */
		case 'L': *dest = 'l'; break; /* local cycle length low byte  */
		case 'm': *dest = 'M'; break; /* microtonal offset            */
		case 'o': *dest = 'O'; break; /* offset                       */
		case 'O': *dest = 'o'; break; /* backwards offset             */
		case 'p': *dest = 'P'; break; /* pitch slide                  */
		case 'q': *dest = 'Q'; break; /* retrigger                    */
		case 'Q': *dest = 'q'; break; /* backwards retrigger          */
		case 'r': *dest = 'R'; break; /* block retrigger              */
		case 'R': *dest = 'r'; break; /* backwards block retrigger    */
		case 's': *dest = 'S'; break; /* send                         */
		case 'S': *dest = 's'; break; /* smooth send                  */
		case 'u': *dest = 'U'; break; /* random offset                */
		case 'U': *dest = 'u'; break; /* random backwards offset      */
		case 'v': *dest = 'V'; break; /* vibrato                      */
		case 'w': *dest = 'W'; break; /* waveshaper                   */
		case 'W': *dest = 'w'; break; /* smooth waveshaper            */
		case 'z': *dest = 'Z'; break; /* filter resonance             */
		case 'Z': *dest = 'z'; break; /* smooth filter resonance      */
	}
}

void descMacro(char c, uint8_t v)
{
	switch (c)
	{
		case ';': printf("\033[%d;%ldH%s", ws.ws_row, (ws.ws_col - strlen("MIDI CC TARGET")) / 2, "MIDI CC TARGET"); break;
		case '@': printf("\033[%d;%ldH%s", ws.ws_row, (ws.ws_col - strlen("MIDI PC")) / 2, "MIDI PC"); break;
		case '.': printf("\033[%d;%ldH%s", ws.ws_row, (ws.ws_col - strlen("MIDI CC")) / 2, "MIDI CC"); break;
		case ',': printf("\033[%d;%ldH%s", ws.ws_row, (ws.ws_col - strlen("SMOOTH MIDI CC")) / 2, "SMOOTH MIDI CC"); break;
		case '%': printf("\033[%d;%ldH%s", ws.ws_row, (ws.ws_col - strlen("CHANCE ROW")) / 2, "CHANCE ROW"); break;
		case 'B': printf("\033[%d;%ldH%s", ws.ws_row, (ws.ws_col - strlen("BPM")) / 2, "BPM"); break;
		case 'C': printf("\033[%d;%ldH%s", ws.ws_row, (ws.ws_col - strlen("NOTE CUT")) / 2, "NOTE CUT"); break;
		case 'D': printf("\033[%d;%ldH%s", ws.ws_row, (ws.ws_col - strlen("NOTE DELAY")) / 2, "NOTE DELAY"); break;
		case 'E': printf("\033[%d;%ldH%s", ws.ws_row, (ws.ws_col - strlen("LOCAL ENVELOPE TIME")) / 2, "LOCAL ENVELOPE TIME"); break;
		case 'F': printf("\033[%d;%ldH%s", ws.ws_row, (ws.ws_col - strlen("FILTER CUTOFF")) / 2, "FILTER CUTOFF"); break;
		case 'f': printf("\033[%d;%ldH%s", ws.ws_row, (ws.ws_col - strlen("SMOOTH FILTER CUTOFF")) / 2, "SMOOTH FILTER CUTOFF"); break;
		case 'G': printf("\033[%d;%ldH%s", ws.ws_row, (ws.ws_col - strlen("STEREO GAIN")) / 2, "STEREO GAIN"); break;
		case 'g': printf("\033[%d;%ldH%s", ws.ws_row, (ws.ws_col - strlen("SMOOTH STEREO GAIN")) / 2, "SMOOTH STEREO GAIN"); break;
		case 'H': printf("\033[%d;%ldH%s", ws.ws_row, (ws.ws_col - strlen("LOCAL PITCH SHIFT")) / 2, "LOCAL PITCH SHIFT"); break;
		case 'I': printf("\033[%d;%ldH%s", ws.ws_row, (ws.ws_col - strlen("CHANCE GAIN")) / 2, "CHANCE GAIN"); break;
		case 'i': printf("\033[%d;%ldH%s", ws.ws_row, (ws.ws_col - strlen("SMOOTH CHANCE GAIN")) / 2, "SMOOTH CHANCE GAIN"); break;
		case 'L': printf("\033[%d;%ldH%s", ws.ws_row, (ws.ws_col - strlen("LOCAL CYCLELENGTH HIGH BYTE")) / 2, "LOCAL CYCLELENGTH HIGH BYTE"); break;
		case 'l': printf("\033[%d;%ldH%s", ws.ws_row, (ws.ws_col - strlen("LOCAL CYCLELENGTH LOW BYTE")) / 2, "LOCAL CYCLELENGTH LOW BYTE"); break;
		case 'M': printf("\033[%d;%ldH%s", ws.ws_row, (ws.ws_col - strlen("MICROTONAL OFFSET")) / 2, "MICROTONAL OFFSET"); break;
		case 'O': printf("\033[%d;%ldH%s", ws.ws_row, (ws.ws_col - strlen("NOTE OFFSET")) / 2, "NOTE OFFSET"); break;
		case 'o': printf("\033[%d;%ldH%s", ws.ws_row, (ws.ws_col - strlen("BACKWARDS NOTE OFFSET")) / 2, "BACKWARDS NOTE OFFSET"); break;
		case 'P': printf("\033[%d;%ldH%s", ws.ws_row, (ws.ws_col - strlen("PITCH SLIDE")) / 2, "PITCH SLIDE"); break;
		case 'Q': printf("\033[%d;%ldH%s", ws.ws_row, (ws.ws_col - strlen("RETRIGGER")) / 2, "RETRIGGER"); break;
		case 'q': printf("\033[%d;%ldH%s", ws.ws_row, (ws.ws_col - strlen("BACKWARDS RETRIGGER")) / 2, "BACKWARDS RETRIGGER"); break;
		case 'R': printf("\033[%d;%ldH%s", ws.ws_row, (ws.ws_col - strlen("BLOCK RETRIGGER")) / 2, "BLOCK RETRIGGER"); break;
		case 'r': printf("\033[%d;%ldH%s", ws.ws_row, (ws.ws_col - strlen("BACKWARDS BLOCK RETRIGGER")) / 2, "BACKWARDS BLOCK RETRIGGER"); break;
		case 'S': printf("\033[%d;%ldH%s", ws.ws_row, (ws.ws_col - strlen("SEND")) / 2, "SEND"); break;
		case 's': printf("\033[%d;%ldH%s", ws.ws_row, (ws.ws_col - strlen("SMOOTH SEND")) / 2, "SMOOTH SEND"); break;
		case 'U': printf("\033[%d;%ldH%s", ws.ws_row, (ws.ws_col - strlen("RANDOM NOTE OFFSET")) / 2, "RANDOM NOTE OFFSET"); break;
		case 'u': printf("\033[%d;%ldH%s", ws.ws_row, (ws.ws_col - strlen("RANDOM BACKWARDS NOTE OFFSET")) / 2, "RANDOM BACKWARDS NOTE OFFSET"); break;
		case 'V': printf("\033[%d;%ldH%s", ws.ws_row, (ws.ws_col - strlen("VIBRATO")) / 2, "VIBRATO"); break;
		case 'W': /* waveshapers */
			switch (v>>4)
			{
				case 0: printf("\033[%d;%ldH%s", ws.ws_row, (ws.ws_col - strlen("HARD CLIP")) / 2, "HARD CLIP"); break;
				case 1: printf("\033[%d;%ldH%s", ws.ws_row, (ws.ws_col - strlen("SOFT CLIP")) / 2, "SOFT CLIP"); break;
				case 2: printf("\033[%d;%ldH%s", ws.ws_row, (ws.ws_col - strlen("RECTIFY")) / 2, "RECTIFY"); break;
				case 3: printf("\033[%d;%ldH%s", ws.ws_row, (ws.ws_col - strlen("RECTIFYx2")) / 2, "RECTIFYx2"); break;
				case 4: printf("\033[%d;%ldH%s", ws.ws_row, (ws.ws_col - strlen("WAVEFOLD")) / 2, "WAVEFOLD"); break;
				case 5: printf("\033[%d;%ldH%s", ws.ws_row, (ws.ws_col - strlen("WAVEWRAP")) / 2, "WAVEWRAP"); break;
				case 6: printf("\033[%d;%ldH%s", ws.ws_row, (ws.ws_col - strlen("SIGN CONVERSION")) / 2, "SIGN CONVERSION"); break;
				case 7: printf("\033[%d;%ldH%s", ws.ws_row, (ws.ws_col - strlen("HARD GATE")) / 2, "HARD GATE"); break;
			}
			break;
		case 'w': /* smooth waveshapers */
			switch (v>>4)
			{
				case 0: printf("\033[%d;%ldH%s", ws.ws_row, (ws.ws_col - strlen("SMOOTH HARD CLIP")) / 2, "SMOOTH HARD CLIP"); break;
				case 1: printf("\033[%d;%ldH%s", ws.ws_row, (ws.ws_col - strlen("SMOOTH SOFT CLIP")) / 2, "SMOOTH SOFT CLIP"); break;
				case 2: printf("\033[%d;%ldH%s", ws.ws_row, (ws.ws_col - strlen("SMOOTH RECTIFY")) / 2, "SMOOTH RECTIFY"); break;
				case 3: printf("\033[%d;%ldH%s", ws.ws_row, (ws.ws_col - strlen("SMOOTH RECTIFYx2")) / 2, "SMOOTH RECTIFYx2"); break;
				case 4: printf("\033[%d;%ldH%s", ws.ws_row, (ws.ws_col - strlen("SMOOTH WAVEFOLD")) / 2, "SMOOTH WAVEFOLD"); break;
				case 5: printf("\033[%d;%ldH%s", ws.ws_row, (ws.ws_col - strlen("SMOOTH WAVEWRAP")) / 2, "SMOOTH WAVEWRAP"); break;
				case 6: printf("\033[%d;%ldH%s", ws.ws_row, (ws.ws_col - strlen("SMOOTH SIGN CONVERSION")) / 2, "SMOOTH SIGN CONVERSION"); break;
				case 7: printf("\033[%d;%ldH%s", ws.ws_row, (ws.ws_col - strlen("SMOOTH HARD GATE")) / 2, "SMOOTH HARD GATE"); break;
			}
			break;
		case 'Z': printf("\033[%d;%ldH%s", ws.ws_row, (ws.ws_col - strlen("FILTER RESONANCE")) / 2, "FILTER RESONANCE"); break;
		case 'z': printf("\033[%d;%ldH%s", ws.ws_row, (ws.ws_col - strlen("SMOOTH FILTER RESONANCE")) / 2, "SMOOTH FILTER RESONANCE"); break;
	}
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
		ramp(cv, p->s->instrumenti[cv->samplerinst]);
		triggerNote(cv, NOTE_OFF, cv->r.inst);
		triggerMidi(fptr, cv, cv->r.note, NOTE_OFF, cv->r.inst);
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

char Gc(jack_nframes_t fptr, int m, channel *cv, row r)
{
	cv->gain = cv->randgain = m;
	return 1;
}
char gc(jack_nframes_t fptr, int m, channel *cv, row r)
{
	cv->targetgain = m;
	return 1;
}
char Ic(jack_nframes_t fptr, int m, channel *cv, row r)
{
	signed char stereo = rand()%((m>>4)+1);
	cv->randgain =
		 (MAX(0, (cv->gain>>4) - stereo - rand()%((m%16)+1))<<4)
		+ MAX(0, (cv->gain%16) - stereo - rand()%((m%16)+1));
	return 1;
}
char ic(jack_nframes_t fptr, int m, channel *cv, row r)
{
	if (!(cv->data.flags&C_FLAG_TARGET_RAND)) cv->data.flags ^= C_FLAG_TARGET_RAND;
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
		if (!(cv->data.flags&C_FLAG_RTRIG_REV)) cv->data.flags ^= C_FLAG_RTRIG_REV;
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
	if (!(cv->data.flags&C_FLAG_RTRIG_REV)) cv->data.flags ^= C_FLAG_RTRIG_REV;
	return Rc(fptr, m, cv, r);
}

char Wc(jack_nframes_t fptr, int m, channel *cv, row r) { cv->waveshaper = m>>4; cv->waveshaperstrength = m%16; return 1; }
char wc(jack_nframes_t fptr, int m, channel *cv, row r) { cv->waveshaper = m>>4; cv->targetwaveshaperstrength = m%16; return 1; }

char Mc(jack_nframes_t fptr, int m, channel *cv, row r)
{ cv->microtonalfinetune = m*DIV255; return 0; }

char percentc(jack_nframes_t fptr, int m, channel *cv, row r) /* returns true to NOT play */
{
	if (rand()%256 > m) return 1;
	else                return 0;
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

char midicctargetc(jack_nframes_t fptr, int m, channel *cv, row r) { cv->midiccindex = m%128; return 1; }
char midipcc(jack_nframes_t fptr, int m, channel *cv, row r)
{
	if (!(cv->data.flags&C_FLAG_MUTE) && p->s->instrumenti[(r.inst != INST_VOID) ? r.inst : cv->r.inst] < p->s->instrumentc)
	{
		instrument *iv = &p->s->instrumentv[p->s->instrumenti[(r.inst != INST_VOID) ? r.inst : cv->r.inst]];
		if (iv->midichannel != -1) midiPC(fptr, iv->midichannel, m%128);
	} return 1;
}
char midiccc(jack_nframes_t fptr, int m, channel *cv, row r)
{
	cv->midicc = m%128;
	if (cv->midiccindex != -1 && !(cv->data.flags&C_FLAG_MUTE) && p->s->instrumenti[(r.inst != INST_VOID) ? r.inst : cv->r.inst] < p->s->instrumentc)
	{
		instrument *iv = &p->s->instrumentv[p->s->instrumenti[(r.inst != INST_VOID) ? r.inst : cv->r.inst]];
		if (iv->midichannel != -1) midiCC(fptr, iv->midichannel, cv->midiccindex, cv->midicc);
	} return 1;
}
char smoothmidiccc(jack_nframes_t fptr, int m, channel *cv, row r)
{ cv->targetmidicc = m%128; return 1; }

char Oc(jack_nframes_t fptr, int m, channel *cv, row r)
{
	if (cv->r.inst != INST_VOID && p->s->instrumenti[cv->r.inst] < p->s->instrumentc)
	{
		instrument *iv = &p->s->instrumentv[p->s->instrumenti[cv->r.inst]];
		if (cv->r.note != NOTE_VOID) /* if playing a note */
		{
			if (r.note == NOTE_VOID) /* if not changing note, explicit ramping needed */
				ramp(cv, p->s->instrumenti[cv->samplerinst]);
			cv->pitchedpointer = (m*DIV255) * (iv->trim[1] - iv->trim[0]);
		}
	} return 0;
}
char oc(jack_nframes_t fptr, int m, channel *cv, row r)
{
	cv->data.flags ^= C_FLAG_REVERSE;
	if (m) return Oc(fptr, m, cv, r);
	return 0;
}
char Uc(jack_nframes_t fptr, int m, channel *cv, row r)
{
	if (cv->r.inst != INST_VOID && p->s->instrumenti[cv->r.inst] < p->s->instrumentc)
	{
		instrument *iv = &p->s->instrumentv[p->s->instrumenti[cv->r.inst]];
		if (cv->r.note != NOTE_VOID) /* if playing a note */
		{
			if (r.note == NOTE_VOID) /* if not changing note, explicit ramping needed */
				ramp(cv, p->s->instrumenti[cv->samplerinst]);
			if (m>>4 == m%16) /* both nibbles are the same */
				cv->pitchedpointer = ((((m>>4)<<4) + rand()%16)*DIV255) * (iv->trim[1] - iv->trim[0]);
			else
			{
				int min = MIN(m>>4, m%16);
				int max = MAX(m>>4, m%16);
				cv->pitchedpointer = (((min + rand()%(max - min +1))<<4)*DIV255) * (iv->trim[1] - iv->trim[0]);
			}
		}
	} return 0;
}
/* TODO: should never reverse in place, kinda important cos this case ramps wrongly */
char uc(jack_nframes_t fptr, int m, channel *cv, row r)
{
	cv->data.flags ^= C_FLAG_REVERSE;
	return Uc(fptr, m, cv, r);
}

char Fc(jack_nframes_t fptr, int m, channel *cv, row r) { cv->filtercut = m; return 1; }
char fc(jack_nframes_t fptr, int m, channel *cv, row r) { cv->targetfiltercut = m; return 1; }
char Zc(jack_nframes_t fptr, int m, channel *cv, row r)
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

char Ec(jack_nframes_t fptr, int m, channel *cv, row r) { cv->localenvelope = m; return 1; }
char Hc(jack_nframes_t fptr, int m, channel *cv, row r) { cv->localpitchshift = m; return 1; }
char Lc(jack_nframes_t fptr, int m, channel *cv, row r)
{
	if (p->s->instrumenti[cv->r.inst] < p->s->instrumentc)
	{
		instrument *iv = &p->s->instrumentv[p->s->instrumenti[cv->r.inst]];
		if (cv->localcyclelength == -1) cv->localcyclelength = iv->cyclelength;
		cv->localcyclelength = (((uint16_t)cv->localcyclelength<<8)>>8) + (m<<8);
	}
	return 1;
}
char lc(jack_nframes_t fptr, int m, channel *cv, row r)
{
	if (p->s->instrumenti[cv->r.inst] < p->s->instrumentc)
	{
		instrument *iv = &p->s->instrumentv[p->s->instrumenti[cv->r.inst]];
		if (cv->localcyclelength == -1) cv->localcyclelength = iv->cyclelength;
		cv->localcyclelength = (((uint16_t)cv->localcyclelength>>8)<<8)+m;
	}
	return 1;
}
