/* clears the playback state of a track */
void clearTrackRuntime(Track *cv)
{
	cv->r.note = NOTE_VOID;
	cv->r.inst = INST_VOID;
	cv->file = 0;

	macroCallbackClear(cv);
	memset(cv->inststate, 0, instGetPlaybackStateSize());
}

/* clears the global variant and frees all local variants */
void initTrackData(Track *cv) /* TODO: should be atomic */
{
	freePatternChain(cv->pattern);
	cv->pattern = newPatternChain();

	cv->mute = 0;

	if (cv->effect)
		clearEffectChain(cv->effect);
}

void clearTrackData(Track *cv)
{ /* NOT atomic */
	initTrackData(cv);
	freePatternChain(cv->pattern);
}

void addTrackRuntime(Track *cv)
{
	cv->rampindex = rampmax;
	cv->rampbuffer = calloc(rampmax * 2, sizeof(float)); /* *2 for stereo */
	cv->inststate = malloc(instGetPlaybackStateSize());
	cv->macrostate = calloc(sizeof(void*), MACRO_CALLBACK_MAX);
	for (int i = 0; i < MACRO_CALLBACK_MAX; i++)
		if (global_macro_callbacks[i].statesize) /* calloc is ambiguous as to whether it will return a null pointer or an illegal pointer if either arg is 0, so explicitly check */
			cv->macrostate[i] = calloc(1, global_macro_callbacks[i].statesize);
	clearTrackRuntime(cv);
}

void debug_dumpTrackState(Song *cs)
{
#ifdef DEBUG_LOGS
	FILE *fp = fopen(".oml_trackdump", "w");

	fprintf(fp, "===== TRACK DUMP =====\n");
	fprintf(fp, "s->track: %p\n", cs->track);
	fprintf(fp, "trackc:   %02x\n\n", cs->track->c);

	for (int i = 0; i < cs->track->c; i++)
	{
		fprintf(fp, "TRACK %02x:\n", i);
		fprintf(fp, "length: %d\n", getSignificantRowc(cs->track->v[i].variant));
		fprintf(fp, "output[0]: %p\n", cs->track->v[i].effect->output[0]);
		fprintf(fp, "\n");
	}

	fprintf(fp, "\n");
	fclose(fp);
#endif
}

static void cb_addTrack(Event *e)
{
	free(((TrackChain*)e->src)->v);
	free(e->src);

	p->redraw = 1;
}

/* .cs can be NULL */
Track *allocTrack(Song *cs, Track *copyfrom)
{
	Track *ret = calloc(1, sizeof(Track));

	addTrackRuntime(ret);
	ret->effect = newEffectChain();
	initTrackData(ret);

	copyTrack(ret, copyfrom);
	return ret;
}

/* copyfrom can be NULL */
void addTrack(Song *cs, uint8_t index, uint16_t count, Track *copyfrom)
{ /* fully atomic */
	/* scale down count if necessary */
	count = MIN(count, TRACK_MAX - cs->track->c);

	TrackChain *newtrack = malloc(sizeof(TrackChain));
	newtrack->c = cs->track->c;
	newtrack->v = calloc(cs->track->c + count, sizeof(Track*));

	if (index)
		memcpy(&newtrack->v[0],
				&cs->track->v[0],
				index * sizeof(Track*));

	if (index < cs->track->c)
		memcpy(&newtrack->v[index+count],
				&cs->track->v[index],
				(cs->track->c - index) * sizeof(Track*));

	/* allocate new tracks */
	for (uint16_t i = 0; i < count; i++)
		newtrack->v[index+i] = allocTrack(cs, copyfrom);

	newtrack->c += count;

	Event e;
	e.sem = M_SEM_SWAP_REQ;
	e.dest = (void**)&cs->track;
	e.src = newtrack;
	e.callback = cb_addTrack;
	pushEvent(&e);
}

void swapTracks(uint8_t index1, uint8_t index2)
{ /* fully atomic */
	TrackChain *newtrack = calloc(1, sizeof(TrackChain) + s->track->c*sizeof(Track*));
	newtrack->c = s->track->c;

	/* copy each track over individually, but swap the destinations for index{1,2} */
	for (uint8_t i = 0; i < newtrack->c; i++)
	{
		if      (i == index1) memcpy(&newtrack->v[index2], &s->track->v[i], sizeof(Track*));
		else if (i == index2) memcpy(&newtrack->v[index1], &s->track->v[i], sizeof(Track*));
		else                  memcpy(&newtrack->v[i],      &s->track->v[i], sizeof(Track*));
	}

	Event e;
	e.sem = M_SEM_SWAP_REQ;
	e.dest = (void**)&s->track;
	e.src = newtrack;
	e.callback = cb_addTrack;
	pushEvent(&e);
}

void _delTrack(Song *cs, Track *cv)
{ /* NOT atomic */
	clearTrackData(cv);
	freeEffectChain(cv->effect);
	if (cv->rampbuffer) free(cv->rampbuffer);
	if (cv->inststate) free(cv->inststate);
	if (cv->macrostate)
	{
		for (int i = 0; i < MACRO_CALLBACK_MAX; i++)
			if (cv->macrostate[i])
				free(cv->macrostate[i]);
		free(cv->macrostate);
	}
}

