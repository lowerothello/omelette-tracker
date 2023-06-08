/* returns true if the pattern is populated */
static bool patternPopulated(PatternChain *pc, uint8_t index)
{
	Pattern *p = pc->v[pc->i[index]];
	for (uint16_t i = 0; i <= p->length; i++)
	{
		if (p->row[i].note != NOTE_VOID)
			return 1;

		for (uint8_t j = 0; j <= pc->macroc; j++)
			if (p->row[i].macro[j].c)
				return 1;
	}
	return 0;
}

/* unsafe unless called atomically */
static PatternChain *reallocPatternChain(PatternChain *pc)
{
	return realloc(pc, sizeof(PatternChain) + pc->c * sizeof(Pattern*));
}

PatternChain *newPatternChain(void)
{
	PatternChain *ret = calloc(1, sizeof(PatternChain));
	memset(ret->order, PATTERN_VOID, PATTERN_ORDER_LENGTH);
	ret->macroc = 1;
	return ret;
}

void freePatternChain(PatternChain *pc)
{
	if (!pc) return;

	for (int i = 0; i < pc->c; i++)
		free(pc->v[i]);
	free(pc);
}

/* doesn't duplicate pc's children, use deepDupPatternChain to also copy children */
PatternChain *dupPatternChain(PatternChain *pc)
{
	PatternChain *ret = malloc(sizeof(PatternChain) + pc->c * sizeof(Pattern*));
	memcpy(ret, pc, sizeof(PatternChain) + pc->c*sizeof(Pattern*));
	return ret;
}

PatternChain *deepDupPatternChain(PatternChain *pc)
{
	PatternChain *ret = dupPatternChain(pc);
	for (uint8_t i = 0; i < pc->c; i++)
		ret->v[i] = dupPattern(pc->v[i], pc->v[i]->length);

	return ret;
}

uint8_t getPatternLength(void) { return s->plen+1; }
uint8_t getPatternChainIndex(uint16_t index) { return index / getPatternLength(); }
uint8_t getPatternIndex(uint16_t index) { return index % getPatternLength(); }
Pattern *getPatternChainPattern(PatternChain *pc, uint16_t index)
{
	size_t pindex = getPatternChainIndex(index);
	return pc->v[pc->i[pc->order[pindex]]];
}

Pattern *dupPattern(Pattern *p, uint16_t newlen)
{
	Pattern *ret = calloc(1, sizeof(Pattern) + (newlen+1)*sizeof(Row));

	for (int i = 0; i <= newlen; i++)
	{
		ret->row[i].note = NOTE_VOID;
		ret->row[i].inst = INST_VOID;
	}
	ret->length = newlen;

	if (p)
		memcpy(ret->row, p->row, (MIN(ret->length, newlen) + 1) * sizeof(Row));

	return ret;
}

static PatternChain *_addPattern(PatternChain *pc, uint8_t index)
{
	pc->c++;
	pc = reallocPatternChain(pc);

	pc->i[index] = pc->c-1;
	pc->v[pc->c-1] = dupPattern(NULL, getPatternLength());
	return pc;
}
static void cb_addPattern(Event *e)
{
	free(e->src);
	p->redraw = 1;
}
bool addPattern(PatternChain **pc, uint8_t index)
{ /* fully atomic */
	if (index == PATTERN_VOID || (*pc)->i[index] != PATTERN_VOID)
		return 1;

	Event e;
	e.sem = M_SEM_SWAP_REQ;
	e.dest = (void**)pc;
	e.src = _addPattern(dupPatternChain(*pc), index);
	e.callback = cb_addPattern;
	pushEvent(&e);
	return 0;
}

