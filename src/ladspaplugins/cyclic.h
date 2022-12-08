#define CYCLIC_MAX_DELAY_S 1
#define CYCLIC_BUFFER_CYCLES 3.0f

#define CYCLIC_OCTAVE_RANGE 3

#define CYCLIC_PORTC 7
const LADSPA_PortDescriptor cyclic_PortDescriptors[CYCLIC_PORTC] =
{
	LADSPA_PORT_INPUT|LADSPA_PORT_AUDIO,
	LADSPA_PORT_INPUT|LADSPA_PORT_AUDIO,
	LADSPA_PORT_OUTPUT|LADSPA_PORT_AUDIO,
	LADSPA_PORT_OUTPUT|LADSPA_PORT_AUDIO,
	LADSPA_PORT_INPUT|LADSPA_PORT_CONTROL,
	LADSPA_PORT_INPUT|LADSPA_PORT_CONTROL,
	LADSPA_PORT_INPUT|LADSPA_PORT_CONTROL,
};

const char * const cyclic_PortNames[CYCLIC_PORTC] =
{
	"Audio In L",
	"Audio In R",
	"Audio Out L",
	"Audio Out R",
	"Cycle Length",
	"Pitch (Semitones)",
	"Tremolo",
};

const LADSPA_PortRangeHint cyclic_PortRangeHints[CYCLIC_PORTC] =
{
	{ 0, 0.0f, 0.0f, },
	{ 0, 0.0f, 0.0f, },
	{ 0, 0.0f, 0.0f, },
	{ 0, 0.0f, 0.0f, },
	{ LADSPA_HINT_BOUNDED_BELOW|LADSPA_HINT_BOUNDED_ABOVE|LADSPA_HINT_DEFAULT_LOW, 0.0f, 1.0f, },
	{ LADSPA_HINT_BOUNDED_BELOW|LADSPA_HINT_BOUNDED_ABOVE|LADSPA_HINT_DEFAULT_MIDDLE, -12.0f * CYCLIC_OCTAVE_RANGE, 12.0f * CYCLIC_OCTAVE_RANGE, },
	{ LADSPA_HINT_BOUNDED_BELOW|LADSPA_HINT_BOUNDED_ABOVE|LADSPA_HINT_INTEGER|LADSPA_HINT_DEFAULT_MINIMUM, 0.0f, 2.0f, },
};

#define CYCLIC_PORT_INL 0
#define CYCLIC_PORT_INR 1
#define CYCLIC_PORT_OUTL 2
#define CYCLIC_PORT_OUTR 3
#define CYCLIC_PORT_CLEN 4
#define CYCLIC_PORT_PITCH 5
#define CYCLIC_PORT_TREM 6


struct CyclicHandle {
	LADSPA_Data *port[CYCLIC_PORTC];
	float        pitch, oldpitch; /* oldpitch is needed for ramping to be correct */
	LADSPA_Data *bufferl;
	LADSPA_Data *bufferr;
	unsigned long blen; /* max buffer length */
	unsigned long bptr; /* buffer pointer, walks up to .blen*CYCLIC_BUFFER_CYCLES */
	unsigned long cptr; /* cycle pointer, walks up to .cptr*CYCLIC_PORT_CYCLELEN */
};

LADSPA_Handle cyclic_instantiate(const LADSPA_Descriptor *desc, unsigned long rate)
{
	struct CyclicHandle *handle = calloc(1, sizeof(struct CyclicHandle));
	handle->blen = rate * CYCLIC_MAX_DELAY_S;
	handle->bufferl = calloc(handle->blen * CYCLIC_BUFFER_CYCLES, sizeof(LADSPA_Data));
	handle->bufferr = calloc(handle->blen * CYCLIC_BUFFER_CYCLES, sizeof(LADSPA_Data));
	handle->bptr = 0;
	handle->cptr = 0;

	return handle;
}
void cyclic_cleanup(LADSPA_Handle handle)
{
	struct CyclicHandle *h = handle;
	free(h->bufferl);
	free(h->bufferr);
	free(h);
}

float cyclicGenPitch(float pitchport)
{
	return powf(M_12_ROOT_2, pitchport);
}

void cyclic_activate(LADSPA_Handle handle)
{
	struct CyclicHandle *h = handle;
	h->pitch = h->oldpitch = cyclicGenPitch(h->port[CYCLIC_PORT_PITCH][0]);
	h->cptr = 0;
}

void cyclic_connect_port(LADSPA_Handle handle, unsigned long i, LADSPA_Data *data)
{
	struct CyclicHandle *h = handle;
	h->port[i] = data;
}

/* handles overflow */
unsigned long pastSample(unsigned long buflen, unsigned long bufptr, unsigned long offset)
{
	if (offset > bufptr) return buflen - (offset - bufptr);
	else                 return bufptr - offset;
}

