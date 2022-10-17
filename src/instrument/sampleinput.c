void instrumentSampleUpArrow(int count)
{
	Instrument *iv = &s->instrument->v[s->instrument->i[w->instrument]];
	if (!iv->sample.length || !iv->sample.data)
		filebrowserUpArrow(count);
	else
		decControlCursor(&cc, count);
}
void instrumentSampleDownArrow(int count)
{
	Instrument *iv = &s->instrument->v[s->instrument->i[w->instrument]];
	if (!iv->sample.length || !iv->sample.data)
		filebrowserDownArrow(count);
	else
		incControlCursor(&cc, count);
}
void instrumentSampleLeftArrow(void)
{
	Instrument *iv = &s->instrument->v[s->instrument->i[w->instrument]];
	if (!iv->sample.length || !iv->sample.data)
		filebrowserLeftArrow(1);
	else
		incControlFieldpointer(&cc);
}
void instrumentSampleRightArrow(void)
{
	Instrument *iv = &s->instrument->v[s->instrument->i[w->instrument]];
	if (!iv->sample.length || !iv->sample.data)
		filebrowserRightArrow(1);
	else
		decControlFieldpointer(&cc);
}
void instrumentSampleHome(void)
{
	Instrument *iv = &s->instrument->v[s->instrument->i[w->instrument]];
	if (!iv->sample.length || !iv->sample.data)
		filebrowserHome();
	else
		setControlCursor(&cc, 0);
}
void instrumentSampleEnd(void)
{
	Instrument *iv = &s->instrument->v[s->instrument->i[w->instrument]];
	if (!iv->sample.length || !iv->sample.data)
		filebrowserEnd();
	else
		setControlCursor(&cc, cc.controlc-1);
}
void instrumentSampleReturn(void)
{
	Instrument *iv = &s->instrument->v[s->instrument->i[w->instrument]];
	if (!iv->sample.length || !iv->sample.data)
		filebrowserReturn();
	else
		toggleKeyControl(&cc);
}
void instrumentSampleBackspace(void)
{
	Instrument *iv = &s->instrument->v[s->instrument->i[w->instrument]];
	if (!iv->sample.length || !iv->sample.data)
		filebrowserBackspace();
	else
		revertKeyControl(&cc);
}
void instrumentSamplePreview(int input)
{
	Instrument *iv = &s->instrument->v[s->instrument->i[w->instrument]];
	if (iv->algorithm != INST_ALG_MIDI && (!iv->sample.length || !iv->sample.data))
		filebrowserPreview(input);
	else
		previewNote(input, w->instrument);
}

int inputInstrumentSample(int input)
{
	switch (input)
	{
		case '\n': case '\r': instrumentSampleReturn   (); p->dirty = 1; break;
		case '\b': case 127:  instrumentSampleBackspace(); p->dirty = 1; break;
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
		default: instrumentSamplePreview(input); break;
	}
	return 0;
}
