void chordDeleteRow(void *_)
{
	Track *cv = s->track->v[w->track];
	yankPartPattern(0, 2+cv->variant->macroc, w->trackerfy, w->trackerfy+MAX(1, w->count)-1, w->track, w->track);
	delPartPattern (0, 2+cv->variant->macroc, w->trackerfy, w->trackerfy+MAX(1, w->count)-1, w->track, w->track);
	trackerDownArrow(MAX(1, w->count));
	regenGlobalRowc(s);
}


void setChordDeleteRow(void)
{
	clearTooltip();
	setTooltipTitle("delete");
	addCountBinds(0);
	addTooltipBind("delete row", 0, XK_d     , TT_DRAW, chordDeleteRow, NULL);
	addTooltipBind("return"    , 0, XK_Escape, 0      , NULL          , NULL);
	w->chord = 'd'; p->redraw = 1;
}
