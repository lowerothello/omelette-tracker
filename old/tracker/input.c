static void setStep(void *step)
{
	w->step = (size_t)step;
	p->redraw = 1;
}
static void trackerEscape(void *arg)
{
	previewNote(' ', INST_VOID);
	cc.mouseadjust = cc.keyadjust = 0;
	if (w->page == PAGE_TRACK_VARIANT)
		switch (w->mode)
		{
			case T_MODE_VISUALREPLACE:                  w->mode = T_MODE_VISUAL; break;
			case T_MODE_VISUAL: case T_MODE_VISUALLINE: w->mode = T_MODE_NORMAL; break;
			default:                                    w->mode = T_MODE_NORMAL; w->keyboardmacro = '\0'; break;
		}
	p->redraw = 1;
}
static void trackerEnterVisualMode(void *arg)
{
	w->visualfx = tfxToVfx(w->trackerfx);
	w->visualfy = w->trackerfy;
	w->visualtrack = w->track;
	switch (w->mode)
	{
		case T_MODE_VISUAL: w->mode = T_MODE_NORMAL; break;
		default:            w->mode = T_MODE_VISUAL; break;
	}
	p->redraw = 1;
}
static void trackerEnterVisualLineMode(void *arg)
{
	w->visualfx = tfxToVfx(w->trackerfx);
	w->visualfy = w->trackerfy;
	w->visualtrack = w->track;
	switch (w->mode)
	{
		case T_MODE_VISUALLINE: w->mode = T_MODE_NORMAL;     break;
		default:                w->mode = T_MODE_VISUALLINE; break;
	}
	p->redraw = 1;
}
static void trackerEnterVisualReplaceMode(void *arg)
{
	w->mode = T_MODE_VISUALREPLACE;
	p->redraw = 1;
}
static void trackerEnterInsertMode(void *arg)
{
	w->mode = T_MODE_INSERT;
	p->redraw = 1;
}
static void setOctaveCount(void *arg)
{
	if (w->count)
	{
		w->octave = MIN(9, w->count);
		p->redraw = 1;
	}
}
static void setRowHighlightCount(void *arg)
{
	if (w->count)
	{
		s->rowhighlight = MIN(16, w->count);
		regenGlobalRowc(s);
		p->redraw = 1;
	}
}
static void toggleSongFollow(void *arg)
{
	w->follow = !w->follow;
	if (s->playing)
		w->trackerfy = s->playfy;
	p->redraw = 1;
}

static void muteCurrentTrack(void *arg)
{
	uint8_t i;
	switch (w->mode)
	{
		case T_MODE_VISUAL: case T_MODE_VISUALLINE:
			for (i = MIN(w->track, w->visualtrack); i <= MAX(w->track, w->visualtrack); i++)
				toggleTrackMute(i);
			break;
		default:
			toggleTrackMute(w->track);
			break;
	}
	p->redraw = 1;
}
static void soloCurrentTrack(void *arg)
{
	uint8_t i;
	switch (w->mode)
	{
		case T_MODE_VISUAL: case T_MODE_VISUALLINE:
			for (i = MIN(w->track, w->visualtrack); i <= MAX(w->track, w->visualtrack); i++)
				toggleTrackSolo(i);
			break;
		default:
			toggleTrackSolo(w->track);
			break;
	}
	p->redraw = 1;
}

static void swapLoopPoint(void *arg)
{
	if (s->loop[1])
	{
		if      (w->trackerfy == s->loop[0]) w->trackerfy = s->loop[1];
		else if (w->trackerfy == s->loop[1]) w->trackerfy = s->loop[0];
		else if (w->trackerfy < (s->loop[0] + s->loop[1])>>1) w->trackerfy = s->loop[0];
		else                                                  w->trackerfy = s->loop[1];
	} p->redraw = 1;
}
static void setLoopPoints(void *arg) /* TODO: shitty disambiguation */
{
	setLoopRange(MIN(w->trackerfy, w->visualfy), MAX(w->trackerfy, w->visualfy));
	regenGlobalRowc(s);
	p->redraw = 1;
}

