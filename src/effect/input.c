void effectUpArrow    (int count) { decControlCursor(&cc, count); }
void effectDownArrow  (int count) { incControlCursor(&cc, count); }
void effectLeftArrow (void) { incControlFieldpointer(&cc); }
void effectRightArrow(void) { decControlFieldpointer(&cc); }
void effectPgUp(EffectChain *chain, int count) { cc.cursor = getCursorFromEffect(chain, MAX(0, getEffectFromCursor(chain, cc.cursor) - count));          }
void effectPgDn(EffectChain *chain, int count) { cc.cursor = getCursorFromEffect(chain, MIN(chain->c-1, getEffectFromCursor(chain, cc.cursor) + count)); }
void effectHome(void) { setControlCursor(&cc, 0);             }
void effectEnd (void) { setControlCursor(&cc, cc.controlc-1); }

void addEffectAfter(EffectChain **chain)
{
	w->pluginbrowserbefore = 0;
	w->oldpage = w->page;
	w->page = PAGE_PLUGINBROWSER;
	w->pluginbrowserchain = chain;
}
void addEffectBefore(EffectChain **chain)
{
	w->pluginbrowserbefore = 1;
	w->oldpage = w->page;
	w->page = PAGE_PLUGINBROWSER;
	w->pluginbrowserchain = chain;
}
void delChainEffect(EffectChain **chain)
{
	if (!(*chain)->c) return;
	uint8_t selectedindex = getEffectFromCursor(*chain, cc.cursor);
	cc.cursor = MAX(0, cc.cursor - (short)getEffectControlCount(&(*chain)->v[selectedindex]));
	delEffect(chain, selectedindex);
}

int inputEffect(EffectChain **chain, int input)
{
	switch (input)
	{
		case '\n': case '\r': toggleKeyControl(&cc); p->redraw = 1; break;
		case '\b': case 127:  revertKeyControl(&cc); p->redraw = 1; break;
		case 'a': addEffectAfter (chain); p->redraw = 1; break;
		case 'A': addEffectBefore(chain); p->redraw = 1; break;
		case 'd': delChainEffect (chain); p->redraw = 1; break;
		case 1:  /* ^a */ incControlValue(&cc); p->redraw = 1; break;
		case 24: /* ^x */ decControlValue(&cc); p->redraw = 1; break;
	} return 0;
}
