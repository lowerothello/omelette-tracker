void drawDummy(instrument *iv, uint8_t index, unsigned short x, unsigned short y, short *cursor, unsigned char fieldpointer)
{
	printf("\033[%d;%dH    DUMMY INSTRUMENT", y, x);
	printf("\033[%d;%dH      does nothing", y + 1, x);
}

/* must be realtime safe */
void dummyProcess(instrument *iv, channel *cv, uint32_t pointer, sample_t *l, sample_t *r)
{}

void dummyChangeType(void **state)
{
	*state = malloc(1);
}

void dummyInit(int index)
{
	t->f[index].indexc = 0;
	t->f[index].statesize = 1;
	t->f[index].draw = &drawDummy;
	t->f[index].process = &dummyProcess;
	t->f[index].changeType = &dummyChangeType;
}
