/*
 * equalizer LADSPA plugin
 *   small filterbank plugin for mutating the frequency balance of a signal.
 *
 *   low/high bands:
 *     CUTOFF control
 *     shelf filter with a BOOST control, at CUTOFF
 *     pass filter with a ROLLOFF control
 *       as ROLLOFF increases, the filter mix is increased and
 *       the filter cutoff moves from the extreme towards the
 *       band cutoff
 *
 *   mid band:
 *     BOOST control
 */

#define EQ_PORTC 12
const LADSPA_PortDescriptor eq_PortDescriptors[EQ_PORTC] =
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
	LADSPA_PORT_INPUT|LADSPA_PORT_CONTROL,
};

const char * const eq_PortNames[EQ_PORTC] =
{
	"Audio In L",
	"Audio In R",
	"Audio Out L",
	"Audio Out R",
	"Output Gain",
	"Low Gain",
	"Low Rolloff",
	"Low Cutoff",
	"Mid Gain",
	"High Gain",
	"High Rolloff",
	"High Cutoff",
};

const LADSPA_PortRangeHint eq_PortRangeHints[EQ_PORTC] =
{
	{ 0, 0.0f, 0.0f, },
	{ 0, 0.0f, 0.0f, },
	{ 0, 0.0f, 0.0f, },
	{ 0, 0.0f, 0.0f, },
	{ LADSPA_HINT_BOUNDED_BELOW|LADSPA_HINT_BOUNDED_ABOVE|LADSPA_HINT_DEFAULT_MIDDLE, -1.0f, 1.0f, }, /* OUT  gain    */

	{ LADSPA_HINT_BOUNDED_BELOW|LADSPA_HINT_BOUNDED_ABOVE|LADSPA_HINT_DEFAULT_MIDDLE, -1.0f, 1.0f, }, /*      gain    */
	{ LADSPA_HINT_BOUNDED_BELOW|LADSPA_HINT_BOUNDED_ABOVE|LADSPA_HINT_DEFAULT_MINIMUM, 0.0f, 1.0f, }, /* LOW  rolloff */
	{ LADSPA_HINT_BOUNDED_BELOW|LADSPA_HINT_BOUNDED_ABOVE|LADSPA_HINT_DEFAULT_LOW,     0.0f, 1.0f, }, /*      cutoff  */

	{ LADSPA_HINT_BOUNDED_BELOW|LADSPA_HINT_BOUNDED_ABOVE|LADSPA_HINT_DEFAULT_MIDDLE, -1.0f, 1.0f, }, /* MID  gain    */

	{ LADSPA_HINT_BOUNDED_BELOW|LADSPA_HINT_BOUNDED_ABOVE|LADSPA_HINT_DEFAULT_MIDDLE, -1.0f, 1.0f, }, /*      gain    */
	{ LADSPA_HINT_BOUNDED_BELOW|LADSPA_HINT_BOUNDED_ABOVE|LADSPA_HINT_DEFAULT_MINIMUM, 0.0f, 1.0f, }, /* HIGH rolloff */
	{ LADSPA_HINT_BOUNDED_BELOW|LADSPA_HINT_BOUNDED_ABOVE|LADSPA_HINT_DEFAULT_HIGH,    0.0f, 1.0f, }, /*      cutoff  */
};

#define EQ_PORT_INL 0
#define EQ_PORT_INR 1
#define EQ_PORT_OUTL 2
#define EQ_PORT_OUTR 3
#define EQ_PORT_OUT_GAIN 4
#define EQ_PORT_LOW_GAIN 5
#define EQ_PORT_LOW_ROLLOFF 6
#define EQ_PORT_LOW_CUTOFF 7
#define EQ_PORT_MID_GAIN 8
#define EQ_PORT_HIGH_GAIN 9
#define EQ_PORT_HIGH_ROLLOFF 10
#define EQ_PORT_HIGH_CUTOFF 11


#define EQ_FILTERC 8
struct EQHandle {
	LADSPA_Data *port[EQ_PORTC];
	SVFilter filter[EQ_FILTERC];
};

LADSPA_Handle eq_instantiate(const LADSPA_Descriptor *desc, unsigned long rate)
{
	struct EQHandle *handle = calloc(1, sizeof(struct EQHandle));

	return handle;
}
void eq_cleanup(LADSPA_Handle handle)
{
	struct EQHandle *h = handle;
	free(h);
}

void eq_activate(LADSPA_Handle handle)
{
	struct EQHandle *h = handle;
	memset(&h->filter, 0, sizeof(SVFilter) * EQ_FILTERC);
}

void eq_connect_port(LADSPA_Handle handle, unsigned long i, LADSPA_Data *data)
{
	struct EQHandle *h = handle;
	h->port[i] = data;
}

