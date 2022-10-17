const uint8_t distortionControlCount = 9;

#define E_D_ALG_HARDCLIP 0
#define E_D_ALG_SOFTCLIP 1
#define E_D_ALG_WAVEFOLD 2
#define E_D_ALG_WAVEWRAP 5
#define E_D_ALG_RECTIFY  6
#define E_D_ALG_SIGNCONV 7
typedef struct
{
	int8_t   bias;
	bool     biasstereo;
	uint8_t  drive;
	uint8_t  gate;
	uint8_t  rectify;

	int8_t   filterparallel;
	uint8_t  gainparallel;

	int8_t   algorithm;

	SVFilter filter[2];
	float    dcblockinput[2];
	float    dcblockoutput[2];
	float    dcblockcutoff;
} DistortionState;


void initDistortion(Effect *e)
{
	e->state = calloc(1, sizeof(DistortionState));
	((DistortionState *)e->state)->gainparallel = 0xff;
	((DistortionState *)e->state)->dcblockcutoff = 1.0f - (130.0f / samplerate);
}

void copyDistortion(Effect *dest, Effect *src)
{
	if (dest->state) free(dest->state);
	dest->type = src->type;
	initDistortion(dest);
	memcpy(dest->state, src->state, sizeof(DistortionState));
}

void serializeDistortion(Effect *e, FILE *fp)
{
	fwrite(e->state, sizeof(DistortionState), 1, fp);
}
void deserializeDistortion(Effect *e, FILE *fp)
{
	initDistortion(e);
	fread(e->state, sizeof(DistortionState), 1, fp);
}


