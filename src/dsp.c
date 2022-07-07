#define WAVE_COUNT 4
#define FILTERTYPE_COUNT 1

#define C5_FREQ 261.63 /* set this to the resonant frequency of your favourite rock for best results */

#define FM_DEPTH 0.005

/* <seconds> */
#define ENVELOPE_ATTACK 0.005
#define ENVELOPE_ATTACK_MIN 3
#define ENVELOPE_DECAY 0.020
#define ENVELOPE_DECAY_MIN 1
#define ENVELOPE_RELEASE 0.020
#define ENVELOPE_RELEASE_MIN 1
#define LFO_MIN 2.00
#define LFO_MAX 0.001
/* </seconds> */

/* the slowest the gate will move in a second */
#define MIN_GATE_SPEED_SEC 10.0


/* not related to the gate macro */
/* the threshold where processing is no longer necessary */
#define NOISE_GATE 0.000001


/* multiply instead of dividing for float powers of 2 */
#define DIV256 0.00390625
#define DIV255 0.00392156862745098
#define DIV128 0.0078125
#define DIV64  0.015625
#define DIV32  0.03125
#define DIV16  0.0625
#define DIV15  0.06666666666666667
#define DIV8   0.125
const double DIVSHRT = 1.0 / SHRT_MAX;
const double DIVCHAR = 1.0 / SCHAR_MAX;


#define M_12_ROOT_2 1.0594630943592953


/* https://www.musicdsp.org/en/latest/Synthesis/216-fast-whitenoise-generator.html */
const float wnoisescale = 2.0f / 0xffffffff;
typedef struct { int x1, x2; } wnoise;
void initWnoise(wnoise *w)
{
	w->x1 = 0x67452301;
	w->x2 = 0xefcdab89;
}
float getWnoise(wnoise *w)
{
	w->x1 ^= w->x2;
	float out = w->x2 * wnoisescale;
	w->x2 += w->x1;
	return out;
}


typedef struct
{
	uint8_t a;
	uint8_t d;
	uint8_t s;
	uint8_t r;
} adsr;


/* state variable filter */
#define MAX_RESONANCE 0.04 /* how far off of infinity to go */
#define MIN_RESONANCE 0.9
typedef struct
{
	double l, h, b, n;
} SVFilter;
void runSVFilter(SVFilter *s, double input, double cutoff, double q)
{
	double F1 = 2 * M_PI * cutoff * 0.2;
	s->l = s->l + F1 * s->b;
	s->h = input - s->l - (q * (-MIN_RESONANCE + MAX_RESONANCE) + MIN_RESONANCE) * s->b;
	s->b = F1 * s->h + s->b;
	s->n = s->h + s->l;
}


