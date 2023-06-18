static void swapFocusPane(void)
{
	switch (w->page)
	{
		case PAGE_VARIANT: w->page = PAGE_PATTERN; break;
		case PAGE_PATTERN: w->page = PAGE_VARIANT; break;
		default: break;
	}
	p->redraw = 1;
}

// static void trackHome (void) { trackSet(0); }
// static void trackEnd  (void) { trackSet(s->track->c-1); }

static void setStep(void *step)
{
	w->step = (size_t)step;
	p->redraw = 1;
}
static void trackerEscape(void)
{
	if (input_api.autorepeaton)
		input_api.autorepeaton();

	previewNote(NOTE_OFF, INST_VOID, 0);
	if (cc.mouseadjust || cc.keyadjust)
	{
		cc.mouseadjust = cc.keyadjust = 0;
		p->redraw = 1;
		return;
	}

	/* walk modes through VISUALREPLACE -> * -> NORMAL */
	switch (w->mode)
	{
		case MODE_VISUALREPLACE: w->mode = MODE_VISUAL; break;
		default:                 w->mode = MODE_NORMAL; break;
	}

	p->redraw = 1;
}
static void trackerEnterVisualMode(void)
{
	switch (w->mode)
	{
		case MODE_VISUAL: w->mode = MODE_NORMAL; break;
		case MODE_VISUALLINE: w->mode = MODE_VISUAL; break;
		default:
			w->visualfx = tfxToVfx(w->trackerfx);
			w->visualfy = w->trackerfy;
			w->visualtrack = w->track;
			w->mode = MODE_VISUAL;
			break;
	}
	p->redraw = 1;
}
static void trackerEnterVisualLineMode(void)
{
	switch (w->mode)
	{
		case MODE_VISUALLINE: w->mode = MODE_NORMAL; break;
		case MODE_VISUAL: w->mode = MODE_VISUALLINE; break;
		default:
			w->visualfx = tfxToVfx(w->trackerfx);
			w->visualfy = w->trackerfy;
			w->visualtrack = w->track;
			w->mode = MODE_VISUALLINE;
			break;
	}
	p->redraw = 1;
}
static void trackerEnterVisualReplaceMode(void)
{
	if (input_api.autorepeatoff)
		input_api.autorepeatoff();

	w->mode = MODE_VISUALREPLACE;
	p->redraw = 1;
}
static void trackerEnterInsertMode(void)
{
	if (input_api.autorepeatoff)
		input_api.autorepeatoff();

	w->mode = MODE_INSERT;
	p->redraw = 1;
}

void toggleSongFollow(void)
{
	w->follow = !w->follow;
	if (w->playing)
		w->trackerfy = w->playfy;
	p->redraw = 1;
}
void togglePatternLoop(void)
{
	w->loop = !w->loop;
	p->redraw = 1;
}

static void muteCurrentTrack(void)
{
	uint8_t i;
	switch (w->mode)
	{
		case MODE_VISUAL: case MODE_VISUALLINE:
			for (i = MIN(w->track, w->visualtrack); i <= MAX(w->track, w->visualtrack); i++)
				toggleTrackMute(i);
			break;
		default:
			toggleTrackMute(w->track);
			break;
	}
	p->redraw = 1;
}
static void soloCurrentTrack(void)
{
	uint8_t i;
	switch (w->mode)
	{
		case MODE_VISUAL: case MODE_VISUALLINE:
			for (i = MIN(w->track, w->visualtrack); i <= MAX(w->track, w->visualtrack); i++)
				toggleTrackSolo(i);
			break;
		default:
			toggleTrackSolo(w->track);
			break;
	}
	p->redraw = 1;
}

static void trackSlideHome(void)
{

	if (!w->track)
	{
		p->redraw = 1;
		return;
	}

	swapTracks(w->track, 0);
	trackerHome();
}
static void trackSlideEnd(void)
{

	if (w->track >= s->track->c-1)
	{
		p->redraw = 1;
		return;
	}

	swapTracks(w->track, s->track->c-1);
	trackerEnd();
}
static void trackSlideLeft(void)
{
	int delta = MAX(1, w->count);
	delta = MIN(delta, w->track);

	if (!delta)
	{
		p->redraw = 1;
		return;
	}

	swapTracks(w->track, w->track - delta);
	trackLeft();
}
static void trackSlideRight(void)
{
	int delta = MAX(1, w->count);
	delta = MIN(delta, s->track->c-1 - w->track);

	if (!delta)
	{
		p->redraw = 1;
		return;
	}

	swapTracks(w->track, w->track + delta);
	trackRight();
}

// static void swapLoopPoint(void)
// {
// 	if (s->loop[1])
// 	{
// 		if      (w->trackerfy == s->loop[0]) w->trackerfy = s->loop[1];
// 		else if (w->trackerfy == s->loop[1]) w->trackerfy = s->loop[0];
// 		else if (w->trackerfy < (s->loop[0] + s->loop[1])>>1) w->trackerfy = s->loop[0];
// 		else                                                  w->trackerfy = s->loop[1];
// 	}
// 	p->redraw = 1;
// }
// static void setLoopPoints(void)
// {
// 	if (s->loop[0] == MIN(w->trackerfy, w->visualfy) && s->loop[1] == MAX(w->trackerfy, w->visualfy))
// 		setLoopRange(0, 0);
// 	else
// 		setLoopRange(MIN(w->trackerfy, w->visualfy), MAX(w->trackerfy, w->visualfy));
//
// 	p->redraw = 1;
// }

