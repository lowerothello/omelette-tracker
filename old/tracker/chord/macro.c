void chordAddMacro(void *_)
{
	for (int i = 0; i < MAX(1, w->count); i++)
		if (s->track->v[w->track].data.variant->macroc < 7)
			s->track->v[w->track].data.variant->macroc++;
	regenGlobalRowc(s);
}
void chordDelMacro(void *_)
{
	for (int i = 0; i < MAX(1, w->count); i++)
	{
		if (s->track->v[w->track].data.variant->macroc) s->track->v[w->track].data.variant->macroc--;
		if (w->trackerfx > 3 + s->track->v[w->track].data.variant->macroc*2)
			w->trackerfx = 3 + s->track->v[w->track].data.variant->macroc*2;
	} regenGlobalRowc(s);
}
void chordSetMacro(void *_)
{
	if (w->count) s->track->v[w->track].data.variant->macroc = MIN(8, w->count) - 1;
	else          s->track->v[w->track].data.variant->macroc = 1;
	if (w->trackerfx > 3 + s->track->v[w->track].data.variant->macroc*2)
		w->trackerfx = 3 + s->track->v[w->track].data.variant->macroc*2;
	regenGlobalRowc(s);
}


void setChordMacro(void *tt)
{
	clearTooltip(tt);
	setTooltipTitle(tt, "macro");
	addCountBinds(tt, 0);
	addTooltipBind(tt, "increment macro columns   ", 0, XK_a     , TT_DRAW, chordAddMacro, NULL);
	addTooltipBind(tt, "decrement macro columns   ", 0, XK_d     , TT_DRAW, chordDelMacro, NULL);
	addTooltipBind(tt, "set macro columns to count", 0, XK_m     , TT_DRAW, chordSetMacro, NULL);
	addTooltipBind(tt, "return"                    , 0, XK_Escape, 0      , NULL         , NULL);
	w->chord = 'm'; p->redraw = 1;
}
