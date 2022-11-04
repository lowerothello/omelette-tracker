void effectAddTypeAfter(EffectChain **chain, uint8_t type)
{
	uint8_t newindex = MIN(getEffectFromCursor(*chain, cc.cursor) + 1, (*chain)->c);
	addEffect(chain, type, 0, newindex, cb_addEffect);
	cc.cursor = getCursorFromEffect(*chain, newindex);
}
void effectAddTypeBefore(EffectChain **chain, uint8_t type)
{
	addEffect(chain, type, 0, getEffectFromCursor(*chain, cc.cursor), cb_addEffect);
}

void effectAddDistortionAfter (void *chain) { effectAddTypeAfter ((EffectChain **)chain, 1); }
void effectAddDistortionBefore(void *chain) { effectAddTypeBefore((EffectChain **)chain, 1); }
void effectAddEqualizerAfter (void *chain) { effectAddTypeAfter ((EffectChain **)chain, 2); }
void effectAddEqualizerBefore(void *chain) { effectAddTypeBefore((EffectChain **)chain, 2); }

void setChordAddEffect(EffectChain **chain)
{
	clearTooltip(&tt);
	setTooltipTitle(&tt, "add effect");
	addTooltipBind(&tt,  "distortion ", 'w', effectAddDistortionAfter, chain);
	addTooltipBind(&tt,  "equalizer  ", 'e', effectAddEqualizerAfter,  chain);
	addTooltipBind(&tt,  "ext. plugin", 'p', effectAddExternalAfter,   chain);
	w->chord = 'a';
}
void setChordAddEffectBefore(EffectChain **chain)
{
	clearTooltip(&tt);
	setTooltipTitle(&tt, "add before");
	addTooltipBind(&tt,  "distortion ", 'w', effectAddDistortionBefore, chain);
	addTooltipBind(&tt,  "equalizer  ", 'e', effectAddEqualizerBefore,  chain);
	addTooltipBind(&tt,  "ext. plugin", 'p', effectAddExternalBefore,   chain);
	w->chord = 'A';
}
