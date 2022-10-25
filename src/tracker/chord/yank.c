void chordYankRow(void *_)
{
	ChannelData *cd = &s->channel->v[w->channel].data;
	yankPartPattern(0, 2+cd->macroc, w->trackerfy, w->trackerfy+MAX(1, w->count)-1, w->channel, w->channel);
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
