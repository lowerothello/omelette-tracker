void freeEffect(Effect *e)
{
	if (!e || !e->state) return;

	switch (e->type)
	{
		case EFFECT_TYPE_NATIVE: freeNativeEffect(&native_db, e); break;
		case EFFECT_TYPE_LADSPA: freeLadspaEffect(e); break;
		case EFFECT_TYPE_LV2:    freeLV2Effect   (e); break;
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
			case EFFECT_TYPE_NATIVE: return getNativeEffectControlCount(&native_db, e);
			case EFFECT_TYPE_LADSPA: return getLadspaEffectControlCount(e);
			case EFFECT_TYPE_LV2:    return getLV2EffectControlCount   (e);
		}
	return 0;
}

EffectChain *_addEffect(EffectChain *chain, unsigned long pluginindex, uint8_t chordindex)
{
	EffectChain *ret = calloc(1, sizeof(EffectChain) + (chain->c+1) * sizeof(Effect));
	memcpy(ret, chain, sizeof(float *) * 4); /* copy input and output */
	ret->c = chain->c + 1;

	if (chordindex)
		memcpy(&ret->v[0],
				&chain->v[0],
				chordindex * sizeof(Effect));

	if (chordindex < chain->c)
		memcpy(&ret->v[chordindex+1],
				&chain->v[chordindex],
				(chain->c - chordindex) * sizeof(Effect));

	if (pluginindex < native_db.descc) /* native */
		initNativeEffect(&native_db, &ret->v[chordindex], pluginindex);
	else if (pluginindex < native_db.descc + ladspa_db.descc) /* ladspa */
		initLadspaEffect(ret, &ret->v[chordindex], ladspa_db.descv[pluginindex - native_db.descc]);
	else /* lv2 */
	{
		const LilvPlugins *lap = lilv_world_get_all_plugins(lv2_db.world);
		uint32_t i = 0;
		LILV_FOREACH(plugins, iter, lap)
			if (i == pluginindex - native_db.descc - ladspa_db.descc)
			{
				initLV2Effect(ret, &ret->v[chordindex], lilv_plugins_get(lap, iter));
				break;
			} else i++;
	}

	return ret;
}
static void cb_addEffect(Event *e)
{
	free(e->src); e->src = NULL;
	p->redraw = 1;
}
void addEffect(EffectChain **chain, unsigned long pluginindex, uint8_t chordindex, void (*cb)(Event *))
{ /* fully atomic */
	if ((*chain)->c < EFFECT_CHAIN_LEN)
	{
		Event e;
		e.sem = M_SEM_SWAP_REQ;
		e.dest = (void **)chain;
		e.src = _addEffect(*chain, pluginindex, chordindex);
		e.callback = cb;
		e.callbackarg = (void *)(size_t)chordindex;
		pushEvent(&e);
	}
}

