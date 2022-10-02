/*
 * bias:
 *   add dc offset to the input signal
 *
 * biasstereo:
 *   invert bias for the right channel
 *
 * drive:
 *   matched input/output gain, distort
 *   more/less without affecting the volume
 *
 * emphasis:
 *   matched input/output bell filters,
 *   distort certain frequencies more/less
 *
 * sidechaincutoff:
 *   dj-style low/high pass to
 *   only distort the highs/lows
 */

#define E_D_ALG_HARDCLIP 0
#define E_D_ALG_SOFTCLIP 1
#define E_D_ALG_WAVEFOLD 2
#define E_D_ALG_WAVEWRAP 5
#define E_D_ALG_HARDGATE 4
typedef struct
{
	int8_t  bias;
	bool    biasstereo;
	int8_t  gain;
	uint8_t tone;
	uint8_t noise;

	uint8_t emphasispitch;
	int8_t  emphasisgain;
	uint8_t emphasiswidth;

	int8_t  parallel;

	uint8_t algorithm;

	SVFilter parallelfilter[2];
} DistortionState;


void initDistortion(Effect *e)
{
	e->state = calloc(1, sizeof(DistortionState));
	((DistortionState *)e->state)->emphasispitch = 0x80;
}

void copyDistortion(Effect *dest, Effect *src) { memcpy(dest->state, src->state, sizeof(DistortionState)); }

void serializeDistortion  (Effect *e, FILE *fp) { fwrite(e->state, sizeof(DistortionState), 1, fp); }
void deserializeDistortion(Effect *e, FILE *fp) { fread (e->state, sizeof(DistortionState), 1, fp); }


const uint8_t distortionControlCount = 10;

