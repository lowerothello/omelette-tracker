Variant *dupVariant(Variant *oldvariant, uint16_t newlen)
{
	Variant *ret = malloc(sizeof(uint16_t) + (newlen+1) * sizeof(Row));

	/* properly zero out the new variant */
	memset(ret->rowv, 0, (newlen+1) * sizeof(Row));
	for (uint32_t i = 0; i <= newlen; i++)
	{
		ret->rowv[i].note = NOTE_VOID;
		ret->rowv[i].inst = INST_VOID;
	}

	ret->rowc = newlen;

	if (oldvariant)
		memcpy(ret->rowv, oldvariant->rowv, (MIN(oldvariant->rowc, newlen)+1) * sizeof(Row));

	return ret;
}

static VariantChain *reallocVariantChain(VariantChain *vc, uint8_t length)
{ return realloc(vc, sizeof(VariantChain) + length*sizeof(Variant*)); }

VariantChain *dupVariantChain(VariantChain *vc)
{
	VariantChain *ret = reallocVariantChain(NULL, vc->c);
	memcpy(ret, vc, sizeof(VariantChain) + vc->c*sizeof(Variant*));
	return ret;
}

uint8_t dupEmptyVariantIndex(VariantChain *vc, uint8_t fallbackindex)
{
	for (short i = 0; i < VARIANT_MAX; i++)
		if (vc->i[i] == VARIANT_VOID)
		{
			if (fallbackindex != VARIANT_VOID && fallbackindex != VARIANT_OFF && variantPopulated(vc, fallbackindex))
			{ /* duplicate the fallback index if it makes sense to do so */
				vc->i[i] = vc->c;
				vc->v[vc->c] = dupVariant(vc->v[vc->i[fallbackindex]], vc->v[vc->i[fallbackindex]]->rowc);
				vc->c++;
			} return i;
		}
	return fallbackindex;
}

/* the effective rowc, how many rows are actually used */
uint16_t getSignificantRowc(VariantChain *vc)
{
	/* get the length if only the variant triggers */
	/* walk through 0, expect underflow */
	int lvtrig = 0;
	for (uint16_t i = vc->main->rowc-1; i < USHRT_MAX; i--)
	{
		if (vc->trig[i].index != VARIANT_VOID)
		{
			lvtrig = i;
			if (vc->trig[i].index != VARIANT_OFF)
				lvtrig += vc->v[vc->i[vc->trig[i].index]]->rowc;
			break;
		}
	}

	for (uint16_t i = vc->main->rowc-1; i != USHRT_MAX; i--)
	{
		if (vc->main->rowv[i].note != NOTE_VOID) return MAX(lvtrig, i);
		if (vc->main->rowv[i].inst != INST_VOID) return MAX(lvtrig, i);
		for (short j = 0; j <= vc->macroc; j++)
			if (vc->main->rowv[i].macro[j].c) return MAX(lvtrig, i);
	}

	return lvtrig+1;
}

/* invalidates past getRow() calls */
void resizeVariantChain(VariantChain *vc, uint16_t newlen)
{
	Vtrig *newtrig = calloc(newlen, sizeof(Vtrig));

	/* can't initialize with memset cos flags will be set to VARIANT_VOID as well */
	for (uint16_t i = 0; i < newlen; i++)
		newtrig[i].index = VARIANT_VOID;

	vc->songlen = newlen;

	/* depends on songv->rowc */
	if (vc->main && vc->trig)
		memcpy(newtrig, vc->trig, MIN(vc->main->rowc, newlen) * sizeof(Vtrig));

	if (vc->trig) free(vc->trig);
	vc->trig = newtrig;

	Variant *temp = dupVariant(vc->main, newlen);
	free(vc->main); vc->main = temp;
}

int addVariant(VariantChain **vc, uint8_t index, uint8_t length)
{
	if (index == VARIANT_VOID || (*vc)->i[index] != VARIANT_VOID) return 1;

	*vc = reallocVariantChain(*vc, (*vc)->c+1);

	(*vc)->i[index] = (*vc)->c;
	(*vc)->v[(*vc)->c] = dupVariant(NULL, length);
	(*vc)->c++;
	return 0;
}
int delVariant(VariantChain **vc, uint8_t index)
{
	if ((*vc)->i[index] == VARIANT_VOID) return 1; /* index not occupied */

	uint8_t cutindex = (*vc)->i[index];
	free((*vc)->v[cutindex]); (*vc)->v[cutindex] = NULL;

	(*vc)->i[index] = VARIANT_VOID;

	(*vc)->c--;
	memmove(&(*vc)->v[cutindex], &(*vc)->v[cutindex+1], ((*vc)->c - cutindex)*sizeof(Variant*));
	*vc = reallocVariantChain(*vc, (*vc)->c);

	for (uint8_t i = 0; i < VARIANT_MAX; i++)
		if ((*vc)->i[i] > cutindex && (*vc)->i[i] != VARIANT_VOID)
			(*vc)->i[i]--;

	return 0;
}

