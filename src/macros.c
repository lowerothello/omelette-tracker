char ifMacro(jack_nframes_t fptr, uint16_t *spr, Track *cv, Row r, char m, bool alt, char (*callback)(jack_nframes_t, uint16_t *, int, Track *, Row))
{
	char ret = 0;
	for (int i = 0; i <= cv->data.variant->macroc; i++)
		if (r.macro[i].c == m && r.macro[i].alt == alt)
			ret = callback(fptr, spr, r.macro[i].v, cv, r);
	return ret;
}
bool linkMacroNibbles(char m)
{
	switch (m)
	{
		case 'G': case 'g':
			return 1;
	} return 0;
}

bool changeMacro(int input, char *dest, bool *destalt, bool targetalt)
{
	/* use the current value if input is NULL */
	if (!input)
	{
		if      (isupper(*dest)) input = *dest + 32;
		else if (islower(*dest)) input = *dest - 32;
		else                     input = *dest;
	}

	if (targetalt)
		switch (input)
		{
			case 'g': *dest = 'G'; *destalt = targetalt; return 1; /* stereo gain jitter        */
			case 'G': *dest = 'g'; *destalt = targetalt; return 1; /* smooth stereo gain jitter */
			case 's': *dest = 'S'; *destalt = targetalt; return 1; /* stereo send jitter        */
			case 'S': *dest = 's'; *destalt = targetalt; return 1; /* smooth stereo send jitter */
			case 'f': *dest = 'F'; *destalt = targetalt; return 1; /* filter jitter             */
			case 'F': *dest = 'f'; *destalt = targetalt; return 1; /* smooth filter jitter      */
			case 'z': *dest = 'Z'; *destalt = targetalt; return 1; /* filter resonance jitter   */
			case 'Z': *dest = 'z'; *destalt = targetalt; return 1; /* smooth filter res jitter  */
			case 'o': *dest = 'O'; *destalt = targetalt; return 1; /* random offset             */
			case 'O': *dest = 'o'; *destalt = targetalt; return 1; /* random backwards offset   */
			case 'r': *dest = 'R'; *destalt = targetalt; return 1; /* retrigger                 */
			case 'R': *dest = 'r'; *destalt = targetalt; return 1; /* backwards retrigger       */
		}
	else
		switch (input)
		{
			case ';': *dest = ';'; *destalt = targetalt; return 1; /* MIDI CC target               */
			case '@': *dest = '@'; *destalt = targetalt; return 1; /* MIDI PC                      */
			case '.': *dest = '.'; *destalt = targetalt; return 1; /* MIDI CC                      */
			case '%': *dest = '%'; *destalt = targetalt; return 1; /* note chance                  */
			case 'b': *dest = 'B'; *destalt = targetalt; return 1; /* bpm                          */
			case 'c': *dest = 'C'; *destalt = targetalt; return 1; /* note cut                     */
			case 'd': *dest = 'D'; *destalt = targetalt; return 1; /* note delay                   */
			case 'e': *dest = 'E'; *destalt = targetalt; return 1; /* local envelope times         */
			case 'E': *dest = 'e'; *destalt = targetalt; return 1; /* local envelope mode          */
			case 'f': *dest = 'F'; *destalt = targetalt; return 1; /* filter                       */
			case 'F': *dest = 'f'; *destalt = targetalt; return 1; /* smooth filter                */
			case 'g': *dest = 'G'; *destalt = targetalt; return 1; /* stereo gain                  */
			case 'G': *dest = 'g'; *destalt = targetalt; return 1; /* smooth stereo gain           */
			case 'h': *dest = 'H'; *destalt = targetalt; return 1; /* local pitch shift            */
			case 'H': *dest = 'h'; *destalt = targetalt; return 1; /* smooth local pitch shift     */
			case 'l': *dest = 'L'; *destalt = targetalt; return 1; /* local cycle length high byte */
			case 'L': *dest = 'l'; *destalt = targetalt; return 1; /* local cycle length low byte  */
			case 'm': *dest = 'M'; *destalt = targetalt; return 1; /* filter mode                  */
			case 'M': *dest = 'm'; *destalt = targetalt; return 1; /* smooth filter mode           */
			case 'o': *dest = 'O'; *destalt = targetalt; return 1; /* offset                       */
			case 'O': *dest = 'o'; *destalt = targetalt; return 1; /* backwards offset             */
			case 'p': *dest = 'P'; *destalt = targetalt; return 1; /* pitch slide                  */
			case 'P': *dest = 'p'; *destalt = targetalt; return 1; /* microtonal offset            */
			case 'r': *dest = 'R'; *destalt = targetalt; return 1; /* block retrigger              */
			case 'R': *dest = 'r'; *destalt = targetalt; return 1; /* backwards block retrigger    */
			case 's': *dest = 'S'; *destalt = targetalt; return 1; /* send                         */
			case 'S': *dest = 's'; *destalt = targetalt; return 1; /* smooth send                  */
			case 'v': *dest = 'V'; *destalt = targetalt; return 1; /* vibrato                      */
			case 'w': *dest = 'W'; *destalt = targetalt; return 1; /* local pitch width            */
			case 'W': *dest = 'w'; *destalt = targetalt; return 1; /* smooth local pitch width     */
			case 'x': *dest = 'X'; *destalt = targetalt; return 1; /* local samplerate             */
			case 'X': *dest = 'x'; *destalt = targetalt; return 1; /* target local samplerate      */
			case 'z': *dest = 'Z'; *destalt = targetalt; return 1; /* filter resonance             */
			case 'Z': *dest = 'z'; *destalt = targetalt; return 1; /* smooth filter resonance      */
		}
	return 0;
}
int changeMacroVtrig(int input, char *dest, bool *destalt, bool targetalt)
{
	/* use the current value if input is NULL */
	if (!input)
	{
		if      (isupper(*dest)) input = *dest + 32;
		else if (islower(*dest)) input = *dest - 32;
		else                     input = *dest;
	}

	if (targetalt)
		switch (input)
		{
			case 'g': *dest = 'G'; *destalt = targetalt; return 1; /* stereo gain jitter        */
			case 'G': *dest = 'g'; *destalt = targetalt; return 1; /* smooth stereo gain jitter */
			case 's': *dest = 'S'; *destalt = targetalt; return 1; /* stereo send jitter        */
			case 'S': *dest = 's'; *destalt = targetalt; return 1; /* smooth stereo send jitter */
			case 'f': *dest = 'F'; *destalt = targetalt; return 1; /* filter jitter             */
			case 'F': *dest = 'f'; *destalt = targetalt; return 1; /* smooth filter jitter      */
			case 'z': *dest = 'Z'; *destalt = targetalt; return 1; /* filter resonance jitter   */
			case 'Z': *dest = 'z'; *destalt = targetalt; return 1; /* smooth filter res jitter  */
			case 'o': *dest = 'O'; *destalt = targetalt; return 1; /* random offset             */
			case 'O': *dest = 'o'; *destalt = targetalt; return 1; /* random backwards offset   */
			case 'r': *dest = 'R'; *destalt = targetalt; return 1; /* retrigger                 */
			case 'R': *dest = 'r'; *destalt = targetalt; return 1; /* backwards retrigger       */
		}
	else
		switch (input)
		{
			case ';': *dest = ';'; *destalt = targetalt; return 1; /* MIDI CC target               */
			case '@': *dest = '@'; *destalt = targetalt; return 1; /* MIDI PC                      */
			case '.': *dest = '.'; *destalt = targetalt; return 1; /* MIDI CC                      */
			case '%': *dest = '%'; *destalt = targetalt; return 1; /* note chance                  */
			case 'b': *dest = 'B'; *destalt = targetalt; return 1; /* bpm                          */
			case 'c': *dest = 'C'; *destalt = targetalt; return 1; /* note cut                     */
			case 'd': *dest = 'D'; *destalt = targetalt; return 1; /* note delay                   */
			case 'e': *dest = 'E'; *destalt = targetalt; return 1; /* local envelope times         */
			case 'E': *dest = 'e'; *destalt = targetalt; return 1; /* local envelope mode          */
			case 'f': *dest = 'F'; *destalt = targetalt; return 1; /* filter                       */
			case 'F': *dest = 'f'; *destalt = targetalt; return 1; /* smooth filter                */
			case 'g': *dest = 'G'; *destalt = targetalt; return 1; /* stereo gain                  */
			case 'G': *dest = 'g'; *destalt = targetalt; return 1; /* smooth stereo gain           */
			case 'h': *dest = 'H'; *destalt = targetalt; return 1; /* local pitch shift            */
			case 'H': *dest = 'h'; *destalt = targetalt; return 1; /* smooth local pitch shift     */
			case 'l': *dest = 'L'; *destalt = targetalt; return 1; /* local cycle length high byte */
			case 'L': *dest = 'l'; *destalt = targetalt; return 1; /* local cycle length low byte  */
			case 'm': *dest = 'M'; *destalt = targetalt; return 1; /* filter mode                  */
			case 'M': *dest = 'm'; *destalt = targetalt; return 1; /* smooth filter mode           */
			case 'o': *dest = 'O'; *destalt = targetalt; return 1; /* offset                       */
			case 'O': *dest = 'o'; *destalt = targetalt; return 1; /* backwards offset             */
			case 'p': *dest = 'P'; *destalt = targetalt; return 1; /* pitch slide                  */
			case 'P': *dest = 'p'; *destalt = targetalt; return 1; /* microtonal offset            */
			case 'r': *dest = 'R'; *destalt = targetalt; return 1; /* block retrigger              */
			case 'R': *dest = 'r'; *destalt = targetalt; return 1; /* backwards block retrigger    */
			case 's': *dest = 'S'; *destalt = targetalt; return 1; /* send                         */
			case 'S': *dest = 's'; *destalt = targetalt; return 1; /* smooth send                  */
			case 'v': *dest = 'V'; *destalt = targetalt; return 1; /* vibrato                      */
			case 'w': *dest = 'W'; *destalt = targetalt; return 1; /* local pitch width            */
			case 'W': *dest = 'w'; *destalt = targetalt; return 1; /* smooth local pitch width     */
			case 'x': *dest = 'X'; *destalt = targetalt; return 1; /* local samplerate             */
			case 'X': *dest = 'x'; *destalt = targetalt; return 1; /* target local samplerate      */
			case 'z': *dest = 'Z'; *destalt = targetalt; return 1; /* filter resonance             */
			case 'Z': *dest = 'z'; *destalt = targetalt; return 1; /* smooth filter resonance      */
		}
	return 0;
}

