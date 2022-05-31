void drawDummy(instrument *iv, uint8_t index, unsigned short x, unsigned short y, short *cursor, unsigned char fieldpointer)
{
	printf("\033[%d;%dH    DUMMY INSTRUMENT", y, x);
	printf("\033[%d;%dH      does nothing", y + 1, x);
}

/* must be realtime safe */
void dummyProcess(instrument *iv, channel *cv, sample_t *l, sample_t *r)
{
	cv->samplepointer++;
	return;
}

void dummyInit(int index)
{
	t->f[index].draw = &drawDummy;
	t->f[index].process = &dummyProcess;
}
