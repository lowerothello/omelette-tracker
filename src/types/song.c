Song *_addSong(void)
{
	Song *cs = calloc(1, sizeof(Song));
	if (!cs) return NULL;

	cs->rowhighlight = 4;
	cs->songbpm = DEF_BPM;

	return cs;
}

#define STARTING_CHANNELC 4 /* how many channels to allocate for new files */
Song *addSong(void)
{
	Song *ret = _addSong();

	ret->instrument = calloc(1, sizeof(InstrumentChain));
	memset(ret->instrument->i, INSTRUMENT_VOID, sizeof(uint8_t) * INSTRUMENT_MAX);

	ret->channel = calloc(1, sizeof(ChannelChain) + STARTING_CHANNELC * sizeof(Channel));
	regenGlobalRowc(ret);
	ret->channel->c = STARTING_CHANNELC;
	for (uint8_t i = 0; i < STARTING_CHANNELC; i++)
		_addChannel(ret, &ret->channel->v[i]);

	return ret;
}

void delSong(Song *cs)
{
	for (int i = 0; i < cs->channel->c; i++)
		_delChannel(&cs->channel->v[i]);
	free(cs->channel);

	for (int i = 0; i < cs->instrument->c; i++)
		_delInstrument(&cs->instrument->v[i]);
	free(cs->instrument);

	if (cs->bpmcache) free(cs->bpmcache);

	free(cs);
}

void reapplyBpm(void)
{
	Event e;
	e.sem = M_SEM_BPM;
	pushEvent(&e);
}
