int inputChannelEffect(ChannelData *cd, int input)
{
	switch (input)
	{ /* set count first */
		case '0': w->count *= 10; w->count += 0; p->redraw = 1; return 1;
		case '1': w->count *= 10; w->count += 1; p->redraw = 1; return 1;
		case '2': w->count *= 10; w->count += 2; p->redraw = 1; return 1;
		case '3': w->count *= 10; w->count += 3; p->redraw = 1; return 1;
		case '4': w->count *= 10; w->count += 4; p->redraw = 1; return 1;
		case '5': w->count *= 10; w->count += 5; p->redraw = 1; return 1;
		case '6': w->count *= 10; w->count += 6; p->redraw = 1; return 1;
		case '7': w->count *= 10; w->count += 7; p->redraw = 1; return 1;
		case '8': w->count *= 10; w->count += 8; p->redraw = 1; return 1;
		case '9': w->count *= 10; w->count += 9; p->redraw = 1; return 1;
		default:
			if (w->chord)
			{
				w->count = MIN(256, w->count);
				inputTooltip(&tt, input);
				p->resize = 1;
			} else
				switch (input)
				{
					case 'c':  /* channel          */ setChordChannel(); p->redraw = 1; return 1;
					case 'b':  /* bpm              */ if (w->count) { s->songbpm = MIN(255, MAX(32, w->count)); reapplyBpm(); } p->redraw = 1; break;
					case 'o':  /* octave           */ w->octave = MIN(9, w->count); p->redraw = 1; break;
					default: return inputEffect(&cd->effect, input);
				}
			break;
	} return 0;
}
