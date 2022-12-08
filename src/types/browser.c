void resizeBrowser(BrowserState *b, short x, short y, short w, short h)
{
	b->x = x;
	b->y = y;
	b->w = w;
	b->h = h;
}

void drawBrowser(BrowserState *b)
{
	char *line;
	line = b->getTitle(b->data);

	if (line)
	{
		if (strlen(line) > b->w) printf("\033[%d;%dH%.*s", b->y, b->x, b->w, line + ((unsigned short)strlen(line) - b->w));
		else                     printf("\033[%d;%dH%.*s", b->y, b->x + ((b->w - (unsigned short)strlen(line))>>1), b->w, line);
		free(line);
	}

	int yo = b->y + (b->h>>1) - b->cursor; /* TODO: can overflow, yo should be unsigned */
	while ((b->getNext(b->data)))
	{
		if (yo > b->y && yo < b->y + b->h)
			b->drawLine(b, yo);
		yo++;
	}

	/* draw the cursor */
	printf("\033[%d;%dH", b->y + (b->h>>1) + b->fyoffset, b->x);
}

void browserUpArrow(BrowserState *b, size_t count)
{
	count *= MAX(1, w->count);
	if (b->cursor < count) b->cursor = 0;
	else                   b->cursor -= count;
	if (b->cursorCB) b->cursorCB(b->data);
	p->redraw = 1;
}
void browserDownArrow(BrowserState *b, size_t count)
{
	count *= MAX(1, w->count);
	b->cursor += count;
	if (b->cursor > b->getLineCount(b->data) - 1)
		b->cursor = b->getLineCount(b->data) - 1;
	if (b->cursorCB) b->cursorCB(b->data);
	p->redraw = 1;
}
void browserHome(BrowserState *b)
{
	b->cursor = 0;
	if (b->cursorCB) b->cursorCB(b->data);
	p->redraw = 1;
}
void browserEnd(BrowserState *b)
{
	b->cursor = b->getLineCount(b->data) - 1;
	if (b->cursorCB) b->cursorCB(b->data);
	p->redraw = 1;
}

static void browserApplyFyoffset(BrowserState *b)
{
	if (!b->fyoffset) return;

	if (b->fyoffset < 0) browserUpArrow  (b, -b->fyoffset);
	else                 browserDownArrow(b,  b->fyoffset);
	b->fyoffset = 0;
}
void browserMouse(BrowserState *b, enum Button button, int x, int y)
{
	switch (button)
	{
		case WHEEL_UP: case WHEEL_UP_CTRL:             /* scroll up     */ browserUpArrow  (b, WHEEL_SPEED); break;
		case WHEEL_DOWN: case WHEEL_DOWN_CTRL:         /* scroll down   */ browserDownArrow(b, WHEEL_SPEED); break;
		case BUTTON_RELEASE: case BUTTON_RELEASE_CTRL: /* release click */ browserApplyFyoffset(b); break;

		case BUTTON1:      case BUTTON3:
		case BUTTON1_CTRL: case BUTTON3_CTRL:
		case BUTTON1_HOLD: case BUTTON1_HOLD_CTRL:
			if (y <= b->y || y >= b->y + b->h) break; /* ignore clicking out of range */

			b->fyoffset = y - (b->y + (b->h>>1));

			if (button == BUTTON3 || button == BUTTON3_CTRL)
			{
				browserApplyFyoffset(b);
				b->commit(b);
			} break;
		default: break;
	}
}
