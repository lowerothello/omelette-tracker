void chordClearTrack(void)
{
	initTrackData(s->track->v[w->track]); /* TODO: not atomic */
	p->redraw = 1;
}

void chordAddTrack(void)
{
	addTrack(s, w->track+1, MAX(1, w->count), NULL);
	/* aaa */
	w->track = MIN(TRACK_MAX-1, w->track + MAX(1, w->count)); /* atomically safe */
}
void chordPasteTrack(void)
{
	addTrack(s, w->track+1, MAX(1, w->count), &w->trackbuffer);
	w->track = MIN(TRACK_MAX-1, w->track + MAX(1, w->count)); /* atomically safe */
}

void chordAddBefore  (void) { addTrack(s, w->track, MAX(1, w->count), NULL); }
void chordPasteBefore(void) { addTrack(s, w->track, MAX(1, w->count), &w->trackbuffer); }

void chordDeleteTrack(void) { delTrack(w->track, MAX(1, w->count)); }

void chordCopyTrack(void)
{
	copyTrack(&w->trackbuffer, s->track->v[w->track]);
	p->redraw = 1;
}

void setChordTrack(void)
{
	clearTooltip();
	setTooltipTitle("track");
	addCountBinds(0);
	addTooltipBind("clear track ", 0, XK_c     , TT_DRAW, (void(*)(void*))chordClearTrack , NULL);
	addTooltipBind("add track   ", 0, XK_a     , TT_DRAW, (void(*)(void*))chordAddTrack   , NULL);
	addTooltipBind("add before  ", 0, XK_A     , TT_DRAW, (void(*)(void*))chordAddBefore  , NULL);
	addTooltipBind("delete track", 0, XK_d     , TT_DRAW, (void(*)(void*))chordDeleteTrack, NULL);
	addTooltipBind("copy track  ", 0, XK_y     , TT_DRAW, (void(*)(void*))chordCopyTrack  , NULL);
	addTooltipBind("paste track ", 0, XK_p     , TT_DRAW, (void(*)(void*))chordPasteTrack , NULL);
	addTooltipBind("paste before", 0, XK_P     , TT_DRAW, (void(*)(void*))chordPasteBefore, NULL);
	addTooltipBind("return"      , 0, XK_Escape, 0      , NULL            , NULL);
	w->chord = 't'; p->redraw = 1;
}
