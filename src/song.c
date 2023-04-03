Song *addSong(void)
{
	Song *cs = calloc(1, sizeof(Song));
	if (!cs) return NULL;

	cs->master = newEffectChain();
	cs->send   = newEffectChain();

	cs->rowhighlight = 4;
	cs->songbpm = DEF_BPM;

	return cs;
}

#define STARTING_TRACKC 4 /* how many tracks to allocate for new files */
void initSong(Song *cs)
{
	cs->inst = calloc(1, sizeof(InstChain));
	memset(cs->inst->i, INSTRUMENT_VOID, sizeof(uint8_t) * INSTRUMENT_MAX);

	cs->track = calloc(1, sizeof(TrackChain));
	cs->track->v = calloc(STARTING_TRACKC, sizeof(Track*));
	regenGlobalRowc(cs);
	cs->track->c = STARTING_TRACKC;
	for (uint8_t i = 0; i < STARTING_TRACKC; i++)
		cs->track->v[i] = allocTrack(cs, NULL);
}

void freeSong(Song *cs)
{
	if (!cs) return;

	freeEffectChain(cs->master);
	freeEffectChain(cs->send);

	for (int i = 0; i < cs->track->c; i++)
	{
		_delTrack(cs, cs->track->v[i]);
		free(cs->track->v[i]);
	}
	free(cs->track->v);
	free(cs->track);

	const InstAPI *api;
	for (int i = 0; i < cs->inst->c; i++)
		if ((api = instGetAPI(cs->inst->v[i].type))) api->free(&cs->inst->v[i]);
	free(cs->inst);

	free(cs);
}

void reapplyBpm(void)
{
	Event e;
	e.sem = M_SEM_BPM;
	pushEvent(&e);
}

void serializeSong(FILE *fp, Song *cs)
{
	struct json_object *jso = json_object_new_object();
	json_object_object_add(jso, "version", json_object_new_int(version));
	json_object_object_add(jso, "samplerate", json_object_new_int(samplerate));
	json_object_object_add(jso, "rowhighlight", json_object_new_int(cs->rowhighlight));
	json_object_object_add(jso, "songbpm", json_object_new_int(cs->songbpm));
	json_object_object_add(jso, "songlen", json_object_new_int(cs->songlen));

	struct json_object *array;
	array = json_object_new_array_ext(3);
	for (int i = 0; i < 3; i++)
		json_object_array_add(array, json_object_new_int(cs->loop[i]));
	json_object_object_add(jso, "loop", array);

	json_object_object_add(jso, "master", serializeEffectChain(cs->master));
	json_object_object_add(jso, "send", serializeEffectChain(cs->send));

	json_object_object_add(jso, "inst", serializeInstChain(cs->inst));
	json_object_object_add(jso, "track", serializeTrackChain(cs->track));

	fputs(json_object_to_json_string_ext(jso, JSON_C_TO_STRING_PLAIN), fp);
	json_object_put(jso);

	fputc('\0', fp);

	serializeInstChainData(fp, cs->inst);
}

Song *deserializeSong(FILE *fp)
{
	fseek(fp, 0, SEEK_END);
	off_t length = ftello(fp);
	fseek(fp, 0, SEEK_SET);

	char *buffer = calloc(1, length);
	fread(buffer, 1, length, fp);

	struct json_tokener *jtok = json_tokener_new();
	struct json_object *jso = json_tokener_parse_ex(jtok, buffer, strlen(buffer));

	// uint16_t version = json_object_get_int(json_object_object_get(jso, "version"));
	double ratemultiplier = (double)samplerate / json_object_get_double(json_object_object_get(jso, "samplerate"));

	Song *ret = calloc(1, sizeof(Song));
	ret->rowhighlight = json_object_get_int(json_object_object_get(jso, "rowhighlight"));
	ret->songbpm = json_object_get_int(json_object_object_get(jso, "songbpm"));
	ret->songlen = json_object_get_int(json_object_object_get(jso, "songlen"));

	for (int i = 0; i < 3; i++)
		ret->loop[i] = json_object_get_int(json_object_array_get_idx(json_object_object_get(jso, "loop"), i));

	ret->master = deserializeEffectChain(json_object_object_get(jso, "master"));
	ret->send = deserializeEffectChain(json_object_object_get(jso, "send"));

	ret->inst = deserializeInstChain(json_object_object_get(jso, "inst"), (void*)((size_t)buffer + strlen(buffer)+1), ratemultiplier);
	ret->track = deserializeTrackChain(json_object_object_get(jso, "track"));

	json_object_put(jso);
	json_tokener_free(jtok);
	free(buffer);

	return ret;
}