static void cb_delTrack(Event *e)
{
	uint16_t count = (size_t)e->callbackarg & 0x00ffff;
	uint8_t index = ((size_t)e->callbackarg & 0xff0000) >> 16;

	for (uint16_t i = 0; i < count; i++)
	{
		_delTrack(s, ((TrackChain*)e->src)->v[index+i]);
		free(((TrackChain*)e->src)->v[index+i]);
	}

	free(((TrackChain*)e->src)->v);
	free(e->src);

	if (w->track > s->track->c-1)
		w->track = s->track->c-1;

	regenGlobalRowc(s);
	p->redraw = 1;
}
void delTrack(uint8_t index, uint16_t count)
{ /* fully atomic */
	/* scale down count if necessary */
	count = MIN(count, s->track->c - 1);
	if (index + count > s->track->c)
		index = s->track->c - count;

	/* TODO: if the last track would be deleted then call clearTrackData(&s->track->v[0]) */

	TrackChain *newtrack = malloc(sizeof(TrackChain));
	newtrack->c = s->track->c - count;
	newtrack->v = calloc(s->track->c - count, sizeof(Track*));

	if (index)
		memcpy(&newtrack->v[0],
				&s->track->v[0],
				index * sizeof(Track*));

	if (index < s->track->c)
		memcpy(&newtrack->v[index],
				&s->track->v[index+count],
				(s->track->c - index - count) * sizeof(Track*));

	Event e;
	e.sem = M_SEM_SWAP_REQ;
	e.dest = (void**)&s->track;
	e.src = newtrack;
	e.callback = cb_delTrack;
	e.callbackarg = (void*)((((size_t)index)<<16) + (size_t)count);
	pushEvent(&e);
}

void copyTrack(Track *dest, Track *src) /* NOT atomic */
{
	if (!dest || !src) return;

	dest->mute = src->mute;
	memcpy(dest->name, src->name, NAME_LEN + 1);
	dest->transpose = src->transpose;

	freePatternChain(dest->pattern);
	dest->pattern = deepDupPatternChain(src->pattern);

	copyEffectChain(&dest->effect, src->effect);
}

Row *getTrackRow(Track *cv, uint16_t index, bool createifmissing)
{
	uint8_t pindex = getPatternChainIndex(index);
	if (cv->pattern->order[pindex] == PATTERN_VOID)
	{
		if (createifmissing)
		{
			_setPatternOrder(&cv->pattern, pindex, dupFreePatternIndex(cv->pattern, cv->pattern->order[pindex]));
			regenGlobalRowc(s);
		} else return NULL;
	}

	return getPatternRow(getPatternChainPattern(cv->pattern, index), getPatternIndex(index));
}

void regenGlobalRowc(Song *cs)
{ /* TODO: atomicity */
	uint16_t songlen = 0;
	for (uint8_t i = 0; i < cs->track->c; i++)
		for (int j = PATTERN_VOID; j >= 0; j--)
			if (cs->track->v[i]->pattern->order[j] != PATTERN_VOID)
				songlen = MAX(songlen, (j+1)*getPatternLength());
	cs->songlen = songlen;
}

void applyTrackMutes(void)
{
	Event e;
	e.sem = M_SEM_TRACK_MUTE;
	pushEvent(&e);
}
void toggleTrackMute(uint8_t track)
{
	s->track->v[track]->mute = !s->track->v[track]->mute;
	applyTrackMutes();
}
void toggleTrackSolo(uint8_t track)
{
	int i;
	bool reset = 0;

	if (!s->track->v[track]->mute)
	{
		for (i = 0; i < s->track->c; i++)
			if (s->track->v[i]->mute)
				reset = 1;

		for (i = 0; i < s->track->c; i++)
			if (i != track && !s->track->v[i]->mute)
				reset = 0;
	}

	if (reset)
	{
		for (i = 0; i < s->track->c; i++)
			s->track->v[i]->mute = 0;
	} else
	{
		for (i = 0; i < s->track->c; i++)
			s->track->v[i]->mute = 1;
		s->track->v[track]->mute = 0;
	}
	applyTrackMutes();
}

struct json_object *serializeTrack(Track *track)
{
	struct json_object *ret = json_object_new_object();
	json_object_object_add(ret, "name", json_object_new_string(track->name));
	json_object_object_add(ret, "transpose", json_object_new_int(track->transpose));

	json_object_object_add(ret, "mute", json_object_new_boolean(track->mute));
	json_object_object_add(ret, "pattern", serializePatternChain(track->pattern));
	json_object_object_add(ret, "effect", serializeEffectChain(track->effect));

	return ret;
}

Track *deserializeTrack(struct json_object *jso)
{
	Track *ret = calloc(1, sizeof(Track));
	addTrackRuntime(ret);

	const char *string = json_object_get_string(json_object_object_get(jso, "name"));
	memcpy(&ret->name, string, MIN(strlen(string), NAME_LEN));
	ret->transpose = json_object_get_int(json_object_object_get(jso, "transpose"));

	ret->mute = json_object_get_boolean(json_object_object_get(jso, "mute"));
	ret->pattern = deserializePatternChain(json_object_object_get(jso, "pattern"));
	ret->effect = deserializeEffectChain(json_object_object_get(jso, "effect"));

	return ret;
}

struct json_object *serializeTrackChain(TrackChain *chain)
{
	struct json_object *ret = json_object_new_array_ext(chain->c);

	for (int i = 0; i < chain->c; i++)
		json_object_array_add(ret, serializeTrack(chain->v[i]));

	return ret;
}

TrackChain *deserializeTrackChain(struct json_object *jso)
{
	TrackChain *ret = malloc(sizeof(TrackChain));
	ret->c = json_object_array_length(jso);

	ret->v = malloc(ret->c * sizeof(Track*));
	for (int i = 0; i < ret->c; i++)
		ret->v[i] = deserializeTrack(json_object_array_get_idx(jso, i));

	return ret;
}
