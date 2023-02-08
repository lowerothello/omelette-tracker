bool macroCut(jack_nframes_t fptr, uint16_t *spr, int m, Track *cv, Row *r)
{
	if (m < 0) return 0;

	if (!m)
	{ /* cut now */
		ramp(cv, 0.0f, p->s->instrument->i[cv->r.inst]); /* TODO: proper rowprogress */
		triggerNote(fptr, cv, cv->r.note, NOTE_OFF, cv->r.inst);
		cv->cutsamples = 0;
		r->note = NOTE_VOID;
		return 1;
	} else /* cut later */
		cv->cutsamples = *spr * m*DIV256;
	return 0;
}
