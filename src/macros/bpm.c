bool macroBpm(jack_nframes_t fptr, uint16_t *spr, int m, Track *cv, Row *r)
{
	if (m < 0) return 0;

	if (m == 0) setBpm(spr, p->s->songbpm);
	else        setBpm(spr, MAX(32, m));
	return 0;
}
