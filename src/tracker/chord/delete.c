void chordDeleteRow(void *_)
{
	TrackData *cd = &s->track->v[w->track].data;
	yankPartPattern(0, 2+cd->variant->macroc, w->trackerfy, w->trackerfy+MAX(1, w->count)-1, w->track, w->track);
	delPartPattern (0, 2+cd->variant->macroc, w->trackerfy, w->trackerfy+MAX(1, w->count)-1, w->track, w->track);
	trackerDownArrow(MAX(1, w->count));
	regenGlobalRowc(s);
}


void setChordDeleteRow(void)
{
	clearTooltip(&tt);
	setTooltipTitle(&tt, "delete");
	addTooltipBind(&tt, "delete row", 'd', chordDeleteRow, NULL);
	w->chord = 'd';
}
