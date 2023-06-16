#define MACRO_GAIN        'G'
#define MACRO_SMOOTH_GAIN 'g'

void macroGainClear(Track *cv, void *state)
{
	MacroState *s = state;
	s->base = s->rand = 0x88;
	s->target = -1;
	s->target_rand = 0;
}

void macroGainPreTrig(uint32_t fptr, uint16_t *spr, Track *cv, Row *r, void *state)
{
	macroStateApply(state);

	Macro *m;
	FOR_ROW_MACROS(i, cv)
	{
		m = &r->macro[i];
		switch (m->c)
		{
			case MACRO_GAIN: macroStateSet(state, m); break;
			case MACRO_SMOOTH_GAIN: macroStateSmooth(state, m); break;
		}
	}
}

void macroGainPostSampler(uint32_t fptr, Track *cv, float rp, float *lf, float *rf, void *state)
{
	float lm = 0.0f, rm = 0.0f;
	macroStateGetStereo(state, rp, &lm, &rm);
	*lf *= lm;
	*rf *= rm;
}
