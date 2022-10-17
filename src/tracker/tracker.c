#define CHANNEL_TRIG_COLS 4

void inputPatternHex(Row *r, char value)
{
	short macro;
	switch (w->trackerfx)
	{
		case 1: if (r->inst == INST_VOID) r->inst++; r->inst<<=4; r->inst+=value; break;
		default:
			macro = (w->trackerfx - 2)>>1;
			if (w->trackerfx % 2 == 1) { r->macro[macro].v <<= 4; r->macro[macro].v += value; }
			break;
	} regenGlobalRowc(s);
}
uint8_t changeNoteOctave(uint8_t octave, uint8_t note)
{
	w->octave = octave;
	if (note == NOTE_VOID) return NOTE_VOID;

	octave *= 12;
	while (note > octave + 11) note -= 12;
	while (note < octave + 0)  note += 12;
	return MIN(NOTE_A10-1, note);
}

void trackerAdjustRight(void) /* mouse adjust only */
{
	Row *r = getChannelRow(&s->channel->v[w->channel].data, w->trackerfy);
	short macro;
	switch (w->trackerfx)
	{
		case 0: r->note++; break;
		case 1:
			if (w->fieldpointer) r->inst+=16;
			else                 r->inst++;
			break;
		default:
			macro = (w->trackerfx - 2)>>1;
			if (w->trackerfx % 2 == 1)
			{
				if (w->fieldpointer) r->macro[macro].v+=16;
				else                 r->macro[macro].v++;
			} break;
	} regenGlobalRowc(s);
}
void trackerAdjustLeft(void) /* mouse adjust only */
{
	Row *r = getChannelRow(&s->channel->v[w->channel].data, w->trackerfy);
	short macro;
	switch (w->trackerfx)
	{
		case 0: r->note--; break;
		case 1:
			if (w->fieldpointer) r->inst-=16;
			else                 r->inst--;
			break;
		default:
			macro = (w->trackerfx - 2)>>1;
			if (w->trackerfx % 2 == 1)
			{
				if (w->fieldpointer) r->macro[macro].v-=16;
				else                 r->macro[macro].v--;
			} break;
	} regenGlobalRowc(s);
}

void trackerUpArrow(int count)
{
	switch (w->page)
	{
		case PAGE_CHANNEL_VARIANT:
			w->follow = 0;
			if (MAX(1, w->count)*count > w->trackerfy) w->trackerfy = 0;
			else                                       w->trackerfy -= MAX(1, w->count)*count;
			break;
		case PAGE_CHANNEL_EFFECT: effectUpArrow(count); break;
	}
}
void trackerDownArrow(int count)
{
	switch (w->page)
	{
		case PAGE_CHANNEL_VARIANT:
			w->follow = 0;
			if (MAX(1, w->count)*count > s->songlen - w->trackerfy -1) w->trackerfy = s->songlen-1;
			else                                                       w->trackerfy += MAX(1, w->count)*count;
			break;
		case PAGE_CHANNEL_EFFECT: effectDownArrow(count); break;
	}
}

