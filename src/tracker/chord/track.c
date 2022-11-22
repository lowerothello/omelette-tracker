void chordClearTrack(void *_)
{
	initTrackData(s, &s->track->v[w->track].data);
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


void setChordTrack(void) {
	clearTooltip(&tt);
	setTooltipTitle(&tt, "track");
	addTooltipBind(&tt, "clear track ", 'c', chordClearTrack,  NULL);
	addTooltipBind(&tt, "add track   ", 'a', chordAddTrack,    NULL);
	addTooltipBind(&tt, "add before  ", 'A', chordAddBefore,   NULL);
	addTooltipBind(&tt, "delete track", 'd', chordDeleteTrack, NULL);
	addTooltipBind(&tt, "copy track  ", 'y', chordCopyTrack,   NULL);
	addTooltipBind(&tt, "paste track ", 'p', chordPasteTrack,  NULL);
	w->chord = 'c';
}
