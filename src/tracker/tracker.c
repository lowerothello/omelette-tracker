#include "visual.h"
#include "draw.c"

#include "variant.c"
#include "pattern.c"
#include "track.c"

// #include "chord/row.c"
#include "chord/track.c"
#include "chord/macro.c"
#include "chord/loop.c"
#include "chord/yank.c"
#include "chord/delete.c"
#include "chord/graphic.c"

#include "input.c"

uint8_t changeNoteOctave(uint8_t octave, uint8_t note)
{
	w->octave = octave;
	if (note == NOTE_VOID) return NOTE_VOID;

	octave *= 12;
	while (note > octave + 11) note -= 12;
	while (note < octave + 0)  note += 12;
	return note;
}

void trackerUpArrow(size_t count)
{
	count *= MAX(1, w->count);
	size_t min;
	int mincount;
	switch (w->mode)
	{
		case MODE_EFFECT:
			min = getCursorFromEffectTrack(w->track);
			mincount = cc.cursor - min;
			decControlCursor(MIN(mincount, count));
			break;
		case MODE_SETTINGS:
			min = w->track*SETTINGS_CONTROLS;
			mincount = cc.cursor - min;
			decControlCursor(MIN(mincount, count));
			break;
		default:
			w->follow = 0;
			if (w->trackerfx >= 0)
			{
				if (count > w->trackerfy)
					w->trackerfy = 0;
				else
					w->trackerfy -= count;
			} else
			{
				if (count*getPatternLength() > w->trackerfy)
					w->trackerfy = 0;
				else
					w->trackerfy -= count*getPatternLength();
			}
			break;
	}
	p->redraw = 1;
}

