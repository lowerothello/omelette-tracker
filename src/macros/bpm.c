void macroBpm(uint32_t fptr, uint16_t *spr, int m, Track *cv, Row *r)
{
	if (m < 0) return;
	if (m == 0) setBpm(spr, p->s->songbpm);
	else        setBpm(spr, MAX(32, m));
}



#define MACRO_BPM 'B'

void macroBpmPreTrig(uint32_t fptr, uint16_t *spr, Track *cv, Row *r)
{
	FOR_ROW_MACROS(i, cv)
		if (r->macro[i].c == MACRO_BPM)
		{
			macroBpm(fptr, spr, r->macro[i].v, cv, r);
			return;
		}
}
