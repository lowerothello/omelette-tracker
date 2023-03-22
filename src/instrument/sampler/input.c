static void samplerInputHome(void) { setControlCursor(0); p->redraw = 1; }
static void samplerInputEnd(void) { setControlCursor(cc.controlc-1); p->redraw = 1; }
static void samplerInputUp(void) { decControlCursor(MAX(1, w->count)); p->redraw = 1; }
static void samplerInputDn(void) { incControlCursor(MAX(1, w->count)); p->redraw = 1; }
static void samplerInputLeft(void)
{
	if (cc.cursor) incControlFieldpointer();
	else if (w->sample) w->sample--;
	p->redraw = 1;
}
static void samplerInputRight(void)
{
	if (cc.cursor) decControlFieldpointer();
	else if (w->sample < SAMPLE_MAX-1) w->sample++;
	p->redraw = 1;
}

static void samplerInputToggle(void) { toggleKeyControl(); p->redraw = 1; }
static void samplerInputRevert(void) { revertKeyControl(); p->redraw = 1; }

static void samplerInputDelete(void)
{
	if (!(w->instrecv != INST_REC_LOCK_OK && w->instreci == w->instrument)
			&& instSafe(s->inst, w->instrument)
			&& !cc.cursor)
	{
		freeWaveform();
		attachSample(&((InstSamplerState*)s->inst->v[s->inst->i[w->instrument]].state)->sample, NULL, w->sample);
		p->redraw = 1;
	}
}

void samplerInput(Inst *iv)
{
	addTooltipBind("cursor up"        , 0          , XK_Up       , 0      , (void(*)(void*))samplerInputUp       , (void*)1);
	addTooltipBind("cursor down"      , 0          , XK_Down     , 0      , (void(*)(void*))samplerInputDn       , (void*)1);
	addTooltipBind("cursor left"      , 0          , XK_Left     , 0      , (void(*)(void*))samplerInputLeft     , NULL    );
	addTooltipBind("cursor right"     , 0          , XK_Right    , 0      , (void(*)(void*))samplerInputRight    , NULL    );
	addTooltipBind("cursor home"      , 0          , XK_Home     , 0      , (void(*)(void*))samplerInputHome     , NULL    );
	addTooltipBind("cursor end"       , 0          , XK_End      , 0      , (void(*)(void*))samplerInputEnd      , NULL    );
	addTooltipBind("increment cell"   , ControlMask, XK_A        , TT_DRAW, (void(*)(void*))incControlValueRedraw, NULL    );
	addTooltipBind("decrement cell"   , ControlMask, XK_X        , TT_DRAW, (void(*)(void*))decControlValueRedraw, NULL    );
	addTooltipBind("toggle"           , 0          , XK_Return   , TT_DRAW, (void(*)(void*))samplerInputToggle   , NULL    );
	addTooltipBind("revert to default", 0          , XK_BackSpace, TT_DRAW, (void(*)(void*))samplerInputRevert   , NULL    );
	switch (w->mode)
	{
		case MODE_NORMAL:
			addTooltipBind("delete", 0, XK_d, 0, (void(*)(void*))samplerInputDelete, NULL);
			break;
		default: break;
	}
}

void samplerMouse(Inst *iv, enum Button button, int x, int y)
{
	mouseControls(button, x, y);
}
