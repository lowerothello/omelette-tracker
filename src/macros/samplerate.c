bool macroLocalSamplerate(jack_nframes_t fptr, uint16_t *spr, int m, Track *cv, Row *r)
{
	if (m < 0) return 0;

	cv->localsamplerate = m;
	return 1;
}
bool macroSmoothLocalSamplerate(jack_nframes_t fptr, uint16_t *spr, int m, Track *cv, Row *r)
{
	if (cv->targetlocalsamplerate != -1) { cv->localsamplerate = cv->targetlocalsamplerate; cv->targetlocalsamplerate = -1; }
	if (m < 0) return 0;

	cv->targetlocalsamplerate = m;
	return 1;
}