void freeVariantChain(VariantChain **vc)
{
	if (*vc)
	{
		if ((*vc)->trig) free((*vc)->trig);
		if ((*vc)->main) free((*vc)->main);
		for (uint8_t i = 0; i < (*vc)->c; i++)
			free((*vc)->v[i]);
		free(*vc); *vc = NULL;
	}
}

/* returns true if the variant is popuplated */
bool variantPopulated(VariantChain *vc, uint8_t index)
{
	for (int i = 0; i < vc->v[vc->i[index]]->rowc; i++)
	{
		if (vc->v[vc->i[index]]->rowv[i].note != NOTE_VOID) return 1;
		for (short j = 0; j <= vc->macroc; j++)
			if (vc->v[vc->i[index]]->rowv[i].macro[j].c) return 1;
	} return 0;
}

/* remove variant if it's empty           */
/* returns true if the variant was pruned */
bool pruneVariant(VariantChain **vc, uint8_t index)
{
	if (index == VARIANT_VOID || index == VARIANT_OFF
			|| (*vc)->i[index] == VARIANT_VOID)
		return 0;

	/* fail if variant is still referenced */
	for (int i = 0; i < (*vc)->main->rowc; i++)
		if ((*vc)->trig[i].index == index) return 0;

	/* fail if variant if populated */
	if (variantPopulated(*vc, index)) return 0;

	delVariant(vc, index);
	return 1;
}

/* returns the last (if any) variant trigger */
/* returns -1 if no vtrig is within range    */
int getVariantChainPrevVtrig(VariantChain *vc, uint16_t index)
{
	/* walk through 0, expect underflow */
	uint16_t iterstop = USHRT_MAX;
	if (index > 256) iterstop = index - 256; /* only walk up to 256 steps backwards (the longest a vtrig can be) */
	for (uint16_t i = index; i != iterstop; i--)
		if (vc->trig[i].index != VARIANT_VOID) return i;

	return -1;
}

uint8_t getEmptyVariantIndex(VariantChain *vc, uint8_t fallbackindex)
{
	for (short i = 0; i < VARIANT_MAX; i++)
		if (vc->i[i] == VARIANT_VOID)
			return i;
	return fallbackindex;
}

void inputVariantChainTrig(VariantChain **vc, uint16_t index, char value)
{
	uint8_t oldvariant = (*vc)->trig[index].index;

	/* initialize to zero before adjusting */
	if ((*vc)->trig[index].index == VARIANT_VOID)
		(*vc)->trig[index].index = 0;

	(*vc)->trig[index].index <<= 4; (*vc)->trig[index].index += value;
	if ((*vc)->trig[index].index == VARIANT_VOID)
		(*vc)->trig[index].flags = 0;

	pruneVariant(vc, oldvariant);
	addVariant(vc, (*vc)->trig[index].index, w->defvariantlength);
}

/* prunes the old index and sets the new index */
void setVariantChainTrig(VariantChain **vc, uint16_t index, uint8_t value)
{
	uint8_t oldvariant = (*vc)->trig[index].index;

	(*vc)->trig[index].index = value;
	if ((*vc)->trig[index].index == VARIANT_VOID)
		(*vc)->trig[index].flags = 0;

	pruneVariant(vc, oldvariant);
	addVariant(vc, (*vc)->trig[index].index, w->defvariantlength);
}

int getVariantChainVariant(Variant **output, VariantChain *vc, uint16_t index)
{
	int i = getVariantChainPrevVtrig(vc, index);
	if (i != -1 && vc->trig[i].index != VARIANT_OFF
			&& (vc->trig[i].flags&C_VTRIG_LOOP
			|| (vc->i[vc->trig[i].index] < vc->c && vc->v[vc->i[vc->trig[i].index]]->rowc >= index - i)))
	{
		if (output) *output = vc->v[vc->i[vc->trig[i].index]];
		return index - i;
	} return -1; /* fallback */
}
int getTrackVariantNoLoop(Variant **output, VariantChain *vc, uint16_t index)
{
	int i = getVariantChainPrevVtrig(vc, index);
	if (i != -1 && vc->trig[i].index != VARIANT_OFF
			&& vc->v[vc->i[vc->trig[i].index]]->rowc >= index - i)
	{
		if (output) *output = vc->v[vc->i[vc->trig[i].index]];
		return index - i;
	} return -1; /* fallback */
}

Row *getVariantRow(Variant *v, uint16_t row)
{
	return &v->rowv[row%(v->rowc+1)];
}


struct json_object *serializeMacro(Macro *m)
{
	struct json_object *ret = json_object_new_object();
	json_object_object_add(ret, "c", json_object_new_int(m->c));
	json_object_object_add(ret, "v", json_object_new_int(m->v));
	json_object_object_add(ret, "t", json_object_new_int(m->t));
	return ret;
}