void cycleUp(void) /* TODO: count */
{
	Variant *v;
	int bound;
	switch (w->page)
	{
		case PAGE_CHANNEL_VARIANT:
			switch (w->mode)
			{
				/* TODO: variant trig mode and variant trig visual mode handling */
				case T_MODE_NORMAL: case T_MODE_INSERT:
					for (int i = 0; i < MAX(1, w->count); i++)
					{
						bound = getChannelVariant(&v, &s->channel->v[w->channel].data, w->trackerfy);
						if (bound != -1) cycleVariantUp(v, bound);
					} break;
				case T_MODE_VISUAL: case T_MODE_VISUALREPLACE:
					cycleUpPartPattern(MIN(tfxToVfx(w->trackerfx), w->visualfx), MAX(tfxToVfx(w->trackerfx), w->visualfx), MIN(w->trackerfy, w->visualfy), MAX(w->trackerfy, w->visualfy), MIN(w->channel, w->visualchannel), MAX(w->channel, w->visualchannel));
					break;
				case T_MODE_VISUALLINE:
					cycleUpPartPattern(0, 2+s->channel->v[w->channel].data.macroc, MIN(w->trackerfy, w->visualfy), MAX(w->trackerfy, w->visualfy), MIN(w->channel, w->visualchannel), MAX(w->channel, w->visualchannel));
					break;
			} regenGlobalRowc(s); break;
		case PAGE_CHANNEL_EFFECT: effectCtrlUpArrow(&s->channel->v[w->channel].data.effect, 1); break;
	}
}
void cycleDown(void) /* TODO: count */
{
	Variant *v;
	int bound;
	switch (w->page)
	{
		case PAGE_CHANNEL_VARIANT:
			switch (w->mode)
			{
				/* TODO: variant trig mode and variant trig visual mode handling */
				case T_MODE_NORMAL: case T_MODE_INSERT:
					for (int i = 0; i < MAX(1, w->count); i++)
					{
						bound = getChannelVariant(&v, &s->channel->v[w->channel].data, w->trackerfy);
						if (bound != -1) cycleVariantDown(v, bound);
					} break;
				case T_MODE_VISUAL: case T_MODE_VISUALREPLACE:
					cycleDownPartPattern(MIN(tfxToVfx(w->trackerfx), w->visualfx), MAX(tfxToVfx(w->trackerfx), w->visualfx), MIN(w->trackerfy, w->visualfy), MAX(w->trackerfy, w->visualfy), MIN(w->channel, w->visualchannel), MAX(w->channel, w->visualchannel));
					break;
				case T_MODE_VISUALLINE:
					cycleDownPartPattern(0, 2+s->channel->v[w->channel].data.macroc, MIN(w->trackerfy, w->visualfy), MAX(w->trackerfy, w->visualfy), MIN(w->channel, w->visualchannel), MAX(w->channel, w->visualchannel));
					break;
			} regenGlobalRowc(s); break;
		case PAGE_CHANNEL_EFFECT: effectCtrlDownArrow(&s->channel->v[w->channel].data.effect, 1); break;
	}
}
void trackerLeftArrow(void)
{
	switch (w->page)
	{
		case PAGE_CHANNEL_VARIANT:
			switch (w->mode)
			{
				case T_MODE_VTRIG: case T_MODE_VTRIG_INSERT: case T_MODE_VTRIG_VISUAL:
					if (MAX(1, w->count) > w->channel) w->channel = 0;
					else                               w->channel -= MAX(1, w->count);
					if (w->trackerfx > 3 + s->channel->v[w->channel].data.macroc * 2)
						w->trackerfx = 3 + s->channel->v[w->channel].data.macroc * 2;
					break;
				default:
					for (int i = 0; i < MAX(1, w->count); i++)
					{
						if      (w->trackerfx == 2 + (s->channel->v[w->channel].data.macroc<<1)) w->trackerfx = 1;
						else if (w->trackerfx == 0)
						{
							if (w->channel > 0)
							{
								w->channel--;
								w->trackerfx = 3;
							}
						} else if (w->trackerfx > 1)
						{
							if (w->trackerfx&0x1) w->trackerfx--;
							else                  w->trackerfx+=3;
						} else                    w->trackerfx--;
					} break;
			} break;
		case PAGE_CHANNEL_EFFECT: effectLeftArrow(); break;
	}
DEBUG=w->trackerfx; p->dirty=1;
}
void channelLeft(void)
{
	if (MAX(1, w->count) > w->channel) w->channel = 0;
	else                               w->channel -= MAX(1, w->count);
	if (w->trackerfx > 3 + s->channel->v[w->channel].data.macroc * 2)
		w->trackerfx = 3 + s->channel->v[w->channel].data.macroc * 2;
}
void trackerRightArrow(void)
{
	switch (w->page)
	{
		case PAGE_CHANNEL_VARIANT:
			switch (w->mode)
			{
				case T_MODE_VTRIG: case T_MODE_VTRIG_INSERT: case T_MODE_VTRIG_VISUAL:
					if (w->channel + MAX(1, w->count) > s->channel->c-1) w->channel = s->channel->c-1;
					else                                             w->channel += MAX(1, w->count);
					if (w->trackerfx > 3 + s->channel->v[w->channel].data.macroc * 2)
						w->trackerfx = 3 + s->channel->v[w->channel].data.macroc * 2;
					break;
				default:
					for (int i = 0; i < MAX(1, w->count); i++)
					{
						if      (w->trackerfx == 1) w->trackerfx = 2 + s->channel->v[w->channel].data.macroc * 2;
						else if (w->trackerfx == 3)
						{
							if (w->channel < s->channel->c-1)
							{
								w->channel++;
								w->trackerfx = 0;
							} else w->trackerfx = 2;
						} else if (w->trackerfx > 1)
						{
							if (w->trackerfx&0x1) w->trackerfx-=3;
							else                  w->trackerfx++;
						} else                    w->trackerfx++;
					} break;
			} break;
		case PAGE_CHANNEL_EFFECT: effectRightArrow(); break;
	}
DEBUG=w->trackerfx; p->dirty=1;
}
void channelRight(void)
{
	w->channel += MAX(1, w->count);
	if (w->channel > s->channel->c-1) w->channel = s->channel->c-1;
	if (w->trackerfx > 3 + s->channel->v[w->channel].data.macroc * 2)
		w->trackerfx = 3 + s->channel->v[w->channel].data.macroc * 2;
}
void trackerHome(void)
{
	switch (w->page)
	{
		case PAGE_CHANNEL_VARIANT:
			w->follow = 0;
			w->trackerfy = STATE_ROWS;
			break;
		case PAGE_CHANNEL_EFFECT: effectHome(); break;
	}
}
void trackerEnd(void)
{
	switch (w->page)
	{
		case PAGE_CHANNEL_VARIANT:
			w->follow = 0;
			w->trackerfy = s->songlen-1;
			break;
		case PAGE_CHANNEL_EFFECT: effectEnd(); break;
	}
}


