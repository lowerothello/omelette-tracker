static void chordAddCommand(void)
{
	Track *cv = s->track->v[w->track];

	for (int i = 0; i < MAX(1, w->count); i++)
		if (cv->pattern->commandc < 7)
			cv->pattern->commandc++;
	p->redraw = 1;
}
static void chordDelCommand(void)
{
	Track *cv = s->track->v[w->track];

	for (int i = 0; i < MAX(1, w->count); i++)
	{
		if (cv->pattern->commandc)
			cv->pattern->commandc--;
		if (w->trackerfx > 3 + cv->pattern->commandc*2)
			w->trackerfx = 3 + cv->pattern->commandc*2;
	}
	p->redraw = 1;
}
static void chordSetCommand(void)
{
	Track *cv = s->track->v[w->track];

	if (w->count) cv->pattern->commandc = MIN(8, w->count) - 1;
	else          cv->pattern->commandc = 1;
	if (w->trackerfx > 3 + cv->pattern->commandc*2)
		w->trackerfx = 3 + cv->pattern->commandc*2;
	p->redraw = 1;
}


void setChordCommand(void)
{
	clearTooltip();
	setTooltipTitle("command");
	addCountBinds(0);
	addTooltipBind("increment command columns   ", 0, XK_a     , TT_DRAW, (void(*)(void*))chordAddCommand, NULL);
	addTooltipBind("decrement command columns   ", 0, XK_d     , TT_DRAW, (void(*)(void*))chordDelCommand, NULL);
	addTooltipBind("set command columns to count", 0, XK_m     , TT_DRAW, (void(*)(void*))chordSetCommand, NULL);
	addTooltipBind("return"                      , 0, XK_Escape, 0      , NULL                           , NULL);
	w->chord = 'c'; p->redraw = 1;
}