static void addEmptyVariant(void *arg)
{
	TrackData *cd = &s->track->v[w->track].data;
	setVariantChainTrig(&cd->variant, w->trackerfy, dupEmptyVariantIndex(cd->variant, cd->variant->trig[w->trackerfy].index));
	regenGlobalRowc(s); p->redraw = 1;
}
/* returns true to force disable step */
static bool visualRange(int8_t *minx, int8_t *maxx, short *miny, short *maxy, uint8_t *mintrack, uint8_t *maxtrack)
{
	TrackData *cd;
	bool ret = 0;

	switch (w->mode)
	{ /* x range */
		case T_MODE_VISUALLINE:
			cd = &s->track->v[MAX(w->track, w->visualtrack)].data;
			*minx = TRACKERFX_VISUAL_MIN;
			*maxx = 2+cd->variant->macroc;
			break;
		case T_MODE_VISUAL:
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
		case T_MODE_VISUAL:
		case T_MODE_VISUALLINE:
		case T_MODE_VISUALREPLACE:
			*miny = MIN(w->trackerfy, w->visualfy);
			*maxy = MAX(w->trackerfy, w->visualfy);
			ret = 1;
			break;
		default: *miny = *maxy = w->trackerfy; break;
	}

	switch (w->mode)
	{ /* track range */
		case T_MODE_VISUAL:
		case T_MODE_VISUALLINE:
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
	if (step) trackerDownArrow(1);
	p->redraw = 1;
}
static void interpolateCells(void *arg)
{
	int8_t  minx,     maxx;
	short   miny,     maxy;
	uint8_t mintrack, maxtrack;
	visualRange(&minx, &maxx, &miny, &maxy, &mintrack, &maxtrack);

	interpolatePartPattern(minx, maxx, miny, maxy, mintrack, maxtrack);

	regenGlobalRowc(s); p->redraw = 1;
}
static void randomCells(void *arg)
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
	/* TODO: instrument default setting */
	int8_t  minx,     maxx;
	short   miny,     maxy;
	uint8_t mintrack, maxtrack;
	visualRange(&minx, &maxx, &miny, &maxy, &mintrack, &maxtrack);

	switch (w->mode)
	{
		case T_MODE_VISUALLINE: addPartPattern(value, minx, maxx, miny, maxy, mintrack, maxtrack, 0, 0); break;
		default:                addPartPattern(value, minx, maxx, miny, maxy, mintrack, maxtrack, 0, 1); break;
	}

	regenGlobalRowc(s); p->redraw = 1;
}
static void trackerInc(void *count) { trackerAdd(+(MAX(1, w->count) * (size_t)count)); }
static void trackerDec(void *count) { trackerAdd(-(MAX(1, w->count) * (size_t)count)); }

static void clearCell(void *arg)
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
		case T_MODE_VISUAL:
		case T_MODE_VISUALLINE:
			w->trackerfx = minx;
			w->track = mintrack;
			w->trackerfy = miny;
			w->mode = T_MODE_NORMAL;
			break;
		default: break;
	}

	p->redraw = 1;
}
static void yankCell(void *arg)
{
	int8_t  minx,     maxx;
	short   miny,     maxy;
	uint8_t mintrack, maxtrack;
	visualRange(&minx, &maxx, &miny, &maxy, &mintrack, &maxtrack);

	yankPartPattern(minx, maxx, miny, maxy, mintrack, maxtrack);

	regenGlobalRowc(s);

	w->trackerfx = minx;
	w->track = mintrack;
	w->trackerfy = miny;
	w->mode = T_MODE_NORMAL;

	p->redraw = 1;
}

static void toggleVariantLoop(void *arg)
{
	TrackData *cd = &s->track->v[w->track].data;
	int i = getVariantChainPrevVtrig(cd->variant, w->trackerfy);
	if (i != -1) cd->variant->trig[i].flags ^= C_VTRIG_LOOP;
	regenGlobalRowc(s); p->redraw = 1;
}
static void toggleVisualVariantLoop(void *arg)
{
	int8_t  minx,     maxx;
	short   miny,     maxy;
	uint8_t mintrack, maxtrack;
	visualRange(&minx, &maxx, &miny, &maxy, &mintrack, &maxtrack);

	loopPartPattern(miny, maxy, mintrack, maxtrack);

	regenGlobalRowc(s); p->redraw = 1;
}

static void bounceRows(void *arg)
{
	int8_t  minx,     maxx;
	short   miny,     maxy;
	uint8_t mintrack, maxtrack;
	visualRange(&minx, &maxx, &miny, &maxy, &mintrack, &maxtrack);

	bouncePartPattern(miny, maxy, mintrack, maxtrack);

	w->trackerfx = 0;
	w->track = mintrack;
	w->trackerfy = miny;
	w->mode = T_MODE_NORMAL;
	regenGlobalRowc(s); p->redraw = 1;
}

