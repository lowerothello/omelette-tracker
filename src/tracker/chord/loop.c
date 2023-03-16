void setLoopRange(uint16_t start, uint16_t end)
{
	s->loop[0] = start;
	if (s->playing)
	{
		if (s->playfy < end) s->loop[1] = end;
		else                 s->loop[2] = end;
	} else s->loop[1] = end;
	p->redraw = 1;
}
void chordLoopBars(void *_)
{
	uint16_t ltrackerfy = MAX(w->trackerfy, STATE_ROWS);
	if (s->loop[0] == ltrackerfy)
		setLoopRange(0, 0);
	else
		setLoopRange(ltrackerfy, ltrackerfy + (4*s->rowhighlight)*MAX(1, w->count) - 1);

	regenGlobalRowc(s);
}
void chordLoopVariant(void *_)
{
	Track *cv = s->track->v[w->track];

	Variant *v;
	uint16_t ltrackerfy = MAX(w->trackerfy, STATE_ROWS);
	int gcvret = getTrackVariantNoLoop(&v, cv->variant, ltrackerfy);
	if (gcvret != -1)
	{
		if (s->loop[0] == ltrackerfy - gcvret && s->loop[1] == ltrackerfy - gcvret + v->rowc)
			 setLoopRange(0, 0);
		else setLoopRange(ltrackerfy - gcvret, ltrackerfy - gcvret + v->rowc);
		regenGlobalRowc(s);
	}
}
void chordDoubleLoopLength   (void *_) { if (!s->loop[1]) setLoopRange(STATE_ROWS, STATE_ROWS + 4*s->rowhighlight - 1); setLoopRange(s->loop[0], s->loop[0] + ((s->loop[1] - s->loop[0])<<1) + 1); regenGlobalRowc(s); }
void chordHalveLoopLength    (void *_) { if (!s->loop[1]) setLoopRange(STATE_ROWS, STATE_ROWS + 4*s->rowhighlight - 1); setLoopRange(s->loop[0], s->loop[0] + ((s->loop[1] - s->loop[0])>>1)); regenGlobalRowc(s);     }
void chordIncrementLoopLength(void *_) { if (!s->loop[1]) setLoopRange(STATE_ROWS, STATE_ROWS + 4*s->rowhighlight - 1); setLoopRange(s->loop[0], s->loop[1] + MAX(1, w->count)); regenGlobalRowc(s);                   }
void chordDecrementLoopLength(void *_) { if (!s->loop[1]) setLoopRange(STATE_ROWS, STATE_ROWS + 4*s->rowhighlight - 1); setLoopRange(s->loop[0], MAX(s->loop[0], s->loop[1] - MAX(1, w->count))); regenGlobalRowc(s);  }
void chordLoopScaleToCursor  (void *_)
{
	if (!s->loop[1])
		setLoopRange(STATE_ROWS, STATE_ROWS + 4*s->rowhighlight - 1);
	if (w->trackerfy < s->loop[0]) setLoopRange(MAX(w->trackerfy, STATE_ROWS), s->loop[1]);
	else                           setLoopRange(s->loop[0], w->trackerfy);
	regenGlobalRowc(s);
}


/* TODO: explicit loop reset bind, not sure which key to bind it to */
void setChordLoop(void)
{
	clearTooltip();
	setTooltipTitle("loop range");
	addCountBinds(0);
	addTooltipBind("loop bars                ", 0, XK_semicolon, TT_DRAW, chordLoopBars           , NULL);
	addTooltipBind("loop the current variant ", 0, XK_v        , TT_DRAW, chordLoopVariant        , NULL);
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
