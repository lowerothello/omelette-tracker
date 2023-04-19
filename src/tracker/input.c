static void trackSet(uint8_t track)
{
	w->track = track;
	if (w->trackerfx > 3 + s->track->v[w->track]->variant->macroc * 2)
		w->trackerfx = 3 + s->track->v[w->track]->variant->macroc * 2;

	if (w->mode == MODE_EFFECT)
		if (!(cc.cursor > getCursorFromEffectTrack(w->track) && cc.cursor < getCursorFromEffectTrack(w->track + 1))) /* (w->track + 1) is always safe here */
			cc.cursor = getCursorFromEffectTrack(w->track);

	p->redraw = 1;
}

static void trackLeft (void) { trackSet(MAX((int)w->track - MAX(1, w->count), 0)); }
static void trackRight(void) { trackSet(MIN(w->track + MAX(1, w->count), s->track->c-1)); }
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
static void trackerEnterEffectMode(void)
{
	w->mode = MODE_EFFECT;
	cc.cursor = getCursorFromEffectTrack(w->track);
	p->redraw = 1;
}
void toggleSongFollow(void)
{
	w->follow = !w->follow;
	if (w->playing)
		w->trackerfy = w->playfy;
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

static void swapLoopPoint(void)
{
	if (s->loop[1])
	{
		if      (w->trackerfy == s->loop[0]) w->trackerfy = s->loop[1];
		else if (w->trackerfy == s->loop[1]) w->trackerfy = s->loop[0];
		else if (w->trackerfy < (s->loop[0] + s->loop[1])>>1) w->trackerfy = s->loop[0];
		else                                                  w->trackerfy = s->loop[1];
	}
	p->redraw = 1;
}
static void setLoopPoints(void)
{
	if (s->loop[0] == MIN(w->trackerfy, w->visualfy) && s->loop[1] == MAX(w->trackerfy, w->visualfy))
		setLoopRange(0, 0);
	else
		setLoopRange(MIN(w->trackerfy, w->visualfy), MAX(w->trackerfy, w->visualfy));

	regenGlobalRowc(s);
	p->redraw = 1;
}

static void addEmptyVariant(void)
{
	Track *cv = s->track->v[w->track];
	setVariantChainTrig(&cv->variant, w->trackerfy, dupEmptyVariantIndex(cv->variant, cv->variant->trig[w->trackerfy].index));
	regenGlobalRowc(s); p->redraw = 1;
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
			*maxx = 2+cv->variant->macroc;
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

	regenGlobalRowc(s);
	if (step) trackerDownArrow(w->step);
	p->redraw = 1;
}
static void interpolateCells(void)
{
	int8_t  minx,     maxx;
	short   miny,     maxy;
	uint8_t mintrack, maxtrack;
	visualRange(&minx, &maxx, &miny, &maxy, &mintrack, &maxtrack);

	interpolatePartPattern(minx, maxx, miny, maxy, mintrack, maxtrack);

	regenGlobalRowc(s); p->redraw = 1;
}
static void randomCells(void)
{
	int8_t  minx,     maxx;
	short   miny,     maxy;
	uint8_t mintrack, maxtrack;
	visualRange(&minx, &maxx, &miny, &maxy, &mintrack, &maxtrack);

	randPartPattern(minx, maxx, miny, maxy, mintrack, maxtrack);

	regenGlobalRowc(s); p->redraw = 1;
}
static void trackerAdd(signed char value)
{
	int8_t  minx,     maxx;
	short   miny,     maxy;
	uint8_t mintrack, maxtrack;
	visualRange(&minx, &maxx, &miny, &maxy, &mintrack, &maxtrack);

	switch (w->mode)
	{
		case MODE_VISUALLINE: addPartPattern(value, minx, maxx, miny, maxy, mintrack, maxtrack, 0, 0); break;
		default:              addPartPattern(value, minx, maxx, miny, maxy, mintrack, maxtrack, 0, 1); break;
	}

	regenGlobalRowc(s); p->redraw = 1;
}

static void trackerInc(void *count) { trackerAdd(+(MAX(1, w->count) * (size_t)count)); }
static void trackerDec(void *count) { trackerAdd(-(MAX(1, w->count) * (size_t)count)); }

static void clearCell(void)
{
	int8_t  minx,     maxx;
	short   miny,     maxy;
	uint8_t mintrack, maxtrack;
	visualRange(&minx, &maxx, &miny, &maxy, &mintrack, &maxtrack);

	if (mintrack == maxtrack && !minx && !maxx) /* edge case to clear both the note and inst columns */
	{
		yankPartPattern(0, 1, miny, maxy, mintrack, maxtrack);
		delPartPattern (0, 1, miny, maxy, mintrack, maxtrack);
	} else
	{
		yankPartPattern(minx, maxx, miny, maxy, mintrack, maxtrack);
		delPartPattern (minx, maxx, miny, maxy, mintrack, maxtrack);
	}

	regenGlobalRowc(s);

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

	regenGlobalRowc(s);

	w->trackerfx = vfxToTfx(w->visualfx);
	w->track = w->visualtrack;
	w->trackerfy = w->visualfy;
	w->mode = MODE_NORMAL;

	p->redraw = 1;
}

static void toggleVariantLoop(void)
{
	Track *cv = s->track->v[w->track];
	int i = getVariantChainPrevVtrig(cv->variant, w->trackerfy);
	if (i != -1) cv->variant->trig[i].flags ^= C_VTRIG_LOOP;
	regenGlobalRowc(s); p->redraw = 1;
}
static void toggleVisualVariantLoop(void)
{
	int8_t  minx,     maxx;
	short   miny,     maxy;
	uint8_t mintrack, maxtrack;
	visualRange(&minx, &maxx, &miny, &maxy, &mintrack, &maxtrack);

	loopPartPattern(miny, maxy, mintrack, maxtrack);

	regenGlobalRowc(s); p->redraw = 1;
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
	regenGlobalRowc(s); p->redraw = 1;
}

static void ripRows(void)
{
	Track *cv = s->track->v[w->track];
	Row *r;
	uint8_t vindex = 0;
	short vminrow = MIN(w->trackerfy, w->visualfy);
	/* loop to find the lowest common free index */
	for (int i = MIN(w->track, w->visualtrack); i <= MAX(w->track, w->visualtrack); i++)
		vindex = MAX(vindex, getEmptyVariantIndex(cv->variant, vindex));

	/* loop to actually rip */
	uint8_t vlength;
	for (int i = MIN(w->track, w->visualtrack); i <= MAX(w->track, w->visualtrack); i++)
	{
		vlength = MIN(VARIANT_ROWMAX, MAX(w->trackerfy, w->visualfy) - vminrow);
		cv = s->track->v[i];

		addVariant(&cv->variant, vindex, vlength);

		/* move rows to the new variant */
		for (int j = 0; j <= vlength; j++)
		{
			r = getTrackRow(cv, vminrow + j);
			memcpy(&cv->variant->v[cv->variant->i[vindex]]->rowv[j], r, sizeof(Row));

			/* only clear the source row if it's in the global variant */
			if (getVariantChainVariant(NULL, cv->variant, vminrow + j) == -1)
			{
				memset(r, 0, sizeof(Row));
				r->note = NOTE_VOID;
				r->inst = INST_VOID;
			}
		}

		/* unnecessarily complex edge case handling */
		if (cv->variant->trig[vminrow + vlength].index == VARIANT_VOID)
			for (int j = vminrow + vlength; j > vminrow; j--)
				if (cv->variant->trig[j].index != VARIANT_VOID)
				{
					if (cv->variant->i[cv->variant->trig[j].index] != VARIANT_VOID && j + cv->variant->v[cv->variant->i[cv->variant->trig[j].index]]->rowc > vminrow + vlength)
					{
						cv->variant->trig[vminrow + vlength].index = cv->variant->trig[j].index;
						// TODO: set vtrig offset to align this correctly
					} break;
				}

		setVariantChainTrig(&cv->variant, vminrow, vindex);
	} regenGlobalRowc(s); p->redraw = 1;
}

static void pushVtrig(void *arg)
{
	Track *cv = s->track->v[w->track];
	short i;
	switch (w->mode)
	{
		case MODE_VISUALREPLACE:
			for (i = MIN(w->trackerfy, w->visualfy); i <= MAX(w->trackerfy, w->visualfy); i++)
				inputVariantChainTrig(&cv->variant, i, (size_t)arg);
			break;
		default:
			inputVariantChainTrig(&cv->variant, w->trackerfy, (size_t)arg);
			break;
	} regenGlobalRowc(s); p->redraw = 1;
}
static void setVtrig(void *arg)
{
	Track *cv = s->track->v[w->track];
	short i;
	switch (w->mode)
	{
		case MODE_VISUALREPLACE:
			for (i = MIN(w->trackerfy, w->visualfy); i <= MAX(w->trackerfy, w->visualfy); i++)
				setVariantChainTrig(&cv->variant, i, (size_t)arg);
			break;
		default:
			setVariantChainTrig(&cv->variant, w->trackerfy, (size_t)arg);
			break;
	} regenGlobalRowc(s); p->redraw = 1;
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
				*r = getTrackRow(cv, i);
				(*r)->note = note;
			} step = 0; break;
		default:
			*r = getTrackRow(cv, w->trackerfy);
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
	regenGlobalRowc(s);
	if (step) trackerDownArrow(w->step);
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
				r = getTrackRow(cv, i);
				r->note = octave*12 + fmodf(r->note-NOTE_MIN, 12) + NOTE_MIN;
			} step = 0; break;
		default:
			r = getTrackRow(cv, w->trackerfy);
			r->note = octave*12 + fmodf(r->note-NOTE_MIN, 12) + NOTE_MIN;
			step = 1; break;
	}

	previewRow(r, 0);
	regenGlobalRowc(s);
	if (step) trackerDownArrow(w->step);
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
				r = getTrackRow(cv, i);
				r->inst = w->instrument;
			} break;
		default:
			r = getTrackRow(cv, w->trackerfy);
			r->inst = w->instrument;
			break;
	} regenGlobalRowc(s); p->redraw = 1;
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
				r = getTrackRow(cv, i);
				if (r->inst == INST_VOID) r->inst++;
				r->inst <<= 4;
				r->inst += (size_t)arg;
			} break;
		default:
			r = getTrackRow(cv, w->trackerfy);
			if (r->inst == INST_VOID) r->inst++;
			r->inst <<= 4;
			r->inst += (size_t)arg;
			break;
	}

	if (r && w->instrecv == INST_REC_LOCK_OK) w->instrument = r->inst;
	regenGlobalRowc(s); p->redraw = 1;
}