#define EQ_RESONANCE 0.0f
#define EQ_GAINSCALE 8.0f
void eq_run(LADSPA_Handle handle, unsigned long bufsize)
{
	struct EQHandle *h = handle;

	float outl, outr;
	for (unsigned long i = 0; i < bufsize; i++)
	{
		outl = h->port[EQ_PORT_INL][i];
		outr = h->port[EQ_PORT_INR][i];
		runSVFilter(&h->filter[0], outl, h->port[EQ_PORT_LOW_CUTOFF][0], EQ_RESONANCE);
		runSVFilter(&h->filter[1], outr, h->port[EQ_PORT_LOW_CUTOFF][0], EQ_RESONANCE);
		runSVFilter(&h->filter[2], outl, h->port[EQ_PORT_HIGH_CUTOFF][0], EQ_RESONANCE);
		runSVFilter(&h->filter[3], outr, h->port[EQ_PORT_HIGH_CUTOFF][0], EQ_RESONANCE);

		/* mid band */
		outl = hardclip((outl - h->filter[0].l - h->filter[2].h) * powf(2.0f, h->port[EQ_PORT_MID_GAIN][0]*EQ_GAINSCALE));
		outr = hardclip((outr - h->filter[1].l - h->filter[3].h) * powf(2.0f, h->port[EQ_PORT_MID_GAIN][0]*EQ_GAINSCALE));

		/* high/low bands */
		outl +=
			hardclip(h->filter[0].l * powf(2.0f, h->port[EQ_PORT_LOW_GAIN ][0]*EQ_GAINSCALE)) +
			hardclip(h->filter[2].h * powf(2.0f, h->port[EQ_PORT_HIGH_GAIN][0]*EQ_GAINSCALE));
		outr +=
			hardclip(h->filter[1].l * powf(2.0f, h->port[EQ_PORT_LOW_GAIN ][0]*EQ_GAINSCALE)) +
			hardclip(h->filter[3].h * powf(2.0f, h->port[EQ_PORT_HIGH_GAIN][0]*EQ_GAINSCALE));

		/* rolloff */
		/* low cut */
		runSVFilter(&h->filter[4], outl, h->port[EQ_PORT_LOW_CUTOFF][0]*h->port[EQ_PORT_LOW_ROLLOFF][0], EQ_RESONANCE);
		runSVFilter(&h->filter[5], outr, h->port[EQ_PORT_LOW_CUTOFF][0]*h->port[EQ_PORT_LOW_ROLLOFF][0], EQ_RESONANCE);
		outl = outl * (1.0f - h->port[EQ_PORT_LOW_ROLLOFF][0]) + h->filter[4].h * h->port[EQ_PORT_LOW_ROLLOFF][0];
		outr = outr * (1.0f - h->port[EQ_PORT_LOW_ROLLOFF][0]) + h->filter[5].h * h->port[EQ_PORT_LOW_ROLLOFF][0];

		/* high cut */
		runSVFilter(&h->filter[6], outl, 1.0f - (1.0f - h->port[EQ_PORT_HIGH_CUTOFF][0])*h->port[EQ_PORT_HIGH_ROLLOFF][0], EQ_RESONANCE);
		runSVFilter(&h->filter[7], outr, 1.0f - (1.0f - h->port[EQ_PORT_HIGH_CUTOFF][0])*h->port[EQ_PORT_HIGH_ROLLOFF][0], EQ_RESONANCE);
		outl = outl * (1.0f - h->port[EQ_PORT_HIGH_ROLLOFF][0]) + h->filter[6].l * h->port[EQ_PORT_HIGH_ROLLOFF][0];
		outr = outr * (1.0f - h->port[EQ_PORT_HIGH_ROLLOFF][0]) + h->filter[7].l * h->port[EQ_PORT_HIGH_ROLLOFF][0];

		h->port[EQ_PORT_OUTL][i] = hardclip(outl * powf(2.0f, h->port[EQ_PORT_OUT_GAIN][0]*EQ_GAINSCALE));
		h->port[EQ_PORT_OUTR][i] = hardclip(outr * powf(2.0f, h->port[EQ_PORT_OUT_GAIN][0]*EQ_GAINSCALE));
	}
}

const LADSPA_Descriptor eq_descriptor =
{
	/* UniqueID            */ UID_OFFSET + BUNDLE_INDEX,
	/* Label               */ "eq",
	/* Properties          */ LADSPA_PROPERTY_HARD_RT_CAPABLE,
	/* Name                */ "Frequency Balance",
	/* Maker               */ MAKER,
	/* Copyright           */ LICENSE,

	/* PortCount           */ EQ_PORTC,
	/* PortDescriptors     */ eq_PortDescriptors,
	/* PortNames           */ eq_PortNames,
	/* PortRangeHints      */ eq_PortRangeHints,

	/* ImplementationData  */ NULL,
	/* instantiate         */ eq_instantiate,
	/* connect_port        */ eq_connect_port,
	/* activate            */ eq_activate,
	/* run                 */ eq_run,
	/* run_adding          */ NULL,
	/* set_run_adding_gain */ NULL,
	/* deactivate          */ NULL,
	/* cleanup             */ eq_cleanup,
};
