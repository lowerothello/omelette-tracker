void chordClearTrack(void)
{
	initTrackData(&s->track->v[w->track].data, s->songlen);
	regenGlobalRowc(s);
}

void chordAddTrack(void)
{
	addTrack(s, w->track+1, MAX(1, w->count), NULL);
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
	copyTrackData(&w->trackbuffer, &s->track->v[w->track].data);
	regenGlobalRowc(s);
}

void setChordTrack(void *tt)
{
	clearTooltip(tt);
	setTooltipTitle(tt, "track");
	addCountBinds(tt, 0);
	addTooltipBind(tt, "clear track ", 0, XK_c     , TT_DRAW, (void(*)(void*))chordClearTrack , NULL);
	addTooltipBind(tt, "add track   ", 0, XK_a     , TT_DRAW, (void(*)(void*))chordAddTrack   , NULL);
	addTooltipBind(tt, "add before  ", 0, XK_A     , TT_DRAW, (void(*)(void*))chordAddBefore  , NULL);
	addTooltipBind(tt, "delete track", 0, XK_d     , TT_DRAW, (void(*)(void*))chordDeleteTrack, NULL);
	addTooltipBind(tt, "copy track  ", 0, XK_y     , TT_DRAW, (void(*)(void*))chordCopyTrack  , NULL);
	addTooltipBind(tt, "paste track ", 0, XK_p     , TT_DRAW, (void(*)(void*))chordPasteTrack , NULL);
	addTooltipBind(tt, "paste before", 0, XK_P     , TT_DRAW, (void(*)(void*))chordPasteBefore, NULL);
	addTooltipBind(tt, "return"      , 0, XK_Escape, 0      , NULL            , NULL);
	w->chord = 'c'; p->redraw = 1;
}
