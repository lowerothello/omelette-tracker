#include "dummy.c"

#ifdef OML_LADSPA
#include "ladspa.c"
#endif

#ifdef OML_LILV_0
#include "lv2.c"
#endif


EffectAPI *effectGetAPI(void)
{
	EffectAPI *ret = calloc(EFFECT_TYPE_COUNT, sizeof(EffectAPI));

	ret[EFFECT_TYPE_DUMMY] = dummy_effect_api;

#ifdef OML_LADSPA
	ret[EFFECT_TYPE_LADSPA] = ladspa_effect_api;
#endif

#ifdef OML_LILV_0
	ret[EFFECT_TYPE_LV2] = lv2_effect_api;
#endif

	return ret;
}

EffectAPI *effect_api;
void initEffectDB(void)
{
	effect_api = effectGetAPI();

	for (EffectType i = 0; i < EFFECT_TYPE_COUNT; i++)
		if (effect_api[i].init_db)
			effect_api[i].init_db();
}

void freeEffectDB(void)
{
	for (EffectType i = 0; i < EFFECT_TYPE_COUNT; i++)
		if (effect_api[i].free_db)
			effect_api[i].free_db();

	free(effect_api);
}


/* IMPORTANT NOTE: effects should not register any more than 16 controls, TODO: fix this, controls should be dynamically allocated */

void freeEffect(Effect *e)
{
	if (!e) return;

	if (effect_api[e->type].free)
		effect_api[e->type].free(e->state);
}

EffectChain *newEffectChain(void)
{
	EffectChain *ret = calloc(1, sizeof(EffectChain));
	ret->input[0] = calloc(buffersize, sizeof(float));
	ret->input[1] = calloc(buffersize, sizeof(float));
	ret->output[0] = calloc(buffersize, sizeof(float));
	ret->output[1] = calloc(buffersize, sizeof(float));

	ret->fader = 0xff;

	return ret;
}

void freeEffectChain(EffectChain *chain)
{
	clearEffectChain(chain);

	if (chain->input[0]) { free(chain->input[0]); chain->input[0] = NULL; }
	if (chain->input[1]) { free(chain->input[1]); chain->input[1] = NULL; }
	if (chain->output[0]) { free(chain->output[0]); chain->output[0] = NULL; }
	if (chain->output[1]) { free(chain->output[1]); chain->output[1] = NULL; }

	free(chain);
}
void clearEffectChain(EffectChain *chain)
{ /* NOTE: NOT thread safe */
	for (int i = 0; i < chain->c; i++)
		freeEffect(&chain->v[i]);
	chain->c = 0;
}

#define EFFECT_IMPLIED_CONTROLS 2
uint32_t getEffectControlCount(Effect *e)
{
	if (e && effect_api[e->type].controlc)
		return effect_api[e->type].controlc(e->state) + EFFECT_IMPLIED_CONTROLS;
	return EFFECT_IMPLIED_CONTROLS;
}

static EffectChain *_addEffect(EffectChain *chain, EffectType type, uint32_t srcindex, uint8_t destindex)
{
	EffectChain *ret = calloc(1, sizeof(EffectChain) + (chain->c+1)*sizeof(Effect));
	memcpy(&ret->input,  &chain->input,  sizeof(float *)*2);
	memcpy(&ret->output, &chain->output, sizeof(float *)*2);
	ret->c = chain->c + 1;

	if (destindex)
		memcpy(&ret->v[0],
				&chain->v[0],
				destindex * sizeof(Effect));

	if (destindex < chain->c)
		memcpy(&ret->v[destindex+1],
				&chain->v[destindex],
				(chain->c - destindex) * sizeof(Effect));

	if (srcindex == (uint32_t)-1) /* paste effect */
	{
		copyEffect(&ret->v[destindex], &w->effectbuffer, chain->input, chain->output);
		return ret;
	}

	ret->v[destindex].type = type;
	if (effect_api[type].init && effect_api[type].db_line)
	{
		/* only care about line.data */
		EffectBrowserLine line = effect_api[type].db_line(srcindex);
		free(line.name);
		free(line.maker);

		ret->v[destindex].state = effect_api[type].init(line.data, ret->input, ret->output);
	}

	return ret;
}
static void cb_addEffect(Event *e)
{
	cc.cursor = getCursorFromEffect(w->track, s->track->v[w->track]->effect, (uint8_t)(size_t)e->callbackarg);
	free(e->src); e->src = NULL;
	p->redraw = 1;
}
/* .srcindex == ((uint32_t)-1) to paste */
void addEffect(EffectChain **chain, EffectType type, uint32_t srcindex, uint8_t destindex)
{ /* fully atomic */
	if ((*chain)->c < EFFECT_CHAIN_LEN)
	{
		Event e;
		e.sem = M_SEM_SWAP_REQ;
		e.dest = (void **)chain;
		e.src = _addEffect(*chain, type, srcindex, destindex);
		e.callback = cb_addEffect;
		e.callbackarg = (void *)(size_t)destindex;
		pushEvent(&e);
	}
}

