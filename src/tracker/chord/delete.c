void chordDeleteRow(void *_)
{
	TrackData *cd = &s->track->v[w->track].data;
	yankPartPattern(0, 2+cd->variant->macroc, w->trackerfy, w->trackerfy+MAX(1, w->count)-1, w->track, w->track);
	delPartPattern (0, 2+cd->variant->macroc, w->trackerfy, w->trackerfy+MAX(1, w->count)-1, w->track, w->track);
	trackerDownArrow(MAX(1, w->count));
	regenGlobalRowc(s);
}


void setChordDeleteRow(void *tt)
{
	clearTooltip(tt);
	setTooltipTitle(tt, "delete");
	addCountBinds(tt, 0);
	addTooltipBind(tt, "delete row", 0, XK_d     , TT_DRAW, chordDeleteRow, NULL);
	addTooltipBind(tt, "return"    , 0, XK_Escape, 0      , NULL          , NULL);
	w->chord = 'd'; p->redraw = 1;
}
