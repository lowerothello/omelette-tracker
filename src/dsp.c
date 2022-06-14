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
