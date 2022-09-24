#define WAVE_COUNT 4
#define FILTERTYPE_COUNT 1

#define C5_FREQ 261.63 /* set this to the resonant frequency of your favourite rock for best results */

#define FM_DEPTH 0.005

/* <seconds> */
#define ENVELOPE_A_STEP 0.1
#define ENVELOPE_A_MIN 0
#define ENVELOPE_D_STEP 0.1
#define ENVELOPE_D_MIN 0
#define LFO_MIN 2.00
#define LFO_MAX 0.001
/* </seconds> */

/* the slowest the gate will move in a second */
#define MIN_GATE_SPEED_SEC 10.0


/* not related to the gate macro */
/* the threshold where processing is no longer necessary */
#define NOISE_GATE 0.00001


/* multiply instead of dividing for float powers of 2 */
#define DIV256   0.00390625
#define DIV128   0.0078125
#define DIV64    0.015625
#define DIV32    0.03125
#define DIV16    0.0625
#define DIV8     0.125
#define DIV255   0.00392156862745098
#define DIV15    0.06666666666666667
#define DIV1000  0.001
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

float hardclip(float input)
{
	return MIN(1.0f, MAX(-1.0f, input));
}

typedef struct
{
	uint8_t a;
	uint8_t d;
	uint8_t s;
	uint8_t r;
} adsr;


/* state variable filter */
#define MAX_RESONANCE 0.005 /* how far off of infinity to go */
#define MIN_RESONANCE 0.95
typedef struct { double l, h, b, n; } SVFilter;
void runSVFilter(SVFilter *s, double input, double cutoff, double q)
{
	double F1 = 2 * M_PI * cutoff * 0.15;
	s->l = s->l + F1 * s->b;
	s->h = input - s->l - (q * (-MIN_RESONANCE + MAX_RESONANCE) + MIN_RESONANCE) * s->b;
	s->b = F1 * s->h + s->b;
	s->n = s->h + s->l;
}

void drawChannels(uint8_t mode, unsigned short y, unsigned short x, char adjust)
{
	switch (mode)
	{
		case 0:
			if (adjust)
			{
				printf("\033[%d;%dH [stereo] ", y+0, x);
				printf("\033[%d;%dH    left  ", y+1, x);
				printf("\033[%d;%dH   right  ", y+2, x);
				// printf("\033[%d;%dH     mix  ", y+3, x);
				// printf("\033[%d;%dH    swap  ", y+4, x);
			} else printf("\033[%d;%dH [stereo] ", y+0, x);
			break;
		case 1:
			if (adjust)
			{
				printf("\033[%d;%dH  stereo  ", y-1, x);
				printf("\033[%d;%dH [  left] ", y+0, x);
				printf("\033[%d;%dH   right  ", y+1, x);
				printf("\033[%d;%dH     mix  ", y+2, x);
				// printf("\033[%d;%dH    swap  ", y+3, x);
			} else printf("\033[%d;%dH [  left] ", y+0, x);
			break;
		case 2:
			if (adjust)
			{
				printf("\033[%d;%dH  stereo  ", y-2, x);
				printf("\033[%d;%dH    left  ", y-1, x);
				printf("\033[%d;%dH [ right] ", y+0, x);
				printf("\033[%d;%dH     mix  ", y+1, x);
				printf("\033[%d;%dH    swap  ", y+2, x);
			} else printf("\033[%d;%dH [ right] ", y+0, x);
			break;
		case 3:
			if (adjust)
			{
				printf("\033[%d;%dH  stereo  ", y-3, x);
				printf("\033[%d;%dH    left  ", y-2, x);
				printf("\033[%d;%dH   right  ", y-1, x);
				printf("\033[%d;%dH [   mix] ", y+0, x);
				printf("\033[%d;%dH    swap  ", y+1, x);
			} else printf("\033[%d;%dH [   mix] ", y+0, x);
			break;
		case 4:
			if (adjust)
			{
				printf("\033[%d;%dH  stereo  ", y-4, x);
				printf("\033[%d;%dH    left  ", y-3, x);
				printf("\033[%d;%dH   right  ", y-2, x);
				printf("\033[%d;%dH     mix  ", y-1, x);
				printf("\033[%d;%dH [  swap] ", y+0, x);
			} else printf("\033[%d;%dH [  swap] ", y+0, x);
			break;
	}
}


/* typedef struct
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
#define OSCILLATOR_TABLE_LEN 512
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
} */

/* phase should be 0-1 */
/* pw should be 0-1    */
/* float oscillator(char wave, float phase, float pw)
{
	switch (wave)
	{
		case 0: return o.triangle[(int)(phase * (float)(OSCILLATOR_TABLE_LEN - 1))]; // triangle
		case 1: return o.saw[(int)(phase * (float)(OSCILLATOR_TABLE_LEN - 1))];      // saw
		case 2: return o.ramp[(int)(phase * (float)(OSCILLATOR_TABLE_LEN - 1))];     // ramp
		case 3: return (phase > pw) ? -1.0f : +1.0f;                                 // square
		case 4: return o.sine[(int)(phase * (float)(OSCILLATOR_TABLE_LEN - 1))];     // sine
		default: return 0.0f;
	}
} */

float triosc(float phase)
{ return (fabsf(fmodf(phase + 0.75f, 1.0f) - 0.5f) * 4.0f) - 1.0f; }

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
float rectify(float input) /* TODO: clicky */
{
	if (fabsf(input) < NOISE_GATE) /* TODO: fix dc properly, high pass the output */
		return 0.0f;
	else
		return hardclip(fabsf(input) * 2.0f - 1.0f);
	return input;
}
float thirddegreepolynomial(float input)
{ return hardclip(1.5f*input - 0.5f*input*input*input); }