void trackerDownArrow(size_t count)
{
	count *= MAX(1, w->count);
	size_t max;
	int maxcount;
	switch (w->mode)
	{
		case MODE_EFFECT:
			max = getCursorFromEffectTrack(w->track + 1) - 1;
			maxcount = max - cc.cursor;
			incControlCursor(MIN(maxcount, count));
			break;
		case MODE_SETTINGS:
			max = (w->track+1)*SETTINGS_CONTROLS - 1;
			maxcount = max - cc.cursor;
			incControlCursor(MIN(maxcount, count));
			break;
		default:
			w->follow = 0;
			if (w->trackerfx >= 0)
				w->trackerfy = MIN(w->trackerfy + count, getPatternLength()*255 - 1);
			else
				w->trackerfy = MIN(w->trackerfy + count*getPatternLength(), getPatternLength()*255 - 1);
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
		case MODE_SETTINGS:
			for (i = 0; i < count; i++)
				incControlFieldpointer();
			break;
		default:
			for (i = 0; i < count; i++)
			{
				if      (w->trackerfx == 2 + (s->track->v[w->track]->pattern->macroc<<1)) w->trackerfx = 1;
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

void trackerRightArrow(size_t count)
{
	int i;
	count *= MAX(1, w->count);
	switch (w->mode)
	{
		case MODE_EFFECT:
		case MODE_SETTINGS:
			for (i = 0; i < count; i++)
				decControlFieldpointer();
			break;
		default:
			for (i = 0; i < count; i++)
			{
				if      (w->trackerfx == 1) w->trackerfx = 2 + s->track->v[w->track]->pattern->macroc * 2;
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

void trackerHome(void)
{
	switch (w->mode)
	{
		case MODE_EFFECT:
			cc.cursor = getCursorFromEffectTrack(w->track);
			break;
		case MODE_SETTINGS:
			cc.cursor = w->track*SETTINGS_CONTROLS;
			break;
		default:
			w->follow = 0;
			if (w->trackerfx >= 0)
				w->trackerfy = getPatternChainIndex(w->trackerfy) * getPatternLength();
			else
				w->trackerfy = getPatternIndex(w->trackerfy);
			break;
	} p->redraw = 1;
}

void trackerEnd(void)
{
	switch (w->mode)
	{
		case MODE_EFFECT:
			cc.cursor = getCursorFromEffectTrack(w->track + 1) - 1;
			break;
		case MODE_SETTINGS:
			cc.cursor = (w->track+1)*SETTINGS_CONTROLS - 1;
			break;
		default:
			w->follow = 0;
			if (w->trackerfx >= 0)
				w->trackerfy = (getPatternChainIndex(w->trackerfy)+1) * getPatternLength() - 1;
			else
				w->trackerfy = 255 * getPatternLength() + getPatternIndex(w->trackerfy);
			break;
	} p->redraw = 1;
}

void cycleUp(void)
{
	size_t count = MAX(1, w->count);
	switch (w->page)
	{
		case PAGE_VARIANT:
			switch (w->mode)
			{
				case MODE_NORMAL: case MODE_INSERT:
					/* TODO: pattern order cycling */
					if (w->trackerfx < 0) break;

					cycleUpPartPattern(count, 0, 2+s->track->v[w->track]->pattern->macroc, w->trackerfy, (getPatternChainIndex(w->trackerfy)+1) * getPatternLength() - 1, w->track, w->track);
					break;
				case MODE_VISUAL: case MODE_VISUALREPLACE:
					cycleUpPartPattern(count, MIN(tfxToVfx(w->trackerfx), w->visualfx), MAX(tfxToVfx(w->trackerfx), w->visualfx), MIN(w->trackerfy, w->visualfy), MAX(w->trackerfy, w->visualfy), MIN(w->track, w->visualtrack), MAX(w->track, w->visualtrack));
					break;
				case MODE_VISUALLINE:
					cycleUpPartPattern(count, 0, 2+s->track->v[w->track]->pattern->macroc, MIN(w->trackerfy, w->visualfy), MAX(w->trackerfy, w->visualfy), MIN(w->track, w->visualtrack), MAX(w->track, w->visualtrack));
					break;
				default: break;
			} break;
		default: break;
	} p->redraw = 1;
}

void cycleDown(void)
{
	size_t count = MAX(1, w->count);
	switch (w->page)
	{
		case PAGE_VARIANT:
			switch (w->mode)
			{
				/* TODO: variant trig mode and variant trig visual mode handling */
				case MODE_NORMAL: case MODE_INSERT:
					/* TODO: pattern order cycling */
					if (w->trackerfx < 0) break;

					cycleDownPartPattern(count, 0, 2+s->track->v[w->track]->pattern->macroc, w->trackerfy, (getPatternChainIndex(w->trackerfy)+1) * getPatternLength() - 1, w->track, w->track);
				case MODE_VISUAL: case MODE_VISUALREPLACE:
					cycleDownPartPattern(count, MIN(tfxToVfx(w->trackerfx), w->visualfx), MAX(tfxToVfx(w->trackerfx), w->visualfx), MIN(w->trackerfy, w->visualfy), MAX(w->trackerfy, w->visualfy), MIN(w->track, w->visualtrack), MAX(w->track, w->visualtrack));
					break;
				case MODE_VISUALLINE:
					cycleDownPartPattern(count, 0, 2+s->track->v[w->track]->pattern->macroc, MIN(w->trackerfy, w->visualfy), MAX(w->trackerfy, w->visualfy), MIN(w->track, w->visualtrack), MAX(w->track, w->visualtrack));
					break;
				default: break;
			} break;
		default: break;
	} p->redraw = 1;
}

void trackerPgUp(void)
{
	switch (w->mode)
	{
		case MODE_EFFECT:
			cc.cursor = getCursorFromEffect(w->track, s->track->v[w->track]->effect,
					MAX(0, getEffectFromCursor(w->track, s->track->v[w->track]->effect, cc.cursor) - (int)MAX(1, w->count)));
			p->redraw = 1; break;
		default:
			trackerUpArrow(s->rowhighlight);
			break;
	}
}

void trackerPgDn(void)
{
	switch (w->mode)
	{
		case MODE_EFFECT:
			cc.cursor = getCursorFromEffect(w->track, s->track->v[w->track]->effect,
					MIN(s->track->v[w->track]->effect->c-1, getEffectFromCursor(w->track, s->track->v[w->track]->effect, cc.cursor) + MAX(1, w->count)));
			p->redraw = 1; break;
		default:
			trackerDownArrow(s->rowhighlight);
			break;
	}
}
