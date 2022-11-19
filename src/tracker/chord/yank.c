void chordYankRow(void *_)
{
	TrackData *cd = &s->track->v[w->track].data;
	yankPartPattern(0, 2+cd->variant->macroc, w->trackerfy, w->trackerfy+MAX(1, w->count)-1, w->track, w->track);
	trackerDownArrow(MAX(1, w->count));
	p->redraw = 1;
}


void setChordYankRow(void)
{
	clearTooltip(&tt);
	setTooltipTitle(&tt, "yank");
	addTooltipBind(&tt, "yank row", 'y', chordYankRow, NULL);
	w->chord = 'y';
}
