void chordGraphicHome(void *_)
{
	trackerHome();
	p->redraw = 1;
}

void setChordGraphic(void *tt)
{
	clearTooltip(tt);
	setTooltipTitle(tt, "graphic");
	addCountBinds(tt, 0);
	addTooltipBind(tt, "home"  , 0, XK_g     , TT_DRAW, chordGraphicHome, NULL);
	addTooltipBind(tt, "return", 0, XK_Escape, 0      , NULL            , NULL);
	w->chord = 'g'; p->redraw = 1;
}
