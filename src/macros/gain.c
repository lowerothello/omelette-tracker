#define MACRO_GAIN        'G'
#define MACRO_SMOOTH_GAIN 'g'

void macroGainClear(Track *cv, void *state)
{
	((MacroState*)state)->base = ((MacroState*)state)->rand = 0x88;
	((MacroState*)state)->target = -1;
	((MacroState*)state)->target_rand = 0;
}

void macroGainPreTrig(uint32_t fptr, uint16_t *spr, Track *cv, Row *r, void *state)
{
	macroStateApply(state);

	FOR_ROW_MACROS(i, cv)
	{
		switch (r->macro[i].c)
		{
			case MACRO_GAIN: macroStateSet(state, r->macro[i]); break;
			case MACRO_SMOOTH_GAIN: macroStateSmooth(state, r->macro[i]); break;
		}
	}
}
