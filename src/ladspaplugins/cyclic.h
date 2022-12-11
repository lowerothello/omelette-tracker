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

enum {
	CYCLIC_PORT_INL,
	CYCLIC_PORT_INR,
	CYCLIC_PORT_OUTL,
	CYCLIC_PORT_OUTR,
	CYCLIC_PORT_CLEN,
	CYCLIC_PORT_PITCH,
	CYCLIC_PORT_TREM,
} CYCLIC_PORT;


struct CyclicHandle {
	LADSPA_Data  *port[CYCLIC_PORTC];
	float         pitch, oldpitch;     /* oldpitch is needed for ramping to be correct */
	unsigned long clen, oldclen;       /* oldclen is needed for ramping to be correct */
	DelayBuffer  *buffer;              /* as far as this plugin is concerned (buffer->len == buffer->siglen) */
	unsigned long blen;                /* the buffer is overallocated, so store the actual max cycle length */
	unsigned long cptr;                /* cycle pointer, walks up to .cptr*CYCLIC_PORT_CYCLELEN */
	unsigned long latency, oldlatency; /* pitching up requires latency */
};

LADSPA_Handle cyclic_instantiate(const LADSPA_Descriptor *desc, unsigned long rate)
{
	struct CyclicHandle *handle = calloc(1, sizeof(struct CyclicHandle));
	handle->blen = rate * CYCLIC_MAX_DELAY_S;
	handle->buffer = allocDelayBuffer(handle->blen * CYCLIC_BUFFER_CYCLES);
	handle->cptr = 0;

	return handle;
}
void cyclic_cleanup(LADSPA_Handle handle)
{
	struct CyclicHandle *h = handle;
	freeDelayBuffer(h->buffer);
	free(h);
}

float cyclicGenPitch(struct CyclicHandle *h)
{
	return powf(M_12_ROOT_2, h->port[CYCLIC_PORT_PITCH][0]);
}
unsigned long cyclicGenClen(struct CyclicHandle *h)
{
	unsigned long ret = (h->blen * h->port[CYCLIC_PORT_CLEN][0]) / h->pitch;
	return MAX(1, ret); /* avoid dividing by 0 */
}
unsigned long cyclicGenLatency(struct CyclicHandle *h)
{
	unsigned long naturalsample = h->clen + (h->clen>>1);
	unsigned long pitchedsample = naturalsample * h->pitch;

	if (naturalsample >= pitchedsample)
		return 1;
	else
		return (pitchedsample - naturalsample) << 1;
}

void cyclic_activate(LADSPA_Handle handle)
{
	struct CyclicHandle *h = handle;
	h->pitch = h->oldpitch = cyclicGenPitch(h);
	h->clen = h->oldclen = cyclicGenClen(h);
	h->latency = h->oldlatency = cyclicGenLatency(h);
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

		/* pitched pointer offset */
		unsigned long pitchptr = h->cptr * h->pitch;
		unsigned long rpitchptr = (h->oldclen + h->cptr) * h->oldpitch;

		h->port[CYCLIC_PORT_OUTL][i] = readDelayBuffer(h->buffer, h->latency+h->cptr - pitchptr, 0);
		h->port[CYCLIC_PORT_OUTR][i] = readDelayBuffer(h->buffer, h->latency+h->cptr - pitchptr, 1);

		float xfade;
		if (h->cptr < h->clen) /* ramping */
		{
			xfade = (float)h->cptr / (float)h->clen;
			h->port[CYCLIC_PORT_OUTL][i] *= xfade;
			h->port[CYCLIC_PORT_OUTR][i] *= xfade;
			h->port[CYCLIC_PORT_OUTL][i] += readDelayBuffer(h->buffer, h->oldlatency + h->oldclen + h->cptr - rpitchptr, 0) * (1.0f - xfade);
			h->port[CYCLIC_PORT_OUTR][i] += readDelayBuffer(h->buffer, h->oldlatency + h->oldclen + h->cptr - rpitchptr, 1) * (1.0f - xfade);
		}
endramp:

		if (h->clen > CYCLIC_TREM_MIN_CLEN)
		{
			if (h->port[CYCLIC_PORT_TREM][0] > 1.0f) /* autopan */
			{
				xfade = fabsf((float)h->cptr/(float)h->clen * 2.0f - 1.0f);
				h->port[CYCLIC_PORT_OUTL][i] *= xfade;
				h->port[CYCLIC_PORT_OUTR][i] *= 1.0f - xfade;
			} else if (h->port[CYCLIC_PORT_TREM][0] > 0.0f) /* tremolo */
			{
				xfade = fabsf((float)h->cptr/(float)h->clen * 2.0f - 1.0f);
				h->port[CYCLIC_PORT_OUTL][i] *= xfade;
				h->port[CYCLIC_PORT_OUTR][i] *= xfade;
			}
		}


		walkDelayBuffer(h->buffer, inl, inr);

		h->cptr++;
		if (h->cptr >= h->clen)
		{
			h->cptr -= h->clen;
			h->oldpitch = h->pitch;
			h->pitch = cyclicGenPitch(h);
			h->oldclen = h->clen;
			h->clen = cyclicGenClen(h);
			h->oldlatency = h->latency;
			h->latency = cyclicGenLatency(h);
		}
	}
}

const LADSPA_Descriptor cyclic_descriptor =
{
	/* UniqueID            */ UID_OFFSET + BUNDLE_INDEX,
	/* Label               */ "cyclic",
	/* Properties          */ 0,
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