void drawWave(uint8_t wave, unsigned short y, unsigned short x, char adjust)
{
	switch (wave)
	{
		case 0:
			if (adjust)
			{
				printf(   "\033[%d;%dH [  tri] ", y+0, x);
				printf(   "\033[%d;%dH    saw  ", y+1, x);
				printf(   "\033[%d;%dH   ramp  ", y+2, x);
				printf(   "\033[%d;%dH  pulse  ", y+3, x);
				printf(   "\033[%d;%dH   sine  ", y+4, x);
			} else printf("\033[%d;%dH [  tri] ", y+0, x);
			break;
		case 1:
			if (adjust)
			{
				printf(   "\033[%d;%dH    tri  ", y-1, x);
				printf(   "\033[%d;%dH [  saw] ", y+0, x);
				printf(   "\033[%d;%dH   ramp  ", y+1, x);
				printf(   "\033[%d;%dH  pulse  ", y+2, x);
				printf(   "\033[%d;%dH   sine  ", y+3, x);
			} else printf("\033[%d;%dH [  saw] ", y+0, x);
			break;
		case 2:
			if (adjust)
			{
				printf(   "\033[%d;%dH    tri  ", y-2, x);
				printf(   "\033[%d;%dH    saw  ", y-1, x);
				printf(   "\033[%d;%dH [ ramp] ", y+0, x);
				printf(   "\033[%d;%dH  pulse  ", y+1, x);
				printf(   "\033[%d;%dH   sine  ", y+2, x);
			} else printf("\033[%d;%dH [ ramp] ", y+0, x);
			break;
		case 3:
			if (adjust)
			{
				printf(   "\033[%d;%dH    tri  ", y-3, x);
				printf(   "\033[%d;%dH    saw  ", y-2, x);
				printf(   "\033[%d;%dH   ramp  ", y-1, x);
				printf(   "\033[%d;%dH [pulse] ", y+0, x);
				printf(   "\033[%d;%dH   sine  ", y+1, x);
			} else printf("\033[%d;%dH [pulse] ", y+0, x);
			break;
		case 4:
			if (adjust)
			{
				printf(   "\033[%d;%dH    tri  ", y-4, x);
				printf(   "\033[%d;%dH    saw  ", y-3, x);
				printf(   "\033[%d;%dH   ramp  ", y-2, x);
				printf(   "\033[%d;%dH  pulse  ", y-1, x);
				printf(   "\033[%d;%dH [ sine] ", y+0, x);
			} else printf("\033[%d;%dH [ sine] ", y+0, x);
			break;
	}
}
void drawFilterType(uint8_t type, unsigned short y, unsigned short x, char adjust)
{
	switch (type)
	{
		case 0:
			if (adjust)
			{
				printf(   "\033[%d;%dH[low]", y+0, x);
				printf(   "\033[%d;%dH  hi ", y+1, x);
				printf(   "\033[%d;%dH bnd ", y+2, x);
				printf(   "\033[%d;%dH rej ", y+3, x);
			} else printf("\033[%d;%dH[low]", y+0, x);
			break;
		case 1:
			if (adjust)
			{
				printf(   "\033[%d;%dH low ", y-1, x);
				printf(   "\033[%d;%dH[ hi]", y+0, x);
				printf(   "\033[%d;%dH bnd ", y+1, x);
				printf(   "\033[%d;%dH rej ", y+2, x);
			} else printf("\033[%d;%dH[ hi]", y+0, x);
			break;
		case 2:
			if (adjust)
			{
				printf(   "\033[%d;%dH low ", y-2, x);
				printf(   "\033[%d;%dH  hi ", y-1, x);
				printf(   "\033[%d;%dH[bnd]", y+0, x);
				printf(   "\033[%d;%dH rej ", y+1, x);
			} else printf("\033[%d;%dH[bnd]", y+0, x);
			break;
		case 3:
			if (adjust)
			{
				printf(   "\033[%d;%dH low ", y-3, x);
				printf(   "\033[%d;%dH  hi ", y-2, x);
				printf(   "\033[%d;%dH bnd ", y-1, x);
				printf(   "\033[%d;%dH[rej]", y+0, x);
			} else printf("\033[%d;%dH[rej]", y+0, x);
			break;
	}
}
void drawBit(char a)
{
	if (a) printf("[X]");
	else   printf("[ ]");
}


/* curve 0: linear */
/* curve 1: exponential */
float adsrEnvelope(adsr env, float curve,
		uint32_t pointer,
		uint32_t releasepointer)
{
	float linear;
	if (releasepointer && releasepointer < pointer)
	{ /* release */
		uint32_t releaselength = (env.r+ENVELOPE_RELEASE_MIN) * ENVELOPE_RELEASE * samplerate;
		linear = (1.0f - MIN(1.0f, (float)(pointer - releasepointer) / (float)releaselength)) * (env.s*DIV256);
	} else
	{
		uint32_t attacklength = (env.a+ENVELOPE_ATTACK_MIN) * ENVELOPE_ATTACK * samplerate;
		uint32_t decaylength = (env.d+ENVELOPE_DECAY_MIN) * ENVELOPE_DECAY * samplerate;
		if (pointer < attacklength)
		{ /* attack */
			linear = (float)pointer / (float)attacklength;
			if (!env.d) linear *= env.s*DIV256; /* ramp to sustain if there's no decay stage */
		} else if (env.s < 255 && pointer < attacklength + decaylength)
		{ /* decay */
			linear = 1.0f - (float)(pointer - attacklength) / (float)decaylength * (1.0f - env.s*DIV256);
		} else /* sustain */
			linear = env.s*DIV256;
	}
	/* lerp between linear and exponential */
	return linear * (1.0f - curve) + powf(linear, 2.0f) * curve;
}