static void _pushMacrov(Row *r, char nibble)
{
	short macro = (w->trackerfx - 2)>>1;
	r->macro[macro].v <<= 4;
	r->macro[macro].t <<= 4;
	switch (nibble)
	{
		/* TODO: check validity before pushing tokens */
		/* TODO: enum or smth for these constants     */
		case '~': r->macro[macro].t += 1; break;
		case '+': r->macro[macro].t += 2; break;
		case '-': r->macro[macro].t += 3; break;
		case '%': r->macro[macro].t += 4; break;

		default: r->macro[macro].v += nibble; break;
	}
}
static void pushMacrov(void *arg)
{
	Track *cv = s->track->v[w->track];
	switch (w->mode)
	{
		case MODE_VISUALREPLACE:
			for (short i = MIN(w->trackerfy, w->visualfy); i <= MAX(w->trackerfy, w->visualfy); i++)
				_pushMacrov(getTrackRow(cv, i), (size_t)arg);
			break;
		default:
			_pushMacrov(getTrackRow(cv, w->trackerfy), (size_t)arg);
			break;
	}
	regenGlobalRowc(s);
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
				r = getTrackRow(cv, i);
				r->macro[macro].c = (size_t)arg;
			} break;
		default:
			r = getTrackRow(cv, w->trackerfy);
			r->macro[macro].c = (size_t)arg;
			break;
	} regenGlobalRowc(s); p->redraw = 1;
}