static void ripRows(void *arg)
{
	TrackData *cd = &s->track->v[w->track].data;
	Row *r;
	uint8_t vindex = 0;
	short vminrow = MIN(w->trackerfy, w->visualfy);
	/* loop to find the lowest common free index */
	for (int i = MIN(w->track, w->visualtrack); i <= MAX(w->track, w->visualtrack); i++)
		vindex = MAX(vindex, getEmptyVariantIndex(s->track->v[i].data.variant, vindex));

	/* loop to actually rip */
	uint8_t vlength;
	for (int i = MIN(w->track, w->visualtrack); i <= MAX(w->track, w->visualtrack); i++)
	{
		vlength = MIN(VARIANT_ROWMAX, MAX(w->trackerfy, w->visualfy) - vminrow);
		cd = &s->track->v[i].data;

		addVariant(&cd->variant, vindex, vlength);

		/* move rows to the new variant */
		for (int j = 0; j <= vlength; j++)
		{
			r = getTrackRow(cd, vminrow + j);
			memcpy(&cd->variant->v[cd->variant->i[vindex]]->rowv[j], r, sizeof(Row));

			/* only clear the source row if it's in the global variant */
			if (getVariantChainVariant(NULL, cd->variant, vminrow + j) == -1)
			{
				memset(r, 0, sizeof(Row));
				r->note = NOTE_VOID;
				r->inst = INST_VOID;
			}
		}

		/* unnecessarily complex edge case handling */
		if (cd->variant->trig[vminrow + vlength].index == VARIANT_VOID)
			for (int j = vminrow + vlength; j > vminrow; j--)
				if (cd->variant->trig[j].index != VARIANT_VOID)
				{
					if (cd->variant->i[cd->variant->trig[j].index] != VARIANT_VOID && j + cd->variant->v[cd->variant->i[cd->variant->trig[j].index]]->rowc > vminrow + vlength)
					{
						cd->variant->trig[vminrow + vlength].index = cd->variant->trig[j].index;
						// TODO: set vtrig offset to align this correctly
					} break;
				}

		setVariantChainTrig(&cd->variant, vminrow, vindex);
	}
	regenGlobalRowc(s); p->redraw = 1;
}

static void setOctave(void *octave)
{
	short i;
	Row *r;
	switch (w->mode)
	{
		case T_MODE_VISUALREPLACE:
			for (i = MIN(w->trackerfy, w->visualfy); i <= MAX(w->trackerfy, w->visualfy); i++)
			{
				r = getTrackRow(&s->track->v[w->track].data, i);
				r->note = changeNoteOctave((size_t)octave, r->note);
			} break;
		default:
			r = getTrackRow(&s->track->v[w->track].data, w->trackerfy);
			r->note = changeNoteOctave((size_t)octave, r->note);
			break;
	}
	regenGlobalRowc(s); p->redraw = 1;
}

static void pushVtrig(void *arg)
{
	short i;
	switch (w->mode)
	{
		case T_MODE_VISUALREPLACE:
			for (i = MIN(w->trackerfy, w->visualfy); i <= MAX(w->trackerfy, w->visualfy); i++)
				inputVariantChainTrig(&s->track->v[w->track].data.variant, i, (size_t)arg);
			break;
		default:
			inputVariantChainTrig(&s->track->v[w->track].data.variant, w->trackerfy, (size_t)arg);
			break;
	}
	regenGlobalRowc(s); p->redraw = 1;
}
static void setVtrig(void *arg)
{
	short i;
	switch (w->mode)
	{
		case T_MODE_VISUALREPLACE:
			for (i = MIN(w->trackerfy, w->visualfy); i <= MAX(w->trackerfy, w->visualfy); i++)
				setVariantChainTrig(&s->track->v[w->track].data.variant, i, (size_t)arg);
			break;
		default:
			setVariantChainTrig(&s->track->v[w->track].data.variant, w->trackerfy, (size_t)arg);
			break;
	}
	regenGlobalRowc(s); p->redraw = 1;
}

static void setNote(size_t note)
{
	Row *r = NULL;
	short i;
	bool step;
	switch (w->mode)
	{
		case T_MODE_VISUALREPLACE:
			for (i = MIN(w->trackerfy, w->visualfy); i <= MAX(w->trackerfy, w->visualfy); i++)
			{
				r = getTrackRow(&s->track->v[w->track].data, i);
				r->note = (size_t)note;
			} step = 0; break;
		default:
			r = getTrackRow(&s->track->v[w->track].data, w->trackerfy);
			r->note = note;
			r->inst = w->instrument;
			step = 1; break;
	}

	previewRow(r);
	regenGlobalRowc(s);
	if (step) trackerDownArrow(w->step);
	p->redraw = 1;
}
static void pushKeyboardMacroCallback(void *offset)
{
	Row *r = NULL;
	short i;
	bool step;
	switch (w->mode)
	{
		case T_MODE_VISUALREPLACE:
			for (i = MIN(w->trackerfy, w->visualfy); i <= MAX(w->trackerfy, w->visualfy); i++)
			{
				r = getTrackRow(&s->track->v[w->track].data, i);
				r->note = NOTE_C5;
				r->macro[0].c = w->keyboardmacro;
				if (linkMacroNibbles(w->keyboardmacro)) r->macro[0].v = ((size_t)offset<<4) + (size_t)offset;
				else                                    r->macro[0].v = ((r->macro[0].v&0x0f)<<4) + (size_t)offset;
			} step = 0; break;
		default:
			r = getTrackRow(&s->track->v[w->track].data, w->trackerfy);
			r->note = NOTE_C5;
			r->macro[0].c = w->keyboardmacro;
			if (linkMacroNibbles(w->keyboardmacro)) r->macro[0].v = ((size_t)offset<<4) + (size_t)offset;
			else                                    r->macro[0].v = ((r->macro[0].v&0x0f)<<4) + (size_t)offset;
			step = 1; break;
	}

	if (r) previewRow(r);
	regenGlobalRowc(s);
	if (step) trackerDownArrow(w->step);
	p->redraw = 1;
}

