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

void inputTooltip(TooltipState *tt, int input)
{
	for (int i = 0; i < tt->entryc; i++)
	{
		if (tt->entryv[i].keybind == input)
		{
			if (tt->entryv[i].callback)
				tt->entryv[i].callback(tt->entryv[i].arg);
			break;
		}
	}
}

void drawTooltip(TooltipState *tt)
{
	if (!tt->entryc) return;
	printf("\033[s");
	short w = tt->maxprettynamelen + 6;
	short h = tt->entryc + 2;
	short x = (ws.ws_col - w)>>1;
	short y = (ws.ws_row - h)>>1;

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
