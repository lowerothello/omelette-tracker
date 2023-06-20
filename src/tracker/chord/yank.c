void chordYankRow(void *_)
{
	Track *cv = s->track->v[w->track];
	yankPartPattern(0, 2+cv->pattern->commandc, w->trackerfy, w->trackerfy+MAX(1, w->count)-1, w->track, w->track);
	trackerDownArrow(MAX(1, w->count));
	p->redraw = 1;
}


void setChordYankRow(void)
{
	clearTooltip();
	setTooltipTitle("yank");
	addCountBinds(0);
	addTooltipBind("yank row", 0, XK_y     , TT_DRAW, chordYankRow, NULL);
	addTooltipBind("return"  , 0, XK_Escape, 0      , NULL        , NULL);
	w->chord = 'y'; p->redraw = 1;
}
