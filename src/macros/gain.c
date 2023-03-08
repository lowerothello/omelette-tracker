#define MACRO_GAIN        'G'
#define MACRO_SMOOTH_GAIN 'g'

void macroGainPreTrig(jack_nframes_t fptr, uint16_t *spr, Track *cv, Row *r)
{
	macroStateApply(&cv->gain);

	FOR_ROW_MACROS(i, cv)
	{
		switch (r->macro[i].c)
		{
			case MACRO_GAIN:        macroStateSet   (&cv->gain, r->macro[i]); break;
			case MACRO_SMOOTH_GAIN: macroStateSmooth(&cv->gain, r->macro[i]); break;
		}
	}
}
