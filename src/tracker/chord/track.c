void chordClearTrack(void *_)
{
	initTrackData(&s->track->v[w->track].data, s->songlen);
	regenGlobalRowc(s);
}
void chordAddTrack(void *_)
{
	addTrack(s, w->track+1, MAX(1, w->count));
	w->track = MIN(TRACK_MAX-1, w->track + MAX(1, w->count)); /* atomically safe */
}
void chordAddBefore(void *_) { addTrack(s, w->track, MAX(1, w->count)); }
void chordDeleteTrack(void *_) { delTrack(w->track, MAX(1, w->count)); }
void chordCopyTrack(void *_)
{
	copyTrackdata(&w->trackbuffer, &s->track->v[w->track].data);
	regenGlobalRowc(s);
}
void chordPasteTrack(void *_)
{
	copyTrackdata(&s->track->v[w->track].data, &w->trackbuffer);
	for (int i = 1; i < MAX(1, w->count); i++)
	{
		if (s->track->c >= TRACK_MAX) break;
		w->track++;
		copyTrackdata(&s->track->v[w->track].data, &w->trackbuffer);
	}
	regenGlobalRowc(s);
}


void setChordTrack(void *tt)
{
	clearTooltip(tt);
	setTooltipTitle(tt, "track");
	addCountBinds(tt, 0);
	addTooltipBind(tt, "clear track ", 0, XK_c     , TT_DRAW, chordClearTrack , NULL);
	addTooltipBind(tt, "add track   ", 0, XK_a     , TT_DRAW, chordAddTrack   , NULL);
	addTooltipBind(tt, "add before  ", 0, XK_A     , TT_DRAW, chordAddBefore  , NULL);
	addTooltipBind(tt, "delete track", 0, XK_d     , TT_DRAW, chordDeleteTrack, NULL);
	addTooltipBind(tt, "copy track  ", 0, XK_y     , TT_DRAW, chordCopyTrack  , NULL);
	addTooltipBind(tt, "paste track ", 0, XK_p     , TT_DRAW, chordPasteTrack , NULL);
	addTooltipBind(tt, "return"      , 0, XK_Escape, 0      , NULL            , NULL);
	w->chord = 'c'; p->redraw = 1;
}
