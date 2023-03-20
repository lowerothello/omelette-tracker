void chordRecordToggle(void *_)
{
	if (w->instrumentrecv != INST_REC_LOCK_OK && w->instrumentreci != w->instrument)
	{ /* stop whichever instrument is already recording */
		toggleRecording(w->instrumentreci, 0);
	} else
	{
		if (!instrumentSafe(s->instrument, w->instrument))
			addInstrument(w->instrument, 0, cb_addRecordInstrument, (void*)(size_t)index);
		else
			toggleRecording(w->instrument, 0);
	}
}
void chordRecordCueToggle(void *_)
{
	if (w->instrumentrecv != INST_REC_LOCK_OK && w->instrumentreci != w->instrument)
	{ /* stop whichever instrument is already recording */
		toggleRecording(w->instrumentreci, 1);
	} else
	{
		if (!instrumentSafe(s->instrument, w->instrument))
			addInstrument(w->instrument, 0, cb_addRecordCueInstrument, (void*)(size_t)index);
		else
			toggleRecording(w->instrument, 1);
	}
}
void chordRecordCancel(void *_)
{
	if (w->instrumentrecv != INST_REC_LOCK_OK)
		w->instrumentrecv = INST_REC_LOCK_PREP_CANCEL;
}


void setChordAddInst(void)
{
	clearTooltip();
	setTooltipTitle("add instrument");
	addCountBinds(0); /* TODO: count support would be cool */
	addTooltipBind("record sample    ", 0, XK_r     , TT_DRAW, chordRecordToggle   , NULL);
	addTooltipBind("cue sample record", 0, XK_q     , TT_DRAW, chordRecordCueToggle, NULL);
	addTooltipBind("return"           , 0, XK_Escape, 0      , instrumentEscape    , NULL);
	w->chord = 'a'; p->redraw = 1;
}
void setChordRecord(void)
{
	clearTooltip();
	setTooltipTitle("record");
	addCountBinds(0); /* TODO: count support would be cool */
	addTooltipBind("toggle recording now", 0, XK_r     , TT_DRAW, chordRecordToggle   , NULL);
	addTooltipBind("cue toggle recording", 0, XK_q     , TT_DRAW, chordRecordCueToggle, NULL);
	addTooltipBind("cancel recording    ", 0, XK_c     , TT_DRAW, chordRecordCancel   , NULL);
	addTooltipBind("return"              , 0, XK_Escape, 0      , instrumentEscape    , NULL);
	w->chord = 'r'; p->redraw = 1;
}
