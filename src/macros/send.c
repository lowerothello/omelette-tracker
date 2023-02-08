bool macroSend(jack_nframes_t fptr, uint16_t *spr, int m, Track *cv, Row *r)
{
	if (m < 0) return 0;

	cv->send.base = cv->send.rand = m;
	return 1;
}
bool macroSmoothSend(jack_nframes_t fptr, uint16_t *spr, int m, Track *cv, Row *r)
{
	if (cv->send.target != -1)
	{
		if (cv->send.target_rand) /* only apply the new gain to rand, not to base */
		{
			cv->send.rand = cv->send.target;
			cv->send.target_rand = 0;
		} else cv->send.base = cv->send.rand = cv->send.target;
		cv->send.target = -1;
	}
	if (m < 0) return 0;

	cv->send.target = m;
	return 1;
}
bool macroSendJitter(jack_nframes_t fptr, uint16_t *spr, int m, Track *cv, Row *r)
{
	if (m < 0) return 0;

	signed char stereo = rand()%((m>>4)+1);
	cv->send.rand =
		 (MAX(0, (cv->send.base>>4)  - stereo - rand()%((m&0x0f)+1))<<4)
		+ MAX(0, (cv->send.base&0xf) - stereo - rand()%((m&0x0f)+1));
	return 1;
}
bool macroSmoothSendJitter(jack_nframes_t fptr, uint16_t *spr, int m, Track *cv, Row *r)
{
	if (m < 0) return 0;

	cv->send.target_rand = 1;
	signed char stereo = rand()%((m>>4)+1);
	cv->send.target =
		 (MAX(0, (cv->send.base>>4)  - stereo - rand()%((m&0x0f)+1))<<4)
		+ MAX(0, (cv->send.base&0xf) - stereo - rand()%((m&0x0f)+1));
	return 1;
}
