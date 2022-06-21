#define WAVE_COUNT 4
#define FILTERTYPE_COUNT 1

#define C5_FREQ 261.63
#define MIN_RESONANCE 0.6 /* higher numbers are softer, M_SQRT2 (~1.4) is the highest */
#define MAX_RESONANCE 0.0 /* lower numbers are harsher */
#define MAX_CUTOFF 12000
#define MIN_CUTOFF 100

#define FM_DEPTH 0.01

/* <seconds> */
#define ENVELOPE_ATTACK  0.005
#define ENVELOPE_DECAY   0.020
#define ENVELOPE_RELEASE 0.020
#define LFO_MIN 2.00
#define LFO_MAX 0.001
/* </seconds> */



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

float runFilter(filter *s, float input, float cut, float res)
{
	double r = MAX_RESONANCE + (MIN_RESONANCE - MAX_RESONANCE) * (1.0 - res);
	double c = MIN(tan((0.5 - (MAX_CUTOFF * cut) * 1.0 / samplerate) * M_PI), MIN_CUTOFF);

	s->a1 = 1.0 / (1.0 + r*c + c*c);
	s->a2 = 2.0 * s->a1;
	s->b1 = 2.0 * (1.0 - c*c) * s->a1;
	s->b2 = (1.0 - r*c + c*c) * s->a1;

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
float xSin(double x)
{
	const double A = -0.015959964859;
	const double B =  217.68468676;
	const double C =  0.000028716332164;
	const double D = -0.0030591066066;
	const double E = -7.3316892871734489e-005;
	double y;

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
	return (float)y;
}

void drawWave(char wave, unsigned short y, unsigned short x, char adjust)
{
	switch (wave)
	{
		case 0:
			if (adjust)
			{
				printf("\033[%d;%dH [  tri] ", y+0, x);
				printf("\033[%d;%dH    saw  ", y+1, x);
				printf("\033[%d;%dH   ramp  ", y+2, x);
				printf("\033[%d;%dH  pulse  ", y+3, x);
				printf("\033[%d;%dH   sine  ", y+4, x);
			} else printf("\033[%d;%dH [  tri] ", y+0, x);
			break;
		case 1:
			if (adjust)
			{
				printf("\033[%d;%dH    tri  ", y-1, x);
				printf("\033[%d;%dH [  saw] ", y+0, x);
				printf("\033[%d;%dH   ramp  ", y+1, x);
				printf("\033[%d;%dH  pulse  ", y+2, x);
				printf("\033[%d;%dH   sine  ", y+3, x);
			} else printf("\033[%d;%dH [  saw] ", y+0, x);
			break;
		case 2:
			if (adjust)
			{
				printf("\033[%d;%dH    tri  ", y-2, x);
				printf("\033[%d;%dH    saw  ", y-1, x);
				printf("\033[%d;%dH [ ramp] ", y+0, x);
				printf("\033[%d;%dH  pulse  ", y+1, x);
				printf("\033[%d;%dH   sine  ", y+2, x);
			} else printf("\033[%d;%dH [ ramp] ", y+0, x);
			break;
		case 3:
			if (adjust)
			{
				printf("\033[%d;%dH    tri  ", y-3, x);
				printf("\033[%d;%dH    saw  ", y-2, x);
				printf("\033[%d;%dH   ramp  ", y-1, x);
				printf("\033[%d;%dH [pulse] ", y+0, x);
				printf("\033[%d;%dH   sine  ", y+1, x);
			} else printf("\033[%d;%dH [pulse] ", y+0, x);
			break;
		case 4:
			if (adjust)
			{
				printf("\033[%d;%dH    tri  ", y-4, x);
				printf("\033[%d;%dH    saw  ", y-3, x);
				printf("\033[%d;%dH   ramp  ", y-2, x);
				printf("\033[%d;%dH  pulse  ", y-1, x);
				printf("\033[%d;%dH [ sine] ", y+0, x);
			} else printf("\033[%d;%dH [ sine] ", y+0, x);
			break;
	}
}
void drawFilterType(char type, unsigned short y, unsigned short x, char adjust)
{
	switch (type)
	{
		case 0:
			if (adjust)
			{
				printf("\033[%d;%dH[low]", y+0, x);
				printf("\033[%d;%dH  hi ", y+1, x);
				printf("\033[%d;%dH bnd ", y+2, x);
				printf("\033[%d;%dH rej ", y+3, x);
			} else printf("\033[%d;%dH[low]", y+0, x);
			break;
		case 1:
			if (adjust)
			{
				printf("\033[%d;%dH low ", y-1, x);
				printf("\033[%d;%dH[ hi]", y+0, x);
				printf("\033[%d;%dH bnd ", y+1, x);
				printf("\033[%d;%dH rej ", y+2, x);
			} else printf("\033[%d;%dH[ hi]", y+0, x);
			break;
		case 2:
			if (adjust)
			{
				printf("\033[%d;%dH low ", y-2, x);
				printf("\033[%d;%dH  hi ", y-1, x);
				printf("\033[%d;%dH[bnd]", y+0, x);
				printf("\033[%d;%dH rej ", y+1, x);
			} else printf("\033[%d;%dH[bnd]", y+0, x);
			break;
		case 3:
			if (adjust)
			{
				printf("\033[%d;%dH low ", y-3, x);
				printf("\033[%d;%dH  hi ", y-2, x);
				printf("\033[%d;%dH bnd ", y-1, x);
				printf("\033[%d;%dH[rej]", y+0, x);
			} else printf("\033[%d;%dH[rej]", y+0, x);
			break;
	}
}
void drawBit(char true)
{
	if (true) printf("[X]");
	else      printf("[ ]");
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
		linear = (1.0 - MIN(1.0, (float)(pointer - releasepointer) / (float)releaselength)) * env.s / 255.0;
	} else
	{
		uint32_t attacklength = env.a * ENVELOPE_ATTACK * samplerate;
		uint32_t decaylength = env.d * ENVELOPE_DECAY * samplerate;
		if (pointer < attacklength)
		{ /* attack */
			linear = (float)pointer / (float)attacklength;
			/* ramp to sustain if there's no decay stage */
			if (!env.d) linear *= env.s / 255.0;
		} else if (env.s < 255 && pointer < attacklength + decaylength)
		{ /* decay */
			linear = 1.0 - (float)(pointer - attacklength)
				/ (float)decaylength * (1.0 - env.s / 255.0);
		} else linear = env.s / 255.0; /* sustain */
	}
	/* lerp between linear and exponential */
	return linear + (powf(2.0, linear) - 1.0 - linear) * curve;
}

/* phase is modulo'd to 0-1 */
/* pw should be 0-1         */
float oscillator(char wave, float phase, float pw)
{
	float output = 0.0;
	switch (wave)
	{
		case 0: /* triangle */
			output = fabsf(fmodf(phase + 0.75, 1.0) - 0.5) * 4 - 1.0;
			break;
		case 1: /* saw      */ output = +1.0 - fmodf(phase, 1.0) * 2; break;
		case 2: /* ramp     */ output = -1.0 + fmodf(phase + 0.5, 1.0) * 2; break;
		case 3: /* square   */
			if (fmodf(phase, 1.0) > pw) output = -1.0;
			else                        output = +1.0;
			break;
		case 4: /* sine     */
			output = xSin(fmodf(phase, 1.0) * 4096);
			break;
	}
	return output;
}
