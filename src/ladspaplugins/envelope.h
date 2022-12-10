#define ENV_PORTC 9
const LADSPA_PortDescriptor env_PortDescriptors[ENV_PORTC] =
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
};

const char * const env_PortNames[ENV_PORTC] =
{
	"Audio In L",
	"Audio In R",
	"Audio Out L",
	"Audio Out R",
	"Threshold",
	"Ratio",
	"Attack",
	"Release",
	"Gain Compression",
};

const LADSPA_PortRangeHint env_PortRangeHints[ENV_PORTC] =
{
	{ 0, 0.0f, 0.0f, },
	{ 0, 0.0f, 0.0f, },
	{ 0, 0.0f, 0.0f, },
	{ 0, 0.0f, 0.0f, },
	{ LADSPA_HINT_BOUNDED_BELOW|LADSPA_HINT_BOUNDED_ABOVE|LADSPA_HINT_DEFAULT_MIDDLE, 0.0f, 1.0f, },
	{ LADSPA_HINT_BOUNDED_BELOW|LADSPA_HINT_BOUNDED_ABOVE|LADSPA_HINT_DEFAULT_LOW, 1.0f, 10.0f, },
	{ LADSPA_HINT_BOUNDED_BELOW|LADSPA_HINT_BOUNDED_ABOVE|LADSPA_HINT_DEFAULT_MIDDLE, 0.0f, 1.0f, },
	{ LADSPA_HINT_BOUNDED_BELOW|LADSPA_HINT_BOUNDED_ABOVE|LADSPA_HINT_DEFAULT_MIDDLE, 0.0f, 1.0f, },
	{ LADSPA_HINT_BOUNDED_BELOW|LADSPA_HINT_BOUNDED_ABOVE|LADSPA_HINT_DEFAULT_MIDDLE, -1.0f, 1.0f, },
};

enum {
	ENV_PORT_INL,
	ENV_PORT_INR,
	ENV_PORT_OUTL,
	ENV_PORT_OUTR,
	ENV_PORT_THRESHOLD,
	ENV_PORT_RATIO,
	ENV_PORT_ATTACK,
	ENV_PORT_RELEASE,
	ENV_PORT_GAINCOMP,
} ENV_PORT;


struct envHandle {
	LADSPA_Data *port[ENV_PORTC];
	SVFilter filter;
	float bobber;
	float attack, release; /* minimum to change per sample, scaled between this and 1.0f */
};

#define ENV_MAX_ATTACK_S  0.1f
#define ENV_MAX_RELEASE_S 1.0f
LADSPA_Handle env_instantiate(const LADSPA_Descriptor *desc, unsigned long rate)
{
	struct envHandle *handle = calloc(1, sizeof(struct envHandle));

	handle->attack = 1.0f / (rate * ENV_MAX_ATTACK_S);
	handle->release = 1.0f / (rate * ENV_MAX_RELEASE_S);

	return handle;
}
void env_cleanup(LADSPA_Handle handle)
{
	free(handle);
}

void env_activate(LADSPA_Handle handle)
{
	struct envHandle *h = handle;
	memset(&h->filter, 0, sizeof(SVFilter));
	h->bobber = 0.0f;
}

void env_connect_port(LADSPA_Handle handle, unsigned long i, LADSPA_Data *data)
{
	struct envHandle *h = handle;
	h->port[i] = data;
}

void env_run(LADSPA_Handle handle, unsigned long bufsize)
{
	struct envHandle *h = handle;

	float target;
	for (unsigned long i = 0; i < bufsize; i++)
	{
		/* nudge the bobber */
		/* average input */
		target = fabsf((h->port[ENV_PORT_INL][i] + h->port[ENV_PORT_INR][i]) * 0.5f);

		/* clamp to just what's above the threshold */
		target = MAX(0.0f, target - h->port[ENV_PORT_THRESHOLD][0]);

		/* apply the ratio */
		target *= h->port[ENV_PORT_RATIO][0];

		if (h->bobber < target) /* attack */
			h->bobber += 1.0f - ((1.0f - h->attack) * h->port[ENV_PORT_ATTACK][0]);
		else /* release */
			h->bobber -= 1.0f - ((1.0f - h->release) * h->port[ENV_PORT_RELEASE][0]);
		h->bobber = MIN(1.0f, MAX(0.0f, h->bobber));


		h->port[ENV_PORT_OUTL][i] = h->port[ENV_PORT_INL][i];
		h->port[ENV_PORT_OUTR][i] = h->port[ENV_PORT_INR][i];

		h->port[ENV_PORT_OUTL][i] *= 1.0f - h->bobber;
		h->port[ENV_PORT_OUTR][i] *= 1.0f - h->bobber;
	}
}

const LADSPA_Descriptor env_descriptor =
{
	/* UniqueID            */ UID_OFFSET + BUNDLE_INDEX,
	/* Label               */ "envelope",
	/* Properties          */ LADSPA_PROPERTY_HARD_RT_CAPABLE,
	/* Name                */ "Envelope Follower",
	/* Maker               */ MAKER,
	/* Copyright           */ LICENSE,

	/* PortCount           */ ENV_PORTC,
	/* PortDescriptors     */ env_PortDescriptors,
	/* PortNames           */ env_PortNames,
	/* PortRangeHints      */ env_PortRangeHints,

	/* ImplementationData  */ NULL,
	/* instantiate         */ env_instantiate,
	/* connect_port        */ env_connect_port,
	/* activate            */ env_activate,
	/* run                 */ env_run,
	/* run_adding          */ NULL,
	/* set_run_adding_gain */ NULL,
	/* deactivate          */ NULL,
	/* cleanup             */ env_cleanup,
};
