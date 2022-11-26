static void setInsertMacro(void *arg)
{
	changeMacro((size_t)arg, &w->keyboardmacro, &w->keyboardmacroalt, 0);
	w->mode = T_MODE_INSERT;
	p->redraw = 1;
}
static void setInsertMacroAlt(void *arg)
{
	changeMacro((size_t)arg, &w->keyboardmacro, &w->keyboardmacroalt, 1);
	w->mode = T_MODE_INSERT;
	p->redraw = 1;
}
void setChordMacroInsert(void *tt)
{
	clearTooltip(tt);
	setTooltipTitle(tt, "macro insert");
	addMacroBinds(tt, "set"    , 0       , setInsertMacro   );
	addMacroBinds(tt, "set alt", Mod1Mask, setInsertMacroAlt);
	addTooltipBind(tt, "return", 0, XK_Escape, 0, NULL, NULL);
	w->chord = 'I'; p->redraw = 1;
}
