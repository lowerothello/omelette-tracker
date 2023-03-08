#define LFO_MAX_S 0.1f
#define LFO_MIN_S 10.0f

typedef struct {
	enum WaveShape shape;
	int8_t         duty;
	uint8_t        speed;
	float          phase;
} Lfo;

/* returns the output */
float runLfo(Lfo *lfo)
{
	lfo->phase += ((1.0f - (lfo->speed*DIV255)) * (LFO_MAX_S - LFO_MIN_S) + LFO_MIN_S) / samplerate;
	while (lfo->phase > 1.0f) lfo->phase -= 1.0f;

	float duty = (lfo->duty + 128)*DIV255;
	switch (lfo->shape)
	{
		case SHAPE_PULSE:
			if (lfo->phase < duty) return  1.0f;
			else                   return -1.0f;
		case SHAPE_LINEAR:
			if (lfo->phase < duty) return -1.0f + (2.0f * ( lfo->phase         * 1.0f/        duty ));
			else                   return  1.0f - (2.0f * ((lfo->phase - duty) * 1.0f/(1.0f - duty)));
		case SHAPE_SINE: break;
	}
	return 0.0f; /* fallback */
}