void insertNote(Row *r, int input)
{
	switch (input)
	{
		case 1:  /* ^a */ r->note+=MAX(1, w->count); break;
		case 24: /* ^x */ r->note-=MAX(1, w->count); break;
		case 127: case '\b': /* backspace */
			r->note = NOTE_VOID;
			r->inst = INST_VOID;
			trackerUpArrow(w->step);
			break;
		case '0': r->note = changeNoteOctave(0, r->note); break;
		case '1': r->note = changeNoteOctave(1, r->note); break;
		case '2': r->note = changeNoteOctave(2, r->note); break;
		case '3': r->note = changeNoteOctave(3, r->note); break;
		case '4': r->note = changeNoteOctave(4, r->note); break;
		case '5': r->note = changeNoteOctave(5, r->note); break;
		case '6': r->note = changeNoteOctave(6, r->note); break;
		case '7': r->note = changeNoteOctave(7, r->note); break;
		case '8': r->note = changeNoteOctave(8, r->note); break;
		case '9': r->note = changeNoteOctave(9, r->note); break;
		default:
			if (w->instrumentrecv == INST_REC_LOCK_OK)
				r->inst = w->instrument;
			switch (w->keyboardmacro)
			{
				case 0: charToNote(input, &r->note); break;
				case 'G': case 'g': /* m.x and m.y are note */
					if (charToKmode(input, 1, &r->macro[0].v, &r->note))
						r->macro[0].c = w->keyboardmacro;
					break;
				default: /* m.x is note */
					if (charToKmode(input, 0, &r->macro[0].v, &r->note))
						r->macro[0].c = w->keyboardmacro;
					break;
			} break;
	} regenGlobalRowc(s);
}
void insertInst(Row *r, int input)
{
	switch (input)
	{
		case 1:  /* ^a */ r->inst+=MAX(1, w->count); if (w->instrumentrecv == INST_REC_LOCK_OK) w->instrument = r->inst; break;
		case 24: /* ^x */ r->inst-=MAX(1, w->count); if (w->instrumentrecv == INST_REC_LOCK_OK) w->instrument = r->inst; break;
		case ' ':            /* space     */ r->inst = w->instrument; break;
		case 127: case '\b': /* backspace */ r->inst = INST_VOID; break;
		case '0':            inputPatternHex(r, 0);  if (w->instrumentrecv == INST_REC_LOCK_OK) w->instrument = r->inst; break;
		case '1':            inputPatternHex(r, 1);  if (w->instrumentrecv == INST_REC_LOCK_OK) w->instrument = r->inst; break;
		case '2':            inputPatternHex(r, 2);  if (w->instrumentrecv == INST_REC_LOCK_OK) w->instrument = r->inst; break;
		case '3':            inputPatternHex(r, 3);  if (w->instrumentrecv == INST_REC_LOCK_OK) w->instrument = r->inst; break;
		case '4':            inputPatternHex(r, 4);  if (w->instrumentrecv == INST_REC_LOCK_OK) w->instrument = r->inst; break;
		case '5':            inputPatternHex(r, 5);  if (w->instrumentrecv == INST_REC_LOCK_OK) w->instrument = r->inst; break;
		case '6':            inputPatternHex(r, 6);  if (w->instrumentrecv == INST_REC_LOCK_OK) w->instrument = r->inst; break;
		case '7':            inputPatternHex(r, 7);  if (w->instrumentrecv == INST_REC_LOCK_OK) w->instrument = r->inst; break;
		case '8':            inputPatternHex(r, 8);  if (w->instrumentrecv == INST_REC_LOCK_OK) w->instrument = r->inst; break;
		case '9':            inputPatternHex(r, 9);  if (w->instrumentrecv == INST_REC_LOCK_OK) w->instrument = r->inst; break;
		case 'A': case 'a':  inputPatternHex(r, 10); if (w->instrumentrecv == INST_REC_LOCK_OK) w->instrument = r->inst; break;
		case 'B': case 'b':  inputPatternHex(r, 11); if (w->instrumentrecv == INST_REC_LOCK_OK) w->instrument = r->inst; break;
		case 'C': case 'c':  inputPatternHex(r, 12); if (w->instrumentrecv == INST_REC_LOCK_OK) w->instrument = r->inst; break;
		case 'D': case 'd':  inputPatternHex(r, 13); if (w->instrumentrecv == INST_REC_LOCK_OK) w->instrument = r->inst; break;
		case 'E': case 'e':  inputPatternHex(r, 14); if (w->instrumentrecv == INST_REC_LOCK_OK) w->instrument = r->inst; break;
		case 'F': case 'f':  inputPatternHex(r, 15); if (w->instrumentrecv == INST_REC_LOCK_OK) w->instrument = r->inst; break;
	} regenGlobalRowc(s);
}
void insertMacroc(Row *r, uint8_t macro, int input)
{
	switch (input)
	{
		case 127: case '\b': /* backspace */ r->macro[macro].c = 0; break;
		default:
			if (!r->macro[macro].c) r->macro[macro].v = 0;
			changeMacro(input, &r->macro[macro].c);
			break;
	} regenGlobalRowc(s);
}
void insertMacrov(Row *r, uint8_t macro, int input)
{
	if (r->macro[macro].c)
		switch (input)
		{
			case 1:  /* ^a */
				switch (r->macro[macro].c)
				{
					case 'G': case 'g': case 'K': case 'k': r->macro[macro].v += MAX(1, w->count)*16;
					default:                                r->macro[macro].v += MAX(1, w->count);
				} break;
			case 24: /* ^x */
				switch (r->macro[macro].c)
				{
					case 'G': case 'g': case 'K': case 'k': r->macro[macro].v -= MAX(1, w->count)*16;
					default:                                r->macro[macro].v -= MAX(1, w->count);
				} break;
			case 127: case '\b': /* backspace */ r->macro[macro].c = 0; break;
			case '0':           inputPatternHex(r, 0);  break;
			case '1':           inputPatternHex(r, 1);  break;
			case '2':           inputPatternHex(r, 2);  break;
			case '3':           inputPatternHex(r, 3);  break;
			case '4':           inputPatternHex(r, 4);  break;
			case '5':           inputPatternHex(r, 5);  break;
			case '6':           inputPatternHex(r, 6);  break;
			case '7':           inputPatternHex(r, 7);  break;
			case '8':           inputPatternHex(r, 8);  break;
			case '9':           inputPatternHex(r, 9);  break;
			case 'A': case 'a': inputPatternHex(r, 10); break;
			case 'B': case 'b': inputPatternHex(r, 11); break;
			case 'C': case 'c': inputPatternHex(r, 12); break;
			case 'D': case 'd': inputPatternHex(r, 13); break;
			case 'E': case 'e': inputPatternHex(r, 14); break;
			case 'F': case 'f': inputPatternHex(r, 15); break;
		}
	regenGlobalRowc(s);
}

