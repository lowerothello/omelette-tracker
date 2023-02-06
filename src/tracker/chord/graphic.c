void chordGraphicHome(void *_)
{
	trackerHome();
	p->redraw = 1;
}

void setChordGraphic(void)
{
	clearTooltip();
	setTooltipTitle("graphic");
	addCountBinds(0);
	addTooltipBind("home"  , 0, XK_g     , TT_DRAW, chordGraphicHome, NULL);
	addTooltipBind("return", 0, XK_Escape, 0      , NULL            , NULL);
	w->chord = 'g'; p->redraw = 1;
}

void addTrackerGraphicBinds()
{
	addTooltipBind("graphic"    , 0, XK_g, TT_DEAD|TT_DRAW, (void(*)(void*))setChordGraphic, NULL);
	addTooltipBind("graphic end", 0, XK_G, 0              , (void(*)(void*))trackerEnd     , NULL);
}