static void pushNoteCallback(void *offset) { setNote(MIN(NOTE_A10-1, (size_t)offset + w->octave*12)); }

static void imposeInst(void *arg)
{
	short i;
	Row *r;
	switch (w->mode)
	{
		case T_MODE_VISUALREPLACE:
			for (i = MIN(w->trackerfy, w->visualfy); i <= MAX(w->trackerfy, w->visualfy); i++)
			{
				r = getTrackRow(&s->track->v[w->track].data, i);
				r->inst = w->instrument;
			} break;
		default:
			r = getTrackRow(&s->track->v[w->track].data, w->trackerfy);
			r->inst = w->instrument;
			break;
	}
	regenGlobalRowc(s); p->redraw = 1;
}

static void pushInst(void *arg)
{
	short i;
	Row *r = NULL;
	switch (w->mode)
	{
		case T_MODE_VISUALREPLACE:
			for (i = MIN(w->trackerfy, w->visualfy); i <= MAX(w->trackerfy, w->visualfy); i++)
			{
				r = getTrackRow(&s->track->v[w->track].data, i);
				if (r->inst == INST_VOID) r->inst++;
				r->inst <<= 4;
				r->inst += (size_t)arg;
			} break;
		default:
			r = getTrackRow(&s->track->v[w->track].data, w->trackerfy);
			if (r->inst == INST_VOID) r->inst++;
			r->inst <<= 4;
			r->inst += (size_t)arg;
			break;
	}

	if (r && w->instrumentrecv == INST_REC_LOCK_OK) w->instrument = r->inst;
	regenGlobalRowc(s); p->redraw = 1;
}
static void pushMacrov(void *arg)
{
	Row *r;
	short i;
	short macro = (w->trackerfx - 2)>>1;
	switch (w->mode)
	{
		case T_MODE_VISUALREPLACE:
			for (i = MIN(w->trackerfy, w->visualfy); i <= MAX(w->trackerfy, w->visualfy); i++)
			{
				r = getTrackRow(&s->track->v[w->track].data, i);
				r->macro[macro].v <<= 4;
				r->macro[macro].v += (size_t)arg;
			} break;
		default:
			r = getTrackRow(&s->track->v[w->track].data, w->trackerfy);
			r->macro[macro].v <<= 4;
			r->macro[macro].v += (size_t)arg;
			break;
	}

	regenGlobalRowc(s); p->redraw = 1;
}
static void pushMacroc(void *arg)
{
	Row *r;
	short i;
	short macro = (w->trackerfx - 2)>>1;
	switch (w->mode)
	{
		case T_MODE_VISUALREPLACE:
			for (i = MIN(w->trackerfy, w->visualfy); i <= MAX(w->trackerfy, w->visualfy); i++)
			{
				r = getTrackRow(&s->track->v[w->track].data, i);
				r->macro[macro].c = (size_t)arg;
			} break;
		default:
			r = getTrackRow(&s->track->v[w->track].data, w->trackerfy);
			r->macro[macro].c = (size_t)arg;
			break;
	}

	regenGlobalRowc(s); p->redraw = 1;
}


