#define MACRO_FILTER             'F'
#define MACRO_SMOOTH_FILTER      'f'
#define MACRO_RESONANCE          'Z'
#define MACRO_SMOOTH_RESONANCE   'z'
#define MACRO_FILTER_MODE        'M'
#define MACRO_SMOOTH_FILTER_MODE 'm'

void macroFilterPostTrig(jack_nframes_t fptr, uint16_t *spr, Track *cv, Row *r)
{
	macroStateApply(&cv->filter.cut);
	macroStateApply(&cv->filter.res);

	FOR_ROW_MACROS(i, cv)
		switch (r->macro[i].c)
		{
			case MACRO_FILTER:           macroStateSet   (&cv->filter.cut, r->macro[i]); break;
			case MACRO_SMOOTH_FILTER:    macroStateSmooth(&cv->filter.cut, r->macro[i]); break;
			case MACRO_RESONANCE:        macroStateSet   (&cv->filter.res, r->macro[i]); break;
			case MACRO_SMOOTH_RESONANCE: macroStateSmooth(&cv->filter.res, r->macro[i]); break;
			case MACRO_FILTER_MODE:
				cv->filter.mode[0] = (r->macro[i].v&0x70)>>4; /* ignore the '8' bit */
				cv->filter.mode[1] =  r->macro[i].v&0x07;     /* ignore the '8' bit */
				break;
			case MACRO_SMOOTH_FILTER_MODE:
				if (cv->filter.targetmode[0] != -1) { cv->filter.mode[0] = cv->filter.targetmode[0]; cv->filter.targetmode[0] = -1; }
				if (cv->filter.targetmode[1] != -1) { cv->filter.mode[1] = cv->filter.targetmode[1]; cv->filter.targetmode[1] = -1; }
				cv->filter.targetmode[0] = (r->macro[i].v&0x70)>>4; /* ignore the '8' bit */
				cv->filter.targetmode[1] =  r->macro[i].v&0x07;     /* ignore the '8' bit */
				break;
		}
}

