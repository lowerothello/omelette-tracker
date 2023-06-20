void resizeBrowser(BrowserState *b, short x, short y, short w, short h)
{
	b->x = x;
	b->y = y;
	b->w = w;
	b->h = h;
}

void browserFree(BrowserState *b)
{
	if (b->data) free(b->data);
	if (b->search) free(b->search);
	free(b);
}

static char *tolowerString(char *s, size_t slen)
{
	for (size_t i = 0; i < slen; i++)
		s[i] = tolower(s[i]);
	return s;
}

static bool browserSearch(BrowserState *b)
{
	bool ret = 0;
	if (b->search && strlen(b->search))
	{
		char *searchline = b->searchLine(b->data);
		if (searchline)
		{
			tolowerString(searchline, strlen(searchline));
			if (strstr(searchline, b->search))
				ret = 1;
			free(searchline);
		}
	}
	return ret;
}

void drawBrowser(BrowserState *b)
{
	char *line = b->getTitle(b->data);
	if (line)
	{
		if (strlen(line) > b->w) printf("\033[%d;%dH%.*s", b->y, b->x, b->w, line + ((unsigned short)strlen(line) - b->w));
		else                     printf("\033[%d;%dH%.*s", b->y, b->x + ((b->w - 3 - (unsigned short)strlen(line))>>1), b->w-3, line);
		free(line);
	}

	int yo = b->y + (b->h>>1) - b->cursor; /* TODO: can overflow, yo should be unsigned */
	while (b->getNext(b->data))
	{
		if (yo > b->y && yo < b->y + b->h)
		{
			if (browserSearch(b)) printf("\033[44m\033[%d;%dH\033[%d@", yo, b->x, b->w - 3);
			b->drawLine(b, yo);
			printf("\033[m");
		}
		yo++;
	}

	drawVerticalScrollbar(b->x + b->w - 2, b->y, b->h, b->getLineCount(b->data), b->cursor);

	/* draw the cursor */
	printf("\033[%d;%dH", b->y + (b->h>>1) + b->fyoffset, b->x);
}

static void _browserUpArrow(BrowserState *b, int count)
{
	count *= MAX(1, w->count);
	if (b->cursor < count) b->cursor = 0;
	else                   b->cursor -= count;
	if (b->cursorCB) b->cursorCB(b->data);
	p->redraw = 1;
}
static void _browserDownArrow(BrowserState *b, int count)
{
	count *= MAX(1, w->count);
	b->cursor += count;
	if (b->cursor > b->getLineCount(b->data) - 1)
		b->cursor = b->getLineCount(b->data) - 1;
	if (b->cursorCB) b->cursorCB(b->data);
	p->redraw = 1;
}
static void browserUpArrow  (BrowserState *b) { _browserUpArrow  (b, 1); }
static void browserDownArrow(BrowserState *b) { _browserDownArrow(b, 1); }
static void browserPgUp(BrowserState *b) { _browserUpArrow  (b, ws.ws_row>>1); }
static void browserPgDn(BrowserState *b) { _browserDownArrow(b, ws.ws_row>>1); }

static void browserHome(BrowserState *b)
{
	b->cursor = 0;
	if (b->cursorCB) b->cursorCB(b->data);
	p->redraw = 1;
}
static void browserEnd(BrowserState *b)
{
	b->cursor = b->getLineCount(b->data) - 1;
	if (b->cursorCB) b->cursorCB(b->data);
	p->redraw = 1;
}

