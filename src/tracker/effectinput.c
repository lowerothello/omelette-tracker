int inputTrackEffect(TrackData *cd, int input)
{
	switch (input)
	{
		case 'c':  /* track          */ setChordTrack(); p->redraw = 1; return 1;
		case 'b':  /* bpm              */ if (w->count) { s->songbpm = MIN(255, MAX(32, w->count)); reapplyBpm(); } p->redraw = 1; break;
		case 'o':  /* octave           */ w->octave = MIN(9, w->count); p->redraw = 1; break;
		default: return inputEffect(&cd->effect, input);
	} return 0;
}
