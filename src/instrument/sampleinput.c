void instrumentUpArrow(int count)
{
	if (!instrumentSafe(s, w->instrument)) return;
	if (w->showfilebrowser)
		browserUpArrow(fbstate, count);
	else
		decControlCursor(&cc, count);
}
void instrumentDownArrow(int count)
{
	if (!instrumentSafe(s, w->instrument)) return;
	if (w->showfilebrowser)
		browserDownArrow(fbstate, count);
	else
		incControlCursor(&cc, count);
}
void instrumentLeftArrow(void)
{
	if (!instrumentSafe(s, w->instrument)) return;
	if (!w->showfilebrowser)
		incControlFieldpointer(&cc);
}
void instrumentRightArrow(void)
{
	if (!instrumentSafe(s, w->instrument)) return;
	if (!w->showfilebrowser)
		decControlFieldpointer(&cc);
}
void instrumentHome(void)
{
	if (!instrumentSafe(s, w->instrument)) return;
	if (w->showfilebrowser)
		browserHome(fbstate);
	else
		setControlCursor(&cc, 0);
}
void instrumentEnd(void)
{
	if (!instrumentSafe(s, w->instrument)) return;
	if (w->showfilebrowser)
		browserEnd(fbstate);
	else
		setControlCursor(&cc, cc.controlc-1);
}
void instrumentSampleReturn(void)
{
	if (!instrumentSafe(s, w->instrument)) return;
	if (w->showfilebrowser)
		fbstate->commit(fbstate);
	else
		toggleKeyControl(&cc);
}
void instrumentSampleBackspace(void)
{
	if (!instrumentSafe(s, w->instrument)) return;
	if (w->showfilebrowser)
		filebrowserBackspace(fbstate);
	else
		revertKeyControl(&cc);
}
void instrumentSamplePreview(int input)
{
	if (!instrumentSafe(s, w->instrument)) return;
	if (w->showfilebrowser)
		filebrowserPreview(fbstate->data, fbstate->cursor, input);
	else
		previewNote(input, w->instrument);
}

int inputInstrumentSample(int input)
{
	switch (input)
	{
		case '\n': case '\r': instrumentSampleReturn   (); p->redraw = 1; break;
		case '\b': case 127:  instrumentSampleBackspace(); p->redraw = 1; break;
		case 1:  /* ^a */ incControlValue(&cc); p->redraw = 1; break;
		case 24: /* ^x */ decControlValue(&cc); p->redraw = 1; break;
		/* case '0': w->octave = 0; p->redraw = 1; break;
		case '1': w->octave = 1; p->redraw = 1; break;
		case '2': w->octave = 2; p->redraw = 1; break;
		case '3': w->octave = 3; p->redraw = 1; break;
		case '4': w->octave = 4; p->redraw = 1; break;
		case '5': w->octave = 5; p->redraw = 1; break;
		case '6': w->octave = 6; p->redraw = 1; break;
		case '7': w->octave = 7; p->redraw = 1; break;
		case '8': w->octave = 8; p->redraw = 1; break;
		case '9': w->octave = 9; p->redraw = 1; break; */
		default: instrumentSamplePreview(input); break;
	} return 0;
}