void applyChannelMutes(void)
{
	Event e;
	e.sem = M_SEM_CHANNEL_MUTE;
	pushEvent(&e);
}
void toggleChannelMute(uint8_t channel)
{
	s->channel->v[channel].data.mute = !s->channel->v[channel].data.mute;
	applyChannelMutes();
}
void toggleChannelSolo(uint8_t channel)
{
	bool flush = 1; /* all channels except the toggled one should be muted */
	bool reset = 1; /* all channels should be unmuted                      */

	for (int i = 0; i < s->channel->c; i++)
	{
		if ( s->channel->v[i].data.mute)                 flush = 0;
		if ( s->channel->v[i].data.mute && i == channel) reset = 0;
		if (!s->channel->v[i].data.mute && i != channel) reset = 0;
	}

	if (flush && !reset)
	{
		for (int i = 0; i < s->channel->c; i++)
			if (i != channel) s->channel->v[i].data.mute = 1;
		applyChannelMutes();
	} else if (reset && !flush)
	{
		for (int i = 0; i < s->channel->c; i++)
			s->channel->v[i].data.mute = 0;
		applyChannelMutes();
	} else toggleChannelMute(channel);
}

void leaveSpecialModes(void)
{
	switch (w->mode)
	{
		case T_MODE_INSERT: case T_MODE_VTRIG: case T_MODE_VTRIG_INSERT: break;
		case T_MODE_VTRIG_VISUAL: w->mode = T_MODE_VTRIG; break;
		default:                 w->mode = T_MODE_NORMAL; break;
	}
}