static void addEmptyPattern(void)
{
	Track *cv = s->track->v[w->track];
	uint8_t pindex = getPatternChainIndex(w->trackerfy);
	setPatternOrder(&cv->pattern, pindex, -1);
	p->redraw = 1;
}
/* returns true to force disable step */
static bool visualRange(int8_t *minx, int8_t *maxx, short *miny, short *maxy, uint8_t *mintrack, uint8_t *maxtrack)
{
	Track *cv;
	bool ret = 0;

	switch (w->mode)
	{ /* x range */
		case MODE_VISUALLINE:
			cv = s->track->v[MAX(w->track, w->visualtrack)];
			*minx = TRACKERFX_VISUAL_MIN;
			*maxx = 2+cv->pattern->macroc;
			break;
		case MODE_VISUAL:
			if (w->track == w->visualtrack)
			{
				*minx = MIN(tfxToVfx(w->trackerfx), w->visualfx);
				*maxx = MAX(tfxToVfx(w->trackerfx), w->visualfx);
			} else
			{
				*minx = (w->track < w->visualtrack) ? tfxToVfx(w->trackerfx) : w->visualfx;
				*maxx = (w->track > w->visualtrack) ? tfxToVfx(w->trackerfx) : w->visualfx;
			} break;
		default: *minx = *maxx = tfxToVfx(w->trackerfx); break;
	}

	switch (w->mode)
	{ /* y range */
		case MODE_VISUAL:
		case MODE_VISUALLINE:
		case MODE_VISUALREPLACE:
			*miny = MIN(w->trackerfy, w->visualfy);
			*maxy = MAX(w->trackerfy, w->visualfy);
			ret = 1;
			break;
		default: *miny = *maxy = w->trackerfy; break;
	}

	switch (w->mode)
	{ /* track range */
		case MODE_VISUAL:
		case MODE_VISUALLINE:
			*mintrack = MIN(w->track, w->visualtrack);
			*maxtrack = MAX(w->track, w->visualtrack);
			break;
		default: *mintrack = *maxtrack = w->track; break;
	}

	return ret;
}
static void toggleCellCase(void *step)
{
	int8_t  minx,     maxx;
	short   miny,     maxy;
	uint8_t mintrack, maxtrack;
	if (visualRange(&minx, &maxx, &miny, &maxy, &mintrack, &maxtrack))
		step = (void*)0;

	tildePartPattern(minx, maxx, miny, maxy, mintrack, maxtrack);

	if (step)
		trackerDownArrow(w->step);
	p->redraw = 1;
}
static void interpolateCells(void)
{
	int8_t  minx,     maxx;
	short   miny,     maxy;
	uint8_t mintrack, maxtrack;
	visualRange(&minx, &maxx, &miny, &maxy, &mintrack, &maxtrack);

	interpolatePartPattern(minx, maxx, miny, maxy, mintrack, maxtrack);

	p->redraw = 1;
}
static void randomCells(void)
{
	int8_t  minx,     maxx;
	short   miny,     maxy;
	uint8_t mintrack, maxtrack;
	visualRange(&minx, &maxx, &miny, &maxy, &mintrack, &maxtrack);

	randPartPattern(minx, maxx, miny, maxy, mintrack, maxtrack);

	p->redraw = 1;
}
static void trackerAdd(signed char value)
{
	int8_t  minx,     maxx;
	short   miny,     maxy;
	uint8_t mintrack, maxtrack;
	visualRange(&minx, &maxx, &miny, &maxy, &mintrack, &maxtrack);

	switch (w->page)
	{
		case PAGE_PATTERN: addPartPatternOrder(value, miny/(s->plen+1), maxy/(s->plen+1), mintrack, maxtrack); break;
		case PAGE_VARIANT: addPartPattern(value, minx, maxx, miny, maxy, mintrack, maxtrack, 0); break;
		default: break;
	}
	p->redraw = 1;
}

static void trackerInc(void *count) { trackerAdd(+(MAX(1, w->count) * (size_t)count)); }
static void trackerDec(void *count) { trackerAdd(-(MAX(1, w->count) * (size_t)count)); }

static void clearCell(void)
{
	int8_t  minx,     maxx;
	short   miny,     maxy;
	uint8_t mintrack, maxtrack;
	visualRange(&minx, &maxx, &miny, &maxy, &mintrack, &maxtrack);

	switch (w->page)
	{
		case PAGE_PATTERN:
			yankPartPatternOrder(miny/(s->plen+1), maxy/(s->plen+1), mintrack, maxtrack);
			delPartPatternOrder (miny/(s->plen+1), maxy/(s->plen+1), mintrack, maxtrack);
			break;
		case PAGE_VARIANT:
			// addPartPattern(value, minx, maxx, miny, maxy, mintrack, maxtrack, 0);
			if (mintrack == maxtrack && !minx && !maxx) /* edge case to clear both the note and inst columns */
			{
				yankPartPattern(0, 1, miny, maxy, mintrack, maxtrack);
				delPartPattern (0, 1, miny, maxy, mintrack, maxtrack);
			} else
			{
				yankPartPattern(minx, maxx, miny, maxy, mintrack, maxtrack);
				delPartPattern (minx, maxx, miny, maxy, mintrack, maxtrack);
			}
			break;
		default: break;
	}

	switch (w->mode) /* leave the mode if necessary, should be done with an arg */
	{
		case MODE_VISUAL:
		case MODE_VISUALLINE:
			w->mode = MODE_NORMAL;
			break;
		default: break;
	}

	p->redraw = 1;
}
static void yankCell(void)
{
	int8_t  minx,     maxx;
	short   miny,     maxy;
	uint8_t mintrack, maxtrack;
	visualRange(&minx, &maxx, &miny, &maxy, &mintrack, &maxtrack);

	yankPartPattern(minx, maxx, miny, maxy, mintrack, maxtrack);

	w->trackerfx = vfxToTfx(w->visualfx);
	w->track = w->visualtrack;
	w->trackerfy = w->visualfy;
	w->mode = MODE_NORMAL;

	p->redraw = 1;
}

