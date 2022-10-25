void chordDeleteRow(void *_)
{
	ChannelData *cd = &s->channel->v[w->channel].data;
	yankPartPattern(0, 2+cd->macroc, w->trackerfy, w->trackerfy+MAX(1, w->count)-1, w->channel, w->channel);
	delPartPattern (0, 2+cd->macroc, w->trackerfy, w->trackerfy+MAX(1, w->count)-1, w->channel, w->channel);
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
