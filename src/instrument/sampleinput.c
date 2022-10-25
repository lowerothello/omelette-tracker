void instrumentSampleUpArrow(int count)
{
	Instrument *iv = &s->instrument->v[s->instrument->i[w->instrument]];
	if (!iv->sample->length)
		filebrowserUpArrow(count);
	else
		decControlCursor(&cc, count);
}
void instrumentSampleDownArrow(int count)
{
	Instrument *iv = &s->instrument->v[s->instrument->i[w->instrument]];
	if (!iv->sample->length)
		filebrowserDownArrow(count);
	else
		incControlCursor(&cc, count);
}
void instrumentSampleLeftArrow(void)
{
	Instrument *iv = &s->instrument->v[s->instrument->i[w->instrument]];
	if (!iv->sample->length)
		filebrowserLeftArrow(1);
	else
		incControlFieldpointer(&cc);
}
void instrumentSampleRightArrow(void)
{
	Instrument *iv = &s->instrument->v[s->instrument->i[w->instrument]];
	if (!iv->sample->length)
		filebrowserRightArrow(1);
	else
		decControlFieldpointer(&cc);
}
void instrumentSampleHome(void)
{
	Instrument *iv = &s->instrument->v[s->instrument->i[w->instrument]];
	if (!iv->sample->length)
		filebrowserHome();
	else
		setControlCursor(&cc, 0);
}
void instrumentSampleEnd(void)
{
	Instrument *iv = &s->instrument->v[s->instrument->i[w->instrument]];
	if (!iv->sample->length)
		filebrowserEnd();
	else
		setControlCursor(&cc, cc.controlc-1);
}
void instrumentSampleReturn(void)
{
	Instrument *iv = &s->instrument->v[s->instrument->i[w->instrument]];
	if (!iv->sample->length)
		filebrowserReturn();
	else
		toggleKeyControl(&cc);
}
void instrumentSampleBackspace(void)
{
	Instrument *iv = &s->instrument->v[s->instrument->i[w->instrument]];
	if (!iv->sample->length)
		filebrowserBackspace();
	else
		revertKeyControl(&cc);
}
void instrumentSamplePreview(int input)
{
	Instrument *iv = &s->instrument->v[s->instrument->i[w->instrument]];
	if (iv->algorithm != INST_ALG_MIDI && !iv->sample->length)
		filebrowserPreview(input);
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
		case '0': w->octave = 0; p->redraw = 1; break;
		case '1': w->octave = 1; p->redraw = 1; break;
		case '2': w->octave = 2; p->redraw = 1; break;
		case '3': w->octave = 3; p->redraw = 1; break;
		case '4': w->octave = 4; p->redraw = 1; break;
		case '5': w->octave = 5; p->redraw = 1; break;
		case '6': w->octave = 6; p->redraw = 1; break;
		case '7': w->octave = 7; p->redraw = 1; break;
		case '8': w->octave = 8; p->redraw = 1; break;
		case '9': w->octave = 9; p->redraw = 1; break;
		default: instrumentSamplePreview(input); break;
	}
	return 0;
}