short getDistortionHeight(Effect *e, short w)
{
	if      (w > EFFECT_WIDTH_CUTOFF_HUGE) return 4;
	else if (w > EFFECT_WIDTH_CUTOFF_WIDE) return 5;
	else                                   return 7;
}
void drawDistortion(Effect *e, ControlState *cc,
		short x, short w,
		short y, short ymin, short ymax)
{
	DistortionState *s = (DistortionState *)e->state;
	short xx;
	ColumnState cs; resetColumn(&cs, w);

	if (w > EFFECT_WIDTH_CUTOFF_HUGE)
	{
		addColumn(&cs, 75);
		addColumn(&cs, 50);

		/* left column */
		xx = x + getNextColumnOffset(&cs);
		if (ymin <= y+0 && ymax >= y+0) printf("\033[%d;%dH \033[1m#                              DISTORTION                               #\033[m ", y+0, xx);
		if (ymin <= y+1 && ymax >= y+1)
		{
			printf("\033[%d;%dHgain/tone: [   ][  ]  algorithm [        ]  bias: [   ][ ]  parallel: [   ]", y+1, xx);
			addControl(cc, xx+12, y+1, &s->gain, 3, 0, 0, 0);
			addControl(cc, xx+17, y+1, &s->tone, 2, 0x0, 0xff, 0);
			addControl(cc, xx+33, y+1, &s->algorithm, 1, 0, 4, 8);
				setControlPrettyName(cc, "HARDCLIP");
				setControlPrettyName(cc, "SOFTCLIP");
				setControlPrettyName(cc, "WAVEFOLD");
				setControlPrettyName(cc, "WAVEWRAP");
				setControlPrettyName(cc, "HARDGATE");
			addControl(cc, xx+51, y+1, &s->bias, 3, 0, 0, 0);
			addControl(cc, xx+56, y+1, &s->biasstereo, 0, 0, 1, 0);
			addControl(cc, xx+71, y+1, &s->parallel, 3, 0, 0, 0);
		} else
		{
			addControl(cc, 0, 0, NULL, 0, 0, 0, 0);
			addControl(cc, 0, 0, NULL, 0, 0, 0, 0);
			addControl(cc, 0, 0, NULL, 0, 0, 0, 0);
			addControl(cc, 0, 0, NULL, 0, 0, 0, 0);
			addControl(cc, 0, 0, NULL, 0, 0, 0, 0);
			addControl(cc, 0, 0, NULL, 0, 0, 0, 0);
		}

		/* right column */
		xx = x + getNextColumnOffset(&cs);
		if (ymin <= y+0 && ymax >= y+0) printf("\033[%d;%dH +                   emphasis                   + ", y+0, xx);
		if (ymin <= y+1 && ymax >= y+1)
		{
			printf("\033[%d;%dHpitch: [  ]  gain: [   ]  width: [00]  noise: [00]", y+1, xx);
			addControl(cc, xx+8,  y+1, &s->emphasispitch, 2, 0x0, 0xff, 0);
			addControl(cc, xx+20, y+1, &s->emphasisgain, 3, 0, 0, 0);
			addControl(cc, xx+34, y+1, &s->emphasiswidth, 2, 0x0, 0xff, 0);
			addControl(cc, xx+47, y+1, &s->noise, 2, 0x0, 0xff, 0);
		} else
		{
			addControl(cc, 0, 0, NULL, 0, 0, 0, 0);
			addControl(cc, 0, 0, NULL, 0, 0, 0, 0);
			addControl(cc, 0, 0, NULL, 0, 0, 0, 0);
			addControl(cc, 0, 0, NULL, 0, 0, 0, 0);
		}
	} else if (w > EFFECT_WIDTH_CUTOFF_WIDE)
	{
		addColumn(&cs, 37);
		addColumn(&cs, 24);

		/* left column */
		xx = x + getNextColumnOffset(&cs);
		if (ymin <= y+0 && ymax >= y+0) printf("\033[%d;%dH \033[1m#            DISTORTION           #\033[m ", y+0, xx);
		if (ymin <= y+1 && ymax >= y+1)
		{
			printf("\033[%d;%dHgain/tone: [   ][  ]  bias:  [   ][ ]", y+1, xx);
			addControl(cc, xx+12, y+1, &s->gain, 3, 0, 0, 0);
			addControl(cc, xx+17, y+1, &s->tone, 2, 0x0, 0xff, 0);
		} else
		{
			addControl(cc, 0, 0, NULL, 0, 0, 0, 0);
			addControl(cc, 0, 0, NULL, 0, 0, 0, 0);
		}
		if (ymin <= y+2 && ymax >= y+2)
		{
			printf("\033[%d;%dHalgorithm [        ]  parallel: [   ]", y+2, xx);
			addControl(cc, xx+11, y+2, &s->algorithm, 1, 0, 4, 8);
				setControlPrettyName(cc, "HARDCLIP");
				setControlPrettyName(cc, "SOFTCLIP");
				setControlPrettyName(cc, "WAVEFOLD");
				setControlPrettyName(cc, "WAVEWRAP");
				setControlPrettyName(cc, "HARDGATE");
		} else addControl(cc, 0, 0, NULL, 0, 0, 0, 0);
		if (ymin <= y+1 && ymax >= y+1)
		{
			addControl(cc, xx+30, y+1, &s->bias, 3, 0, 0, 0);
			addControl(cc, xx+35, y+1, &s->biasstereo, 0, 0, 1, 0);
		} else
		{
			addControl(cc, 0, 0, NULL, 0, 0, 0, 0);
			addControl(cc, 0, 0, NULL, 0, 0, 0, 0);
		}
		if (ymin <= y+2 && ymax >= y+2)
			addControl(cc, xx+33, y+2, &s->parallel, 3, 0, 0, 0);
		else addControl(cc, 0, 0, NULL, 0, 0, 0, 0);

		/* right column */
		xx = x + getNextColumnOffset(&cs);
		if (ymin <= y+0 && ymax >= y+0) printf("\033[%d;%dH +      emphasis      + ", y+0, xx);
		if (ymin <= y+1 && ymax >= y+1) printf("\033[%d;%dHpitch: [  ]  width: [  ]", y+1, xx);
		if (ymin <= y+2 && ymax >= y+2) printf("\033[%d;%dHgain: [   ]  noise: [  ]", y+2, xx);
		if (ymin <= y+1 && ymax >= y+1) addControl(cc, xx+8,  y+1, &s->emphasispitch, 2, 0x0, 0xff, 0); else addControl(cc, 0, 0, NULL, 0, 0, 0, 0);
		if (ymin <= y+2 && ymax >= y+2) addControl(cc, xx+7,  y+2, &s->emphasisgain,  3, 0,   0,    0); else addControl(cc, 0, 0, NULL, 0, 0, 0, 0);
		if (ymin <= y+1 && ymax >= y+1) addControl(cc, xx+21, y+1, &s->emphasiswidth, 2, 0x0, 0xff, 0); else addControl(cc, 0, 0, NULL, 0, 0, 0, 0);
		if (ymin <= y+2 && ymax >= y+2) addControl(cc, xx+21, y+2, &s->noise,         2, 0x0, 0xff, 0); else addControl(cc, 0, 0, NULL, 0, 0, 0, 0);
	} else
	{
		addColumn(&cs, 20);
		addColumn(&cs, 12);

		/* left column */
		xx = x + getNextColumnOffset(&cs);
		if (ymin <= y+0 && ymax >= y+0) printf("\033[%d;%dH \033[1m#   DISTORTION   #\033[m ", y+0, xx);
		if (ymin <= y+1 && ymax >= y+1)
		{
			printf("\033[%d;%dHgain/tone: [   ][  ]", y+1, xx);
			addControl(cc, xx+12, y+1, &s->gain, 3, 0, 0, 0);
			addControl(cc, xx+17, y+1, &s->tone, 2, 0x0, 0xff, 0);
		} else
		{
			addControl(cc, 0, 0, NULL, 0, 0, 0, 0);
			addControl(cc, 0, 0, NULL, 0, 0, 0, 0);
		}
		if (ymin <= y+2 && ymax >= y+2)
		{
			printf("\033[%d;%dHalgorithm [        ]", y+2, xx);
			addControl(cc, xx+11, y+2, &s->algorithm, 1, 0, 4, 8);
				setControlPrettyName(cc, "HARDCLIP");
				setControlPrettyName(cc, "SOFTCLIP");
				setControlPrettyName(cc, "WAVEFOLD");
				setControlPrettyName(cc, "WAVEWRAP");
				setControlPrettyName(cc, "HARDGATE");
		} else addControl(cc, 0, 0, NULL, 0, 0, 0, 0);
		if (ymin <= y+3 && ymax >= y+3)
		{
			printf("\033[%d;%dHbias:       [   ][ ]", y+3, xx);
			addControl(cc, xx+13, y+3, &s->bias,      3, 0, 0, 0);
			addControl(cc, xx+18, y+3, &s->biasstereo, 0, 0, 1, 0);
		} else
		{
			addControl(cc, 0, 0, NULL, 0, 0, 0, 0);
			addControl(cc, 0, 0, NULL, 0, 0, 0, 0);
		}
		if (ymin <= y+4 && ymax >= y+4)
		{
			printf("\033[%d;%dHparallel:      [   ]", y+4, xx);
			addControl(cc, xx+16, y+4, &s->parallel,  3, 0, 0, 0);
		} else addControl(cc, 0, 0, NULL, 0, 0, 0, 0);

		/* right column */
		xx = x + getNextColumnOffset(&cs);
		if (ymin <= y+0 && ymax >= y+0)   printf("\033[%d;%dH+ emphasis +", y+0, xx);
		if (ymin <= y+1 && ymax >= y+1) { printf("\033[%d;%dHpitch:  [  ]", y+1, xx); addControl(cc, xx+9, y+1, &s->emphasispitch, 2, 0x0, 0xff, 0); } else addControl(cc, 0, 0, NULL, 0, 0, 0, 0);
		if (ymin <= y+2 && ymax >= y+2) { printf("\033[%d;%dHgain:  [   ]", y+2, xx); addControl(cc, xx+8, y+2, &s->emphasisgain,  3, 0,   0,    0); } else addControl(cc, 0, 0, NULL, 0, 0, 0, 0);
		if (ymin <= y+3 && ymax >= y+3) { printf("\033[%d;%dHwidth:  [  ]", y+3, xx); addControl(cc, xx+9, y+3, &s->emphasiswidth, 2, 0x0, 0xff, 0); } else addControl(cc, 0, 0, NULL, 0, 0, 0, 0);
		if (ymin <= y+4 && ymax >= y+4) { printf("\033[%d;%dHnoise:  [  ]", y+4, xx); addControl(cc, xx+9, y+4, &s->noise,         2, 0x0, 0xff, 0); } else addControl(cc, 0, 0, NULL, 0, 0, 0, 0);
	}
}

