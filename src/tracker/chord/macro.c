void chordAddMacro(void *_)
{
	for (int i = 0; i < MAX(1, w->count); i++)
		if (s->channel->v[w->channel].data.macroc < 7)
			s->channel->v[w->channel].data.macroc++;
	regenGlobalRowc(s);
}
void chordDelMacro(void *_)
{
	for (int i = 0; i < MAX(1, w->count); i++)
	{
		if (s->channel->v[w->channel].data.macroc) s->channel->v[w->channel].data.macroc--;
		if (w->trackerfx > 3 + s->channel->v[w->channel].data.macroc*2)
			w->trackerfx = 3 + s->channel->v[w->channel].data.macroc*2;
	} regenGlobalRowc(s);
}
void chordSetMacro(void *_)
{
	if (w->count) s->channel->v[w->channel].data.macroc = MIN(8, w->count) - 1;
	else          s->channel->v[w->channel].data.macroc = 1;
	if (w->trackerfx > 3 + s->channel->v[w->channel].data.macroc*2)
		w->trackerfx = 3 + s->channel->v[w->channel].data.macroc*2;
	regenGlobalRowc(s);
}


void setChordMacro(void) {
	clearTooltip(&tt);
	setTooltipTitle(&tt, "macro");
	addTooltipBind(&tt, "increment macro columns   ", 'a', chordAddMacro, NULL);
	addTooltipBind(&tt, "decrement macro columns   ", 'd', chordDelMacro, NULL);
	addTooltipBind(&tt, "set macro columns to count", 'm', chordSetMacro, NULL);
	w->chord = 'm';
}
