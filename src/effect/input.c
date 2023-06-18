static void effectUpArrow(size_t count)
{
	w->follow = 0;
	size_t min = getCursorFromEffectTrack(w->track);
	int mincount = cc.cursor - min;
	decControlCursor(MIN(mincount, count*MAX(1, w->count)));
	p->redraw = 1;
}
static void effectDownArrow(size_t count)
{
	w->follow = 0;
	size_t max = getCursorFromEffectTrack(w->track + 1) - 1;
	int maxcount = max - cc.cursor;
	incControlCursor(MIN(maxcount, count*MAX(1, w->count)));
	p->redraw = 1;
}

static void effectLeftArrow(size_t count)
{
	count *= MAX(1, w->count);
	for (int i = 0; i < count; i++)
		incControlFieldpointer();
	p->redraw = 1;
}

static void effectRightArrow(size_t count)
{
	count *= MAX(1, w->count);
	for (int i = 0; i < count; i++)
		decControlFieldpointer();
	p->redraw = 1;
}

static void effectHome(void)
{
	w->follow = 0;
	cc.cursor = getCursorFromEffectTrack(w->track);
	p->redraw = 1;
}

static void effectEnd(void)
{
	w->follow = 0;
	cc.cursor = getCursorFromEffectTrack(w->track + 1) - 1;
	p->redraw = 1;
}

static void effectPgUp(void)
{
	EffectChain *ec = s->track->v[w->track]->effect;
	cc.cursor = getCursorFromEffect(w->track, ec, MAX(0, getEffectFromCursor(w->track, ec, cc.cursor) - (int)MAX(1, w->count)));
	p->redraw = 1;
}

static void effectPgDn(void)
{
	EffectChain *ec = s->track->v[w->track]->effect;
	cc.cursor = getCursorFromEffect(w->track, ec, MIN(ec->c-1, getEffectFromCursor(w->track, ec, cc.cursor) + MAX(1, w->count)));
	p->redraw = 1;
}

static void addEffectBelow(void)
{
	w->pluginbrowserbefore = 0;
	w->oldpage = w->page;
	w->page = PAGE_PLUGINBROWSER;
	p->redraw = 1;
}

static void addEffectAbove(void)
{
	w->pluginbrowserbefore = 1;
	w->oldpage = w->page;
	w->page = PAGE_PLUGINBROWSER;
	p->redraw = 1;
}

static void pasteEffectBelow(void)
{
	if (w->effectbuffer.type != EFFECT_TYPE_DUMMY)
		addEffect(&s->track->v[w->track]->effect, EFFECT_TYPE_DUMMY, -1, MIN(getEffectFromCursor(w->track, s->track->v[w->track]->effect, cc.cursor)+1, s->track->v[w->track]->effect->c));
	p->redraw = 1;
}

static void pasteEffectAbove(void)
{
	if (w->effectbuffer.type != EFFECT_TYPE_DUMMY)
		addEffect(&s->track->v[w->track]->effect, EFFECT_TYPE_DUMMY, -1, getEffectFromCursor(w->track, s->track->v[w->track]->effect, cc.cursor));
	p->redraw = 1;
}

static void delChainEffect(void)
{
	EffectChain *ec = s->track->v[w->track]->effect;
	if (!ec->c) return;
	uint8_t selectedindex = getEffectFromCursor(w->track, ec, cc.cursor);
	cc.cursor = MAX(0, cc.cursor - (short)getEffectControlCount(&ec->v[selectedindex]));
	delEffect(&ec, selectedindex);
	p->redraw = 1;
}

static void copyChainEffect(void)
{
	EffectChain *ec = s->track->v[w->track]->effect;
	if (!ec->c) return;
	uint8_t selectedindex = getEffectFromCursor(w->track, ec, cc.cursor);
	copyEffect(&w->effectbuffer, &ec->v[selectedindex], NULL, NULL);
	p->redraw = 1;
}

static void slideEffectUp(void)
{
	EffectChain *ec = s->track->v[w->track]->effect;
	if (!ec->c) return;
	uint8_t selectedindex = getEffectFromCursor(w->track, ec, cc.cursor);
	if (selectedindex)
		swapEffect(&ec, selectedindex, selectedindex - 1);
}

static void slideEffectDown(void)
{
	EffectChain *ec = s->track->v[w->track]->effect;
	if (!ec->c) return;
	uint8_t selectedindex = getEffectFromCursor(w->track, ec, cc.cursor);
	if (selectedindex < ec->c - 1)
		swapEffect(&ec, selectedindex, selectedindex + 1);
}

