#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <ladspa.h>
#include <limits.h>

#define MAX(X, Y) ((X > Y) ? X : Y)
#define MIN(X, Y) ((X < Y) ? X : Y)

#include "../dsp/dsp.c" /* omelette's dsp functions */
#include "../dsp/delaybuffer.c"

#define UID_OFFSET 69420 /* haha funny number */

#define MAKER "libi"
#define LICENSE "CC0"

/* definition soup to set pass state to individual plugins */
#define BUNDLE_INDEX 0
#include "cyclic.h"
#undef BUNDLE_INDEX

#define BUNDLE_INDEX 1
#include "eq.h"
#undef BUNDLE_INDEX

#define BUNDLE_INDEX 2
#include "envelope.h"
#undef BUNDLE_INDEX

#define BUNDLE_INDEX 3
#include "dist.h"
#undef BUNDLE_INDEX

const LADSPA_Descriptor *ladspa_descriptor(unsigned long index)
{
	switch (index)
	{
		case 0: return &cyclic_descriptor; /* super nice cyclic pitch shift */
		case 1: return &eq_descriptor;     /* simple equalizer */
		case 2: return &env_descriptor;    /* VERY unfinished lol */
		case 3: return &dist_descriptor;   /* small clipper, not super interesting yet */
		default: return NULL;
	}
}
