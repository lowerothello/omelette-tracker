void effectUpArrow    (int count) { decControlCursor(&cc, count); }
void effectDownArrow  (int count) { incControlCursor(&cc, count); }
void effectLeftArrow (void) { incControlFieldpointer(&cc); }
void effectRightArrow(void) { decControlFieldpointer(&cc); }
void effectPgUp(EffectChain *chain, int count)
{
	count *= MAX(1, w->count);
	cc.cursor = getCursorFromEffect(chain, MAX(0, getEffectFromCursor(chain, cc.cursor) - count));
	p->redraw = 1;
}
void effectPgDn(EffectChain *chain, int count)
{
	count *= MAX(1, w->count);
	cc.cursor = getCursorFromEffect(chain, MIN(chain->c-1, getEffectFromCursor(chain, cc.cursor) + count));
	p->redraw = 1;
}
void effectHome(void) { setControlCursor(&cc, 0);             }
void effectEnd (void) { setControlCursor(&cc, cc.controlc-1); }

void addEffectAfter(EffectChain **chain)
{
	w->pluginbrowserbefore = 0;
	w->oldpage = w->page;
	w->page = PAGE_PLUGINBROWSER;
	w->pluginbrowserchain = chain;
	p->redraw = 1;
}
void addEffectBefore(EffectChain **chain)
{
	w->pluginbrowserbefore = 1;
	w->oldpage = w->page;
	w->page = PAGE_PLUGINBROWSER;
	w->pluginbrowserchain = chain;
	p->redraw = 1;
}
void delChainEffect(EffectChain **chain)
{
	if (!(*chain)->c) return;
	uint8_t selectedindex = getEffectFromCursor(*chain, cc.cursor);
	cc.cursor = MAX(0, cc.cursor - (short)getEffectControlCount(&(*chain)->v[selectedindex]));
	delEffect(chain, selectedindex);
	p->redraw = 1;
}

int inputEffect(EffectChain **chain, int input)
{
	switch (input)
	{
		case '\n': case '\r': toggleKeyControl(&cc); break;
		case '\b': case 127:  revertKeyControl(&cc); break;
		case 'a': addEffectAfter (chain); break;
		case 'A': addEffectBefore(chain); break;
		case 'd': delChainEffect (chain); break;
		case 1:  /* ^a */ incControlValue(&cc); break;
		case 24: /* ^x */ decControlValue(&cc); break;
	} return 0;
}

static void toggleKeyControlRedraw(ControlState *cc) { toggleKeyControl(cc); p->redraw = 1; }
static void revertKeyControlRedraw(ControlState *cc) { revertKeyControl(cc); p->redraw = 1; }
static void incControlValueRedraw (ControlState *cc) { incControlValue (cc); p->redraw = 1; }
static void decControlValueRedraw (ControlState *cc) { decControlValue (cc); p->redraw = 1; }
void initEffectInput(TooltipState *tt, EffectChain **chain)
{
	addTooltipBind(tt, "toggle checkmark button" , 0          , XK_Return   , 0, (void(*)(void*))toggleKeyControlRedraw, &cc  ); /* TODO: are these casts safe? */
	addTooltipBind(tt, "reset control to default", 0          , XK_BackSpace, 0, (void(*)(void*))revertKeyControlRedraw, &cc  );
	addTooltipBind(tt, "add effect after cursor" , 0          , XK_a        , 0, (void(*)(void*))addEffectAfter        , chain);
	addTooltipBind(tt, "add effect before cursor", 0          , XK_A        , 0, (void(*)(void*))addEffectBefore       , chain);
	addTooltipBind(tt, "delete effect"           , 0          , XK_d        , 0, (void(*)(void*))delChainEffect        , chain);
	addTooltipBind(tt, "increment"               , ControlMask, XK_a        , 0, (void(*)(void*))incControlValueRedraw , &cc  );
	addTooltipBind(tt, "decrement"               , ControlMask, XK_x        , 0, (void(*)(void*))decControlValueRedraw , &cc  );
}
