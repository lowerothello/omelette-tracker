void commandBpm(uint32_t fptr, uint16_t *spr, int m, Track *cv, Row *r)
{
	if (m < 0) return;
	if (m == 0) setBpm(spr, p->s->songbpm);
	else        setBpm(spr, MAX(32, m));
}



#define COMMAND_BPM 'B'

void commandBpmPreTrig(uint32_t fptr, uint16_t *spr, Track *cv, Row *r, void *state)
{
	FOR_ROW_COMMANDS(i, cv)
		switch (r->command[i].c)
		{
			case COMMAND_BPM:
				commandBpm(fptr, spr, r->command[i].v, cv, r);
				break;
		}
}
