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
	int8_t  drive;

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

/* returns the height of the widget */
int drawDistortion(Effect *e, ControlState *cc,
		short x, short y, short w, short ymin, short ymax)
{
	DistortionState *s = (DistortionState *)&e->state;
	if (ymin <= y+0 && ymax >= y+0) printf("\033[%d;%dH\033[1mDISTORTION\033[m", y+0, x+3);
	if (ymin <= y+1 && ymax >= y+1)
	{
		printf("\033[%d;%dHdrive:      [   ]", y+1, x+0);
		addControl(cc, x+14, y+1, &s->drive, 3, -128, 127, 0);
	} else
		addControl(cc, 0, 0, NULL, 0, 0, 0, 0);

	if (ymin <= y+2 && ymax >= y+2)
	{
		printf("\033[%d;%dHbias:    [   ][ ]", y+2, x+0);
		addControl(cc, x+11, y+2, &s->bias,       3, -128, 127, 0);
		addControl(cc, x+16, y+2, &s->biasstereo, 1, 0,    1,   0);
	} else
	{
		addControl(cc, 0, 0, NULL, 0, 0, 0, 0);
		addControl(cc, 0, 0, NULL, 0, 0, 0, 0);
	}

	if (ymin <= y+3 && ymax >= y+3)
	{
		printf("\033[%d;%dHparallel:   [   ]", y+3, x+0);
		addControl(cc, x+14, y+3, &s->parallel, 3, -128, 127, 0);
	} else
		addControl(cc, 0, 0, NULL, 0, 0, 0, 0);

	if (ymin <= y+0 && ymax >= y+0) printf("\033[%d;%dH+  emphasis  +", y+0, x+w-13);
	if (ymin <= y+1 && ymax >= y+1)
	{
		printf("\033[%d;%dHpitch:    [  ]", y+1, x+w-13);
		addControl(cc, x+14, y+1, &s->emphasispitch, 2, 0, 255, 0);
	} else
		addControl(cc, 0, 0, NULL, 0, 0, 0, 0);

	if (ymin <= y+2 && ymax >= y+2)
	{
		printf("\033[%d;%dHgain:    [   ]", y+2, x+w-13);
		addControl(cc, x+11, y+2, &s->emphasisgain, 3, -128, 127, 0);
	} else
		addControl(cc, 0, 0, NULL, 0, 0, 0, 0);

	if (ymin <= y+3 && ymax >= y+3)
	{
		printf("\033[%d;%dHwidth:    [  ]", y+3, x+w-13);
		addControl(cc, x+14, y+3, &s->emphasiswidth, 2, 0, 255, 0);
	} else
		addControl(cc, 0, 0, NULL, 0, 0, 0, 0);

	if (ymin <= y+4 && ymax >= y+4)
	{
		printf("\033[%d;%dHalgorithm [        ]", y+4, (x+w-20)>>1);
		addControl(cc, ((x+w-20)>>1) + 11, y+4, &s->algorithm, 1, 0, 5, 8);
			setControlPrettyName(cc, "HARDCLIP");
			setControlPrettyName(cc, "SOFTCLIP");
			setControlPrettyName(cc, "WAVEFOLD");
			setControlPrettyName(cc, "WAVEWRAP");
			setControlPrettyName(cc, "HARDGATE");
	} else
		addControl(cc, 0, 0, NULL, 0, 0, 0, 0);

	return 5;
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
	a = powf(DRIVE_COEF, s->drive);
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
			if (s->drive < 0)
			{
				if (fabsf(affectl) < (-s->drive)*DIV128) affectl = 0.0f;
				if (fabsf(affectr) < (-s->drive)*DIV128) affectr = 0.0f;
			} else
			{
				if (fabsf(affectl) < s->drive*DIV128) affectl = 0.0f;
				if (fabsf(affectr) < s->drive*DIV128) affectr = 0.0f;
			}
			break;
	}

	*l = affectl * bypassl;
	*r = affectr * bypassr;
}
