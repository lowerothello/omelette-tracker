void effectUpArrow    (int count) { decControlCursor(&cc, count); }
void effectDownArrow  (int count) { incControlCursor(&cc, count); }
void effectCtrlUpArrow  (EffectChain *chain, int count) { cc.cursor = getCursorFromEffect(chain, MAX(0, getEffectFromCursor(chain, cc.cursor) - count));          }
void effectCtrlDownArrow(EffectChain *chain, int count) { cc.cursor = getCursorFromEffect(chain, MIN(chain->c-1, getEffectFromCursor(chain, cc.cursor) + count)); }
void effectLeftArrow (void) { incControlFieldpointer(&cc); }
void effectRightArrow(void) { decControlFieldpointer(&cc); }
void effectHome(void) { setControlCursor(&cc, 0);             }
void effectEnd (void) { setControlCursor(&cc, cc.controlc-1); }

void effectAddTypeAfter(EffectChain *chain, uint8_t type)
{
	uint8_t newindex = MIN(getEffectFromCursor(chain, cc.cursor) + 1, chain->c);
	addEffect(chain, type, newindex);
	cc.cursor = getCursorFromEffect(chain, newindex);
}
void effectAddTypeBefore(EffectChain *chain, uint8_t type)
{
	addEffect(chain, type, getEffectFromCursor(chain, cc.cursor));
}

void effectAddDistortionAfter (void *chain) { effectAddTypeAfter ((EffectChain *)chain, 1); }
void effectAddDistortionBefore(void *chain) { effectAddTypeBefore((EffectChain *)chain, 1); }
void effectAddEqualizerAfter (void *chain) { effectAddTypeAfter ((EffectChain *)chain, 2); }
void effectAddEqualizerBefore(void *chain) { effectAddTypeBefore((EffectChain *)chain, 2); }

void effectAddExternalAfter(void *chain)
{
	w->pluginplacebefore = 0;
	switch (w->page)
	{
		case PAGE_CHANNEL_EFFECT: w->page = PAGE_CHANNEL_EFFECT_PLUGINBROWSER; break;
		case PAGE_INSTRUMENT_EFFECT: w->page = PAGE_INSTRUMENT_EFFECT_PLUGINBROWSER; break;
	} p->dirty = 1;
}
void effectAddExternalBefore(void *chain)
{
	w->pluginplacebefore = 1;
	switch (w->page)
	{
		case PAGE_CHANNEL_EFFECT: w->page = PAGE_CHANNEL_EFFECT_PLUGINBROWSER; break;
		case PAGE_INSTRUMENT_EFFECT: w->page = PAGE_INSTRUMENT_EFFECT_PLUGINBROWSER; break;
	} p->dirty = 1;
}

void setChordAddEffect(EffectChain *chain)
{
	clearTooltip(&tt);
	setTooltipTitle(&tt, "add effect");
	addTooltipBind(&tt,  "distortion ", 'w', effectAddDistortionAfter, chain);
	addTooltipBind(&tt,  "equalizer  ", 'e', effectAddEqualizerAfter,  chain);
	addTooltipBind(&tt,  "ext. plugin", 'p', effectAddExternalAfter,   chain);
	w->chord = 'a';
}
void setChordAddEffectBefore(EffectChain *chain)
{
	clearTooltip(&tt);
	setTooltipTitle(&tt, "add before");
	addTooltipBind(&tt,  "distortion ", 'w', effectAddDistortionBefore, chain);
	addTooltipBind(&tt,  "equalizer  ", 'e', effectAddEqualizerBefore,  chain);
	addTooltipBind(&tt,  "ext. plugin", 'p', effectAddExternalBefore,   chain);
	w->chord = 'A';
}

int inputEffect(EffectChain *chain, int input)
{
	uint8_t selectedindex;

	if (w->chord)
	{
		inputTooltip(&tt, input); p->dirty = 1;
	} else
		switch (input)
		{
			case '\n': case '\r': toggleKeyControl(&cc); p->dirty = 1; break;
			case '\b': case 127:  revertKeyControl(&cc); p->dirty = 1; break;
			case 'a': setChordAddEffect(chain);       p->dirty = 1; return 1; break;
			case 'A': setChordAddEffectBefore(chain); p->dirty = 1; return 1; break;
			case 'd':
				selectedindex = getEffectFromCursor(chain, cc.cursor);
				cc.cursor = MAX(0, cc.cursor - (short)getEffectControlCount(&chain->v[selectedindex]));
				delEffect(chain, selectedindex);
				p->dirty = 1; break;
			case 1:  /* ^a */ incControlValue(&cc); p->dirty = 1; break;
			case 24: /* ^x */ decControlValue(&cc); p->dirty = 1; break;
		}
	return 0;
}
