void chordClearChannel(void *_)
{
	initChannelData(s, &s->channel->v[w->channel].data);
	regenGlobalRowc(s);
}
void chordAddChannel(void *_)
{
	addChannel(s, w->channel+1, MAX(1, w->count));
	w->channel = MIN(CHANNEL_MAX-1, w->channel + MAX(1, w->count)); /* atomically safe */
}
void chordAddBefore(void *_) { addChannel(s, w->channel, MAX(1, w->count)); }
void chordDeleteChannel(void *_) { delChannel(w->channel, MAX(1, w->count)); }
void chordCopyChannel(void *_)
{
	copyChanneldata(&w->channelbuffer, &s->channel->v[w->channel].data);
	regenGlobalRowc(s);
}
void chordPasteChannel(void *_)
{
	copyChanneldata(&s->channel->v[w->channel].data, &w->channelbuffer);
	for (int i = 1; i < MAX(1, w->count); i++)
	{
		if (s->channel->c >= CHANNEL_MAX) break;
		w->channel++;
		copyChanneldata(&s->channel->v[w->channel].data, &w->channelbuffer);
	}
	regenGlobalRowc(s);
}


void setChordChannel(void) {
	clearTooltip(&tt);
	setTooltipTitle(&tt, "channel");
	addTooltipBind(&tt, "clear channel ", 'c', chordClearChannel,  NULL);
	addTooltipBind(&tt, "add channel   ", 'a', chordAddChannel,    NULL);
	addTooltipBind(&tt, "add before    ", 'A', chordAddBefore,     NULL);
	addTooltipBind(&tt, "delete channel", 'd', chordDeleteChannel, NULL);
	addTooltipBind(&tt, "copy channel  ", 'y', chordCopyChannel,   NULL);
	addTooltipBind(&tt, "paste channel ", 'p', chordPasteChannel,  NULL);
	w->chord = 'c';
}
