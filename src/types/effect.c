
/* cutoffs between effect widths */
#define EFFECT_WIDTH_CUTOFF_WIDE 64
#define EFFECT_WIDTH_CUTOFF_HUGE 128

/* IMPORTANT NOTE: effects should not register any more than 16 controls */
/* TODO: fix this, controls should be dynamically allocated              */
#include "../effect/distortion.c"
#include "../effect/equalizer.c"

#include "../effect/ladspa.c"

#define EFFECT_TYPE_LADSPA 255
#define EFFECT_TYPE_LV2    254 /* TODO */
#define EFFECT_TYPE_CLAP   253 /* TODO */

void freeEffect(Effect *e)
{
	if (!e || !e->state) return;

	switch (e->type)
	{
		case EFFECT_TYPE_LADSPA: freeLadspaEffect(e); break;
	}
	free(e->state);
	e->state = NULL;
}

EffectChain *newEffectChain(float *input[2], float *output[2])
{
	EffectChain *ret = calloc(1, sizeof(EffectChain));

	if (input) {
		ret->input[0] = input[0];
		ret->input[1] = input[1];
	}

	if (output) {
		ret->output[0] = output[0];
		ret->output[1] = output[1];
	}

	return ret;
}

void clearEffectChain(EffectChain *chain)
{ /* NOTE: NOT thread safe */
	for (int i = 0; i < chain->c; i++)
		freeEffect(&chain->v[i]);
	chain->c = 0;
}


uint8_t getEffectControlCount(Effect *e)
{
	if (e)
		switch (e->type)
		{
			case 1:                  return distortionControlCount;
			case 2:                  return equalizerControlCount;
			case EFFECT_TYPE_LADSPA: return getLadspaEffectControlCount(e);
		}
	return 0;
}

/* pluginindex is only read for external effect types (ladspa, lv2, clap, etc.) */
EffectChain *_addEffect(EffectChain *chain, uint8_t type, unsigned long pluginindex, uint8_t index)
{
	EffectChain *ret = calloc(1, sizeof(EffectChain) + (chain->c+1) * sizeof(Effect));
	memcpy(ret, chain, sizeof(float *) * 4); /* copy input and output */
	ret->c = chain->c + 1;

	if (index)
		memcpy(&ret->v[0],
				&chain->v[0],
				index * sizeof(Effect));

	if (index < chain->c)
		memcpy(&ret->v[index+1],
				&chain->v[index],
				(chain->c - index) * sizeof(Effect));

	ret->v[index].type = type;
	switch (type)
	{
		case 1:                  initDistortion  (&ret->v[index]); break;
		case 2:                  initEqualizer   (&ret->v[index]); break;
		case EFFECT_TYPE_LADSPA: initLadspaEffect(ret, &ret->v[index], ladspa_db.descv[pluginindex]); break;
	}

	return ret;
}
void cb_addEffect(Event *e)
{
	free(e->src); e->src = NULL;
	p->redraw = 1;
}
/* pluginindex is only read for external effect types (ladspa, lv2, clap, etc.) */
void addEffect(EffectChain **chain, uint8_t type, unsigned long pluginindex, uint8_t index, void (*cb)(Event *))
{ /* fully atomic */
	if ((*chain)->c < EFFECT_CHAIN_LEN)
	{
		Event e;
		e.sem = M_SEM_SWAP_REQ;
		e.dest = (void **)chain;
		e.src = _addEffect(*chain, type, pluginindex, index);
		e.callback = cb;
		e.callbackarg = (void *)(size_t)index;
		pushEvent(&e);
	}
}

EffectChain *_delEffect(EffectChain *chain, uint8_t index)
{
	EffectChain *ret = calloc(1, sizeof(EffectChain) + (chain->c-1) * sizeof(Effect));
	memcpy(ret, chain, sizeof(float *) * 4); /* copy input and output */
	ret->c = chain->c - 1;

	if (index)
		memcpy(&ret->v[0],
				&chain->v[0],
				index * sizeof(Effect));

	if (index < chain->c)
		memcpy(&ret->v[index],
				&chain->v[index+1],
				(chain->c - index - 1) * sizeof(Effect));

	return ret;
}
void cb_delEffect(Event *e)
{
	freeEffect(&((EffectChain *)e->src)->v[(size_t)e->callbackarg]);
	free(e->src); e->src = NULL;
	p->redraw = 1;
}
void delEffect(EffectChain **chain, uint8_t index)
{ /* fully atomic */
	if ((*chain)->c)
	{
		Event e;
		e.sem = M_SEM_SWAP_REQ;
		e.dest = (void **)chain;
		e.src = _delEffect(*chain, index);
		e.callback = cb_delEffect;
		e.callbackarg = (void *)(size_t)index;
		pushEvent(&e);
	}
}

