/*
 * tuple, 24bit colour
 */
typedef struct {
	uint8_t r, g, b;
} Tuple;

enum TermColourType {
	TRUECOLOUR_NONE,
	TRUECOLOUR_LINUX,
	TRUECOLOUR_XTERM,
};

typedef struct {
	Tuple   colour[8];
	uint8_t semaphore; /* if this is >= 8 then all colours have been updated */
	enum TermColourType type;
} TrueColourState;

void getTrueColourType(TrueColourState *tc)
{
#ifdef DISABLE_TRUECOLOUR
	tc->type = TRUECOLOUR_NONE;
#else
	if (getenv("OML_NOTC"))               { tc->type = TRUECOLOUR_NONE ; return; }
	if (!strcmp(getenv("TERM"), "LINUX")) { tc->type = TRUECOLOUR_LINUX; return; }

	/* TODO: proper test for truecolour r/w support */
	tc->type = TRUECOLOUR_XTERM;
#endif
}

/*
 * read the first 8 terminal colours into tc
 */
void trueColourReadStateNonblock(TrueColourState *tc)
{
	tc->semaphore = 0;
	switch (tc->type)
	{
		case TRUECOLOUR_XTERM: {
				for (int i = 0; i < 8; i++)
					printf("\033]4;%d;?\007", i);
			} break;
		default: break;
	}
}
void trueColourReadStateBlock(TrueColourState *tc)
{
	tc->semaphore = 0;
	int index;
	unsigned int r, g, b;
	switch (tc->type)
	{
		case TRUECOLOUR_XTERM: {
				for (int i = 0; i < 8; i++)
					printf("\033]4;%d;?\007", i);

				for (int i = 0; i < 8; i++)
				{
					scanf("\033]4;%d;rgb:%02x%*02x/%02x%*02x/%02x%*02x\007", &index, &r, &g, &b);
					tc->colour[index].r = r;
					tc->colour[index].g = g;
					tc->colour[index].b = b;
				}
			} break;
		default: break;
	}
}

/*
 * write tc to the first 8 terminal colours
 */
void trueColourWriteState(TrueColourState *tc)
{
	switch (tc->type)
	{
		case TRUECOLOUR_LINUX:
			for (int i = 0; i < 8; i++)
				printf("\033]P%x%02x%02x%02x", i,
						tc->colour[i].r,
						tc->colour[i].g,
						tc->colour[i].b);
		case TRUECOLOUR_XTERM:
			for (int i = 0; i < 8; i++)
				printf("\033]4;%d;#%02x%02x%02x\007", i,
						tc->colour[i].r,
						tc->colour[i].g,
						tc->colour[i].b);
			break;
		case TRUECOLOUR_NONE: break;
	}
}

/*
 * set the active fg/bg colour to a tuple
 */
void bgTuple(TrueColourState *tc, Tuple t) { if (tc->type != TRUECOLOUR_NONE) printf("\033[48;2;%d;%d;%dm", t.r, t.g, t.b); }
void fgTuple(TrueColourState *tc, Tuple t) { if (tc->type != TRUECOLOUR_NONE) printf("\033[38;2;%d;%d;%dm", t.r, t.g, t.b); }

/* internal function to avoid clipping */
static uint8_t oversample(int32_t big)
{
	big = big < 0xff ? big : 0xff; /* MIN */
	big = big > 0x00 ? big : 0x00; /* MAX */
	return big;
}

/*
 * fade between two tuples
 * f == 0.0f is t1
 * f == 1.0f is t2
 */
Tuple lerpTuple(float f, Tuple t1, Tuple t2)
{
	Tuple t;
	t.r = oversample((int32_t)t1.r*(1.0f - f) + (int32_t)t2.r*(f));
	t.g = oversample((int32_t)t1.g*(1.0f - f) + (int32_t)t2.g*(f));
	t.b = oversample((int32_t)t1.b*(1.0f - f) + (int32_t)t2.b*(f));
	return t;
}
Tuple greyscaleTuple(TrueColourState *tc, float f)
{
	return lerpTuple(f, tc->colour[0], tc->colour[7]);
}

/*
 * add two tuples together, subtract if f is negative
 */
Tuple addTuple(float f, Tuple t1, Tuple t2)
{
	Tuple t;
	t.r = oversample((int32_t)t1.r + (int32_t)t2.r*(f));
	t.g = oversample((int32_t)t1.g + (int32_t)t2.g*(f));
	t.b = oversample((int32_t)t1.b + (int32_t)t2.b*(f));
	return t;
}
