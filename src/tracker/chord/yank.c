void chordYankRow(void *_)
{
	TrackData *cd = &s->track->v[w->track].data;
	yankPartPattern(0, 2+cd->variant->macroc, w->trackerfy, w->trackerfy+MAX(1, w->count)-1, w->track, w->track);
	trackerDownArrow(MAX(1, w->count));
	p->redraw = 1;
}


void setChordYankRow(void *tt)
{
	clearTooltip(tt);
	setTooltipTitle(tt, "yank");
	addCountBinds(tt, 0);
	addTooltipBind(tt, "yank row", 0, XK_y     , TT_DRAW, chordYankRow, NULL);
	addTooltipBind(tt, "return"  , 0, XK_Escape, 0      , NULL        , NULL);
	w->chord = 'y'; p->redraw = 1;
}
