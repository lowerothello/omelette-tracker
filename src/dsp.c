#define WAVE_COUNT 4
#define FILTERTYPE_COUNT 1

#define C5_FREQ 261.63
#define MIN_RESONANCE 0.6 /* higher numbers are softer, M_SQRT2 (~1.4) is the highest */
#define MAX_RESONANCE 0.0 /* lower numbers are harsher */
#define MAX_CUTOFF 12000
#define MIN_CUTOFF 100

#define FM_DEPTH 0.005

/* <seconds> */
#define ENVELOPE_ATTACK  0.005
#define ENVELOPE_DECAY   0.020
#define ENVELOPE_RELEASE 0.020
#define LFO_MIN 2.00
#define LFO_MAX 0.001
/* </seconds> */

/* the slowest the gate will move in a second */
#define MIN_GATE_SPEED_SEC 10.0


/* multiply instead of dividing for powers of 2 */
#define DIV256 0.00390625
#define DIV128 0.0078125
#define DIV64  0.015625
#define DIV32  0.03125
#define DIV16  0.0625
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


/* https://www.musicdsp.org/en/latest/Filters/38-lp-and-hp-filter.html */
typedef struct
{ double a1, a2, b1, b2, n1, n2; } filter;

void calcFilter(filter *s, float cut, float res)
{
	double r = MAX_RESONANCE + (MIN_RESONANCE - MAX_RESONANCE) * (1.0 - res);
	double c = MIN(tan((0.5 - (MAX_CUTOFF * cut) * 1.0 / samplerate) * M_PI), MIN_CUTOFF);

	s->a1 = 1.0 / (1.0 + r*c + c*c);
	s->a2 = 2.0 * s->a1;
	s->b1 = 2.0 * (1.0 - c*c) * s->a1;
	s->b2 = (1.0 - r*c + c*c) * s->a1;
}
float runFilter(filter *s, float input)
{
	return s->a1*input + s->a2*s->n1 + s->a1*s->n2 - s->b1*s->n1 - s->b2*s->n2;
}


/* http://www.tinygod.com/code/BLorenzOsc.zip */
typedef struct
{
	float mDX;
	float mDY;
	float mDZ;
	float mDT;
	float mFreq;
	float mX;
	float mY;
	float mZ;

	float mA;
	float mB;
	float mC;
} lorenz;

void lorenzInit(lorenz *l)
{
	l->mA = 10.0;
	l->mB = 28.0;
	l->mC = 2.666;
	l->mDX = l->mDY = l->mDZ = 0.0;
	l->mX = l->mY = l->mZ = 1.0;
	l->mFreq = 440.0;
	l->mDT = l->mFreq / samplerate;
}

void lorenzIterate(lorenz *l)
{
	l->mDX = l->mA * (l->mY - l->mX);
	l->mDY = l->mX * (l->mB - l->mZ) - l->mY;
	l->mDZ = l->mX * l->mY - l->mC * l->mZ;

	l->mX += l->mDX * l->mDT;
	l->mY += l->mDY * l->mDT;
	l->mZ += l->mDZ * l->mDT;
}

float lorenzCurrent(lorenz *l)
{ return l->mX * 0.05107; }

float lorenzAlternative(lorenz *l)
{ return l->mY * 0.03679; }



/* https://www.musicdsp.org/en/latest/Synthesis/241-quick-dirty-sine.html */
/* 0 <= x < 4096 */
float xSin(float x)
{
	const float A = -0.015959964859;
	const float B =  217.68468676;
	const float C =  0.000028716332164;
	const float D = -0.0030591066066;
	const float E = -7.3316892871734489e-005;
	float y;

	char negate = 0;
	if (x > 2048)
	{
		negate = 1;
		x -= 2048;
	}
	if (x > 1024)
		x = 2048 - x;
	if (negate)
		y = -((A+x)/(B+C*x*x)+D*x-E);
	else
		y=(A+x)/(B+C*x*x)+D*x-E;
	return y;
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
		uint32_t releaselength = env.r * ENVELOPE_RELEASE * samplerate;
		linear = (1.0 - MIN(1.0, (float)(pointer - releasepointer) / (float)releaselength)) * (env.s*DIV256);
	} else
	{
		uint32_t attacklength = env.a * ENVELOPE_ATTACK * samplerate;
		uint32_t decaylength = env.d * ENVELOPE_DECAY * samplerate;
		if (pointer < attacklength)
		{ /* attack */
			linear = (float)pointer / (float)attacklength;
			if (!env.d) linear *= env.s*DIV256; /* ramp to sustain if there's no decay stage */
		} else if (env.s < 255 && pointer < attacklength + decaylength)
		{ /* decay */
			linear = 1.0 - (float)(pointer - attacklength) / (float)decaylength * (1.0 - env.s*DIV256);
		} else /* sustain */
			linear = env.s*DIV256;
	}
	/* lerp between linear and exponential */
	return linear * (1.0 - curve) + powf(linear, 2.0) * curve;
}

/* phase is modulo'd to 0-1 */
/* pw should be 0-1         */
float oscillator(char wave, float phase, float pw)
{
	float output = 0.0;
	switch (wave)
	{
		case 0: /* triangle */ output = fabsf(fmodf(phase + 0.75, 1.0) - 0.5) * 4 - 1.0; break;
		case 1: /* saw      */ output = +1.0 - fmodf(phase, 1.0) * 2; break;
		case 2: /* ramp     */ output = -1.0 + fmodf(phase + 0.5, 1.0) * 2; break;
		case 3: /* square   */ if (fmodf(phase, 1.0) > pw) output = -1.0; else output = +1.0; break;
		case 4: /* sine     */ output = xSin(fmodf(phase, 1.0) * 4096); break;
	}
	return output;
}


/* waveshaper threshold */
const float wst = 1.0;
float wavefolder(float input)
{
	while (input < -wst || input > wst)
	{
		if (input >  wst) input =  wst - input +  wst;
		if (input < -wst) input = -wst - input + -wst;
	}
	return input;
}
float wavewrapper(float input)
{
	while (input >  wst) input -= wst;
	while (input < -wst) input += wst;
	return input;
}
float signedunsigned(float input)
{
	if (input == 0.0) /* fix dc */
		return 0.0;
	else
	{
		if (input > 0.0) return input - wst;
		else             return input + wst;
	}
}
float rectify(char type, float input) /* TODO: clicky */
{
	if (input == 0.0) /* fix dc, might be some audible diracs */
		return 0.0;
	else
	{
		switch (type)
		{
			case 0: /* full-wave    */ return MIN(wst, MAX(-wst, fabsf(input) * 2 - wst));
			case 1: /* full-wave x2 */ return MIN(wst, MAX(-wst, fabsf(fabsf(input) * 2 - wst) * 2 - wst));
		}
	}
	return input;
}

float hardclip(float input)
{ return MIN(wst, MAX(-wst, input)); }

float thirddegreepolynomial(float input)
{ return hardclip(1.5*input - 0.5*input*input*input); }
