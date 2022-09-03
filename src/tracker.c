#define T_MODE_NORMAL 0
#define T_MODE_INSERT 1
#define T_MODE_MOUSEADJUST 2
#define T_MODE_VISUAL 3
#define T_MODE_VISUALLINE 4
#define T_MODE_VISUALREPLACE 5
#define T_MODE_SONG 6
#define T_MODE_SONG_INSERT 7
#define T_MODE_SONG_VISUAL 8
#define T_MODE_SONG_MOUSEADJUST 9

#include "trackerdraw.c"

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

void prunePattern(uint8_t index)
{
	if (index == PATTERN_VOID || s->patterni[index] == PATTERN_VOID) return;

	/* don't remove if pattern is still referenced */
	for (short i = 0; i < SONG_MAX; i++) if (s->songi[i] == index) return;

	pattern *pv = s->patternv[s->patterni[index]];
	/* don't remove if pattern is populated */
	for (short c = 0; c < s->channelc; c++)
		for (short i = 0; i < pv->rowcc[c] + 1; i++)
		{
			row r = pv->rowv[c][i];
			if (r.note != NOTE_VOID || r.macro[0].c || r.macro[1].c) return;
		}
	delPattern(index);
}
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
	uint8_t modulorow = w->trackerfy % (s->patternv[s->patterni[s->songi[w->songfy]]]->rowcc[w->channel]+1);
	row *r = &s->patternv[s->patterni[s->songi[w->songfy]]]->rowv[w->channel][modulorow];
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
	short macro;
	uint8_t modulorow = w->trackerfy % (s->patternv[s->patterni[s->songi[w->songfy]]]->rowcc[w->channel]+1);
	row *r = &s->patternv[s->patterni[s->songi[w->songfy]]]->rowv[w->channel][modulorow];
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

void trackerUpArrow(void)
{
	int i;
	if (w->flags&W_FLAG_FOLLOW) w->flags ^= W_FLAG_FOLLOW;
	switch (w->mode)
	{
		case T_MODE_SONG: case T_MODE_SONG_INSERT: case T_MODE_SONG_VISUAL:
			w->songfy -= MAX(1, w->count);
			if (w->songfy < 0) w->songfy = 0;
			if (s->songi[w->songfy] != PATTERN_VOID
					&& w->trackerfy > s->patternv[s->patterni[s->songi[w->songfy]]]->rowc)
				w->trackerfy = s->patternv[s->patterni[s->songi[w->songfy]]]->rowc;
			break;
		default:
			for (i = 0; i < MAX(1, w->count); i++)
			{
				if (s->songi[w->songfy] == PATTERN_VOID) return;
				w->trackerfy -= MAX(1, w->step);
				if (w->trackerfy > s->patternv[s->patterni[s->songi[w->songfy]]]->rowc)
					w->trackerfy = s->patternv[s->patterni[s->songi[w->songfy]]]->rowc;
				else if (w->trackerfy < 0)
				{
					if (w->songfy > 0 && s->songi[w->songfy - 1] != PATTERN_VOID)
					{
						w->songfy--;
						w->trackerfy = s->patternv[s->patterni[s->songi[w->songfy]]]->rowc;
					} else w->trackerfy = 0;
				}
			} break;
	}
}
void trackerDownArrow(void)
{
	int i;
	if (w->flags&W_FLAG_FOLLOW) w->flags ^= W_FLAG_FOLLOW;
	switch (w->mode)
	{
		case T_MODE_SONG: case T_MODE_SONG_INSERT: case T_MODE_SONG_VISUAL:
			w->songfy += MAX(1, w->count);
			if (w->songfy > SONG_MAX-1) w->songfy = SONG_MAX-1;
			if (s->songi[w->songfy] != PATTERN_VOID
					&& w->trackerfy > s->patternv[s->patterni[s->songi[w->songfy]]]->rowc)
				w->trackerfy = s->patternv[s->patterni[s->songi[w->songfy]]]->rowc;
			break;
		default:
			for (i = 0; i < MAX(1, w->count); i++)
			{
				if (s->songi[w->songfy] == PATTERN_VOID) return;
				w->trackerfy += MAX(1, w->step);
				if (w->trackerfy < 0) w->trackerfy = 0;
				else if (w->trackerfy > s->patternv[s->patterni[s->songi[w->songfy]]]->rowc)
				{
					if (w->songfy < SONG_MAX-1 && s->songi[w->songfy + 1] != PATTERN_VOID)
					{
						w->trackerfy = 0;
						w->songfy++;
					} else w->trackerfy = s->patternv[s->patterni[s->songi[w->songfy]]]->rowc;
				}
			} break;
	}
}
void cycleUp(void)
{
	short bound;
	pattern *pv;
	row hold;
	int i, j;
	switch (w->mode)
	{
		case T_MODE_SONG: case T_MODE_SONG_INSERT: case T_MODE_SONG_VISUAL:
			bound = w->songfy;
			for (i = 0; i < MAX(1, w->count); i++)
			{
				for (j = bound; j < SONG_MAX-1; j++)
				{
					s->songi[j] = s->songi[j+1];
					s->songf[j] = s->songf[j+1];
				} s->songi[SONG_MAX-1] = PATTERN_VOID; s->songf[SONG_MAX-1] = 0;
			} break;
		case T_MODE_NORMAL: case T_MODE_INSERT:
			if (s->songi[w->songfy] == PATTERN_VOID) return;
			bound = w->trackerfy%(s->patternv[s->patterni[s->songi[w->songfy]]]->rowcc[w->channel]+1);
			for (i = 0; i < MAX(1, w->count); i++)
			{
				pv = s->patternv[s->patterni[s->songi[w->songfy]]];
				hold = pv->rowv[w->channel][bound]; /* hold the first row */
				for (j = bound; j < pv->rowcc[w->channel]; j++)
					pv->rowv[w->channel][j] = pv->rowv[w->channel][j+1];
				pv->rowv[w->channel][pv->rowcc[w->channel]] = hold;
			} break;
		case T_MODE_VISUAL: case T_MODE_VISUALREPLACE:
			cycleUpPartPattern(MIN(tfxToVfx(w->trackerfx), w->visualfx), MAX(tfxToVfx(w->trackerfx), w->visualfx), MIN(w->trackerfy, w->visualfy), MAX(w->trackerfy, w->visualfy), MIN(w->channel, w->visualchannel), MAX(w->channel, w->visualchannel));
			redraw(); break;
		case T_MODE_VISUALLINE:
			cycleUpPartPattern(0, 2+s->channelv[w->channel].macroc, MIN(w->trackerfy, w->visualfy), MAX(w->trackerfy, w->visualfy), MIN(w->channel, w->visualchannel), MAX(w->channel, w->visualchannel));
			redraw(); break;
	}
}
void cycleDown(void)
{
	short bound;
	pattern *pv;
	row hold;
	int i, j;
	switch (w->mode)
	{
		case T_MODE_SONG: case T_MODE_SONG_INSERT: case T_MODE_SONG_VISUAL:
			bound = w->songfy;
			for (i = 0; i < MAX(1, w->count); i++)
			{
				for (j = 254; j >= bound; j--) /* TODO: magic number */
				{
					s->songi[j+1] = s->songi[j];
					s->songf[j+1] = s->songf[j];
				} s->songi[bound] = PATTERN_VOID; s->songf[bound] = 0;
			} break;
		case T_MODE_NORMAL: case T_MODE_INSERT:
			if (s->songi[w->songfy] == PATTERN_VOID) return;
			bound = w->trackerfy%(s->patternv[s->patterni[s->songi[w->songfy]]]->rowcc[w->channel]+1);
			for (i = 0; i < MAX(1, w->count); i++)
			{
				pv = s->patternv[s->patterni[s->songi[w->songfy]]];
				hold = pv->rowv[w->channel][pv->rowcc[w->channel]]; /* hold the last row */
				for (j = pv->rowcc[w->channel] - 1; j >= bound; j--)
					pv->rowv[w->channel][j+1] = pv->rowv[w->channel][j];
				pv->rowv[w->channel][bound] = hold;
			} break;
		case T_MODE_VISUAL: case T_MODE_VISUALREPLACE:
			cycleDownPartPattern(MIN(tfxToVfx(w->trackerfx), w->visualfx), MAX(tfxToVfx(w->trackerfx), w->visualfx), MIN(w->trackerfy, w->visualfy), MAX(w->trackerfy, w->visualfy), MIN(w->channel, w->visualchannel), MAX(w->channel, w->visualchannel));
			redraw(); break;
		case T_MODE_VISUALLINE:
			cycleDownPartPattern(0, 2+s->channelv[w->channel].macroc, MIN(w->trackerfy, w->visualfy), MAX(w->trackerfy, w->visualfy), MIN(w->channel, w->visualchannel), MAX(w->channel, w->visualchannel));
			redraw(); break;
	}
}
void trackerLeftArrow(void)
{
	if (w->flags&W_FLAG_FOLLOW) w->flags ^= W_FLAG_FOLLOW;
	switch (w->mode)
	{
		case T_MODE_SONG: case T_MODE_SONG_INSERT: case T_MODE_SONG_VISUAL: break;
		default:
			for (int i = 0; i < MAX(1, w->count); i++)
			{
				if (s->songi[w->songfy] == PATTERN_VOID) return;
				w->trackerfx--;
				if (w->trackerfx < 0)
				{
					if (w->channel > 0)
					{
						w->channel--;
						w->trackerfx = 2 + s->channelv[w->channel].macroc * 2;
					} else w->trackerfx = 0;
				}
			} break;
	}
}
void channelLeft(void)
{
	if (w->flags&W_FLAG_FOLLOW) w->flags ^= W_FLAG_FOLLOW;
	switch (w->mode)
	{
		case T_MODE_SONG: case T_MODE_SONG_INSERT: case T_MODE_SONG_VISUAL: break;
		default:
			for (int i = 0; i < MAX(1, w->count); i++)
			{
				if (s->songi[w->songfy] != PATTERN_VOID)
				{
					if (w->channel > 0)
					{
						w->channel--;
						if (w->trackerfx > 2 + s->channelv[w->channel].macroc * 2)
							w->trackerfx = 2 + s->channelv[w->channel].macroc * 2;
					}
				}
			} break;
	}
}
void trackerRightArrow(void)
{
	if (w->flags&W_FLAG_FOLLOW) w->flags ^= W_FLAG_FOLLOW;
	switch (w->mode)
	{
		case T_MODE_SONG: case T_MODE_SONG_INSERT: case T_MODE_SONG_VISUAL: break;
		default:
			for (int i = 0; i < MAX(1, w->count); i++)
			{
				if (s->songi[w->songfy] == PATTERN_VOID) return;
				w->trackerfx++;
				if (w->trackerfx > 2 + s->channelv[w->channel].macroc * 2)
				{
					if (w->channel < s->channelc - 1)
					{
						w->channel++;
						w->trackerfx = 0;
					} else w->trackerfx = 2 + s->channelv[w->channel].macroc * 2;
				}
			} break;
	}
}
void channelRight(void)
{
	if (w->flags&W_FLAG_FOLLOW) w->flags ^= W_FLAG_FOLLOW;
	switch (w->mode)
	{
		case T_MODE_SONG: case T_MODE_SONG_INSERT: case T_MODE_SONG_VISUAL: break;
		default:
			for (int i = 0; i < MAX(1, w->count); i++)
			{
				if (s->songi[w->songfy] == PATTERN_VOID) return;
				if (w->channel < s->channelc - 1)
				{
					w->channel++;
					if (w->trackerfx > 2 + s->channelv[w->channel].macroc * 2)
						w->trackerfx = 2 + s->channelv[w->channel].macroc * 2;
				}
			} break;
	}
}
void trackerHome(void)
{
	if (w->flags&W_FLAG_FOLLOW) w->flags ^= W_FLAG_FOLLOW;
	switch (w->mode)
	{
		case T_MODE_SONG: case T_MODE_SONG_INSERT: case T_MODE_SONG_VISUAL:
			w->songfy = 0;
			if (s->songi[w->songfy] != PATTERN_VOID
					&& w->trackerfy > s->patternv[s->patterni[s->songi[w->songfy]]]->rowc)
				w->trackerfy = s->patternv[s->patterni[s->songi[w->songfy]]]->rowc;
			break;
		default:
			w->trackerfy = 0;
			break;
	}
}
void trackerEnd(void)
{
	if (w->flags&W_FLAG_FOLLOW) w->flags ^= W_FLAG_FOLLOW;
	switch (w->mode)
	{
		case T_MODE_SONG: case T_MODE_SONG_INSERT: case T_MODE_SONG_VISUAL:
			w->songfy = SONG_MAX-1;
			if (s->songi[w->songfy] != PATTERN_VOID
					&& w->trackerfy > s->patternv[s->patterni[s->songi[w->songfy]]]->rowc)
				w->trackerfy = s->patternv[s->patterni[s->songi[w->songfy]]]->rowc;
			break;
		default:
			w->trackerfy = s->patternv[s->patterni[s->songi[w->songfy]]]->rowc;
			break;
	}
}
void trackerPageUp(void)
{
	switch (w->mode)
	{
		case T_MODE_SONG: case T_MODE_SONG_INSERT: case T_MODE_SONG_VISUAL:
			w->songfy -= s->rowhighlight * MAX(1, w->count);
			if (w->songfy < 0) w->songfy = 0;
			if (s->songi[w->songfy] != PATTERN_VOID
					&& w->trackerfy > s->patternv[s->patterni[s->songi[w->songfy]]]->rowc)
				w->trackerfy = s->patternv[s->patterni[s->songi[w->songfy]]]->rowc;
			break;
		default:
			for (int i = 0; i < MAX(1, w->count); i++)
			{
				if (s->songi[w->songfy] == PATTERN_VOID) break;
				w->trackerfy -= s->rowhighlight;
				while (w->trackerfy < 0)
				{
					if (w->songfy > 0 && s->songi[w->songfy - 1] != PATTERN_VOID)
					{
						w->songfy--;
						w->trackerfy += s->patternv[s->patterni[s->songi[w->songfy]]]->rowc + 1;
					} else w->trackerfy = 0;
				}
			} break;
	}
}
void trackerPageDown(void)
{
	switch (w->mode)
	{
		case T_MODE_SONG: case T_MODE_SONG_INSERT: case T_MODE_SONG_VISUAL:
			w->songfy += s->rowhighlight * MAX(1, w->count);
			if (w->songfy > SONG_MAX-1) w->songfy = SONG_MAX-1;
			if (s->songi[w->songfy] != PATTERN_VOID
					&& w->trackerfy > s->patternv[s->patterni[s->songi[w->songfy]]]->rowc)
				w->trackerfy = s->patternv[s->patterni[s->songi[w->songfy]]]->rowc;
			break;
		default:
			for (int i = 0; i < MAX(1, w->count); i++)
			{
				if (s->songi[w->songfy] == PATTERN_VOID) break;
				w->trackerfy += s->rowhighlight;
				while (w->trackerfy > s->patternv[s->patterni[s->songi[w->songfy]]]->rowc)
				{
					if (w->songfy < SONG_MAX && s->songi[w->songfy + 1] != PATTERN_VOID)
					{
						w->songfy++;
						w->trackerfy -= s->patternv[s->patterni[s->songi[w->songfy]]]->rowc + 1;
					} else w->trackerfy = s->patternv[s->patterni[s->songi[w->songfy]]]->rowc;
				}
			} break;
	}
}

