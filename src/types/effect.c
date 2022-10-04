
/* cutoffs between effect widths */
#define EFFECT_WIDTH_CUTOFF_WIDE 64
#define EFFECT_WIDTH_CUTOFF_HUGE 128

/* IMPORTANT NOTE: effects can NOT register any more than 16 controls! */
/* TODO: fix this, controls should be dynamically allocated            */
#include "../effect/distortion.c"
#include "../effect/equalizer.c"


uint8_t getEffectControlCount(Effect *e)
{
	switch (e->type)
	{
		case 1: return distortionControlCount;
		case 2: return equalizerControlCount;
	} return 0; /* fallback */
}

void freeEffect(Effect *e)
{
	if (!e->state) return;

	switch (e->type)
	{
		case 1: break;
		case 2: break;
	}
	free(e->state);
	e->state = NULL;
}
void setEffectType(Effect *e, uint8_t type)
{
	freeEffect(e);

	e->type = type;
	switch (type)
	{
		case 1: initDistortion(e); break;
		case 2: initEqualizer(e); break;
	}
}
void addEffect(EffectChain *chain, uint8_t type, uint8_t index)
{
	if (chain->c < EFFECT_CHAIN_LEN)
	{
		for (int i = chain->c; i > index; i--) /* make space for the new index */
			chain->v[i] = chain->v[i-1];

		chain->v[index].state = NULL;
		setEffectType(&chain->v[index], type);

		chain->c++;
	}
}
void delEffect(EffectChain *chain, uint8_t index)
{
	if (chain->c)
	{
		freeEffect(&chain->v[index]);
		for (int i = index; i < chain->c; i++) /* fill the gap left by the old index */
			chain->v[i] = chain->v[i+1];

		chain->c--;
	}
}

/* cursor should be ((ControlState)cc).cursor compatible */
uint8_t getEffectFromCursor(EffectChain *chain, uint8_t cursor)
{
	uint8_t offset = 0;
	for (int i = 0; i < chain->c; i++)
	{
		offset += getEffectControlCount(&chain->v[i]);
		if (offset > cursor) return i;
	} return 0; /* fallback */
}

uint8_t getCursorFromEffect(EffectChain *chain, uint8_t index)
{
	uint8_t offset = 0;
	for (int i = 0; i < MIN(index, chain->c); i++)
		offset += getEffectControlCount(&chain->v[i]);
	return offset;
}

void copyEffect(Effect *dest, Effect *src)
{
	setEffectType(dest, src->type);

	switch (src->type)
	{
		case 1: copyDistortion(dest, src); break;
		case 2: copyEqualizer(dest, src); break;
	}
}

void serializeEffect(Effect *e, FILE *fp)
{
	fputc(e->type, fp);

	switch (e->type)
	{
		case 1: serializeDistortion(e, fp); break;
		case 2: serializeEqualizer(e, fp); break;
	}
}
void deserializeEffect(Effect *e, FILE *fp, uint8_t major, uint8_t minor)
{
	e->type = fgetc(fp);

	switch (e->type)
	{
		case 1: initDistortion(e); deserializeDistortion(e, fp); break;
		case 2: initEqualizer(e); deserializeEqualizer(e, fp); break;
	}
}

short getEffectHeight(Effect *e, short w)
{
	switch (e->type)
	{
		case 1: return getDistortionHeight(e, w);
		case 2: return getEqualizerHeight(e, w);
	} return 0; /* fallback */
}
int drawEffect(Effect *e, ControlState *cc, bool selected,
		short x, short w,
		short y, short ymin, short ymax)
{
	switch (e->type)
	{
		case 1: drawDistortion(e, cc, x, w, y, ymin, ymax); break;
		case 2: drawEqualizer(e, cc, x, w, y, ymin, ymax); break;
	}
	short ret = getEffectHeight(e, w);

	y--;
	if (selected) printf("\033[1m");
	if (ymin <= y && ymax >= y)
	{
		for (int i = 0; i < w; i++)
			printf("\033[%d;%dH─", y, x+i);
		printf("\033[%d;%dH┌\033[%d;%dH┒",
				y, x-1,
				y, x+w);
	}
	if (ymin <= y+ret-1 && ymax >= y+ret-1)
	{
		for (int i = 0; i < w; i++)
			printf("\033[%d;%dH━", y+ret-1, x+i);
		printf("\033[%d;%dH┕\033[%d;%dH┛",
				y+ret-1, x-1,
				y+ret-1, x+w);
	}
	for (int i = 1; i < ret-1; i++)
	{
		if (ymin <= y+i && ymax >= y+i)
			printf("\033[%d;%dH│\033[%d;%dH┃",
					y+i, x-1,
					y+i, x+w);
	}
	if (selected) printf("\033[22m");

	return ret;
}

void stepEffect(Effect *e, float *l, float *r)
{
	switch (e->type)
	{
		case 1: stepDistortion(e, l, r); break;
		case 2: stepEqualizer(e, l, r); break;
	}
}
