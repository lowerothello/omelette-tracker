/* small library to handle drawing data in columns */

#define MAX_COLUMNS 8
typedef struct
{
	short   pagewidth;
	uint8_t pointer;
	uint8_t columnc;
	short   columnv[MAX_COLUMNS];
	short   margin;
	short   marginremainder;
	short   offset;
} ColumnState;


void resetColumn(ColumnState *s, short pagewidth)
{
	s->pointer = 0;
	s->pagewidth = s->margin = pagewidth;
	s->columnc = 0;
	s->offset = 0;
	memset(s->columnv, 0, sizeof(short) * MAX_COLUMNS);
}

/* returns true on error */
int addColumn(ColumnState *s, short columnwidth)
{
	if (s->pointer) return 1; /* adding a new column invalidates already popped offsets */

	s->columnv[s->columnc] = columnwidth;
	s->columnc++;
	return 0;
}

/* returns < 0 on error */
short getNextColumnOffset(ColumnState *s)
{
	if (s->pointer >= s->columnc) return -1; /* no more columns to pop */

	if (s->pointer == 0)
	{
		/* s->margin already equals s->pagewidth */
		for (int i = 0; i < s->columnc; i++)
			s->margin -= s->columnv[i];

		if (s->margin < 0) return -2; /* columns are wider than the page */

		/* try to distribute the remainder instead of just leaving it at the end */
		s->marginremainder = s->margin % (s->columnc+1);

		s->margin /= s->columnc+1;
		s->offset = s->margin;
	} else
		s->offset += s->columnv[s->pointer - 1] + s->margin + s->marginremainder / (s->columnc-1);

	s->pointer++;
	return s->offset;
}
