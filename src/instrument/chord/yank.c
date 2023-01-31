void chordYankInstrument(void *_) { yankInstrument(w->instrument); p->redraw = 1; }


void setChordYankInstrument(void *tt)
{
	clearTooltip(tt);
	setTooltipTitle(tt, "yank");
	addCountBinds(tt, 0);
	addTooltipBind(tt, "yank instrument", 0, XK_y     , TT_DRAW, chordYankInstrument, NULL);
	addTooltipBind(tt, "return"         , 0, XK_Escape, 0      , instrumentEscape   , NULL);
	w->chord = 'y'; p->redraw = 1;
}