static void bounceRows(void)
{
	int8_t  minx,     maxx;
	short   miny,     maxy;
	uint8_t mintrack, maxtrack;
	visualRange(&minx, &maxx, &miny, &maxy, &mintrack, &maxtrack);

	bouncePartPattern(miny, maxy, mintrack, maxtrack);

	w->trackerfx = 0;
	w->track = mintrack;
	w->trackerfy = miny;
	w->mode = MODE_NORMAL;
	p->redraw = 1;
}

static void pushPatternOrderWrap(void *arg)
{
	Track *cv = s->track->v[w->track];
	short i;
	uint8_t end;
	switch (w->mode)
	{
		case MODE_VISUALREPLACE:
			end = MAX(w->trackerfy, w->visualfy)/(s->plen+1);
			for (i = MIN(w->trackerfy, w->visualfy)/(s->plen+1); i <= end; i++)
				pushPatternOrder(&cv->pattern, i, (size_t)arg);
			break;
		default:
			pushPatternOrder(&cv->pattern, w->trackerfy/(s->plen+1), (size_t)arg);
			break;
	}
	p->redraw = 1;
}
static void setPatternOrderWrap(void *arg)
{
	Track *cv = s->track->v[w->track];
	short i;
	uint8_t end;
	switch (w->mode)
	{
		case MODE_VISUALREPLACE:
			end = MAX(w->trackerfy, w->visualfy)/(s->plen+1);
			for (i = MIN(w->trackerfy, w->visualfy)/(s->plen+1); i <= end; i++)
				setPatternOrder(&cv->pattern, i, (size_t)arg);
			break;
		default:
			setPatternOrder(&cv->pattern, w->trackerfy/(s->plen+1), (size_t)arg);
			break;
	}
	p->redraw = 1;
}

static bool setNote(size_t note, Row **r)
{
	Track *cv = s->track->v[w->track];
	short i;
	bool step;
	switch (w->mode)
	{
		case MODE_VISUALREPLACE:
			for (i = MIN(w->trackerfy, w->visualfy); i <= MAX(w->trackerfy, w->visualfy); i++)
			{
				*r = getTrackRow(cv, i, 1);
				(*r)->note = note;
			} step = 0; break;
		default:
			*r = getTrackRow(cv, w->trackerfy, 1);
			(*r)->note = note;
			(*r)->inst = w->instrument;
			step = 1; break;
	} return step;
}
static void pressNote(size_t note)
{
	Row *r = NULL;
	bool step = setNote(note, &r);
	previewNote(r->note, r->inst, 0);
	if (step)
		trackerDownArrow(w->step);
	p->redraw = 1;
}

static void releaseNote(size_t note)
{
	previewNote(note, 0, 1);
	p->redraw = 1;
}

static void setNoteOctave(size_t octave)
{
	Track *cv = s->track->v[w->track];
	Row *r = NULL;
	short i;
	bool step;
	switch (w->mode)
	{
		case MODE_VISUALREPLACE:
			for (i = MIN(w->trackerfy, w->visualfy); i <= MAX(w->trackerfy, w->visualfy); i++)
			{
				r = getTrackRow(cv, i, 0);
				if (r)
					r->note = octave*12 + fmodf(r->note-NOTE_MIN, 12) + NOTE_MIN;
			} step = 0; break;
		default:
			r = getTrackRow(cv, w->trackerfy, 0);
			if (r)
				r->note = octave*12 + fmodf(r->note-NOTE_MIN, 12) + NOTE_MIN;
			step = 1; break;
	}

	previewRow(r, 0);
	if (step)
		trackerDownArrow(w->step);
	p->redraw = 1;
}

static void imposeInst(void)
{
	Track *cv = s->track->v[w->track];
	short i;
	Row *r;
	switch (w->mode)
	{
		case MODE_VISUALREPLACE:
			for (i = MIN(w->trackerfy, w->visualfy); i <= MAX(w->trackerfy, w->visualfy); i++)
			{
				r = getTrackRow(cv, i, 1);
				r->inst = w->instrument;
			} break;
		default:
			r = getTrackRow(cv, w->trackerfy, 1);
			r->inst = w->instrument;
			break;
	}
	p->redraw = 1;
}

