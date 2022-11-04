void chordGraphicHome(void *_)
{
	trackerHome();
	p->redraw = 1;
}


void setChordDeleteRow(void)
{
	clearTooltip(&tt);
	setTooltipTitle(&tt, "graphic");
	addTooltipBind(&tt, "home", 'g', chordGraphicHome, NULL);
	w->chord = 'g';
}