Macro deserializeMacro(struct json_object *jso)
{
	Macro m;
	m.c = json_object_get_int(json_object_object_get(jso, "c"));
	m.v = json_object_get_int(json_object_object_get(jso, "v"));
	m.t = json_object_get_int(json_object_object_get(jso, "t"));
	return m;
}

struct json_object *serializeRow(Row *r)
{
	struct json_object *ret = json_object_new_object();
	json_object_object_add(ret, "note", json_object_new_int(r->note));
	json_object_object_add(ret, "inst", json_object_new_int(r->inst));

	struct json_object *array = json_object_new_array_ext(8);
	for (int i = 0; i < 8; i++)
		json_object_array_add(array, serializeMacro(&r->macro[i]));
	json_object_object_add(ret, "macro", array);

	return ret;
}

Row deserializeRow(struct json_object *jso)
{
	Row r;
	r.note = json_object_get_int(json_object_object_get(jso, "note"));
	r.inst = json_object_get_int(json_object_object_get(jso, "inst"));

	for (int i = 0; i < 8; i++)
		r.macro[i] = deserializeMacro(json_object_array_get_idx(json_object_object_get(jso, "macro"), i));
	return r;
}

struct json_object *serializeVariant(Variant *v)
{
	struct json_object *ret = json_object_new_array_ext(v->rowc + 1);
	for (int i = 0; i < v->rowc+1; i++)
		json_object_array_add(ret, serializeRow(&v->rowv[i]));
	return ret;
}

Variant *deserializeVariant(struct json_object *jso)
{
	Variant *ret = malloc(sizeof(Variant) + json_object_array_length(jso) * sizeof(Row));
	ret->rowc = json_object_array_length(jso) - 1;

	for (int i = 0; i < ret->rowc+1; i++)
		ret->rowv[i] = deserializeRow(json_object_array_get_idx(jso, i));

	return ret;
}

struct json_object *serializeVtrig(Vtrig *trig)
{
	struct json_object *ret = json_object_new_object();
	json_object_object_add(ret, "index", json_object_new_int(trig->index));
	json_object_object_add(ret, "flags", json_object_new_int(trig->flags));
	return ret;
}

Vtrig deserializeVtrig(struct json_object *jso)
{
	Vtrig ret;
	ret.index = json_object_get_int(json_object_object_get(jso, "index"));
	ret.flags = json_object_get_int(json_object_object_get(jso, "flags"));
	return ret;
}

struct json_object *serializeVariantChain(VariantChain *chain)
{
	int i;
	struct json_object *ret = json_object_new_object();
	json_object_object_add(ret, "songlen", json_object_new_int(chain->songlen));
	json_object_object_add(ret, "macroc", json_object_new_int(chain->macroc));

	struct json_object *array;
	array = json_object_new_array_ext(chain->songlen);
	for (i = 0; i < chain->songlen; i++)
		json_object_array_add(array, serializeVtrig(&chain->trig[i]));
	json_object_object_add(ret, "trig", array);

	json_object_object_add(ret, "main", serializeVariant(chain->main));

	array = json_object_new_array_ext(VARIANT_MAX);
	for (i = 0; i < VARIANT_MAX; i++)
		json_object_array_add(array, json_object_new_int(chain->i[i]));
	json_object_object_add(ret, "index", array);

	array = json_object_new_array_ext(chain->c);
	for (i = 0; i < chain->c; i++)
		json_object_array_add(array, serializeVariant(chain->v[i]));
	json_object_object_add(ret, "data", array);

	return ret;
}

VariantChain *deserializeVariantChain(struct json_object *jso)
{
	int i;
	VariantChain *ret = calloc(1, sizeof(VariantChain) + sizeof(Variant*) * json_object_array_length(json_object_object_get(jso, "data")));

	ret->songlen = json_object_get_int(json_object_object_get(jso, "songlen"));
	ret->macroc = json_object_get_int(json_object_object_get(jso, "macroc"));

	ret->trig = calloc(ret->songlen, sizeof(Vtrig));
	for (i = 0; i < ret->songlen; i++)
		ret->trig[i] = deserializeVtrig(json_object_array_get_idx(json_object_object_get(jso, "trig"), i));

	ret->main = deserializeVariant(json_object_object_get(jso, "main"));

	for (i = 0; i < VARIANT_MAX; i++)
		ret->i[i] = json_object_get_int(json_object_array_get_idx(json_object_object_get(jso, "index"), i));

	ret->c = json_object_array_length(json_object_object_get(jso, "data"));

	for (i = 0; i < ret->c; i++)
		ret->v[i] = deserializeVariant(json_object_array_get_idx(json_object_object_get(jso, "data"), i));

	return ret;
}