static void pushInst(void *arg)
{
	Track *cv = s->track->v[w->track];
	short i;
	Row *r = NULL;
	switch (w->mode)
	{
		case MODE_VISUALREPLACE:
			for (i = MIN(w->trackerfy, w->visualfy); i <= MAX(w->trackerfy, w->visualfy); i++)
			{
				r = getTrackRow(cv, i, 1);
				if (r->inst == INST_VOID) r->inst++;
				r->inst <<= 4;
				r->inst += (size_t)arg;
			} break;
		default:
			r = getTrackRow(cv, w->trackerfy, 1);
			if (r->inst == INST_VOID) r->inst++;
			r->inst <<= 4;
			r->inst += (size_t)arg;
			break;
	}

	if (r && w->instrecv == INST_REC_LOCK_OK)
		w->instrument = r->inst;
	p->redraw = 1;
}

static void _pushMacrov(Row *r, char nibble)
{
	Macro *m = &r->macro[(w->trackerfx - 2)>>1];
	m->v <<= 4;
	m->t <<= 4;
	switch (nibble)
	{
		/* TODO: check validity before pushing tokens */
		/* TODO: enum or smth for these constants     */
		case '~': m->t += 1; break;
		case '+': m->t += 2; break;
		case '-': m->t += 3; break;
		case '%': m->t += 4; break;

		default: m->v += nibble; break;
	}
}
static void pushMacrov(void *arg)
{
	Track *cv = s->track->v[w->track];
	switch (w->mode)
	{
		case MODE_VISUALREPLACE:
			for (short i = MIN(w->trackerfy, w->visualfy); i <= MAX(w->trackerfy, w->visualfy); i++)
				_pushMacrov(getTrackRow(cv, i, 1), (size_t)arg);
			break;
		default:
			_pushMacrov(getTrackRow(cv, w->trackerfy, 1), (size_t)arg);
			break;
	}
	p->redraw = 1;
}
static void pushMacroc(void *arg)
{
	Track *cv = s->track->v[w->track];
	Row *r;
	short i;
	short macro = (w->trackerfx - 2)>>1;
	switch (w->mode)
	{
		case MODE_VISUALREPLACE:
			for (i = MIN(w->trackerfy, w->visualfy); i <= MAX(w->trackerfy, w->visualfy); i++)
			{
				r = getTrackRow(cv, i, 1);
				r->macro[macro].c = (size_t)arg;
			} break;
		default:
			r = getTrackRow(cv, w->trackerfy, 1);
			r->macro[macro].c = (size_t)arg;
			break;
	}
	p->redraw = 1;
}


static void trackerAdjustLeft(Track *cv) /* mouse adjust only */
{
	Row *r = getTrackRow(cv, w->trackerfy, 1);
	short macro;
	switch (w->page)
	{
		case PAGE_PATTERN:
			if (!w->playing)
			{
				if (w->fieldpointer) setPatternOrder(&cv->pattern, w->trackerfy/(s->plen+1), cv->pattern->order[w->trackerfy/(s->plen+1)]-16);
				else                 setPatternOrder(&cv->pattern, w->trackerfy/(s->plen+1), cv->pattern->order[w->trackerfy/(s->plen+1)]-1);
			} break;
		case PAGE_VARIANT:
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
			} break;
		default: break;
	}
	p->redraw = 1;
}
static void trackerAdjustRight(Track *cv) /* mouse adjust only */
{
	Row *r = getTrackRow(cv, w->trackerfy, 1);
	short macro;
	switch (w->page)
	{
		case PAGE_PATTERN:
			if (!w->playing)
			{
				if (w->fieldpointer) setPatternOrder(&cv->pattern, w->trackerfy/(s->plen+1), cv->pattern->order[w->trackerfy/(s->plen+1)]+16);
				else                 setPatternOrder(&cv->pattern, w->trackerfy/(s->plen+1), cv->pattern->order[w->trackerfy/(s->plen+1)]+1);
			} break;
		case PAGE_VARIANT:
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
			} break;
		default: break;
	}
	p->redraw = 1;
}

static bool trackerMouseHeader(enum Button button, int x, int y, short *tx)
{
	if (y <= TRACK_ROW)
		for (int i = 0; i < s->track->c; i++)
		{
			*tx += getTrackWidth(s->track->v[i]);
			if (*tx > x)
				switch (button)
				{
					case BUTTON1: case BUTTON1_CTRL: w->trackoffset = i - w->track; return 1;
					case BUTTON2: case BUTTON2_CTRL: toggleTrackSolo(i); return 1;
					case BUTTON3: case BUTTON3_CTRL: toggleTrackMute(i); return 1;
					default: return 0;
				}
		}
	return 0;
}

static uint8_t getClickedTrack(short *tx, short x)
{
	short chanw;
	for (int i = 0; i < s->track->c; i++)
	{
		chanw = getTrackWidth(s->track->v[i]);
		if (i == s->track->c - 1 || *tx + chanw > x)
			return i;

		*tx += chanw;
	}
	return 0; /* fallback */
}

static void queueIndex(size_t index)
{
	if (w->queue == index) w->queue = -1;
	else                   w->queue = index;
	p->redraw = 1;
}

