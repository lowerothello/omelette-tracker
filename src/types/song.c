Song *_addSong(void)
{
	Song *cs = calloc(1, sizeof(Song));
	if (!cs) return NULL;

	cs->rowhighlight = 4;
	cs->songbpm = DEF_BPM;
	w->request = REQ_BPM;

	return cs;
}

Song *addSong(void)
{
	Song *ret = _addSong();

	ret->instrument = calloc(1, sizeof(InstrumentChain));
	memset(ret->instrument->i, INSTRUMENT_VOID, sizeof(uint8_t) * INSTRUMENT_MAX);

	return ret;
}

void delSong(Song *cs)
{
	for (int i = 0; i < cs->channelc; i++)
		_delChannel(&cs->channelv[i]);
	free(cs->channelv);

	for (int i = 0; i < cs->instrument->c; i++)
		_delInstrument(&cs->instrument->v[i]);
	free(cs->instrument);

	free(cs);
}
