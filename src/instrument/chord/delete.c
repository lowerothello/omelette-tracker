void chordDeleteInstrument(void *_)
{
	if (!(w->instrumentrecv != INST_REC_LOCK_OK && w->instrumentreci == w->instrument) && instrumentSafe(s->instrument, w->instrument))
	{
		yankInstrument(w->instrument);
		delInstrument (w->instrument);
	} p->redraw = 1;
}


void setChordDeleteInstrument(void)
{
	clearTooltip();
	setTooltipTitle("delete");
	addCountBinds(0);
	addTooltipBind("delete instrument", 0, XK_d     , TT_DRAW, chordDeleteInstrument, NULL);
	addTooltipBind("return"           , 0, XK_Escape, 0      , instrumentEscape     , NULL);
	w->chord = 'd'; p->redraw = 1;
}