short getDistortionHeight(Effect *e, short w)
{
	/* if      (w > EFFECT_WIDTH_CUTOFF_HUGE) return 4;
	else if (w > EFFECT_WIDTH_CUTOFF_WIDE) return 5;
	else                                   return 7; */
	return 6;
}
void drawDistortion(Effect *e, ControlState *cc,
		short x, short w,
		short y, short ymin, short ymax)
{
	DistortionState *s = (DistortionState *)e->state;
	short xx;
	ColumnState cs; resetColumn(&cs, w);

	/* if (w > EFFECT_WIDTH_CUTOFF_HUGE)
	{
		addColumn(&cs, 75);
		addColumn(&cs, 50);

		// left column
		xx = x + getNextColumnOffset(&cs);
		if (ymin <= y+0 && ymax >= y+0) printf("\033[%d;%dH \033[1m#                              DISTORTION                               #\033[m ", y+0, xx);
		if (ymin <= y+1 && ymax >= y+1)
		{
			printf("\033[%d;%dHdrive:          [  ]  algorithm [        ]  bias: [   ][ ]  parallel: [   ]", y+1, xx);
			addControl(cc, xx+17, y+1, &s->drive, 2, 0x0, 0xff, 0x0, 0, NULL, NULL);
			addControl(cc, xx+33, y+1, &s->algorithm, 1, 0, 5, 0, 8, NULL, NULL);
				setControlPrettyName(cc, "HARDCLIP");
				setControlPrettyName(cc, "SOFTCLIP");
				setControlPrettyName(cc, "WAVEFOLD");
				setControlPrettyName(cc, "WAVEWRAP");
				setControlPrettyName(cc, " RECTIFY");
				setControlPrettyName(cc, "SIGNCONV");
			addControl(cc, xx+51, y+1, &s->bias, 3, -128, 127, 0, 0, NULL, NULL);
			addControl(cc, xx+56, y+1, &s->biasstereo, 0, 0, 1, 0, 0, NULL, NULL);
			addControl(cc, xx+71, y+1, &s->filterparallel, 3, -128, 127, 0, 0, NULL, NULL);
		} else
		{
			addControl(cc, 0, 0, NULL, 0, 0, 0, 0, 0, NULL, NULL);
			addControl(cc, 0, 0, NULL, 0, 0, 0, 0, 0, NULL, NULL);
			addControl(cc, 0, 0, NULL, 0, 0, 0, 0, 0, NULL, NULL);
			addControl(cc, 0, 0, NULL, 0, 0, 0, 0, 0, NULL, NULL);
			addControl(cc, 0, 0, NULL, 0, 0, 0, 0, 0, NULL, NULL);
		}

		// right column
		xx = x + getNextColumnOffset(&cs);
		if (ymin <= y+0 && ymax >= y+0) printf("\033[%d;%dH +                   emphasis                   + ", y+0, xx);
		if (ymin <= y+1 && ymax >= y+1)
		{
			printf("\033[%d;%dHpitch: [  ]  gain: [   ]  width: [00]  gate:  [00]", y+1, xx);
			addControl(cc, xx+8,  y+1, &s->emphasispitch, 2, 0x0, 0xff, 0x0, 0, NULL, NULL);
			addControl(cc, xx+20, y+1, &s->emphasisgain, 3, -128, 127, 0, 0, NULL, NULL);
			addControl(cc, xx+34, y+1, &s->emphasiswidth, 2, 0x0, 0xff, 0x0, 0, NULL, NULL);
			addControl(cc, xx+47, y+1, &s->gate, 2, 0x0, 0xff, 0x0, 0, NULL, NULL);
		} else
		{
			addControl(cc, 0, 0, NULL, 0, 0, 0, 0, 0, NULL, NULL);
			addControl(cc, 0, 0, NULL, 0, 0, 0, 0, 0, NULL, NULL);
			addControl(cc, 0, 0, NULL, 0, 0, 0, 0, 0, NULL, NULL);
			addControl(cc, 0, 0, NULL, 0, 0, 0, 0, 0, NULL, NULL);
		}
	} else if (w > EFFECT_WIDTH_CUTOFF_WIDE)
	{
		addColumn(&cs, 37);
		addColumn(&cs, 24);

		// left column
		xx = x + getNextColumnOffset(&cs);
		if (ymin <= y+0 && ymax >= y+0) printf("\033[%d;%dH \033[1m#            DISTORTION           #\033[m ", y+0, xx);
		if (ymin <= y+1 && ymax >= y+1)
		{
			printf("\033[%d;%dHdrive:          [  ]  bias:  [   ][ ]", y+1, xx);
			addControl(cc, xx+17, y+1, &s->drive, 2, 0x0, 0xff, 0x0, 0, NULL, NULL);
		} else
		{
			addControl(cc, 0, 0, NULL, 0, 0, 0, 0, 0, NULL, NULL);
		}
		if (ymin <= y+2 && ymax >= y+2)
		{
			printf("\033[%d;%dHalgorithm [        ]  parallel: [   ]", y+2, xx);
			addControl(cc, xx+11, y+2, &s->algorithm, 1, 0, 5, 0, 8, NULL, NULL);
				setControlPrettyName(cc, "HARDCLIP");
				setControlPrettyName(cc, "SOFTCLIP");
				setControlPrettyName(cc, "WAVEFOLD");
				setControlPrettyName(cc, "WAVEWRAP");
				setControlPrettyName(cc, " RECTIFY");
				setControlPrettyName(cc, "SIGNCONV");
		} else addControl(cc, 0, 0, NULL, 0, 0, 0, 0, 0, NULL, NULL);
		if (ymin <= y+1 && ymax >= y+1)
		{
			addControl(cc, xx+30, y+1, &s->bias, 3, -128, 127, 0, 0, NULL, NULL);
			addControl(cc, xx+35, y+1, &s->biasstereo, 0, 0, 1, 0, 0, NULL, NULL);
		} else
		{
			addControl(cc, 0, 0, NULL, 0, 0, 0, 0, 0, NULL, NULL);
			addControl(cc, 0, 0, NULL, 0, 0, 0, 0, 0, NULL, NULL);
		}
		if (ymin <= y+2 && ymax >= y+2)
			addControl(cc, xx+33, y+2, &s->filterparallel, 3, -128, 127, 0, 0, NULL, NULL);
		else addControl(cc, 0, 0, NULL, 0, 0, 0, 0, 0, NULL, NULL);

		// right column
		xx = x + getNextColumnOffset(&cs);
		if (ymin <= y+0 && ymax >= y+0) printf("\033[%d;%dH +      emphasis      + ", y+0, xx);
		if (ymin <= y+1 && ymax >= y+1) printf("\033[%d;%dHpitch: [  ]  width: [  ]", y+1, xx);
		if (ymin <= y+2 && ymax >= y+2) printf("\033[%d;%dHgain: [   ]  gate:  [  ]", y+2, xx);
		if (ymin <= y+1 && ymax >= y+1) addControl(cc, xx+8,  y+1, &s->emphasispitch, 2, 0x0, 0xff, 0x0, 0, NULL, NULL); else addControl(cc, 0, 0, NULL, 0, 0, 0, 0, 0, NULL, NULL);
		if (ymin <= y+2 && ymax >= y+2) addControl(cc, xx+7,  y+2, &s->emphasisgain,  3,-128, 127,  0,   0, NULL, NULL); else addControl(cc, 0, 0, NULL, 0, 0, 0, 0, 0, NULL, NULL);
		if (ymin <= y+1 && ymax >= y+1) addControl(cc, xx+21, y+1, &s->emphasiswidth, 2, 0x0, 0xff, 0x0, 0, NULL, NULL); else addControl(cc, 0, 0, NULL, 0, 0, 0, 0, 0, NULL, NULL);
		if (ymin <= y+2 && ymax >= y+2) addControl(cc, xx+21, y+2, &s->gate,          2, 0x0, 0xff, 0x0, 0, NULL, NULL); else addControl(cc, 0, 0, NULL, 0, 0, 0, 0, 0, NULL, NULL);
	} else */
	{
		addColumn(&cs, 20);
		addColumn(&cs, 12);

		/* left column */
		xx = x + getNextColumnOffset(&cs);
		if (ymin <= y+0 && ymax >= y+0) printf("\033[%d;%dH \033[1m#   DISTORTION   #\033[m ", y+0, xx);
		if (ymin <= y+1 && ymax >= y+1)
		{
			printf("\033[%d;%dHalgorithm [        ]", y+1, xx);
			addControl(cc, xx+11, y+1, &s->algorithm, 1, 0, 5, 0, 8, NULL, NULL);
				setControlPrettyName(cc, "HARDCLIP");
				setControlPrettyName(cc, "SOFTCLIP");
				setControlPrettyName(cc, "WAVEFOLD");
				setControlPrettyName(cc, "WAVEWRAP");
				setControlPrettyName(cc, " RECTIFY");
				setControlPrettyName(cc, "SIGNCONV");
		} else addControl(cc, 0, 0, NULL, 0, 0, 0, 0, 0, NULL, NULL);
		if (ymin <= y+2 && ymax >= y+2)
		{
			printf("\033[%d;%dHbias:       [   ][ ]", y+2, xx);
			addControl(cc, xx+13, y+2, &s->bias,       3, -128, 127, 0, 0, NULL, NULL);
			addControl(cc, xx+18, y+2, &s->biasstereo, 0, 0, 1, 0, 0, NULL, NULL);
		} else { addControl(cc, 0, 0, NULL, 0, 0, 0, 0, 0, NULL, NULL); addControl(cc, 0, 0, NULL, 0, 0, 0, 0, 0, NULL, NULL); }
		if (ymin <= y+3 && ymax >= y+3)
		{
			printf("\033[%d;%dHparallel:  [   ][  ]", y+3, xx);
			addControl(cc, xx+12, y+3, &s->filterparallel, 3, -128, 127, 0, 0, NULL, NULL);
			addControl(cc, xx+17, y+3, &s->gainparallel,   2, 0x0, 0xff, 0x0, 0, NULL, NULL);
		} else { addControl(cc, 0, 0, NULL, 0, 0, 0, 0, 0, NULL, NULL); addControl(cc, 0, 0, NULL, 0, 0, 0, 0, 0, NULL, NULL); }

		/* right column */
		xx = x + getNextColumnOffset(&cs);
		if (ymin <= y+1 && ymax >= y+1) { printf("\033[%d;%dHdrive:  [  ]", y+1, xx); addControl(cc, xx+9, y+1, &s->drive,   2, 0x0, 0xff, 0x0, 0, NULL, NULL); } else addControl(cc, 0, 0, NULL, 0, 0, 0, 0, 0, NULL, NULL);
		if (ymin <= y+2 && ymax >= y+2) { printf("\033[%d;%dHgate:   [  ]", y+2, xx); addControl(cc, xx+9, y+2, &s->gate,    2, 0x0, 0xff, 0x0, 0, NULL, NULL); } else addControl(cc, 0, 0, NULL, 0, 0, 0, 0, 0, NULL, NULL);
		if (ymin <= y+3 && ymax >= y+3) { printf("\033[%d;%dHrectify:[  ]", y+3, xx); addControl(cc, xx+9, y+3, &s->rectify, 2, 0x0, 0xff, 0x0, 0, NULL, NULL); } else addControl(cc, 0, 0, NULL, 0, 0, 0, 0, 0, NULL, NULL);
	}
}

