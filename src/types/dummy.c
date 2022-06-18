void drawDummy(instrument *iv, uint8_t index, unsigned short x, unsigned short y, short *cursor, unsigned char fieldpointer)
{
	printf("\033[%d;%dH    DUMMY INSTRUMENT", y, x);
	printf("\033[%d;%dH      does nothing", y + 1, x);
}

void dummyMouseToIndex(int, int, int, short *index, signed char *)
{
	*index = 0;
}

/* must be realtime safe */
void dummyProcess(instrument *iv, channel *cv, uint32_t pointer, sample_t *l, sample_t *r)
{}

void dummyInitType(void **state)
{
	*state = malloc(1);
}

void dummyWrite(void **, FILE *) {}
void dummyRead(void **, FILE *) {}

void dummyInit(int index)
{
	t->f[index].indexc = 0;
	t->f[index].statesize = 1;
	t->f[index].draw = &drawDummy;
	t->f[index].mouseToIndex = &dummyMouseToIndex;
	t->f[index].process = &dummyProcess;
	t->f[index].initType = &dummyInitType;
	t->f[index].write = &dummyWrite;
	t->f[index].read = &dummyRead;
}