static void trackerAdjustRight(Track *cv) /* mouse adjust only */
{
	Row *r = getTrackRow(cv, w->trackerfy);
	short macro;
	switch (w->trackerfx)
	{
		case -1:
			if (!w->playing)
			{
				if (w->fieldpointer) setVariantChainTrig(&cv->variant, w->trackerfy, cv->variant->trig[w->trackerfy].index+16);
				else                 setVariantChainTrig(&cv->variant, w->trackerfy, cv->variant->trig[w->trackerfy].index+1);
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
static void trackerAdjustLeft(Track *cv) /* mouse adjust only */
{
	Row *r = getTrackRow(cv, w->trackerfy);
	short macro;
	switch (w->trackerfx)
	{
		case -1:
			if (!w->playing)
			{
				if (w->fieldpointer) setVariantChainTrig(&cv->variant, w->trackerfy, cv->variant->trig[w->trackerfy].index-16);
				else                 setVariantChainTrig(&cv->variant, w->trackerfy, cv->variant->trig[w->trackerfy].index-1);
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

static bool trackerMouseHeader(enum Button button, int x, int y, short *tx)
{
	if (y <= TRACK_ROW)
		for (int i = 0; i < s->track->c; i++)
		{
			*tx += TRACK_TRIG_PAD + 11 + 4*(s->track->v[i]->variant->macroc+1);
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
static void trackerMouse(enum Button button, int x, int y)
{
	if (rulerMouse(button, x, y)) return;

	short chanw;
	int i, j;
	Track *cv = s->track->v[w->track];
	short oldtrackerfx = w->trackerfx;
	uint8_t oldtrack = w->track;

	short tx;

	p->redraw = 1;

	switch (button)
	{
		case BUTTON2_HOLD: case BUTTON2_HOLD_CTRL:
		case BUTTON3_HOLD: case BUTTON3_HOLD_CTRL:
			break;
		case WHEEL_UP: case WHEEL_UP_CTRL:     w->count = WHEEL_SPEED; trackerUpArrow  (1); w->count = 0; break;
		case WHEEL_DOWN: case WHEEL_DOWN_CTRL: w->count = WHEEL_SPEED; trackerDownArrow(1); w->count = 0; break;
		case BUTTON_RELEASE: case BUTTON_RELEASE_CTRL:
			if (w->trackoffset) trackSet(w->track + w->trackoffset);

			if      (w->fyoffset < 0) { w->count = -w->fyoffset; trackerUpArrow  (1); }
			else if (w->fyoffset > 0) { w->count =  w->fyoffset; trackerDownArrow(1); }

			if      (w->shiftoffset < 0) { w->count = -w->shiftoffset; shiftUp  (); }
			else if (w->shiftoffset > 0) { w->count =  w->shiftoffset; shiftDown(); }

			w->count = 0;
			w->fyoffset = w->shiftoffset = w->trackoffset = w->fieldpointer = 0;

			/* leave mouseadjust mode */
			if (w->mode == MODE_MOUSEADJUST) w->mode = w->oldmode;
			/* falls through intentionally */
		default:
			switch (w->mode)
			{
				case MODE_EFFECT:
					tx = 1 + genConstSfx(EFFECT_WIDTH);
					for (i = 0; i < s->track->c; i++)
					{
						tx += EFFECT_WIDTH;
						if (tx > x)
						{
							if (y <= TRACK_ROW)
							{
								switch (button)
								{
									case BUTTON2: case BUTTON2_CTRL: toggleTrackSolo(i); goto trackerInputEffectTrack;
									case BUTTON3: case BUTTON3_CTRL: toggleTrackMute(i); goto trackerInputEffectTrack;
									default: break;
								}
							}
							w->trackoffset = i - w->track;
							goto trackerInputEffectTrack; /* beeeg break */
						}
					}
trackerInputEffectTrack:
					mouseControls(button, x, y);
					break;
				default:
					tx = 1 + TRACK_LINENO_COLS + 2 + genSfx(TRACK_LINENO_COLS);
					if (trackerMouseHeader(button, x, y, &tx)) break;
					switch (button)
					{
						case BUTTON_RELEASE: case BUTTON_RELEASE_CTRL:
							break;
						case BUTTON1_HOLD: case BUTTON1_HOLD_CTRL:
							w->follow = 0;
							if (w->mode == MODE_MOUSEADJUST)
							{
								if      (x > w->mousex) { trackerAdjustRight(cv); }
								else if (x < w->mousex) { trackerAdjustLeft (cv); }
							}
							w->mousex = x; break;
						default: /* click */
							if (y > ws.ws_row - 1) break; /* ignore clicking out of range */
							w->follow = 0;

							for (i = 0; i < s->track->c; i++)
							{
								chanw = TRACK_TRIG_PAD + 11 + 4*(s->track->v[i]->variant->macroc+1);
								if (i == s->track->c-1 || tx+chanw > x) /* clicked on track i */
								{
									if (button == BUTTON1_CTRL) { w->step = MIN(15, abs(y - w->centre)); break; }

									switch (button)
									{
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
										default: break;
									}
									if (x-tx < TRACK_TRIG_PAD+2) /* vtrig column */
									{
										w->trackerfx = -1;
										if (x-tx < 2) w->fieldpointer = 1;
										else          w->fieldpointer = 0;
									} else if (x-tx < TRACK_TRIG_PAD + 6) /* note column */
										w->trackerfx = 0;
									else if (x-tx < TRACK_TRIG_PAD + 9) /* inst column */
									{
										w->trackerfx = 1;
										if (x-tx < TRACK_TRIG_PAD + 8) w->fieldpointer = 1;
										else                              w->fieldpointer = 0;
									} else if (x-tx > TRACK_TRIG_PAD + 8 + 4*(s->track->v[i]->variant->macroc+1)) /* star column */
									{
										w->trackerfx = 3;
										w->fieldpointer = 0;
									} else /* macro column */
									{
										j = x-tx - (TRACK_TRIG_PAD + 9);
										if ((j>>1)&0x1) w->trackerfx = 3 + ((s->track->v[i]->variant->macroc - (j>>2))<<1)+0;
										else            w->trackerfx = 3 + ((s->track->v[i]->variant->macroc - (j>>2))<<1)-1;
										if (j&0x1) w->fieldpointer = 0;
										else       w->fieldpointer = 1;
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
								tx += chanw;
							}
					}
					break;
			}
	}
}

static void addEffectBelow(void)
{
	w->pluginbrowserbefore = 0;
	w->oldpage = w->page;
	w->page = PAGE_PLUGINBROWSER;
	p->redraw = 1;
}
static void addEffectAbove(void)
{
	w->pluginbrowserbefore = 1;
	w->oldpage = w->page;
	w->page = PAGE_PLUGINBROWSER;
	p->redraw = 1;
}
static void pasteEffectBelow(uint8_t track)
{
	if (w->effectbuffer.type != EFFECT_TYPE_DUMMY)
		addEffect(&s->track->v[track]->effect, EFFECT_TYPE_DUMMY, -1, MIN(getEffectFromCursor(track, s->track->v[track]->effect, cc.cursor)+1, s->track->v[track]->effect->c), cb_addEffect);
	p->redraw = 1;
}
static void pasteEffectAbove(uint8_t track)
{
	if (w->effectbuffer.type != EFFECT_TYPE_DUMMY)
		addEffect(&s->track->v[track]->effect, EFFECT_TYPE_DUMMY, -1, getEffectFromCursor(track, s->track->v[track]->effect, cc.cursor), cb_addEffect);
	p->redraw = 1;
}
static void delChainEffect(uint8_t track)
{
	if (!s->track->v[track]->effect->c) return;
	uint8_t selectedindex = getEffectFromCursor(track, s->track->v[track]->effect, cc.cursor);
	cc.cursor = MAX(0, cc.cursor - (short)getEffectControlCount(&s->track->v[track]->effect->v[selectedindex]));
	delEffect(&s->track->v[track]->effect, selectedindex);
	p->redraw = 1;
}
static void copyChainEffect(uint8_t track)
{
	if (!s->track->v[track]->effect->c) return;
	uint8_t selectedindex = getEffectFromCursor(track, s->track->v[track]->effect, cc.cursor);
	copyEffect(&w->effectbuffer, &s->track->v[track]->effect->v[selectedindex], NULL, NULL);
	p->redraw = 1;
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
		case MODE_EFFECT:
			addCountBinds(0);
			addTrackerNavBinds();
			addTooltipBind("next effect", ControlMask, XK_Down, 0, (void(*)(void*))trackerPgUp, NULL);
			addTooltipBind("prev effect", ControlMask, XK_Up  , 0, (void(*)(void*))trackerPgDn, NULL);
			addRulerBinds();
			addTrackerGraphicBinds();
			addTooltipBind("return"                  , 0          , XK_e        , 0, (void(*)(void*))trackerEscape   , NULL);
			addTooltipBind("toggle checkmark button" , 0          , XK_Return   , 0, (void(*)(void*))toggleKeyControl, NULL);
			addTooltipBind("reset control to default", 0          , XK_BackSpace, 0, (void(*)(void*))revertKeyControl, NULL);
			addTooltipBind("add effect below"        , 0          , XK_a        , 0, (void(*)(void*))addEffectBelow  , NULL);
			addTooltipBind("add effect above"        , 0          , XK_A        , 0, (void(*)(void*))addEffectAbove  , NULL);
			addTooltipBind("paste effect below"      , 0          , XK_p        , 0, (void(*)(void*))pasteEffectBelow, (void*)(size_t)w->track);
			addTooltipBind("paste effect above"      , 0          , XK_P        , 0, (void(*)(void*))pasteEffectAbove, (void*)(size_t)w->track);
			addTooltipBind("delete effect"           , 0          , XK_d        , 0, (void(*)(void*))delChainEffect  , (void*)(size_t)w->track);
			addTooltipBind("copy effect"             , 0          , XK_y        , 0, (void(*)(void*))copyChainEffect , (void*)(size_t)w->track);
			addTooltipBind("increment"               , ControlMask, XK_A        , 0, (void(*)(void*))incControlValue , NULL);
			addTooltipBind("decrement"               , ControlMask, XK_X        , 0, (void(*)(void*))decControlValue , NULL);
			break;
		case MODE_NORMAL:
			addCountBinds(0);
			addTrackerNavBinds();
			addRulerBinds();
			addTrackerGraphicBinds();
			addTooltipBind("mute current track"    , 0                   , XK_Return    , 0              , (void(*)(void*))muteCurrentTrack          , NULL     );
			addTooltipBind("solo current track"    , 0                   , XK_s         , 0              , (void(*)(void*))soloCurrentTrack          , NULL     );
			addTooltipBind("inc work octave"       , 0                   , XK_plus      , TT_DRAW        , (void(*)(void*))incOctave                 , NULL     );
			addTooltipBind("inc work octave"       , 0                   , XK_equal     , 0              , (void(*)(void*))incOctave                 , NULL     );
			addTooltipBind("dec work octave"       , 0                   , XK_minus     , TT_DRAW        , (void(*)(void*))decOctave                 , NULL     );
			addTooltipBind("cursor left"           , 0                   , XK_h         , 0              , (void(*)(void*))trackerLeftArrow          , (void*)1 );
			addTooltipBind("cursor down"           , 0                   , XK_j         , 0              , (void(*)(void*))trackerDownArrow          , (void*)1 );
			addTooltipBind("cursor up"             , 0                   , XK_k         , 0              , (void(*)(void*))trackerUpArrow            , (void*)1 );
			addTooltipBind("cursor right"          , 0                   , XK_l         , 0              , (void(*)(void*))trackerRightArrow         , (void*)1 );
			addTooltipBind("variant cycle up"      , ControlMask         , XK_Up        , 0              , (void(*)(void*))cycleUp                   , NULL     );
			addTooltipBind("variant cycle down"    , ControlMask         , XK_Down      , 0              , (void(*)(void*))cycleDown                 , NULL     );
			addTooltipBind("song shift up"         , ShiftMask           , XK_Up        , 0              , (void(*)(void*))shiftUp                   , NULL     );
			addTooltipBind("song shift down"       , ShiftMask           , XK_Down      , 0              , (void(*)(void*))shiftDown                 , NULL     );

			addTooltipBind("slide track home"      , ShiftMask           , XK_Home      , 0              , (void(*)(void*))trackSlideHome            , NULL     );
			addTooltipBind("slide track end"       , ShiftMask           , XK_End       , 0              , (void(*)(void*))trackSlideEnd             , NULL     );
			addTooltipBind("slide track left"      , ShiftMask           , XK_Left      , 0              , (void(*)(void*))trackSlideLeft            , NULL     );
			addTooltipBind("slide track right"     , ShiftMask           , XK_Right     , 0              , (void(*)(void*))trackSlideRight           , NULL     );

			addTooltipBind("variant cycle up"      , ControlMask         , XK_K         , 0              , (void(*)(void*))cycleUp                   , NULL     );
			addTooltipBind("variant cycle down"    , ControlMask         , XK_J         , 0              , (void(*)(void*))cycleDown                 , NULL     );
			addTooltipBind("track left"            , ControlMask         , XK_H         , 0              , (void(*)(void*))trackLeft                 , NULL     );
			addTooltipBind("track right"           , ControlMask         , XK_L         , 0              , (void(*)(void*))trackRight                , NULL     );
			addTooltipBind("song shift up"         , 0                   , XK_K         , 0              , (void(*)(void*))shiftUp                   , NULL     );
			addTooltipBind("song shift down"       , 0                   , XK_J         , 0              , (void(*)(void*))shiftDown                 , NULL     );
			addTooltipBind("enter insert mode"     , 0                   , XK_i         , 0              , (void(*)(void*))trackerEnterInsertMode    , NULL     );
			addTooltipBind("enter effect mode"     , 0                   , XK_e         , 0              , (void(*)(void*))trackerEnterEffectMode    , NULL     );
			addTooltipBind("enter visual mode"     , 0                   , XK_v         , 0              , (void(*)(void*))trackerEnterVisualMode    , NULL     );
			addTooltipBind("enter visual mode"     , ControlMask         , XK_v         , 0              , (void(*)(void*))trackerEnterVisualMode    , NULL     );
			addTooltipBind("enter visual line mode", 0                   , XK_V         , 0              , (void(*)(void*))trackerEnterVisualLineMode, NULL     );
			addTooltipBind("delete"                , 0                   , XK_d         , TT_DEAD|TT_DRAW, (void(*)(void*))setChordDeleteRow         , NULL     );
			addTooltipBind("yank"                  , 0                   , XK_y         , TT_DEAD|TT_DRAW, (void(*)(void*))setChordYankRow           , NULL     );
			addTooltipBind("track"                 , 0                   , XK_c         , TT_DEAD|TT_DRAW, (void(*)(void*))setChordTrack             , NULL     );
			addTooltipBind("macro"                 , 0                   , XK_m         , TT_DEAD|TT_DRAW, (void(*)(void*))setChordMacro             , NULL     );
			addTooltipBind("row"                   , 0                   , XK_r         , TT_DEAD|TT_DRAW, (void(*)(void*))setChordRow               , NULL     );
			addTooltipBind("loop"                  , 0                   , XK_semicolon , TT_DEAD|TT_DRAW, (void(*)(void*))setChordLoop              , NULL     );
			addTooltipBind("put"                   , 0                   , XK_p         , TT_DRAW        , (void(*)(void*))putPartPattern            , (void*)1 );
			addTooltipBind("mix put"               , 0                   , XK_P         , 0              , (void(*)(void*))mixPutPartPattern         , (void*)0 );
			addTooltipBind("toggle song follow"    , 0                   , XK_f         , TT_DRAW        , (void(*)(void*))toggleSongFollow          , NULL     );
			addTooltipBind("clear cell"            , 0                   , XK_x         , TT_DRAW        , (void(*)(void*))clearCell                 , NULL     );
			addTooltipBind("clear cell"            , 0                   , XK_BackSpace , 0              , (void(*)(void*))clearCell                 , NULL     );
			addTooltipBind("jump loop points"      , 0                   , XK_percent   , TT_DRAW        , (void(*)(void*))swapLoopPoint             , NULL     );
			addTooltipBind("increment cell"        , ControlMask         , XK_A         , TT_DRAW        , trackerInc                                , (void*)1 );
			addTooltipBind("decrement cell"        , ControlMask         , XK_X         , TT_DRAW        , trackerDec                                , (void*)1 );
			addTooltipBind("octave increment cell" , ControlMask|Mod1Mask, XK_A         , TT_DRAW        , trackerInc                                , (void*)12);
			addTooltipBind("octave decrement cell" , ControlMask|Mod1Mask, XK_X         , TT_DRAW        , trackerDec                                , (void*)12);
			addTooltipBind("add empty variant"     , 0                   , XK_a         , 0              , (void(*)(void*))addEmptyVariant           , NULL     );
			addTooltipBind("toggle variant loop"   , 0                   , XK_period    , TT_DRAW        , (void(*)(void*))toggleVariantLoop         , NULL     );
			addTooltipBind("toggle cell case"      , 0                   , XK_asciitilde, TT_DRAW        , toggleCellCase                            , (void*)1 );
			break;
		case MODE_INSERT:
		case MODE_VISUALREPLACE:
			addTrackerNavBinds();
			addTooltipBind("mute current track"   , 0                   , XK_Return   , 0      , (void(*)(void*))muteCurrentTrack, NULL     );
			addTooltipBind("increment cell"       , ControlMask         , XK_A        , TT_DRAW, trackerInc                      , (void*)1 );
			addTooltipBind("decrement cell"       , ControlMask         , XK_X        , TT_DRAW, trackerDec                      , (void*)1 );
			addTooltipBind("octave increment cell", ControlMask|Mod1Mask, XK_A        , TT_DRAW, trackerInc                      , (void*)12);
			addTooltipBind("octave decrement cell", ControlMask|Mod1Mask, XK_X        , TT_DRAW, trackerDec                      , (void*)12);
			addTooltipBind("clear cell"           , 0                   , XK_BackSpace, 0      , (void(*)(void*))clearCell       , NULL     );
			switch (w->trackerfx)
			{
				case -1: /* vtrig */
					addHexBinds("push cell", 0, pushVtrig);
					addTooltipBind("stop cell", 0, XK_space, 0, setVtrig, (void*)VARIANT_OFF);
					break;
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
			addTooltipBind("rip rows to sample"   , 0                   , XK_b           , TT_DRAW, (void(*)(void*))bounceRows                   , NULL     );
			addTooltipBind("rip rows to variant"  , 0                   , XK_a           , TT_DRAW, (void(*)(void*))ripRows                      , NULL     );
			addTooltipBind("toggle variant loop"  , 0                   , XK_period      , 0      , (void(*)(void*))toggleVisualVariantLoop      , NULL     );
			addTooltipBind("jump loop points"     , 0                   , XK_percent     , 0      , (void(*)(void*))swapLoopPoint                , NULL     );
			addTooltipBind("set loop range"       , 0                   , XK_semicolon   , TT_DRAW, (void(*)(void*))setLoopPoints                , NULL     );
			break;
		default: break;
	}
}
