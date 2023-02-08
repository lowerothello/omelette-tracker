bool macroRowChance(jack_nframes_t fptr, uint16_t *spr, int m, Track *cv, Row *r) /* returns true to NOT play */
{
	if (m < 0) return 0;

	if (rand()%256 > m) return 1;
	else                return 0;
}
