#include "visual.h"
#include "draw.c"

#include "variant.c"
#include "track.c"

#include "chord/row.c"
#include "chord/track.c"
#include "chord/macro.c"
#include "chord/loop.c"
#include "chord/yank.c"
#include "chord/delete.c"
#include "chord/graphic.c"
#include "chord/insert.c"

#include "input.c"

uint8_t changeNoteOctave(uint8_t octave, uint8_t note)
{
	w->octave = octave;
	if (note == NOTE_VOID) return NOTE_VOID;

	octave *= 12;
	while (note > octave + 11) note -= 12;
	while (note < octave + 0)  note += 12;
	return MIN(NOTE_A10-1, note);
}

void trackerUpArrow(size_t count)
{
	count *= MAX(1, w->count);
	switch (w->mode)
	{
		case MODE_EFFECT:
			decControlCursor(&cc, count);
			break;
		default:
			w->follow = 0;
			if (count > w->trackerfy) w->trackerfy = 0;
			else                      w->trackerfy -= count;
			break;
	}
	p->redraw = 1;
}

void trackerDownArrow(size_t count)
{
	count *= MAX(1, w->count);
	switch (w->mode)
	{
		case MODE_EFFECT:
			incControlCursor(&cc, count);
			break;
		default:
			w->follow = 0;
			if (count > s->songlen - w->trackerfy -1) w->trackerfy = s->songlen-1;
			else                                      w->trackerfy += count;
			break;
	}
	p->redraw = 1;
}

void trackerLeftArrow(size_t count)
{
	int i;
	count *= MAX(1, w->count);
	switch (w->mode)
	{
		case MODE_EFFECT:
			for (i = 0; i < count; i++)
				incControlFieldpointer(&cc);
			break;
		default:
			for (i = 0; i < count; i++)
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
	int i;
	count *= MAX(1, w->count);
	switch (w->mode)
	{
		case MODE_EFFECT:
			for (i = 0; i < count; i++)
				decControlFieldpointer(&cc);
			break;
		default:
			for (i = 0; i < count; i++)
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
	switch (w->mode)
	{
		case MODE_EFFECT:
			setControlCursor(&cc, 0);
			break;
		default:
			w->follow = 0;
			if (w->trackerfy == STATE_ROWS) w->track = 0;
			else                            w->trackerfy = STATE_ROWS;
			break;
	} p->redraw = 1;
}

void trackerEnd(void)
{
	switch (w->mode)
	{
		case MODE_EFFECT:
			setControlCursor(&cc, cc.controlc-1);
			break;
		default:
			w->follow = 0;
			if (w->trackerfy == s->songlen-1) w->track = s->track->c-1;
			else                              w->trackerfy = s->songlen-1;
			break;
	} p->redraw = 1;
}

void cycleUp(size_t count)
{
	count *= MAX(1, w->count);
	Variant *v;
	int bound;
	switch (w->page)
	{
		case PAGE_VARIANT:
			switch (w->mode)
			{
				/* TODO: variant trig mode and variant trig visual mode handling */
				case MODE_NORMAL: case MODE_INSERT:
					for (int i = 0; i < count; i++)
					{
						bound = getVariantChainVariant(&v, s->track->v[w->track].data.variant, w->trackerfy);
						if (bound != -1) cycleVariantUp(v, bound);
					} break;
				case MODE_VISUAL: case MODE_VISUALREPLACE:
					cycleUpPartPattern(count, MIN(tfxToVfx(w->trackerfx), w->visualfx), MAX(tfxToVfx(w->trackerfx), w->visualfx), MIN(w->trackerfy, w->visualfy), MAX(w->trackerfy, w->visualfy), MIN(w->track, w->visualtrack), MAX(w->track, w->visualtrack));
					break;
				case MODE_VISUALLINE:
					cycleUpPartPattern(count, 0, 2+s->track->v[w->track].data.variant->macroc, MIN(w->trackerfy, w->visualfy), MAX(w->trackerfy, w->visualfy), MIN(w->track, w->visualtrack), MAX(w->track, w->visualtrack));
					break;
				default: break;
			} break;
		default: break;
	} p->redraw = 1;
}

void cycleDown(size_t count)
{
	count *= MAX(1, w->count);
	Variant *v;
	int bound;
	switch (w->page)
	{
		case PAGE_VARIANT:
			switch (w->mode)
			{
				/* TODO: variant trig mode and variant trig visual mode handling */
				case MODE_NORMAL: case MODE_INSERT:
					for (int i = 0; i < count; i++)
					{
						bound = getVariantChainVariant(&v, s->track->v[w->track].data.variant, w->trackerfy);
						if (bound != -1) cycleVariantDown(v, bound);
					} break;
				case MODE_VISUAL: case MODE_VISUALREPLACE:
					cycleDownPartPattern(count, MIN(tfxToVfx(w->trackerfx), w->visualfx), MAX(tfxToVfx(w->trackerfx), w->visualfx), MIN(w->trackerfy, w->visualfy), MAX(w->trackerfy, w->visualfy), MIN(w->track, w->visualtrack), MAX(w->track, w->visualtrack));
					break;
				case MODE_VISUALLINE:
					cycleDownPartPattern(count, 0, 2+s->track->v[w->track].data.variant->macroc, MIN(w->trackerfy, w->visualfy), MAX(w->trackerfy, w->visualfy), MIN(w->track, w->visualtrack), MAX(w->track, w->visualtrack));
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
		case PAGE_VARIANT:
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
		case PAGE_VARIANT:
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
	switch (w->mode)
	{
		case MODE_EFFECT:
			count *= MAX(1, w->count);
			cc.cursor = getCursorFromEffect(s->track->v[w->track].data.effect, MAX(0, getEffectFromCursor(s->track->v[w->track].data.effect, cc.cursor) - (int)count));
			p->redraw = 1; break;
		default:
			trackerUpArrow(s->rowhighlight*count);
			break;
	}
}

void trackerPgDn(size_t count)
{
	switch (w->mode)
	{
		case MODE_EFFECT:
			count *= MAX(1, w->count);
			cc.cursor = getCursorFromEffect(s->track->v[w->track].data.effect, MIN(s->track->v[w->track].data.effect->c-1, getEffectFromCursor(s->track->v[w->track].data.effect, cc.cursor) + count));
			p->redraw = 1; break;
		default:
			trackerDownArrow(s->rowhighlight*count);
			break;
	}
}