void chordRowScaleToCursor(void *_)
{
	ChannelData *cd = &s->channel->v[w->channel].data;
	int gcvret = getPrevVtrig(cd, w->trackerfy);
	if (gcvret == -1 || cd->trig[gcvret].index == VARIANT_OFF) return;
	uint8_t vi = cd->varianti[cd->trig[gcvret].index];

	Variant *v = _copyVariant(cd->variantv[vi], w->trackerfy - gcvret);

	free(cd->variantv[vi]); cd->variantv[vi] = v;
	regenGlobalRowc(s);
}
void chordRowLengthToCount(void *_)
{
	ChannelData *cd = &s->channel->v[w->channel].data;
	int gcvret = getChannelVariant(NULL, cd, w->trackerfy);
	if (gcvret == -1) return;
	uint8_t vi = cd->varianti[cd->trig[w->trackerfy - gcvret].index];

	Variant *v = _copyVariant(cd->variantv[vi], w->count ? w->count-1 : w->defvariantlength);

	free(cd->variantv[vi]); cd->variantv[vi] = v;
	if (!(cd->trig[w->trackerfy - gcvret].flags&C_VTRIG_LOOP)
			&& w->trackerfy > w->trackerfy - gcvret + cd->variantv[vi]->rowc)
		w->trackerfy = w->trackerfy - gcvret + cd->variantv[vi]->rowc;
	regenGlobalRowc(s);
}
void chordRowIncrementLength(void *_)
{
	ChannelData *cd = &s->channel->v[w->channel].data;
	int gcvret = getChannelVariant(NULL, cd, w->trackerfy);
	if (gcvret == -1) return;
	uint8_t vi = cd->varianti[cd->trig[w->trackerfy - gcvret].index];

	Variant *v = _copyVariant(cd->variantv[vi], MIN(VARIANT_ROWMAX, cd->variantv[vi]->rowc + MAX(1, w->count)));

	free(cd->variantv[vi]); cd->variantv[vi] = v;
	if (!(cd->trig[w->trackerfy - gcvret].flags&C_VTRIG_LOOP)
			&& w->trackerfy > w->trackerfy - gcvret + cd->variantv[vi]->rowc)
		w->trackerfy = w->trackerfy - gcvret + cd->variantv[vi]->rowc;
	regenGlobalRowc(s);
}
void chordRowDecrementLength(void *_)
{
	ChannelData *cd = &s->channel->v[w->channel].data;
	int gcvret = getChannelVariant(NULL, cd, w->trackerfy);
	if (gcvret == -1) return;
	uint8_t vi = cd->varianti[cd->trig[w->trackerfy - gcvret].index];

	Variant *v = _copyVariant(cd->variantv[vi], MAX(0, cd->variantv[vi]->rowc - MAX(1, w->count)));

	free(cd->variantv[vi]); cd->variantv[vi] = v;
	if (!(cd->trig[w->trackerfy - gcvret].flags&C_VTRIG_LOOP)
			&& w->trackerfy > w->trackerfy - gcvret + cd->variantv[vi]->rowc)
		w->trackerfy = w->trackerfy - gcvret + cd->variantv[vi]->rowc;
	regenGlobalRowc(s);
}
void chordRowCopyDown(void *_)
{
	ChannelData *cd = &s->channel->v[w->channel].data;
	int gcvret = getChannelVariant(NULL, cd, w->trackerfy);
	if (gcvret == -1) return;
	uint8_t vi = cd->varianti[cd->trig[w->trackerfy - gcvret].index];

	Variant *v = _copyVariant(NULL, MIN(VARIANT_ROWMAX, (cd->variantv[vi]->rowc+1)*(2*MAX(1, w->count)) - 1));
	for (int i = 0; i <= v->rowc; i++)
		v->rowv[i] = cd->variantv[vi]->rowv[i%(cd->variantv[vi]->rowc+1)];

	free(cd->variantv[vi]); cd->variantv[vi] = v;
	if (!(cd->trig[w->trackerfy - gcvret].flags&C_VTRIG_LOOP)
			&& w->trackerfy > w->trackerfy - gcvret + cd->variantv[vi]->rowc)
		w->trackerfy = w->trackerfy - gcvret + cd->variantv[vi]->rowc;
	regenGlobalRowc(s);
}
void chordRowDiscardHalf(void *_)
{
	ChannelData *cd = &s->channel->v[w->channel].data;
	int gcvret = getChannelVariant(NULL, cd, w->trackerfy);
	if (gcvret == -1) return;
	uint8_t vi = cd->varianti[cd->trig[w->trackerfy - gcvret].index];

	Variant *v = _copyVariant(cd->variantv[vi], MAX(0, (cd->variantv[vi]->rowc+1)/(2*MAX(1, w->count)) - 1));

	free(cd->variantv[vi]); cd->variantv[vi] = v;
	if (!(cd->trig[w->trackerfy - gcvret].flags&C_VTRIG_LOOP)
			&& w->trackerfy > w->trackerfy - gcvret + cd->variantv[vi]->rowc)
		w->trackerfy = w->trackerfy - gcvret + cd->variantv[vi]->rowc;
	regenGlobalRowc(s);
}
void chordRowAddBlanks(void *_)
{
	ChannelData *cd = &s->channel->v[w->channel].data;
	int gcvret = getChannelVariant(NULL, cd, w->trackerfy);
	if (gcvret == -1) return;
	uint8_t vi = cd->varianti[cd->trig[w->trackerfy - gcvret].index];

	Variant *v = _copyVariant(NULL, MIN(VARIANT_ROWMAX, (cd->variantv[vi]->rowc+1)*(2*MAX(1, w->count)) - 1));
	for (int i = 0; i <= cd->variantv[vi]->rowc; i++)
		v->rowv[i * MAX(1, w->count)*2] = cd->variantv[vi]->rowv[i];

	free(cd->variantv[vi]); cd->variantv[vi] = v;
	if (!(cd->trig[w->trackerfy - gcvret].flags&C_VTRIG_LOOP)
			&& w->trackerfy > w->trackerfy - gcvret + cd->variantv[vi]->rowc)
		w->trackerfy = w->trackerfy - gcvret + cd->variantv[vi]->rowc;
	regenGlobalRowc(s);
}
void chordRowDiscardEveryOther(void *_)
{
	ChannelData *cd = &s->channel->v[w->channel].data;
	int gcvret = getChannelVariant(NULL, cd, w->trackerfy);
	if (gcvret == -1) return;
	uint8_t vi = cd->varianti[cd->trig[w->trackerfy - gcvret].index];

	Variant *v = _copyVariant(NULL, MAX(0, (cd->variantv[vi]->rowc+1)/(2*MAX(1, w->count)) - 1));
	for (int i = 0; i <= v->rowc; i++)
		v->rowv[i] = cd->variantv[vi]->rowv[i * MAX(1, w->count)*2];

	free(cd->variantv[vi]); cd->variantv[vi] = v;
	if (!(cd->trig[w->trackerfy - gcvret].flags&C_VTRIG_LOOP)
			&& w->trackerfy > w->trackerfy - gcvret + cd->variantv[vi]->rowc)
		w->trackerfy = w->trackerfy - gcvret + cd->variantv[vi]->rowc;
	regenGlobalRowc(s);
}
void chordRowBurn(void *_)
{
	ChannelData *cd = &s->channel->v[w->channel].data;
	int gcvret = getChannelVariant(NULL, cd, w->trackerfy);
	if (gcvret == -1) return;
	uint8_t vvi = cd->trig[w->trackerfy - gcvret].index;
	uint8_t vi = cd->varianti[vvi];

	cd->trig[w->trackerfy - gcvret].index = VARIANT_VOID;

	for (int i = 0; i < cd->variantv[vi]->rowc; i++)
		memcpy(getChannelRow(cd, (w->trackerfy - gcvret) + i), &cd->variantv[vi]->rowv[i], sizeof(Row));

	pruneVariant(cd, vvi);
	regenGlobalRowc(s);
}

