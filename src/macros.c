char ifMacro(jack_nframes_t fptr, uint16_t *spr, Track *cv, Row r, char m, char (*callback)(jack_nframes_t, uint16_t *, int, Track *, Row))
{
	char ret = 0;
	for (int i = 0; i <= cv->data.variant->macroc; i++)
		if (r.macro[i].c == m)
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

bool changeMacro(int input, char *dest)
{
	/* use the current value if input is '\0' */
	if (!input)
	{
		if      (isupper(*dest)) input = tolower(*dest);
		else if (islower(*dest)) input = toupper(*dest);
		else                     input = *dest;
	}

	switch (input)
	{
		/* ^xx: row repititions, trigger the row every xx times it's tried */
		case '%': *dest = '%'; return 1; /* note chance                  */
		case 'b': *dest = 'B'; return 1; /* bpm                          */
		/* bxx: speed */
		case 'c': *dest = 'C'; return 1; /* note cut                     */
		case 'd': *dest = 'D'; return 1; /* note delay                   */
		case 'e': *dest = 'E'; return 1; /* local envelope times         */
		case 'E': *dest = 'e'; return 1; /* local envelope mode          */
		case 'f': *dest = 'F'; return 1; /* filter                       */
		case 'F': *dest = 'f'; return 1; /* smooth filter                */
		case 'g': *dest = 'G'; return 1; /* stereo gain                  */
		case 'G': *dest = 'g'; return 1; /* smooth stereo gain           */
		case 'h': *dest = 'H'; return 1; /* local pitch shift            */
		case 'H': *dest = 'h'; return 1; /* smooth local pitch shift     */
		/* Ixx: instrument jitter */
		case 'j': *dest = 'J'; return 1; /* stereo gain jitter           */
		case 'J': *dest = 'j'; return 1; /* smooth stereo gain jitter    */
		case 'k': *dest = 'K'; return 1; /* filter jitter                */
		case 'K': *dest = 'k'; return 1; /* smooth filter jitter         */
		case 'l': *dest = 'L'; return 1; /* local cycle length high byte */
		case 'L': *dest = 'l'; return 1; /* local cycle length low byte  */
		case 'm': *dest = 'M'; return 1; /* filter mode                  */
		case 'M': *dest = 'm'; return 1; /* smooth filter mode           */
		/* Nxy: tremolo */
		/* nxy: autopan */
		case 'o': *dest = 'O'; return 1; /* offset                       */
		case 'O': *dest = 'o'; return 1; /* backwards offset             */
		case 'p': *dest = 'P'; return 1; /* portamento                   */
		case 'P': *dest = 'p'; return 1; /* microtonal offset            */
		case 'q': *dest = 'Q'; return 1; /* retrigger                    */
		case 'Q': *dest = 'q'; return 1; /* backwards retrigger          */
		case 'r': *dest = 'R'; return 1; /* block retrigger              */
		case 'R': *dest = 'r'; return 1; /* backwards block retrigger    */
		case 's': *dest = 'S'; return 1; /* send                         */
		case 'S': *dest = 's'; return 1; /* smooth send                  */
		/* Txx: local timestretch        */
		/* txx: smooth local timestretch */
		case 'u': *dest = 'U'; return 1; /* random offset                */
		case 'U': *dest = 'u'; return 1; /* random backwards offset      */
		case 'v': *dest = 'V'; return 1; /* vibrato                      */
		/* vxy: stereo vibrato */
		case 'w': *dest = 'W'; return 1; /* local pitch width            */
		case 'W': *dest = 'w'; return 1; /* smooth local pitch width     */
		case 'x': *dest = 'X'; return 1; /* local samplerate             */
		case 'X': *dest = 'x'; return 1; /* target local samplerate      */
		case 'z': *dest = 'Z'; return 1; /* filter resonance             */
		case 'Z': *dest = 'z'; return 1; /* smooth filter resonance      */
	}
	return 0;
}
void addMacroBinds(TooltipState *tt, const char *prettyname, unsigned int state, void (*callback)(void*))
{
	addTooltipPrettyPrint(tt, prettyname, "macro");
	addTooltipBind(tt, "note chance"                 , state, XK_percent, 0, callback, (void*)'%');
	addTooltipBind(tt, "bpm"                         , state, XK_b      , 0, callback, (void*)'B');
	addTooltipBind(tt, "note cut"                    , state, XK_c      , 0, callback, (void*)'C');
	addTooltipBind(tt, "note delay"                  , state, XK_d      , 0, callback, (void*)'D');
	addTooltipBind(tt, "local envelope times"        , state, XK_e      , 0, callback, (void*)'E');
	addTooltipBind(tt, "local envelope mode"         , state, XK_E      , 0, callback, (void*)'e');
	addTooltipBind(tt, "filter"                      , state, XK_f      , 0, callback, (void*)'F');
	addTooltipBind(tt, "smooth filter"               , state, XK_F      , 0, callback, (void*)'f');
	addTooltipBind(tt, "stereo gain"                 , state, XK_g      , 0, callback, (void*)'G');
	addTooltipBind(tt, "smooth stereo gain"          , state, XK_G      , 0, callback, (void*)'g');
	addTooltipBind(tt, "local pitch shift"           , state, XK_h      , 0, callback, (void*)'H');
	addTooltipBind(tt, "smooth local pitch shift"    , state, XK_H      , 0, callback, (void*)'h');
	addTooltipBind(tt, "stereo gain jitter"          , state, XK_j      , 0, callback, (void*)'J');
	addTooltipBind(tt, "smooth stereo gain jitter"   , state, XK_J      , 0, callback, (void*)'j');
	addTooltipBind(tt, "filter jitter"               , state, XK_k      , 0, callback, (void*)'K');
	addTooltipBind(tt, "smooth filter jitter"        , state, XK_K      , 0, callback, (void*)'k');
	addTooltipBind(tt, "local cycle length high byte", state, XK_l      , 0, callback, (void*)'L');
	addTooltipBind(tt, "local cycle length low byte" , state, XK_L      , 0, callback, (void*)'l');
	addTooltipBind(tt, "filter mode"                 , state, XK_m      , 0, callback, (void*)'M');
	addTooltipBind(tt, "smooth filter mode"          , state, XK_M      , 0, callback, (void*)'m');
	addTooltipBind(tt, "offset"                      , state, XK_o      , 0, callback, (void*)'O');
	addTooltipBind(tt, "backwards offset"            , state, XK_O      , 0, callback, (void*)'o');
	addTooltipBind(tt, "portamento"                  , state, XK_p      , 0, callback, (void*)'P');
	addTooltipBind(tt, "microtonal offset"           , state, XK_P      , 0, callback, (void*)'p');
	addTooltipBind(tt, "retrigger"                   , state, XK_q      , 0, callback, (void*)'Q');
	addTooltipBind(tt, "backwards retrigger"         , state, XK_Q      , 0, callback, (void*)'q');
	addTooltipBind(tt, "block retrigger"             , state, XK_r      , 0, callback, (void*)'R');
	addTooltipBind(tt, "backwards block retrigger"   , state, XK_R      , 0, callback, (void*)'r');
	addTooltipBind(tt, "send"                        , state, XK_s      , 0, callback, (void*)'S');
	addTooltipBind(tt, "smooth send"                 , state, XK_S      , 0, callback, (void*)'s');
	addTooltipBind(tt, "random offset"               , state, XK_u      , 0, callback, (void*)'U');
	addTooltipBind(tt, "random backwards offset"     , state, XK_U      , 0, callback, (void*)'u');
	addTooltipBind(tt, "vibrato"                     , state, XK_v      , 0, callback, (void*)'V');
	addTooltipBind(tt, "local pitch width"           , state, XK_w      , 0, callback, (void*)'W');
	addTooltipBind(tt, "smooth local pitch width"    , state, XK_W      , 0, callback, (void*)'w');
	addTooltipBind(tt, "local samplerate"            , state, XK_x      , 0, callback, (void*)'X');
	addTooltipBind(tt, "target local samplerate"     , state, XK_X      , 0, callback, (void*)'x');
	addTooltipBind(tt, "filter resonance"            , state, XK_z      , 0, callback, (void*)'Z');
	addTooltipBind(tt, "smooth filter resonance"     , state, XK_Z      , 0, callback, (void*)'z');
}

void descMacro(char c, uint8_t v)
{
	char text[64] = {'\0'};
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