/* cursor is (ControlState).cursor compatible */
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

void copyEffect(EffectChain *destchain, Effect *dest, Effect *src)
{
	if (!src) return;

	freeEffect(dest);
	dest->type = src->type;
	switch (src->type)
	{
		case 1:                  copyDistortion(dest, src); break;
		case 2:                  copyEqualizer(dest, src); break;
		case EFFECT_TYPE_LADSPA: copyLadspaEffect(destchain, dest, src); break;
	}
}
void copyEffectChain(EffectChain **dest, EffectChain *src)
{ /* NOT atomic */
	EffectChain *ret = calloc(1, sizeof(EffectChain) + src->c * sizeof(Effect));
	memcpy(ret, *dest, sizeof(float *) * 4); /* copy input and output */
	ret->c = src->c;

	for (uint8_t i = 0; i < src->c; i++)
		copyEffect(ret, &ret->v[i], &src->v[i]);

	clearEffectChain(*dest);
	free(*dest);
	*dest = ret;
}


void serializeEffect(Effect *e, FILE *fp)
{
	if (!e) return;
	fputc(e->type, fp);

	switch (e->type)
	{
		case 1:                  serializeDistortion(e, fp); break;
		case 2:                  serializeEqualizer(e, fp); break;
		case EFFECT_TYPE_LADSPA: serializeLadspaEffect(e, fp); break;
	}
}
void serializeEffectChain(EffectChain *chain, FILE *fp)
{
	fwrite(&chain->c, sizeof(uint8_t), 1, fp);
	for (int i = 0; i < chain->c; i++)
		serializeEffect(&chain->v[i], fp);
}

void deserializeEffect(EffectChain *chain, Effect *e, FILE *fp, uint8_t major, uint8_t minor)
{
	if (!e) return;
	e->type = fgetc(fp);

	switch (e->type)
	{
		case 1:                  deserializeDistortion(e, fp); break;
		case 2:                  deserializeEqualizer(e, fp); break;
		case EFFECT_TYPE_LADSPA: deserializeLadspaEffect(chain, e, fp); break;
	}
}
void deserializeEffectChain(EffectChain **chain, FILE *fp, uint8_t major, uint8_t minor)
{
	uint8_t tempc;
	fread(&tempc, sizeof(uint8_t), 1, fp); /* how many effects to allocate */

	if (*chain) free(*chain);
	*chain = calloc(1, sizeof(EffectChain) + tempc * sizeof(Effect));
	(*chain)->c = tempc;

	for (int i = 0; i < (*chain)->c; i++)
		deserializeEffect(*chain, &(*chain)->v[i], fp, major, minor);
}

short getEffectHeight(Effect *e, short w)
{
	if (e)
		switch (e->type)
		{
			case 1:                  return getDistortionHeight(e, w);
			case 2:                  return getEqualizerHeight(e, w);
			case EFFECT_TYPE_LADSPA: return getLadspaEffectHeight(e, w);
		}
	return 0;
}
int drawEffect(Effect *e, ControlState *cc, bool selected,
		short x, short w,
		short y, short ymin, short ymax)
{
	if (!e) return 0;

	switch (e->type)
	{
		case 1:                  drawDistortion(e, cc, x, w, y, ymin, ymax); break;
		case 2:                  drawEqualizer(e, cc, x, w, y, ymin, ymax); break;
		case EFFECT_TYPE_LADSPA: drawLadspaEffect(e, cc, x, w, y, ymin, ymax); break;
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

/* e->input and e->output should be arrays of at least length samplecount */
void runEffect(uint32_t samplecount, EffectChain *chain, Effect *e)
{
	if (!e) return;
	switch (e->type)
	{
		case 1:                  runDistortion  (samplecount, chain, e); break;
		case 2:                  runEqualizer   (samplecount, chain, e); break;
		case EFFECT_TYPE_LADSPA: runLadspaEffect(samplecount, chain, e); break;
	}
}
