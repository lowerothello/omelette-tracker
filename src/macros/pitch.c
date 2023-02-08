bool macroPortamento(jack_nframes_t fptr, uint16_t *spr, int m, Track *cv, Row *r)
{
	if (m < 0) return 0;

	if (cv->portamentosamplepointer > cv->portamentosamples)
	{
		cv->portamentosamples = (*spr * m)/16;
		cv->portamentosamplepointer = 0;
		cv->startportamentofinetune = cv->portamentofinetune;
		cv->targetportamentofinetune = (r->note - (cv->r.note + cv->portamentofinetune));
	}
	r->note = NOTE_VOID;
	return 1;
}

bool macroPitchOffset(jack_nframes_t fptr, uint16_t *spr, int m, Track *cv, Row *r)
{
	if (m < 0) return 0;

	cv->microtonalfinetune = m*DIV255;
	return 0;
}

bool macroVibrato(jack_nframes_t fptr, uint16_t *spr, int m, Track *cv, Row *r)
{
	if (m < 0) return 0;

	cv->vibrato = m&0xf;
	if (!cv->vibratosamples) /* reset the phase if starting */
		cv->vibratosamplepointer = 0;
	cv->vibratosamples = *spr / (((m>>4) + 1) / 16.0); /* use floats for slower lfo speeds */
	cv->vibratosamplepointer = MIN(cv->vibratosamplepointer, cv->vibratosamples - 1);
	return 1;
}
