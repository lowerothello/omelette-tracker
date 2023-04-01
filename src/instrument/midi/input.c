static void midiInputHome(void) { setControlCursor(0); }
static void midiInputEnd(void) { setControlCursor(cc.controlc-1); }
static void midiInputUp(void) { decControlCursor(MAX(1, w->count)); }
static void midiInputDn(void) { incControlCursor(MAX(1, w->count)); }

void midiInput(Inst *iv)
{
	addTooltipBind("cursor up"        , 0          , XK_Up       , 0      , (void(*)(void*))midiInputUp           , (void*)1);
	addTooltipBind("cursor down"      , 0          , XK_Down     , 0      , (void(*)(void*))midiInputDn           , (void*)1);
	addTooltipBind("cursor left"      , 0          , XK_Left     , 0      , (void(*)(void*))incControlFieldpointer, NULL    );
	addTooltipBind("cursor right"     , 0          , XK_Right    , 0      , (void(*)(void*))decControlFieldpointer, NULL    );
	addTooltipBind("cursor home"      , 0          , XK_Home     , 0      , (void(*)(void*))midiInputHome         , NULL    );
	addTooltipBind("cursor end"       , 0          , XK_End      , 0      , (void(*)(void*))midiInputEnd          , NULL    );
	addTooltipBind("increment cell"   , ControlMask, XK_A        , TT_DRAW, (void(*)(void*))incControlValue       , NULL    );
	addTooltipBind("decrement cell"   , ControlMask, XK_X        , TT_DRAW, (void(*)(void*))decControlValue       , NULL    );
	addTooltipBind("toggle"           , 0          , XK_Return   , TT_DRAW, (void(*)(void*))toggleKeyControl      , NULL    );
	addTooltipBind("revert to default", 0          , XK_BackSpace, TT_DRAW, (void(*)(void*))revertKeyControl      , NULL    );
}

void midiMouse(Inst *iv, enum Button button, int x, int y)
{
	mouseControls(button, x, y);
}

