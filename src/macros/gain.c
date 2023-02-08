bool macroGain(jack_nframes_t fptr, uint16_t *spr, int m, Track *cv, Row *r)
{
	if (m < 0) return 0;

	cv->gain.base = cv->gain.rand = m;
	return 1;
}
bool macroSmoothGain(jack_nframes_t fptr, uint16_t *spr, int m, Track *cv, Row *r)
{
	if (cv->gain.target != -1)
	{
		if (cv->gain.target_rand) /* only apply the new gain to rand, not to base */
		{
			cv->gain.rand = cv->gain.target;
			cv->gain.target_rand = 0;
		} else cv->gain.base = cv->gain.rand = cv->gain.target;
		cv->gain.target = -1;
	}
	if (m < 0) return 0;

	cv->gain.target = m;
	return 1;
}
bool macroGainJitter(jack_nframes_t fptr, uint16_t *spr, int m, Track *cv, Row *r)
{
	if (m < 0) return 0;

	signed char stereo = rand()%((m>>4)+1);
	cv->gain.rand =
		 (MAX(0, (cv->gain.base>>4)  - stereo - rand()%((m&0x0f)+1))<<4)
		+ MAX(0, (cv->gain.base&0xf) - stereo - rand()%((m&0x0f)+1));
	return 1;
}
bool macroSmoothGainJitter(jack_nframes_t fptr, uint16_t *spr, int m, Track *cv, Row *r)
{
	if (m < 0) return 0;

	cv->gain.target_rand = 1;
	signed char stereo = rand()%((m>>4)+1);
	cv->gain.target =
		 (MAX(0, (cv->gain.base>>4)  - stereo - rand()%((m&0x0f)+1))<<4)
		+ MAX(0, (cv->gain.base&0xf) - stereo - rand()%((m&0x0f)+1));
	return 1;
}
