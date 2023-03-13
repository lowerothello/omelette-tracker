#define MACRO_SEND        'S'
#define MACRO_SMOOTH_SEND 's'

void macroSendPreTrig(uint32_t fptr, uint16_t *spr, Track *cv, Row *r)
{
	macroStateApply(&cv->send);

	FOR_ROW_MACROS(i, cv)
	{
		switch (r->macro[i].c)
		{
			case MACRO_SEND:        macroStateSet   (&cv->send, r->macro[i]); break;
			case MACRO_SMOOTH_SEND: macroStateSmooth(&cv->send, r->macro[i]); break;
		}
	}
}