static void effectMouse(enum Button button, int x, int y)
{
	if (rulerMouse(button, x, y)) return;
	p->redraw = 1;

	int i;

	short tx;

	switch (button)
	{
		case BUTTON2_HOLD: case BUTTON2_HOLD_CTRL:
		case BUTTON3_HOLD: case BUTTON3_HOLD_CTRL:
			break;
		case BUTTON_RELEASE: case BUTTON_RELEASE_CTRL:
			if (w->trackoffset) trackSet(w->track + w->trackoffset);

			if      (w->fyoffset < 0) { w->count = -w->fyoffset; effectUpArrow  (1); }
			else if (w->fyoffset > 0) { w->count =  w->fyoffset; effectDownArrow(1); }

			w->count = 0;
			w->fyoffset = w->shiftoffset = w->trackoffset = w->fieldpointer = 0;

			/* leave mouseadjust mode */
			if (w->mode == MODE_MOUSEADJUST) w->mode = w->oldmode;
			/* falls through intentionally */
		default:
			switch (button)
			{
				case WHEEL_UP: case WHEEL_UP_CTRL:
					w->count = WHEEL_SPEED;
					effectUpArrow(1);
					w->count = 0; break;
				case WHEEL_DOWN: case WHEEL_DOWN_CTRL:
					w->count = WHEEL_SPEED;
					effectDownArrow(1);
					w->count = 0; break;
				default:
					tx = 1 + genConstSfx(EFFECT_WIDTH, ws.ws_col);
					for (i = 0; i < s->track->c; i++)
					{
						tx += EFFECT_WIDTH;
						if (tx > x)
						{
							if (y <= TRACK_ROW)
							{
								switch (button)
								{
									case BUTTON2: case BUTTON2_CTRL: toggleTrackSolo(i); goto inputEffectTrack;
									case BUTTON3: case BUTTON3_CTRL: toggleTrackMute(i); goto inputEffectTrack;
									default: break;
								}
							}
							switch (button)
							{
								case BUTTON1: case BUTTON1_CTRL: w->trackoffset = i - w->track; break;
								default: break;
							}
							goto inputEffectTrack; /* beeeg break */
						}
					} break;
			}
inputEffectTrack:
			mouseControls(button, x, y);
			break;
	}
}


void initEffectInput(void)
{
	setTooltipTitle("mixer");
	setTooltipMouseCallback(effectMouse);
	addDecimalBinds("set step", Mod1Mask, setStep);
	addTooltipBind("return"     , 0          , XK_Escape, 0, (void(*)(void*))trackerEscape, NULL);
	addTooltipBind("track left" , ControlMask, XK_Left  , 0, (void(*)(void*))trackLeft    , NULL);
	addTooltipBind("track right", ControlMask, XK_Right , 0, (void(*)(void*))trackRight   , NULL);

	addCountBinds(0);

	addTooltipBind("cursor up"   , 0, XK_Up       , 0, (void(*)(void*))effectUpArrow   , (void*)1);
	addTooltipBind("cursor down" , 0, XK_Down     , 0, (void(*)(void*))effectDownArrow , (void*)1);
	addTooltipBind("cursor left" , 0, XK_Left     , 0, (void(*)(void*))effectLeftArrow , (void*)1);
	addTooltipBind("cursor right", 0, XK_Right    , 0, (void(*)(void*))effectRightArrow, (void*)1);
	addTooltipBind("cursor home" , 0, XK_Home     , 0, (void(*)(void*))effectHome      , NULL    );
	addTooltipBind("cursor end"  , 0, XK_End      , 0, (void(*)(void*))effectEnd       , NULL    );
	addTooltipBind("cursor pgup" , 0, XK_Page_Up  , 0, (void(*)(void*))effectPgUp      , NULL    );
	addTooltipBind("cursor pgdn" , 0, XK_Page_Down, 0, (void(*)(void*))effectPgDn      , NULL    );

	addTooltipBind("next effect", ControlMask, XK_Down, 0, (void(*)(void*))effectPgDn, NULL);
	addTooltipBind("prev effect", ControlMask, XK_Up  , 0, (void(*)(void*))effectPgUp, NULL);
	addTooltipBind("slide effect up",   ControlMask|ShiftMask, XK_Up  , 0, (void(*)(void*))slideEffectUp  , NULL);
	addTooltipBind("slide effect down", ControlMask|ShiftMask, XK_Down, 0, (void(*)(void*))slideEffectDown, NULL);
	addRulerBinds();
	addTooltipBind("toggle checkmark button" , 0          , XK_Return   , 0              , (void(*)(void*))toggleKeyControl, NULL);
	addTooltipBind("reset control to default", 0          , XK_BackSpace, 0              , (void(*)(void*))revertKeyControl, NULL);
	addTooltipBind("add effect below"        , 0          , XK_a        , 0              , (void(*)(void*))addEffectBelow  , NULL);
	addTooltipBind("add effect above"        , 0          , XK_A        , 0              , (void(*)(void*))addEffectAbove  , NULL);
	addTooltipBind("paste effect below"      , 0          , XK_p        , 0              , (void(*)(void*))pasteEffectBelow, NULL);
	addTooltipBind("paste effect above"      , 0          , XK_P        , 0              , (void(*)(void*))pasteEffectAbove, NULL);
	addTooltipBind("delete effect"           , 0          , XK_d        , 0              , (void(*)(void*))delChainEffect  , NULL);
	addTooltipBind("copy effect"             , 0          , XK_y        , 0              , (void(*)(void*))copyChainEffect , NULL);
	addTooltipBind("increment"               , ControlMask, XK_A        , 0              , (void(*)(void*))incControlValue , NULL);
	addTooltipBind("decrement"               , ControlMask, XK_X        , 0              , (void(*)(void*))decControlValue , NULL);
	addTooltipBind("track"                   , 0          , XK_c        , TT_DEAD|TT_DRAW, (void(*)(void*))setChordTrack   , NULL);
}
