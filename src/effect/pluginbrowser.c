BrowserState *pbstate;

typedef struct
{
	uint32_t           i;
	LilvIter          *iter;
	const LilvPlugins *lap;
	bool firstLine;
} PluginBrowserData;

char *pluginBrowserGetTitle(void *data)
{
	PluginBrowserData *pbd = data;
	pbd->i = 0;
	pbd->firstLine = 1;
	pbd->lap = lilv_world_get_all_plugins(lv2_db.world);
	pbd->iter = lilv_plugins_begin(pbd->lap);
	return strdup("plugins");
}

uint32_t getPluginDBc(void)
{ return native_db.descc + ladspa_db.descc + lilv_plugins_size(lilv_world_get_all_plugins(lv2_db.world)); }
uint32_t pluginBrowserGetLineCount(void *data) { return getPluginDBc(); }

bool pluginBrowserGetNext(void *data)
{
	PluginBrowserData *pbd = data;

	if (pbd->i >= native_db.descc + ladspa_db.descc)
		pbd->iter = lilv_plugins_next(pbd->lap, pbd->iter);

	if (!pbd->firstLine) pbd->i++;
	else pbd->firstLine = 0;

	return (pbd->i < getPluginDBc());
}
void pluginBrowserDrawLine(BrowserState *b, int y)
{
	PluginBrowserData *pbd = b->data;

	uint32_t ii = 0;
	/* native */
	if (pbd->i < native_db.descc)
	{
		printf("\033[%d;1H%.*s",  y, (ws.ws_col>>1) - 3, native_db.descv[pbd->i - ii]->name);
		printf("\033[%d;%dH%.*s", y,  ws.ws_col>>1, ws.ws_col - (ws.ws_col>>1) - 7, native_db.descv[pbd->i - ii]->author);
		printf("\033[%d;%dH%s",   y,  ws.ws_col - 5, "NATIVE");
		return;
	}
	ii += native_db.descc;

	/* ladspa */
	if (pbd->i < native_db.descc + ladspa_db.descc)
	{
		printf("\033[%d;1H%.*s",  y, (ws.ws_col>>1) - 3, ladspa_db.descv[pbd->i - ii]->Name);
		printf("\033[%d;%dH%.*s", y,  ws.ws_col>>1, ws.ws_col - (ws.ws_col>>1) - 7, ladspa_db.descv[pbd->i - ii]->Maker);
		printf("\033[%d;%dH%s",   y,  ws.ws_col - 5, "LADSPA");
		return;
	}
	ii += ladspa_db.descc;

	/* lv2 */
	/* the last one, so no conditional needed */
	const LilvPlugin *lp = lilv_plugins_get(pbd->lap, pbd->iter);
	LilvNode *node;

	node = lilv_plugin_get_name(lp);
	if (node) {
		printf("\033[%d;1H%.*s", y, (ws.ws_col>>1) - 4, lilv_node_as_string(node));
		lilv_node_free(node);
	}

	node = lilv_plugin_get_author_name(lp);
	if (node) {
		printf("\033[%d;%dH%.*s", y, ws.ws_col>>1, ws.ws_col - (ws.ws_col>>1) - 7, lilv_node_as_string(node));
		lilv_node_free(node);
	}

	printf("\033[%d;%dH%s", y, ws.ws_col - 5, "LV2");
}

void cb_addEffectLadspaAfter(Event *e)
{
	cc.cursor = getCursorFromEffect(*w->pluginbrowserchain, (size_t)e->callbackarg);
	cb_addEffect(e);
}
void pluginBrowserCommit(BrowserState *b)
{
	w->page = w->oldpage;
	if (w->pluginbrowserbefore) addEffect(w->pluginbrowserchain, b->cursor,     getEffectFromCursor(*w->pluginbrowserchain, cc.cursor), cb_addEffect);
	else                        addEffect(w->pluginbrowserchain, b->cursor, MIN(getEffectFromCursor(*w->pluginbrowserchain, cc.cursor) + 1, (*w->pluginbrowserchain)->c), cb_addEffectLadspaAfter);
}

BrowserState *initPluginBrowser(void)
{
	BrowserState *ret = calloc(1, sizeof(BrowserState));

	ret->getTitle     = pluginBrowserGetTitle;
	ret->getNext      = pluginBrowserGetNext;
	ret->drawLine     = pluginBrowserDrawLine;
	ret->getLineCount = pluginBrowserGetLineCount;
	ret->commit       = pluginBrowserCommit;

	ret->data = calloc(1, sizeof(PluginBrowserData));

	return ret;
}

void freePluginBrowser(BrowserState *b) { free(b->data); }



void drawPluginEffectBrowser(void) { drawBrowser(pbstate); }

void pluginEffectBrowserInput(int input)
{
	int button, x, y;
	switch (input)
	{
		case '\n': case '\r':
			pbstate->commit(pbstate);
			p->redraw = 1; break;
		case '\033':
			switch (getchar())
			{
				case 'O':
					switch (getchar())
					{
						case 'P': /* xterm f1 */ showTracker   (); break;
						case 'Q': /* xterm f2 */ showInstrument(); break;
						case 'R': /* xterm f3 */ showMaster    (); break;
					} p->redraw = 1; break;
				case '[': /* CSI */
					switch (getchar())
					{
						case '[':
							switch (getchar())
							{
								case 'A': /* linux f1 */ showTracker   (); break;
								case 'B': /* linux f2 */ showInstrument(); break;
								case 'C': /* linux f2 */ showMaster    (); break;
								case 'E': /* linux f5 */ startPlayback(); break;
							} p->redraw = 1; break;
						case 'A': /* up arrow   */ browserUpArrow  (pbstate, 1); p->redraw = 1; break;
						case 'B': /* down arrow */ browserDownArrow(pbstate, 1); p->redraw = 1; break;
						case 'H': /* xterm home */ browserHome     (pbstate);    p->redraw = 1; break;
						case '4': /* end */ if (getchar() == '~') { browserEnd(pbstate); p->redraw = 1; } break;
						case '5': /* page up / shift+scrollup */
							switch (getchar())
							{
								case '~': /* page up */
									browserUpArrow(pbstate, ws.ws_row>>1);
									p->redraw = 1; break;
								case ';': /* shift+scrollup */
									getchar(); /* 2 */
									getchar(); /* ~ */
									browserUpArrow(pbstate, ws.ws_row>>1);
									p->redraw = 1; break;
							} break;
						case '6': /* page dn / shift+scrolldn */
							switch (getchar())
							{
								case '~': /* page dn */
									browserDownArrow(pbstate, ws.ws_row>>1);
									p->redraw = 1; break;
								case ';': /* shift+scrolldn */
									getchar(); /* 2 */
									getchar(); /* ~ */
									browserDownArrow(pbstate, ws.ws_row>>1);
									p->redraw = 1; break;
							} break;
						case '1':
							switch (getchar())
							{
								case '5': /* xterm f5   */ getchar(); startPlayback(); break;
								case '7': /*       f6   */ getchar(); stopPlayback (); break;
								case ';': /* mod+arrow  */ getchar(); break;
								case '~': /* linux home */ browserHome(pbstate); p->redraw = 1; break;
							} break;
						case 'M':
							/* gcc resolves args backwards so store these getchar calls in vars (tcc doesn't have this bug) */
							button = getchar();
							x = getchar()-32;
							y = getchar()-32;
							browserMouse(pbstate, button, x, y);
							p->redraw = 1; break;
					} break;
				default: /* escape */
					w->page = w->oldpage;
					p->redraw = 1; break;
			} break;
	}
}