void macroFilterPostSampler(jack_nframes_t fptr, Track *cv, float rp, float *lf, float *rf)
{
	float cutoff_l = 0.0f, cutoff_r = 0.0f;
	float resonance_l = 0.0f, resonance_r = 0.0f;
	macroStateGetStereo(&cv->filter.cut, rp, &cutoff_l, &cutoff_r);
	macroStateGetStereo(&cv->filter.res, rp, &resonance_l, &resonance_r);

/* #define RUN_FILTER(CHANNEL, MODE, PASS) \
	runSVFilter(&cv->filter.f##CHANNEL[0], *CHANNEL##f, cutoff_##CHANNEL, resonance_##CHANNEL); \
	switch (cv->filter.mode[MODE]&0x3) \
	{ \
		case 0: if (cutoff_##CHANNEL < 1.0f) *CHANNEL##f = cv->filter.f##CHANNEL[PASS].CHANNEL; break; \
	} */

	/* first pass (12dB/oct) */
	runSVFilter(&cv->filter.fl[0], *lf, cutoff_l, resonance_l);
	runSVFilter(&cv->filter.fr[0], *rf, cutoff_r, resonance_r);
	switch (cv->filter.mode[0]&0x3)
	{
		case 0: /* low-pass  */ if (cutoff_l < 1.0f) *lf = cv->filter.fl[0].l; break;
		case 1: /* high-pass */ if (cutoff_l > 0.0f) *lf = cv->filter.fl[0].h; break;
		case 2: /* band-pass */ *lf = cv->filter.fl[0].b; break;
		case 3: /* notch     */ *lf = cv->filter.fl[0].n; break;
	}
	if (cv->filter.targetmode[0] != -1)
		switch (cv->filter.targetmode[0]&0x3)
		{
			case 0: /* low-pass  */ if (cutoff_l < 1.0f) *lf += (cv->filter.fl[0].l - *lf) * rp; break;
			case 1: /* high-pass */ if (cutoff_l > 0.0f) *lf += (cv->filter.fl[0].h - *lf) * rp; break;
			case 2: /* band-pass */ *lf += (cv->filter.fl[0].b - *lf) * rp; break;
			case 3: /* notch     */ *lf += (cv->filter.fl[0].n - *lf) * rp; break;
		}
	switch (cv->filter.mode[1]&0x3)
	{
		case 0: /* low-pass  */ if (cutoff_r < 1.0f) *rf = cv->filter.fr[0].l; break;
		case 1: /* high-pass */ if (cutoff_r > 0.0f) *rf = cv->filter.fr[0].h; break;
		case 2: /* band-pass */ *rf = cv->filter.fr[0].b; break;
		case 3: /* notch     */ *rf = cv->filter.fr[0].n; break;
	}
	if (cv->filter.targetmode[1] != -1)
		switch (cv->filter.targetmode[1]&0x3)
		{
			case 0: /* low-pass  */ if (cutoff_r < 1.0f) *rf += (cv->filter.fr[0].l - *rf) * rp; break;
			case 1: /* high-pass */ if (cutoff_r > 0.0f) *rf += (cv->filter.fr[0].h - *rf) * rp; break;
			case 2: /* band-pass */ *rf += (cv->filter.fr[0].b - *rf) * rp; break;
			case 3: /* notch     */ *rf += (cv->filter.fr[0].n - *rf) * rp; break;
		}

	/* second pass (24dB/oct) */
	runSVFilter(&cv->filter.fl[1], *lf, cutoff_l, resonance_l);
	runSVFilter(&cv->filter.fr[1], *rf, cutoff_r, resonance_r);
	if (cv->filter.mode[0]&0b100) /* if the '4' bit is set */
		switch (cv->filter.mode[0]&0x3)
		{
			case 0: /* low-pass  */ if (cutoff_l < 1.0f) *lf = cv->filter.fl[1].l; break;
			case 1: /* high-pass */ if (cutoff_r > 0.0f) *lf = cv->filter.fl[1].h; break;
			case 2: /* band-pass */ *lf = cv->filter.fl[1].b; break;
			case 3: /* notch     */ *lf = cv->filter.fl[1].n; break;
		}
	if (cv->filter.targetmode[0] != -1 && cv->filter.targetmode[0]&0b100) /* if the '4' bit is set */
		switch (cv->filter.targetmode[0]&0x3)
		{
			case 0: /* low-pass  */ if (cutoff_l < 1.0f) { *lf += (cv->filter.fl[1].l - *lf) * rp; } break;
			case 1: /* high-pass */ if (cutoff_r > 0.0f) { *lf += (cv->filter.fl[1].h - *lf) * rp; } break;
			case 2: /* band-pass */ *lf += (cv->filter.fl[1].b - *lf) * rp; break;
			case 3: /* notch     */ *lf += (cv->filter.fl[1].n - *lf) * rp; break;
		}
	if (cv->filter.mode[1]&0b100) /* if the '4' bit is set */
		switch (cv->filter.mode[1]&0x3)
		{
			case 0: /* low-pass  */ if (cutoff_r < 1.0f) *rf = cv->filter.fr[1].l; break;
			case 1: /* high-pass */ if (cutoff_r > 0.0f) *rf = cv->filter.fr[1].h; break;
			case 2: /* band-pass */ *rf = cv->filter.fr[1].b; break;
			case 3: /* notch     */ *rf = cv->filter.fr[1].n; break;
		}
	if (cv->filter.targetmode[1] != -1 && cv->filter.targetmode[1]&0b100) /* if the '4' bit is set */
		switch (cv->filter.targetmode[1]&0x3)
		{
			case 0: /* low-pass  */ if (cutoff_r < 1.0f) { *rf += (cv->filter.fr[1].l - *rf) * rp; } break;
			case 1: /* high-pass */ if (cutoff_r > 0.0f) { *rf += (cv->filter.fr[1].h - *rf) * rp; } break;
			case 2: /* band-pass */ *rf += (cv->filter.fr[1].b - *rf) * rp; break;
			case 3: /* notch     */ *rf += (cv->filter.fr[1].n - *rf) * rp; break;
		}
}
