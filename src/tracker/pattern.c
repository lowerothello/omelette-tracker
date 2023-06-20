/* returns true if the pattern is populated */
static bool patternPopulated(PatternChain *pc, uint8_t index)
{
	if (index == PATTERN_VOID)
		return 0;

	Pattern *p = pc->v[pc->i[index]];
	for (uint16_t i = 0; i <= p->length; i++)
	{
		if (p->row[i].note != NOTE_VOID)
			return 1;

		for (uint8_t j = 0; j <= pc->commandc; j++)
			if (p->row[i].command[j].c)
				return 1;
	}
	return 0;
}

PatternChain *newPatternChain(void)
{
	PatternChain *ret = calloc(1, sizeof(PatternChain));
	memset(ret->order, PATTERN_VOID, PATTERN_VOID);
	memset(ret->i, PATTERN_VOID, PATTERN_VOID);
	ret->commandc = 1;
	return ret;
}

void freePatternChain(PatternChain *pc)
{
	if (!pc) return;

	for (int i = 0; i < pc->c; i++)
		free(pc->v[i]);
	free(pc);
}

static Pattern *dupPattern(Pattern *p, uint16_t newlen)
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

/* doesn't duplicate pc's children, use deepDupPatternChain to also copy children */
static PatternChain *dupPatternChain(PatternChain *pc)
{
	PatternChain *ret = malloc(sizeof(PatternChain) + pc->c * sizeof(Pattern*));
	memcpy(ret, pc, sizeof(PatternChain) + pc->c*sizeof(Pattern*));
	return ret;
}

static PatternChain *deepDupPatternChain(PatternChain *pc)
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

/* dup can be NULL */
static PatternChain *_addPattern(PatternChain *pc, uint8_t index, Pattern *dup)
{
	if (index == PATTERN_VOID || pc->i[index] != PATTERN_VOID)
		return pc; /* pattern already exists, don't try to add it */

	pc->c++;
	pc = realloc(pc, sizeof(PatternChain) + pc->c * sizeof(Pattern*));

	pc->i[index] = pc->c-1;
	pc->v[pc->c-1] = dupPattern(dup, getPatternLength());
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
	e.src = _addPattern(dupPatternChain(*pc), index, NULL);
	e.callback = cb_addPattern;
	pushEvent(&e);
	return 0;
}

/* returns a pointer to the pattern removed, or NULL if no patterns were removed */
Pattern *_delPattern(PatternChain **pc, uint8_t index)
{
	if ((*pc)->i[index] == PATTERN_VOID)
		return NULL; /* index not occupied */

	int cutindex = (*pc)->i[index];
	Pattern *ret = (*pc)->v[(*pc)->i[index]];

	(*pc)->i[index] = PATTERN_VOID;
	(*pc)->c--;
	memmove(&(*pc)->v[cutindex], &(*pc)->v[cutindex+1], ((*pc)->c - cutindex)*sizeof(Pattern*));
	*pc = realloc(*pc, sizeof(PatternChain) + (*pc)->c * sizeof(Pattern*));

	for (int i = 0; i < PATTERN_VOID; i++)
		if ((*pc)->i[i] > cutindex && (*pc)->i[i] != PATTERN_VOID)
			(*pc)->i[i]--;

	return ret;
}
static void cb_delPattern(Event *e)
{
	free(e->callbackarg); /* safe to call on NULL */
	free(e->src);
	regenSongLength(); /* for setPatternOrder */
	p->redraw = 1;
}
bool delPattern(PatternChain **pc, uint8_t index)
{ /* fully atomic */
	PatternChain *newchain = dupPatternChain(*pc);
	Pattern *cutpattern = _delPattern(&newchain, index);

	if (!cutpattern)
	{
		free(newchain);
		return 1;
	}

	Event e;
	e.sem = M_SEM_SWAP_REQ;
	e.dest = (void**)pc;
	e.src = newchain;
	e.callback = cb_delPattern;
	e.callbackarg = (void*)cutpattern;
	pushEvent(&e);
	return 0;
}

Row *getPatternRow(Pattern *pc, uint8_t index)
{
	return &pc->row[index];
}

