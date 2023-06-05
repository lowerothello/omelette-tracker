void setLoopRange(uint16_t start, uint16_t end)
{
	s->loop[0] = start;
	if (w->playing)
	{
		if (w->playfy < end) s->loop[1] = end;
		else                 s->loop[2] = end;
	} else s->loop[1] = end;
	p->redraw = 1;
}
void chordLoopBars(void *_)
{
	uint16_t ltrackerfy = w->trackerfy;
	if (s->loop[0] == ltrackerfy)
		setLoopRange(0, 0);
	else
		setLoopRange(ltrackerfy, ltrackerfy + (4*s->rowhighlight)*MAX(1, w->count) - 1);

	regenGlobalRowc(s);
	p->redraw = 1;
}
void chordLoopPattern(void *_)
{
	uint16_t pindex = w->trackerfy / getPatternLength();
	uint16_t lstart = pindex * getPatternLength();
	uint16_t lend = (pindex+1) * getPatternLength();
	if (s->loop[0] == lstart && s->loop[1] == lend)
		setLoopRange(0, 0);
	else
		setLoopRange(lstart, lend);

	regenGlobalRowc(s);
	p->redraw = 1;
}
void chordDoubleLoopLength   (void *_) { if (!s->loop[1]) setLoopRange(0, 4*s->rowhighlight - 1); setLoopRange(s->loop[0], s->loop[0] + ((s->loop[1] - s->loop[0])<<1) + 1); regenGlobalRowc(s); p->redraw = 1; }
void chordHalveLoopLength    (void *_) { if (!s->loop[1]) setLoopRange(0, 4*s->rowhighlight - 1); setLoopRange(s->loop[0], s->loop[0] + ((s->loop[1] - s->loop[0])>>1));     regenGlobalRowc(s); p->redraw = 1; }
void chordIncrementLoopLength(void *_) { if (!s->loop[1]) setLoopRange(0, 4*s->rowhighlight - 1); setLoopRange(s->loop[0], s->loop[1] + MAX(1, w->count));                   regenGlobalRowc(s); p->redraw = 1; }
void chordDecrementLoopLength(void *_) { if (!s->loop[1]) setLoopRange(0, 4*s->rowhighlight - 1); setLoopRange(s->loop[0], MAX(s->loop[0], s->loop[1] - MAX(1, w->count)));  regenGlobalRowc(s); p->redraw = 1; }
void chordLoopScaleToCursor  (void *_)
{
	if (!s->loop[1])
		setLoopRange(0, 4*s->rowhighlight - 1);
	if (w->trackerfy < s->loop[0]) setLoopRange(w->trackerfy, s->loop[1]);
	else                           setLoopRange(s->loop[0], w->trackerfy);
	regenGlobalRowc(s);
	p->redraw = 1;
}


/* TODO: explicit loop reset bind, not sure which key to bind it to */
void setChordLoop(void)
{
	clearTooltip();
	setTooltipTitle("loop range");
	addCountBinds(0);
	addTooltipBind("loop bars                ", 0, XK_semicolon, TT_DRAW, chordLoopBars           , NULL);
	addTooltipBind("loop the current pattern ", 0, XK_v        , TT_DRAW, chordLoopPattern        , NULL);
	addTooltipBind("double the loop length   ", 0, XK_plus     , TT_DRAW, chordDoubleLoopLength   , NULL);
	addTooltipBind("double the loop length   ", 0, XK_asterisk , TT_DRAW, chordDoubleLoopLength   , NULL);
	addTooltipBind("halve the loop length    ", 0, XK_minus    , TT_DRAW, chordHalveLoopLength    , NULL);
	addTooltipBind("halve the loop length    ", 0, XK_slash    , TT_DRAW, chordHalveLoopLength    , NULL);
	addTooltipBind("increment the loop length", 0, XK_a        , TT_DRAW, chordIncrementLoopLength, NULL);
	addTooltipBind("decrement the loop length", 0, XK_d        , TT_DRAW, chordDecrementLoopLength, NULL);
	addTooltipBind("scale loop to cursor     ", 0, XK_c        , TT_DRAW, chordLoopScaleToCursor  , NULL);
	addTooltipBind("return"                   , 0, XK_Escape   , 0      , NULL                    , NULL);
	w->chord = ';'; p->redraw = 1;
}
