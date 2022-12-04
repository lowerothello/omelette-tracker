void drawMaster(void)
{
	clearControls(&cc);

	switch (w->page)
	{
		case PAGE_EFFECT_MASTER: drawEffects(s->master, &cc, 1, (ws.ws_col - MIN_EFFECT_WIDTH-2)>>1, MIN_EFFECT_WIDTH-2, TRACK_ROW + 1); break;
		case PAGE_EFFECT_SEND:   drawEffects(s->send,   &cc, 1, (ws.ws_col - MIN_EFFECT_WIDTH-2)>>1, MIN_EFFECT_WIDTH-2, TRACK_ROW + 1); break;
		default: break;
	}

	drawControls(&cc);
}

static void masterUpArrow   (void *count) { decControlCursor(&cc, (size_t)count * MAX(1, w->count)); p->redraw = 1; }
static void masterDownArrow (void *count) { incControlCursor(&cc, (size_t)count * MAX(1, w->count)); p->redraw = 1; }
static void masterLeftArrow (void *count) { incControlFieldpointer(&cc); p->redraw = 1; }
static void masterRightArrow(void *count) { decControlFieldpointer(&cc); p->redraw = 1; }
static void masterHome      (void *count) { setControlCursor(&cc, 0); p->redraw = 1; }
static void masterEnd       (void *count) { setControlCursor(&cc, cc.controlc-1); p->redraw = 1; }

static void masterPgUp(void *count)
{
	switch (w->page)
	{
		case PAGE_EFFECT_MASTER: effectPgUp(s->master, (size_t)count); break;
		case PAGE_EFFECT_SEND:   effectPgUp(s->send,   (size_t)count); break;
		default: break;
	} p->redraw = 1;
}
static void masterPgDn(void *count)
{
	switch (w->page)
	{
		case PAGE_EFFECT_MASTER: effectPgDn(s->master, (size_t)count); break;
		case PAGE_EFFECT_SEND:   effectPgDn(s->send,   (size_t)count); break;
		default: break;
	} p->redraw = 1;
}

static void masterMouse(enum _BUTTON button, int x, int y)
{
	switch (button)
	{
		case WHEEL_UP: case WHEEL_UP_CTRL:     masterPgUp((void*)1); break;
		case WHEEL_DOWN: case WHEEL_DOWN_CTRL: masterPgDn((void*)1); break;
		default: mouseControls(&cc, button, x, y); break;
	}
}

void initMasterInput(TooltipState *tt)
{
	setTooltipTitle(tt, "master");
	setTooltipMouseCallback(tt, masterMouse);
	addTooltipBind(tt, "cursor up"   , 0, XK_Up       , 0, masterUpArrow   , (void*)1);
	addTooltipBind(tt, "cursor down" , 0, XK_Down     , 0, masterDownArrow , (void*)1);
	addTooltipBind(tt, "cursor left" , 0, XK_Left     , 0, masterLeftArrow , NULL    );
	addTooltipBind(tt, "cursor right", 0, XK_Right    , 0, masterRightArrow, NULL    );
	addTooltipBind(tt, "cursor home" , 0, XK_Home     , 0, masterHome      , NULL    );
	addTooltipBind(tt, "cursor end"  , 0, XK_End      , 0, masterEnd       , NULL    );
	addTooltipBind(tt, "cursor pgup" , 0, XK_Page_Up  , 0, masterPgUp      , (void*)1);
	addTooltipBind(tt, "cursor pgdn" , 0, XK_Page_Down, 0, masterPgDn      , (void*)1);
	switch (w->page)
	{
		case PAGE_EFFECT_MASTER: initEffectInput(tt, &s->master); break;
		case PAGE_EFFECT_SEND:   initEffectInput(tt, &s->send);   break;
		default: break;
	}
}