#define CYCLIC_TREM_MIN_CLEN 20 /* tremolo with low clen is LOUD */
void cyclic_run(LADSPA_Handle handle, unsigned long bufsize)
{
	struct CyclicHandle *h = handle;

	LADSPA_Data inl, inr;
	for (unsigned long i = 0; i < bufsize; i++)
	{
		inl = h->port[CYCLIC_PORT_INL][i];
		inr = h->port[CYCLIC_PORT_INR][i];

		/* cycle length, in samples */
		unsigned long clen = h->blen * h->port[CYCLIC_PORT_CLEN][0];
		clen = MAX(1, clen); /* avoid dividing by 0 */

		/* pitched cycle length */
		unsigned long pclen = clen / h->pitch;
		pclen = MAX(1, pclen); /* avoid dividing by 0 */
		pclen = MIN(clen, pclen);

		/* ramp length */
		// unsigned long rlen = pclen>>1; /* 0 is safe */

		/* subcycle pointer */
		unsigned long rptr = h->cptr%pclen;

		/* pitched pointer offset */
		unsigned long pitchptr = rptr * h->pitch;

		h->port[CYCLIC_PORT_OUTL][i] = h->bufferl[pastSample(h->blen*CYCLIC_BUFFER_CYCLES, h->bptr, clen+h->cptr - pitchptr)];
		h->port[CYCLIC_PORT_OUTR][i] = h->bufferr[pastSample(h->blen*CYCLIC_BUFFER_CYCLES, h->bptr, clen+h->cptr - pitchptr)];

		float xfade;
		unsigned long rpitchptr;
		if (rptr < pclen) /* ramping */
		{
			if (rptr == h->cptr) /* ramp out the prev cycle */
			{
				xfade = (float)rptr / (float)pclen;
				h->port[CYCLIC_PORT_OUTL][i] *= xfade;
				h->port[CYCLIC_PORT_OUTR][i] *= xfade;
				rpitchptr = (pclen - (clen%pclen) + rptr) * h->pitch;
				// h->port[CYCLIC_PORT_OUTL][i] += h->bufferl[pastSample(h->blen*CYCLIC_BUFFER_CYCLES, h->bptr, (clen<<1)+rptr - rpitchptr)] * (1.0f - xfade);
				// h->port[CYCLIC_PORT_OUTR][i] += h->bufferr[pastSample(h->blen*CYCLIC_BUFFER_CYCLES, h->bptr, (clen<<1)+rptr - rpitchptr)] * (1.0f - xfade);
			}
			else /* ramp out the current cycle */
			{
				xfade = (float)rptr / (float)MAX(1, MIN(pclen, clen - (h->cptr - rptr)));
				h->port[CYCLIC_PORT_OUTL][i] *= xfade;
				h->port[CYCLIC_PORT_OUTR][i] *= xfade;
				// h->port[CYCLIC_PORT_OUTL][i] *= 0.0f;
				// h->port[CYCLIC_PORT_OUTR][i] *= 0.0f;
				// rpitchptr = ((clen+rptr)%pclen) * h->pitch;
				rpitchptr = pitchptr;
				h->port[CYCLIC_PORT_OUTL][i] += h->bufferl[pastSample(h->blen*CYCLIC_BUFFER_CYCLES, h->bptr, clen+h->cptr - rpitchptr)] * (1.0f - xfade);
				h->port[CYCLIC_PORT_OUTR][i] += h->bufferr[pastSample(h->blen*CYCLIC_BUFFER_CYCLES, h->bptr, clen+h->cptr - rpitchptr)] * (1.0f - xfade);
			}
		}

		if (clen > CYCLIC_TREM_MIN_CLEN)
		{
			if (h->port[CYCLIC_PORT_TREM][0] > 1.0f) /* autopan */
			{
				xfade = fabsf((float)h->cptr/(float)clen * 2.0f - 1.0f);
				h->port[CYCLIC_PORT_OUTL][i] *= xfade;
				h->port[CYCLIC_PORT_OUTR][i] *= 1.0f - xfade;
			} else if (h->port[CYCLIC_PORT_TREM][0] > 0.0f) /* tremolo */
			{
				xfade = fabsf((float)h->cptr/(float)clen * 2.0f - 1.0f);
				h->port[CYCLIC_PORT_OUTL][i] *= xfade;
				h->port[CYCLIC_PORT_OUTR][i] *= xfade;
			}
		}

		h->bufferl[h->bptr] = inl;
		h->bufferr[h->bptr] = inr;

		h->bptr++;
		if (h->bptr >= h->blen*CYCLIC_BUFFER_CYCLES)
			h->bptr -= h->blen*CYCLIC_BUFFER_CYCLES;

		h->cptr++;
		if (h->cptr >= clen)
		{
			h->cptr -= clen;
			h->oldpitch = h->pitch;
			h->pitch = cyclicGenPitch(h->port[CYCLIC_PORT_PITCH][0]);
		}
	}
}

const LADSPA_Descriptor cyclic_descriptor =
{
	/* UniqueID            */ UID_OFFSET + BUNDLE_INDEX,
	/* Label               */ "cyclic",
	/* Properties          */ LADSPA_PROPERTY_HARD_RT_CAPABLE,
	/* Name                */ "Cyclic Pitch Distortion",
	/* Maker               */ MAKER,
	/* Copyright           */ LICENSE,

	/* PortCount           */ CYCLIC_PORTC,
	/* PortDescriptors     */ cyclic_PortDescriptors,
	/* PortNames           */ cyclic_PortNames,
	/* PortRangeHints      */ cyclic_PortRangeHints,

	/* ImplementationData  */ NULL,
	/* instantiate         */ cyclic_instantiate,
	/* connect_port        */ cyclic_connect_port,
	/* activate            */ cyclic_activate,
	/* run                 */ cyclic_run,
	/* run_adding          */ NULL,
	/* set_run_adding_gain */ NULL,
	/* deactivate          */ NULL,
	/* cleanup             */ cyclic_cleanup,
};
