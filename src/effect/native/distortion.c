enum {
	E_D_ALG_HARDCLIP,
	E_D_ALG_SOFTCLIP,
	E_D_ALG_WAVEFOLD,
	E_D_ALG_WAVEWRAP,
	E_D_ALG_RECTIFY,
	E_D_ALG_SIGNCONV,
} E_D_ALG;

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


void initDistortion(NativeState *state)
{
	((DistortionState *)state->instance)->gainparallel = 0xff;
	((DistortionState *)state->instance)->dcblockcutoff = 1.0f - (130.0f / samplerate);
}

void drawDistortion(NativeState *state, ControlState *cc,
		short x, short w,
		short y, short ymin, short ymax)
{
	DistortionState *s = state->instance;

	short xx;
	ColumnState cs; resetColumn(&cs, w);

	addColumn(&cs, 20);
	addColumn(&cs, 12);

	/* left column */
	xx = x + getNextColumnOffset(&cs);
	if (ymin <= y+0 && ymax >= y+0) printf("\033[%d;%dH \033[1m#   DISTORTION   #\033[m ", y+0, xx);
	if (ymin <= y+1 && ymax >= y+1)
	{
		printf("\033[%d;%dHalgorithm [        ]", y+1, xx);
		addControlInt(cc, xx+11, y+1, &s->algorithm, 1, 0, 5, 0, 8, 6, NULL, NULL);
			addScalePointInt(cc, "HARDCLIP", E_D_ALG_HARDCLIP);
			addScalePointInt(cc, "SOFTCLIP", E_D_ALG_SOFTCLIP);
			addScalePointInt(cc, "WAVEFOLD", E_D_ALG_WAVEFOLD);
			addScalePointInt(cc, "WAVEWRAP", E_D_ALG_WAVEWRAP);
			addScalePointInt(cc, " RECTIFY", E_D_ALG_RECTIFY );
			addScalePointInt(cc, "SIGNCONV", E_D_ALG_SIGNCONV);
	} else addControlDummy(cc, 0, 0);
	if (ymin <= y+2 && ymax >= y+2)
	{
		printf("\033[%d;%dHbias:       [   ][ ]", y+2, xx);
		addControlInt(cc, xx+13, y+2, &s->bias,       3, -128, 127, 0, 0, 0, NULL, NULL);
		addControlInt(cc, xx+18, y+2, &s->biasstereo, 0, 0, 1, 0, 0, 0, NULL, NULL);
	} else { addControlDummy(cc, 0, 0); addControlDummy(cc, 0, 0); }
	if (ymin <= y+3 && ymax >= y+3)
	{
		printf("\033[%d;%dHparallel:  [   ][  ]", y+3, xx);
		addControlInt(cc, xx+12, y+3, &s->filterparallel, 3, -128, 127, 0, 0, 0, NULL, NULL);
		addControlInt(cc, xx+17, y+3, &s->gainparallel,   2, 0x0, 0xff, 0x0, 0, 0, NULL, NULL);
	} else { addControlDummy(cc, 0, 0); addControlDummy(cc, 0, 0); }

	/* right column */
	xx = x + getNextColumnOffset(&cs);
	if (ymin <= y+1 && ymax >= y+1) { printf("\033[%d;%dHdrive:  [  ]", y+1, xx); addControlInt(cc, xx+9, y+1, &s->drive,   2, 0x0, 0xff, 0x0, 0, 0, NULL, NULL); } else addControlDummy(cc, 0, 0);
	if (ymin <= y+2 && ymax >= y+2) { printf("\033[%d;%dHgate:   [  ]", y+2, xx); addControlInt(cc, xx+9, y+2, &s->gate,    2, 0x0, 0xff, 0x0, 0, 0, NULL, NULL); } else addControlDummy(cc, 0, 0);
	if (ymin <= y+3 && ymax >= y+3) { printf("\033[%d;%dHrectify:[  ]", y+3, xx); addControlInt(cc, xx+9, y+3, &s->rectify, 2, 0x0, 0xff, 0x0, 0, 0, NULL, NULL); } else addControlDummy(cc, 0, 0);
}


float wavefolder(float input)
{
	while (input < -1.0f || input > 1.0f)
	{
		if (input >  1.0f) input =  1.0f - input +  1.0f;
		if (input < -1.0f) input = -1.0f - input + -1.0f;
	}
	return input;
}
float wavewrapper(float input, float maxrange)
{
	while (input >  maxrange) input -= maxrange;
	while (input < -maxrange) input += maxrange;
	return input;
}
float signedunsigned(float input)
{
	if (input > 0.0f) return input - 0.0f;
	else              return input + 1.0f;
}
float rectify(float input)
{ return (fabsf(input) * 2.0f) - 1.0f; }
float thirddegreepolynomial(float input)
{ return 1.5f*input - 0.5f*input*input*input; }

#define DRIVE_COEF 1.02214f
void runDistortion(uint32_t samplecount, EffectChain *chain, void **instance)
{
	DistortionState *s = *instance;

	float gain, bias, makeup;
	float affectl, affectr;
	float bypassl, bypassr;

	for (uint32_t fptr = 0; fptr < samplecount; fptr++)
	{
		if (s->filterparallel == 0)
		{
			runSVFilter(&s->filter[0], chain->input[0][fptr], 0.0f, 0.0f);
			runSVFilter(&s->filter[1], chain->input[1][fptr], 0.0f, 0.0f);
			affectl = chain->input[0][fptr];
			affectr = chain->input[1][fptr];
		} else if (s->filterparallel < 0)
		{
			runSVFilter(&s->filter[0], chain->input[0][fptr], 1.0f + s->filterparallel*DIV128, 0.0f);
			runSVFilter(&s->filter[1], chain->input[1][fptr], 1.0f + s->filterparallel*DIV128, 0.0f);
			affectl = s->filter[0].l;
			affectr = s->filter[1].l;
		} else
		{
			runSVFilter(&s->filter[0], chain->input[0][fptr], s->filterparallel*DIV128, 0.0f);
			runSVFilter(&s->filter[1], chain->input[1][fptr], s->filterparallel*DIV128, 0.0f);
			affectl = s->filter[0].h;
			affectr = s->filter[1].h;
		}
		bypassl = chain->input[0][fptr] - affectl;
		bypassr = chain->input[1][fptr] - affectr;

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
		chain->input[0][fptr] = (s->dcblockoutput[0]*makeup + bypassl) * gain + chain->input[0][fptr] * (1.0f - gain);
		chain->input[1][fptr] = (s->dcblockoutput[1]*makeup + bypassr) * gain + chain->input[1][fptr] * (1.0f - gain);
	}
}

NATIVE_Descriptor *distortionDescriptor(void)
{
	NATIVE_Descriptor *ret = calloc(1, sizeof(NATIVE_Descriptor));

	ret->controlc = 8;
	ret->height   = 6;
	ret->instance_size = sizeof(DistortionState);

	const char name[] = "distortion";
	const char author[] = "lib";
	ret->name = malloc((strlen(name)+1) * sizeof(char)); strcpy(ret->name, name);
	ret->author = malloc((strlen(author)+1) * sizeof(char)); strcpy(ret->author, author);

	ret->init = initDistortion;
	ret->draw = drawDistortion;
	ret->run = runDistortion;

	return ret;
}
