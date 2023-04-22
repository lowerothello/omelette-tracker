#define STAGING_FORMAT "\033[1m"
#define RESET_FORMAT "\033[34m"

/* undefined behaviour if multibyte chars are printed  */
/* only culls the x axis, culling the y axis is simple */
void printCulling(const char *s, short x, short y, short minx, short maxx)
{
	if (x < minx) { if (minx - x < MIN(strlen(s), maxx - x)) printf("\033[%d;%dH%.*s", y, minx, (int)(maxx - x - (minx - x)), s+MIN(minx - x, strlen(s))); }
	else if (x <= maxx)                                      printf("\033[%d;%dH%.*s", y, x, (maxx+1) - x, s);
}

/* draw an opaque bounding box on the screen using box drawing glyphs */
void drawBoundingBox(short x, short y, short w, short h, short xmin, short xmax, short ymin, short ymax)
{
	int i;
	for (i = 0; i <= h; i++)
		if (ymin <= y+i && ymax >= y+i)
		{
			printf("\033[%d;%dH\033[%dX", y+i, x+w, MAX(xmin, x) - MIN(xmax, x+w));
#ifndef DISABLE_BOX_OUTLINE
			if (x-1 >= xmin && x-1 <= xmax) printf("\033[%d;%dH│", y+i, x-1);
			if (x+w >= xmin && x+w <= xmax) printf("\033[%d;%dH│", y+i, x+w);
		}
	if (ymin <= y && ymax >= y)
	{
		for (i = MAX(xmin, x); i <= MIN(xmax, x+w); i++)
			printf("\033[%d;%dH─", y, i);
		if (x-1 >= xmin && x-1 <= xmax) printf("\033[%d;%dH┌", y, x-1);
		if (x+w >= xmin && x+w <= xmax) printf("\033[%d;%dH┐", y, x+w);
	}
	if (ymin <= y+h && ymax >= y+h)
	{
		for (i = MAX(xmin, x); i <= MIN(xmax, x+w); i++)
			printf("\033[%d;%dH─", y+h, i);
		if (x-1 >= xmin && x-1 <= xmax) printf("\033[%d;%dH└", y+h, x-1);
		if (x+w >= xmin && x+w <= xmax) printf("\033[%d;%dH┘", y+h, x+w);
#endif /* DISABLE_BOX_OUTLILNE */
	} /* can be in either of 2 indentations, depending on the result of the #ifdef */
}

void drawVerticalLine(short x, short y, short h, short xmin, short xmax, short ymin, short ymax)
{
	if (x < xmin || x > xmax) return;

	for (int i = 0; i <= h; i++)
		if (ymin <= y+i && ymax >= y+i)
			printf("\033[%d;%dH│", y+i, x);
}

void drawHorizontalLine(short x, short y, short w, short xmin, short xmax, short ymin, short ymax)
{
	if (y < ymin || y > ymax) return;

	for (int i = 0; i <= w; i++)
		if (xmin <= x+i && xmax >= x+i)
			printf("\033[%d;%dH─", y, x+i);
}

struct termios origterm;
void initTerminal(void)
{
	puts("\033[?1049h"); /* switch to the back buffer */
	puts("\033[?1002h"); /* enable mouse */

	struct termios term;
	tcgetattr(1, &term);
	origterm = term;
	term.c_lflag &= (~ECHO & ~ICANON);
	tcsetattr(1, TCSANOW, &term); /* disable ECHO and ICANON */

	/* truecolour */
	// fcntl(0, F_SETFL, 0); /* ensure blocking io */
	// getTrueColourType(&tc);
	// trueColourReadStateBlock(&tc);

	fcntl(0, F_SETFL, O_NONBLOCK); /* non-blocking io */
}

void cleanupTerminal(void)
{
	puts("\033[?1002l"); /* disable mouse */

	fcntl(0, F_SETFL, 0); /* reset to blocking stdin reads */
	tcsetattr(1, TCSANOW, &origterm); /* reset to the original termios */

	printf("\033[%d;0H\033[2K", ws.ws_row); /* clear the screen and leave the cursor at the bottom left */
	puts("\033[?1049l"); /* reset to the front buffer */
}
