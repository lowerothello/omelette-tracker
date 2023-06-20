#define COMMAND_NOTE_CHANCE '%'

void commandChancePreTrig(uint32_t fptr, uint16_t *spr, Track *cv, Row *r, void *state)
{
	FOR_ROW_COMMANDS(i, cv)
		if (r->command[i].c == COMMAND_NOTE_CHANCE)
		{
			if (r->command[i].v && rand()%256 > r->command[i].v) r->note = NOTE_VOID;
			return;
		}
}
