static bool _macroOffset(jack_nframes_t fptr, uint16_t *spr, int m, Track *cv, Row *r)
{
	if (p->s->instrument->i[cv->r.inst] < p->s->instrument->c)
	{
		Instrument *iv = &p->s->instrument->v[p->s->instrument->i[cv->r.inst]];
		if (cv->r.note != NOTE_VOID) /* if playing a note */
		{
			if (r->note == NOTE_VOID) /* if not changing note, explicit ramping needed */
				ramp(cv, 0.0f, p->s->instrument->i[cv->r.inst]);
			cv->pitchedpointer = (m*DIV256) * iv->trimlength * (float)samplerate / (float)iv->sample->rate;
		}
	} return 0;
}
bool macroOffset(jack_nframes_t fptr, uint16_t *spr, int m, Track *cv, Row *r)
{
	if (m < 0) return 0;

	cv->reverse = 0;
	return _macroOffset(fptr, spr, m, cv, r);
}
bool macroReverseOffset(jack_nframes_t fptr, uint16_t *spr, int m, Track *cv, Row *r)
{
	if (m < 0) return 0;

	cv->reverse = 1;
	if (m) return _macroOffset(fptr, spr, m, cv, r);
	return 0;
}
bool macroOffsetJitter(jack_nframes_t fptr, uint16_t *spr, int m, Track *cv, Row *r)
{
	if (m < 0) return 0;

	if (p->s->instrument->i[cv->r.inst] < p->s->instrument->c)
	{
		Instrument *iv = &p->s->instrument->v[p->s->instrument->i[cv->r.inst]];
		if (cv->r.note != NOTE_VOID) /* if playing a note */
		{
			if (r->note == NOTE_VOID) /* if not changing note, explicit ramping needed */
				ramp(cv, 0.0f, p->s->instrument->i[cv->r.inst]);
			if (m>>4 == (m&0xf)) /* both nibbles are the same */
				cv->pitchedpointer = (((m&0xf) + (rand()&0xf))*DIV256) * iv->trimlength * (float)samplerate / (float)iv->sample->rate;
			else
			{
				int min = MIN(m>>4, m&0xf);
				int max = MAX(m>>4, m&0xf);
				cv->pitchedpointer = (((min + rand()%(max - min +1))<<4)*DIV256) * iv->trimlength * (float)samplerate / (float)iv->sample->rate;
			}
		}
	} return 0;
}
/* TODO: should never reverse in place, kinda important cos this case ramps wrongly */
bool macroReverseOffsetJitter(jack_nframes_t fptr, uint16_t *spr, int m, Track *cv, Row *r)
{
	if (m < 0) return 0;

	cv->reverse = !cv->reverse;
	return macroOffsetJitter(fptr, spr, m, cv, r);
}
