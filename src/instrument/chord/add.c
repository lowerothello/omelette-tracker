void chordRecordToggle(void *_)
{
	if (w->instrecv != INST_REC_LOCK_OK && w->instreci != w->instrument)
	{ /* stop whichever inst is already recording */
		toggleRecording(w->instreci, 0);
	} else
	{
		if (!instSafe(s->inst, w->instrument))
			addInst(w->instrument, 0, cb_addRecordInst, (void*)(size_t)index);
		else
			toggleRecording(w->instrument, 0);
	}
}
void chordRecordCueToggle(void *_)
{
	if (w->instrecv != INST_REC_LOCK_OK && w->instreci != w->instrument)
	{ /* stop whichever inst is already recording */
		toggleRecording(w->instreci, 1);
	} else
	{
		if (!instSafe(s->inst, w->instrument))
			addInst(w->instrument, 0, cb_addRecordCueInst, (void*)(size_t)index);
		else
			toggleRecording(w->instrument, 1);
	}
}
void chordRecordCancel(void *_)
{
	if (w->instrecv != INST_REC_LOCK_OK)
		w->instrecv = INST_REC_LOCK_PREP_CANCEL;
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
