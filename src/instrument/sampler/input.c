static void samplerInputHome(void) { setControlCursor(0); }
static void samplerInputEnd (void) { setControlCursor(cc.controlc-1); }
static void samplerInputUp(void) { decControlCursor(MAX(1, w->count)); }
static void samplerInputDn(void) { incControlCursor(MAX(1, w->count)); }

static void instrumentSamplePressPreviewActive(size_t note) { previewNote(note, w->instrument, 0); }
static void instrumentSampleReleasePreviewActive(size_t note) { previewNote(note, w->instrument, 1); }


static void samplerInput(Inst *iv)
{
	InstSamplerState *s = iv->state;

	if (s->sample)
	{
		addTooltipBind("cursor up"        , 0          , XK_Up       , 0      , (void(*)(void*))samplerInputUp        , NULL);
		addTooltipBind("cursor down"      , 0          , XK_Down     , 0      , (void(*)(void*))samplerInputDn        , NULL);
		addTooltipBind("cursor left"      , 0          , XK_Left     , 0      , (void(*)(void*))incControlFieldpointer, NULL);
		addTooltipBind("cursor right"     , 0          , XK_Right    , 0      , (void(*)(void*))decControlFieldpointer, NULL);
		addTooltipBind("cursor home"      , 0          , XK_Home     , 0      , (void(*)(void*))samplerInputHome      , NULL);
		addTooltipBind("cursor end"       , 0          , XK_End      , 0      , (void(*)(void*))samplerInputEnd       , NULL);
		addTooltipBind("increment cell"   , ControlMask, XK_A        , TT_DRAW, (void(*)(void*))incControlValue       , NULL);
		addTooltipBind("decrement cell"   , ControlMask, XK_X        , TT_DRAW, (void(*)(void*))decControlValue       , NULL);
		addTooltipBind("toggle"           , 0          , XK_Return   , TT_DRAW, (void(*)(void*))toggleKeyControl      , NULL);
		addTooltipBind("revert to default", 0          , XK_BackSpace, TT_DRAW, (void(*)(void*))revertKeyControl      , NULL);

		addTooltipBind("", 0, XK_Page_Up  , 0, NULL, NULL);
		addTooltipBind("", 0, XK_Page_Down, 0, NULL, NULL);

		switch (w->mode)
		{
			case MODE_NORMAL:
				addTooltipBind("", 0, XK_slash    , 0, NULL, NULL);
				addTooltipBind("", 0, XK_n        , 0, NULL, NULL);
				addTooltipBind("", 0, XK_N        , 0, NULL, NULL);
				break;
			case MODE_INSERT:
				addNotePressBinds("preview note", 0, w->octave, (void(*)(void*))instrumentSamplePressPreviewActive);
				addNoteReleaseBinds("preview release", 0, w->octave, (void(*)(void*))instrumentSampleReleasePreviewActive);
				break;
			default: break;
		}
	}
}

static void samplerMouse(Inst *iv, enum Button button, int x, int y)
{
	InstSamplerState *s = iv->state;

	if (!s->sample) /* file browser handling */
	{
		short cols = getMaxInstUICols(&cyclicInstUI, ws.ws_col - (INSTRUMENT_INDEX_COLS - 1));
		short rows = getInstUIRows(&cyclicInstUI, cols);

		short wh = MAX((ws.ws_row - 1 - (TRACK_ROW+1)) - rows, INSTUI_WAVEFORM_MIN);

		if (y > TRACK_ROW + 1 && y < TRACK_ROW + wh - 1)
			browserMouse(fbstate, button, x, y);
	}
	mouseControls(button, x, y);
}