void chordAddMacro(void *_)
{
	for (int i = 0; i < MAX(1, w->count); i++)
		if (s->channel->v[w->channel].data.macroc < 7)
			s->channel->v[w->channel].data.macroc++;
	regenGlobalRowc(s);
}
void chordDeleteMacro(void *_)
{
	for (int i = 0; i < MAX(1, w->count); i++)
	{
		if (s->channel->v[w->channel].data.macroc) s->channel->v[w->channel].data.macroc--;
		if (w->trackerfx > 3 + s->channel->v[w->channel].data.macroc*2)
			w->trackerfx = 3 + s->channel->v[w->channel].data.macroc*2;
	}
	regenGlobalRowc(s);
}
void chordSetMacro(void *_)
{
	if (w->count) s->channel->v[w->channel].data.macroc = MIN(8, w->count) - 1;
	else          s->channel->v[w->channel].data.macroc = 1;
	if (w->trackerfx > 3 + s->channel->v[w->channel].data.macroc*2)
		w->trackerfx = 3 + s->channel->v[w->channel].data.macroc*2;
	regenGlobalRowc(s);
}

void chordClearChannel(void *_)
{
	clearChanneldata(s, &s->channel->v[w->channel].data);
	regenGlobalRowc(s);
}
void chordAddChannel(void *_)
{
	addChannel(s, w->channel+1, MAX(1, w->count));
	w->channel = MIN(CHANNEL_MAX-1, w->channel + MAX(1, w->count)); /* atomically safe */
}
void chordAddBefore(void *_)
{
	addChannel(s, w->channel, MAX(1, w->count));
}
void chordDeleteChannel(void *_)
{
	delChannel(w->channel, MAX(1, w->count));
}
void chordCopyChannel(void *_)
{
	copyChanneldata(&w->channelbuffer, &s->channel->v[w->channel].data);
	regenGlobalRowc(s);
}
void chordPasteChannel(void *_)
{
	copyChanneldata(&s->channel->v[w->channel].data, &w->channelbuffer);
	for (int i = 1; i < MAX(1, w->count); i++)
	{
		if (s->channel->c >= CHANNEL_MAX) break;
		w->channel++;
		copyChanneldata(&s->channel->v[w->channel].data, &w->channelbuffer);
	}
	regenGlobalRowc(s);
}

