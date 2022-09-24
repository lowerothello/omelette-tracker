/*
 * type:
 *   - 0 for undefined type
 *
 * state:
 *   pointer to a type-specific struct
 */
typedef struct
{
	uint8_t type;
	void   *state;
} Effect;


#include "distortion.c"


void setEffectType(Effect *e, uint8_t type)
{
	e->type = type;
	if (e->state)
	{
		free(e->state);
		e->state = NULL;
	}

	switch (type)
	{
		case 1: initDistortion(e); break;
	}
}
