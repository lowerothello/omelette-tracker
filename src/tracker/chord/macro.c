void chordAddMacro(void *_)
{
	Track *cv = s->track->v[w->track];

	for (int i = 0; i < MAX(1, w->count); i++)
		if (cv->pattern->macroc < 7)
			cv->pattern->macroc++;
	regenGlobalRowc(s);
	p->redraw = 1;
}
void chordDelMacro(void *_)
{
	Track *cv = s->track->v[w->track];

	for (int i = 0; i < MAX(1, w->count); i++)
	{
		if (cv->pattern->macroc)
			cv->pattern->macroc--;
		if (w->trackerfx > 3 + cv->pattern->macroc*2)
			w->trackerfx = 3 + cv->pattern->macroc*2;
	}
	regenGlobalRowc(s);
	p->redraw = 1;
}
void chordSetMacro(void *_)
{
	Track *cv = s->track->v[w->track];

	if (w->count) cv->pattern->macroc = MIN(8, w->count) - 1;
	else          cv->pattern->macroc = 1;
	if (w->trackerfx > 3 + cv->pattern->macroc*2)
		w->trackerfx = 3 + cv->pattern->macroc*2;
	regenGlobalRowc(s);
	p->redraw = 1;
}


void setChordMacro(void)
{
	clearTooltip();
	setTooltipTitle("macro");
	addCountBinds(0);
	addTooltipBind("increment macro columns   ", 0, XK_a     , TT_DRAW, chordAddMacro, NULL);
	addTooltipBind("decrement macro columns   ", 0, XK_d     , TT_DRAW, chordDelMacro, NULL);
	addTooltipBind("set macro columns to count", 0, XK_m     , TT_DRAW, chordSetMacro, NULL);
	addTooltipBind("return"                    , 0, XK_Escape, 0      , NULL         , NULL);
	w->chord = 'm'; p->redraw = 1;
}
