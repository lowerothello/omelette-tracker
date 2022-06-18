#define WAVE_COUNT 4
#define FILTERTYPE_COUNT 1

#define C5_FREQ 261.63
#define MIN_RESONANCE 0.7 /* higher numbers are softer, M_SQRT2 (~1.4) is the highest */
#define MAX_RESONANCE 0.0 /* lower numbers are harsher */
#define MAX_CUTOFF 12000

#define FM_DEPTH 2 /* fase modulation depth */

/* https://www.musicdsp.org/en/latest/Synthesis/216-fast-whitenoise-generator.html */
const float g_fScale = 2.0f / 0xffffffff;
int g_x1 = 0x67452301;
int g_x2 = 0xefcdab89;

typedef struct
{
	uint8_t a;
	uint8_t d;
	uint8_t s;
	uint8_t r;
} adsr;



/* https://www.musicdsp.org/en/latest/Filters/38-lp-and-hp-filter.html */
typedef struct
{
	double c;
	double a1, a2, a3;
	double b1, b2;
} filter;

void calcLp(filter *f, float cutoff, float resonance)
{
	double r = MAX_RESONANCE + (MIN_RESONANCE - MAX_RESONANCE) * (1.0 - resonance);
	f->c = tan((0.5 - (MAX_CUTOFF * cutoff) * 1.0 / samplerate) * M_PI);

	f->a1 = 1.0 / (1.0 + r * f->c + f->c * f->c);
	f->a2 = 2 * f->a1;
	f->a3 = f->a1;
	f->b1 = 2.0 * (1.0 - f->c * f->c) * f->a1;
	f->b2 = (1.0 - r * f->c + f->c * f->c) * f->a1;
}
void calcHp(filter *f, float cutoff, float resonance)
{
	double r = MAX_RESONANCE + (MIN_RESONANCE - MAX_RESONANCE) * (1.0 - resonance);
	f->c = tan(M_PI * (MAX_CUTOFF * cutoff) / samplerate);

	f->a1 = 1.0 / (1.0 + r * f->c + f->c * f->c);
	f->a2 = -2 * f->a1;
	f->a3 = f->a1;
	f->b1 = 2.0 * (f->c * f->c - 1.0) * f->a1;
	f->b2 = (1.0 - r * f->c + f->c * f->c) * f->a1;
}
float runFilter(filter *f, float n, float n_1, float n_2)
{
	return f->a1 * n + f->a2 * n_1 + f->a3 * n_2 - f->b1 * n_1 - f->b2 * n_2;
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

void drawWave(char wave)
{
	switch (wave)
	{
		case 0:  printf("[   tri]"); break;
		case 1:  printf("[   saw]"); break;
		case 2:  printf("[  ramp]"); break;
		case 3:  printf("[square]"); break;
		case 4:  printf("[  sine]"); break;
		default: printf("[ ?????]"); break;
	}
}
void drawFilterType(char type)
{
	switch (type)
	{
		case 0:  printf("[ low]"); break;
		case 1:  printf("[high]"); break;
		// case 2:  printf("[band]"); break;
		default: printf("[????]"); break;
	}
}


void adsrEnvelope(adsr env, float *gain,
		uint32_t pointer,
		uint32_t releasepointer)
{
	if (releasepointer && releasepointer < pointer)
	{ /* release */
		if (env.r)
		{
			uint32_t releaselength = env.r * ENVELOPE_RELEASE * samplerate;
			if (pointer - releasepointer < releaselength)
				*gain *= 1.0 - (float)(pointer - releasepointer)
					/ (float)releaselength * env.s / 255.0;
			else { gain = NULL; return; }
		} else { gain = NULL; return; }
	}

	uint32_t attacklength = env.a * ENVELOPE_ATTACK * samplerate;
	uint32_t decaylength = env.d * ENVELOPE_DECAY * samplerate;
	if (pointer < attacklength)
	{ /* attack */
		*gain *= (float)pointer / (float)attacklength;
		/* attack straight to sustain if there's no decay stage */
		if (!env.d) *gain *= env.s / 255.0;
	} else if (env.s < 255 && pointer < attacklength + decaylength)
	{ /* decay */
		*gain *= 1.0 - (float)(pointer - attacklength)
			/ (float)decaylength * (1.0 - env.s / 255.0);
	} else
	{ /* sustain */
		*gain *= env.s / 255.0;
	}
}

/* phase is modulod to 0-1 */
/* pw is 0-1    */
float oscillator(char wave, float phase, float pw)
{
	float output = 0.0;
	switch (wave)
	{
		case 0: /* triangle */
			output = fabsf(fmodf(phase + 0.25, 1.0) - 0.5) * 4 - 1.0;
			break;
		case 1: /* saw      */ output = +1.0 - fmodf(phase + 0.5, 1.0) * 2; break;
		case 2: /* ramp     */ output = -1.0 + fmodf(phase + 0.5, 1.0) * 2; break;
		case 3: /* square   */
			if (fmodf(phase, 1.0) > pw) output = +1.0;
			else                        output = -1.0;
			break;
		case 4: /* sine     */
			output = xSin(phase * 4096);
			break;
	}
	return output;
}