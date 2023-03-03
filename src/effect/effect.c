#include "ladspa.c"
#include "lv2.c"

/* IMPORTANT NOTE: effects should not register any more than 16 controls */
/* TODO: fix this, controls should be dynamically allocated              */

void freeEffect(Effect *e)
{
	if (!e) return;

	switch (e->type)
	{
		case EFFECT_TYPE_DUMMY:  break;
		case EFFECT_TYPE_LADSPA: freeLadspaEffect(&e->ladspa); break;
		case EFFECT_TYPE_LV2:    freeLV2Effect   (&e->lv2);    break;
	}
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
			case EFFECT_TYPE_DUMMY:  return 1;
			case EFFECT_TYPE_LADSPA: return getLadspaEffectControlCount(&e->ladspa);
			case EFFECT_TYPE_LV2:    return getLV2EffectControlCount   (&e->lv2);
		}
	return 0;
}

EffectChain *_addEffect(EffectChain *chain, unsigned long pluginindex, uint8_t index)
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

	if (!pluginindex) /* paste effect */
	{
		copyEffect(&ret->v[index], &w->effectbuffer, chain->input, chain->output);
		return ret;
	}

	pluginindex--; /* apply the offset */
	if (pluginindex < ladspa_db.descc) /* ladspa */
	{
		ret->v[index].type = EFFECT_TYPE_LADSPA;
		initLadspaEffect(&ret->v[index].ladspa, ret->input, ret->output, ladspa_db.descv[pluginindex]);
	} else /* lv2 */
	{
		const LilvPlugins *lap = lilv_world_get_all_plugins(lv2_db.world);
		uint32_t i = 0;
		LILV_FOREACH(plugins, iter, lap)
			if (i == pluginindex - ladspa_db.descc)
			{
				ret->v[index].type = EFFECT_TYPE_LV2;
				initLV2Effect(&ret->v[index].lv2, ret->input, ret->output, lilv_plugins_get(lap, iter));
				break;
			} else i++;
	}

	return ret;
}
void cb_addEffect(Event *e)
{
	free(e->src); e->src = NULL;
	p->redraw = 1;
}

/* pluginindex is offset by 1, pluginindex==0 will paste */
void addEffect(EffectChain **chain, unsigned long pluginindex, uint8_t index, void (*cb)(Event*))
{ /* fully atomic */
	if ((*chain)->c < EFFECT_CHAIN_LEN)
	{
		Event e;
		e.sem = M_SEM_SWAP_REQ;
		e.dest = (void **)chain;
		e.src = _addEffect(*chain, pluginindex, index);
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
static void cb_delEffect(Event *e)
{
	freeEffect(&((EffectChain *)e->src)->v[(size_t)e->callbackarg]);
	free(e->src); e->src = NULL;
	p->redraw = 1;
}
void delEffect(EffectChain **chain, uint8_t index)
{ /* fully atomic */
	if ((*chain)->c)
	{
		copyEffect(&w->effectbuffer, &(*chain)->v[index], NULL, NULL);
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

void copyEffect(Effect *dest, Effect *src, float **input, float **output)
{
	if (!src) return;

	freeEffect(dest);
	switch (src->type)
	{
		case EFFECT_TYPE_DUMMY:  break;
		case EFFECT_TYPE_LADSPA: copyLadspaEffect(&dest->ladspa, &src->ladspa, input, output); break;
		case EFFECT_TYPE_LV2:    copyLV2Effect   (&dest->lv2, &src->lv2, input, output); break;
	}
}
void copyEffectChain(EffectChain **dest, EffectChain *src)
{ /* NOT atomic */
	EffectChain *ret = calloc(1, sizeof(EffectChain) + src->c * sizeof(Effect));
	memcpy(ret, *dest, sizeof(float *) * 4); /* copy input and output */
	ret->c = src->c;

	for (uint8_t i = 0; i < src->c; i++)
		copyEffect(&ret->v[i], &src->v[i], NULL, NULL);

	clearEffectChain(*dest);
	free(*dest);
	*dest = ret;
}

/* e->input[0/1] and e->output[0/1] should be arrays of at least length samplecount */
void runEffect(uint32_t samplecount, EffectChain *chain, Effect *e)
{
	if (!e) return;
	switch (e->type)
	{
		case EFFECT_TYPE_DUMMY:  break;
		case EFFECT_TYPE_LADSPA: runLadspaEffect(samplecount, &e->ladspa, chain->input, chain->output); break;
		case EFFECT_TYPE_LV2:    runLV2Effect   (samplecount, &e->lv2, chain->input, chain->output);    break;
	}
}
