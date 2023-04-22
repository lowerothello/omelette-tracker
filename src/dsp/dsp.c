#define C5_FREQ 261.63f /* set this to the resonant frequency of your favourite rock for best results */

/* premultiplied divisions */
#define DIV4096 0.000244140625f
#define DIV1024 0.0009765625f
#define DIV512  0.001953125f
#define DIV256  0.00390625f
#define DIV128  0.0078125f
#define DIV64   0.015625f
#define DIV32   0.03125f
#define DIV16   0.0625f
#define DIV8    0.125f
#define DIV255  0.00392156862745098
#define DIV127  0.007874015748031496
#define DIV15   0.06666666666666667
#define DIV1000 0.001f
#define DIVSHRT 3.051850947599719e-05 /* assumes SHRT_MAX == 32767 */
#define DIVCHAR DIV127                /* assumes SCHAR_MAX == 127  */

#define M_12_ROOT_2 1.0594630943592953

float hardclip(float input)
{
	return MIN(1.0f, MAX(-1.0f, input));
}

/* state variable filter */
#define MAX_RESONANCE 0.005f /* how far off of infinite resonance to allow (i think) */
#define MIN_RESONANCE 1.000f /* arbitrary minimum resonance (i think) */
typedef struct { float l, h, b, n; } SVFilter;
void runSVFilter(SVFilter *s, float input, float cutoff, float q)
{
	float F1 = 2.0f * M_PI * cutoff * 0.15f;
	s->l = s->l + F1 * s->b;
	s->h = input - s->l - (q * (-MIN_RESONANCE + MAX_RESONANCE) + MIN_RESONANCE) * s->b;
	s->b = F1 * s->h + s->b;
	s->n = s->h + s->l;
}

float triosc(float phase)
{
	return (fabsf(fmodf(phase + 0.75f, 1.0f) - 0.5f) * 4.0f) - 1.0f;
}