void setLoopRange(uint16_t start, uint16_t end)
{
	s->loop[0] = start;
	if (s->playing)
	{
		if (s->playfy < end) s->loop[1] = end;
		else                 s->loop[2] = end;
	} else
		s->loop[1] = end;
}
void chordLoopContext(void *_)
{
	Variant *v;
	uint16_t ltrackerfy = MAX(w->trackerfy, STATE_ROWS);
	int gcvret = getChannelVariantNoLoop(&v, &s->channel->v[w->channel].data, ltrackerfy);
	if (gcvret == -1)
	{ /* not in a variant */
		if (s->loop[0] == ltrackerfy)
			setLoopRange(0, 0);
		else
			setLoopRange(ltrackerfy, ltrackerfy + 4*s->rowhighlight - 1);
	} else
	{ /* in a variant */
		if (s->loop[0] == ltrackerfy - gcvret
		 && s->loop[1] == ltrackerfy - gcvret + v->rowc)
			setLoopRange(0, 0);
		else
			setLoopRange(ltrackerfy - gcvret, ltrackerfy - gcvret + v->rowc);
	} regenGlobalRowc(s);
}
void chordDoubleLoopLength   (void *_) { if (!s->loop[1]) setLoopRange(STATE_ROWS, STATE_ROWS + 4*s->rowhighlight - 1); setLoopRange(s->loop[0], s->loop[0] + ((s->loop[1] - s->loop[0])<<1) + 1); regenGlobalRowc(s); }
void chordHalveLoopLength    (void *_) { if (!s->loop[1]) setLoopRange(STATE_ROWS, STATE_ROWS + 4*s->rowhighlight - 1); setLoopRange(s->loop[0], s->loop[0] + ((s->loop[1] - s->loop[0])>>1)); regenGlobalRowc(s);     }
void chordIncrementLoopLength(void *_) { if (!s->loop[1]) setLoopRange(STATE_ROWS, STATE_ROWS + 4*s->rowhighlight - 1); setLoopRange(s->loop[0], s->loop[1] + MAX(1, w->count)); regenGlobalRowc(s);                   }
void chordDecrementLoopLength(void *_) { if (!s->loop[1]) setLoopRange(STATE_ROWS, STATE_ROWS + 4*s->rowhighlight - 1); setLoopRange(s->loop[0], MAX(s->loop[0], s->loop[1] - MAX(1, w->count))); regenGlobalRowc(s);  }
void chordLoopScaleToCursor  (void *_)
{
	if (!s->loop[1])
		setLoopRange(STATE_ROWS, STATE_ROWS + 4*s->rowhighlight - 1);
	if (w->trackerfy < s->loop[0]) setLoopRange(MAX(w->trackerfy, STATE_ROWS), s->loop[1]);
	else                           setLoopRange(s->loop[0], w->trackerfy);

	regenGlobalRowc(s);
}

