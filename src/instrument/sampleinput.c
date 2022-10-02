void instrumentSampleUpArrow  (int count) { decControlCursor(&cc, count); }
void instrumentSampleDownArrow(int count) { incControlCursor(&cc, count); }
void instrumentSampleLeftArrow (void) { incControlFieldpointer(&cc); }
void instrumentSampleRightArrow(void) { decControlFieldpointer(&cc); }
void instrumentSampleHome(void) { setControlCursor(&cc, 0);             }
void instrumentSampleEnd (void) { setControlCursor(&cc, cc.controlc-1); }

int inputInstrumentSample(int input)
{
	switch (input)
	{
		case '\n': case '\r': toggleKeyControl(&cc); p->dirty = 1; break;
		case 1:  /* ^a */ incControlValue(&cc); p->dirty = 1; break;
		case 24: /* ^x */ decControlValue(&cc); p->dirty = 1; break;
		case '0': w->octave = 0; p->dirty = 1; break;
		case '1': w->octave = 1; p->dirty = 1; break;
		case '2': w->octave = 2; p->dirty = 1; break;
		case '3': w->octave = 3; p->dirty = 1; break;
		case '4': w->octave = 4; p->dirty = 1; break;
		case '5': w->octave = 5; p->dirty = 1; break;
		case '6': w->octave = 6; p->dirty = 1; break;
		case '7': w->octave = 7; p->dirty = 1; break;
		case '8': w->octave = 8; p->dirty = 1; break;
		case '9': w->octave = 9; p->dirty = 1; break;
		default: previewNote(input, w->instrument); break;
	}
	return 0;
}
