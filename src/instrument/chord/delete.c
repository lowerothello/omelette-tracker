void chordDeleteInstrument(void *_)
{
	if (!(w->instrumentrecv != INST_REC_LOCK_OK && w->instrumentreci == w->instrument) && instrumentSafe(s->instrument, w->instrument))
	{
		yankInstrument(w->instrument);
		delInstrument (w->instrument);
	} p->redraw = 1;
}


void setChordDeleteInstrument(void *tt)
{
	clearTooltip(tt);
	setTooltipTitle(tt, "delete");
	addCountBinds(tt, 0);
	addTooltipBind(tt, "delete instrument", 0, XK_d     , TT_DRAW, chordDeleteInstrument, NULL);
	addTooltipBind(tt, "return"           , 0, XK_Escape, 0      , NULL                 , NULL);
	w->chord = 'D'; p->redraw = 1;
}
