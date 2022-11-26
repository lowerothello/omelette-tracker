#define TRACK_TRIG_COLS 5

#include "chord/row.c"
#include "chord/track.c"
#include "chord/macro.c"
#include "chord/loop.c"
#include "chord/yank.c"
#include "chord/delete.c"
#include "chord/graphic.c"
#include "chord/insert.c"

void inputPatternHex(Row *r, char value) /* TODO: remove */
{
	short macro;
	switch (w->trackerfx)
	{
		case 0: /* should never be reached */ break;
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

void trackerAdjustRight(TrackData *cd) /* mouse adjust only */
{
	Row *r = getTrackRow(&s->track->v[w->track].data, w->trackerfy);
	short macro;
	switch (w->trackerfx)
	{
		case -1:
			if (!s->playing)
			{
				if (w->fieldpointer) setVariantChainTrig(&cd->variant, w->trackerfy, cd->variant->trig[w->trackerfy].index+16);
				else                 setVariantChainTrig(&cd->variant, w->trackerfy, cd->variant->trig[w->trackerfy].index+1);
			} break;
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
void trackerAdjustLeft(TrackData *cd) /* mouse adjust only */
{
	Row *r = getTrackRow(&s->track->v[w->track].data, w->trackerfy);
	short macro;
	switch (w->trackerfx)
	{
		case -1:
			if (!s->playing)
			{
				if (w->fieldpointer) setVariantChainTrig(&cd->variant, w->trackerfy, cd->variant->trig[w->trackerfy].index-16);
				else                 setVariantChainTrig(&cd->variant, w->trackerfy, cd->variant->trig[w->trackerfy].index-1);
			} break;
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

void trackerUpArrow(size_t count)
{
	count *= MAX(1, w->count);
	switch (w->page)
	{
		case PAGE_TRACK_VARIANT:
			w->follow = 0;
			if (count > w->trackerfy) w->trackerfy = 0;
			else                      w->trackerfy -= count;
			break;
		case PAGE_TRACK_EFFECT: effectUpArrow(count); break;
		default: break;
	} p->redraw = 1;
}

void trackerDownArrow(size_t count)
{
	count *= MAX(1, w->count);
	switch (w->page)
	{
		case PAGE_TRACK_VARIANT:
			w->follow = 0;
			if (count > s->songlen - w->trackerfy -1) w->trackerfy = s->songlen-1;
			else                                      w->trackerfy += count;
			break;
		case PAGE_TRACK_EFFECT: effectDownArrow(count); break;
		default: break;
	} p->redraw = 1;
}

void trackerLeftArrow(size_t count)
{
	count *= MAX(1, w->count);
	switch (w->page)
	{
		case PAGE_TRACK_VARIANT:
			for (int i = 0; i < count; i++)
			{
				if      (w->trackerfx == 2 + (s->track->v[w->track].data.variant->macroc<<1)) w->trackerfx = 1;
				else if (w->trackerfx == TRACKERFX_MIN)
				{
					if (w->track > 0)
					{
						w->track--;
						w->trackerfx = 3;
					}
				} else if (w->trackerfx > 1)
				{
					if (w->trackerfx&0x1) w->trackerfx--;
					else                  w->trackerfx+=3;
				} else w->trackerfx--;
			} break;
		case PAGE_TRACK_EFFECT: effectLeftArrow(); break;
		default: break;
	} p->redraw = 1;
}

void trackLeft(size_t count)
{
	count *= MAX(1, w->count);
	if (count > w->track) w->track = 0;
	else                    w->track -= count;
	if (w->trackerfx > 3 + s->track->v[w->track].data.variant->macroc * 2)
		w->trackerfx = 3 + s->track->v[w->track].data.variant->macroc * 2;
	p->redraw = 1;
}

void trackerRightArrow(size_t count)
{
	count *= MAX(1, w->count);
	switch (w->page)
	{
		case PAGE_TRACK_VARIANT:
			for (int i = 0; i < count; i++)
			{
				if      (w->trackerfx == 1) w->trackerfx = 2 + s->track->v[w->track].data.variant->macroc * 2;
				else if (w->trackerfx == 3)
				{
					if (w->track < s->track->c-1)
					{
						w->track++;
						w->trackerfx = TRACKERFX_MIN;
					} else w->trackerfx = 3;
				} else if (w->trackerfx > 1)
				{
					if (w->trackerfx&0x1) w->trackerfx-=3;
					else                  w->trackerfx++;
				} else w->trackerfx++;
			} break;
		case PAGE_TRACK_EFFECT: effectRightArrow(); break;
		default: break;
	} p->redraw = 1;
}

void trackRight(size_t count)
{
	count *= MAX(1, w->count);
	w->track += count;
	if (w->track > s->track->c-1) w->track = s->track->c-1;
	if (w->trackerfx > 3 + s->track->v[w->track].data.variant->macroc * 2)
		w->trackerfx = 3 + s->track->v[w->track].data.variant->macroc * 2;
	p->redraw = 1;
}

void trackerHome(void)
{
	switch (w->page)
	{
		case PAGE_TRACK_VARIANT:
			w->follow = 0;
			if (w->trackerfy == STATE_ROWS) w->track = 0;
			else                            w->trackerfy = STATE_ROWS;
			break;
		case PAGE_TRACK_EFFECT: effectHome(); break;
		default: break;
	} p->redraw = 1;
}

void trackerEnd(void)
{
	switch (w->page)
	{
		case PAGE_TRACK_VARIANT:
			w->follow = 0;
			if (w->trackerfy == s->songlen-1) w->track = s->track->c-1;
			else                              w->trackerfy = s->songlen-1;
			break;
		case PAGE_TRACK_EFFECT: effectEnd(); break;
		default: break;
	} p->redraw = 1;
}

void cycleUp(size_t count) /* TODO: count */
{
	count *= MAX(1, w->count);
	Variant *v;
	int bound;
	switch (w->page)
	{
		case PAGE_TRACK_VARIANT:
			switch (w->mode)
			{
				/* TODO: variant trig mode and variant trig visual mode handling */
				case T_MODE_NORMAL: case T_MODE_INSERT:
					for (int i = 0; i < count; i++)
					{
						bound = getVariantChainVariant(&v, s->track->v[w->track].data.variant, w->trackerfy);
						if (bound != -1) cycleVariantUp(v, bound);
					} break;
				case T_MODE_VISUAL: case T_MODE_VISUALREPLACE:
					cycleUpPartPattern(MIN(tfxToVfx(w->trackerfx), w->visualfx), MAX(tfxToVfx(w->trackerfx), w->visualfx), MIN(w->trackerfy, w->visualfy), MAX(w->trackerfy, w->visualfy), MIN(w->track, w->visualtrack), MAX(w->track, w->visualtrack));
					break;
				case T_MODE_VISUALLINE:
					cycleUpPartPattern(0, 2+s->track->v[w->track].data.variant->macroc, MIN(w->trackerfy, w->visualfy), MAX(w->trackerfy, w->visualfy), MIN(w->track, w->visualtrack), MAX(w->track, w->visualtrack));
					break;
				default: break;
			} break;
		default: break;
	} p->redraw = 1;
}

void cycleDown(size_t count) /* TODO: count */
{
	count *= MAX(1, w->count);
	Variant *v;
	int bound;
	switch (w->page)
	{
		case PAGE_TRACK_VARIANT:
			switch (w->mode)
			{
				/* TODO: variant trig mode and variant trig visual mode handling */
				case T_MODE_NORMAL: case T_MODE_INSERT:
					for (int i = 0; i < count; i++)
					{
						bound = getVariantChainVariant(&v, s->track->v[w->track].data.variant, w->trackerfy);
						if (bound != -1) cycleVariantDown(v, bound);
					} break;
				case T_MODE_VISUAL: case T_MODE_VISUALREPLACE:
					cycleDownPartPattern(MIN(tfxToVfx(w->trackerfx), w->visualfx), MAX(tfxToVfx(w->trackerfx), w->visualfx), MIN(w->trackerfy, w->visualfy), MAX(w->trackerfy, w->visualfy), MIN(w->track, w->visualtrack), MAX(w->track, w->visualtrack));
					break;
				case T_MODE_VISUALLINE:
					cycleDownPartPattern(0, 2+s->track->v[w->track].data.variant->macroc, MIN(w->trackerfy, w->visualfy), MAX(w->trackerfy, w->visualfy), MIN(w->track, w->visualtrack), MAX(w->track, w->visualtrack));
					break;
				default: break;
			} break;
		default: break;
	} p->redraw = 1;
}

void shiftUp(size_t count)
{
	count *= MAX(1, w->count);
	TrackData *cd;
	switch (w->page)
	{
		case PAGE_TRACK_VARIANT:
			trackerUpArrow(count);
			for (uint8_t i = 0; i < s->track->c; i++)
			{
				cd = &s->track->v[i].data;
				memmove(&cd->variant->trig      [w->trackerfy], &cd->variant->trig      [w->trackerfy + count], sizeof(Vtrig) * (s->songlen - w->trackerfy - count));
				memmove(&cd->variant->main->rowv[w->trackerfy], &cd->variant->main->rowv[w->trackerfy + count], sizeof(Row)   * (s->songlen - w->trackerfy - count));
			}
			if (s->loop[1])
			{
				if (s->loop[0] > w->trackerfy) s->loop[0] -= count;
				if (s->loop[1] > w->trackerfy) s->loop[1] -= count;
			} regenGlobalRowc(s); break;
		default: break;
	} p->redraw = 1;
}

void shiftDown(size_t count)
{
	count *= MAX(1, w->count);
	TrackData *cd;
	switch (w->page)
	{
		case PAGE_TRACK_VARIANT:
			for (uint8_t i = 0; i < s->track->c; i++)
			{
				cd = &s->track->v[i].data;
				memmove(&cd->variant->trig      [w->trackerfy + count], &cd->variant->trig      [w->trackerfy], sizeof(Vtrig) * (s->songlen - w->trackerfy - count));
				memmove(&cd->variant->main->rowv[w->trackerfy + count], &cd->variant->main->rowv[w->trackerfy], sizeof(Row)   * (s->songlen - w->trackerfy - count));

				/* zero out the new row(s) */
				memset(&cd->variant->main->rowv[w->trackerfy], 0, sizeof(Row) * count);
				for (int i = 0; i < count; i++)
				{
					cd->variant->main->rowv[w->trackerfy+i].note = NOTE_VOID;
					cd->variant->main->rowv[w->trackerfy+i].inst = INST_VOID;
					cd->variant->trig[w->trackerfy+i].index = VARIANT_VOID;
					cd->variant->trig[w->trackerfy+i].flags = 0;
				}
			}
			if (s->loop[1])
			{
				if (s->loop[0] >= w->trackerfy) s->loop[0] += count;
				if (s->loop[1] >= w->trackerfy) s->loop[1] += count;
			} trackerDownArrow(count); regenGlobalRowc(s); break;
		default: break;
	} p->redraw = 1;
}


void trackerPgUp(size_t count)
{
	switch (w->page)
	{
		case PAGE_TRACK_VARIANT: trackerUpArrow(s->rowhighlight*count); break;
		case PAGE_TRACK_EFFECT:  effectPgUp(s->track->v[w->track].data.effect, count); break;
		default: break;
	}
}

void trackerPgDn(size_t count)
{
	switch (w->page)
	{
		case PAGE_TRACK_VARIANT: trackerDownArrow(s->rowhighlight*count); break;
		case PAGE_TRACK_EFFECT:  effectPgDn(s->track->v[w->track].data.effect, count); break;
		default: break;
	}
}



void insertVtrig(TrackData *cd, uint16_t fy, int input)
{
	switch (input)
	{
		case '0':           inputVariantChainTrig(&cd->variant, fy, 0);  break;
		case '1':           inputVariantChainTrig(&cd->variant, fy, 1);  break;
		case '2':           inputVariantChainTrig(&cd->variant, fy, 2);  break;
		case '3':           inputVariantChainTrig(&cd->variant, fy, 3);  break;
		case '4':           inputVariantChainTrig(&cd->variant, fy, 4);  break;
		case '5':           inputVariantChainTrig(&cd->variant, fy, 5);  break;
		case '6':           inputVariantChainTrig(&cd->variant, fy, 6);  break;
		case '7':           inputVariantChainTrig(&cd->variant, fy, 7);  break;
		case '8':           inputVariantChainTrig(&cd->variant, fy, 8);  break;
		case '9':           inputVariantChainTrig(&cd->variant, fy, 9);  break;
		case 'A': case 'a': inputVariantChainTrig(&cd->variant, fy, 10); break;
		case 'B': case 'b': inputVariantChainTrig(&cd->variant, fy, 11); break;
		case 'C': case 'c': inputVariantChainTrig(&cd->variant, fy, 12); break;
		case 'D': case 'd': inputVariantChainTrig(&cd->variant, fy, 13); break;
		case 'E': case 'e': inputVariantChainTrig(&cd->variant, fy, 14); break;
		case 'F': case 'f': inputVariantChainTrig(&cd->variant, fy, 15); break;
		case ' ':                            setVariantChainTrig(&cd->variant, fy, VARIANT_OFF ); break;
		case 127: case '\b': /* backspace */ setVariantChainTrig(&cd->variant, fy, VARIANT_VOID); break;
		case 1:  /* ^a */                    addPartPattern( MAX(1, w->count), tfxToVfx(w->trackerfx), tfxToVfx(w->trackerfx), w->trackerfy, w->trackerfy, w->track, w->track, 0, 1); break;
		case 24: /* ^x */                    addPartPattern(-MAX(1, w->count), tfxToVfx(w->trackerfx), tfxToVfx(w->trackerfx), w->trackerfy, w->trackerfy, w->track, w->track, 0, 1); break;
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

			if (w->keyboardmacro)
			{
				if (charToKmode(input, linkMacroNibbles(w->keyboardmacro), &r->macro[0].v, &r->note))
				{
					r->macro[0].c   = w->keyboardmacro;
					r->macro[0].alt = w->keyboardmacroalt;
				}
			} else charToNote(input, &r->note);
			break;
	}
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
	}
}
int insertMacroc(Macro *m, int input, bool alt)
{
	int ret = 1;
	switch (input)
	{
		case 127: case '\b': /* backspace */ m->c = 0; break;
		default:
			if (!m->c) m->v = 0;
			ret = changeMacro(input, &m->c, &m->alt, alt);
			break;
	} return ret;
}
bool insertMacrov(Macro *m, int input)
{
	if (m->c)
		switch (input)
		{
			case 1:  /* ^a */
				if (linkMacroNibbles(m->c)) m->v += MAX(1, w->count)*16;
				else                        m->v += MAX(1, w->count);
				return 1;
			case 24: /* ^x */
				if (linkMacroNibbles(m->c)) m->v -= MAX(1, w->count)*16;
				else                        m->v -= MAX(1, w->count);
				return 1;
			case 127: case '\b': /* backspace */ m->c = 0; return 1;
			case '0':            m->v <<= 4; m->v += 0x0;  return 1;
			case '1':            m->v <<= 4; m->v += 0x1;  return 1;
			case '2':            m->v <<= 4; m->v += 0x2;  return 1;
			case '3':            m->v <<= 4; m->v += 0x3;  return 1;
			case '4':            m->v <<= 4; m->v += 0x4;  return 1;
			case '5':            m->v <<= 4; m->v += 0x5;  return 1;
			case '6':            m->v <<= 4; m->v += 0x6;  return 1;
			case '7':            m->v <<= 4; m->v += 0x7;  return 1;
			case '8':            m->v <<= 4; m->v += 0x8;  return 1;
			case '9':            m->v <<= 4; m->v += 0x9;  return 1;
			case 'A': case 'a':  m->v <<= 4; m->v += 0xa;  return 1;
			case 'B': case 'b':  m->v <<= 4; m->v += 0xb;  return 1;
			case 'C': case 'c':  m->v <<= 4; m->v += 0xc;  return 1;
			case 'D': case 'd':  m->v <<= 4; m->v += 0xd;  return 1;
			case 'E': case 'e':  m->v <<= 4; m->v += 0xe;  return 1;
			case 'F': case 'f':  m->v <<= 4; m->v += 0xf;  return 1;
		}
	return 0;
}
/* returns true if work was done */
bool insertMacro(Macro *m, int input, bool alt)
{
	switch (input)
	{
		case '~': /* toggle case */
			if      (isupper(m->c)) m->c += 32;
			else if (islower(m->c)) m->c -= 32;
			return 1;
		default:
			if (!(w->trackerfx&1)) return insertMacroc(m, input, alt);
			else                   return insertMacrov(m, input);
	} return 0;
}

void applyTrackMutes(void)
{
	Event e;
	e.sem = M_SEM_TRACK_MUTE;
	pushEvent(&e);
}
void toggleTrackMute(uint8_t track)
{
	s->track->v[track].data.mute = !s->track->v[track].data.mute;
	applyTrackMutes();
}
void toggleTrackSolo(uint8_t track)
{
	bool flush = 1; /* all tracks except the toggled one should be muted */
	bool reset = 1; /* all tracks should be unmuted                      */

	for (int i = 0; i < s->track->c; i++)
	{
		if ( s->track->v[i].data.mute)                 flush = 0;
		if ( s->track->v[i].data.mute && i == track) reset = 0;
		if (!s->track->v[i].data.mute && i != track) reset = 0;
	}

	if (flush && !reset)
	{
		for (int i = 0; i < s->track->c; i++)
			if (i != track) s->track->v[i].data.mute = 1;
		applyTrackMutes();
	} else if (reset && !flush)
	{
		for (int i = 0; i < s->track->c; i++)
			s->track->v[i].data.mute = 0;
		applyTrackMutes();
	} else toggleTrackMute(track);
}

int trackerMouseHeader(int button, int x, int y, short *tx)
{
	if (y <= TRACK_ROW)
		for (int i = 0; i < s->track->c; i++)
		{
			*tx += TRACK_TRIG_COLS + 8 + 4*(s->track->v[i].data.variant->macroc+1);
			if (*tx > x)
				switch (button)
				{
					case BUTTON1: case BUTTON1_CTRL: w->trackoffset = i - w->track; return 1;
					case BUTTON2: case BUTTON2_CTRL: toggleTrackSolo(i); return 1;
					case BUTTON3: case BUTTON3_CTRL: toggleTrackMute(i); return 1;
				}
		}
	return 0;
}
