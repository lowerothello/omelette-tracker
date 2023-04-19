#define DIST_PORTC 11
const LADSPA_PortDescriptor dist_PortDescriptors[DIST_PORTC] =
{
	LADSPA_PORT_INPUT|LADSPA_PORT_AUDIO,
	LADSPA_PORT_INPUT|LADSPA_PORT_AUDIO,
	LADSPA_PORT_OUTPUT|LADSPA_PORT_AUDIO,
	LADSPA_PORT_OUTPUT|LADSPA_PORT_AUDIO,
	LADSPA_PORT_INPUT|LADSPA_PORT_CONTROL,
	LADSPA_PORT_INPUT|LADSPA_PORT_CONTROL,
	LADSPA_PORT_INPUT|LADSPA_PORT_CONTROL,
	LADSPA_PORT_INPUT|LADSPA_PORT_CONTROL,
	LADSPA_PORT_INPUT|LADSPA_PORT_CONTROL,
	LADSPA_PORT_INPUT|LADSPA_PORT_CONTROL,
	LADSPA_PORT_INPUT|LADSPA_PORT_CONTROL,
};

const char * const dist_PortNames[DIST_PORTC] =
{
	"Audio In L",
	"Audio In R",
	"Audio Out L",
	"Audio Out R",
	"Output Gain",
	"Drive",
	"Bias",
	"Stereo Bias",
	"Rectify Mix",
	"Rectifiers",
	"Gate",
};

const LADSPA_PortRangeHint dist_PortRangeHints[DIST_PORTC] =
{
	{ 0, 0.0f, 0.0f, },
	{ 0, 0.0f, 0.0f, },
	{ 0, 0.0f, 0.0f, },
	{ 0, 0.0f, 0.0f, },
	{ LADSPA_HINT_BOUNDED_BELOW|LADSPA_HINT_BOUNDED_ABOVE|LADSPA_HINT_DEFAULT_MAXIMUM,  0.0f, 1.0f, },
	{ LADSPA_HINT_BOUNDED_BELOW|LADSPA_HINT_BOUNDED_ABOVE|LADSPA_HINT_DEFAULT_MINIMUM,  0.0f, 1.0f, },
	{ LADSPA_HINT_BOUNDED_BELOW|LADSPA_HINT_BOUNDED_ABOVE|LADSPA_HINT_DEFAULT_MIDDLE,  -1.0f, 1.0f, },
	{ LADSPA_HINT_TOGGLED|LADSPA_HINT_DEFAULT_0, 0.0f, 1.0f, },
	{ LADSPA_HINT_BOUNDED_BELOW|LADSPA_HINT_BOUNDED_ABOVE|LADSPA_HINT_DEFAULT_MINIMUM,  0.0f, 1.0f, },
	{ LADSPA_HINT_BOUNDED_BELOW|LADSPA_HINT_BOUNDED_ABOVE|LADSPA_HINT_DEFAULT_MINIMUM|LADSPA_HINT_INTEGER, 1.0f, 7.0f, },
	{ LADSPA_HINT_BOUNDED_BELOW|LADSPA_HINT_BOUNDED_ABOVE|LADSPA_HINT_DEFAULT_MINIMUM,  0.0f, 1.0f, },
};

enum {
	DIST_PORT_INL,
	DIST_PORT_INR,
	DIST_PORT_OUTL,
	DIST_PORT_OUTR,
	DIST_PORT_OUTGAIN,
	DIST_PORT_DRIVE,
	DIST_PORT_BIAS,
	DIST_PORT_BIASSTEREO,
	DIST_PORT_RECTIFYMIX,
	DIST_PORT_RECTIFIERS,
	DIST_PORT_GATE,
} DIST_PORT;


struct DistHandle {
	LADSPA_Data *port[DIST_PORTC];
	float        dcblockinput[2];
	float        dcblockoutput[2];
	float        wetrms, dryrms;
	float        dcblockcutoff;
};

LADSPA_Handle dist_instantiate(const LADSPA_Descriptor *desc, unsigned long rate)
{
	struct DistHandle *ret = calloc(1, sizeof(struct CyclicHandle));
	ret->dcblockcutoff = 1.0f - (130.0f / rate);
	return ret;
}

void dist_activate(LADSPA_Handle handle)
{
	struct DistHandle *h = handle;
	memset(&h->dcblockinput, 0, sizeof(float) * 2);
	memset(&h->dcblockoutput, 0, sizeof(float) * 2);
}

