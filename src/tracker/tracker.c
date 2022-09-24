#define T_MODE_NORMAL 0
#define T_MODE_INSERT 1
#define T_MODE_MOUSEADJUST 2
#define T_MODE_VISUAL 3
#define T_MODE_VISUALLINE 4
#define T_MODE_VISUALREPLACE 5
#define T_MODE_VTRIG 6
#define T_MODE_VTRIG_INSERT 7
#define T_MODE_VTRIG_VISUAL 8
#define T_MODE_VTRIG_MOUSEADJUST 9

void inputPatternHex(row *r, char value)
{
	short macro;
	switch (w->trackerfx)
	{
		case 1: if (r->inst == INST_VOID) r->inst++; r->inst<<=4; r->inst+=value; break;
		default:
			macro = (w->trackerfx - 2)>>1;
			if (w->trackerfx % 2 == 1) { r->macro[macro].v <<= 4; r->macro[macro].v += value; }
			break;
	}
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
	row *r = getChannelRow(&s->channelv[w->channel].data, w->trackerfy);
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
	}
}
void trackerAdjustLeft(void) /* mouse adjust only */
{
	row *r = getChannelRow(&s->channelv[w->channel].data, w->trackerfy);
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
	}
}

void trackerUpArrow(int rows)
{
	if (w->flags&W_FLAG_FOLLOW) w->flags ^= W_FLAG_FOLLOW;
	if (MAX(1, w->count)*rows > w->trackerfy) w->trackerfy = 0;
	else                                      w->trackerfy -= MAX(1, w->count)*rows;
}
void trackerDownArrow(int rows)
{
	if (w->flags&W_FLAG_FOLLOW) w->flags ^= W_FLAG_FOLLOW;
	if (MAX(1, w->count)*rows > s->songlen - w->trackerfy -1) w->trackerfy = s->songlen-1;
	else                                                   w->trackerfy += MAX(1, w->count)*rows;
}

void cycleUp(void)
{
	variant *v;
	uint16_t bound;
	switch (w->mode)
	{
		/* TODO: variant trig mode and variant trig visual mode handling */
		case T_MODE_NORMAL: case T_MODE_INSERT:
			for (int i = 0; i < MAX(1, w->count); i++)
			{
				bound = getChannelVariant(&v, &s->channelv[w->channel].data, w->trackerfy);
				if (bound != -1) cycleVariantUp(v, bound);
			} break;
		case T_MODE_VISUAL: case T_MODE_VISUALREPLACE:
			cycleUpPartPattern(MIN(tfxToVfx(w->trackerfx), w->visualfx), MAX(tfxToVfx(w->trackerfx), w->visualfx), MIN(w->trackerfy, w->visualfy), MAX(w->trackerfy, w->visualfy), MIN(w->channel, w->visualchannel), MAX(w->channel, w->visualchannel));
			break;
		case T_MODE_VISUALLINE:
			cycleUpPartPattern(0, 2+s->channelv[w->channel].data.macroc, MIN(w->trackerfy, w->visualfy), MAX(w->trackerfy, w->visualfy), MIN(w->channel, w->visualchannel), MAX(w->channel, w->visualchannel));
			break;
	}
	// regenGlobalRowc(s); /* doesn't matter rn but may in the future */
}
void cycleDown(void)
{
	variant *v;
	uint16_t bound;
	switch (w->mode)
	{
		/* TODO: variant trig mode and variant trig visual mode handling */
		case T_MODE_NORMAL: case T_MODE_INSERT:
			for (int i = 0; i < MAX(1, w->count); i++)
			{
				bound = getChannelVariant(&v, &s->channelv[w->channel].data, w->trackerfy);
				if (bound != -1) cycleVariantDown(v, bound);
			} break;
		case T_MODE_VISUAL: case T_MODE_VISUALREPLACE:
			cycleDownPartPattern(MIN(tfxToVfx(w->trackerfx), w->visualfx), MAX(tfxToVfx(w->trackerfx), w->visualfx), MIN(w->trackerfy, w->visualfy), MAX(w->trackerfy, w->visualfy), MIN(w->channel, w->visualchannel), MAX(w->channel, w->visualchannel));
			break;
		case T_MODE_VISUALLINE:
			cycleDownPartPattern(0, 2+s->channelv[w->channel].data.macroc, MIN(w->trackerfy, w->visualfy), MAX(w->trackerfy, w->visualfy), MIN(w->channel, w->visualchannel), MAX(w->channel, w->visualchannel));
			break;
	}
	// regenGlobalRowc(s); /* doesn't matter rn but may in the future */
}
void trackerLeftArrow(void)
{
	switch (w->mode)
	{
		case T_MODE_VTRIG: case T_MODE_VTRIG_INSERT: case T_MODE_VTRIG_VISUAL:
			if (MAX(1, w->count) > w->channel) w->channel = 0;
			else                               w->channel -= MAX(1, w->count);
			if (w->trackerfx > 3 + s->channelv[w->channel].data.macroc * 2)
				w->trackerfx = 3 + s->channelv[w->channel].data.macroc * 2;
			break;
		default:
			for (int i = 0; i < MAX(1, w->count); i++)
			{
				w->trackerfx--;
				if (w->trackerfx < 0)
				{
					if (w->channel > 0)
					{
						w->channel--;
						w->trackerfx = 3 + s->channelv[w->channel].data.macroc * 2;
					} else w->trackerfx = 0;
				}
			} break;
	}
}
void channelLeft(void)
{
	if (MAX(1, w->count) > w->channel) w->channel = 0;
	else                               w->channel -= MAX(1, w->count);
	if (w->trackerfx > 3 + s->channelv[w->channel].data.macroc * 2)
		w->trackerfx = 3 + s->channelv[w->channel].data.macroc * 2;
}
void trackerRightArrow(void)
{
	switch (w->mode)
	{
		case T_MODE_VTRIG: case T_MODE_VTRIG_INSERT: case T_MODE_VTRIG_VISUAL:
			if (w->channel + MAX(1, w->count) > s->channelc-1) w->channel = s->channelc-1;
			else                                             w->channel += MAX(1, w->count);
			if (w->trackerfx > 3 + s->channelv[w->channel].data.macroc * 2)
				w->trackerfx = 3 + s->channelv[w->channel].data.macroc * 2;
			break;
		default:
			for (int i = 0; i < MAX(1, w->count); i++)
			{
				w->trackerfx++;
				if (w->trackerfx > 3 + s->channelv[w->channel].data.macroc * 2)
				{
					if (w->channel < s->channelc-1)
					{
						w->channel++;
						w->trackerfx = 0;
					} else w->trackerfx = 3 + s->channelv[w->channel].data.macroc * 2;
				}
			} break;
	}
}
void channelRight(void)
{
	w->channel += MAX(1, w->count);
	if (w->channel > s->channelc-1) w->channel = s->channelc-1;
	if (w->trackerfx > 3 + s->channelv[w->channel].data.macroc * 2)
		w->trackerfx = 3 + s->channelv[w->channel].data.macroc * 2;
}
void trackerHome(void)
{
	if (w->flags&W_FLAG_FOLLOW) w->flags ^= W_FLAG_FOLLOW;
	w->trackerfy = STATE_ROWS;
}
void trackerEnd(void)
{
	if (w->flags&W_FLAG_FOLLOW) w->flags ^= W_FLAG_FOLLOW;
	w->trackerfy = s->songlen-1;
}


