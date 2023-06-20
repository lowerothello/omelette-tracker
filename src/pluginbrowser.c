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

static char *pluginBrowserSearchLine(void *data)
{
	uint32_t i = ((struct PluginBrowserData*)data)->i;

	EffectType type = getPluginIndexEffectType(&i);

	if (!effect_api[type].db_line) return NULL;

	EffectBrowserLine line = effect_api[type].db_line(i);

	size_t retlen = 0;
	if (line.name) retlen += strlen(line.name);
	if (line.maker) retlen += strlen(line.maker);

	char *ret = calloc(sizeof(char), retlen + 2); /* +0x00, +0x20 */

	if (line.name)
	{
		strcat(ret, line.name);
		free(line.name);
	}

	strcat(ret, " ");

	if (line.maker)
	{
		strcat(ret, line.maker);
		free(line.maker);
	}

	return ret;
}

static void pluginBrowserCommit(BrowserState *b)
{
	VALGRIND_PRINTF("pluginBrowserCommit()\n");
	w->page = w->oldpage;
	uint32_t i = b->cursor;
	EffectType type = getPluginIndexEffectType(&i);
	EffectChain **ec = &s->track->v[w->track]->effect;
	if (w->pluginbrowserbefore) addEffect(ec, type, i, getEffectFromCursor(w->track, *ec));
	else                        addEffect(ec, type, i, MIN(getEffectFromCursor(w->track, *ec) + 1, (*ec)->c));
}

static void pluginBrowserMouse(enum Button button, int x, int y)
{
	if (rulerMouse(button, x, y)) return;
	browserMouse(pbstate, button, x, y);
	p->redraw = 1;
}

static void pluginBrowserEscape(void *arg)
{
	w->page = w->oldpage;
	p->redraw = 1;
}

void initPluginEffectBrowserInput(void)
{
	setTooltipTitle("pluginbrowser");
	setTooltipMouseCallback(pluginBrowserMouse);
	addBrowserBinds(pbstate);
	addBrowserSearchBinds(pbstate);
	addTooltipBind("return", 0, XK_Escape   , 0, pluginBrowserEscape, NULL);
}

BrowserState *initPluginBrowser(void)
{
	BrowserState *ret = calloc(1, sizeof(BrowserState));

	ret->getTitle     = pluginBrowserGetTitle;
	ret->getNext      = pluginBrowserGetNext;
	ret->drawLine     = pluginBrowserDrawLine;
	ret->searchLine   = pluginBrowserSearchLine;
	ret->getLineCount = (uint32_t(*)(void*))getPluginDBc;
	ret->commit       = pluginBrowserCommit;

	ret->data = calloc(1, sizeof(struct PluginBrowserData));

	return ret;
}