EffectChain *_delEffect(EffectChain *chain, uint8_t chordindex)
{
	EffectChain *ret = calloc(1, sizeof(EffectChain) + (chain->c-1) * sizeof(Effect));
	memcpy(ret, chain, sizeof(float *) * 4); /* copy input and output */
	ret->c = chain->c - 1;

	if (chordindex)
		memcpy(&ret->v[0],
				&chain->v[0],
				chordindex * sizeof(Effect));

	if (chordindex < chain->c)
		memcpy(&ret->v[chordindex],
				&chain->v[chordindex+1],
				(chain->c - chordindex - 1) * sizeof(Effect));

	return ret;
}
static void cb_delEffect(Event *e)
{
	freeEffect(&((EffectChain *)e->src)->v[(size_t)e->callbackarg]);
	free(e->src); e->src = NULL;
	p->redraw = 1;
}
void delEffect(EffectChain **chain, uint8_t chordindex)
{ /* fully atomic */
	if ((*chain)->c)
	{
		Event e;
		e.sem = M_SEM_SWAP_REQ;
		e.dest = (void **)chain;
		e.src = _delEffect(*chain, chordindex);
		e.callback = cb_delEffect;
		e.callbackarg = (void *)(size_t)chordindex;
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

uint8_t getCursorFromEffect(EffectChain *chain, uint8_t chordindex)
{
	uint8_t offset = 0;
	for (int i = 0; i < MIN(chordindex, chain->c); i++)
		offset += getEffectControlCount(&chain->v[i]);
	return offset;
}

void copyEffect(EffectChain *destchain, Effect *dest, Effect *src)
{
	if (!src) return;

	freeEffect(dest);
	switch (src->type)
	{
		case EFFECT_TYPE_NATIVE: copyNativeEffect(&native_db, dest, src); break;
		case EFFECT_TYPE_LADSPA: copyLadspaEffect(destchain, dest, src);  break;
		case EFFECT_TYPE_LV2:    copyLV2Effect   (destchain, dest, src);  break;
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
		case EFFECT_TYPE_NATIVE: serializeNativeEffect(&native_db, e, fp); break;
		case EFFECT_TYPE_LADSPA: serializeLadspaEffect(e, fp);             break;
		case EFFECT_TYPE_LV2:    serializeLV2Effect   (e, fp);             break;
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
		case EFFECT_TYPE_NATIVE: deserializeNativeEffect(&native_db, e, fp); break;
		case EFFECT_TYPE_LADSPA: deserializeLadspaEffect(chain, e, fp);      break;
		case EFFECT_TYPE_LV2:    deserializeLV2Effect   (chain, e, fp);      break;
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

short getEffectHeight(Effect *e)
{
	if (e)
		switch (e->type)
		{
			case EFFECT_TYPE_NATIVE: return getNativeEffectHeight(&native_db, e);
			case EFFECT_TYPE_LADSPA: return getLadspaEffectHeight(e);
			case EFFECT_TYPE_LV2:    return getLV2EffectHeight   (e);
		}
	return 0;
}

void drawBoundingBox(short x, short y, short w, short h, short xmin, short xmax, short ymin, short ymax)
{
	int i;
	if (ymin <= y && ymax >= y)
	{
		for (i = MAX(xmin, x); i <= MIN(xmax, x+w); i++)
			printf("\033[%d;%dH─", y, i);
		if (x-1 >= xmin && x-1 <= xmax) printf("\033[%d;%dH┌", y, x-1);
		if (x+w >= xmin && x+w <= xmax) printf("\033[%d;%dH┒", y, x+w);
	}
	if (ymin <= y+h && ymax >= y+h)
	{
		for (i = MAX(xmin, x); i <= MIN(xmax, x+w); i++)
			printf("\033[%d;%dH━", y+h, i);
		if (x-1 >= xmin && x-1 <= xmax) printf("\033[%d;%dH┕", y+h, x-1);
		if (x+w >= xmin && x+w <= xmax) printf("\033[%d;%dH┛", y+h, x+w);
	}
	for (i = 1; i < h; i++)
		if (ymin <= y+i && ymax >= y+i)
		{
			if (x-1 >= xmin && x-1 <= xmax) printf("\033[%d;%dH│", y+i, x-1);
			if (x+w >= xmin && x+w <= xmax) printf("\033[%d;%dH┃", y+i, x+w);
		}
}

int drawEffect(Effect *e, ControlState *cc, bool selected, short x, short w, short y, short ymin, short ymax)
{
	if (!e) return 0;

	switch (e->type)
	{
		case EFFECT_TYPE_NATIVE: drawNativeEffect(&native_db, e, cc, x, w, y, ymin, ymax); break;
		case EFFECT_TYPE_LADSPA: drawLadspaEffect(            e, cc, x, w, y, ymin, ymax); break;
		case EFFECT_TYPE_LV2:    drawLV2Effect   (            e, cc, x, w, y, ymin, ymax); break;
	}
	short ret = getEffectHeight(e);

	y--;
	if (selected) printf("\033[1m");
	drawBoundingBox(x, y, w, ret-1, 1, ws.ws_col, ymin, ymax);
	if (selected) printf("\033[22m");

	return ret;
}

/* e->input[0/1] and e->output[0/1] should be arrays of at least length samplecount */
void runEffect(uint32_t samplecount, EffectChain *chain, Effect *e)
{
	if (!e) return;
	switch (e->type)
	{
		case EFFECT_TYPE_NATIVE: runNativeEffect(&native_db, samplecount, chain, e); break;
		case EFFECT_TYPE_LADSPA: runLadspaEffect(            samplecount, chain, e); break;
		case EFFECT_TYPE_LV2:    runLV2Effect   (            samplecount, chain, e); break;
	}
}