static void trackerMouse(enum Button button, int x, int y)
{
	if (rulerMouse(button, x, y)) return;
	p->redraw = 1;

	int i, j;
	short oldtrackerfx = w->trackerfx;
	uint8_t oldtrack = w->track;

	short tx, offset, sfx;

	switch (button)
	{
		case BUTTON2_HOLD: case BUTTON2_HOLD_CTRL:
		case BUTTON3_HOLD: case BUTTON3_HOLD_CTRL:
			break;
		case BUTTON_RELEASE: case BUTTON_RELEASE_CTRL:
			if (w->trackoffset) trackSet(w->track + w->trackoffset);

			if      (w->fyoffset < 0) { w->count = -w->fyoffset; trackerUpArrow  (1); }
			else if (w->fyoffset > 0) { w->count =  w->fyoffset; trackerDownArrow(1); }

			w->count = 0;
			w->fyoffset = w->shiftoffset = w->trackoffset = w->fieldpointer = 0;

			/* leave mouseadjust mode */
			if (w->mode == MODE_MOUSEADJUST) w->mode = w->oldmode;
			/* falls through intentionally */
		default:
			offset = MIN(s->track->c * 3, ws.ws_col>>2)+PATTERN_GUTTER;
			sfx = genSfx(ws.ws_col - ((TRACK_LINENO_COLS<<1) + offset));
			tx = TRACK_LINENO_COLS + 3 + sfx + offset;
			if (trackerMouseHeader(button, x, y, &tx)) break;
			tx = MAX(tx, TRACK_LINENO_COLS + 3 + offset);
			switch (button)
			{
				case BUTTON_RELEASE: case BUTTON_RELEASE_CTRL:
					break;
				case BUTTON1_HOLD: case BUTTON1_HOLD_CTRL:
					w->follow = 0;
					if (w->mode == MODE_MOUSEADJUST)
					{
						if      (x > w->mousex) { trackerAdjustRight(s->track->v[w->track]); }
						else if (x < w->mousex) { trackerAdjustLeft (s->track->v[w->track]); }
					}
					w->mousex = x; break;
				case WHEEL_UP: case WHEEL_UP_CTRL:
					if (x < tx - TRACK_LINENO_COLS - 3)
					{
						if (WHEEL_SPEED*getPatternLength() > w->trackerfy)
							w->trackerfy = 0;
						else
							w->trackerfy -= WHEEL_SPEED*getPatternLength();
					} else
					{
						if (WHEEL_SPEED > w->trackerfy)
							w->trackerfy = 0;
						else
							w->trackerfy -= WHEEL_SPEED;
					}
					w->count = 0; break;
				case WHEEL_DOWN: case WHEEL_DOWN_CTRL:
					if (x < tx - TRACK_LINENO_COLS - 3)
						w->trackerfy = MIN(w->trackerfy + WHEEL_SPEED*getPatternLength(), getPatternLength()*255 - 1);
					else
						w->trackerfy = MIN(w->trackerfy + WHEEL_SPEED, getPatternLength()*255 - 1);
					w->count = 0; break;
				default: /* click */
					if (y > ws.ws_row - 1) break; /* ignore clicking out of range */
					w->follow = 0;

					if (x < tx - TRACK_LINENO_COLS - 3)
					{
						tx = MAX(offset, MAX(sfx, 0) + offset) - TRACK_LINENO_COLS - (offset-PATTERN_GUTTER) + genConstSfx(3, offset-PATTERN_GUTTER) - 1;
						i = ((x - tx) / 3) - 1;
						i = MIN(MAX(i, 0), s->track->c - 1);
						tx += i*3;
						w->page = PAGE_PATTERN;
					} else
					{
						i = getClickedTrack(&tx, x);
						w->page = PAGE_VARIANT;
					}

					switch (button)
					{
						case BUTTON1_CTRL:
							w->step = MIN(15, abs(y - w->centre));
							return;
						case BUTTON1:
							if (w->mode != MODE_INSERT) /* suggest normal mode, but allow insert */
								w->mode = MODE_NORMAL;
							break;
						case BUTTON3:
							if (!(w->mode == MODE_VISUAL || w->mode == MODE_VISUALLINE || w->mode == MODE_VISUALREPLACE))
							{
								w->visualfx = tfxToVfx(oldtrackerfx);
								w->visualfy = w->trackerfy;
								w->visualtrack = oldtrack;
								w->mode = MODE_VISUAL;
							} break;
						case BUTTON2:
							i = w->trackerfy/(s->plen+1) + (y - w->centre);
							if (i >= 0 && i < PATTERN_VOID)
								queueIndex(i);
							return;
						default: break;
					}

					switch (w->page)
					{
						case PAGE_VARIANT:
							if (x-tx < 4) /* note column */
								w->trackerfx = 0;
							else if (x-tx < 7) /* inst column */
							{
								w->trackerfx = 1;
								if (x-tx < 6) w->fieldpointer = 1;
								else                              w->fieldpointer = 0;
							} else if (x-tx > getTrackWidth(s->track->v[i]) - 3) /* star column */
							{
								w->trackerfx = 3;
								w->fieldpointer = 0;
							} else /* macro column */
							{
								j = x-tx - 7;
								if ((j>>1)&0x1) w->trackerfx = 3 + ((s->track->v[i]->pattern->macroc - (j>>2))<<1)+0;
								else            w->trackerfx = 3 + ((s->track->v[i]->pattern->macroc - (j>>2))<<1)-1;
								if (j&0x1) w->fieldpointer = 0;
								else       w->fieldpointer = 1;
							} break;
						case PAGE_PATTERN:
							w->fieldpointer = (x-tx < 4);
							break;
						default: break;
					}

					w->trackoffset = i - w->track;
					if (button == BUTTON3_CTRL) w->shiftoffset = y - w->centre;
					else                        w->fyoffset    = y - w->centre;

					if (button == BUTTON2 || button == BUTTON2_CTRL)
					{
						if (w->trackerfx == 0)
						{
							yankPartPattern(0, 1, w->trackerfy+w->fyoffset, w->trackerfy+w->fyoffset, w->track+w->trackoffset, w->track+w->trackoffset);
							delPartPattern (0, 1, w->trackerfy+w->fyoffset, w->trackerfy+w->fyoffset, w->track+w->trackoffset, w->track+w->trackoffset);
						} else
						{
							yankPartPattern(tfxToVfx(w->trackerfx), tfxToVfx(w->trackerfx), w->trackerfy+w->fyoffset, w->trackerfy+w->fyoffset, w->track+w->trackoffset, w->track+w->trackoffset);
							delPartPattern (tfxToVfx(w->trackerfx), tfxToVfx(w->trackerfx), w->trackerfy+w->fyoffset, w->trackerfy+w->fyoffset, w->track+w->trackoffset, w->track+w->trackoffset);
						} break;
					}

					/* enter adjust */
					if ((button == BUTTON1 || button == BUTTON1_CTRL)
							&& !w->fyoffset && w->trackerfx == oldtrackerfx && !w->trackoffset)
					{
						w->oldmode = w->mode;
						w->mode = MODE_MOUSEADJUST;
						w->mousex = x;
					} break;
			}
			break;
	}
}

