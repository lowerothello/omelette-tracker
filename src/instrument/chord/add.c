void chordAddSample(void *_) { addInstrument(w->instrument, INST_ALG_GRANULAR, cb_addInstrument); p->redraw = 1; }
void chordAddMIDI  (void *_) { addInstrument(w->instrument, INST_ALG_MIDI,     cb_addInstrument); p->redraw = 1; }

void chordRecordToggle(void *_)
{
	if (w->instrumentrecv != INST_REC_LOCK_OK && w->instrumentreci != w->instrument)
	{ /* stop whichever instrument is already recording */
		toggleRecording(w->instrumentreci, 0);
	} else
	{
		if (!instrumentSafe(s, w->instrument)) addInstrument(w->instrument, 0, cb_addRecordInstrument);
		else                                   toggleRecording(w->instrument, 0);
	}
}
void chordRecordCueToggle(void *_)
{
	if (w->instrumentrecv != INST_REC_LOCK_OK && w->instrumentreci != w->instrument)
	{ /* stop whichever instrument is already recording */
		toggleRecording(w->instrumentreci, 1);
	} else
	{
		if (!instrumentSafe(s, w->instrument)) addInstrument(w->instrument, 0, cb_addRecordCueInstrument);
		else                                   toggleRecording(w->instrument, 1);
	}
}
void chordRecordCancel(void *_)
{
	if (w->instrumentrecv != INST_REC_LOCK_OK)
		w->instrumentrecv = INST_REC_LOCK_PREP_CANCEL;
}


void setChordAddInst(void)
{
	clearTooltip(&tt);
	setTooltipTitle(&tt, "add instrument");
	addTooltipBind(&tt, "load sample      ", 'a', chordAddSample, NULL);
	addTooltipBind(&tt, "record sample    ", 'r', chordRecordToggle, NULL);
	addTooltipBind(&tt, "cue sample record", 'q', chordRecordCueToggle, NULL);
	addTooltipBind(&tt, "MIDI             ", 'm', chordAddMIDI, NULL);
	w->chord = 'a';
}
void setChordRecord(void)
{
	clearTooltip(&tt);
	setTooltipTitle(&tt, "record");
	addTooltipBind(&tt, "toggle recording now", 'r', chordRecordToggle, NULL);
	addTooltipBind(&tt, "cue toggle recording", 'q', chordRecordCueToggle, NULL);
	addTooltipBind(&tt, "cancel recording    ", 'c', chordRecordCancel, NULL);
	w->chord = 'r';
}