#define DRIVE_COEF 1.02214f
void stepDistortion(Effect *e, float *l, float *r)
{
	DistortionState *s = (DistortionState *)&e->state;
	float affectl = 0.0f;
	float affectr = 0.0f;
	float bypassl = 0.0f;
	float bypassr = 0.0f;

	runSVFilter(&s->parallelfilter[0], *l, fabs(s->parallel*DIV128), 0.0);
	runSVFilter(&s->parallelfilter[1], *l, fabs(s->parallel*DIV128), 0.0);

	if (s->parallel == 0)
	{
		affectl = *l;
		affectr = *r;
	} else if (s->parallel < 0)
	{
		affectl = s->parallelfilter[0].l;
		affectr = s->parallelfilter[1].l;
		bypassl = s->parallelfilter[0].h;
		bypassr = s->parallelfilter[1].h;
	} else
	{
		affectl = s->parallelfilter[0].h;
		affectr = s->parallelfilter[1].h;
		bypassl = s->parallelfilter[0].l;
		bypassr = s->parallelfilter[1].l;
	}

	float a, b; /* registers */

	/* bias */
	b = s->bias*DIV128;
	if (s->biasstereo)
	{
		affectl += b;
		affectr -= b;
	} else
	{
		affectl += b;
		affectr += b;
	}

	/* drive */
	a = powf(DRIVE_COEF, s->gain);
	b = 1.0f / sqrtf(a);

	switch (s->algorithm)
	{
		case E_D_ALG_HARDCLIP:
			affectl = hardclip(affectl*a)*b;
			affectr = hardclip(affectr*a)*b;
			break;
		case E_D_ALG_SOFTCLIP:
			affectl = thirddegreepolynomial(affectl*a)*b;
			affectr = thirddegreepolynomial(affectr*a)*b;
			break;
		case E_D_ALG_WAVEFOLD:
			affectl = wavefolder(affectl*a)*b;
			affectr = wavefolder(affectr*a)*b;
			break;
		case E_D_ALG_WAVEWRAP:
			affectl = wavewrapper(affectl*a)*b;
			affectr = wavewrapper(affectr*a)*b;
			break;
		case E_D_ALG_HARDGATE: /* doesn't use the a/b registers */
			if (s->gain < 0)
			{
				if (fabsf(affectl) < (-s->gain)*DIV128) affectl = 0.0f;
				if (fabsf(affectr) < (-s->gain)*DIV128) affectr = 0.0f;
			} else
			{
				if (fabsf(affectl) < s->gain*DIV128) affectl = 0.0f;
				if (fabsf(affectr) < s->gain*DIV128) affectr = 0.0f;
			}
			break;
	}

	*l = affectl * bypassl;
	*r = affectr * bypassr;
}
