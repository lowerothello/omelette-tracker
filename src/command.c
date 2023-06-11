void wordSplit(char *output, char *line, int wordt)
{
	int wordc = 0;
	size_t p;
	strcpy(output, "");
	char lastwhitespace = 0;

	size_t linelen = strlen(line);
	for (size_t i = 0; i < linelen; i++)
	{
		if (isspace(line[i]))
		{
			if (!lastwhitespace)
				if (wordc++ > wordt) break;
			lastwhitespace = 1;
		} else
		{
			if (wordc == wordt)
			{
				p = strlen(output);
				output[p + 1] = '\0';
				output[p + 0] = line[i];
			}
			lastwhitespace = 0;
		}
	}
}

void setCommand(
		bool    (*callback)(char*, void *arg),
		void (*keycallback)(char*, void *arg),
		void (*tabcallback)(char*, void *arg),
		void *arg,
		bool historyenabled,
		char *prompt,
		char *startvalue
	)
{
	w->command.callback = callback;
	w->command.keycallback = keycallback;
	w->command.tabcallback = tabcallback;
	w->command.arg = arg;

	w->command.history = (int)historyenabled - 1; /* 0 for history, -1 for no history */

	strcpy(w->command.prompt, prompt);
	strcpy(w->command.historyv[w->command.historyc], startvalue);
	w->command.commandptr = strlen(startvalue);
}

void drawCommand(void)
{
	if (w->mode == MODE_COMMAND) /* command mode */
	{
		printf("\033[?25h\033[%d;0H%s%s\033[%d;%dH", ws.ws_row, w->command.prompt, w->command.historyv[w->command.historyc], ws.ws_row, (w->command.commandptr + (unsigned short)strlen(w->command.prompt) + 1) % ws.ws_col);
		w->command.error[0] = '\0';
	} else if (strlen(w->command.error))
		printf("\033[s\033[%d;0H%s\033[u", ws.ws_row, w->command.error);
}

static void previousHistory(void *arg)
{
	if (w->command.history < 0) return;
	w->command.history = MIN((w->command.history + 1) % COMMAND_HISTORY_LENGTH, w->command.historyc);
	strcpy(w->command.historyv[w->command.historyc], w->command.historyv[w->command.historyc - w->command.history]);
	w->command.commandptr = strlen(w->command.historyv[w->command.historyc]);
	p->redraw = 1;
}
static void nextHistory(void *arg)
{
	if (w->command.history < 0) return;
	w->command.history = MAX(w->command.history - 1, 0);
	strcpy(w->command.historyv[w->command.historyc], w->command.historyv[w->command.historyc - w->command.history]);
	w->command.commandptr = strlen(w->command.historyv[w->command.historyc]);
	p->redraw = 1;
}
static void cursorLeft (void *arg)
{
	if (w->command.commandptr) w->command.commandptr--;
	p->redraw = 1;
}
static void cursorRight(void *arg)
{
	if (w->command.commandptr < strlen(w->command.historyv[w->command.historyc]))
		w->command.commandptr++;
	p->redraw = 1;
}
static void cursorHome (void *arg)
{
	w->command.commandptr = 0;
	p->redraw = 1;
}
static void cursorEnd  (void *arg)
{
	w->command.commandptr = strlen(w->command.historyv[w->command.historyc]);
	p->redraw = 1;
}

static void commandAutocomplete(void *arg)
{
	if (w->command.tabcallback) w->command.tabcallback(w->command.historyv[w->command.historyc], w->command.arg);
	w->command.commandptr = strlen(w->command.historyv[w->command.historyc]);
	p->redraw = 1;
}
static void commandBackspace(void *arg)
{
	if (w->command.commandptr > 0)
	{
		char *s = w->command.historyv[w->command.historyc];
		size_t slen = strlen(s);
		for (int i = 0; i < slen; i++)
			if (s[i] != '\0' && i > w->command.commandptr - 2)
				s[i] = s[i + 1];
		w->command.commandptr--;
		if (w->command.keycallback) w->command.keycallback(s, w->command.arg);
	}
	p->redraw = 1;
}
static void commandCtrlU(void *arg)
{
	char *s = w->command.historyv[w->command.historyc];
	memcpy(s, s + w->command.commandptr, COMMAND_LENGTH - w->command.commandptr);
	w->command.commandptr = 0;
	if (w->command.keycallback) w->command.keycallback(s, w->command.arg);
	p->redraw = 1;
}
static void commandCtrlK(void *arg)
{
	char *s = w->command.historyv[w->command.historyc];
	s[w->command.commandptr] = '\0';
	if (w->command.keycallback) w->command.keycallback(s, w->command.arg);
	p->redraw = 1;
}

static void commandEscape(void *arg) { w->mode = w->oldmode; p->redraw = 1; }

static void commandCommit(void *arg)
{
	w->mode = w->oldmode;

	char *s = w->command.historyv[w->command.historyc];
	if (strcmp(s, ""))
	{
		if (w->command.keycallback)
			w->command.keycallback(s, w->command.arg);

		if (w->command.callback)
			if (w->command.callback(s, w->command.arg))
				cleanup(0);

		if (w->command.history < 0) goto commandCommitEnd;
		w->command.historyc++;

		/* protect against reaching an int limit, VERY unnecessary lol */
		if (w->command.historyc >= COMMAND_HISTORY_LENGTH * 2)
			w->command.historyc =  COMMAND_HISTORY_LENGTH;
	}
commandCommitEnd:
	p->redraw = 1;
}

static void commandInputKey(void *key)
{
	char *s = w->command.historyv[w->command.historyc];
	s[strlen(s) + 1] = '\0'; /* ensure that the nullbyte at the end of the string is safe to overwrite by putting another one after it */
	for (int i = strlen(s); i > 0; i--)
		if (i > w->command.commandptr - 1) s[i + 1] = s[i];
		else break;

	s[w->command.commandptr] = (char)(size_t)key;
	w->command.commandptr++;
	if (w->command.keycallback) w->command.keycallback(s, w->command.arg);
	p->redraw = 1;
}

void initCommandInput(void)
{
	setTooltipTitle("command");
	addTooltipBind("previous history"  , 0          , XK_Up       , 0      , previousHistory    , NULL);
	addTooltipBind("next history"      , 0          , XK_Down     , 0      , nextHistory        , NULL);
	addTooltipBind("cursor left"       , 0          , XK_Left     , 0      , cursorLeft         , NULL);
	addTooltipBind("cursor right"      , 0          , XK_Right    , 0      , cursorRight        , NULL);
	addTooltipBind("cursor home"       , 0          , XK_Home     , 0      , cursorHome         , NULL);
	addTooltipBind("cursor end"        , 0          , XK_End      , 0      , cursorEnd          , NULL);
	addTooltipBind("leave command mode", 0          , XK_Escape   , 0      , commandEscape      , NULL);
	addTooltipBind("autocomplete"      , 0          , XK_Tab      , TT_DRAW, commandAutocomplete, NULL);
	addTooltipBind("commit"            , 0          , XK_Return   , TT_DRAW, commandCommit      , NULL);
	addTooltipBind("backspace"         , 0          , XK_BackSpace, 0      , commandBackspace   , NULL);
	addTooltipBind("clear to beginning", ControlMask, XK_U        , 0      , commandCtrlU       , NULL);
	addTooltipBind("clear to end"      , ControlMask, XK_K        , 0      , commandCtrlK       , NULL);
	addPrintableAsciiBinds("input", 0, commandInputKey);
}
