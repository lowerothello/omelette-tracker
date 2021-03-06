#define T_MODE_NORMAL 0
#define T_MODE_VISUAL 1
#define T_MODE_MOUSEADJUST 2
#define T_MODE_INSERT 3
#define T_MODE_VISUALLINE 5
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
		case ';': *dest = ';'; break; /* MIDI CC target          */
		case '@': *dest = '@'; break; /* MIDI PC                 */
		case '.': *dest = '.'; break; /* MIDI CC                 */
		case ',': *dest = ','; break; /* smooth MIDI CC          */
		case '%': *dest = '%'; break; /* note chance             */
		case 'b': *dest = 'B'; break; /* bpm                     */
		case 'c': *dest = 'C'; break; /* note cut                */
		case 'd': *dest = 'D'; break; /* note delay              */
		case 'D': *dest = 'd'; break; /* fine note delay         */
		/* Exx - local envelope times */
		case 'f': *dest = 'F'; break; /* filter                  */
		case 'F': *dest = 'f'; break; /* smooth filter           */
		case 'g': *dest = 'G'; break; /* gain                    */
		case 'G': *dest = 'g'; break; /* smooth gain             */
		/* Hxx - local pitch shift        */
		/* hxx - smooth local pitch shift */
		/* Lxx - local cycle length       */
		case 'm': *dest = 'M'; break; /* microtonal offset       */
		case 'o': *dest = 'O'; break; /* offset                  */
		case 'O': *dest = 'o'; break; /* backwards offset        */
		case 'p': *dest = 'P'; break; /* pitch slide             */
		case 'r': *dest = 'R'; break; /* retrigger               */
		/* Sxy - send to output group */
		case 'v': *dest = 'V'; break; /* vibrato TODO: MIDI      */
		case 'w': *dest = 'W'; break; /* waveshaper              */
		case 'W': *dest = 'w'; break; /* smooth waveshaper       */
		case 'z': *dest = 'Z'; break; /* filter resonance        */
		case 'Z': *dest = 'z'; break; /* smooth filter resonance */

		/* ?xx - local cycle length        */
		/* ?xx - smooth local cycle length */
	}
}