/* returns a pointer to the pattern removed, or NULL if no patterns were removed */
Pattern *_prunePattern(PatternChain **pc, uint8_t index)
{
	if (index == PATTERN_VOID || (*pc)->i[index] == PATTERN_VOID)
		return NULL;

	/* fail if the pattern is still referenced */
	for (int i = 0; i < PATTERN_VOID; i++)
		if ((*pc)->order[i] == index)
			return NULL;

	/* fail if the pattern is populated */
	if (patternPopulated(*pc, index))
		return NULL;

	return _delPattern(pc, index);
}
/* remove pattern if it's empty, returns false if the pattern was pruned */
bool prunePattern(PatternChain **pc, uint8_t index)
{ /* fully atomic */
	PatternChain *newchain = dupPatternChain(*pc);
	Pattern *cutpattern = _prunePattern(&newchain, index);

	if (!cutpattern)
	{
		free(newchain);
		return 1;
	}

	Event e;
	e.sem = M_SEM_SWAP_REQ;
	e.dest = (void**)pc;
	e.src = newchain;
	e.callback = cb_delPattern; /* reuse the delete callback */
	e.callbackarg = (void*)cutpattern;
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

// /* NOTE: gonna put this here cos i always forget:
//  *       this function should give an empty pattern if applied to an unpopulated index,
//  *       however it should duplicate the index into a new slot for a populated index!
//  *       that's why it's written like it is, and why .fallbackindex is passed. */
// uint8_t dupFreePatternIndex(PatternChain *pc, uint8_t fallbackindex)
// {
// 	uint8_t index = getFreePatternIndex(pc, fallbackindex);
// 	if (index != fallbackindex)
// 	{
// 		if (fallbackindex != PATTERN_VOID && patternPopulated(pc, fallbackindex))
// 		{ /* duplicate the fallback index if it's populated */
// 			pc->i[index] = pc->c;
// 			pc->v[pc->c] = dupPattern(pc->v[pc->i[fallbackindex]], pc->v[pc->i[fallbackindex]]->length);
// 			pc->c++;
// 		} return index;
// 	} return fallbackindex;
// }

/* set the playback order directly */
/* .value == -1 to duplicate the index in place */
Pattern *_setPatternOrder(PatternChain **pc, uint8_t index, short value)
{
	uint8_t oldindex = (*pc)->order[index];

	Pattern *dup = NULL;
	if (value == -1)
	{
		value = oldindex;
		uint8_t index = getFreePatternIndex(*pc, value);
		if (index != value)
		{
			/* duplicate the fallback index if it's populated */
			if (patternPopulated(*pc, value))
				dup = (*pc)->v[(*pc)->i[value]];
			value = index;
		}
	}

	(*pc)->order[index] = value;

	Pattern *cutpattern = _prunePattern(pc, oldindex);

	*pc = _addPattern(*pc, (*pc)->order[index], dup);

	return cutpattern;
}

/* .pc is NOT allowed to be a pointer to a local variable :3 */
void setPatternOrder(PatternChain **pc, uint8_t index, short value)
{
	PatternChain *newchain = dupPatternChain(*pc);
	Pattern *cutpattern = _setPatternOrder(&newchain, index, value);

	Event e;
	e.sem = M_SEM_SWAP_REQ;
	e.dest = (void**)pc;
	e.src = newchain;
	e.callback = cb_delPattern;
	e.callbackarg = (void*)cutpattern;
	pushEvent(&e);
}
/* setPatternOrder, but with immediate access to newchain    */
/* be careful using, has a tendancy to smash the event queue */
PatternChain *setPatternOrderPeek(PatternChain **pc, uint8_t index, short value)
{
	PatternChain *newchain = dupPatternChain(*pc);
	Pattern *cutpattern = _setPatternOrder(&newchain, index, value);

	Event e;
	e.sem = M_SEM_QUEUE_SWAP_REQ;
	e.dest = (void**)pc;
	e.src = newchain;
	e.callback = cb_delPattern;
	e.callbackarg = (void*)cutpattern;
	pushEvent(&e);

	return newchain;
}

/* push a hex digit into the playback order */
void pushPatternOrder(PatternChain **pc, uint8_t index, char value)
{
	PatternChain *newchain = dupPatternChain(*pc);

	uint8_t newindex = newchain->order[index];
	/* initialize to zero before adjusting */
	if (newindex == PATTERN_VOID)
		newindex = 0;

	newindex <<= 4;
	newindex += value;

	Pattern *cutpattern = _setPatternOrder(&newchain, index, newindex);

	Event e;
	e.sem = M_SEM_SWAP_REQ;
	e.dest = (void**)pc;
	e.src = newchain;
	e.callback = cb_delPattern;
	e.callbackarg = (void*)cutpattern;
	pushEvent(&e);
}

struct json_object *serializeCommand(Command *m)
{
	struct json_object *ret = json_object_new_object();
	json_object_object_add(ret, "c", json_object_new_int(m->c));
	json_object_object_add(ret, "v", json_object_new_int(m->v));
	json_object_object_add(ret, "t", json_object_new_int(m->t));
	return ret;
}
Command deserializeCommand(struct json_object *jso)
{
	Command m;
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
		json_object_array_add(array, serializeCommand(&r->command[i]));
	json_object_object_add(ret, "command", array);

	return ret;
}
Row deserializeRow(struct json_object *jso)
{
	Row r;
	r.note = json_object_get_int(json_object_object_get(jso, "note"));
	r.inst = json_object_get_int(json_object_object_get(jso, "inst"));

	for (int i = 0; i < 8; i++)
		r.command[i] = deserializeCommand(json_object_array_get_idx(json_object_object_get(jso, "command"), i));
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
	json_object_object_add(ret, "commandc", json_object_new_int(pc->commandc));

	array = json_object_new_array_ext(PATTERN_VOID);
	for (i = 0; i < PATTERN_VOID; i++)
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
	ret->commandc = json_object_get_int(json_object_object_get(jso, "commandc"));

	for (i = 0; i < PATTERN_VOID; i++)
		ret->order[i] = json_object_get_int(json_object_array_get_idx(json_object_object_get(jso, "order"), i));

	ret->c = c;

	for (i = 0; i < PATTERN_VOID; i++)
		ret->i[i] = json_object_get_int(json_object_array_get_idx(json_object_object_get(jso, "index"), i));

	for (i = 0; i < ret->c; i++)
		ret->v[i] = deserializePattern(json_object_array_get_idx(json_object_object_get(jso, "data"), i));

	return ret;
}

void regenSongLength(void)
{
	uint8_t slen = 0;
	uint8_t i, j;
	for (i = 0; i < s->track->c; i++)
		for (j = 0; j < PATTERN_VOID; j++)
			if (s->track->v[i]->pattern->order[j] != PATTERN_VOID)
				slen = MAX(slen, j);

	s->slen = slen;
}