void dist_cleanup(LADSPA_Handle handle)
{
	free(handle);
}

void dist_connect_port(LADSPA_Handle handle, unsigned long i, LADSPA_Data *data)
{
	struct DistHandle *h = handle;
	h->port[i] = data;
}

float rectify(float input) { return (fabsf(input) * 2.0f) - 1.0f; }

#define DIST_MAX_DRIVE 100.0f
void dist_run(LADSPA_Handle handle, unsigned long bufsize)
{
	struct DistHandle *h = handle;

	LADSPA_Data inl, inr;
	float drive, rect;
	unsigned int j;
	for (unsigned long i = 0; i < bufsize; i++)
	{
		inl = h->port[DIST_PORT_INL][i];
		inr = h->port[DIST_PORT_INR][i];

		h->dryrms = (h->dryrms + powf(fabsf(inl + inr)*0.5f, 2.0f))*0.5f;

		inl += h->port[DIST_PORT_BIAS][0];

		if (h->port[DIST_PORT_BIASSTEREO][0] > 0.0f)
			inr += h->port[DIST_PORT_BIAS][0] * -1.0f;
		else
			inr += h->port[DIST_PORT_BIAS][0];

		if (fabsf(inl) < h->port[DIST_PORT_GATE][0])
			inl = 0.0f;
		else
		{
			rect = inl;
			for (j = 0; j < h->port[DIST_PORT_RECTIFIERS][0]; j++)
				rect = rectify(rect);
			inl = rect * h->port[DIST_PORT_RECTIFYMIX][0] + inl * (1.0f - h->port[DIST_PORT_RECTIFYMIX][0]);
		}

		if (fabsf(inr) < h->port[DIST_PORT_GATE][0])
			inr = 0.0f;
		else
		{
			rect = inr;
			for (j = 0; j < h->port[DIST_PORT_RECTIFIERS][0]; j++)
				rect = rectify(rect);
			inr = rect * h->port[DIST_PORT_RECTIFYMIX][0] + inr * (1.0f - h->port[DIST_PORT_RECTIFYMIX][0]);
		}

		drive = (h->port[DIST_PORT_DRIVE][0] * DIST_MAX_DRIVE) + 1.0f;
		// drive = powf(DIST_MAX_DRIVE, h->port[DIST_PORT_DRIVE][0]);
		inl = hardclip(inl * drive);
		inr = hardclip(inr * drive);

		h->wetrms = (h->wetrms + powf(fabsf(inl + inr)*0.5f, 2.0f))*0.5f;

		inl -= h->wetrms - h->dryrms;
		inr -= h->wetrms - h->dryrms;

		inl *= h->port[DIST_PORT_OUTGAIN][0];
		inr *= h->port[DIST_PORT_OUTGAIN][0];

		h->dcblockoutput[0] = inl - h->dcblockinput[0] + h->dcblockcutoff * h->dcblockoutput[0];
		h->dcblockoutput[1] = inr - h->dcblockinput[1] + h->dcblockcutoff * h->dcblockoutput[1];
		h->dcblockinput[0] = inl;
		h->dcblockinput[1] = inr;

		h->port[DIST_PORT_OUTL][i] = h->dcblockoutput[0];
		h->port[DIST_PORT_OUTR][i] = h->dcblockoutput[1];
	}
}

const LADSPA_Descriptor dist_descriptor =
{
	/* UniqueID            */ UID_OFFSET + BUNDLE_INDEX,
	/* Label               */ "dist",
	/* Properties          */ LADSPA_PROPERTY_HARD_RT_CAPABLE,
	/* Name                */ "Clipper",
	/* Maker               */ MAKER,
	/* Copyright           */ LICENSE,

	/* PortCount           */ DIST_PORTC,
	/* PortDescriptors     */ dist_PortDescriptors,
	/* PortNames           */ dist_PortNames,
	/* PortRangeHints      */ dist_PortRangeHints,

	/* ImplementationData  */ NULL,
	/* instantiate         */ dist_instantiate,
	/* connect_port        */ dist_connect_port,
	/* activate            */ dist_activate,
	/* run                 */ dist_run,
	/* run_adding          */ NULL,
	/* set_run_adding_gain */ NULL,
	/* deactivate          */ NULL,
	/* cleanup             */ dist_cleanup,
};
