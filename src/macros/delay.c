bool macroDelay(jack_nframes_t fptr, uint16_t *spr, int m, Track *cv, Row *r)
{
	if (m <= 0) return 0;

	cv->delaysamples = *spr * m*DIV256;
	cv->delaynote = r->note;
	cv->delayinst = r->inst;
	r->note = NOTE_VOID;
	return 1;
}
