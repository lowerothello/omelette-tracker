void chordDeleteInstrument(void *_)
{
	if (!(w->instrumentrecv != INST_REC_LOCK_OK && w->instrumentreci == w->instrument) && instrumentSafe(s, w->instrument))
	{
		yankInstrument(w->instrument);
		delInstrument (w->instrument);
	} p->redraw = 1;
}


void setChordDeleteInstrument(void)
{
	clearTooltip(&tt);
	setTooltipTitle(&tt, "delete");
	addTooltipBind(&tt, "delete instrument", 'd', chordDeleteInstrument, NULL);
	w->chord = 'D';
}
