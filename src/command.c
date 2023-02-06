void wordSplit(char *output, char *line, int wordt)
{
	int wordc = 0;
	size_t p;
	strcpy(output, "");
	char lastwhitespace = 0;

	for (size_t i = 0; i < strlen(line); i++)
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

void setCommand(Command *command,
		bool (*callback)(char*, enum _Mode*),
		void (*keycallback)(char*),
		void (*tabcallback)(char*),
		char historyenabled,
		char *prompt,
		char *startvalue)
{
	command->callback = callback;
	command->keycallback = keycallback;
	command->tabcallback = tabcallback;
	if (historyenabled)
		command->history = 0;
	else
		command->history = -1;

	strcpy(command->prompt, prompt);
	strcpy(command->historyv[command->historyc], startvalue);
	command->commandptr = strlen(startvalue);
}

void drawCommand(Command *command, enum _Mode mode)
{
	if (mode == MODE_COMMAND) /* command mode */
	{
		printf("\033[?25h\033[%d;0H%s%s\033[%d;%dH", ws.ws_row, command->prompt, command->historyv[command->historyc], ws.ws_row, (command->commandptr + (unsigned short)strlen(command->prompt) + 1) % ws.ws_col);
		command->error[0] = '\0';
	} else if (strlen(command->error))
		printf("\033[s\033[%d;0H%s\033[u", ws.ws_row, command->error);
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
	if (w->command.tabcallback) w->command.tabcallback(w->command.historyv[w->command.historyc]);
	w->command.commandptr = strlen(w->command.historyv[w->command.historyc]);
	p->redraw = 1;
}
static void commandBackspace(void *arg)
{
	if (w->command.commandptr > 0)
	{
		for (int i = 0; i < strlen(w->command.historyv[w->command.historyc]); i++)
			if (w->command.historyv[w->command.historyc][i] != '\0' && i > w->command.commandptr - 2)
				w->command.historyv[w->command.historyc][i] = w->command.historyv[w->command.historyc][i + 1];
		w->command.commandptr--;
		if (w->command.keycallback) w->command.keycallback(w->command.historyv[w->command.historyc]);
	}
	p->redraw = 1;
}
static void commandCtrlU(void *arg)
{
	memcpy(w->command.historyv[w->command.historyc], w->command.historyv[w->command.historyc] + w->command.commandptr, COMMAND_LENGTH - w->command.commandptr);
	w->command.commandptr = 0;
	if (w->command.keycallback) w->command.keycallback(w->command.historyv[w->command.historyc]);
	p->redraw = 1;
}
static void commandCtrlK(void *arg)
{
	w->command.historyv[w->command.historyc][w->command.commandptr] = '\0';
	if (w->command.keycallback) w->command.keycallback(w->command.historyv[w->command.historyc]);
	p->redraw = 1;
}

static void commandEscape(void *arg) { w->mode = w->oldmode; p->redraw = 1; }

static void commandCommit(void *arg)
{
	w->mode = w->oldmode;

	if (strcmp(w->command.historyv[w->command.historyc], ""))
	{
		if (w->command.keycallback) w->command.keycallback(w->command.historyv[w->command.historyc]);
		if (w->command.callback) if (w->command.callback  (w->command.historyv[w->command.historyc], &w->mode)) cleanup(0);

		if (w->command.history < 0) cleanup(0);
		w->command.historyc++;

		/* protect against reaching an int limit, VERY unnecessary lol */
		if (w->command.historyc >= COMMAND_HISTORY_LENGTH * 2)
			w->command.historyc =  COMMAND_HISTORY_LENGTH;
	}
	p->redraw = 1;
}

static void commandInputKey(void *key)
{
	w->command.historyv[w->command.historyc][strlen(w->command.historyv[w->command.historyc]) + 1] = '\0';
	for (int i = strlen(w->command.historyv[w->command.historyc]); i > 0; i--)
		if (i > w->command.commandptr - 1) w->command.historyv[w->command.historyc][i + 1] = w->command.historyv[w->command.historyc][i];
		else break;

	w->command.historyv[w->command.historyc][w->command.commandptr] = (char)(size_t)key;
	w->command.commandptr++;
	if (w->command.keycallback) w->command.keycallback(w->command.historyv[w->command.historyc]);
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