static void browserApplyFyoffset(BrowserState *b)
{
	if (!b->fyoffset) return;

	if (b->fyoffset < 0) _browserUpArrow  (b, -b->fyoffset);
	else                 _browserDownArrow(b,  b->fyoffset);
	b->fyoffset = 0;
}
static void browserCommit(BrowserState *b)
{
	b->commit(b);
	p->redraw = 1;
}
void browserMouse(BrowserState *b, enum Button button, int x, int y)
{
	switch (button)
	{
		case WHEEL_UP: case WHEEL_UP_CTRL:             /* scroll up     */ _browserUpArrow  (b, WHEEL_SPEED); break;
		case WHEEL_DOWN: case WHEEL_DOWN_CTRL:         /* scroll down   */ _browserDownArrow(b, WHEEL_SPEED); break;
		case BUTTON_RELEASE: case BUTTON_RELEASE_CTRL: /* release click */ browserApplyFyoffset(b); break;

		case BUTTON1:      case BUTTON3:
		case BUTTON1_CTRL: case BUTTON3_CTRL:
		case BUTTON1_HOLD: case BUTTON1_HOLD_CTRL:
			if (y <= b->y || y >= b->y + b->h) break; /* ignore clicking out of range */

			b->fyoffset = y - (b->y + (b->h>>1));

			if (button == BUTTON3 || button == BUTTON3_CTRL)
			{
				browserApplyFyoffset(b);
				browserCommit(b);
			} break;
		default: break;
	}
}

static void browserSearchNext(BrowserState *b, bool includecurrent)
{
	free(b->getTitle(b->data)); /* TODO: redundant alloc */

	uint32_t i = 0;
	while (b->getNext(b->data))
	{
		if (i >= b->cursor + 1 - includecurrent && browserSearch(b))
		{
			b->cursor = i;
			return;
		}
		i++;
	}
	/* TODO: needs to loop */
}
static void browserSearchNextBind(BrowserState *b) { browserSearchNext(b, 0); p->redraw = 1; }

static void browserSearchPrev(BrowserState *b, bool includecurrent)
{
	free(b->getTitle(b->data)); /* TODO: redundant alloc */

	uint32_t i = 0;
	uint32_t lastfind = b->cursor + 1;
	while (b->getNext(b->data))
	{
		if (i >= b->cursor - includecurrent) /* slight underflow is actually safe here */
		{
			if (lastfind == b->cursor + 1)
			{
				/* TODO: needs to loop */
			} else
				b->cursor = lastfind;

			return;
		}

		if (browserSearch(b))
			lastfind = i;
		i++;
	}
}
static void browserSearchPrevBind(BrowserState *b) { browserSearchPrev(b, 0); p->redraw = 1; }

static void browserSearchKeyCallback(char *repl, void *arg)
{
	BrowserState *b = arg;

	if (b->search) free(b->search);
	b->search = strdup(repl);
	browserSearchNext(b, 1);
	p->redraw = 1;
}

static void browserSearchStart(BrowserState *b)
{
	if (b->search) { free(b->search); b->search = NULL; }
	setRepl(NULL, browserSearchKeyCallback, NULL, b, 0, "/", "");
	w->mode = MODE_REPL;
	p->redraw = 1;
}

void addBrowserBinds(BrowserState *b)
{
	addTooltipBind("cursor up"   , 0, XK_Up       , 0, (void(*)(void*))browserUpArrow       , b);
	addTooltipBind("cursor down" , 0, XK_Down     , 0, (void(*)(void*))browserDownArrow     , b);
	addTooltipBind("cursor home" , 0, XK_Home     , 0, (void(*)(void*))browserHome          , b);
	addTooltipBind("cursor end"  , 0, XK_End      , 0, (void(*)(void*))browserEnd           , b);
	addTooltipBind("cursor pgup" , 0, XK_Page_Up  , 0, (void(*)(void*))browserPgUp          , b);
	addTooltipBind("cursor pgdn" , 0, XK_Page_Down, 0, (void(*)(void*))browserPgDn          , b);
	// addTooltipBind("return"      , 0, XK_Escape   , 0, pluginBrowserEscape                  , NULL    );
	addTooltipBind("commit"      , 0, XK_Return   , 0, (void(*)(void*))browserCommit        , b);
}
void addBrowserSearchBinds(BrowserState *b)
{
	addTooltipBind("search start", 0, XK_slash    , 0, (void(*)(void*))browserSearchStart   , b);
	addTooltipBind("search next" , 0, XK_n        , 0, (void(*)(void*))browserSearchNextBind, b);
	addTooltipBind("search prev" , 0, XK_N        , 0, (void(*)(void*))browserSearchPrevBind, b);
}