void descMacro(char c, uint8_t v, bool alt)
{
	char text[64];
	memset(&text, '\0', sizeof(char) * 64);
	if (alt)
		switch (c)
		{
			case 'G': strcpy(text, "STEREO GAIN JITTER"); break;
			case 'g': strcpy(text, "SMOOTH STEREO GAIN JITTER"); break;
			case 'S': strcpy(text, "STEREO SEND JITTER"); break;
			case 's': strcpy(text, "SMOOTH STEREO SEND JITTER"); break;
			case 'F': strcpy(text, "FILTER CUTOFF JITTER"); break;
			case 'f': strcpy(text, "SMOOTH FILTER CUTOFF JITTER"); break;
			case 'Z': strcpy(text, "FILTER RESONANCE JITTER"); break;
			case 'z': strcpy(text, "SMOOTH FILTER RESONANCE JITTER"); break;
			case 'O': strcpy(text, "RANDOM SAMPLE OFFSET / WT POS JITTER"); break;
			case 'o': strcpy(text, "RANDOM REVERSE SAMPLE OFFSET / SMOOTH WT POS JITTER"); break;
			case 'R': strcpy(text, "ALTERNATE RETRIGGER"); break;
			case 'r': strcpy(text, "REVERSE ALTERNATE RETRIGGER"); break;
		}
	else
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
			case 'L': strcpy(text, "LOCAL CYCLELENGTH HIGH BYTE"); break;
			case 'l': strcpy(text, "LOCAL CYCLELENGTH LOW BYTE"); break;
			case 'M': strcpy(text, "FILTER MODE"); break;
			case 'm': strcpy(text, "SMOOTH FILTER MODE"); break;
			case 'O': strcpy(text, "SAMPLE OFFSET / WT POS"); break;
			case 'o': strcpy(text, "REVERSE SAMPLE OFFSET / SMOOTH WT POS"); break;
			case 'P': strcpy(text, "PORTAMENTO"); break;
			case 'p': strcpy(text, "FINE PITCH OFFSET"); break;
			case 'R': strcpy(text, "BLOCK RETRIGGER"); break;
			case 'r': strcpy(text, "REVERSE BLOCK RETRIGGER"); break;
			case 'S': strcpy(text, "STEREO SEND"); break;
			case 's': strcpy(text, "SMOOTH STEREO SEND"); break;
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


char _Vc(jack_nframes_t fptr, uint16_t *spr, int m, Track *cv, Row r)
{
	cv->vibrato = m&0xf;
	if (!cv->vibratosamples) /* reset the phase if starting */
		cv->vibratosamplepointer = 0;
	cv->vibratosamples = *spr / (((m>>4) + 1) / 16.0); /* use floats for slower lfo speeds */
	cv->vibratosamplepointer = MIN(cv->vibratosamplepointer, cv->vibratosamples - 1);
	return 1;
}
char Vc(jack_nframes_t fptr, uint16_t *spr, int m, Track *cv, Row r)
{ return _Vc(fptr, spr, m, cv, r); }

char Bc(jack_nframes_t fptr, uint16_t *spr, int m, Track *cv, Row r)
{
	if (m == 0) setBpm(spr, p->s->songbpm);
	else        setBpm(spr, MAX(32, m));
	return 0;
}

char Cc(jack_nframes_t fptr, uint16_t *spr, int m, Track *cv, Row r)
{
	if (!m)
	{ /* cut now */
		ramp(cv, 0.0f, p->s->instrument->i[cv->samplerinst]); /* TODO: proper rowprogress */
		triggerNote(fptr, cv, cv->r.note, NOTE_OFF, cv->r.inst);
		cv->cutsamples = 0;
		return 1;
	} else /* cut later */
		cv->cutsamples = *spr * m*DIV256;
	return 0;
}

char Pc(jack_nframes_t fptr, uint16_t *spr, int m, Track *cv, Row r)
{
	if (cv->portamentosamplepointer > cv->portamentosamples)
	{
		cv->portamentosamples = (*spr * m)/16;
		cv->portamentosamplepointer = 0;
		cv->startportamentofinetune = cv->portamentofinetune;
		cv->targetportamentofinetune = (r.note - (cv->r.note + cv->portamentofinetune));
	} return 1;
}

char Dc(jack_nframes_t fptr, uint16_t *spr, int m, Track *cv, Row r)
{
	if (!m) return 0;
	cv->delaysamples = *spr * m*DIV256;
	cv->delaynote = r.note;
	cv->delayinst = r.inst;
	return 1;
}

char Gc(jack_nframes_t fptr, uint16_t *spr, int m, Track *cv, Row r) { cv->gain.base = cv->gain.rand = m; return 1; }
char gc(jack_nframes_t fptr, uint16_t *spr, int m, Track *cv, Row r) { cv->gain.target = m; return 1; }
char altGc(jack_nframes_t fptr, uint16_t *spr, int m, Track *cv, Row r)
{
	signed char stereo = rand()%((m>>4)+1);
	cv->gain.rand =
		 (MAX(0, (cv->gain.base>>4)  - stereo - rand()%((m&0x0f)+1))<<4)
		+ MAX(0, (cv->gain.base&0xf) - stereo - rand()%((m&0x0f)+1));
	return 1;
}
char altgc(jack_nframes_t fptr, uint16_t *spr, int m, Track *cv, Row r)
{
	cv->gain.target_rand = 1;
	signed char stereo = rand()%((m>>4)+1);
	cv->gain.target =
		 (MAX(0, (cv->gain.base>>4)  - stereo - rand()%((m&0x0f)+1))<<4)
		+ MAX(0, (cv->gain.base&0xf) - stereo - rand()%((m&0x0f)+1));
	return 1;
}

char Sc(jack_nframes_t fptr, uint16_t *spr, int m, Track *cv, Row r) { cv->send.base = cv->send.rand = m; return 1; }
char sc(jack_nframes_t fptr, uint16_t *spr, int m, Track *cv, Row r) { cv->send.target = m; return 1; }
char altSc(jack_nframes_t fptr, uint16_t *spr, int m, Track *cv, Row r)
{
	signed char stereo = rand()%((m>>4)+1);
	cv->send.rand =
		 (MAX(0, (cv->send.base>>4)  - stereo - rand()%((m&0x0f)+1))<<4)
		+ MAX(0, (cv->send.base&0xf) - stereo - rand()%((m&0x0f)+1));
	return 1;
}
char altsc(jack_nframes_t fptr, uint16_t *spr, int m, Track *cv, Row r)
{
	cv->send.target_rand = 1;
	signed char stereo = rand()%((m>>4)+1);
	cv->send.target =
		 (MAX(0, (cv->send.base>>4)  - stereo - rand()%((m&0x0f)+1))<<4)
		+ MAX(0, (cv->send.base&0xf) - stereo - rand()%((m&0x0f)+1));
	return 1;
}

char _altRc(jack_nframes_t fptr, uint16_t *spr, int m, Track *cv, Row r)
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
char altRc(jack_nframes_t fptr, uint16_t *spr, int m, Track *cv, Row r)
{
	cv->data.rtrig_rev = 0;
	return _altRc(fptr, spr, m, cv, r);
}
char altrc(jack_nframes_t fptr, uint16_t *spr, int m, Track *cv, Row r)
{
	cv->data.rtrig_rev = 1;
	return _altRc(fptr, spr, m, cv, r);
}
char Rc(jack_nframes_t fptr, uint16_t *spr, int m, Track *cv, Row r)
{
	cv->rtrigpointer = cv->rtrigcurrentpointer = cv->pointer;
	cv->rtrigpitchedpointer = cv->rtrigcurrentpitchedpointer = cv->pitchedpointer;
	cv->rtrigblocksize = m>>4;
	if (m&0xf) cv->rtrigsamples = *spr / (m&0xf);
	else       cv->rtrigsamples = *spr * (cv->rtrigblocksize+1);
	return 1;
}
char rc(jack_nframes_t fptr, uint16_t *spr, int m, Track *cv, Row r)
{
	cv->data.rtrig_rev = 1;
	return Rc(fptr, spr, m, cv, r);
}

char pc(jack_nframes_t fptr, uint16_t *spr, int m, Track *cv, Row r)
{ cv->microtonalfinetune = m*DIV255; return 0; }

char percentc(jack_nframes_t fptr, uint16_t *spr, int m, Track *cv, Row r) /* returns true to NOT play */
{
	if (rand()%256 > m) return 1;
	else                return 0;
}

char midicctargetc(jack_nframes_t fptr, uint16_t *spr, int m, Track *cv, Row r) { cv->midiccindex = m%128; return 1; }
char midipcc(jack_nframes_t fptr, uint16_t *spr, int m, Track *cv, Row r)
{
	if (!cv->data.mute && p->s->instrument->i[(r.inst != INST_VOID) ? r.inst : cv->r.inst] < p->s->instrument->c)
	{
		Instrument *iv = &p->s->instrument->v[p->s->instrument->i[(r.inst != INST_VOID) ? r.inst : cv->r.inst]];
		if (iv->algorithm == INST_ALG_MIDI) midiPC(fptr, iv->midi.channel, m%128);
	} return 1;
}
char midiccc(jack_nframes_t fptr, uint16_t *spr, int m, Track *cv, Row r)
{
	cv->midicc = m%128;
	if (cv->midiccindex != -1 && !cv->data.mute && p->s->instrument->i[(r.inst != INST_VOID) ? r.inst : cv->r.inst] < p->s->instrument->c)
	{
		Instrument *iv = &p->s->instrument->v[p->s->instrument->i[(r.inst != INST_VOID) ? r.inst : cv->r.inst]];
		if (iv->algorithm == INST_ALG_MIDI) midiCC(fptr, iv->midi.channel, cv->midiccindex, cv->midicc);
	} return 1;
}

char _Oc(jack_nframes_t fptr, uint16_t *spr, int m, Track *cv, Row r)
{
	if (p->s->instrument->i[cv->r.inst] < p->s->instrument->c)
	{
		Instrument *iv = &p->s->instrument->v[p->s->instrument->i[cv->r.inst]];
		if (cv->r.note != NOTE_VOID) /* if playing a note */
		{
			if (r.note == NOTE_VOID) /* if not changing note, explicit ramping needed */
				ramp(cv, 0.0f, p->s->instrument->i[cv->samplerinst]);
			cv->pitchedpointer = (m*DIV256) * iv->trimlength * (float)samplerate / (float)iv->sample->rate;
		}
	} return 0;
}
char Oc(jack_nframes_t fptr, uint16_t *spr, int m, Track *cv, Row r)
{
	cv->data.reverse = 0;
	return _Oc(fptr, spr, m, cv, r);
}
char oc(jack_nframes_t fptr, uint16_t *spr, int m, Track *cv, Row r)
{
	cv->data.reverse = 1;
	if (m) return _Oc(fptr, spr, m, cv, r);
	return 0;
}
char altOc(jack_nframes_t fptr, uint16_t *spr, int m, Track *cv, Row r)
{
	if (p->s->instrument->i[cv->r.inst] < p->s->instrument->c)
	{
		Instrument *iv = &p->s->instrument->v[p->s->instrument->i[cv->r.inst]];
		if (cv->r.note != NOTE_VOID) /* if playing a note */
		{
			if (r.note == NOTE_VOID) /* if not changing note, explicit ramping needed */
				ramp(cv, 0.0f, p->s->instrument->i[cv->samplerinst]);
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
char altoc(jack_nframes_t fptr, uint16_t *spr, int m, Track *cv, Row r)
{
	cv->data.reverse = !cv->data.reverse;
	return altOc(fptr, spr, m, cv, r);
}

char Fc(jack_nframes_t fptr, uint16_t *spr, int m, Track *cv, Row r)
{
	cv->filter.cut[0] = cv->filter.randcut[0] =  m&0xf0;
	cv->filter.cut[1] = cv->filter.randcut[1] = (m&0x0f)<<4;
	return 1;
}
char fc(jack_nframes_t fptr, uint16_t *spr, int m, Track *cv, Row r)
{
	cv->filter.targetcut[0] =  m&0xf0;
	cv->filter.targetcut[1] = (m&0x0f)<<4;
	return 1;
}
char altFc(jack_nframes_t fptr, uint16_t *spr, int m, Track *cv, Row r)
{
	short stereo = rand()%((m&0xf0)+1);
	cv->filter.randcut[0] = MAX(0, cv->filter.cut[0] - stereo - rand()%(((m&0x0f)<<4)+1));
	cv->filter.randcut[1] = MAX(0, cv->filter.cut[1] - stereo - rand()%(((m&0x0f)<<4)+1));
	return 1;
}
char altfc(jack_nframes_t fptr, uint16_t *spr, int m, Track *cv, Row r)
{
	cv->filter.targetcut_rand = 1;
	short stereo = rand()%((m&0xf0)+1);
	cv->filter.targetcut[0] = MAX(0, cv->filter.cut[0] - stereo - rand()%(((m&0x0f)<<4)+1));
	cv->filter.targetcut[1] = MAX(0, cv->filter.cut[1] - stereo - rand()%(((m&0x0f)<<4)+1));
	return 1;
}

char Zc(jack_nframes_t fptr, uint16_t *spr, int m, Track *cv, Row r)
{
	cv->filter.res[0] = cv->filter.randres[0] =  m&0xf0;
	cv->filter.res[1] = cv->filter.randres[1] = (m&0x0f)<<4;
	return 1;
}
char zc(jack_nframes_t fptr, uint16_t *spr, int m, Track *cv, Row r)
{
	cv->filter.targetres[0] =  m&0xf0;
	cv->filter.targetres[1] = (m&0x0f)<<4;
	return 1;
}
char altZc(jack_nframes_t fptr, uint16_t *spr, int m, Track *cv, Row r)
{
	short stereo = rand()%((m&0xf0)+1);
	cv->filter.randres[0] = MAX(0, cv->filter.res[0] - stereo - rand()%(((m&0x0f)<<4)+1));
	cv->filter.randres[1] = MAX(0, cv->filter.res[1] - stereo - rand()%(((m&0x0f)<<4)+1));
	return 1;
}
char altzc(jack_nframes_t fptr, uint16_t *spr, int m, Track *cv, Row r)
{
	cv->filter.targetres_rand = 1;
	short stereo = rand()%((m&0xf0)+1);
	cv->filter.targetres[0] = MAX(0, cv->filter.res[0] - stereo - rand()%(((m&0x0f)<<4)+1));
	cv->filter.targetres[1] = MAX(0, cv->filter.res[1] - stereo - rand()%(((m&0x0f)<<4)+1));
	return 1;
}

char Mc(jack_nframes_t fptr, uint16_t *spr, int m, Track *cv, Row r)
{
	cv->filter.mode[0] = (m&0x70)>>4; /* ignore the '8' bit */
	cv->filter.mode[1] =  m&0x07;     /* ignore the '8' bit */
	return 1;
}
char mc(jack_nframes_t fptr, uint16_t *spr, int m, Track *cv, Row r)
{
	cv->filter.targetmode[0] = (m&0x70)>>4; /* ignore the '8' bit */
	cv->filter.targetmode[1] =  m&0x07;     /* ignore the '8' bit */
	return 1;
}

char Xc(jack_nframes_t fptr, uint16_t *spr, int m, Track *cv, Row r) { cv->localsamplerate = m; return 1; }
char xc(jack_nframes_t fptr, uint16_t *spr, int m, Track *cv, Row r) { cv->targetlocalsamplerate = m; return 1; }

char Ec(jack_nframes_t fptr, uint16_t *spr, int m, Track *cv, Row r) { cv->localenvelope = m;         return 1; }
char ec(jack_nframes_t fptr, uint16_t *spr, int m, Track *cv, Row r) { cv->localsustain = m;          return 1; }
char Hc(jack_nframes_t fptr, uint16_t *spr, int m, Track *cv, Row r) { cv->localpitchshift = m;       return 1; }
char hc(jack_nframes_t fptr, uint16_t *spr, int m, Track *cv, Row r) { cv->targetlocalpitchshift = m; return 1; }
char Wc(jack_nframes_t fptr, uint16_t *spr, int m, Track *cv, Row r) { cv->localpitchwidth = m;       return 1; }
char wc(jack_nframes_t fptr, uint16_t *spr, int m, Track *cv, Row r) { cv->targetlocalpitchwidth = m; return 1; }
char Lc(jack_nframes_t fptr, uint16_t *spr, int m, Track *cv, Row r)
{
	if (p->s->instrument->i[cv->r.inst] < p->s->instrument->c)
	{
		Instrument *iv = &p->s->instrument->v[p->s->instrument->i[cv->r.inst]];
		if (cv->localcyclelength == -1) cv->localcyclelength = iv->granular.cyclelength;
		cv->localcyclelength = (((uint16_t)cv->localcyclelength<<8)>>8) + (m<<8);
	}
	return 1;
}
char lc(jack_nframes_t fptr, uint16_t *spr, int m, Track *cv, Row r)
{
	if (p->s->instrument->i[cv->r.inst] < p->s->instrument->c)
	{
		Instrument *iv = &p->s->instrument->v[p->s->instrument->i[cv->r.inst]];
		if (cv->localcyclelength == -1) cv->localcyclelength = iv->granular.cyclelength;
		cv->localcyclelength = (((uint16_t)cv->localcyclelength>>8)<<8)+m;
	}
	return 1;
}
