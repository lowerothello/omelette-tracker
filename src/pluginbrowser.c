struct PluginBrowserData
{
	uint32_t i;
};

static uint32_t getPluginDBc(void)
{
	uint32_t ret = 0;
	for (EffectType i = 0; i < EFFECT_TYPE_COUNT; i++)
		if (effect_api[i].db_count)
			ret += effect_api[i].db_count();
	return ret;
}
static EffectType getPluginIndexEffectType(uint32_t *index)
{
	uint32_t walk = 0, newwalk; /* walk must be initialized here, this cruft bug took me like an hour to find */
	for (EffectType i = 0; i < EFFECT_TYPE_COUNT; i++)
		if (effect_api[i].db_count)
		{
			newwalk = walk + effect_api[i].db_count();
			if (newwalk > *index)
			{
				*index -= walk;
				return i;
			}
			walk = newwalk;
		}
	return EFFECT_TYPE_DUMMY;
}

static char *pluginBrowserGetTitle(void *data)
{
	((struct PluginBrowserData*)data)->i = (uint32_t)-1;
	return strdup("plugins");
}

static bool pluginBrowserGetNext(void *data)
{
	return (bool)(++((struct PluginBrowserData*)data)->i < getPluginDBc());
}

static void pluginBrowserDrawLine(BrowserState *b, int y)
{
	uint32_t i = ((struct PluginBrowserData*)b->data)->i;

	EffectType type = getPluginIndexEffectType(&i);

	if (effect_api[type].db_line)
	{
		EffectBrowserLine line = effect_api[type].db_line(i);
		if (line.name) printCulling(line.name, b->x, y, b->x, b->x + (b->w>>1) - 2);
		if (line.maker) printCulling(line.maker, b->x + (b->w>>1), y, b->x + (b->w>>1), b->x + b->w - 10);
		if (effect_api[type].name) printCulling(effect_api[type].name, b->x + b->w - 8, y, b->x + b->w - 8, b->x + b->w);

		if (line.name) free(line.name);
		if (line.maker) free(line.maker);
	}
}

static void cb_addEffectAfter(Event *e)
{
	cc.cursor = getCursorFromEffect(*w->pluginbrowserchain, (size_t)e->callbackarg);
	cb_addEffect(e);
}
static void pluginBrowserCommit(BrowserState *b)
{
	w->page = w->oldpage;
	uint32_t i = b->cursor;
	EffectType type = getPluginIndexEffectType(&i);
	if (w->pluginbrowserbefore) addEffect(w->pluginbrowserchain, type, i, getEffectFromCursor(*w->pluginbrowserchain, cc.cursor), cb_addEffect);
	else                        addEffect(w->pluginbrowserchain, type, i, MIN(getEffectFromCursor(*w->pluginbrowserchain, cc.cursor) + 1, (*w->pluginbrowserchain)->c), cb_addEffectAfter);
}

static void pluginBrowserMouse(enum Button button, int x, int y)
{
	if (rulerMouse(button, x, y)) return;
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

void initPluginEffectBrowserInput(void)
{
	setTooltipTitle("pluginbrowser");
	setTooltipMouseCallback(pluginBrowserMouse);
	addTooltipBind("cursor up"   , 0, XK_Up       , 0, pluginBrowserUpArrow       , (void*)1);
	addTooltipBind("cursor down" , 0, XK_Down     , 0, pluginBrowserDownArrow     , (void*)1);
	addTooltipBind("cursor home" , 0, XK_Home     , 0, (void(*)(void*))browserHome, pbstate );
	addTooltipBind("cursor end"  , 0, XK_End      , 0, (void(*)(void*))browserEnd , pbstate );
	addTooltipBind("cursor pgup" , 0, XK_Page_Up  , 0, pluginBrowserPgUp          , (void*)1);
	addTooltipBind("cursor pgdn" , 0, XK_Page_Down, 0, pluginBrowserPgDn          , (void*)1);
	addTooltipBind("return"      , 0, XK_Escape   , 0, pluginBrowserEscape        , NULL    );
	addTooltipBind("commit"      , 0, XK_Return   , 0, pluginBrowserCommitBind    , NULL    );
}

BrowserState *initPluginBrowser(void)
{
	BrowserState *ret = calloc(1, sizeof(BrowserState));

	ret->getTitle     = pluginBrowserGetTitle;
	ret->getNext      = pluginBrowserGetNext;
	ret->drawLine     = pluginBrowserDrawLine;
	ret->getLineCount = (uint32_t(*)(void*))getPluginDBc;
	ret->commit       = pluginBrowserCommit;

	ret->data = calloc(1, sizeof(struct PluginBrowserData));

	return ret;
}

void freePluginBrowser(BrowserState *b)
{
	free(b->data);
	free(b);
}