#define DRIVE_COEF 1.02214f
void runDistortion(uint32_t samplecount, Effect *e)
{
	float gain, bias, makeup;
	DistortionState *s = e->state;
	float affectl, affectr;
	float bypassl, bypassr;

	for (uint32_t fptr = 0; fptr < samplecount; fptr++)
	{
		if (s->filterparallel == 0)
		{
			runSVFilter(&s->filter[0], e->input[0][fptr], 0.0f, 0.0f);
			runSVFilter(&s->filter[1], e->input[1][fptr], 0.0f, 0.0f);
			affectl = e->input[0][fptr];
			affectr = e->input[1][fptr];
		} else if (s->filterparallel < 0)
		{
			runSVFilter(&s->filter[0], e->input[0][fptr], 1.0f + s->filterparallel*DIV128, 0.0f);
			runSVFilter(&s->filter[1], e->input[1][fptr], 1.0f + s->filterparallel*DIV128, 0.0f);
			affectl = s->filter[0].l;
			affectr = s->filter[1].l;
		} else
		{
			runSVFilter(&s->filter[0], e->input[0][fptr], s->filterparallel*DIV128, 0.0f);
			runSVFilter(&s->filter[1], e->input[1][fptr], s->filterparallel*DIV128, 0.0f);
			affectl = s->filter[0].h;
			affectr = s->filter[1].h;
		}
		bypassl = e->input[0][fptr] - affectl;
		bypassr = e->input[1][fptr] - affectr;

		gain = s->rectify*DIV255;
		if (fabsf(affectl) < s->gate*DIV512) affectl = 0.0f;
		else                                 affectl = rectify(affectl)*gain + affectl*(1.0f-gain);
		if (fabsf(affectr) < s->gate*DIV512) affectr = 0.0f;
		else                                 affectr = rectify(affectr)*gain + affectr*(1.0f-gain);

		/* pre gain */
		gain = powf(DRIVE_COEF, s->drive);
		makeup = (1.0f / sqrtf(gain)) * 2.0f;

		affectl *= gain;
		affectr *= gain;

		/* bias */
		bias = s->bias*DIV128;
		if (s->biasstereo) { affectl += bias;
							 affectr -= bias; }
		else               { affectl += bias;
							 affectr += bias; }

		/* distortion stage */
		switch (s->algorithm)
		{
			case E_D_ALG_HARDCLIP: break;
			case E_D_ALG_SOFTCLIP:
				affectl = thirddegreepolynomial(affectl);
				affectr = thirddegreepolynomial(affectr);
				break;
			case E_D_ALG_WAVEFOLD:
				affectl = wavefolder(affectl);
				affectr = wavefolder(affectr);
				break;
			case E_D_ALG_WAVEWRAP: /* TODO: does nothing? */
				affectl = wavewrapper(affectl, 0.5f);
				affectr = wavewrapper(affectr, 0.5f);
				break;
			case E_D_ALG_RECTIFY: /* TODO: mix control instead of input gain? */
				affectl = rectify(affectl);
				affectr = rectify(affectr);
				break;
			case E_D_ALG_SIGNCONV:
				affectl = signedunsigned(affectl);
				affectr = signedunsigned(affectr);
				break;
		}

		affectl = hardclip(affectl);
		affectr = hardclip(affectr);

		s->dcblockoutput[0] = affectl - s->dcblockinput[0] + s->dcblockcutoff * s->dcblockoutput[0];
		s->dcblockoutput[1] = affectr - s->dcblockinput[1] + s->dcblockcutoff * s->dcblockoutput[1];
		s->dcblockinput[0] = affectl;
		s->dcblockinput[1] = affectr;

		/* final mix */
		gain = s->gainparallel*DIV255;
		e->input[0][fptr] = (s->dcblockoutput[0]*makeup + bypassl) * gain + e->input[0][fptr] * (1.0f - gain);
		e->input[1][fptr] = (s->dcblockoutput[1]*makeup + bypassr) * gain + e->input[1][fptr] * (1.0f - gain);
	}
}