void insertNote(row *r, int key)
{
	if (w->instrumentrecv == INST_REC_LOCK_OK)
		r->inst = w->instrument;
	switch (w->keyboardmacro)
	{
		case 0: charToNote(key, &r->note); break;
		case 'G': case 'g': case 'K': case 'k': /* m.x and m.y are note */
			if (charToKmode(key, 1, &r->macro[0].v, &r->note))
				r->macro[0].c = w->keyboardmacro;
			break;
		default: /* m.x is note */
			if (charToKmode(key, 0, &r->macro[0].v, &r->note))
				r->macro[0].c = w->keyboardmacro;
			break;
	}
}
void insertInst(row *r, int input)
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
	}
}
void insertMacroc(row *r, uint8_t macro, int input)
{
	switch (input)
	{
		case 127: case '\b': /* backspace */ r->macro[macro].c = 0; break;
		default:
			if (!r->macro[macro].c) r->macro[macro].v = 0;
			changeMacro(input, &r->macro[macro].c);
			break;
	}
}
void insertMacrov(row *r, uint8_t macro, int input)
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
}

void toggleChannelMute(uint8_t channel)
{
	s->channelv[channel].data.flags ^= C_FLAG_MUTE;
	if (s->channelv[channel].data.flags&C_FLAG_MUTE && w->instrumentlockv == INST_GLOBAL_LOCK_OK)
	{
		w->instrumentlocki = channel;
		w->instrumentlockv = INST_GLOBAL_CHANNEL_MUTE;
	}
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

void chordRow(channeldata *cd, int input)
{
	variant *v;
	int gcvret = getChannelVariant(NULL, cd, w->trackerfy);
	if (gcvret == -1) return;
	uint8_t vi = cd->varianti[cd->trig[w->trackerfy - gcvret].index];
	switch (input)
	{
		case 'c': v = _copyVariant(cd->variantv[vi], gcvret); break;
		case 'r': v = _copyVariant(cd->variantv[vi], w->count ? w->count-1 : w->defvariantlength); break;
		case 'a': v = _copyVariant(cd->variantv[vi], MIN(VARIANT_ROWMAX, cd->variantv[vi]->rowc + MAX(1, w->count))); break;
		case 'd': v = _copyVariant(cd->variantv[vi], MAX(0, cd->variantv[vi]->rowc - MAX(1, w->count))); break;
		case '+': v = _copyVariant(cd->variantv[vi], MIN(VARIANT_ROWMAX, (cd->variantv[vi]->rowc+1)*(2*MAX(1, w->count)) - 1)); break;
		case '-': v = _copyVariant(cd->variantv[vi], MAX(0, (cd->variantv[vi]->rowc+1)/(2*MAX(1, w->count)) - 1)); break;
		case '*':
			v = _copyVariant(NULL, MIN(VARIANT_ROWMAX, (cd->variantv[vi]->rowc+1)*(2*MAX(1, w->count)) - 1));
			for (int i = 0; i <= cd->variantv[vi]->rowc; i++)
				v->rowv[i * MAX(1, w->count)*2] = cd->variantv[vi]->rowv[i];
			break;
		case '/':
			v = _copyVariant(NULL, MAX(0, (cd->variantv[vi]->rowc+1)/(2*MAX(1, w->count)) - 1));
			for (int i = 0; i <= v->rowc; i++)
				v->rowv[i] = cd->variantv[vi]->rowv[i * MAX(1, w->count)*2];
			break;
	} free(cd->variantv[vi]); cd->variantv[vi] = v;
	if (!(cd->trig[w->trackerfy - gcvret].flags&C_VTRIG_LOOP)
			&& w->trackerfy > w->trackerfy - gcvret + cd->variantv[vi]->rowc)
		w->trackerfy = w->trackerfy - gcvret + cd->variantv[vi]->rowc;
}

void chordMacro(channeldata *cd, int input)
{
	switch (input)
	{
		case 'a': /* add */
			for (int i = 0; i < MAX(1, w->count); i++)
				if (cd->macroc < 7) cd->macroc++;
			break;
		case 'd': /* delete */
			for (int i = 0; i < MAX(1, w->count); i++)
			{
				if (cd->macroc) cd->macroc--;
				if (w->trackerfx > 2+cd->macroc * 2)
					w->trackerfx = 2+cd->macroc * 2;
			} break;
		case 'm': /* set */
			if (w->count) cd->macroc = MIN(8, w->count) - 1;
			else          cd->macroc = 1;
			break;
	}
}

void chordChannel(channeldata *cd, int input)
{
	switch (input)
	{
		case 'c': /* clear */ clearChanneldata(s, cd); break;
		case 'a': /* add */
			for (int i = 0; i < MAX(1, w->count); i++)
			{
				if (s->channelc >= CHANNEL_MAX) break;
				addChannel(s, w->channel+1);
				w->channel++;
			} break;
		case 'A': /* add before */
			for (int i = 0; i < MAX(1, w->count); i++)
			{
				if (s->channelc >= CHANNEL_MAX) break;
				addChannel(s, w->channel);
			} break;
		case 'd': /* delete */
			for (int i = 0; i < MAX(1, w->count); i++)
			{
				delChannel(w->channel);
				if (w->channel > s->channelc-1)
					w->channel = s->channelc-1;
			} break;
		case 'D': /* delete to end */ /* ignores w->count */
			if (w->channel == 0) w->channel++;
			for (uint8_t i = s->channelc; i > w->channel; i--)
				delChannel(i - 1);
			w->channel--;
			break;
		case 'y': /* yank */ copyChanneldata(&w->channelbuffer, cd); redraw(); break;
		case 'p': /* put  */
			copyChanneldata(cd, &w->channelbuffer);
			for (int i = 1; i < MAX(1, w->count); i++)
			{
				if (s->channelc >= CHANNEL_MAX) break;
				w->channel++;
				copyChanneldata(&s->channelv[w->channel].data, &w->channelbuffer);
			} break;
	}
}

void chordLoop(channeldata *cd, int input)
{
	variant *v;
	int gcvret = getChannelVariant(&v, cd, w->trackerfy);
	switch (input)
	{
		case ';': /* toggle loop */
			if (gcvret == -1)
			{ /* not in a variant */
				if (s->loop[0] == w->trackerfy
				 && s->loop[1] == MIN(s->songlen-1, w->trackerfy + 4*s->rowhighlight - 1))
				{
					s->loop[0] = STATE_ROWS;
					s->loop[1] = s->songlen-1;
				} else
				{
					s->loop[0] = w->trackerfy;
					s->loop[1] = MIN(s->songlen-1, w->trackerfy + 4*s->rowhighlight - 1);
				}
			} else
			{ /* in a variant */
				if (s->loop[0] == w->trackerfy - gcvret
				 && s->loop[1] == MIN(s->songlen-1, w->trackerfy - gcvret + v->rowc))
				{
					s->loop[0] = STATE_ROWS;
					s->loop[1] = s->songlen-1;
				} else
				{
					s->loop[0] = w->trackerfy - gcvret;
					s->loop[1] = MIN(s->songlen-1, w->trackerfy - gcvret + v->rowc);
				}
			} break;
		case '+': case '*': s->loop[1] = MIN(s->songlen-1, s->loop[0] + ((s->loop[1] - s->loop[0])<<1) + 1); break;
		case '-': case '/': s->loop[1] = s->loop[0] + ((s->loop[1] - s->loop[0])>>1); break;
		case 'a': s->loop[1] = MIN(s->songlen-1, s->loop[1] + 1); break;
		case 'd': s->loop[1] = MAX(s->loop[0], s->loop[1] - 1); break;
	}
}
