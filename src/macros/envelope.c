bool macroLocalAttDec(jack_nframes_t fptr, uint16_t *spr, int m, Track *cv, Row *r)
{
	if (m < 0) return 0;

	cv->localenvelope = m;
	return 1;
}
bool macroLocalSusRel(jack_nframes_t fptr, uint16_t *spr, int m, Track *cv, Row *r)
{
	if (m < 0) return 0;

	cv->localsustain = m;
	return 1;
}