static EffectChain *_swapEffect(EffectChain *chain, uint8_t s1, uint8_t s2)
{
	EffectChain *ret = calloc(1, sizeof(EffectChain) + chain->c*sizeof(Effect));
	memcpy(ret, chain, sizeof(EffectChain) + chain->c*sizeof(Effect));

	memcpy(&ret->v[s1], &chain->v[s2], sizeof(Effect));
	memcpy(&ret->v[s2], &chain->v[s1], sizeof(Effect));

	return ret;
}
void swapEffect(EffectChain **chain, uint8_t s1, uint8_t s2)
{ /* fully atomic */
	if ((*chain)->c < EFFECT_CHAIN_LEN)
	{
		Event e;
		e.sem = M_SEM_SWAP_REQ;
		e.dest = (void **)chain;
		e.src = _swapEffect(*chain, s1, s2);
		e.callback = cb_addEffect;
		e.callbackarg = (void *)(size_t)s2;
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
	cc.cursor = getCursorFromEffect(w->track, s->track->v[w->track]->effect, (uint8_t)(size_t)e->callbackarg);
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

size_t getCursorFromEffectTrack(uint8_t track)
{
	size_t ret = 0;
	EffectChain *ec;
	for (uint8_t i = 0; i < track; i++)
	{
		ec = s->track->v[i]->effect;
		if (ec->c)
			for (uint8_t j = 0; j < ec->c; j++)
				ret += getEffectControlCount(&ec->v[j]);
		else
			ret++;
		ret += 2;
	}

	return ret;
}

/* cursor is (ControlState).cursor compatible */
uint8_t getEffectFromCursor(uint8_t track, EffectChain *chain, size_t cursor)
{
	size_t offset = getCursorFromEffectTrack(track);
	for (int i = 0; i < chain->c; i++)
	{
		offset += getEffectControlCount(&chain->v[i]);
		if (offset > cursor) return i;
	}
	return chain->c - 1; /* fallback */
}

size_t getCursorFromEffect(uint8_t track, EffectChain *chain, uint8_t index)
{
	size_t offset = getCursorFromEffectTrack(track);
	for (int i = 0; i < MIN(index, chain->c - 1); i++)
		offset += getEffectControlCount(&chain->v[i]);
	return offset;
}

void copyEffect(Effect *dest, Effect *src, float **input, float **output)
{
	if (!src) return;

	freeEffect(dest);

	dest->bypass = src->bypass;
	dest->inputgain = src->inputgain;

	if (effect_api[src->type].copy)
	{
		dest->type = src->type;
		dest->state = effect_api[src->type].copy(src->state, input, output);
	}
}
void copyEffectChain(EffectChain **dest, EffectChain *src)
{ /* NOT atomic */
	EffectChain *ret = calloc(1, sizeof(EffectChain) + src->c * sizeof(Effect));
	memcpy(ret, *dest, sizeof(float*) * 4); /* copy input and output */
	ret->c = src->c;
	ret->fader = src->fader;
	ret->panner = src->panner;

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

	if (effect_api[e->type].run)
	{
		float inputgain = powf(e->inputgain*DIV127 + 1.0f, GAIN_STAGE_SLOPE);
		for (uint32_t i = 0; i < samplecount; i++)
		{
			chain->input[0][i] *= inputgain;
			chain->input[1][i] *= inputgain;
		}

		effect_api[e->type].run(e->state, samplecount, chain->input, chain->output);
		if (!e->bypass)
		{
			memcpy(chain->input[0], chain->output[0], samplecount * sizeof(float));
			memcpy(chain->input[1], chain->output[1], samplecount * sizeof(float));
		}
	}
}

void runEffectChain(uint32_t samplecount, EffectChain *chain)
{
	for (uint8_t i = 0; i < chain->c; i++)
		runEffect(samplecount, chain, &chain->v[i]);

	float volume = powf(chain->fader*DIV255, GAIN_STAGE_SLOPE);
	float panning = chain->panner*DIV127;
	float inputgainl = volume * (panning > 0.0f ? 1.0f - panning : 1.0f);
	float inputgainr = volume * (panning < 0.0f ? 1.0f + panning : 1.0f);
	for (uint32_t i = 0; i < samplecount; i++)
	{
		chain->input[0][i] *= inputgainl;
		chain->input[1][i] *= inputgainr;
	}
}

struct json_object *serializeEffect(Effect *e)
{
	struct json_object *ret = json_object_new_object();
	json_object_object_add(ret, "type", json_object_new_string(EffectTypeString[e->type]));
	json_object_object_add(ret, "bypass", json_object_new_boolean(e->bypass));
	json_object_object_add(ret, "inputgain", json_object_new_int(e->inputgain));

	if (effect_api[e->type].serialize)
		json_object_object_add(ret, "state", effect_api[e->type].serialize(e->state));
	else
		json_object_object_add(ret, "state", NULL);

	return ret;
}

struct json_object *serializeEffectChain(EffectChain *ec)
{
	struct json_object *ret = json_object_new_object();
	json_object_object_add(ret, "fader", json_object_new_int(ec->fader));
	json_object_object_add(ret, "panner", json_object_new_int(ec->panner));

	struct json_object *chain = json_object_new_array_ext(ec->c);
	for (int i = 0; i < ec->c; i++)
		json_object_array_add(chain, serializeEffect(&ec->v[i]));
	json_object_object_add(ret, "chain", chain);

	return ret;
}

void deserializeEffect(EffectChain *ec, uint8_t index, struct json_object *jso)
{
	const char *string;
	if ((string = json_object_get_string(json_object_object_get(jso, "type"))))
		for (int j = 0; j < EFFECT_TYPE_COUNT; j++)
		{
			if (!strcmp(string, EffectTypeString[j]))
			{
				ec->v[index].type = j;
				break;
			}
		}
	else ec->v[index].type = 0;

	ec->v[index].bypass = json_object_get_boolean(json_object_object_get(jso, "bypass"));
	ec->v[index].inputgain = json_object_get_int(json_object_object_get(jso, "inputgain"));

	if (effect_api[ec->v[index].type].deserialize)
		ec->v[index].state = effect_api[ec->v[index].type].deserialize(json_object_object_get(jso, "state"), ec->input, ec->output);
	else
		ec->v[index].state = NULL;
}

EffectChain *deserializeEffectChain(struct json_object *jso)
{
	EffectChain *ret = newEffectChain();
	ret->fader = json_object_get_int(json_object_object_get(jso, "fader"));
	ret->panner = json_object_get_int(json_object_object_get(jso, "panner"));

	struct json_object *chain = json_object_object_get(jso, "chain");
	ret->c = json_object_array_length(chain);
	ret = realloc(ret, sizeof(EffectChain) + ret->c * sizeof(Effect));

	for (int i = 0; i < ret->c; i++)
		deserializeEffect(ret, i, json_object_array_get_idx(chain, i));

	return ret;
}


#include "draw.c"
#include "input.c"
