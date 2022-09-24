song *_addSong(void)
{
	song *cs = calloc(1, sizeof(song));
	if (!cs) return NULL;

	cs->instrumentc = 0;

	cs->rowhighlight = 4;
	cs->songbpm = DEF_BPM;
	w->request = REQ_BPM;

	memset(cs->instrumenti, INSTRUMENT_VOID, sizeof(uint8_t) * INSTRUMENT_MAX);

	return cs;
}

void delSong(song *cs)
{
	for (int i = 0; i < cs->channelc; i++)
		_delChannel(&cs->channelv[i]);
	free(cs->channelv);

	for (int i = 0; i < cs->instrumentc; i++)
		_delInstrument(&cs->instrumentv[i]);
	free(cs->instrumentv);

	free(cs);
}
