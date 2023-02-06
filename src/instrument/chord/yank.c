void chordYankInstrument(void *_) { yankInstrument(w->instrument); p->redraw = 1; }


void setChordYankInstrument(void)
{
	clearTooltip();
	setTooltipTitle("yank");
	addCountBinds(0);
	addTooltipBind("yank instrument", 0, XK_y     , TT_DRAW, chordYankInstrument, NULL);
	addTooltipBind("return"         , 0, XK_Escape, 0      , instrumentEscape   , NULL);
	w->chord = 'y'; p->redraw = 1;
}