void prunePattern(uint8_t index, short callingindex)
{
	if (index == 255) return; /* invalid index */
	if (s->patterni[index] == 0) return; /* pattern doesn't exist */

	/* don't remove if pattern is still referenced */
	for (short i = 0; i < 256; i++) if (s->songi[i] == index && i != callingindex) return;

	pattern *p = s->patternv[s->patterni[index]];
	/* don't remove if pattern is populated */
	for (short c = 0; c < s->channelc; c++)
		for (short i = 0; i < p->rowcc[c] + 1; i++)
		{
			row r = p->rowv[c][i];
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
			macro = (w->trackerfx - 2) / 2;
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
	return MIN(A10-1, note);
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
			macro = (w->trackerfx - 2) / 2;
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
			macro = (w->trackerfx - 2) / 2;
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
	if (w->flags&0b1) w->flags ^= 0b1;
	switch (w->mode)
	{
		case T_MODE_SONG: case T_MODE_SONG_INSERT: case T_MODE_SONG_VISUAL:
			for (i = 0; i < MAX(1, w->count); i++)
				if (w->songfy)
					w->songfy--;
			break;
		default:
			for (i = 0; i < MAX(1, w->count); i++)
			{
				if (s->songi[w->songfy] == 255) return;
				w->trackerfy -= MAX(1, w->step);
				if (w->trackerfy < 0)
				{
					if (w->songfy > 0 && s->songi[w->songfy - 1] != 255)
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
	if (w->flags&0b1) w->flags ^= 0b1;
	switch (w->mode)
	{
		case T_MODE_SONG: case T_MODE_SONG_INSERT: case T_MODE_SONG_VISUAL:
			for (i = 0; i < MAX(1, w->count); i++)
				if (w->songfy < 255)
					w->songfy++;
			break;
		default:
			for (i = 0; i < MAX(1, w->count); i++)
			{
				if (s->songi[w->songfy] == 255) return;
				w->trackerfy += MAX(1, w->step);
				if (w->trackerfy > s->patternv[s->patterni[s->songi[w->songfy]]]->rowc)
				{
					if (w->songfy < 255 && s->songi[w->songfy + 1] != 255)
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
	switch (w->mode)
	{
		case T_MODE_SONG: case T_MODE_SONG_INSERT: case T_MODE_SONG_VISUAL:
			bound = w->songfy;
			for (int i = 0; i < MAX(1, w->count); i++)
			{
				for (int j = bound; j < 255; j++)
				{
					s->songi[j] = s->songi[j+1];
					s->songf[j] = s->songf[j+1];
				} s->songi[255] = 255; s->songf[255] = 0;
			} break;
		case T_MODE_NORMAL: case T_MODE_INSERT:
			if (s->songi[w->songfy] == 255) return;
			bound = w->trackerfy%(s->patternv[s->patterni[s->songi[w->songfy]]]->rowcc[w->channel]+1);
			for (int i = 0; i < MAX(1, w->count); i++)
			{
				pattern *p = s->patternv[s->patterni[s->songi[w->songfy]]];
				row hold = p->rowv[w->channel][bound]; /* hold the first row */
				for (int j = bound; j < p->rowcc[w->channel]; j++)
					p->rowv[w->channel][j] = p->rowv[w->channel][j+1];
				p->rowv[w->channel][p->rowcc[w->channel]] = hold; }
			break;
	}
}
void cycleDown(void)
{
	short bound;
	switch (w->mode)
	{
		case T_MODE_SONG: case T_MODE_SONG_INSERT: case T_MODE_SONG_VISUAL:
			bound = w->songfy;
			for (int i = 0; i < MAX(1, w->count); i++)
			{
				for (int j = 254; j >= bound; j--)
				{
					s->songi[j+1] = s->songi[j];
					s->songf[j+1] = s->songf[j];
				} s->songi[bound] = 255; s->songf[bound] = 0;
			} break;
		case T_MODE_NORMAL: case T_MODE_INSERT:
			if (s->songi[w->songfy] == 255) return;
			bound = w->trackerfy%(s->patternv[s->patterni[s->songi[w->songfy]]]->rowcc[w->channel]+1);
			for (int i = 0; i < MAX(1, w->count); i++)
			{
				pattern *p = s->patternv[s->patterni[s->songi[w->songfy]]];
				row hold = p->rowv[w->channel][p->rowcc[w->channel]]; /* hold the last row */
				for (int j = p->rowcc[w->channel] - 1; j >= bound; j--)
					p->rowv[w->channel][j+1] = p->rowv[w->channel][j];
				p->rowv[w->channel][bound] = hold;
			} break;
	}
}
void trackerLeftArrow(void)
{
	if (w->flags&0b1) w->flags ^= 0b1;
	switch (w->mode)
	{
		case T_MODE_SONG: case T_MODE_SONG_INSERT: case T_MODE_SONG_VISUAL: break;
		default:
			for (int i = 0; i < MAX(1, w->count); i++)
			{
				if (s->songi[w->songfy] == 255) return;
				w->trackerfx--;
				if (w->trackerfx < 0)
				{
					if (w->channel > 0)
					{
						w->channel--;
						w->trackerfx = 1 + s->channelv[w->channel].macroc * 2;
					} else w->trackerfx = 0;
					if (w->visiblechannels % 2 == 0)
					{
						if (w->channeloffset
								&& w->channel < s->channelc - w->visiblechannels / 2)
							w->channeloffset--;
					} else if (w->channeloffset
							&& w->channel < s->channelc - 1 - w->visiblechannels / 2)
						w->channeloffset--;
				}
			} break;
	}
}
void channelLeft(void)
{
	if (w->flags&0b1) w->flags ^= 0b1;
	switch (w->mode)
	{
		case T_MODE_SONG: case T_MODE_SONG_INSERT: case T_MODE_SONG_VISUAL: break;
		default:
			for (int i = 0; i < MAX(1, w->count); i++)
			{
				if (s->songi[w->songfy] != 255)
				{
					if (w->channel > 0)
					{
						w->channel--;
						if (w->trackerfx > 1 + s->channelv[w->channel].macroc * 2)
							w->trackerfx = 1 + s->channelv[w->channel].macroc * 2;
					}
					if (w->visiblechannels % 2 == 0)
					{
						if (w->channeloffset
								&& w->channel < s->channelc - w->visiblechannels / 2)
							w->channeloffset--;
					} else if (w->channeloffset
							&& w->channel < s->channelc - 1 - w->visiblechannels / 2)
						w->channeloffset--;
				}
			} break;
	}
}
void trackerRightArrow(void)
{
	if (w->flags&0b1) w->flags ^= 0b1;
	switch (w->mode)
	{
		case T_MODE_SONG: case T_MODE_SONG_INSERT: case T_MODE_SONG_VISUAL: break;
		default:
			for (int i = 0; i < MAX(1, w->count); i++)
			{
				if (s->songi[w->songfy] == 255) return;
				w->trackerfx++;
				if (w->trackerfx > 1 + s->channelv[w->channel].macroc * 2)
				{
					if (w->channel < s->channelc - 1)
					{
						w->channel++;
						w->trackerfx = 0;
					} else w->trackerfx = 1 + s->channelv[w->channel].macroc * 2;
					if (w->channeloffset + w->visiblechannels < s->channelc
							&& w->channel > w->visiblechannels / 2)
						w->channeloffset++;
				}
			} break;
	}
}
void channelRight(void)
{
	if (w->flags&0b1) w->flags ^= 0b1;
	switch (w->mode)
	{
		case T_MODE_SONG: case T_MODE_SONG_INSERT: case T_MODE_SONG_VISUAL: break;
		default:
			for (int i = 0; i < MAX(1, w->count); i++)
			{
				if (s->songi[w->songfy] == 255) return;
				if (w->channel < s->channelc - 1)
				{
					w->channel++;
					if (w->trackerfx > 1 + s->channelv[w->channel].macroc * 2)
						w->trackerfx = 1 + s->channelv[w->channel].macroc * 2;
				}
				if (w->channeloffset + w->visiblechannels < s->channelc
						&& w->channel > w->visiblechannels / 2)
					w->channeloffset++;
			} break;
	}
}
void trackerHome(void)
{
	switch (w->mode)
	{
		case T_MODE_SONG: case T_MODE_SONG_INSERT: case T_MODE_SONG_VISUAL:
			w->songfy = 0;
			break;
		default:
			w->trackerfy = 0;
			break;
	}
}
void trackerEnd(void)
{
	switch (w->mode)
	{
		case T_MODE_SONG: case T_MODE_SONG_INSERT: case T_MODE_SONG_VISUAL:
			w->songfy = 255;
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
			for (int i = 0; i < MAX(1, w->count); i++)
			{
				w->songfy -= s->rowhighlight;
				if (w->songfy < 0) { w->songfy = 0; break; }
			} break;
		default:
			for (int i = 0; i < MAX(1, w->count); i++)
			{
				if (s->songi[w->songfy] == 255) break;
				w->trackerfy -= s->rowhighlight;
				if (w->trackerfy < 0)
				{
					if (w->songfy > 0 && s->songi[w->songfy - 1] != 255)
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
			for (int i = 0; i < MAX(1, w->count); i++)
			{
				w->songfy += s->rowhighlight;
				if (w->songfy >= 255)
				{
					w->songfy = 255;
					break;
				}
			} break;
		default:
			for (int i = 0; i < MAX(1, w->count); i++)
			{
				if (s->songi[w->songfy] == 255) break;
				w->trackerfy += s->rowhighlight;
				if (w->trackerfy > s->patternv[s->patterni[s->songi[w->songfy]]]->rowc)
				{
					if (w->songfy < 255 && s->songi[w->songfy + 1] != 255)
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
	prunePattern(s->songi[w->songfy], w->songfy);
	if (s->songi[w->songfy] == 255)
		s->songi[w->songfy] = 0;
	s->songi[w->songfy] <<= 4; s->songi[w->songfy] += value;
	addPattern(s->songi[w->songfy], 0);
	redraw();
}

void insertNote(row *r, uint8_t note)
{
	if (note != NOTE_VOID)
	{
		if (w->instrumentrecv == INST_REC_LOCK_OK)
			r->inst = w->instrument;
		if (note == NOTE_OFF) r->note = NOTE_OFF;
		else switch (w->keyboardmacro)
		{
			case 0: r->note = note; break;
			case 'G': /* m.x and m.y are note */
				r->note = C5;
				note -= w->octave*12;
				if (note>=12 && note<=19)
				{
					r->macro[0].c = w->keyboardmacro;
					r->macro[0].v = (note - 12) * 16 + (note - 12);
				}
				if (note>=24 && note<=31)
				{
					r->macro[0].c = w->keyboardmacro;
					r->macro[0].v = (note - 16) * 16 + (note - 16);
				}
				break;
			default: /* m.x is note */
				r->note = C5;
				note -= w->octave*12;
				if (note>=12 && note<=19)
				{
					r->macro[0].c = w->keyboardmacro;
					r->macro[0].v = (note - 12) * 16;
				}
				if (note>=24 && note<=31)
				{
					r->macro[0].c = w->keyboardmacro;
					r->macro[0].v = (note - 16) * 16;
				}
				break;
		}
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

void toggleChannelMute(void)
{
	s->channelv[w->channel].mute = !s->channelv[w->channel].mute;
	if (s->channelv[w->channel].mute && w->instrumentlockv == INST_GLOBAL_LOCK_OK)
	{
		w->instrumentlocki = w->channel;
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
	pattern *p;
	uint8_t note, modulorow;
	switch (input)
	{
		case '\033': /* escape */
			switch (getchar())
			{
				case 'O':
					previewNote(NOTE_OFF, INST_VOID, w->channel);
					switch (getchar())
					{
						case 'P': /* xterm f1 */ showTracker(); break;
						case 'Q': /* xterm f2 */ showInstrument(); break;
					} redraw(); break;
				case '[':
					previewNote(NOTE_OFF, INST_VOID, w->channel);
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
							unsigned short maxwidth = LINENO_COLS + SONGLIST_COLS;
							uint8_t maxchannels = 0;
							for (int i = 0; i < s->channelc; i++)
							{
								if (i+w->channeloffset > s->channelc - 1) break;
								if (maxwidth + 9 + 4*s->channelv[i+w->channeloffset].macroc > ws.ws_col) break;
								maxwidth += 9 + 4*s->channelv[i+w->channeloffset].macroc;
								maxchannels++;
							}
							unsigned short dx = (ws.ws_col - maxwidth) / 2 + 1 + LINENO_COLS + SONGLIST_COLS;

							switch (button)
							{
								case WHEEL_UP: case WHEEL_UP_CTRL:
									if (w->flags&0b1) w->flags ^= 0b1;

									if (x < dx - LINENO_COLS)
									{
										w->songfy -= WHEEL_SPEED;
										if (w->songfy < 0) w->songfy = 0;
									} else
									{
										if (s->songi[w->songfy] == 255) break;
										w->trackerfy -= WHEEL_SPEED;
										if (w->trackerfy < 0)
										{
											if (w->songfy > 0 && s->songi[w->songfy - 1] != 255)
											{
												w->songfy--;
												w->trackerfy += s->patternv[s->patterni[s->songi[w->songfy]]]->rowc + 1;
											} else w->trackerfy = 0;
										}
									} break;
								case WHEEL_DOWN: case WHEEL_DOWN_CTRL:
									if (w->flags&0b1) w->flags ^= 0b1;

									if (x < dx - LINENO_COLS)
									{
										w->songfy += WHEEL_SPEED;
										if (w->songfy > 255) w->songfy = 255;
									} else
									{
										if (s->songi[w->songfy] == 255) break;
										w->trackerfy += WHEEL_SPEED;
										if (w->trackerfy > s->patternv[s->patterni[s->songi[w->songfy]]]->rowc)
										{
											if (w->songfy < 255 && s->songi[w->songfy + 1] != 255)
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
											else if (w->songfy > 255) w->songfy = 255;
											break;
										default:
											if (s->songi[w->songfy] == 255) break;
											w->trackerfy += w->fyoffset;
											if (w->trackerfy < 0)
											{
												if (w->songfy > 0 && s->songi[w->songfy - 1] != 255)
												{
													w->songfy--;
													w->trackerfy += s->patternv[s->patterni[s->songi[w->songfy]]]->rowc + 1;
												} else w->trackerfy = 0;
											} else if (w->trackerfy > s->patternv[s->patterni[s->songi[w->songfy]]]->rowc)
											{
												if (w->songfy < 255 && s->songi[w->songfy + 1] != 255)
												{
													w->trackerfy -= s->patternv[s->patterni[s->songi[w->songfy]]]->rowc + 1;
													w->songfy++;
												} else w->trackerfy = s->patternv[s->patterni[s->songi[w->songfy]]]->rowc;
											}
											w->channeloffset =
												MAX(0, MIN(s->channelc - 1 - w->visiblechannels / 2,
															w->channel - w->visiblechannels / 2));
											break;
									} w->fyoffset = w->fieldpointer = 0;

									switch (w->mode)
									{ /* leave mouseadjust mode */
										case T_MODE_MOUSEADJUST: case T_MODE_SONG_MOUSEADJUST:
											w->mode = w->oldmode; break;
									} break;
								case BUTTON1_HOLD:
									if (w->flags&0b1) w->flags ^= 0b1;
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
													addPattern(s->songi[w->songfy], 0);
												}
												else if (x < w->mousex)
												{
													if (w->fieldpointer) s->songi[w->songfy]-=16;
													else                 s->songi[w->songfy]--;
													addPattern(s->songi[w->songfy], 0);
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
									if (w->flags&0b1) w->flags ^= 0b1;

									if (x < dx - LINENO_COLS)
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
												prunePattern(s->songi[w->songfy], w->songfy + y - w->centre);
												s->songi[w->songfy + y - w->centre] = 255;
												s->songf[w->songfy + y - w->centre] = 0;
											case BUTTON1: case BUTTON1_CTRL:
												if (w->mode != T_MODE_SONG_INSERT) w->mode = T_MODE_SONG;
												if (y - w->centre == 0)
												{
													w->oldmode = w->mode;
													w->mode = T_MODE_SONG_MOUSEADJUST;
													w->mousex = x;
													if (x < dx - LINENO_COLS - 3) w->fieldpointer = 1;
													else                          w->fieldpointer = 0;
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
										}
										if (s->songi[w->songfy] == 255) break;
										for (int i = 0; i < maxchannels; i++)
										{
											if (x < dx + 9 + 4*s->channelv[i+w->channeloffset].macroc)
											{
												w->channel = i+w->channeloffset;
												goto dxandwchannelset;
											} dx += 9 + 4*s->channelv[i+w->channeloffset].macroc;
										}
										w->channel = w->channeloffset + maxchannels-1;
										dx -= 9 + 4*s->channelv[w->channeloffset + maxchannels-1].macroc;
dxandwchannelset:
										/* channel row, for mute/solo */
										if (y <= CHANNEL_ROW)
										{
											if (button == BUTTON1 || button == BUTTON1_CTRL)
											{
												if (w->mode != T_MODE_INSERT) /* suggest mode 0, but allow insert */
													w->mode = T_MODE_NORMAL;
												s->channelv[w->channel].mute = !s->channelv[w->channel].mute;
												if (s->channelv[w->channel].mute && w->instrumentlockv == INST_GLOBAL_LOCK_OK)
												{
													w->instrumentlocki = w->channel;
													w->instrumentlockv = INST_GLOBAL_CHANNEL_MUTE;
												}
											}
										} else if (button == BUTTON1_CTRL)
										{
											w->channel = oldchannel;
											w->step = MIN(15, abs(y - w->centre));
										} else
										{
											switch (button)
											{
												case BUTTON1:
													if (w->mode != T_MODE_INSERT) /* suggest mode 0, but allow insert */
														w->mode = T_MODE_NORMAL;
													break;
												case BUTTON3: case BUTTON3_CTRL:
													if (!(w->mode == T_MODE_VISUAL || w->mode == T_MODE_VISUALLINE))
													{
														w->visualfx = tfxToVfx(oldtrackerfx);
														w->visualfy = w->trackerfy;
														w->visualchannel = oldchannel;
														w->mode = T_MODE_VISUAL;
													} break;
											}
											if (x < dx + 3) w->trackerfx = 0;
											else if (x < dx + 6) w->trackerfx = 1;
											else w->trackerfx = (x - dx - 6) / 2 + 2;
											if (w->trackerfx > (s->channelv[w->channel].macroc + 1) * 2 - 1)
												w->trackerfx = (s->channelv[w->channel].macroc + 1) * 2 - 1;
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
													case 1: if (x - dx < 5) w->fieldpointer = 1; break;
													default:
														macro = (w->trackerfx - 2) / 2;
														if (w->trackerfx % 2 == 1 && x - dx < 9 + 4*macro)
															w->fieldpointer = 1;
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
					previewNote(NOTE_OFF, INST_VOID, w->channel);
					switch (w->mode)
					{
						case T_MODE_VISUAL: case T_MODE_VISUALLINE: w->mode = T_MODE_NORMAL; break;
						case T_MODE_SONG_INSERT: case T_MODE_SONG_VISUAL: w->mode = T_MODE_SONG; break;
						case T_MODE_SONG: break;
						default: w->mode = T_MODE_NORMAL; break;
					} redraw(); break;
			} break;
		default:
			switch (w->mode)
			{
				case T_MODE_VISUALLINE:
					switch (input)
					{
						case '\n': case '\r': toggleChannelMute(); redraw(); break;
						case 'v': /* visual       */ w->mode = T_MODE_VISUAL; redraw(); break;
						case 'V': /* visual  line */ w->mode = T_MODE_NORMAL; redraw(); break;
						case 'k': /* up arrow     */ trackerUpArrow(); redraw(); break;
						case 'j': /* down arrow   */ trackerDownArrow(); redraw(); break;
						case 'h': /* left arrow   */ trackerLeftArrow(); redraw(); break;
						case 'l': /* right arrow  */ trackerRightArrow(); redraw(); break;
						case '[': /* chnl left    */ channelLeft(); redraw(); break;
						case ']': /* chnl right   */ channelRight(); redraw(); break;
						case '{': /* cycle up     */ cycleUp(); redraw(); break;
						case '}': /* cycle down   */ cycleDown(); redraw(); break;
						case '~': /* vi tilde    */
							tildePartPattern(0, 1+s->channelv[w->channel].macroc,
									MIN(w->trackerfy, w->visualfy), MAX(w->trackerfy, w->visualfy),
									MIN(w->channel, w->visualchannel), MAX(w->channel, w->visualchannel));
							redraw(); break;
						case '%': /* random */
							randPartPattern(MAX(1, w->count),
									0, 1+s->channelv[w->channel].macroc,
									MIN(w->trackerfy, w->visualfy), MAX(w->trackerfy, w->visualfy),
									MIN(w->channel, w->visualchannel), MAX(w->channel, w->visualchannel));
							redraw(); break;
						case 1: /* ^a */
							addPartPattern(MAX(1, w->count),
									0, 1+s->channelv[w->channel].macroc,
									MIN(w->trackerfy, w->visualfy), MAX(w->trackerfy, w->visualfy),
									MIN(w->channel, w->visualchannel), MAX(w->channel, w->visualchannel));
							redraw(); break;
						case 24: /* ^x */
							addPartPattern(-MAX(1, w->count),
									0, 1+s->channelv[w->channel].macroc,
									MIN(w->trackerfy, w->visualfy), MAX(w->trackerfy, w->visualfy),
									MIN(w->channel, w->visualchannel), MAX(w->channel, w->visualchannel));
							redraw(); break;
						case 'x': case 'd': /* pattern cut */
							if (s->songi[w->songfy] == 255) break;
							yankPartPattern(
									0, 1+s->channelv[w->channel].macroc,
									MIN(w->trackerfy, w->visualfy), MAX(w->trackerfy, w->visualfy),
									MIN(w->channel, w->visualchannel), MAX(w->channel, w->visualchannel));
							delPartPattern(
									0, 1+s->channelv[w->channel].macroc,
									MIN(w->trackerfy, w->visualfy), MAX(w->trackerfy, w->visualfy),
									MIN(w->channel, w->visualchannel), MAX(w->channel, w->visualchannel));
							w->trackerfx = 0;
							w->trackerfy = MIN(w->trackerfy, w->visualfy);
							w->channel = MIN(w->channel, w->visualchannel);
							w->mode = T_MODE_NORMAL;
							redraw(); break;
						case 'y': /* pattern copy */
							if (s->songi[w->songfy] == 255) break;
							yankPartPattern(
									0, 1+s->channelv[w->channel].macroc,
									MIN(w->trackerfy, w->visualfy), MAX(w->trackerfy, w->visualfy),
									MIN(w->channel, w->visualchannel), MAX(w->channel, w->visualchannel));
							w->trackerfx = 0;
							w->trackerfy = MIN(w->trackerfy, w->visualfy);
							w->channel = MIN(w->channel, w->visualchannel);
							w->mode = T_MODE_NORMAL;
							redraw(); break;
					} break;
				case T_MODE_VISUAL:
					if (w->chord)
					{
						w->count = MIN(256, w->count);
						switch (w->chord)
						{
							case 'r': /* replace */
								if (w->channel == w->visualchannel && tfxToVfx(w->trackerfx) == w->visualfx)
								{
									switch (w->trackerfx)
									{
										case 0:  /* note  */
											note = charToNote(input);
											modulorow = w->trackerfy % (s->patternv[s->patterni[s->songi[w->songfy]]]->rowcc[w->channel]+1);
											r = &s->patternv[s->patterni[s->songi[w->songfy]]]->rowv[w->channel][modulorow];
											previewNote(note, r->inst, w->channel);
											for (i = MIN(w->trackerfy, w->visualfy); i <= MAX(w->trackerfy, w->visualfy); i++)
											{
												modulorow = i % (s->patternv[s->patterni[s->songi[w->songfy]]]->rowcc[w->channel]+1);
												r = &s->patternv[s->patterni[s->songi[w->songfy]]]->rowv[w->channel][modulorow];
												insertNote(r, note);
											} break;
										case 1:  /* inst  */
											for (i = MIN(w->trackerfy, w->visualfy); i <= MAX(w->trackerfy, w->visualfy); i++)
											{
												modulorow = i % (s->patternv[s->patterni[s->songi[w->songfy]]]->rowcc[w->channel]+1);
												r = &s->patternv[s->patterni[s->songi[w->songfy]]]->rowv[w->channel][modulorow];
												insertInst(r, input);
											} break;
										default: /* macro */
											macro = (w->trackerfx - 2) / 2;
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
													insertMacrov(r, macro, input);
												}
											} break;
									}
								} redraw(); return;
						} w->count = 0;
					} else
						switch (input)
						{
							case '\n': case '\r': toggleChannelMute(); redraw(); break;
							case 'v': /* exit visual */ w->mode = T_MODE_NORMAL;     redraw(); break;
							case 'V': /* visual line */ w->mode = T_MODE_VISUALLINE; redraw(); break;
							case 'r': /* replace     */ w->chord = 'r'; redraw(); return;
							case 'k': /* up arrow    */ trackerUpArrow(); redraw(); break;
							case 'j': /* down arrow  */ trackerDownArrow(); redraw(); break;
							case 'h': /* left arrow  */ trackerLeftArrow(); redraw(); break;
							case 'l': /* right arrow */ trackerRightArrow(); redraw(); break;
							case '[': /* chnl left   */ channelLeft(); redraw(); break;
							case ']': /* chnl right  */ channelRight(); redraw(); break;
							case '{': /* cycle up    */ cycleUp(); redraw(); break;
							case '}': /* cycle down  */ cycleDown(); redraw(); break;
							case '~': /* vi tilde    */
								tildePartPattern(MIN(tfxToVfx(w->trackerfx), w->visualfx), MAX(tfxToVfx(w->trackerfx), w->visualfx),
										MIN(w->trackerfy, w->visualfy), MAX(w->trackerfy, w->visualfy),
										MIN(w->channel, w->visualchannel), MAX(w->channel, w->visualchannel));
								redraw(); break;
							case '%': /* random */
								randPartPattern(MAX(1, w->count),
										MIN(tfxToVfx(w->trackerfx), w->visualfx), MAX(tfxToVfx(w->trackerfx), w->visualfx),
										MIN(w->trackerfy, w->visualfy), MAX(w->trackerfy, w->visualfy),
										MIN(w->channel, w->visualchannel), MAX(w->channel, w->visualchannel));
								redraw(); break;
							case 1: /* ^a */
								addPartPattern(MAX(1, w->count),
										MIN(tfxToVfx(w->trackerfx), w->visualfx), MAX(tfxToVfx(w->trackerfx), w->visualfx),
										MIN(w->trackerfy, w->visualfy), MAX(w->trackerfy, w->visualfy),
										MIN(w->channel, w->visualchannel), MAX(w->channel, w->visualchannel));
								redraw(); break;
							case 24: /* ^x */
								addPartPattern(-MAX(1, w->count),
										MIN(tfxToVfx(w->trackerfx), w->visualfx), MAX(tfxToVfx(w->trackerfx), w->visualfx),
										MIN(w->trackerfy, w->visualfy), MAX(w->trackerfy, w->visualfy),
										MIN(w->channel, w->visualchannel), MAX(w->channel, w->visualchannel));
								redraw(); break;
							case 'x': case 'd': /* pattern cut */
								if (s->songi[w->songfy] == 255) break;
								yankPartPattern(
										MIN(tfxToVfx(w->trackerfx), w->visualfx), MAX(tfxToVfx(w->trackerfx), w->visualfx),
										MIN(w->trackerfy, w->visualfy), MAX(w->trackerfy, w->visualfy),
										MIN(w->channel, w->visualchannel), MAX(w->channel, w->visualchannel));
								delPartPattern(
										MIN(tfxToVfx(w->trackerfx), w->visualfx), MAX(tfxToVfx(w->trackerfx), w->visualfx),
										MIN(w->trackerfy, w->visualfy), MAX(w->trackerfy, w->visualfy),
										MIN(w->channel, w->visualchannel), MAX(w->channel, w->visualchannel));
								w->trackerfx = MIN(w->trackerfx, vfxToTfx(w->visualfx));
								w->trackerfy = MIN(w->trackerfy, w->visualfy);
								w->channel = MIN(w->channel, w->visualchannel);
								w->mode = T_MODE_NORMAL;
								redraw(); break;
							case 'y': /* pattern copy */
								if (s->songi[w->songfy] == 255) break;
								yankPartPattern(
										MIN(tfxToVfx(w->trackerfx), w->visualfx), MAX(tfxToVfx(w->trackerfx), w->visualfx),
										MIN(w->trackerfy, w->visualfy), MAX(w->trackerfy, w->visualfy),
										MIN(w->channel, w->visualchannel), MAX(w->channel, w->visualchannel));
								w->trackerfx = MIN(w->trackerfx, vfxToTfx(w->visualfx));
								w->trackerfy = MIN(w->trackerfy, w->visualfy);
								w->channel = MIN(w->channel, w->visualchannel);
								w->mode = T_MODE_NORMAL;
								redraw(); break;
						} break;
				case T_MODE_SONG:
					if (w->chord)
					{
						w->count = MIN(256, w->count);
						switch (w->chord)
						{
							case 'Q': /* record */ recordBinds(w->instrument, input); break;
							case 'K': /* keyboard macro */ /* ignores w->count */
								w->keyboardmacro = '\0';
								if (input != 'k' && input != 'K') changeMacro(input, &w->keyboardmacro);
								break;
						} w->count = 0; redraw();
					} else
						switch (input)
						{
							case '\t': /* leave song mode    */ w->mode = T_MODE_NORMAL; redraw(); break;
							case 'f':  /* toggle song follow */ w->flags ^= 0b1; redraw(); break;
							case 'Q':  /* record             */ w->chord = 'Q'; redraw(); return;
							case 'K':  /* keyboard macro     */ w->chord = 'K'; redraw(); return;
							case 'k':  /* up arrow           */ trackerUpArrow(); redraw(); break;
							case 'j':  /* down arrow         */ trackerDownArrow(); redraw(); break;
							case 'h':  /* left arrow         */ trackerLeftArrow(); redraw(); break;
							case 'l':  /* right arrow        */ trackerRightArrow(); redraw(); break;
							case '[':  /* chnl left          */ channelLeft(); redraw(); break;
							case ']':  /* chnl right         */ channelRight(); redraw(); break;
							case '{':  /* cycle up           */ cycleUp(); redraw(); break;
							case '}':  /* cycle down         */ cycleDown(); redraw(); break;
							case 'i':  /* enter insert mode  */ w->mode = T_MODE_SONG_INSERT; redraw(); break;
							case 'v':  /* enter visual mode  */
								w->visualfy = w->songfy;
								w->mode = T_MODE_SONG_VISUAL;
								redraw(); break;
							case 1: /* ^a */
								if (s->playing && s->songp == w->songfy) break;
								prunePattern(s->songi[w->songfy], w->songfy);
								s->songi[w->songfy]+=MAX(1, w->count);
								addPattern(s->songi[w->songfy], 0);
								redraw(); break;
							case 24: /* ^x */
								if (s->playing && s->songp == w->songfy) break;
								prunePattern(s->songi[w->songfy], w->songfy);
								s->songi[w->songfy]-=MAX(1, w->count);
								addPattern(s->songi[w->songfy], 0);
								if (s->songi[w->songfy] == 255 && w->songnext == w->songfy + 1)
									w->songnext = 0;
								redraw(); break;
							case '?': /* loop */
								if (s->songi[w->songfy] != 255)
									s->songf[w->songfy] = !s->songf[w->songfy];
								redraw(); break;
							case '>': /* next */
								if (s->songi[w->songfy] != 255)
								{
									if (w->songnext == w->songfy + 1) w->songnext = 0;
									else                              w->songnext = w->songfy + 1;
								} redraw(); break;
							case 'a': /* empty (add new) */
								if (s->playing && s->songp == w->songfy) break;
								prunePattern(s->songi[w->songfy], w->songfy);
								s->songi[w->songfy] = emptySongIndex(s->songi[w->songfy]);
								addPattern(s->songi[w->songfy], 0);
								redraw(); break;
							case 'c': /* clone */
								if (s->playing && s->songp == w->songfy) break;
								if (s->songi[w->songfy] != 255)
									s->songi[w->songfy] = duplicatePattern(s->songi[w->songfy]);
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
							case 'x': case 127: case '\b': /* backspace */
								if (s->playing && s->songp == w->songfy) break;
								if (w->songnext == w->songfy + 1)
									w->songnext = 0;
								prunePattern(s->songi[w->songfy], w->songfy);
								s->songi[w->songfy] = 255;
								s->songf[w->songfy] = 0;
								redraw(); break;
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
											if (s->songi[w->songfy] == 255) break;
											p = s->patternv[s->patterni[s->songi[w->songfy]]];
											j = MIN(p->rowc, w->trackerfy - 1 + MAX(1, w->count)) - w->trackerfy;
											yankPartPattern(0, 1+s->channelv[w->channel].macroc,
													w->trackerfy, w->trackerfy+j,
													w->channel, w->channel);
											delPartPattern(0, 1+s->channelv[w->channel].macroc,
													w->trackerfy, w->trackerfy+j,
													w->channel, w->channel);
											for (i = 0; i < j; i++) trackerDownArrow();
											redraw();
										} break;
									case 'y': if (input == 'y') /* yank */
										{
											if (s->songi[w->songfy] == 255) break;
											p = s->patternv[s->patterni[s->songi[w->songfy]]];
											j = MIN(p->rowc-1, w->trackerfy - 1 + MAX(1, w->count)) - w->trackerfy;
											yankPartPattern(0, 1+s->channelv[w->channel].macroc,
													w->trackerfy, w->trackerfy+j,
													w->channel, w->channel);
											for (i = 0; i < j; i++) trackerDownArrow();
											redraw();
										} break;
									case 'c': /* channel */
										switch (input)
										{
											case 'c': /* clear */
												clearPatternChannel(s, s->patterni[w->pattern], w->channel);
												resize(0); break;
											case 'a': /* add */
												for (i = 0; i < MAX(1, w->count); i++)
												{
													if (s->channelc >= MAX_CHANNELS-1) break;
													addChannel(s, w->channel+1);
													w->channel++;
													if (w->channeloffset + w->visiblechannels < s->channelc
															&& w->channel > w->visiblechannels / 2)
														w->channeloffset++;
												} resize(0); break;
											case 'A': /* add before */
												for (i = 0; i < MAX(1, w->count); i++)
												{
													if (s->channelc >= MAX_CHANNELS-1) break;
													addChannel(s, w->channel);
												} resize(0); break;
											case 'd': /* delete */
												for (i = 0; i < MAX(1, w->count); i++)
												{
													if (delChannel(w->channel)) break;
													if (w->channel > s->channelc - 1)
														w->channel--;
												} resize(0); break;
											case 'D': /* delete to end */ /* ignores w->count */
												if (w->channel == 0) w->channel++;
												for (uint8_t i = s->channelc; i > w->channel; i--)
													delChannel(i - 1);
												w->channel--;
												resize(0); break;
											case 'y': /* yank */ /* ignores w->count */
												yankChannel(w->pattern, w->channel);
												redraw(); break;
											case 'p': /* put */
												putChannel(w->pattern, w->channel);
												for (i = 1; i < MAX(1, w->count); i++)
												{
													if (s->channelc >= MAX_CHANNELS-1) break;
													w->channel++;
													putChannel(w->pattern, w->channel);
												} resize(0); break;
											case 'P': /* mix put */
												mixPutChannel(w->pattern, w->channel);
												for (i = 1; i < MAX(1, w->count); i++)
												{
													if (s->channelc >= MAX_CHANNELS-1) break;
													w->channel++;
													mixPutChannel(w->pattern, w->channel);
												} resize(0); break;
										} break;
									case 'm': /* macro */
										switch (input)
										{
											case 'a': /* add */
												for (i = 0; i < MAX(1, w->count); i++)
													if (s->channelv[w->channel].macroc < 8)
														s->channelv[w->channel].macroc++;
												resize(0); break;
											case 'd': /* delete */
												for (i = 0; i < MAX(1, w->count); i++)
												{
													if (s->channelv[w->channel].macroc > 1)
														s->channelv[w->channel].macroc--;
													if (w->trackerfx > 1 + s->channelv[w->channel].macroc * 2)
														w->trackerfx = s->channelv[w->channel].macroc * 2;
												} resize(0); break;
											case 'm': /* set */
												if (w->count) s->channelv[w->channel].macroc = MIN(8, w->count);
												else s->channelv[w->channel].macroc = 2;
												resize(0); break;
										} break;
									case 'Q': /* record */ recordBinds(w->instrument, input); redraw(); break;
									case 'K': /* keyboard macro */ /* ignores w->count */
										w->keyboardmacro = '\0';
										if (input != 'k' && input != 'K') changeMacro(input, &w->keyboardmacro);
										redraw(); break;
									case 'g': /* graphic */
										if (input == 'g') w->trackerfy = 0;
										redraw(); break;
									case 'r': /* row */
										p = s->patternv[s->patterni[s->songi[w->songfy]]];
										switch (input)
										{
											case 'c': case 'C': /* ignores count */
												j = p->rowcc[w->channel];
												p->rowcc[w->channel] = w->trackerfy;
												while (j < p->rowcc[w->channel])
													if (j < 127)
													{
														memcpy(&p->rowv[w->channel][j + 1], p->rowv[w->channel],
																sizeof(row) * (j + 1));
														j = (j + 1) * 2 - 1;
													} else
													{
														memcpy(&p->rowv[w->channel][j + 1], p->rowv[w->channel],
																sizeof(row) * (255 - j));
														j = 255; break;
													}
												redraw(); break;
											case 'r': case 'R':
												j = p->rowcc[w->channel];
												if (w->count) p->rowcc[w->channel] = w->count - 1;
												else          p->rowcc[w->channel] = p->rowc;
												while (j < p->rowcc[w->channel])
													if (j < 127)
													{
														memcpy(&p->rowv[w->channel][j + 1], p->rowv[w->channel],
																sizeof(row) * (j + 1));
														j = (j + 1) * 2 - 1;
													} else
													{
														memcpy(&p->rowv[w->channel][j + 1], p->rowv[w->channel],
																sizeof(row) * (255 - j));
														j = 255; break;
													}
												redraw(); break;
											case 'd': case 'D':
												for (i = 0; i < MAX(1, w->count); i++)
													if (p->rowcc[w->channel]) p->rowcc[w->channel]--;
												redraw(); break;
											case 'a': case 'A':
												for (i = 0; i < MAX(1, w->count); i++)
													if (p->rowcc[w->channel] < 255)
													{
														p->rowcc[w->channel]++;
														memset(&p->rowv[w->channel][p->rowcc[w->channel]], 0, sizeof(row));
														p->rowv[w->channel][p->rowcc[w->channel]].note = NOTE_VOID;
														p->rowv[w->channel][p->rowcc[w->channel]].inst = INST_VOID;
													} else break;
												redraw(); break;
											case '-': case '_':
												for (i = 0; i < MAX(1, w->count); i++)
													if (p->rowcc[w->channel] == 255)
														p->rowcc[w->channel] = 127;
													else if (p->rowcc[w->channel])
														p->rowcc[w->channel] = (p->rowcc[w->channel] + 1) / 2 - 1;
													else break;
												redraw(); break;
											case '+': case '=':
												for (i = 0; i < MAX(1, w->count); i++)
													if (p->rowcc[w->channel] < 127)
													{
														memcpy(&p->rowv[w->channel][p->rowcc[w->channel] + 1], p->rowv[w->channel],
																sizeof(row) * (p->rowcc[w->channel] + 1));
														p->rowcc[w->channel] = (p->rowcc[w->channel] + 1) * 2 - 1;
													} else
													{
														memcpy(&p->rowv[w->channel][p->rowcc[w->channel] + 1], p->rowv[w->channel],
																sizeof(row) * (255 - p->rowcc[w->channel]));
														p->rowcc[w->channel] = 255;
														break;
													}
												redraw(); break;
											case '/': case '?':
												for (i = 0; i < MAX(1, w->count); i++)
													if (p->rowcc[w->channel] == 255)
													{
														for (j = 0; j < 127; j++)
															p->rowv[w->channel][j] = p->rowv[w->channel][j*2];
														p->rowcc[w->channel] = 127;
													} else if (p->rowcc[w->channel])
													{
														for (j = 0; j < p->rowcc[w->channel] + 1; j++)
															p->rowv[w->channel][j] = p->rowv[w->channel][j*2];
														p->rowcc[w->channel] = (p->rowcc[w->channel] + 1) / 2 - 1;
													} else break;
												redraw(); break;
											case '*': /* no shift bind for this one */
												for (i = 0; i < MAX(1, w->count); i++)
													if (p->rowcc[w->channel] < 127)
													{
														for (j = p->rowcc[w->channel] + 1; j > 0; j--)
														{
															p->rowv[w->channel][j*2] = p->rowv[w->channel][j];
															memset(&p->rowv[w->channel][j*2-1], 0, sizeof(row));
															p->rowv[w->channel][j*2-1].note = NOTE_VOID;
															p->rowv[w->channel][j*2-1].inst = INST_VOID;
														} p->rowcc[w->channel] = (p->rowcc[w->channel] + 1) * 2 - 1;
													} else if (p->rowcc[w->channel] < 255)
													{
														for (j = p->rowcc[w->channel] + 1; j > 0; j--)
														{
															p->rowv[w->channel][j*2] = p->rowv[w->channel][j];
															memset(&p->rowv[w->channel][j*2-1], 0, sizeof(row));
															p->rowv[w->channel][j*2-1].note = NOTE_VOID;
															p->rowv[w->channel][j*2-1].inst = INST_VOID;
														} p->rowcc[w->channel] = 255;
													} else break;
												redraw(); break;
										}
										p->rowc = 0;
										for (i = 0; i < MAX_CHANNELS; i++)
											p->rowc = MAX(p->rowc, p->rowcc[i]);
										w->trackerfy = MIN(p->rowc, w->trackerfy);
										break;
									case 'R': /* global row */
										p = s->patternv[s->patterni[s->songi[w->songfy]]];
										switch (input)
										{
											case 'c': case 'C': /* ignores count */
												k = p->rowcc[w->channel];
												for (i = 0; i < MAX_CHANNELS; i++)
												{
													j = k;
													p->rowcc[i] = w->trackerfy;
													while (j < p->rowcc[i])
														if (j < 127)
														{
															memcpy(&p->rowv[i][j + 1], p->rowv[i],
																	sizeof(row) * (j + 1));
															j = (j + 1) * 2 - 1;
														} else
														{
															memcpy(&p->rowv[i][j + 1], p->rowv[i],
																	sizeof(row) * (255 - j));
															j = 255; break;
														}
												} redraw(); break;
											case 'r': case 'R':
												k = p->rowcc[w->channel];
												for (i = 0; i < MAX_CHANNELS; i++)
												{
													j = k;
													if (w->count) p->rowcc[i] = w->count - 1;
													else          p->rowcc[i] = p->rowc;
													while (j < p->rowcc[i])
														if (j < 127)
														{
															memcpy(&p->rowv[i][j + 1], p->rowv[i],
																	sizeof(row) * (j + 1));
															j = (j + 1) * 2 - 1;
														} else
														{
															memcpy(&p->rowv[i][j + 1], p->rowv[i],
																	sizeof(row) * (255 - j));
															j = 255; break;
														}
												} redraw(); break;
											case 'd': case 'D':
												for (i = 0; i < MAX_CHANNELS; i++)
													for (j = 0; j < MAX(1, w->count); j++) if (p->rowcc[i]) p->rowcc[i]--;
												redraw(); break;
											case 'a': case 'A':
												for (i = 0; i < MAX_CHANNELS; i++)
													for (j = 0; j < MAX(1, w->count); j++)
														if (p->rowcc[i] < 255)
														{
															p->rowcc[i]++;
															memset(&p->rowv[i][p->rowcc[i]], 0, sizeof(row));
															p->rowv[i][p->rowcc[i]].note = NOTE_VOID;
															p->rowv[i][p->rowcc[i]].inst = INST_VOID;
														} else break;
												redraw(); break;
											case '-': case '_':
												for (i = 0; i < MAX_CHANNELS; i++)
													for (j = 0; j < MAX(1, w->count); j++)
													{
														if (p->rowcc[i] == 255)
															p->rowcc[i] = 127;
														else if (p->rowcc[i])
															p->rowcc[i] = (p->rowcc[i] + 1) / 2 - 1;
														else break;
													}
												redraw(); break;
											case '+': case '=':
												for (i = 0; i < MAX_CHANNELS; i++)
													for (j = 0; j < MAX(1, w->count); j++)
														if (p->rowcc[i] < 127)
														{
															memcpy(&p->rowv[i][p->rowcc[i] + 1], p->rowv[i],
																	sizeof(row) * (p->rowcc[i] + 1));
															p->rowcc[i] = (p->rowcc[i] + 1) * 2 - 1;
														} else
														{
															memcpy(&p->rowv[w->channel][p->rowcc[i] + 1], p->rowv[w->channel],
																	sizeof(row) * (255 - p->rowcc[i]));
															p->rowcc[i] = 255;
															break;
														}
												redraw(); break;
											case '/': case '?':
												for (i = 0; i < MAX_CHANNELS; i++)
													for (k = 0; k < MAX(1, w->count); k++)
														if (p->rowcc[i] == 255)
														{
															for (j = 0; j < 127; j++)
																p->rowv[i][j] = p->rowv[i][j*2];
															p->rowcc[i] = 127;
														} else if (p->rowcc[i])
														{
															for (j = 0; j < p->rowcc[i] + 1; j++)
																p->rowv[i][j] = p->rowv[i][j*2];
															p->rowcc[i] = (p->rowcc[i] + 1) / 2 - 1;
														} else break;
												redraw(); break;
											case '*': /* no shift bind for this one */
												for (i = 0; i < MAX_CHANNELS; i++)
													for (k = 0; k < MAX(1, w->count); k++)
														if (p->rowcc[i] < 127)
														{
															for (j = p->rowcc[i] + 1; j > 0; j--)
															{
																p->rowv[i][j*2] = p->rowv[i][j];
																memset(&p->rowv[i][j*2-1], 0, sizeof(row));
																p->rowv[i][j*2-1].note = NOTE_VOID;
																p->rowv[i][j*2-1].inst = INST_VOID;
															}
															p->rowcc[i] = (p->rowcc[i] + 1) * 2 - 1;
														} else if (p->rowcc[i] < 255)
														{
															for (j = p->rowcc[i] + 1; j > 0; j--)
															{
																p->rowv[i][j*2] = p->rowv[i][j];
																memset(&p->rowv[i][j*2-1], 0, sizeof(row));
																p->rowv[i][j*2-1].note = NOTE_VOID;
																p->rowv[i][j*2-1].inst = INST_VOID;
															}
															p->rowcc[i] = 255;
														} else break;
												redraw(); break;
										}
										p->rowc = 0;
										for (i = 0; i < MAX_CHANNELS; i++)
											p->rowc = MAX(p->rowc, p->rowcc[i]);
										w->trackerfy = MIN(p->rowc, w->trackerfy);
										break;
								} w->count = 0;
							} else
							{
								modulorow = w->trackerfy % (s->patternv[s->patterni[s->songi[w->songfy]]]->rowcc[w->channel]+1);
								r = &s->patternv[s->patterni[s->songi[w->songfy]]]->rowv[w->channel][modulorow];
								switch (input)
								{
									case '\n': case '\r': toggleChannelMute(); redraw(); break;
									case 'f': /* toggle song follow */ w->flags ^= 0b1; redraw(); break;
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
									case 'v': /* enter visual mode  */
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
									case 'Q':  /* record          */ w->chord = 'Q'; redraw(); return;
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
									case 'p':  /* pattern put     */ /* TODO: count */
										if (s->songi[w->songfy] == 255) break;
										putPartPattern();
										w->trackerfy = MIN(w->trackerfy + w->pbfy[1] - w->pbfy[0],
												s->patternv[s->patterni[s->songi[w->songfy]]]->rowc - 1);
										trackerDownArrow();
										redraw(); break;
									case 'P': /* pattern put before */ /* TODO: count */
										if (s->songi[w->songfy] == 255) break;
										putPartPattern();
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
												macro = (w->trackerfx - 2) / 2;
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
							} break;
					} break;
				case T_MODE_SONG_VISUAL:
					switch (input)
					{
						case 'v': w->mode = T_MODE_SONG; redraw(); break;
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
						case 'd': case 'x': /* song delete */
							w->songbufferlen = MAX(w->songfy, w->visualfy) - MIN(w->songfy, w->visualfy) +1;
							if (w->songbufferlen)
							{
								memcpy(w->songibuffer, &s->songi[MIN(w->songfy, w->visualfy)], w->songbufferlen);
								memcpy(w->songfbuffer, &s->songf[MIN(w->songfy, w->visualfy)], w->songbufferlen);
								for (i = 0; i < w->songbufferlen; i++)
								{
									s->songi[MIN(w->songfy, w->visualfy)+i] = 255;
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
								if (s->songi[MIN(w->songfy, w->visualfy)+i] != 255)
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
							prunePattern(s->songi[w->songfy], w->songfy);
							s->songi[w->songfy] = 255;
							s->songf[w->songfy] = 0;
							redraw(); break;
					} break;
				case T_MODE_INSERT:
					if (s->songi[w->songfy] == 255) break;
					modulorow = w->trackerfy % (s->patternv[s->patterni[s->songi[w->songfy]]]->rowcc[w->channel]+1);
					r = &s->patternv[s->patterni[s->songi[w->songfy]]]->rowv[w->channel][modulorow];
					if (input == '\n' || input == '\r')
					{
						toggleChannelMute(); redraw();
					} else
						switch (w->trackerfx)
						{
							case 0: /* note */
								switch (input)
								{
									case 1:  /* ^a */ r->note+=MAX(1, w->count); break;
									case 24: /* ^x */ r->note-=MAX(1, w->count); break;
									case ' ': /* space */
										r->note = NOTE_OFF;
										w->trackerfy += w->step;
										if (w->trackerfy > s->patternv[s->patterni[s->songi[w->songfy]]]->rowc)
										{
											if (w->songfy < 255 && s->songi[w->songfy + 1] != 255)
											{
												w->trackerfy = 0;
												w->songfy++;
											} else w->trackerfy = s->patternv[s->patterni[s->songi[w->songfy]]]->rowc;
										} break;
									case 127: case '\b': /* backspace */
										r->note = NOTE_VOID;
										r->inst = INST_VOID;
										w->trackerfy -= w->step;
										if (w->trackerfy < 0)
										{
											if (w->songfy > 0 && s->songi[w->songfy - 1] != 255)
											{
												w->songfy--;
												w->trackerfy = s->patternv[s->patterni[s->songi[w->songfy]]]->rowc;
											} else w->trackerfy = 0;
										} break;
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
										note = charToNote(input);
										insertNote(r, note);
										previewNote(note, r->inst, w->channel);
										w->trackerfy += w->step;
										if (w->trackerfy > s->patternv[s->patterni[s->songi[w->songfy]]]->rowc)
										{
											if (w->songfy < 255 && s->songi[w->songfy + 1] != 255)
											{
												w->trackerfy = 0;
												w->songfy++;
											} else w->trackerfy = s->patternv[s->patterni[s->songi[w->songfy]]]->rowc;
										} break;
								} break;
							case 1: /* instrument */ insertInst(r, input); break;
							default: /* macros */
								macro = (w->trackerfx - 2) / 2;
								switch (input)
								{
									case '~': /* toggle case */
										if      (isupper(r->macro[macro].c)) r->macro[macro].c += 32;
										else if (islower(r->macro[macro].c)) r->macro[macro].c -= 32;
										break;
									default:
										if (!(w->trackerfx%2)) insertMacroc(r, macro, input);
										else                   insertMacrov(r, macro, input);
										break;
								} break;
						} redraw(); break;
			} break;
	}
	if (w->count) { w->count = 0; redraw(); }
	if (w->chord) { w->chord = '\0'; redraw(); }
	pushPatternHistoryIfNew(s->patternv[s->patterni[s->songi[w->songfy]]]);
}
