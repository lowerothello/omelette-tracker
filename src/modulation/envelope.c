#define ENVELOPE_A_STEP 0.02f
#define ENVELOPE_A_MIN  0.00f

#define ENVELOPE_D_STEP 0.13f
#define ENVELOPE_D_MIN  0.01f

#define ENVELOPE_GATE 0.00001f


typedef struct {
	uint16_t adsr;   /* control */
	float    output; /* output  */

	bool     release; /* set high to trigger release */

	/* internal */
	uint32_t a;
	uint32_t d;
	float    s;
	uint32_t r;
	uint32_t pointer;
} Envelope;

void applyEnvelopeControlChanges(Envelope *env)
{
	env->a = (((env->adsr & 0xf000) >> 12)+ENVELOPE_A_MIN) * ENVELOPE_A_STEP * samplerate;
	env->d = (((env->adsr & 0x0f00) >> 8 )+ENVELOPE_D_MIN) * ENVELOPE_D_STEP * samplerate;
	env->s =  ((env->adsr & 0x00f0) >> 4 ) * DIV15;
	env->r = (((env->adsr & 0x000f) >> 0 )+ENVELOPE_D_MIN) * ENVELOPE_D_STEP * samplerate;

}
/* returns true if the envelope has finished */
bool envelope(Envelope *env)
{
	if (env->release)
	{
		if (env->r) env->output = MAX(env->output - (1.0f/env->r), 0.0f);
		else        env->output = 0.0f;
	} else
	{
		if (env->pointer <= env->a)
		{
			if (env->a) env->output = MIN(env->output + (1.0f/env->a), 1.0f);
			else        env->output = 1.0f;
		} else if (env->pointer < env->a + env->d) env->output -= ((1.0f - env->s)/env->d);
		else                                       env->output = env->s;
	}

	env->pointer++;

	if (env->pointer > env->a && env->output < ENVELOPE_GATE)
		return 1;
	return 0;
}
