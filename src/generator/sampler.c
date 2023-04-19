/* downsampling approximation                 */
/* not perfectly accurate cos floats are used */
static double calcDecimate(uint8_t decimate, double pointer)
{
	double d = pow(1.0 + (1.0 - decimate*DIV255), 2);
	return (uint32_t)(pointer/d)*d;
}

static void calcDecimateLerp(uint8_t decimate, double pointer,
		double *retptr, double *retptrnext, double *retdelta)
{
	double d = pow(1.0 + (1.0 - decimate*DIV255), 2);
	*retptr = (uint32_t)(pointer/d)*d;
	*retptrnext = (uint32_t)((pointer+d)/d)*d;
	*retdelta = fmod(pointer, *retptr) / d;
}

void getSample(double ptr, bool interpolate, uint8_t decimate, int8_t bitdepth, Sample *s, short *output)
{
	uint8_t shift = 0xf - bitdepth;
	double iptr;
	if (interpolate && modf(ptr, &iptr) > 0.0)
	{
		double retptr, retptrnext, retdelta;
		calcDecimateLerp(decimate, ptr, &retptr, &retptrnext, &retdelta);

		if (retptr <= (s->length-1) * s->channels)
			*output += (s->data[(uint32_t)retptr]>>shift)<<shift;
		if (retptrnext <= (s->length-1) * s->channels)
		{
			*output *= 1.0 - retdelta;
			*output += ((s->data[(uint32_t)retptrnext]>>shift)<<shift) * retdelta;
		}
	} else
	{
		ptr = calcDecimate(decimate, ptr);
		if (ptr <= (s->length-1) * s->channels)
			*output += (s->data[(uint32_t)ptr]>>shift)<<shift;
	}
	*output *= s->gain*DIV256;
}

void getSampleLoopRamp(double ptr, double rptr, float lerp, bool interpolate, uint8_t decimate, int8_t bitdepth, Sample *s, short *output)
{
	uint8_t shift = 0xf - bitdepth;

	if (interpolate)
	{
		double retptr, retptrnext, retdelta;
		double rretptr, rretptrnext, rretdelta;
		calcDecimateLerp(decimate, ptr, &retptr, &retptrnext, &retdelta);
		calcDecimateLerp(decimate, rptr, &rretptr, &rretptrnext, &rretdelta);

		if (retptr <= (s->length-1) * s->channels)
			*output += (s->data[(uint32_t)retptr]>>shift)<<shift;
		if (retptrnext <= (s->length-1) * s->channels)
		{
			*output *= 1.0 - retdelta;
			*output += ((s->data[(uint32_t)retptrnext]>>shift)<<shift) * retdelta;
		}

		short rampoutput = 0;
		if (rretptr <= (s->length-1) * s->channels)
			rampoutput += (s->data[(uint32_t)rretptr]>>shift)<<shift;
		if (rretptrnext <= (s->length-1) * s->channels)
		{
			rampoutput *= 1.0f - rretdelta;
			rampoutput += ((s->data[(uint32_t)rretptrnext]>>shift)<<shift) * rretdelta;
		}

		*output *= 1.0f - lerp;
		*output += rampoutput * lerp;
	} else
	{
		ptr =  calcDecimate(decimate, ptr);
		rptr = calcDecimate(decimate, rptr);

		if (ptr <= (s->length-1) * s->channels)
			*output += (s->data[(uint32_t)ptr]>>shift)<<shift;
		if (rptr <= (s->length-1) * s->channels)
		{
			*output *= 1.0f - lerp;
			*output += ((s->data[(uint32_t)rptr]>>shift)<<shift)*lerp;
		}
	}
	*output *= s->gain*DIV256;
}

float semitoneShortToMultiplier(int16_t input)
{
	if (input < 0) return powf(M_12_ROOT_2, -((abs(input)>>12)*12 + (abs(input)&0x0fff)*DIV256));
	else           return powf(M_12_ROOT_2, (input>>12)*12 + (input&0x0fff)*DIV256);
}

