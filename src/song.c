Song *addSong(void)
{
	Song *cs = calloc(1, sizeof(Song));
	if (!cs) return NULL;

	cs->master = newEffectChain();

	cs->rowhighlight = 4;
	cs->songbpm = DEF_BPM;
	cs->plen = DEFAULT_PATTERN_LENGTH;

	cs->inst = calloc(1, sizeof(InstChain));
	memset(cs->inst->i, INSTRUMENT_VOID, sizeof(uint8_t) * INSTRUMENT_MAX);

	cs->track = calloc(1, sizeof(TrackChain));
	cs->track->v = calloc(STARTING_TRACKC, sizeof(Track*));
	cs->track->c = STARTING_TRACKC;
	for (uint8_t i = 0; i < STARTING_TRACKC; i++)
		cs->track->v[i] = allocTrack(cs, NULL);

	return cs;
}

void freeSong(Song *cs)
{
	if (!cs) return;

	freeEffectChain(cs->master);

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
	json_object_object_add(jso, "plen", json_object_new_int(cs->plen));
	json_object_object_add(jso, "rowhighlight", json_object_new_int(cs->rowhighlight));
	json_object_object_add(jso, "songbpm", json_object_new_int(cs->songbpm));

	json_object_object_add(jso, "master", serializeEffectChain(cs->master));

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
	ret->plen = json_object_get_int(json_object_object_get(jso, "plen"));
	ret->rowhighlight = json_object_get_int(json_object_object_get(jso, "rowhighlight"));
	ret->songbpm = json_object_get_int(json_object_object_get(jso, "songbpm"));

	ret->master = deserializeEffectChain(json_object_object_get(jso, "master"));

	ret->inst = deserializeInstChain(json_object_object_get(jso, "inst"), (void*)((size_t)buffer + strlen(buffer)+1), ratemultiplier);
	ret->track = deserializeTrackChain(json_object_object_get(jso, "track"));

	json_object_put(jso);
	json_tokener_free(jtok);
	free(buffer);

	return ret;
}
