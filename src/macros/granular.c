bool macroLocalPitchShift(jack_nframes_t fptr, uint16_t *spr, int m, Track *cv, Row *r)
{
	if (m < 0) return 0;

	cv->localpitchshift = m;
	return 1;
}
bool macroSmoothLocalPitchShift(jack_nframes_t fptr, uint16_t *spr, int m, Track *cv, Row *r)
{
	if (cv->targetlocalpitchshift != -1) { cv->localpitchshift = cv->targetlocalpitchshift; cv->targetlocalpitchshift = -1; }
	if (m < 0) return 0;

	cv->targetlocalpitchshift = m;
	return 1;
}
bool macroLocalPitchWidth(jack_nframes_t fptr, uint16_t *spr, int m, Track *cv, Row *r)
{
	if (m < 0) return 0;

	cv->localpitchwidth = m;
	return 1;
}
bool macroSmoothLocalPitchWidth(jack_nframes_t fptr, uint16_t *spr, int m, Track *cv, Row *r)
{
	if (cv->targetlocalpitchwidth != -1) { cv->localpitchwidth = cv->targetlocalpitchwidth; cv->targetlocalpitchwidth = -1; }
	if (m < 0) return 0;

	cv->targetlocalpitchwidth = m;
	return 1;
}
bool macroLocalCycleLengthHighByte(jack_nframes_t fptr, uint16_t *spr, int m, Track *cv, Row *r)
{
	if (m < 0) return 0;

	if (p->s->instrument->i[cv->r.inst] < p->s->instrument->c)
	{
		Instrument *iv = &p->s->instrument->v[p->s->instrument->i[cv->r.inst]];
		if (cv->localcyclelength == -1) cv->localcyclelength = iv->granular.cyclelength;
		cv->localcyclelength = (((uint16_t)cv->localcyclelength<<8)>>8) + (m<<8);
	}
	return 1;
}
bool macroLocalCycleLengthLowByte(jack_nframes_t fptr, uint16_t *spr, int m, Track *cv, Row *r)
{
	if (m < 0) return 0;

	if (p->s->instrument->i[cv->r.inst] < p->s->instrument->c)
	{
		Instrument *iv = &p->s->instrument->v[p->s->instrument->i[cv->r.inst]];
		if (cv->localcyclelength == -1) cv->localcyclelength = iv->granular.cyclelength;
		cv->localcyclelength = (((uint16_t)cv->localcyclelength>>8)<<8)+m;
	}
	return 1;
}