/* returns the pattern removed, or -1 if no patterns were removed */
int _delPattern(PatternChain **pc, uint8_t index)
{
	if ((*pc)->i[index] == PATTERN_VOID)
		return -1; /* index not occupied */

	int cutindex = (*pc)->i[index];

	(*pc)->i[index] = PATTERN_VOID;
	(*pc)->c--;
	memmove(&(*pc)->v[cutindex], &(*pc)->v[cutindex+1], ((*pc)->c - cutindex)*sizeof(Pattern*));
	*pc = reallocPatternChain(*pc);

	for (int i = 0; i < PATTERN_VOID; i++)
		if ((*pc)->i[i] > cutindex && (*pc)->i[i] != PATTERN_VOID)
			(*pc)->i[i]--;

	return cutindex;
}
static void cb_delPattern(Event *e)
{
	int cutindex = (int)(size_t)e->callbackarg;

	free(((PatternChain*)e->src)->v[cutindex]);
	free(e->src);
	p->redraw = 1;
}
bool delPattern(PatternChain **pc, uint8_t index)
{ /* fully atomic */
	PatternChain *newchain = dupPatternChain(*pc);
	int cutindex = _delPattern(&newchain, index);

	if (cutindex == -1)
	{
		free(newchain);
		return 1;
	}

	Event e;
	e.sem = M_SEM_SWAP_REQ;
	e.dest = (void**)pc;
	e.src = newchain;
	e.callback = cb_delPattern;
	e.callbackarg = (void*)(size_t)cutindex;
	pushEvent(&e);
	return 0;
}

Row *getPatternRow(Pattern *pc, uint8_t index)
{
	return &pc->row[index];
}

/* returns the pattern removed, or -1 if no patterns were removed */
int _prunePattern(PatternChain **pc, uint8_t index)
{
	if (index == PATTERN_VOID || (*pc)->i[index] == PATTERN_VOID)
		return -1;

	/* fail if the pattern is still referenced */
	for (int i = 0; i < PATTERN_ORDER_LENGTH; i++)
		if ((*pc)->order[i] == index)
			return -1;

	/* fail if the pattern is populated */
	if (patternPopulated(*pc, index))
		return -1;

	return _delPattern(pc, index);
}
/* remove pattern if it's empty, returns false if the pattern was pruned */
bool prunePattern(PatternChain **pc, uint8_t index)
{ /* fully atomic */
	PatternChain *newchain = dupPatternChain(*pc);
	int cutindex = _prunePattern(&newchain, index);

	if (cutindex == -1)
	{
		free(newchain);
		return 1;
	}

	Event e;
	e.sem = M_SEM_SWAP_REQ;
	e.dest = (void**)pc;
	e.src = newchain;
	e.callback = cb_delPattern; /* reuse the delete callback */
	e.callbackarg = (void*)(size_t)cutindex;
	pushEvent(&e);
	return 0;
}

/* returns the first free pattern slot, or .fallbackindex if there are no free slots */
uint8_t getFreePatternIndex(PatternChain *pc, uint8_t fallbackindex)
{
	for (int i = 0; i < PATTERN_VOID; i++)
		if (pc->i[i] == PATTERN_VOID)
			return i;
	return fallbackindex;
}
uint8_t dupFreePatternIndex(PatternChain *pc, uint8_t fallbackindex)
{
	for (int i = 0; i < PATTERN_VOID; i++)
		if (pc->i[i] == PATTERN_VOID)
		{
			if (fallbackindex != PATTERN_VOID && patternPopulated(pc, fallbackindex))
			{ /* duplicate the fallback index if it makes sense to do so */
				pc->i[i] = pc->c;
				pc->v[pc->c] = dupPattern(pc->v[pc->i[fallbackindex]], pc->v[pc->i[fallbackindex]]->length);
				pc->c++;
			} return i;
		}
	return fallbackindex;
}

