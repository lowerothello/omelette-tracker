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


static void pluginBrowserMouse(enum _BUTTON button, int x, int y)
{
	browserMouse(pbstate, button, x, y);
	p->redraw = 1;
}

static void pluginBrowserUpArrow  (void *count) { browserUpArrow  (pbstate, (size_t)count); }
static void pluginBrowserDownArrow(void *count) { browserDownArrow(pbstate, (size_t)count); }
static void pluginBrowserPgUp(void *count) { browserUpArrow  (pbstate, (size_t)count * (ws.ws_row>>1)); }
static void pluginBrowserPgDn(void *count) { browserDownArrow(pbstate, (size_t)count * (ws.ws_row>>1)); }

static void pluginBrowserEscape(void *arg)
{
	w->page = w->oldpage;
	p->redraw = 1;
}
static void pluginBrowserCommitBind(void *arg) /* TODO: shitty disambiguation */
{
	pbstate->commit(pbstate);
	p->redraw = 1;
}

void initPluginEffectBrowserInput(TooltipState *tt)
{
	setTooltipTitle(tt, "pluginbrowser");
	setTooltipMouseCallback(tt, pluginBrowserMouse);
	addTooltipBind(tt, "cursor up"   , 0, XK_Up       , 0, pluginBrowserUpArrow       , (void*)1);
	addTooltipBind(tt, "cursor down" , 0, XK_Down     , 0, pluginBrowserDownArrow     , (void*)1);
	addTooltipBind(tt, "cursor home" , 0, XK_Home     , 0, (void(*)(void*))browserHome, pbstate );
	addTooltipBind(tt, "cursor end"  , 0, XK_End      , 0, (void(*)(void*))browserEnd , pbstate );
	addTooltipBind(tt, "cursor pgup" , 0, XK_Page_Up  , 0, pluginBrowserPgUp          , (void*)1);
	addTooltipBind(tt, "cursor pgdn" , 0, XK_Page_Down, 0, pluginBrowserPgDn          , (void*)1);
	addTooltipBind(tt, "return"      , 0, XK_Escape   , 0, pluginBrowserEscape        , NULL    );
	addTooltipBind(tt, "commit"      , 0, XK_Return   , 0, pluginBrowserCommitBind    , NULL    );
}
