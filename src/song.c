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

	cs->playfy = STATE_ROWS;
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

	if (cs->bpmcache) free(cs->bpmcache);

	free(cs);
}

void reapplyBpm(void)
{
	Event e;
	e.sem = M_SEM_BPM;
	pushEvent(&e);
}