static void trackerAdjustRight(TrackData *cd) /* mouse adjust only */
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
static void trackerAdjustLeft(TrackData *cd) /* mouse adjust only */
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
static bool trackerMouseHeader(enum Button button, int x, int y, short *tx)
{
	if (y <= TRACK_ROW)
		for (int i = 0; i < s->track->c; i++)
		{
			*tx += TRACK_TRIG_PAD + 11 + 4*(s->track->v[i].data.variant->macroc+1);
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
	short chanw;
	int i, j;
	TrackData *cd = &s->track->v[w->track].data;
	short oldtrackerfx = w->trackerfx;
	uint8_t oldtrack = w->track;

	short tx = 1 + TRACK_LINENO_COLS + 2 + genSfx(TRACK_LINENO_COLS);

	switch (button)
	{
		case BUTTON2_HOLD: case BUTTON2_HOLD_CTRL:
		case BUTTON3_HOLD: case BUTTON3_HOLD_CTRL:
			break;
		case BUTTON_RELEASE: case BUTTON_RELEASE_CTRL:
			w->count = 0;
			if      (w->trackoffset < 0) trackLeft (-w->trackoffset);
			else if (w->trackoffset > 0) trackRight( w->trackoffset);

			if      (w->fyoffset < 0) trackerUpArrow  (-w->fyoffset);
			else if (w->fyoffset > 0) trackerDownArrow( w->fyoffset);

			if      (w->shiftoffset < 0) shiftUp  (-w->shiftoffset);
			else if (w->shiftoffset > 0) shiftDown( w->shiftoffset);
			w->fyoffset = w->shiftoffset = w->trackoffset = w->fieldpointer = 0;

			/* leave mouseadjust mode */
			if (w->mode == T_MODE_MOUSEADJUST) w->mode = w->oldmode;

			p->redraw = 1;
			/* falls through intentionally */
		default:
			switch (w->page)
			{
				case PAGE_TRACK_VARIANT:
					switch (button)
					{
						case BUTTON_RELEASE: case BUTTON_RELEASE_CTRL:
							break;
						case WHEEL_UP: case WHEEL_UP_CTRL:     trackerUpArrow  (WHEEL_SPEED); break;
						case WHEEL_DOWN: case WHEEL_DOWN_CTRL: trackerDownArrow(WHEEL_SPEED); break;
						case BUTTON1_HOLD: case BUTTON1_HOLD_CTRL:
							w->follow = 0;
							if (w->mode == T_MODE_MOUSEADJUST)
							{
								if      (x > w->mousex) { trackerAdjustRight(cd); }
								else if (x < w->mousex) { trackerAdjustLeft (cd); }
							}
							w->mousex = x; break;
						default: /* click */
							if (trackerMouseHeader(button, x, y, &tx)) break;

							if (y > ws.ws_row - 1) break; /* ignore clicking out of range */
							w->follow = 0;

							for (i = 0; i < s->track->c; i++)
							{
								chanw = TRACK_TRIG_PAD + 11 + 4*(s->track->v[i].data.variant->macroc+1);
								if (i == s->track->c-1 || tx+chanw > x) /* clicked on track i */
								{
									if (button == BUTTON1_CTRL) { w->step = MIN(15, abs(y - w->centre)); break; }

									switch (button)
									{
										case BUTTON1:
											if (w->mode != T_MODE_INSERT) /* suggest normal mode, but allow insert */
												w->mode = T_MODE_NORMAL;
											break;
										case BUTTON3:
											if (!(w->mode == T_MODE_VISUAL || w->mode == T_MODE_VISUALLINE || w->mode == T_MODE_VISUALREPLACE))
											{
												w->visualfx = tfxToVfx(oldtrackerfx);
												w->visualfy = w->trackerfy;
												w->visualtrack = oldtrack;
												w->mode = T_MODE_VISUAL;
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
									} else if (x-tx > TRACK_TRIG_PAD + 8 + 4*(s->track->v[i].data.variant->macroc+1)) /* star column */
									{
										w->trackerfx = 3;
										w->fieldpointer = 0;
									} else /* macro column */
									{
										j = x-tx - (TRACK_TRIG_PAD + 9);
										if ((j>>1)&0x1) w->trackerfx = 3 + ((s->track->v[i].data.variant->macroc - (j>>2))<<1)+0;
										else            w->trackerfx = 3 + ((s->track->v[i].data.variant->macroc - (j>>2))<<1)-1;
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
											&& w->fyoffset == 0 && w->trackerfx == oldtrackerfx && w->trackoffset == 0)
									{
										w->oldmode = w->mode;
										w->mode = T_MODE_MOUSEADJUST;
										w->mousex = x;
									} break;
								}
								tx += chanw;
							}
					} break;
				case PAGE_TRACK_EFFECT:
					switch (button)
					{
						case WHEEL_UP: case WHEEL_UP_CTRL:     effectPgUp(cd->effect, WHEEL_SPEED); break;
						case WHEEL_DOWN: case WHEEL_DOWN_CTRL: effectPgDn(cd->effect, WHEEL_SPEED); break;
						default: mouseControls(&cc, button, x, y); break;
					} break;
				default: break;
			} p->redraw = 1; break;
	}
}

void initTrackerInput(TooltipState *tt)
{
	setTooltipTitle(tt, "tracker");
	setTooltipMouseCallback(tt, trackerMouse);
	addTooltipBind(tt, "cursor up"      , 0          , XK_Up       , 0, (void(*)(void*))trackerUpArrow   , (void*)1);
	addTooltipBind(tt, "cursor down"    , 0          , XK_Down     , 0, (void(*)(void*))trackerDownArrow , (void*)1);
	addTooltipBind(tt, "cursor left"    , 0          , XK_Left     , 0, (void(*)(void*))trackerLeftArrow , (void*)1);
	addTooltipBind(tt, "cursor right"   , 0          , XK_Right    , 0, (void(*)(void*))trackerRightArrow, (void*)1);
	addTooltipBind(tt, "cursor home"    , 0          , XK_Home     , 0, (void(*)(void*))trackerHome      , NULL    );
	addTooltipBind(tt, "cursor end"     , 0          , XK_End      , 0, (void(*)(void*))trackerEnd       , NULL    );
	addTooltipBind(tt, "cursor pgup"    , 0          , XK_Page_Up  , 0, (void(*)(void*))trackerPgUp      , (void*)1);
	addTooltipBind(tt, "cursor pgdn"    , 0          , XK_Page_Down, 0, (void(*)(void*))trackerPgDn      , (void*)1);
	addTooltipBind(tt, "cycle up"       , ControlMask, XK_Up       , 0, (void(*)(void*))cycleUp          , (void*)1);
	addTooltipBind(tt, "cycle down"     , ControlMask, XK_Down     , 0, (void(*)(void*))cycleDown        , (void*)1);
	addTooltipBind(tt, "track left"     , ControlMask, XK_Left     , 0, (void(*)(void*))trackLeft        , (void*)1);
	addTooltipBind(tt, "track right"    , ControlMask, XK_Right    , 0, (void(*)(void*))trackRight       , (void*)1);
	addTooltipBind(tt, "song shift up"  , ShiftMask  , XK_Up       , 0, (void(*)(void*))shiftUp          , (void*)1);
	addTooltipBind(tt, "song shift down", ShiftMask  , XK_Down     , 0, (void(*)(void*))shiftDown        , (void*)1);
	addTooltipBind(tt, "return"         , 0          , XK_Escape   , 0, trackerEscape                    , NULL    );
	addDecimalBinds(tt, "set step", Mod1Mask, setStep);
	switch (w->page)
	{
		case PAGE_TRACK_VARIANT:
			addTooltipBind(tt, "mute current track", 0, XK_Return, 0, muteCurrentTrack, NULL);
			switch (w->mode)
			{
				case T_MODE_NORMAL:
					addTooltipBind(tt, "solo current track"    , 0                   , XK_s           , 0              , soloCurrentTrack                 , NULL     );
					addCountBinds(tt, 0);
					addTooltipBind(tt, "cursor left"           , 0                   , XK_h           , 0              , (void(*)(void*))trackerLeftArrow , (void*)1 );
					addTooltipBind(tt, "cursor down"           , 0                   , XK_j           , 0              , (void(*)(void*))trackerDownArrow , (void*)1 );
					addTooltipBind(tt, "cursor up"             , 0                   , XK_k           , 0              , (void(*)(void*))trackerUpArrow   , (void*)1 );
					addTooltipBind(tt, "cursor right"          , 0                   , XK_l           , 0              , (void(*)(void*))trackerRightArrow, (void*)1 );
					addTooltipBind(tt, "track left"            , 0                   , XK_bracketleft , 0              , (void(*)(void*))trackLeft        , (void*)1 );
					addTooltipBind(tt, "track right"           , 0                   , XK_bracketright, 0              , (void(*)(void*))trackRight       , (void*)1 );
					addTooltipBind(tt, "cycle up"              , 0                   , XK_braceleft   , 0              , (void(*)(void*))cycleUp          , (void*)1 );
					addTooltipBind(tt, "cycle down"            , 0                   , XK_braceright  , 0              , (void(*)(void*))cycleDown        , (void*)1 );
					addTooltipBind(tt, "song shift up"         , 0                   , XK_less        , 0              , (void(*)(void*))shiftUp          , (void*)1 );
					addTooltipBind(tt, "song shift down"       , 0                   , XK_greater     , 0              , (void(*)(void*))shiftDown        , (void*)1 );
					addTooltipBind(tt, "enter insert mode"     , 0                   , XK_i           , 0              , trackerEnterInsertMode           , NULL     );
					addTooltipBind(tt, "set insert mode macro" , 0                   , XK_I           , 0              , setChordMacroInsert              , tt       );
					addTooltipBind(tt, "enter visual mode"     , 0                   , XK_v           , 0              , trackerEnterVisualMode           , NULL     );
					addTooltipBind(tt, "enter visual mode"     , ControlMask         , XK_V           , 0              , trackerEnterVisualMode           , NULL     );
					addTooltipBind(tt, "enter visual line mode", 0                   , XK_V           , 0              , trackerEnterVisualLineMode       , NULL     );
					addTooltipBind(tt, "delete"                , 0                   , XK_d           , TT_DEAD|TT_DRAW, setChordDeleteRow                , tt       );
					addTooltipBind(tt, "graphic"               , 0                   , XK_g           , TT_DEAD|TT_DRAW, setChordGraphic                  , tt       );
					addTooltipBind(tt, "graphic end"           , 0                   , XK_G           , 0              , (void(*)(void*))trackerEnd       , NULL     );
					addTooltipBind(tt, "yank"                  , 0                   , XK_y           , TT_DEAD|TT_DRAW, setChordYankRow                  , tt       );
					addTooltipBind(tt, "track"                 , 0                   , XK_c           , TT_DEAD|TT_DRAW, setChordTrack                    , tt       );
					addTooltipBind(tt, "macro"                 , 0                   , XK_m           , TT_DEAD|TT_DRAW, setChordMacro                    , tt       );
					addTooltipBind(tt, "row"                   , 0                   , XK_r           , TT_DEAD|TT_DRAW, setChordRow                      , tt       );
					addTooltipBind(tt, "loop"                  , 0                   , XK_semicolon   , TT_DEAD|TT_DRAW, setChordLoop                     , tt       );
					addTooltipBind(tt, "put"                   , 0                   , XK_p           , 0              , (void(*)(void*))putPartPattern   , (void*)1 );
					addTooltipBind(tt, "mix put"               , 0                   , XK_P           , 0              , (void(*)(void*))mixPutPartPattern, (void*)0 );
					addTooltipBind(tt, "set bpm"               , 0                   , XK_b           , 0              , (void(*)(void*))setBpmCount      , NULL     );
					addTooltipBind(tt, "set row highlight"     , 0                   , XK_t           , 0              , setRowHighlightCount             , NULL     );
					addTooltipBind(tt, "set octave"            , 0                   , XK_o           , 0              , setOctaveCount                   , NULL     );
					addTooltipBind(tt, "toggle song follow"    , 0                   , XK_f           , 0              , toggleSongFollow                 , NULL     );
					addTooltipBind(tt, "clear cell"            , 0                   , XK_x           , 0              , clearCell                        , NULL     );
					addTooltipBind(tt, "clear cell"            , 0                   , XK_BackSpace   , 0              , clearCell                        , NULL     );
					addTooltipBind(tt, "jump loop points"      , 0                   , XK_percent     , 0              , swapLoopPoint                    , NULL     );
					addTooltipBind(tt, "increment cell"        , ControlMask         , XK_A           , TT_DRAW        , trackerInc                       , (void*)1 );
					addTooltipBind(tt, "decrement cell"        , ControlMask         , XK_X           , TT_DRAW        , trackerDec                       , (void*)1 );
					addTooltipBind(tt, "octave increment cell" , ControlMask|Mod1Mask, XK_A           , TT_DRAW        , trackerInc                       , (void*)12);
					addTooltipBind(tt, "octave decrement cell" , ControlMask|Mod1Mask, XK_X           , TT_DRAW        , trackerDec                       , (void*)12);
					addTooltipBind(tt, "add empty variant"     , 0                   , XK_a           , 0              , addEmptyVariant                  , NULL     );
					addTooltipBind(tt, "toggle variant loop"   , 0                   , XK_period      , 0              , toggleVariantLoop                , NULL     );
					addTooltipBind(tt, "toggle cell case"      , 0                   , XK_asciitilde  , 0              , toggleCellCase                   , (void*)1 );
					break;
				case T_MODE_INSERT:
				case T_MODE_VISUALREPLACE:
					// addCountBinds(tt, 0); /* TODO: call this, deprecated octave binds conflict currently */
					addTooltipBind(tt, "increment cell"       , ControlMask         , XK_A        , TT_DRAW, trackerInc, (void*)1 );
					addTooltipBind(tt, "decrement cell"       , ControlMask         , XK_X        , TT_DRAW, trackerDec, (void*)1 );
					addTooltipBind(tt, "octave increment cell", ControlMask|Mod1Mask, XK_A        , TT_DRAW, trackerInc, (void*)12);
					addTooltipBind(tt, "octave decrement cell", ControlMask|Mod1Mask, XK_X        , TT_DRAW, trackerDec, (void*)12);
					addTooltipBind(tt, "clear cell"           , 0                   , XK_BackSpace, 0      , clearCell , NULL     );
					switch (w->trackerfx)
					{
						case -1: /* vtrig */
							addHexBinds(tt, "push cell", 0, pushVtrig);
							addTooltipBind(tt, "stop cell", 0, XK_space, 0, setVtrig, (void*)VARIANT_OFF);
							break;
						case 0: /* note */
							if (w->keyboardmacro)
								addHexBinds(tt, "push cell", 0, pushKeyboardMacroCallback);
							else
							{
								addDecimalBinds(tt, "set octave", 0, setOctave);
								addNoteBinds(tt, "push cell", 0, pushNoteCallback);
							}
							addTooltipBind(tt, "stop cell", 0, XK_space, 0, (void(*)(void*))setNote, (void*)NOTE_OFF);
							break;
						case 1: /* inst */
							addHexBinds(tt, "push cell", 0, pushInst);
							addTooltipBind(tt, "impose state on cell", 0, XK_space, 0, imposeInst, NULL);
							break;
						default: /* macro */
							addTooltipBind(tt, "toggle cell case", 0, XK_asciitilde, 0, toggleCellCase, (void*)0);
							if (!(w->trackerfx&1)) /* macroc */ addMacroBinds(tt, "push cell", 0, pushMacroc);
							else                   /* macrov */ addHexBinds  (tt, "push cell", 0, pushMacrov);
							break;
					} break;
				case T_MODE_VISUAL:
				case T_MODE_VISUALLINE:
					addTooltipBind(tt, "solo current track"   , 0                   , XK_s           , 0      , soloCurrentTrack                 , NULL     );
					addCountBinds(tt, 0);
					addTooltipBind(tt, "cursor left"          , 0                   , XK_h           , 0      , (void(*)(void*))trackerLeftArrow , (void*)1 );
					addTooltipBind(tt, "cursor down"          , 0                   , XK_j           , 0      , (void(*)(void*))trackerDownArrow , (void*)1 );
					addTooltipBind(tt, "cursor up"            , 0                   , XK_k           , 0      , (void(*)(void*))trackerUpArrow   , (void*)1 );
					addTooltipBind(tt, "cursor right"         , 0                   , XK_l           , 0      , (void(*)(void*))trackerRightArrow, (void*)1 );
					addTooltipBind(tt, "track left"           , 0                   , XK_bracketleft , 0      , (void(*)(void*))trackLeft        , (void*)1 );
					addTooltipBind(tt, "track right"          , 0                   , XK_bracketright, 0      , (void(*)(void*))trackRight       , (void*)1 );
					addTooltipBind(tt, "cell-wise"            , 0                   , XK_v           , 0      , trackerEnterVisualMode           , NULL     );
					addTooltipBind(tt, "cell-wise"            , ControlMask         , XK_V           , 0      , trackerEnterVisualMode           , NULL     );
					addTooltipBind(tt, "line-wise"            , 0                   , XK_V           , 0      , trackerEnterVisualLineMode       , NULL     );
					addTooltipBind(tt, "replace"              , 0                   , XK_r           , 0      , trackerEnterVisualReplaceMode    , NULL     );
					addTooltipBind(tt, "toggle cell case"     , 0                   , XK_asciitilde  , 0      , toggleCellCase                   , (void*)0 );
					addTooltipBind(tt, "interpolate cells"    , 0                   , XK_i           , 0      , interpolateCells                 , NULL     );
					addTooltipBind(tt, "randomize cells"      , 0                   , XK_percent     , 0      , randomCells                      , NULL     );
					addTooltipBind(tt, "increment cell"       , ControlMask         , XK_A           , TT_DRAW, trackerInc                       , (void*)1 );
					addTooltipBind(tt, "decrement cell"       , ControlMask         , XK_X           , TT_DRAW, trackerDec                       , (void*)1 );
					addTooltipBind(tt, "octave increment cell", ControlMask|Mod1Mask, XK_A           , TT_DRAW, trackerInc                       , (void*)12);
					addTooltipBind(tt, "octave decrement cell", ControlMask|Mod1Mask, XK_X           , TT_DRAW, trackerDec                       , (void*)12);
					addTooltipBind(tt, "delete"               , 0                   , XK_x           , 0      , clearCell                        , NULL     );
					addTooltipBind(tt, "delete"               , 0                   , XK_d           , 0      , clearCell                        , NULL     );
					addTooltipBind(tt, "delete"               , 0                   , XK_BackSpace   , 0      , clearCell                        , NULL     );
					addTooltipBind(tt, "yank"                 , 0                   , XK_y           , 0      , yankCell                         , NULL     );
					addTooltipBind(tt, "rip rows to sample"   , 0                   , XK_b           , 0      , bounceRows                       , NULL     );
					addTooltipBind(tt, "rip rows to variant"  , 0                   , XK_a           , 0      , ripRows                          , NULL     );
					addTooltipBind(tt, "toggle variant loop"  , 0                   , XK_period      , 0      , toggleVisualVariantLoop          , NULL     );
					addTooltipBind(tt, "jump loop points"     , 0                   , XK_percent     , 0      , swapLoopPoint                    , NULL     );
					addTooltipBind(tt, "set loop range"       , 0                   , XK_semicolon   , 0      , setLoopPoints                    , NULL     );
					break;
				default: break;
			} break;
		case PAGE_TRACK_EFFECT:
			addTooltipBind(tt, "track"            , 0, XK_c, TT_DEAD, setChordTrack              , tt  );
			addTooltipBind(tt, "set bpm"          , 0, XK_b, 0      , (void(*)(void*))setBpmCount, NULL);
			addTooltipBind(tt, "set row highlight", 0, XK_t, 0      , setRowHighlightCount       , NULL);
			addTooltipBind(tt, "set octave"       , 0, XK_o, 0      , setOctaveCount             , NULL);
			initEffectInput(tt, &s->track->v[w->track].data.effect);
			break;
		default: break;
	}
}

						// case 'f': /* toggle inst browsr */ w->instrument = emptyInstrument(0); chordAddSample(0); return;
						// case 'r': /* record inst        */ setChordRecord(); p->redraw = 1; return;
						// case 'a': /* add empty inst     */ w->instrument = emptyInstrument(0); setChordAddInst(); p->redraw = 1; return;
						// case 'e': /* add empty inst     */                                     setChordAddInst(); p->redraw = 1; return;