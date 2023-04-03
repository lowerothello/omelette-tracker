#define MACRO_GAIN        'G'
#define MACRO_SMOOTH_GAIN 'g'

void macroGainClear(Track *cv, void *state)
{
	cv->gain.base = cv->gain.rand = 0x88;
	cv->gain.target = -1;
	cv->gain.target_rand = 0;
}

void macroGainPreTrig(uint32_t fptr, uint16_t *spr, Track *cv, Row *r, void *state)
{
	macroStateApply(&cv->gain);

	FOR_ROW_MACROS(i, cv)
	{
		switch (r->macro[i].c)
		{
			case MACRO_GAIN: macroStateSet(&cv->gain, r->macro[i]); break;
			case MACRO_SMOOTH_GAIN: macroStateSmooth(&cv->gain, r->macro[i]); break;
		}
	}
}
