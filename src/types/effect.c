#include "../effect/distortion.c"

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

int drawEffect(Effect *e, ControlState *cc,
		short x, short y, short w, short ymin, short ymax)
{
	switch (e->type)
	{
		case 1: return drawDistortion(e, cc, x, y, w, ymin, ymax);
	}
	return 0;
}

void stepEffect(Effect *e, float *l, float *r)
{
	switch (e->type)
	{
		case 1: stepDistortion(e, l, r); break;
	}
}
