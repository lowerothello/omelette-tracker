#include "visual.h"
#include "draw.c"

#include "pattern.c"
#include "track.c"

// #include "chord/row.c"
#include "chord/track.c"
#include "chord/macro.c"
// #include "chord/loop.c"
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

	w->follow = 0;
	switch (w->page)
	{
		case PAGE_VARIANT:
			if (count > w->trackerfy)
				w->trackerfy = 0;
			else
				w->trackerfy -= count;
			break;
		case PAGE_PATTERN:
			if (count*getPatternLength() > w->trackerfy)
				w->trackerfy = 0;
			else
				w->trackerfy -= count*getPatternLength();
			break;
		default: break;
	}
	p->redraw = 1;
}

void trackerDownArrow(size_t count)
{
	count *= MAX(1, w->count);

	w->follow = 0;
	switch (w->page)
	{
		case PAGE_VARIANT:
			w->trackerfy = MIN(w->trackerfy + count, getPatternLength()*255 - 1);
			break;
		case PAGE_PATTERN:
			w->trackerfy = MIN(w->trackerfy + count*getPatternLength(), getPatternLength()*255 - 1);
			break;
		default: break;
	}
	p->redraw = 1;
}

void trackSet(uint8_t track)
{
	w->track = track;
	Track *cv = s->track->v[w->track];
	if (w->trackerfx > 3 + cv->pattern->macroc * 2)
		w->trackerfx = 3 + cv->pattern->macroc * 2;

	switch (w->page)
	{
		case PAGE_EFFECT:
			if (!(cc.cursor > getCursorFromEffectTrack(w->track) && cc.cursor < getCursorFromEffectTrack(w->track + 1)))
				cc.cursor = getCursorFromEffectTrack(w->track);
			break;
		default: break;
	}

	p->redraw = 1;
}

void trackLeft (void) { trackSet(MAX((int)w->track - MAX(1, w->count), 0)); }
void trackRight(void) { trackSet(MIN(w->track + MAX(1, w->count), s->track->c-1)); }

void trackerLeftArrow(size_t count)
{
	int i;
	count *= MAX(1, w->count);
	uint8_t macroc = s->track->v[w->track]->pattern->macroc;
	switch (w->page)
	{
		case PAGE_VARIANT:
			for (i = 0; i < count; i++)
			{
				if      (w->trackerfx == 2 + (macroc<<1)) w->trackerfx = 1;
				else if (!w->trackerfx)
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
		case PAGE_PATTERN:
			for (i = 0; i < count; i++)
				trackLeft();
			break;
		default: break;
	} p->redraw = 1;
}

void trackerRightArrow(size_t count)
{
	int i;
	count *= MAX(1, w->count);
	uint8_t macroc = s->track->v[w->track]->pattern->macroc;
	switch (w->page)
	{
		case PAGE_VARIANT:
			for (i = 0; i < count; i++)
			{
				if      (w->trackerfx == 1) w->trackerfx = 2 + macroc * 2;
				else if (w->trackerfx == 3)
				{
					if (w->track < s->track->c-1)
					{
						w->track++;
						w->trackerfx = 0;
					} else w->trackerfx = 3;
				} else if (w->trackerfx > 1)
				{
					if (w->trackerfx&0x1) w->trackerfx-=3;
					else                  w->trackerfx++;
				} else w->trackerfx++;
			} break;
		case PAGE_PATTERN:
			for (i = 0; i < count; i++)
				trackRight();
			break;
		default: break;
	} p->redraw = 1;
}

void trackerHome(void)
{
	w->follow = 0;
	switch (w->page)
	{
		case PAGE_VARIANT: w->trackerfy = getPatternChainIndex(w->trackerfy) * getPatternLength(); break;
		case PAGE_PATTERN: w->trackerfy = getPatternIndex(w->trackerfy); break;
		default: break;
	} p->redraw = 1;
}

void trackerEnd(void)
{
	w->follow = 0;
	switch (w->page)
	{
		case PAGE_VARIANT: w->trackerfy = (getPatternChainIndex(w->trackerfy)+1) * getPatternLength() - 1; break;
		case PAGE_PATTERN: w->trackerfy = 255 * getPatternLength() + getPatternIndex(w->trackerfy); break;
		default: break;
	} p->redraw = 1;
}

void cycleUp(void)
{
	size_t count = MAX(1, w->count);
	uint8_t macroc = s->track->v[w->track]->pattern->macroc;
	switch (w->page)
	{
		case PAGE_VARIANT:
			switch (w->mode)
			{
				case MODE_NORMAL: case MODE_INSERT:
					cycleUpPartPattern(count, 0, 2+macroc, w->trackerfy, (getPatternChainIndex(w->trackerfy)+1) * getPatternLength() - 1, w->track, w->track);
					break;
				case MODE_VISUAL: case MODE_VISUALREPLACE:
					cycleUpPartPattern(count, MIN(tfxToVfx(w->trackerfx), w->visualfx), MAX(tfxToVfx(w->trackerfx), w->visualfx), MIN(w->trackerfy, w->visualfy), MAX(w->trackerfy, w->visualfy), MIN(w->track, w->visualtrack), MAX(w->track, w->visualtrack));
					break;
				case MODE_VISUALLINE:
					cycleUpPartPattern(count, 0, 2+macroc, MIN(w->trackerfy, w->visualfy), MAX(w->trackerfy, w->visualfy), MIN(w->track, w->visualtrack), MAX(w->track, w->visualtrack));
					break;
				default: break;
			} break;
		case PAGE_PATTERN:
			/* TODO: */
			break;
		default: break;
	} p->redraw = 1;
}

void cycleDown(void)
{
	size_t count = MAX(1, w->count);
	uint8_t macroc = s->track->v[w->track]->pattern->macroc;
	switch (w->page)
	{
		case PAGE_VARIANT:
			switch (w->mode)
			{
				/* TODO: variant trig mode and variant trig visual mode handling */
				case MODE_NORMAL: case MODE_INSERT:
					cycleDownPartPattern(count, 0, 2+macroc, w->trackerfy, (getPatternChainIndex(w->trackerfy)+1) * getPatternLength() - 1, w->track, w->track);
					break;
				case MODE_VISUAL: case MODE_VISUALREPLACE:
					cycleDownPartPattern(count, MIN(tfxToVfx(w->trackerfx), w->visualfx), MAX(tfxToVfx(w->trackerfx), w->visualfx), MIN(w->trackerfy, w->visualfy), MAX(w->trackerfy, w->visualfy), MIN(w->track, w->visualtrack), MAX(w->track, w->visualtrack));
					break;
				case MODE_VISUALLINE:
					cycleDownPartPattern(count, 0, 2+macroc, MIN(w->trackerfy, w->visualfy), MAX(w->trackerfy, w->visualfy), MIN(w->track, w->visualtrack), MAX(w->track, w->visualtrack));
					break;
				default: break;
			} break;
		case PAGE_PATTERN:
			/* TODO: */
			break;
		default: break;
	} p->redraw = 1;
}

void trackerPgUp(void)
{
	trackerUpArrow(s->rowhighlight);
}

void trackerPgDn(void)
{
	trackerDownArrow(s->rowhighlight);
}