static void addTrackerNavBinds(void)
{
	addTooltipBind("cursor up"   , 0, XK_Up       , 0, (void(*)(void*))trackerUpArrow   , (void*)1);
	addTooltipBind("cursor down" , 0, XK_Down     , 0, (void(*)(void*))trackerDownArrow , (void*)1);
	addTooltipBind("cursor left" , 0, XK_Left     , 0, (void(*)(void*))trackerLeftArrow , (void*)1);
	addTooltipBind("cursor right", 0, XK_Right    , 0, (void(*)(void*))trackerRightArrow, (void*)1);
	addTooltipBind("cursor home" , 0, XK_Home     , 0, (void(*)(void*))trackerHome      , NULL    );
	addTooltipBind("cursor end"  , 0, XK_End      , 0, (void(*)(void*))trackerEnd       , NULL    );
	addTooltipBind("cursor pgup" , 0, XK_Page_Up  , 0, (void(*)(void*))trackerPgUp      , NULL    );
	addTooltipBind("cursor pgdn" , 0, XK_Page_Down, 0, (void(*)(void*))trackerPgDn      , NULL    );
}

void initTrackerInput(void)
{
	setTooltipTitle("tracker");
	setTooltipMouseCallback(trackerMouse);
	addDecimalBinds("set step", Mod1Mask, setStep);
	addTooltipBind("return"     , 0          , XK_Escape, 0, (void(*)(void*))trackerEscape, NULL);
	addTooltipBind("track left" , ControlMask, XK_Left  , 0, (void(*)(void*))trackLeft    , NULL);
	addTooltipBind("track right", ControlMask, XK_Right , 0, (void(*)(void*))trackRight   , NULL);
	switch (w->mode)
	{
		case MODE_NORMAL:
			addCountBinds(0);
			addTrackerNavBinds();
			addRulerBinds();
			addTrackerGraphicBinds();
			addTooltipBind("swap focus pane"       , 0          , XK_Tab       , TT_DRAW        , (void(*)(void*))swapFocusPane             , NULL    );
			addTooltipBind("mute current track"    , 0          , XK_Return    , 0              , (void(*)(void*))muteCurrentTrack          , NULL    );
			addTooltipBind("solo current track"    , 0          , XK_s         , 0              , (void(*)(void*))soloCurrentTrack          , NULL    );
			addTooltipBind("inc work octave"       , 0          , XK_plus      , TT_DRAW        , (void(*)(void*))incOctave                 , NULL    );
			addTooltipBind("inc work octave"       , 0          , XK_equal     , 0              , (void(*)(void*))incOctave                 , NULL    );
			addTooltipBind("dec work octave"       , 0          , XK_minus     , TT_DRAW        , (void(*)(void*))decOctave                 , NULL    );
			addTooltipBind("cursor left"           , 0          , XK_h         , 0              , (void(*)(void*))trackerLeftArrow          , (void*)1);
			addTooltipBind("cursor down"           , 0          , XK_j         , 0              , (void(*)(void*))trackerDownArrow          , (void*)1);
			addTooltipBind("cursor up"             , 0          , XK_k         , 0              , (void(*)(void*))trackerUpArrow            , (void*)1);
			addTooltipBind("cursor right"          , 0          , XK_l         , 0              , (void(*)(void*))trackerRightArrow         , (void*)1);
			addTooltipBind("variant cycle up"      , ControlMask, XK_Up        , 0              , (void(*)(void*))cycleUp                   , NULL    );
			addTooltipBind("variant cycle down"    , ControlMask, XK_Down      , 0              , (void(*)(void*))cycleDown                 , NULL    );
			addTooltipBind("slide track home"      , ShiftMask  , XK_Home      , 0              , (void(*)(void*))trackSlideHome            , NULL    );
			addTooltipBind("slide track end"       , ShiftMask  , XK_End       , 0              , (void(*)(void*))trackSlideEnd             , NULL    );
			addTooltipBind("slide track left"      , ShiftMask  , XK_Left      , 0              , (void(*)(void*))trackSlideLeft            , NULL    );
			addTooltipBind("slide track right"     , ShiftMask  , XK_Right     , 0              , (void(*)(void*))trackSlideRight           , NULL    );
			addTooltipBind("variant cycle up"      , ControlMask, XK_K         , 0              , (void(*)(void*))cycleUp                   , NULL    );
			addTooltipBind("variant cycle down"    , ControlMask, XK_J         , 0              , (void(*)(void*))cycleDown                 , NULL    );
			addTooltipBind("track left"            , ControlMask, XK_H         , 0              , (void(*)(void*))trackLeft                 , NULL    );
			addTooltipBind("track right"           , ControlMask, XK_L         , 0              , (void(*)(void*))trackRight                , NULL    );
			addTooltipBind("enter insert mode"     , 0          , XK_i         , 0              , (void(*)(void*))trackerEnterInsertMode    , NULL    );
			addTooltipBind("enter visual mode"     , 0          , XK_v         , 0              , (void(*)(void*))trackerEnterVisualMode    , NULL    );
			addTooltipBind("enter visual mode"     , ControlMask, XK_v         , 0              , (void(*)(void*))trackerEnterVisualMode    , NULL    );
			addTooltipBind("enter visual line mode", 0          , XK_V         , 0              , (void(*)(void*))trackerEnterVisualLineMode, NULL    );
			addTooltipBind("delete"                , 0          , XK_d         , TT_DEAD|TT_DRAW, (void(*)(void*))setChordDeleteRow         , NULL    );
			addTooltipBind("yank"                  , 0          , XK_y         , TT_DEAD|TT_DRAW, (void(*)(void*))setChordYankRow           , NULL    );
			addTooltipBind("track"                 , 0          , XK_c         , TT_DEAD|TT_DRAW, (void(*)(void*))setChordTrack             , NULL    );
			addTooltipBind("macro"                 , 0          , XK_m         , TT_DEAD|TT_DRAW, (void(*)(void*))setChordMacro             , NULL    );
			// addTooltipBind("row"                   , 0          , XK_r         , TT_DEAD|TT_DRAW, (void(*)(void*))setChordRow               , NULL    );
			// addTooltipBind("loop"                  , 0          , XK_semicolon , TT_DEAD|TT_DRAW, (void(*)(void*))setChordLoop              , NULL    );
			addTooltipBind("put"                   , 0          , XK_p         , TT_DRAW        , (void(*)(void*))putPartPattern            , (void*)1);
			addTooltipBind("mix put"               , 0          , XK_P         , 0              , (void(*)(void*))mixPutPartPattern         , (void*)0);
			addTooltipBind("toggle song follow"    , 0          , XK_f         , TT_DRAW        , (void(*)(void*))toggleSongFollow          , NULL    );
			addTooltipBind("toggle pattern loop"   , 0          , XK_semicolon , TT_DRAW        , (void(*)(void*))togglePatternLoop         , NULL    );
			addTooltipBind("clear cell"            , 0          , XK_x         , TT_DRAW        , (void(*)(void*))clearCell                 , NULL    );
			addTooltipBind("clear cell"            , 0          , XK_BackSpace , 0              , (void(*)(void*))clearCell                 , NULL    );
			addTooltipBind("increment cell"        , ControlMask, XK_A         , TT_DRAW        , trackerInc                                , (void*)1);
			addTooltipBind("decrement cell"        , ControlMask, XK_X         , TT_DRAW        , trackerDec                                , (void*)1);
			switch (w->page)
			{
				case PAGE_VARIANT:
					addTooltipBind("toggle cell case"      , 0                   , XK_asciitilde, TT_DRAW, toggleCellCase, (void*)1 );
					addTooltipBind("octave increment cell" , ControlMask|Mod1Mask, XK_A         , TT_DRAW, trackerInc    , (void*)12);
					addTooltipBind("octave decrement cell" , ControlMask|Mod1Mask, XK_X         , TT_DRAW, trackerDec    , (void*)12);
					break;
				case PAGE_PATTERN:
					addTooltipBind("add empty pattern"     , 0, XK_a, TT_DRAW, (void(*)(void*))addEmptyPattern, NULL                                     );
					addTooltipBind("queue index"           , 0, XK_q, TT_DRAW, (void(*)(void*))queueIndex     , (void*)(size_t)(w->trackerfy/(s->plen+1)));
					break;
				default: break;
			} break;
		case MODE_INSERT:
		case MODE_VISUALREPLACE:
			addTrackerNavBinds();
			addTooltipBind("mute current track"   , 0                   , XK_Return   , 0      , (void(*)(void*))muteCurrentTrack, NULL     );
			addTooltipBind("increment cell"       , ControlMask         , XK_A        , TT_DRAW, trackerInc                      , (void*)1 );
			addTooltipBind("decrement cell"       , ControlMask         , XK_X        , TT_DRAW, trackerDec                      , (void*)1 );
			addTooltipBind("octave increment cell", ControlMask|Mod1Mask, XK_A        , TT_DRAW, trackerInc                      , (void*)12);
			addTooltipBind("octave decrement cell", ControlMask|Mod1Mask, XK_X        , TT_DRAW, trackerDec                      , (void*)12);
			addTooltipBind("clear cell"           , 0                   , XK_BackSpace, 0      , (void(*)(void*))clearCell       , NULL     );
			switch (w->page)
			{
				case PAGE_VARIANT:
					switch (w->trackerfx)
					{
						case 0: /* note */
							addNotePressBinds("push cell", 0, w->octave, (void(*)(void*))pressNote);
							addNoteReleaseBinds("release preview", 0, w->octave, (void(*)(void*))releaseNote);
							addDecimalBinds("set octave", 0, (void(*)(void*))setNoteOctave);
							break;
						case 1: /* inst */
							addHexBinds("push cell", 0, pushInst);
							addTooltipBind("impose state on cell", 0, XK_space, 0, (void(*)(void*))imposeInst, NULL);
							break;
						default: /* macro */
							if (!(w->trackerfx&1)) /* macroc */
								addMacroBinds("push cell", 0, pushMacroc);
							else                   /* macrov */
							{
								addHexBinds("push cell", 0, pushMacrov);
								addTooltipBind("push lfo token", 0, XK_asciitilde, 0, pushMacrov, (void*)'~');
								addTooltipBind("push inc token", 0, XK_plus      , 0, pushMacrov, (void*)'+');
								addTooltipBind("push dec token", 0, XK_minus     , 0, pushMacrov, (void*)'-');
								addTooltipBind("push rng token", 0, XK_percent   , 0, pushMacrov, (void*)'%');
							} break;
					} break;
				case PAGE_PATTERN:
					addHexBinds("push cell", 0, pushPatternOrderWrap);
					addTooltipBind("stop cell", 0, XK_space, 0, setPatternOrderWrap, (void*)PATTERN_VOID);
					break;
				default: break;
			} break;
		case MODE_VISUAL:
		case MODE_VISUALLINE:
			addTrackerNavBinds();
			addTooltipBind("mute current track"   , 0                   , XK_Return      , 0      , (void(*)(void*))muteCurrentTrack             , NULL     );
			addTooltipBind("solo current track"   , 0                   , XK_s           , 0      , (void(*)(void*))soloCurrentTrack             , NULL     );
			addCountBinds(0);
			addTooltipBind("cursor left"          , 0                   , XK_h           , 0      , (void(*)(void*))trackerLeftArrow             , (void*)1 );
			addTooltipBind("cursor down"          , 0                   , XK_j           , 0      , (void(*)(void*))trackerDownArrow             , (void*)1 );
			addTooltipBind("cursor up"            , 0                   , XK_k           , 0      , (void(*)(void*))trackerUpArrow               , (void*)1 );
			addTooltipBind("cursor right"         , 0                   , XK_l           , 0      , (void(*)(void*))trackerRightArrow            , (void*)1 );
			addTooltipBind("track left"           , 0                   , XK_bracketleft , 0      , (void(*)(void*))trackLeft                    , NULL);
			addTooltipBind("track right"          , 0                   , XK_bracketright, 0      , (void(*)(void*))trackRight                   , NULL);
			addTooltipBind("cell-wise"            , 0                   , XK_v           , 0      , (void(*)(void*))trackerEnterVisualMode       , NULL     );
			addTooltipBind("cell-wise"            , ControlMask         , XK_V           , 0      , (void(*)(void*))trackerEnterVisualMode       , NULL     );
			addTooltipBind("line-wise"            , 0                   , XK_V           , 0      , (void(*)(void*))trackerEnterVisualLineMode   , NULL     );
			addTooltipBind("replace"              , 0                   , XK_r           , 0      , (void(*)(void*))trackerEnterVisualReplaceMode, NULL     );
			addTooltipBind("toggle cell case"     , 0                   , XK_asciitilde  , TT_DRAW, toggleCellCase                               , (void*)0 );
			addTooltipBind("interpolate cells"    , 0                   , XK_i           , TT_DRAW, (void(*)(void*))interpolateCells             , NULL     );
			addTooltipBind("randomize cells"      , 0                   , XK_percent     , TT_DRAW, (void(*)(void*))randomCells                  , NULL     );
			addTooltipBind("increment cell"       , ControlMask         , XK_A           , TT_DRAW, trackerInc                                   , (void*)1 );
			addTooltipBind("decrement cell"       , ControlMask         , XK_X           , TT_DRAW, trackerDec                                   , (void*)1 );
			addTooltipBind("octave increment cell", ControlMask|Mod1Mask, XK_A           , TT_DRAW, trackerInc                                   , (void*)12);
			addTooltipBind("octave decrement cell", ControlMask|Mod1Mask, XK_X           , TT_DRAW, trackerDec                                   , (void*)12);
			addTooltipBind("delete"               , 0                   , XK_x           , 0      , (void(*)(void*))clearCell                    , NULL     );
			addTooltipBind("delete"               , 0                   , XK_d           , 0      , (void(*)(void*))clearCell                    , NULL     );
			addTooltipBind("delete"               , 0                   , XK_BackSpace   , 0      , (void(*)(void*))clearCell                    , NULL     );
			addTooltipBind("yank"                 , 0                   , XK_y           , 0      , (void(*)(void*))yankCell                     , NULL     );
			addTooltipBind("bounce rows to sample", 0                   , XK_b           , TT_DRAW, (void(*)(void*))bounceRows                   , NULL     );
			// addTooltipBind("jump loop points"     , 0                   , XK_percent     , 0      , (void(*)(void*))swapLoopPoint                , NULL     );
			// addTooltipBind("set loop range"       , 0                   , XK_semicolon   , TT_DRAW, (void(*)(void*))setLoopPoints                , NULL     );
			break;
		default: break;
	}
}
