typedef struct
{
	char  *prettyname;
	int    keybind;
	void (*callback)(void *);
	void  *arg; /* arg passed to callback */
} TooltipEntry;

typedef struct
{
	short maxprettynamelen;
	char *prettytitle;

	uint8_t       entryc;
	TooltipEntry *entryv;
} TooltipState;


void clearTooltip(TooltipState *tt)
{
	for (int i = 0; i < tt->entryc; i++)
	{
		if (tt->entryv[i].prettyname)
			free(tt->entryv[i].prettyname);
		tt->entryv[i].prettyname = NULL;
	}

	if (tt->entryv)
	{
		free(tt->entryv);
		tt->entryv = NULL;
	}

	if (tt->prettytitle)
	{
		free(tt->prettytitle);
		tt->prettytitle = NULL;
	}

	tt->entryc = 0;
	tt->maxprettynamelen = 0;
}

void setTooltipTitle(TooltipState *tt, char *prettytitle)
{
	if (tt->prettytitle) free(tt->prettytitle);
	tt->prettytitle = malloc(strlen(prettytitle)+1);
	strcpy(tt->prettytitle, prettytitle);
}

void addTooltipBind(TooltipState *tt, char *prettyname, int keybind, void (*callback)(void *), void *arg)
{
	TooltipEntry *newentryv = calloc(tt->entryc+1, sizeof(TooltipEntry));

	if (tt->entryv)
	{
		memcpy(newentryv, tt->entryv, tt->entryc * sizeof(TooltipEntry));
		free(tt->entryv);
	}
	newentryv[tt->entryc].prettyname = malloc(strlen(prettyname)+1);
	strcpy(newentryv[tt->entryc].prettyname, prettyname);
	newentryv[tt->entryc].keybind = keybind;
	newentryv[tt->entryc].callback = callback;
	newentryv[tt->entryc].arg = arg;

	tt->maxprettynamelen = MAX(tt->maxprettynamelen, strlen(prettyname));
	tt->entryv = newentryv;
	tt->entryc++;
}

bool inputTooltip(TooltipState *tt, int input)
{
	switch (input) /* handle counts first */
	{
		case '0': w->count *= 10; w->count += 0; return 1;
		case '1': w->count *= 10; w->count += 1; return 1;
		case '2': w->count *= 10; w->count += 2; return 1;
		case '3': w->count *= 10; w->count += 3; return 1;
		case '4': w->count *= 10; w->count += 4; return 1;
		case '5': w->count *= 10; w->count += 5; return 1;
		case '6': w->count *= 10; w->count += 6; return 1;
		case '7': w->count *= 10; w->count += 7; return 1;
		case '8': w->count *= 10; w->count += 8; return 1;
		case '9': w->count *= 10; w->count += 9; return 1;
		default:
			w->count = MIN(256, w->count);

			for (int i = 0; i < tt->entryc; i++)
			{
				if (tt->entryv[i].keybind == tolower(input)) /* TODO: tolower() should be opt in per control */
				{
					if (tt->entryv[i].callback)
						tt->entryv[i].callback(tt->entryv[i].arg);
					return 0;
				}
			} w->count = 0; break;
	}

	/* no callback triggered */
	clearTooltip(tt);
	return 0;
}

void drawTooltip(TooltipState *tt)
{
	if (!tt->entryc) return;
	printf("\033[s");
	short w = tt->maxprettynamelen + 6;
	short h = tt->entryc + 2;
	short x = ws.ws_col - w - 1;
	short y = ws.ws_row - h - 1;

	for (short i = 0; i < tt->entryc; i++)
		printf("\033[%d;%dH│ %s  \033[2m%c\033[m │", y+1+i, x, tt->entryv[i].prettyname, tt->entryv[i].keybind);

	if (tt->prettytitle)
	{
		printf("\033[%d;%dH┌\033[%d;%dH┐", y-1, x, y-1, x+w);
		printf("\033[%d;%dH│\033[%d;%dH│", y,   x, y,   x+w);
		for (short i = 1; i < w; i++) printf("\033[%d;%dH ", y,   x+i);
		for (short i = 1; i < w; i++) printf("\033[%d;%dH─", y-1, x+i);
		printf("\033[%d;%dH\033[1m%s\033[m", y, x + ((w - ((short)strlen(tt->prettytitle)-1))>>1), tt->prettytitle);
	} else
	{
		printf("\033[%d;%dH┌\033[%d;%dH┐", y, x, y, x+w);
		for (short i = 1; i < w; i++) printf("\033[%d;%dH─", y, x+i);
	}

	printf("\033[%d;%dH└\033[%d;%dH┘", y+h-1, x, y+h-1, x+w);
	for (short i = 1; i < w; i++) printf("\033[%d;%dH─", y+h-1, x+i);
	
	/* set the cursor pos */
	printf("\033[u");
}