int trackerMouseHeader(int button, int x, int y, short *tx)
{
	if (y == CHANNEL_ROW-2)
	{
		switch (w->page)
		{ /* hacky implementation */
			case PAGE_CHANNEL_VARIANT: if (x >= ((ws.ws_col-17)>>1) + 9) showTracker(); break;
			case PAGE_CHANNEL_EFFECT:  if (x <  ((ws.ws_col-17)>>1) + 9) showTracker(); break;
		} return 1;
	} else if (y < CHANNEL_ROW-2)
	{
		if (x >= ((ws.ws_col-17)>>1) + 7) showInstrument();
		return 1;
	} else if (y <= CHANNEL_ROW)
	{
		for (int i = 0; i < s->channel->c; i++)
		{
			*tx += CHANNEL_TRIG_COLS;
			*tx += 8 + 4*(s->channel->v[i].data.macroc+1);
			if (*tx > x)
			{
				switch (button)
				{
					case BUTTON1: case BUTTON1_CTRL: w->channeloffset = i - w->channel; break;
					case BUTTON2: case BUTTON2_CTRL: toggleChannelSolo(i); break;
					case BUTTON3: case BUTTON3_CTRL: toggleChannelMute(i); break;
				} break;
			}
		} return 1;
	}
	return 0;
}

/* chord bindings */
void setChordRow(void) {
	clearTooltip(&tt);
	setTooltipTitle(&tt, "variant rows");
	addTooltipBind(&tt, "scale variant to cursor    ", 'c', chordRowScaleToCursor,     NULL);
	addTooltipBind(&tt, "set variant length to count", 'r', chordRowLengthToCount,     NULL);
	addTooltipBind(&tt, "increment variant length   ", 'a', chordRowIncrementLength,   NULL);
	addTooltipBind(&tt, "decrement variant length   ", 'd', chordRowDecrementLength,   NULL);
	addTooltipBind(&tt, "double variant length      ", '+', chordRowCopyDown,          NULL);
	addTooltipBind(&tt, "halve variant length       ", '-', chordRowDiscardHalf,       NULL);
	addTooltipBind(&tt, "stretch variant length     ", '*', chordRowAddBlanks,         NULL);
	addTooltipBind(&tt, "shrink variant length      ", '/', chordRowDiscardEveryOther, NULL);
	addTooltipBind(&tt, "burn variant               ", 'b', chordRowBurn,              NULL);
	w->chord = 'r';
}
void setChordMacro(void) {
	clearTooltip(&tt);
	setTooltipTitle(&tt, "macro");
	addTooltipBind(&tt, "increment macro columns   ", 'a', chordAddMacro,    NULL);
	addTooltipBind(&tt, "decrement macro columns   ", 'd', chordDeleteMacro, NULL);
	addTooltipBind(&tt, "set macro columns to count", 'm', chordSetMacro,    NULL);
	w->chord = 'm';
}
void setChordChannel(void) {
	clearTooltip(&tt);
	setTooltipTitle(&tt, "channel");
	addTooltipBind(&tt, "clear channel ", 'c', chordClearChannel,  NULL);
	addTooltipBind(&tt, "add channel   ", 'a', chordAddChannel,    NULL);
	addTooltipBind(&tt, "add before    ", 'A', chordAddBefore,     NULL);
	addTooltipBind(&tt, "delete channel", 'd', chordDeleteChannel, NULL);
	addTooltipBind(&tt, "copy channel  ", 'y', chordCopyChannel,   NULL);
	addTooltipBind(&tt, "paste channel ", 'p', chordPasteChannel,  NULL);
	w->chord = 'c';
}
void setChordLoop(void) {
	clearTooltip(&tt);
	setTooltipTitle(&tt, "loop range");
	addTooltipBind(&tt, "loop the current context ", ';', chordLoopContext,         NULL);
	addTooltipBind(&tt, "double the loop length   ", '+', chordDoubleLoopLength,    NULL);
	addTooltipBind(&tt, "double the loop length   ", '*', chordDoubleLoopLength,    NULL);
	addTooltipBind(&tt, "halve the loop length    ", '-', chordHalveLoopLength,     NULL);
	addTooltipBind(&tt, "halve the loop length    ", '/', chordHalveLoopLength,     NULL);
	addTooltipBind(&tt, "increment the loop length", 'a', chordIncrementLoopLength, NULL);
	addTooltipBind(&tt, "decrement the loop length", 'd', chordDecrementLoopLength, NULL);
	addTooltipBind(&tt, "scale loop to cursor     ", 'c', chordLoopScaleToCursor,   NULL);
	w->chord = ';';
}
