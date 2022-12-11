#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <ladspa.h>
#include <limits.h>

#define MAX(X, Y) ((X > Y) ? X : Y)
#define MIN(X, Y) ((X < Y) ? X : Y)

#include "../dsp.c" /* omelette's dsp functions */
#include "../delaybuffer.c"

#define UID_OFFSET 69420 /* haha funny number */

#define MAKER "libi"
#define LICENSE "CC0"

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

const LADSPA_Descriptor *ladspa_descriptor(unsigned long index)
{
	switch (index)
	{
		case 0: return &cyclic_descriptor;
		case 1: return &eq_descriptor;
		case 2: return &env_descriptor;
		case 3: return &dist_descriptor;
		default: return NULL;
	}
}
