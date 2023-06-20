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

void setRepl(
		bool    (*callback)(char*, void *arg),
		void (*keycallback)(char*, void *arg),
		void (*tabcallback)(char*, void *arg),
		void *arg,
		bool historyenabled,
		char *prompt,
		char *startvalue
	)
{
	w->repl.callback = callback;
	w->repl.keycallback = keycallback;
	w->repl.tabcallback = tabcallback;
	w->repl.arg = arg;

	w->repl.history = (int)historyenabled - 1; /* 0 for history, -1 for no history */

	strcpy(w->repl.prompt, prompt);
	strcpy(w->repl.historyv[w->repl.historyc], startvalue);
	w->repl.reploffset = strlen(startvalue);
}

void drawRepl(void)
{
	if (w->mode == MODE_REPL) /* repl mode */
	{
		printf("\033[?25h\033[%d;0H%s%s\033[%d;%dH", ws.ws_row, w->repl.prompt, w->repl.historyv[w->repl.historyc], ws.ws_row, (w->repl.reploffset + (unsigned short)strlen(w->repl.prompt) + 1) % ws.ws_col);
		w->repl.error[0] = '\0';
	} else if (strlen(w->repl.error))
		printf("\033[s\033[%d;0H%s\033[u", ws.ws_row, w->repl.error);
}

static void previousHistory(void *arg)
{
	if (w->repl.history < 0) return;
	w->repl.history = MIN((w->repl.history + 1) % REPL_HISTORY_LENGTH, w->repl.historyc);
	strcpy(w->repl.historyv[w->repl.historyc], w->repl.historyv[w->repl.historyc - w->repl.history]);
	w->repl.reploffset = strlen(w->repl.historyv[w->repl.historyc]);
	p->redraw = 1;
}
static void nextHistory(void *arg)
{
	if (w->repl.history < 0) return;
	w->repl.history = MAX(w->repl.history - 1, 0);
	strcpy(w->repl.historyv[w->repl.historyc], w->repl.historyv[w->repl.historyc - w->repl.history]);
	w->repl.reploffset = strlen(w->repl.historyv[w->repl.historyc]);
	p->redraw = 1;
}
static void cursorLeft (void *arg)
{
	if (w->repl.reploffset) w->repl.reploffset--;
	p->redraw = 1;
}
static void cursorRight(void *arg)
{
	if (w->repl.reploffset < strlen(w->repl.historyv[w->repl.historyc]))
		w->repl.reploffset++;
	p->redraw = 1;
}
static void cursorHome (void *arg)
{
	w->repl.reploffset = 0;
	p->redraw = 1;
}
static void cursorEnd  (void *arg)
{
	w->repl.reploffset = strlen(w->repl.historyv[w->repl.historyc]);
	p->redraw = 1;
}

static void replAutocomplete(void *arg)
{
	if (w->repl.tabcallback) w->repl.tabcallback(w->repl.historyv[w->repl.historyc], w->repl.arg);
	w->repl.reploffset = strlen(w->repl.historyv[w->repl.historyc]);
	p->redraw = 1;
}
static void replBackspace(void *arg)
{
	if (w->repl.reploffset > 0)
	{
		char *s = w->repl.historyv[w->repl.historyc];
		size_t slen = strlen(s);
		for (int i = 0; i < slen; i++)
			if (s[i] != '\0' && i > w->repl.reploffset - 2)
				s[i] = s[i + 1];
		w->repl.reploffset--;
		if (w->repl.keycallback) w->repl.keycallback(s, w->repl.arg);
	}
	p->redraw = 1;
}
static void replCtrlU(void *arg)
{
	char *s = w->repl.historyv[w->repl.historyc];
	memcpy(s, s + w->repl.reploffset, REPL_LENGTH - w->repl.reploffset);
	w->repl.reploffset = 0;
	if (w->repl.keycallback) w->repl.keycallback(s, w->repl.arg);
	p->redraw = 1;
}
static void replCtrlK(void *arg)
{
	char *s = w->repl.historyv[w->repl.historyc];
	s[w->repl.reploffset] = '\0';
	if (w->repl.keycallback) w->repl.keycallback(s, w->repl.arg);
	p->redraw = 1;
}

static void replEscape(void *arg) { w->mode = w->oldmode; p->redraw = 1; }

static void replCommit(void *arg)
{
	w->mode = w->oldmode;

	char *s = w->repl.historyv[w->repl.historyc];
	if (strcmp(s, ""))
	{
		if (w->repl.keycallback)
			w->repl.keycallback(s, w->repl.arg);

		if (w->repl.callback)
			if (w->repl.callback(s, w->repl.arg))
				cleanup(0);

		if (w->repl.history < 0) goto replCommitEnd;
		w->repl.historyc++;

		/* protect against reaching an int limit, VERY unnecessary lol */
		if (w->repl.historyc >= REPL_HISTORY_LENGTH * 2)
			w->repl.historyc =  REPL_HISTORY_LENGTH;
	}
replCommitEnd:
	p->redraw = 1;
}

static void replInputKey(void *key)
{
	char *s = w->repl.historyv[w->repl.historyc];
	s[strlen(s) + 1] = '\0'; /* ensure that the nullbyte at the end of the string is safe to overwrite by putting another one after it */
	for (int i = strlen(s); i > 0; i--)
		if (i > w->repl.reploffset - 1) s[i + 1] = s[i];
		else break;

	s[w->repl.reploffset] = (char)(size_t)key;
	w->repl.reploffset++;
	if (w->repl.keycallback) w->repl.keycallback(s, w->repl.arg);
	p->redraw = 1;
}

void initReplInput(void)
{
	setTooltipTitle("repl");
	addTooltipBind("previous history"  , 0          , XK_Up       , 0      , previousHistory , NULL);
	addTooltipBind("next history"      , 0          , XK_Down     , 0      , nextHistory     , NULL);
	addTooltipBind("cursor left"       , 0          , XK_Left     , 0      , cursorLeft      , NULL);
	addTooltipBind("cursor right"      , 0          , XK_Right    , 0      , cursorRight     , NULL);
	addTooltipBind("cursor home"       , 0          , XK_Home     , 0      , cursorHome      , NULL);
	addTooltipBind("cursor end"        , 0          , XK_End      , 0      , cursorEnd       , NULL);
	addTooltipBind("leave repl mode"   , 0          , XK_Escape   , 0      , replEscape      , NULL);
	addTooltipBind("autocomplete"      , 0          , XK_Tab      , TT_DRAW, replAutocomplete, NULL);
	addTooltipBind("commit"            , 0          , XK_Return   , TT_DRAW, replCommit      , NULL);
	addTooltipBind("backspace"         , 0          , XK_BackSpace, 0      , replBackspace   , NULL);
	addTooltipBind("clear to beginning", ControlMask, XK_U        , 0      , replCtrlU       , NULL);
	addTooltipBind("clear to end"      , ControlMask, XK_K        , 0      , replCtrlK       , NULL);
	addPrintableAsciiBinds("input", 0, replInputKey);
}
