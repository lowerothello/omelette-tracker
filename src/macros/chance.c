#define MACRO_NOTE_CHANCE '%'

void macroChancePreTrig(uint32_t fptr, uint16_t *spr, Track *cv, Row *r, void *state)
{
	FOR_ROW_MACROS(i, cv)
		if (r->macro[i].c == MACRO_NOTE_CHANCE)
		{
			if (r->macro[i].v && rand()%256 > r->macro[i].v) r->note = NOTE_VOID;
			return;
		}
}