/* push a hex digit into the playback order */
void pushPatternOrder(PatternChain **pc, uint8_t index, char value)
{
	PatternChain *newchain = dupPatternChain(*pc);
	uint8_t oldindex = newchain->order[index];

	/* initialize to zero before adjusting */
	if (newchain->order[index] == PATTERN_VOID)
		newchain->order[index] = 0;

	newchain->order[index] <<= 4;
	newchain->order[index] += value;

	int cutindex = _prunePattern(&newchain, oldindex);
	newchain = _addPattern(newchain, newchain->order[index]);

	Event e;
	e.sem = M_SEM_SWAP_REQ;
	e.dest = (void**)pc;
	e.src = newchain;
	if (cutindex == -1) e.callback = cb_addPattern;
	else                e.callback = cb_delPattern;
	e.callbackarg = (void*)(size_t)cutindex;
	pushEvent(&e);
}

/* set the playback order directly */
int _setPatternOrder(PatternChain **pc, uint8_t index, uint8_t value)
{
	uint8_t oldindex = (*pc)->order[index];

	(*pc)->order[index] = value;

	int cutindex = _prunePattern(pc, oldindex);
	*pc = _addPattern(*pc, (*pc)->order[index]);
	return cutindex;
}
void setPatternOrder(PatternChain **pc, uint8_t index, uint8_t value)
{
	PatternChain *newchain = dupPatternChain(*pc);
	int cutindex = _setPatternOrder(&newchain, index, value);

	Event e;
	e.sem = M_SEM_SWAP_REQ;
	e.dest = (void**)pc;
	e.src = newchain;
	if (cutindex == -1) e.callback = cb_addPattern;
	else                e.callback = cb_delPattern;
	e.callbackarg = (void*)(size_t)cutindex;
	pushEvent(&e);
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

struct json_object *serializePattern(Pattern *p)
{
	struct json_object *ret = json_object_new_array_ext(p->length+1);
	for (int i = 0; i <= p->length; i++)
		json_object_array_add(ret, serializeRow(&p->row[i]));
	return ret;
}
Pattern *deserializePattern(struct json_object *jso)
{
	Pattern *ret = malloc(sizeof(Pattern) + json_object_array_length(jso)*sizeof(Row));

	ret->length = json_object_array_length(jso) - 1;
	for (int i = 0; i <= ret->length; i++)
		ret->row[i] = deserializeRow(json_object_array_get_idx(jso, i));
	return ret;
}

struct json_object *serializePatternChain(PatternChain *pc)
{
	int i;
	struct json_object *array;
	struct json_object *ret = json_object_new_object();
	json_object_object_add(ret, "macroc", json_object_new_int(pc->macroc));

	array = json_object_new_array_ext(PATTERN_ORDER_LENGTH);
	for (i = 0; i < PATTERN_ORDER_LENGTH; i++)
		json_object_array_add(array, json_object_new_int(pc->order[i]));
	json_object_object_add(ret, "order", array);

	array = json_object_new_array_ext(PATTERN_VOID);
	for (i = 0; i < PATTERN_VOID; i++)
		json_object_array_add(array, json_object_new_int(pc->i[i]));
	json_object_object_add(ret, "index", array);

	array = json_object_new_array_ext(pc->c);
	for (i = 0; i < pc->c; i++)
		json_object_array_add(array, serializePattern(pc->v[i]));
	json_object_object_add(ret, "data", array);

	return ret;
}
PatternChain *deserializePatternChain(struct json_object *jso)
{
	int i;
	uint8_t c = json_object_array_length(json_object_object_get(jso, "data"));
	PatternChain *ret = calloc(1, sizeof(PatternChain) + sizeof(Pattern*) * c);
	ret->macroc = json_object_get_int(json_object_object_get(jso, "macroc"));

	for (i = 0; i < PATTERN_ORDER_LENGTH; i++)
		ret->order[i] = json_object_get_int(json_object_array_get_idx(json_object_object_get(jso, "order"), i));

	ret->c = c;

	for (i = 0; i < PATTERN_VOID; i++)
		ret->i[i] = json_object_get_int(json_object_array_get_idx(json_object_object_get(jso, "index"), i));

	for (i = 0; i < ret->c; i++)
		ret->v[i] = deserializePattern(json_object_array_get_idx(json_object_object_get(jso, "data"), i));

	return ret;
}
