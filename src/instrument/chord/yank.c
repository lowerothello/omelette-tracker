void chordYankInstrument(void *_) { yankInstrument(w->instrument); p->redraw = 1; }


void setChordYankInstrument(void)
{
	clearTooltip(&tt);
	setTooltipTitle(&tt, "yank");
	addTooltipBind(&tt, "yank instrument", 'y', chordYankInstrument, NULL);
	w->chord = 'y';
}
