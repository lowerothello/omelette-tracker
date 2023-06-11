typedef struct MacroFilterState
{
	SVFilter fl[2], fr[2];
	int8_t mode[2], targetmode[2]; /* TODO: should be a (MacroState) */

	MacroState cut;
	MacroState res;
} MacroFilterState;

#define MACRO_FILTER             'F'
#define MACRO_SMOOTH_FILTER      'f'
#define MACRO_RESONANCE          'Z'
#define MACRO_SMOOTH_RESONANCE   'z'
#define MACRO_FILTER_MODE        'M'
#define MACRO_SMOOTH_FILTER_MODE 'm'

void macroFilterClear(Track *cv, void *state)
{
	MacroFilterState *ms = state;

	ms->mode[0] = ms->mode[1] = 0;
	ms->targetmode[0] = ms->targetmode[1] = -1;

	ms->cut.base = ms->cut.rand = 0xff;
	ms->cut.target = -1;
	ms->cut.target_rand = 0;

	ms->res.base = ms->res.rand = 0x00;
	ms->res.target = -1;
	ms->res.target_rand = 0;
}

void macroFilterPostTrig(uint32_t fptr, uint16_t *spr, Track *cv, Row *r, void *state)
{
	MacroFilterState *ms = state;

	macroStateApply(&ms->cut);
	macroStateApply(&ms->res);

	Macro *m;
	FOR_ROW_MACROS(i, cv)
	{
		m = &r->macro[i];
		switch (m->c)
		{
			case MACRO_FILTER:           macroStateSet   (&ms->cut, m); break;
			case MACRO_SMOOTH_FILTER:    macroStateSmooth(&ms->cut, m); break;
			case MACRO_RESONANCE:        macroStateSet   (&ms->res, m); break;
			case MACRO_SMOOTH_RESONANCE: macroStateSmooth(&ms->res, m); break;
			case MACRO_FILTER_MODE:
				ms->mode[0] = (m->v&0x70)>>4; /* ignore the '8' bit */
				ms->mode[1] =  m->v&0x07;     /* ignore the '8' bit */
				break;
			case MACRO_SMOOTH_FILTER_MODE:
				if (ms->targetmode[0] != -1) { ms->mode[0] = ms->targetmode[0]; ms->targetmode[0] = -1; }
				if (ms->targetmode[1] != -1) { ms->mode[1] = ms->targetmode[1]; ms->targetmode[1] = -1; }
				ms->targetmode[0] = (m->v&0x70)>>4; /* ignore the '8' bit */
				ms->targetmode[1] =  m->v&0x07;     /* ignore the '8' bit */
				break;
		}
	}
}

