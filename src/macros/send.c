#define MACRO_SEND        'S'
#define MACRO_SMOOTH_SEND 's'

void macroSendClear(Track *cv, void *state)
{
	cv->send.base = cv->send.rand = 0x00;
	cv->send.target = -1;
	cv->send.target_rand = 0;
}

void macroSendPreTrig(uint32_t fptr, uint16_t *spr, Track *cv, Row *r, void *state)
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