void inputSongHex(char value)
{
	uint8_t oldpattern = s->songi[w->songfy];
	if (s->songi[w->songfy] == PATTERN_VOID)
		s->songi[w->songfy] = 0;
	s->songi[w->songfy] <<= 4; s->songi[w->songfy] += value;
	prunePattern(oldpattern);
	addPattern(s->songi[w->songfy]);
	redraw();
}

void insertNote(row *r, int key)
{
	if (w->instrumentrecv == INST_REC_LOCK_OK)
		r->inst = w->instrument;
	switch (w->keyboardmacro)
	{
		case 0: charToNote(key, &r->note); break;
		case 'G': /* m.x and m.y are note */
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
					case 'G': r->macro[macro].v += MAX(1, w->count)*16;
					default:  r->macro[macro].v += MAX(1, w->count);
				} break;
			case 24: /* ^x */
				switch (r->macro[macro].c)
				{
					case 'G': r->macro[macro].v -= MAX(1, w->count)*16;
					default:  r->macro[macro].v -= MAX(1, w->count);
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
	s->channelv[channel].flags ^= C_FLAG_MUTE;
	if (s->channelv[channel].flags&C_FLAG_MUTE && w->instrumentlockv == INST_GLOBAL_LOCK_OK)
	{
		w->instrumentlocki = channel;
		w->instrumentlockv = INST_GLOBAL_CHANNEL_MUTE;
	}
}

void leaveSpecialModes(void)
{
	switch (w->mode)
	{
		case T_MODE_INSERT: case T_MODE_SONG: case T_MODE_SONG_INSERT: break;
		case T_MODE_SONG_VISUAL: w->mode = T_MODE_SONG; break;
		default:                 w->mode = T_MODE_NORMAL; break;
	}
}
void trackerInput(int input)
{
	int button, x, y, i, j, k;
	short macro;
	row *r;
	pattern *pv;
	uint8_t modulorow, oldpattern;
	switch (input)
	{
		case '\033': /* escape */
			switch (getchar())
			{
				case 'O':
					previewNote(' ', INST_VOID, w->channel);
					switch (getchar())
					{
						case 'P': /* xterm f1 */ showTracker(); break;
						case 'Q': /* xterm f2 */ showInstrument(); break;
					} redraw(); break;
				case '[':
					previewNote(' ', INST_VOID, w->channel);
					switch (getchar())
					{
						case '[':
							switch (getchar())
							{
								case 'A': /* linux f1 */ showTracker(); break;
								case 'B': /* linux f2 */ showInstrument(); break;
								case 'E': /* linux f5 */ leaveSpecialModes(); startPlayback(); break;
							} redraw(); break;
						case 'A': /* up arrow    */ trackerUpArrow(); redraw(); break;
						case 'B': /* down arrow  */ trackerDownArrow(); redraw(); break;
						case 'D': /* left arrow  */ trackerLeftArrow(); redraw(); break;
						case 'C': /* right arrow */ trackerRightArrow(); redraw(); break;
						case '1': /* mod+arrow / f5 - f8 */
							switch (getchar())
							{
								case '5': /* xterm f5 */ leaveSpecialModes(); startPlayback(); getchar(); break;
								case '7': /* f6       */ leaveSpecialModes(); stopPlayback(); getchar(); break;
								case ';': /* mod+arrow */
									switch (getchar())
									{
										case '5': /* ctrl+arrow */
											switch (getchar())
											{
												case 'D': /* left  */ channelLeft(); redraw(); break;
												case 'C': /* right */ channelRight(); redraw(); break;
												case 'A': /* up    */ cycleUp(); redraw(); break;
												case 'B': /* down  */ cycleDown(); redraw(); break;
											} break;
										default: getchar(); break;
									} break;
								case '~': /* linux home */ trackerHome(); redraw(); break;
							} break;
						case 'H': /* xterm home */ trackerHome(); redraw(); break;
						case '4': /* end        */
							if (getchar() == '~') { trackerEnd(); redraw(); }
							break;
						case '5': /* page up   */ trackerPageUp(); getchar(); redraw(); break;
						case '6': /* page down */ trackerPageDown(); getchar(); redraw(); break;
						case 'M': /* mouse */
							button = getchar();
							x = getchar() - 32;
							y = getchar() - 32;

							short oldtrackerfx = w->trackerfx;
							uint8_t oldchannel = w->channel;

							short sfx = 0;
							short tx = 0;
							short otx;
							for (int i = 0; i < s->channelc; i++)
							{
								if (i == w->channel)
								{
									sfx = tx + ((9 + 4*(s->channelv[i].macroc+1) + LINENO_COLS + SONGLIST_COLS)>>1);
									break;
								} tx += 9 + 4*(s->channelv[i].macroc+1);
							} sfx = MIN(0, (ws.ws_col>>1) - sfx);

							switch (button)
							{
								case WHEEL_UP: case WHEEL_UP_CTRL:
									if (w->flags&W_FLAG_FOLLOW) w->flags ^= W_FLAG_FOLLOW;

									if (x < SONGLIST_COLS)
									{
										w->songfy -= WHEEL_SPEED;
										if (w->songfy < 0) w->songfy = 0;
										if (s->songi[w->songfy] != PATTERN_VOID
												&& w->trackerfy > s->patternv[s->patterni[s->songi[w->songfy]]]->rowc)
											w->trackerfy = s->patternv[s->patterni[s->songi[w->songfy]]]->rowc;
									} else
									{
										if (s->songi[w->songfy] == PATTERN_VOID) break;
										w->trackerfy -= WHEEL_SPEED;
										if (w->trackerfy < 0)
										{
											if (w->songfy > 0 && s->songi[w->songfy - 1] != PATTERN_VOID)
											{
												w->songfy--;
												w->trackerfy += s->patternv[s->patterni[s->songi[w->songfy]]]->rowc + 1;
											} else w->trackerfy = 0;
										}
									} break;
								case WHEEL_DOWN: case WHEEL_DOWN_CTRL:
									if (w->flags&W_FLAG_FOLLOW) w->flags ^= W_FLAG_FOLLOW;

									if (x < SONGLIST_COLS)
									{
										w->songfy += WHEEL_SPEED;
										if (w->songfy > SONG_MAX-1) w->songfy = SONG_MAX-1;
										if (s->songi[w->songfy] != PATTERN_VOID
												&& w->trackerfy > s->patternv[s->patterni[s->songi[w->songfy]]]->rowc)
											w->trackerfy = s->patternv[s->patterni[s->songi[w->songfy]]]->rowc;
									} else
									{
										if (s->songi[w->songfy] == PATTERN_VOID) break;
										w->trackerfy += WHEEL_SPEED;
										if (w->trackerfy > s->patternv[s->patterni[s->songi[w->songfy]]]->rowc)
										{
											if (w->songfy < SONG_MAX-1 && s->songi[w->songfy + 1] != PATTERN_VOID)
											{
												w->trackerfy -= s->patternv[s->patterni[s->songi[w->songfy]]]->rowc + 1;
												w->songfy++;
											} else w->trackerfy = s->patternv[s->patterni[s->songi[w->songfy]]]->rowc;
										}
									} break;
								case BUTTON_RELEASE: case BUTTON_RELEASE_CTRL:
									switch (w->mode)
									{
										case T_MODE_SONG: case T_MODE_SONG_INSERT: case T_MODE_SONG_VISUAL:
											w->songfy += w->fyoffset;
											if (w->songfy < 0) w->songfy = 0;
											else if (w->songfy > SONG_MAX-1) w->songfy = SONG_MAX-1;
											break;
										default:
											if (s->songi[w->songfy] == PATTERN_VOID) break;
											w->trackerfy += w->fyoffset;
											while (w->trackerfy < 0)
											{
												if (w->songfy > 0 && s->songi[w->songfy - 1] != PATTERN_VOID)
												{
													w->songfy--;
													w->trackerfy += s->patternv[s->patterni[s->songi[w->songfy]]]->rowc + 1;
												} else w->trackerfy = 0;
											}
											while (w->trackerfy > s->patternv[s->patterni[s->songi[w->songfy]]]->rowc)
											{
												if (w->songfy < SONG_MAX-1 && s->songi[w->songfy + 1] != PATTERN_VOID)
												{
													w->trackerfy -= s->patternv[s->patterni[s->songi[w->songfy]]]->rowc + 1;
													w->songfy++;
												} else w->trackerfy = s->patternv[s->patterni[s->songi[w->songfy]]]->rowc;
											} break;
									} w->fyoffset = w->fieldpointer = 0;

									switch (w->mode)
									{ /* leave mouseadjust mode */
										case T_MODE_MOUSEADJUST: case T_MODE_SONG_MOUSEADJUST:
											w->mode = w->oldmode; break;
									} break;
								case BUTTON1_HOLD:
									if (w->flags&W_FLAG_FOLLOW) w->flags ^= W_FLAG_FOLLOW;
									switch (w->mode)
									{
										case T_MODE_MOUSEADJUST:
											if      (x > w->mousex) trackerAdjustRight();
											else if (x < w->mousex) trackerAdjustLeft();
											break;
										case T_MODE_SONG_MOUSEADJUST:
											if (!s->playing)
											{
												if (x > w->mousex)
												{
													if (w->fieldpointer) s->songi[w->songfy]+=16;
													else                 s->songi[w->songfy]++;
													addPattern(s->songi[w->songfy]);
												}
												else if (x < w->mousex)
												{
													if (w->fieldpointer) s->songi[w->songfy]-=16;
													else                 s->songi[w->songfy]--;
													addPattern(s->songi[w->songfy]);
												}
											} break;
									} w->mousex = x; break;
								default: /* click */
									if (y <= CHANNEL_ROW-2)
									{
										if (x < (ws.ws_col-17) / 2 + 7) showTracker();
										else                            showInstrument();
										break;
									}
									if (y > ws.ws_row - 1) break; /* ignore clicking out of range */
									if (w->flags&W_FLAG_FOLLOW) w->flags ^= W_FLAG_FOLLOW;

									if (x < SONGLIST_COLS)
									{ /* song list */
										switch (w->mode)
										{
											case T_MODE_SONG: case T_MODE_SONG_INSERT: case T_MODE_SONG_VISUAL: break;
											case T_MODE_INSERT: w->mode = T_MODE_SONG_INSERT; break;
											default: w->mode = T_MODE_SONG; break;
										}
										switch (button)
										{
											case BUTTON2: case BUTTON2_CTRL:
												if (s->playing && s->songp == w->songfy + y - w->centre) break;
												if (w->songnext == w->songfy + y - w->centre + 1)
													w->songnext = 0;
												oldpattern = s->songi[w->songfy + y - w->centre];
												s->songi[w->songfy + y - w->centre] = PATTERN_VOID;
												s->songf[w->songfy + y - w->centre] = 0;
												prunePattern(oldpattern);
											case BUTTON1: case BUTTON1_CTRL:
												if (w->mode != T_MODE_SONG_INSERT) w->mode = T_MODE_SONG;
												if (y - w->centre == 0)
												{
													w->oldmode = w->mode;
													w->mode = T_MODE_SONG_MOUSEADJUST;
													w->mousex = x;
													if (x < SONGLIST_COLS - 3) w->fieldpointer = 1;
													else                       w->fieldpointer = 0;
												} break;
											case BUTTON3: case BUTTON3_CTRL:
												if (w->mode != T_MODE_SONG_VISUAL)
												{
													w->visualfy = w->songfy;
													w->mode = T_MODE_SONG_VISUAL;
												} break;
										} w->fyoffset = y - w->centre;
									} else
									{ /* tracker channels */
										switch (w->mode)
										{
											case T_MODE_SONG: case T_MODE_SONG_VISUAL: w->mode = T_MODE_NORMAL; break;
											case T_MODE_SONG_INSERT: w->mode = T_MODE_INSERT; break;
										} if (s->songi[w->songfy] == PATTERN_VOID) break;

										if (y <= CHANNEL_ROW)
										{
											/* oldchannel is used here as a buffer for which channel to mute */
											tx = 0;
											for (int i = 0; i < s->channelc; i++)
											{
												tx += 9 + 4*(s->channelv[i].macroc+1);
												if (tx > x - sfx - SONGLIST_COLS - LINENO_COLS)
												{
													oldchannel = i;
													break;
												}
											} if (tx < x - sfx - SONGLIST_COLS - LINENO_COLS)
												oldchannel = s->channelc - 1;

											if (button == BUTTON1 || button == BUTTON1_CTRL)
											{
												if (w->mode != T_MODE_INSERT) /* suggest mode 0, but allow insert */
													w->mode = T_MODE_NORMAL;
												toggleChannelMute(oldchannel);
											}
										} else if (button == BUTTON1_CTRL) w->step = MIN(15, abs(y - w->centre));
										else
										{
											switch (button)
											{
												case BUTTON1:
													if (w->mode != T_MODE_INSERT) /* suggest mode 0, but allow insert */
														w->mode = T_MODE_NORMAL;
													break;
												case BUTTON3: case BUTTON3_CTRL:
													if (!(w->mode == T_MODE_VISUAL || w->mode == T_MODE_VISUALLINE || w->mode == T_MODE_VISUALREPLACE))
													{
														w->visualfx = tfxToVfx(oldtrackerfx);
														w->visualfy = w->trackerfy;
														w->visualchannel = oldchannel;
														w->mode = T_MODE_VISUAL;
													} break;
											}

											tx = otx = 0;
											int j;
											for (int i = 0; i < s->channelc; i++)
											{
												otx = tx; /* old tx */
												tx += 9 + 4*(s->channelv[i].macroc+1);
												if (tx > x - sfx - SONGLIST_COLS - LINENO_COLS)
												{
													w->channel = i;
													j = (x - sfx - SONGLIST_COLS - LINENO_COLS) - otx;
													if      (j<3) w->trackerfx = 0;
													else if (j<6) w->trackerfx = 1;
													else          w->trackerfx = (j-6)/2 + 2;
													break;
												}
											}
											if (tx < x - sfx - SONGLIST_COLS - LINENO_COLS)
											{
												w->channel = s->channelc - 1;
												w->trackerfx = (s->channelv[w->channel].macroc+2)*2 - 1;
											}
											if (w->trackerfx > (s->channelv[w->channel].macroc+2)*2 - 1)
												w->trackerfx = (s->channelv[w->channel].macroc+2)*2 - 1;

											w->fyoffset = y - w->centre;

											if (button == BUTTON2 || button == BUTTON2_CTRL)
											{
												if (w->trackerfx == 0)
												{
													yankPartPattern(0, 1, w->trackerfy + w->fyoffset, w->trackerfy + w->fyoffset, w->channel, w->channel);
													delPartPattern(0, 1, w->trackerfy + w->fyoffset, w->trackerfy + w->fyoffset, w->channel, w->channel);
												} else
												{
													yankPartPattern(tfxToVfx(w->trackerfx), tfxToVfx(w->trackerfx), w->trackerfy + w->fyoffset, w->trackerfy + w->fyoffset, w->channel, w->channel);
													delPartPattern(tfxToVfx(w->trackerfx), tfxToVfx(w->trackerfx), w->trackerfy + w->fyoffset, w->trackerfy + w->fyoffset, w->channel, w->channel);
												} break;
											}

											/* enter adjust */
											if ((button == BUTTON1 || button == BUTTON1_CTRL)
													&& w->fyoffset == 0 && w->trackerfx == oldtrackerfx && w->channel == oldchannel)
											{
												w->oldmode = w->mode;
												w->mode = T_MODE_MOUSEADJUST;
												w->mousex = x;
												w->fieldpointer = 0;
												switch (w->trackerfx)
												{
													case 1: if (x - (otx + sfx) - LINENO_COLS - SONGLIST_COLS < 5) w->fieldpointer = 1; break;
													default:
														macro = (w->trackerfx - 2)>>1;
														if (w->trackerfx % 2 == 1 && x - (otx + sfx) - LINENO_COLS - SONGLIST_COLS < 9 + 4*macro)
															w->fieldpointer = 1;
														else w->fieldpointer = 0;
														break;
												}
											}
										}
									} break;
							} redraw(); break;
					} break;
				/* alt+numbers, change step if in insert mode */
				case '0': w->step = 0; redraw(); break;
				case '1': w->step = 1; redraw(); break;
				case '2': w->step = 2; redraw(); break;
				case '3': w->step = 3; redraw(); break;
				case '4': w->step = 4; redraw(); break;
				case '5': w->step = 5; redraw(); break;
				case '6': w->step = 6; redraw(); break;
				case '7': w->step = 7; redraw(); break;
				case '8': w->step = 8; redraw(); break;
				case '9': w->step = 9; redraw(); break;
				default: /* escape */
					previewNote(' ', INST_VOID, w->channel);
					switch (w->mode)
					{
						case T_MODE_VISUALREPLACE: w->mode = T_MODE_VISUAL; break;
						case T_MODE_VISUAL: case T_MODE_VISUALLINE: w->mode = T_MODE_NORMAL; break;
						case T_MODE_SONG_INSERT: case T_MODE_SONG_VISUAL: w->mode = T_MODE_SONG; break;
						case T_MODE_SONG: break;
						default: w->mode = T_MODE_NORMAL; break;
					} redraw(); break;
			} break;
		default:
			switch (w->mode)
			{
				case T_MODE_VISUALREPLACE:
					if (input == '\n' || input == '\r')
					{
						toggleChannelMute(w->channel); redraw();
					} else
					{
						switch (w->trackerfx)
						{
							case 0: /* note */
								for (i = MIN(w->trackerfy, w->visualfy); i <= MAX(w->trackerfy, w->visualfy); i++)
								{
									modulorow = i % (s->patternv[s->patterni[s->songi[w->songfy]]]->rowcc[w->channel]+1);
									r = &s->patternv[s->patterni[s->songi[w->songfy]]]->rowv[w->channel][modulorow];
									insertNote(r, input);
								}
								/* <preview> */
								modulorow = w->trackerfy % (s->patternv[s->patterni[s->songi[w->songfy]]]->rowcc[w->channel]+1);
								r = &s->patternv[s->patterni[s->songi[w->songfy]]]->rowv[w->channel][modulorow];
								previewNote(input, r->inst, w->channel);
								/* </preview> */
								break;
							case 1: /* inst */
								for (i = MIN(w->trackerfy, w->visualfy); i <= MAX(w->trackerfy, w->visualfy); i++)
								{
									modulorow = i % (s->patternv[s->patterni[s->songi[w->songfy]]]->rowcc[w->channel]+1);
									r = &s->patternv[s->patterni[s->songi[w->songfy]]]->rowv[w->channel][modulorow];
									insertInst(r, input);
								} break;
							default: /* macro */
								macro = (w->trackerfx - 2)>>1;
								if (!(w->trackerfx%2))
								{ /* macroc */
									for (i = MIN(w->trackerfy, w->visualfy); i <= MAX(w->trackerfy, w->visualfy); i++)
									{
										modulorow = i % (s->patternv[s->patterni[s->songi[w->songfy]]]->rowcc[w->channel]+1);
										r = &s->patternv[s->patterni[s->songi[w->songfy]]]->rowv[w->channel][modulorow];
										insertMacroc(r, macro, input);
									}
								} else
								{ /* macrov */
									for (i = MIN(w->trackerfy, w->visualfy); i <= MAX(w->trackerfy, w->visualfy); i++)
									{
										modulorow = i % (s->patternv[s->patterni[s->songi[w->songfy]]]->rowcc[w->channel]+1);
										r = &s->patternv[s->patterni[s->songi[w->songfy]]]->rowv[w->channel][modulorow];
										if (w->keyboardmacro)
											switch (r->macro[macro].c ? r->macro[macro].c : w->keyboardmacro)
											{
												case 0: insertMacrov(r, macro, input); break;
												case 'G':
													if (charToKmode(input, 1, &r->macro[macro].v, NULL) && !r->macro[macro].c)
														r->macro[macro].c = w->keyboardmacro;
													break;
												default:
													if (charToKmode(input, 0, &r->macro[macro].v, NULL) && !r->macro[macro].c)
														r->macro[macro].c = w->keyboardmacro;
													break;
											}
										else insertMacrov(r, macro, input);
									}
								} break;
						} redraw(); break;
					}
				case T_MODE_VISUAL: case T_MODE_VISUALLINE:
					switch (input)
					{
						case '\n': case '\r': toggleChannelMute(w->channel); redraw(); break;
						case 'v': case 22: /* visual */
							switch (w->mode)
							{
								case T_MODE_VISUAL:     w->mode = T_MODE_NORMAL; redraw(); break;
								case T_MODE_VISUALLINE: w->mode = T_MODE_VISUAL; redraw(); break;
							} break;
						case 'V': /* visual line */
							switch (w->mode)
							{
								case T_MODE_VISUAL:     w->mode = T_MODE_VISUALLINE; redraw(); break;
								case T_MODE_VISUALLINE: w->mode = T_MODE_NORMAL; redraw(); break;
							} break;
						case 'r': /* replace     */ w->mode = T_MODE_VISUALREPLACE; redraw(); break;;
						case 'k': /* up arrow    */ trackerUpArrow(); redraw(); break;
						case 'j': /* down arrow  */ trackerDownArrow(); redraw(); break;
						case 'h': /* left arrow  */ trackerLeftArrow(); redraw(); break;
						case 'l': /* right arrow */ trackerRightArrow(); redraw(); break;
						case '[': /* chnl left   */ channelLeft(); redraw(); break;
						case ']': /* chnl right  */ channelRight(); redraw(); break;
						case '{': /* cycle up    */ cycleUp(); redraw(); break;
						case '}': /* cycle down  */ cycleDown(); redraw(); break;
						case '~': /* vi tilde */
							switch (w->mode)
							{
								case T_MODE_VISUAL:     tildePartPattern(MIN(tfxToVfx(w->trackerfx), w->visualfx), MAX(tfxToVfx(w->trackerfx), w->visualfx), MIN(w->trackerfy, w->visualfy), MAX(w->trackerfy, w->visualfy), MIN(w->channel, w->visualchannel), MAX(w->channel, w->visualchannel)); redraw(); break;
								case T_MODE_VISUALLINE: tildePartPattern(0, 2+s->channelv[w->channel].macroc, MIN(w->trackerfy, w->visualfy), MAX(w->trackerfy, w->visualfy), MIN(w->channel, w->visualchannel), MAX(w->channel, w->visualchannel)); redraw(); break;
							} break;
						case 'i': /* interpolate */
							switch (w->mode)
							{
								case T_MODE_VISUAL:     interpolatePartPattern(MIN(tfxToVfx(w->trackerfx), w->visualfx), MAX(tfxToVfx(w->trackerfx), w->visualfx), MIN(w->trackerfy, w->visualfy), MAX(w->trackerfy, w->visualfy), MIN(w->channel, w->visualchannel), MAX(w->channel, w->visualchannel)); redraw(); break;
								case T_MODE_VISUALLINE: interpolatePartPattern(0, 2+s->channelv[w->channel].macroc, MIN(w->trackerfy, w->visualfy), MAX(w->trackerfy, w->visualfy), MIN(w->channel, w->visualchannel), MAX(w->channel, w->visualchannel)); redraw(); break;
							} break;
						case '%': /* random */
							switch (w->mode)
							{
								case T_MODE_VISUAL:     randPartPattern(MIN(tfxToVfx(w->trackerfx), w->visualfx), MAX(tfxToVfx(w->trackerfx), w->visualfx), MIN(w->trackerfy, w->visualfy), MAX(w->trackerfy, w->visualfy), MIN(w->channel, w->visualchannel), MAX(w->channel, w->visualchannel)); redraw(); break;
								case T_MODE_VISUALLINE: randPartPattern(0, 2+s->channelv[w->channel].macroc, MIN(w->trackerfy, w->visualfy), MAX(w->trackerfy, w->visualfy), MIN(w->channel, w->visualchannel), MAX(w->channel, w->visualchannel)); redraw(); break;
							} break;
						case 1: /* ^a */
							switch (w->mode)
							{
								case T_MODE_VISUAL:     addPartPattern(MAX(1, w->count), MIN(tfxToVfx(w->trackerfx), w->visualfx), MAX(tfxToVfx(w->trackerfx), w->visualfx), MIN(w->trackerfy, w->visualfy), MAX(w->trackerfy, w->visualfy), MIN(w->channel, w->visualchannel), MAX(w->channel, w->visualchannel)); redraw(); break;
								case T_MODE_VISUALLINE: addPartPattern(MAX(1, w->count), 0, 2+s->channelv[w->channel].macroc, MIN(w->trackerfy, w->visualfy), MAX(w->trackerfy, w->visualfy), MIN(w->channel, w->visualchannel), MAX(w->channel, w->visualchannel)); redraw(); break;
							} break;
						case 24: /* ^x */
							switch (w->mode)
							{
								case T_MODE_VISUAL:     addPartPattern(-MAX(1, w->count), MIN(tfxToVfx(w->trackerfx), w->visualfx), MAX(tfxToVfx(w->trackerfx), w->visualfx), MIN(w->trackerfy, w->visualfy), MAX(w->trackerfy, w->visualfy), MIN(w->channel, w->visualchannel), MAX(w->channel, w->visualchannel)); redraw(); break;
								case T_MODE_VISUALLINE: addPartPattern(-MAX(1, w->count), 0, 2+s->channelv[w->channel].macroc, MIN(w->trackerfy, w->visualfy), MAX(w->trackerfy, w->visualfy), MIN(w->channel, w->visualchannel), MAX(w->channel, w->visualchannel)); redraw(); break;
							} break;
						case 'x': case 'd': /* pattern cut */
							if (s->songi[w->songfy] == PATTERN_VOID) break;
							switch (w->mode)
							{
								case T_MODE_VISUAL:
									yankPartPattern(MIN(tfxToVfx(w->trackerfx), w->visualfx), MAX(tfxToVfx(w->trackerfx), w->visualfx), MIN(w->trackerfy, w->visualfy), MAX(w->trackerfy, w->visualfy), MIN(w->channel, w->visualchannel), MAX(w->channel, w->visualchannel));
									delPartPattern(MIN(tfxToVfx(w->trackerfx), w->visualfx), MAX(tfxToVfx(w->trackerfx), w->visualfx), MIN(w->trackerfy, w->visualfy), MAX(w->trackerfy, w->visualfy), MIN(w->channel, w->visualchannel), MAX(w->channel, w->visualchannel));
									break;
								case T_MODE_VISUALLINE:
									yankPartPattern(0, 2+s->channelv[w->channel].macroc, MIN(w->trackerfy, w->visualfy), MAX(w->trackerfy, w->visualfy), MIN(w->channel, w->visualchannel), MAX(w->channel, w->visualchannel));
									delPartPattern(0, 2+s->channelv[w->channel].macroc, MIN(w->trackerfy, w->visualfy), MAX(w->trackerfy, w->visualfy), MIN(w->channel, w->visualchannel), MAX(w->channel, w->visualchannel));
									break;
							}
							w->trackerfx = MIN(w->trackerfx, vfxToTfx(w->visualfx));
							w->trackerfy = MIN(w->trackerfy, w->visualfy);
							w->channel = MIN(w->channel, w->visualchannel);
							w->mode = T_MODE_NORMAL;
							redraw(); break;
						case 'y': /* pattern copy */
							if (s->songi[w->songfy] == PATTERN_VOID) break;
							switch (w->mode)
							{
								case T_MODE_VISUAL:     yankPartPattern(MIN(tfxToVfx(w->trackerfx), w->visualfx), MAX(tfxToVfx(w->trackerfx), w->visualfx), MIN(w->trackerfy, w->visualfy), MAX(w->trackerfy, w->visualfy), MIN(w->channel, w->visualchannel), MAX(w->channel, w->visualchannel)); break;
								case T_MODE_VISUALLINE: yankPartPattern(0, 2+s->channelv[w->channel].macroc, MIN(w->trackerfy, w->visualfy), MAX(w->trackerfy, w->visualfy), MIN(w->channel, w->visualchannel), MAX(w->channel, w->visualchannel)); break;
							}
							w->trackerfx = MIN(w->trackerfx, vfxToTfx(w->visualfx));
							w->trackerfy = MIN(w->trackerfy, w->visualfy);
							w->channel = MIN(w->channel, w->visualchannel);
							w->mode = T_MODE_NORMAL;
							redraw(); break;
					} break;
				case T_MODE_SONG:
					switch (input)
					{ /* set count first */
						case '0': w->count *= 10; w->count += 0; redraw(); return;
						case '1': w->count *= 10; w->count += 1; redraw(); return;
						case '2': w->count *= 10; w->count += 2; redraw(); return;
						case '3': w->count *= 10; w->count += 3; redraw(); return;
						case '4': w->count *= 10; w->count += 4; redraw(); return;
						case '5': w->count *= 10; w->count += 5; redraw(); return;
						case '6': w->count *= 10; w->count += 6; redraw(); return;
						case '7': w->count *= 10; w->count += 7; redraw(); return;
						case '8': w->count *= 10; w->count += 8; redraw(); return;
						case '9': w->count *= 10; w->count += 9; redraw(); return;
						default:
							if (w->chord)
							{
								w->count = MIN(256, w->count);
								switch (w->chord)
								{
									case 'K': /* keyboard modes */
										w->keyboardmacro = '\0';
										if (input != 'k' && input != 'K') changeMacro(input, &w->keyboardmacro);
										break;
								} w->count = 0; redraw();
							} else
								switch (input)
								{
									case '\t': /* leave song mode    */ w->mode = T_MODE_NORMAL; redraw(); break;
									case 'f':  /* toggle song follow */ w->flags ^= W_FLAG_FOLLOW; redraw(); break;
									case 'K':  /* keyboard macro     */ w->chord = 'K'; redraw(); return;
									case 'k':  /* up arrow           */ trackerUpArrow(); redraw(); break;
									case 'j':  /* down arrow         */ trackerDownArrow(); redraw(); break;
									case 'h':  /* left arrow         */ trackerLeftArrow(); redraw(); break;
									case 'l':  /* right arrow        */ trackerRightArrow(); redraw(); break;
									case '[':  /* chnl left          */ channelLeft(); redraw(); break;
									case ']':  /* chnl right         */ channelRight(); redraw(); break;
									case '{':  /* cycle up           */ cycleUp(); redraw(); break;
									case '}':  /* cycle down         */ cycleDown(); redraw(); break;
									case 'b':  /* bpm                */ if (w->count) s->songbpm = MIN(255, MAX(32, w->count)); w->request = REQ_BPM; redraw(); break;
									case 't':  /* row highlight      */ if (w->count) s->rowhighlight = MIN(16, w->count); redraw(); break;
									case 's':  /* step               */ w->step = MIN(15, w->count); redraw(); break;
									case 'o':  /* octave             */ w->octave = MIN(9, w->count); redraw(); break;
									case 'i':          /* enter insert mode */ w->mode = T_MODE_SONG_INSERT; redraw(); break;
									case 'v': case 22: /* enter visual mode */
										w->visualfy = w->songfy;
										w->mode = T_MODE_SONG_VISUAL;
										redraw(); break;
									case 1: /* ^a */
										if (s->playing && s->songp == w->songfy) break;
										oldpattern = s->songi[w->songfy];
										s->songi[w->songfy]+=MAX(1, w->count);
										prunePattern(oldpattern);
										addPattern(s->songi[w->songfy]);
										redraw(); break;
									case 24: /* ^x */
										if (s->playing && s->songp == w->songfy) break;
										oldpattern = s->songi[w->songfy];
										s->songi[w->songfy]-=MAX(1, w->count);
										prunePattern(oldpattern);
										addPattern(s->songi[w->songfy]);
										if (s->songi[w->songfy] == PATTERN_VOID && w->songnext == w->songfy + 1)
											w->songnext = 0;
										redraw(); break;
									case '?': /* loop */ if (s->songi[w->songfy] != PATTERN_VOID) s->songf[w->songfy] = !s->songf[w->songfy]; redraw(); break;
									case '>': /* next */
										if (s->songi[w->songfy] != PATTERN_VOID)
										{
											if (w->songnext == w->songfy + 1) w->songnext = 0;
											else                              w->songnext = w->songfy + 1;
										} redraw(); break;
									case 'a': /* empty (add new) */
										if (s->playing && s->songp == w->songfy) break;
										oldpattern = s->songi[w->songfy];
										s->songi[w->songfy] = emptySongIndex(s->songi[w->songfy]);
										prunePattern(oldpattern);
										addPattern(s->songi[w->songfy]);
										redraw(); break;
									case 'J': /* join with next pattern */
										if (s->playing && s->songp == w->songfy) break;
										if (s->songi[w->songfy+0] == PATTERN_VOID || s->songi[w->songfy+1] == PATTERN_VOID) break;

										pattern pa[2];
										memcpy(&pa[0], s->patternv[s->patterni[s->songi[w->songfy+0]]], sizeof(pattern));
										oldpattern = s->songi[w->songfy+0]; s->songi[w->songfy+0] = PATTERN_VOID; prunePattern(oldpattern);
										memcpy(&pa[1], s->patternv[s->patterni[s->songi[w->songfy+1]]], sizeof(pattern));
										oldpattern = s->songi[w->songfy+1]; s->songi[w->songfy+1] = PATTERN_VOID; prunePattern(oldpattern);

										uint8_t cattruncate = MAX(0, MIN((short)pa[0].rowc + (short)pa[1].rowc + 1, 255) - (short)pa[0].rowc); /* TODO: pattern max definition */

										s->songi[w->songfy] = emptySongIndex(s->songi[w->songfy]);
										_addPattern(s, s->patternc);
										s->patterni[s->songi[w->songfy]] = s->patternc;
										s->patternc++;
										pv = s->patternv[s->patterni[s->songi[w->songfy]]];

										for (i = 0; i < MAX_CHANNELS; i++)
										{
											renderPatternChannel(&pa[0], i, 0);
											renderPatternChannel(&pa[1], i, 0);

											DEBUG=pa[0].rowc + cattruncate;
											p->dirty=1;
											memcpy(&pv->rowv[i], &pa[0].rowv[i], sizeof(row) * pa[0].rowc + 1);
											memcpy(&pv->rowv[i][pa[0].rowc + 1], &pa[1].rowv[i], sizeof(row) * cattruncate);
											pv->rowcc[i] = pa[0].rowc + cattruncate;
										}
										pv->rowc = pa[0].rowc + cattruncate;
										pushPatternHistory(pv);

										/* TODO: contiguity */
										redraw(); break;
									case 'c': /* clone */
										if (s->playing && s->songp == w->songfy) break;
										if (s->songi[w->songfy] == PATTERN_VOID) break;
										oldpattern = s->songi[w->songfy];
										s->songi[w->songfy] = duplicatePattern(s->songi[w->songfy]);
										prunePattern(oldpattern);
										redraw(); break;
									case 'p': /* song paste */
										if (s->playing && s->songp == w->songfy) break;
										if (w->songbufferlen)
										{
											memcpy(&s->songi[w->songfy], w->songibuffer, w->songbufferlen);
											memcpy(&s->songf[w->songfy], w->songfbuffer, w->songbufferlen);
											w->songfy += w->songbufferlen;
										} redraw(); break;
									case 'P': /* song paste above */
										if (s->playing && s->songp == w->songfy) break;
										if (w->songbufferlen)
										{
											memcpy(&s->songi[w->songfy], w->songibuffer, w->songbufferlen);
											memcpy(&s->songf[w->songfy], w->songfbuffer, w->songbufferlen);
										} redraw(); break;
									case 'y': /* song copy */
										w->songbufferlen = 1;
										w->songibuffer[0] = s->songi[w->songfy];
										w->songfbuffer[0] = s->songf[w->songfy];
										w->mode = T_MODE_SONG;
										redraw(); break;
									case 'd': case 'x': case 127: case '\b': /* song delete */
										if (s->playing && s->songp == w->songfy) break;
										if (w->songnext == w->songfy + 1)
											w->songnext = 0;
										w->songbufferlen = 1;
										w->songibuffer[0] = s->songi[w->songfy];
										w->songfbuffer[0] = s->songf[w->songfy];
										oldpattern = s->songi[w->songfy];
										s->songi[w->songfy] = PATTERN_VOID;
										s->songf[w->songfy] = 0;
										prunePattern(oldpattern);
										redraw(); break;
								} break;
					} break;
				case T_MODE_NORMAL:
					switch (input)
					{ /* set count first */
						case '0': w->count *= 10; w->count += 0; redraw(); return;
						case '1': w->count *= 10; w->count += 1; redraw(); return;
						case '2': w->count *= 10; w->count += 2; redraw(); return;
						case '3': w->count *= 10; w->count += 3; redraw(); return;
						case '4': w->count *= 10; w->count += 4; redraw(); return;
						case '5': w->count *= 10; w->count += 5; redraw(); return;
						case '6': w->count *= 10; w->count += 6; redraw(); return;
						case '7': w->count *= 10; w->count += 7; redraw(); return;
						case '8': w->count *= 10; w->count += 8; redraw(); return;
						case '9': w->count *= 10; w->count += 9; redraw(); return;
						default:
							if (w->chord)
							{
								w->count = MIN(256, w->count);
								switch (w->chord)
								{
									case 'd': if (input == 'd') /* delete */
										{
											if (s->songi[w->songfy] == PATTERN_VOID) break;
											pv = s->patternv[s->patterni[s->songi[w->songfy]]];
											j = MIN(pv->rowc, w->trackerfy - 1 + MAX(1, w->count)) - w->trackerfy;
											yankPartPattern(0, 2+s->channelv[w->channel].macroc, w->trackerfy, w->trackerfy+j, w->channel, w->channel);
											delPartPattern(0, 2+s->channelv[w->channel].macroc, w->trackerfy, w->trackerfy+j, w->channel, w->channel);
											for (i = 0; i < j; i++) trackerDownArrow();
											redraw();
										} break;
									case 'y': if (input == 'y') /* yank */
										{
											if (s->songi[w->songfy] == PATTERN_VOID) break;
											pv = s->patternv[s->patterni[s->songi[w->songfy]]];
											j = MIN(pv->rowc-1, w->trackerfy - 1 + MAX(1, w->count)) - w->trackerfy;
											yankPartPattern(0, 2+s->channelv[w->channel].macroc, w->trackerfy, w->trackerfy+j, w->channel, w->channel);
											for (i = 0; i < j; i++) trackerDownArrow();
											redraw();
										} break;
									case 'c': /* channel */
										switch (input)
										{
											case 'c': /* clear */ clearPatternChannel(s, s->patterni[w->pattern], w->channel); resize(0); break;
											case 'a': /* add */
												for (i = 0; i < MAX(1, w->count); i++)
												{
													if (s->channelc >= MAX_CHANNELS) break;
													addChannel(s, w->channel+1);
													w->channel++;
												} resize(0); break;
											case 'A': /* add before */
												for (i = 0; i < MAX(1, w->count); i++)
												{
													if (s->channelc >= MAX_CHANNELS) break;
													addChannel(s, w->channel);
												} resize(0); break;
											case 'd': /* delete */
												for (i = 0; i < MAX(1, w->count); i++)
												{
													if (delChannel(w->channel)) break;
													if (w->channel > s->channelc-1)
														w->channel = s->channelc-1;
												} resize(0); break;
											case 'D': /* delete to end */ /* ignores w->count */
												if (w->channel == 0) w->channel++;
												for (uint8_t i = s->channelc; i > w->channel; i--)
													delChannel(i - 1);
												w->channel--;
												resize(0); break;
											case 'y': /* yank */ yankChannel(w->pattern, w->channel); redraw(); break;
											case 'p': /* put */
												putChannel(w->pattern, w->channel);
												for (i = 1; i < MAX(1, w->count); i++)
												{
													if (s->channelc >= MAX_CHANNELS) break;
													w->channel++;
													putChannel(w->pattern, w->channel);
												} resize(0); break;
											case 'P': /* mix put */
												mixPutChannel(w->pattern, w->channel);
												for (i = 1; i < MAX(1, w->count); i++)
												{
													if (s->channelc >= MAX_CHANNELS) break;
													w->channel++;
													mixPutChannel(w->pattern, w->channel);
												} resize(0); break;
										} break;
									case 'm': /* macro */
										switch (input)
										{
											case 'a': /* add */
												for (i = 0; i < MAX(1, w->count); i++)
													if (s->channelv[w->channel].macroc < 7)
														s->channelv[w->channel].macroc++;
												resize(0); break;
											case 'd': /* delete */
												for (i = 0; i < MAX(1, w->count); i++)
												{
													if (s->channelv[w->channel].macroc)
														s->channelv[w->channel].macroc--;
													if (w->trackerfx > 2+s->channelv[w->channel].macroc * 2)
														w->trackerfx = 2+s->channelv[w->channel].macroc * 2;
												} resize(0); break;
											case 'm': /* set */
												if (w->count) s->channelv[w->channel].macroc = MIN(8, w->count) - 1;
												else s->channelv[w->channel].macroc = 1;
												resize(0); break;
										} break;
									case 'K': /* keyboard mode */ /* ignores w->count */
										w->keyboardmacro = '\0';
										if (input != 'k' && input != 'K') changeMacro(input, &w->keyboardmacro);
										redraw(); break;
									case 'g': /* graphic */ if (input == 'g') w->trackerfy = 0; redraw(); break;
									case 'r': /* row */
										pv = s->patternv[s->patterni[s->songi[w->songfy]]];
										switch (input)
										{
											case 'c': case 'C': renderPatternChannel(pv, w->channel, w->trackerfy + 1); redraw(); break;
											case 'r': case 'R': renderPatternChannel(pv, w->channel, w->count); redraw(); break;
											case 'd': case 'D':
												for (i = 0; i < MAX(1, w->count); i++)
													if (pv->rowcc[w->channel]) pv->rowcc[w->channel]--;
												redraw(); break;
											case 'a': case 'A':
												for (i = 0; i < MAX(1, w->count); i++)
													if (pv->rowcc[w->channel] < 255)
													{
														pv->rowcc[w->channel]++;
														memset(&pv->rowv[w->channel][pv->rowcc[w->channel]], 0, sizeof(row));
														pv->rowv[w->channel][pv->rowcc[w->channel]].note = NOTE_VOID;
														pv->rowv[w->channel][pv->rowcc[w->channel]].inst = INST_VOID;
													} else break;
												redraw(); break;
											case '-': case '_':
												for (i = 0; i < MAX(1, w->count); i++)
													if (pv->rowcc[w->channel] == 255)
														pv->rowcc[w->channel] = 127;
													else if (pv->rowcc[w->channel])
														pv->rowcc[w->channel] = (pv->rowcc[w->channel] + 1) / 2 - 1;
													else break;
												redraw(); break;
											case '+': case '=':
												for (i = 0; i < MAX(1, w->count); i++)
													if (pv->rowcc[w->channel] < 127)
													{
														memcpy(&pv->rowv[w->channel][pv->rowcc[w->channel] + 1], pv->rowv[w->channel],
																sizeof(row) * (pv->rowcc[w->channel] + 1));
														pv->rowcc[w->channel] = (pv->rowcc[w->channel] + 1) * 2 - 1;
													} else
													{
														memcpy(&pv->rowv[w->channel][pv->rowcc[w->channel] + 1], pv->rowv[w->channel],
																sizeof(row) * (255 - pv->rowcc[w->channel]));
														pv->rowcc[w->channel] = 255;
														break;
													}
												redraw(); break;
											case '/': case '?':
												for (i = 0; i < MAX(1, w->count); i++)
													if (pv->rowcc[w->channel] == 255)
													{
														for (j = 0; j < 127; j++)
															pv->rowv[w->channel][j] = pv->rowv[w->channel][j*2];
														pv->rowcc[w->channel] = 127;
													} else if (pv->rowcc[w->channel])
													{
														for (j = 0; j < pv->rowcc[w->channel] + 1; j++)
															pv->rowv[w->channel][j] = pv->rowv[w->channel][j*2];
														pv->rowcc[w->channel] = (pv->rowcc[w->channel] + 1) / 2 - 1;
													} else break;
												redraw(); break;
											case '*': /* no shift bind for this one */
												for (i = 0; i < MAX(1, w->count); i++)
													if (pv->rowcc[w->channel] < 127)
													{
														for (j = pv->rowcc[w->channel] + 1; j > 0; j--)
														{
															pv->rowv[w->channel][j*2] = pv->rowv[w->channel][j];
															memset(&pv->rowv[w->channel][j*2-1], 0, sizeof(row));
															pv->rowv[w->channel][j*2-1].note = NOTE_VOID;
															pv->rowv[w->channel][j*2-1].inst = INST_VOID;
														} pv->rowcc[w->channel] = (pv->rowcc[w->channel] + 1) * 2 - 1;
													} else if (pv->rowcc[w->channel] < 255)
													{
														for (j = pv->rowcc[w->channel] + 1; j > 0; j--)
														{
															pv->rowv[w->channel][j*2] = pv->rowv[w->channel][j];
															memset(&pv->rowv[w->channel][j*2-1], 0, sizeof(row));
															pv->rowv[w->channel][j*2-1].note = NOTE_VOID;
															pv->rowv[w->channel][j*2-1].inst = INST_VOID;
														} pv->rowcc[w->channel] = 255;
													} else break;
												redraw(); break;
										}
										pv->rowc = 0;
										for (i = 0; i < MAX_CHANNELS; i++)
											pv->rowc = MAX(pv->rowc, pv->rowcc[i]);
										w->trackerfy = MIN(pv->rowc, w->trackerfy);
										break;
									case 'R': /* global row */
										pv = s->patternv[s->patterni[s->songi[w->songfy]]];
										switch (input)
										{
											case 'c': case 'C': for (i = 0; i < MAX_CHANNELS; i++) renderPatternChannel(pv, i, w->trackerfy + 1); redraw(); break;
											case 'r': case 'R': for (i = 0; i < MAX_CHANNELS; i++) renderPatternChannel(pv, i, w->count); redraw(); break;
											case 'd': case 'D':
												for (i = 0; i < MAX_CHANNELS; i++)
													for (j = 0; j < MAX(1, w->count); j++)
														if (pv->rowcc[i]) pv->rowcc[i]--;
												redraw(); break;
											case 'a': case 'A':
												for (i = 0; i < MAX_CHANNELS; i++)
													for (j = 0; j < MAX(1, w->count); j++)
														if (pv->rowcc[i] < 255)
														{
															pv->rowcc[i]++;
															memset(&pv->rowv[i][pv->rowcc[i]], 0, sizeof(row));
															pv->rowv[i][pv->rowcc[i]].note = NOTE_VOID;
															pv->rowv[i][pv->rowcc[i]].inst = INST_VOID;
														} else break;
												redraw(); break;
											case '-': case '_':
												for (i = 0; i < MAX_CHANNELS; i++)
													for (j = 0; j < MAX(1, w->count); j++)
													{
														if (pv->rowcc[i] == 255)
															pv->rowcc[i] = 127;
														else if (pv->rowcc[i])
															pv->rowcc[i] = (pv->rowcc[i] + 1) / 2 - 1;
														else break;
													}
												redraw(); break;
											case '+': case '=':
												for (i = 0; i < MAX_CHANNELS; i++)
													for (j = 0; j < MAX(1, w->count); j++)
														if (pv->rowcc[i] < 127)
														{
															memcpy(&pv->rowv[i][pv->rowcc[i] + 1], pv->rowv[i],
																	sizeof(row) * (pv->rowcc[i] + 1));
															pv->rowcc[i] = (pv->rowcc[i] + 1) * 2 - 1;
														} else
														{
															memcpy(&pv->rowv[w->channel][pv->rowcc[i] + 1], pv->rowv[w->channel],
																	sizeof(row) * (255 - pv->rowcc[i]));
															pv->rowcc[i] = 255;
															break;
														}
												redraw(); break;
											case '/': case '?':
												for (i = 0; i < MAX_CHANNELS; i++)
													for (k = 0; k < MAX(1, w->count); k++)
														if (pv->rowcc[i] == 255)
														{
															for (j = 0; j < 127; j++)
																pv->rowv[i][j] = pv->rowv[i][j*2];
															pv->rowcc[i] = 127;
														} else if (pv->rowcc[i])
														{
															for (j = 0; j < pv->rowcc[i] + 1; j++)
																pv->rowv[i][j] = pv->rowv[i][j*2];
															pv->rowcc[i] = (pv->rowcc[i] + 1) / 2 - 1;
														} else break;
												redraw(); break;
											case '*': /* no shift bind for this one */
												for (i = 0; i < MAX_CHANNELS; i++)
													for (k = 0; k < MAX(1, w->count); k++)
														if (pv->rowcc[i] < 127)
														{
															for (j = pv->rowcc[i] + 1; j > 0; j--)
															{
																pv->rowv[i][j*2] = pv->rowv[i][j];
																memset(&pv->rowv[i][j*2-1], 0, sizeof(row));
																pv->rowv[i][j*2-1].note = NOTE_VOID;
																pv->rowv[i][j*2-1].inst = INST_VOID;
															}
															pv->rowcc[i] = (pv->rowcc[i] + 1) * 2 - 1;
														} else if (pv->rowcc[i] < 255)
														{
															for (j = pv->rowcc[i] + 1; j > 0; j--)
															{
																pv->rowv[i][j*2] = pv->rowv[i][j];
																memset(&pv->rowv[i][j*2-1], 0, sizeof(row));
																pv->rowv[i][j*2-1].note = NOTE_VOID;
																pv->rowv[i][j*2-1].inst = INST_VOID;
															}
															pv->rowcc[i] = 255;
														} else break;
												redraw(); break;
										}
										pv->rowc = 0;
										for (i = 0; i < MAX_CHANNELS; i++)
											pv->rowc = MAX(pv->rowc, pv->rowcc[i]);
										w->trackerfy = MIN(pv->rowc, w->trackerfy);
										break;
								} w->count = 0;
							} else if (s->patterni[s->songi[w->songfy]])
							{
								modulorow = w->trackerfy % (s->patternv[s->patterni[s->songi[w->songfy]]]->rowcc[w->channel]+1);
								r = &s->patternv[s->patterni[s->songi[w->songfy]]]->rowv[w->channel][modulorow];
								switch (input)
								{
									case '\n': case '\r': toggleChannelMute(w->channel); redraw(); break;
									case 'f': /* toggle song follow */ w->flags ^= W_FLAG_FOLLOW; redraw(); break;
									case 'i': /* enter insert mode  */ w->mode = T_MODE_INSERT; redraw(); break;
									case 'k': /* up arrow           */ trackerUpArrow(); redraw(); break;
									case 'u': /* undo               */ popPatternHistory(s->patterni[s->songi[w->songfy]]); redraw(); break;
									case 18:  /* redo               */ unpopPatternHistory(s->patterni[s->songi[w->songfy]]); redraw(); break;
									case 'j': /* down arrow         */ trackerDownArrow(); redraw(); break;
									case 'h': /* left arrow         */ trackerLeftArrow(); redraw(); break;
									case 'l': /* right arrow        */ trackerRightArrow(); redraw(); break;
									case '[': /* chnl left          */ channelLeft(); redraw(); break;
									case ']': /* chnl right         */ channelRight(); redraw(); break;
									case '{': /* cycle up           */ cycleUp(); redraw(); break;
									case '}': /* cycle down         */ cycleDown(); redraw(); break;
									case 'v': case 22: /* enter visual mode */
										w->visualfx = tfxToVfx(w->trackerfx);
										w->visualfy = w->trackerfy;
										w->visualchannel = w->channel;
										w->mode = T_MODE_VISUAL;
										redraw(); break;
									case 'V': /* enter visual line mode */
										w->visualfx = tfxToVfx(w->trackerfx);
										w->visualfy = w->trackerfy;
										w->visualchannel = w->channel;
										w->mode = T_MODE_VISUALLINE;
										redraw(); break;
									case '\t': /* enter song mode */ w->mode = T_MODE_SONG; redraw(); break;
									case 'y':  /* pattern copy    */ w->chord = 'y'; redraw(); return;
									case 'd':  /* pattern cut     */ w->chord = 'd'; redraw(); return;
									case 'c':  /* channel         */ w->chord = 'c'; redraw(); return;
									case 'm':  /* macro           */ w->chord = 'm'; redraw(); return;
									case 'K':  /* keyboard macro  */ w->chord = 'K'; redraw(); return;
									case 'r':  /* row             */ w->chord = 'r'; redraw(); return;
									case 'R':  /* global row      */ w->chord = 'R'; redraw(); return;
									case 'g':  /* graphic misc    */ w->chord = 'g'; redraw(); return;
									case 'G':  /* graphic end     */ w->trackerfy = s->patternv[s->patterni[s->songi[w->songfy]]]->rowc; redraw(); break;
									case 'b':  /* bpm             */ if (w->count) s->songbpm = MIN(255, MAX(32, w->count)); w->request = REQ_BPM; redraw(); break;
									case 't':  /* row highlight   */ if (w->count) s->rowhighlight = MIN(16, w->count); redraw(); break;
									case 's':  /* step            */ w->step = MIN(15, w->count); redraw(); break;
									case 'o':  /* octave          */
										if (!w->trackerfx) r->note = changeNoteOctave(MIN(9, w->count), r->note);
										else               w->octave = MIN(9, w->count);
										redraw(); break;
									case 'p':  /* pattern put */ /* TODO: count */
										if (s->songi[w->songfy] == PATTERN_VOID) break;
										putPartPattern();
										w->trackerfy = MIN(w->trackerfy + w->pbfy[1] - w->pbfy[0], s->patternv[s->patterni[s->songi[w->songfy]]]->rowc - 1);
										trackerDownArrow();
										redraw(); break;
									case 'P': /* pattern put before */
										if (s->songi[w->songfy] == PATTERN_VOID) break;
										mixPutPartPattern();
										redraw(); break;
									case 'x': case 127: case '\b': /* backspace */
										if (w->trackerfx == 0)
										{
											yankPartPattern(0, 1, w->trackerfy, w->trackerfy, w->channel, w->channel);
											delPartPattern(0, 1, w->trackerfy, w->trackerfy, w->channel, w->channel);
										} else
										{
											yankPartPattern(tfxToVfx(w->trackerfx), tfxToVfx(w->trackerfx), w->trackerfy, w->trackerfy, w->channel, w->channel);
											delPartPattern(tfxToVfx(w->trackerfx), tfxToVfx(w->trackerfx), w->trackerfy, w->trackerfy, w->channel, w->channel);
										} redraw(); break;
									case '%': /* random */
										randPartPattern(tfxToVfx(w->trackerfx), tfxToVfx(w->trackerfx), w->trackerfy, w->trackerfy, w->channel, w->channel);
										redraw(); break;
									default: /* column specific */
										switch (w->trackerfx)
										{
											case 0: /* note */
												switch (input)
												{
													case 1:  /* ^a */ r->note+=MAX(1, w->count); break;
													case 24: /* ^x */ r->note-=MAX(1, w->count); break;
												} break;
											case 1: /* instrument */
												switch (input)
												{
													case 1: /* ^a */
														r->inst+=MAX(1, w->count);
														if (w->instrumentrecv == INST_REC_LOCK_OK) w->instrument = r->inst;
														break;
													case 24: /* ^x */
														r->inst-=MAX(1, w->count);
														if (w->instrumentrecv == INST_REC_LOCK_OK) w->instrument = r->inst;
														break;
												} break;
											default:
												macro = (w->trackerfx - 2)>>1;
												switch (input)
												{
													case 1:  /* ^a */
														switch (r->macro[macro].c)
														{
															case 'G': r->macro[macro].v += MAX(1, w->count)*16;
															default:  r->macro[macro].v += MAX(1, w->count);
														} break;
													case 24: /* ^x */
														switch (r->macro[macro].c)
														{
															case 'G': r->macro[macro].v -= MAX(1, w->count)*16;
															default:  r->macro[macro].v -= MAX(1, w->count);
														} break;
													case '~': /* toggle case */
														if      (isupper(r->macro[macro].c)) changeMacro(r->macro[macro].c, &r->macro[macro].c);
														else if (islower(r->macro[macro].c)) changeMacro(r->macro[macro].c, &r->macro[macro].c);
														break;
												} break;
										} redraw(); break;
								}
							} else if (input == '\t') /* enter song mode */ w->mode = T_MODE_SONG; redraw(); break;
							break;
					} break;
				case T_MODE_SONG_VISUAL:
					switch (input)
					{
						case 'v': case 22: w->mode = T_MODE_SONG; redraw(); break;
						case 'k': /* up arrow    */ trackerUpArrow(); redraw(); break;
						case 'j': /* down arrow  */ trackerDownArrow(); redraw(); break;
						case 'h': /* left arrow  */ trackerLeftArrow(); redraw(); break;
						case 'l': /* right arrow */ trackerRightArrow(); redraw(); break;
						case '[': /* chnl left   */ channelLeft(); redraw(); break;
						case ']': /* chnl right  */ channelRight(); redraw(); break;
						case '{': /* cycle up    */ cycleUp(); redraw(); break;
						case '}': /* cycle down  */ cycleDown(); redraw(); break;
						case 'y': /* song copy   */
							w->songbufferlen = MAX(w->songfy, w->visualfy) - MIN(w->songfy, w->visualfy) +1;
							if (w->songbufferlen)
							{
								memcpy(w->songibuffer, &s->songi[MIN(w->songfy, w->visualfy)], w->songbufferlen);
								memcpy(w->songfbuffer, &s->songf[MIN(w->songfy, w->visualfy)], w->songbufferlen);
							}
							w->songfy = MIN(w->songfy, w->visualfy);
							w->mode = T_MODE_SONG;
							redraw(); break;
						case 'd': case 'x': case 127: case '\b': /* song delete */
							w->songbufferlen = MAX(w->songfy, w->visualfy) - MIN(w->songfy, w->visualfy) +1;
							if (w->songbufferlen)
							{
								memcpy(w->songibuffer, &s->songi[MIN(w->songfy, w->visualfy)], w->songbufferlen);
								memcpy(w->songfbuffer, &s->songf[MIN(w->songfy, w->visualfy)], w->songbufferlen);
								for (i = 0; i < w->songbufferlen; i++)
								{
									s->songi[MIN(w->songfy, w->visualfy)+i] = PATTERN_VOID;
									s->songf[MIN(w->songfy, w->visualfy)+i] = 0;
									if (w->songnext == MIN(w->songfy, w->visualfy)+i + 1)
										w->songnext = 0;
								}
							}
							w->songfy = MIN(w->songfy, w->visualfy);
							w->mode = T_MODE_SONG;
							redraw(); break;
						case '?': /* block loop */
							for (i = 0; i < MAX(w->songfy, w->visualfy) - MIN(w->songfy, w->visualfy) +1; i++)
								if (s->songi[MIN(w->songfy, w->visualfy)+i] != PATTERN_VOID)
									s->songf[MIN(w->songfy, w->visualfy)+i] = !s->songf[MIN(w->songfy, w->visualfy)+i];
							redraw(); break;
					} break;
				case T_MODE_SONG_INSERT:
					if (s->playing && s->songp == w->songfy) break;
					switch (input)
					{
						case '0':           inputSongHex(0);  break;
						case '1':           inputSongHex(1);  break;
						case '2':           inputSongHex(2);  break;
						case '3':           inputSongHex(3);  break;
						case '4':           inputSongHex(4);  break;
						case '5':           inputSongHex(5);  break;
						case '6':           inputSongHex(6);  break;
						case '7':           inputSongHex(7);  break;
						case '8':           inputSongHex(8);  break;
						case '9':           inputSongHex(9);  break;
						case 'A': case 'a': inputSongHex(10); break;
						case 'B': case 'b': inputSongHex(11); break;
						case 'C': case 'c': inputSongHex(12); break;
						case 'D': case 'd': inputSongHex(13); break;
						case 'E': case 'e': inputSongHex(14); break;
						case 'F': case 'f': inputSongHex(15); break;
						case 127: case '\b': /* backspace */
							if (w->songnext == w->songfy + 1)
								w->songnext = 0;
							oldpattern = s->songi[w->songfy];
							s->songi[w->songfy] = PATTERN_VOID;
							s->songf[w->songfy] = 0;
							prunePattern(oldpattern);
							redraw(); break;
					} break;
				case T_MODE_INSERT:
					if (s->songi[w->songfy] == PATTERN_VOID) break;
					modulorow = w->trackerfy % (s->patternv[s->patterni[s->songi[w->songfy]]]->rowcc[w->channel]+1);
					r = &s->patternv[s->patterni[s->songi[w->songfy]]]->rowv[w->channel][modulorow];
					if (input == '\n' || input == '\r') { toggleChannelMute(w->channel); redraw(); }
					else
						switch (w->trackerfx)
						{
							case 0: /* note */
								switch (input)
								{
									case 1:  /* ^a */ r->note+=MAX(1, w->count); break;
									case 24: /* ^x */ r->note-=MAX(1, w->count); break;
									case 127: case '\b': /* backspace */
										r->note = NOTE_VOID;
										r->inst = INST_VOID;
										w->trackerfy -= w->step;
										if (w->trackerfy < 0)
										{
											if (w->songfy > 0 && s->songi[w->songfy - 1] != PATTERN_VOID)
											{
												w->songfy--;
												w->trackerfy = s->patternv[s->patterni[s->songi[w->songfy]]]->rowc;
											} else w->trackerfy = 0;
										} break;
									/* case '0': r->note = changeNoteOctave(0, r->note); break;
									case '1': r->note = changeNoteOctave(1, r->note); break;
									case '2': r->note = changeNoteOctave(2, r->note); break;
									case '3': r->note = changeNoteOctave(3, r->note); break;
									case '4': r->note = changeNoteOctave(4, r->note); break;
									case '5': r->note = changeNoteOctave(5, r->note); break;
									case '6': r->note = changeNoteOctave(6, r->note); break;
									case '7': r->note = changeNoteOctave(7, r->note); break;
									case '8': r->note = changeNoteOctave(8, r->note); break;
									case '9': r->note = changeNoteOctave(9, r->note); break; */
									default:
										insertNote(r, input);
										previewNote(input, r->inst, w->channel);
										w->trackerfy += w->step;
										if (w->trackerfy > s->patternv[s->patterni[s->songi[w->songfy]]]->rowc)
										{
											if (w->songfy < SONG_MAX-1 && s->songi[w->songfy + 1] != PATTERN_VOID)
											{
												w->trackerfy = 0;
												w->songfy++;
											} else w->trackerfy = s->patternv[s->patterni[s->songi[w->songfy]]]->rowc;
										} break;
								} break;
							case 1: /* instrument */ insertInst(r, input); break;
							default: /* macros */
								macro = (w->trackerfx - 2)>>1;
								switch (input)
								{
									case '~': /* toggle case */
										if      (isupper(r->macro[macro].c)) r->macro[macro].c += 32;
										else if (islower(r->macro[macro].c)) r->macro[macro].c -= 32;
										break;
									default:
										if (!(w->trackerfx%2)) insertMacroc(r, macro, input);
										else
											if (w->keyboardmacro)
												switch (r->macro[macro].c ? r->macro[macro].c : w->keyboardmacro)
												{
													case 0: insertMacrov(r, macro, input); break;
													case 'G':
														if (charToKmode(input, 1, &r->macro[macro].v, NULL) && !r->macro[macro].c)
															r->macro[macro].c = w->keyboardmacro;
														break;
													default:
														if (charToKmode(input, 0, &r->macro[macro].v, NULL) && !r->macro[macro].c)
															r->macro[macro].c = w->keyboardmacro;
														break;
												}
											else insertMacrov(r, macro, input);
										break;
								} break;
						} redraw(); break;
			} break;
	}
	if (w->count) { w->count = 0; redraw(); }
	if (w->chord) { w->chord = '\0'; redraw(); }
	pushPatternHistoryIfNew(s->patternv[s->patterni[s->songi[w->songfy]]]);
}