typedef struct
{
	float *triangle;
	float *saw;
	float *ramp;
	float *sine;
} oscillatortable;
oscillatortable o;

void freeOscillator(void)
{
	free(o.triangle);
	free(o.saw);
	free(o.ramp);
	free(o.sine);
}

#define OSCILLATOR_TABLE_LEN 2048
void genOscillator(void)
{
	o.triangle = calloc(OSCILLATOR_TABLE_LEN, sizeof(float));
	for (size_t i = 0; i < OSCILLATOR_TABLE_LEN; i++)
		o.triangle[i] = fabsf(fmodf(((float)i / (OSCILLATOR_TABLE_LEN - 1)) + 0.75f, 1.0f) - 0.5f) * 4.0f - 1.0f;

	o.saw = calloc(OSCILLATOR_TABLE_LEN, sizeof(float));
	for (size_t i = 0; i < OSCILLATOR_TABLE_LEN; i++)
		o.saw[i] = 1.0f - (float)i / (OSCILLATOR_TABLE_LEN - 1) * 2.0f;

	o.ramp = calloc(OSCILLATOR_TABLE_LEN, sizeof(float));
	for (size_t i = 0; i < OSCILLATOR_TABLE_LEN; i++)
		o.ramp[i] = -1.0f + fmodf((float)i / (OSCILLATOR_TABLE_LEN - 1) + 0.5f, 1.0f) * 2.0f;

	o.sine = calloc(OSCILLATOR_TABLE_LEN, sizeof(float));
	for (size_t i = 0; i < OSCILLATOR_TABLE_LEN; i++)
		o.sine[i] = sinf((float)i / (OSCILLATOR_TABLE_LEN - 1) * M_PI * 2);
}

/* phase is modulo'd to 0-1 */
/* pw should be 0-1         */
float oscillator(char wave, float phase, float pw)
{
	switch (wave)
	{
		case 0: /* triangle */ return o.triangle[(int)(phase * (float)(OSCILLATOR_TABLE_LEN - 1))];
		case 1: /* saw      */ return o.saw[(int)(phase * (float)(OSCILLATOR_TABLE_LEN - 1))];
		case 2: /* ramp     */ return o.ramp[(int)(phase * (float)(OSCILLATOR_TABLE_LEN - 1))];
		case 3: /* square   */ return (phase > pw) ? -1.0f : +1.0f;
		case 4: /* sine     */ return o.sine[(int)(phase * (float)(OSCILLATOR_TABLE_LEN - 1))];
		default: return 0.0f;
	}
}

/* waveshaper threshold */
float wavefolder(float input)
{
	while (input < -1.0f || input > 1.0f)
	{
		if (input >  1.0f) input =  1.0f - input +  1.0f;
		if (input < -1.0f) input = -1.0f - input + -1.0f;
	}
	return input;
}
float wavewrapper(float input)
{
	while (input >  1.0f) input -= 1.0f;
	while (input < -1.0f) input += 1.0f;
	return input;
}
float signedunsigned(float input)
{
	if (fabsf(input) < NOISE_GATE) /* fix dc */
		return 0.0f;
	else
	{
		if (input > 0.0f) return input - 1.0f;
		else              return input + 1.0f;
	}
}
float hardclip(float input)
{ return MIN(1.0f, MAX(-1.0f, input)); }
float rectify(char type, float input) /* TODO: clicky */
{
	if (fabsf(input) < NOISE_GATE) /* TODO: fix dc properly, high pass it */
		return 0.0f;
	else
	{
		switch (type)
		{
			case 0: /* full-wave    */ return hardclip(fabsf(input) * 2 - 1.0f);
			case 1: /* full-wave x2 */ return hardclip(fabsf(fabsf(input) * 2 - 1.0f) * 2 - 1.0f);
		}
	}
	return input;
}
float thirddegreepolynomial(float input)
{ return hardclip(1.5f*input - 0.5f*input*input*input); }