void macroFilterPostSampler(uint32_t fptr, Track *cv, float rp, float *lf, float *rf, void *state)
{
	MacroFilterState *ms = state;

	float cutoff_l = 0.0f, cutoff_r = 0.0f;
	float resonance_l = 0.0f, resonance_r = 0.0f;
	macroStateGetStereo(&ms->cut, rp, &cutoff_l, &cutoff_r);
	macroStateGetStereo(&ms->res, rp, &resonance_l, &resonance_r);

/* #define RUN_FILTER(CHANNEL, MODE, PASS) \
	runSVFilter(&ms->f##CHANNEL[0], *CHANNEL##f, cutoff_##CHANNEL, resonance_##CHANNEL); \
	switch (ms->mode[MODE]&0x3) \
	{ \
		case 0: if (cutoff_##CHANNEL < 1.0f) *CHANNEL##f = ms->f##CHANNEL[PASS].CHANNEL; break; \
	} */

	/* first pass (12dB/oct) */
	runSVFilter(&ms->fl[0], *lf, cutoff_l, resonance_l);
	runSVFilter(&ms->fr[0], *rf, cutoff_r, resonance_r);
	switch (ms->mode[0]&0x3)
	{
		case 0: /* low-pass  */ if (cutoff_l < 1.0f) *lf = ms->fl[0].l; break;
		case 1: /* high-pass */ if (cutoff_l > 0.0f) *lf = ms->fl[0].h; break;
		case 2: /* band-pass */ *lf = ms->fl[0].b; break;
		case 3: /* notch     */ *lf = ms->fl[0].n; break;
	}
	if (ms->targetmode[0] != -1)
		switch (ms->targetmode[0]&0x3)
		{
			case 0: /* low-pass  */ if (cutoff_l < 1.0f) *lf += (ms->fl[0].l - *lf) * rp; break;
			case 1: /* high-pass */ if (cutoff_l > 0.0f) *lf += (ms->fl[0].h - *lf) * rp; break;
			case 2: /* band-pass */ *lf += (ms->fl[0].b - *lf) * rp; break;
			case 3: /* notch     */ *lf += (ms->fl[0].n - *lf) * rp; break;
		}
	switch (ms->mode[1]&0x3)
	{
		case 0: /* low-pass  */ if (cutoff_r < 1.0f) *rf = ms->fr[0].l; break;
		case 1: /* high-pass */ if (cutoff_r > 0.0f) *rf = ms->fr[0].h; break;
		case 2: /* band-pass */ *rf = ms->fr[0].b; break;
		case 3: /* notch     */ *rf = ms->fr[0].n; break;
	}
	if (ms->targetmode[1] != -1)
		switch (ms->targetmode[1]&0x3)
		{
			case 0: /* low-pass  */ if (cutoff_r < 1.0f) *rf += (ms->fr[0].l - *rf) * rp; break;
			case 1: /* high-pass */ if (cutoff_r > 0.0f) *rf += (ms->fr[0].h - *rf) * rp; break;
			case 2: /* band-pass */ *rf += (ms->fr[0].b - *rf) * rp; break;
			case 3: /* notch     */ *rf += (ms->fr[0].n - *rf) * rp; break;
		}

	/* second pass (24dB/oct) */
	runSVFilter(&ms->fl[1], *lf, cutoff_l, resonance_l);
	runSVFilter(&ms->fr[1], *rf, cutoff_r, resonance_r);
	if (ms->mode[0]&0b100) /* if the '4' bit is set */
		switch (ms->mode[0]&0x3)
		{
			case 0: /* low-pass  */ if (cutoff_l < 1.0f) *lf = ms->fl[1].l; break;
			case 1: /* high-pass */ if (cutoff_r > 0.0f) *lf = ms->fl[1].h; break;
			case 2: /* band-pass */ *lf = ms->fl[1].b; break;
			case 3: /* notch     */ *lf = ms->fl[1].n; break;
		}
	if (ms->targetmode[0] != -1 && ms->targetmode[0]&0b100) /* if the '4' bit is set */
		switch (ms->targetmode[0]&0x3)
		{
			case 0: /* low-pass  */ if (cutoff_l < 1.0f) { *lf += (ms->fl[1].l - *lf) * rp; } break;
			case 1: /* high-pass */ if (cutoff_r > 0.0f) { *lf += (ms->fl[1].h - *lf) * rp; } break;
			case 2: /* band-pass */ *lf += (ms->fl[1].b - *lf) * rp; break;
			case 3: /* notch     */ *lf += (ms->fl[1].n - *lf) * rp; break;
		}
	if (ms->mode[1]&0b100) /* if the '4' bit is set */
		switch (ms->mode[1]&0x3)
		{
			case 0: /* low-pass  */ if (cutoff_r < 1.0f) *rf = ms->fr[1].l; break;
			case 1: /* high-pass */ if (cutoff_r > 0.0f) *rf = ms->fr[1].h; break;
			case 2: /* band-pass */ *rf = ms->fr[1].b; break;
			case 3: /* notch     */ *rf = ms->fr[1].n; break;
		}
	if (ms->targetmode[1] != -1 && ms->targetmode[1]&0b100) /* if the '4' bit is set */
		switch (ms->targetmode[1]&0x3)
		{
			case 0: /* low-pass  */ if (cutoff_r < 1.0f) { *rf += (ms->fr[1].l - *rf) * rp; } break;
			case 1: /* high-pass */ if (cutoff_r > 0.0f) { *rf += (ms->fr[1].h - *rf) * rp; } break;
			case 2: /* band-pass */ *rf += (ms->fr[1].b - *rf) * rp; break;
			case 3: /* notch     */ *rf += (ms->fr[1].n - *rf) * rp; break;
		}
}
