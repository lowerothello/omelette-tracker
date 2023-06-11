static void samplerInputHome(void) { setControlCursor(0); }
static void samplerInputEnd (void) { setControlCursor(cc.controlc-1); }
static void samplerInputUp(void) { decControlCursor(MAX(1, w->count)); }
static void samplerInputDn(void) { incControlCursor(MAX(1, w->count)); }

static void samplerSampleLeft (void)
{
	InstSamplerState *ss = s->inst->v[s->inst->i[w->instrument]].state;

	if (w->sample)
	{
		/* using ints for state so maths with them is signed */
		int prevoffset = 0;
		if ((*ss->sample)[w->sample]) prevoffset = sampleInstUI.controls;

		w->sample--;

		int nextoffset = 0;
		if ((*ss->sample)[w->sample]) nextoffset = sampleInstUI.controls;

		cc.cursor = MAX(0, cc.cursor + nextoffset - prevoffset);
	}
}

static void samplerSampleRight(void)
{
	InstSamplerState *ss = s->inst->v[s->inst->i[w->instrument]].state;

	if (w->sample < SAMPLE_MAX-1)
	{
		/* using ints for state so maths with them is signed */
		int prevoffset = 0;
		if ((*ss->sample)[w->sample]) prevoffset = sampleInstUI.controls;

		w->sample++;

		int nextoffset = 0;
		if ((*ss->sample)[w->sample]) nextoffset = sampleInstUI.controls;

		cc.cursor = MAX(0, cc.cursor + nextoffset - prevoffset);
	}
}

static void samplerInputDelete(void)
{
	InstSamplerState *ss = s->inst->v[s->inst->i[w->instrument]].state;

	if (!(w->instrecv != INST_REC_LOCK_OK && w->instreci == w->instrument)
			&& instSafe(s->inst, w->instrument))
	{
		freeWaveform();
		attachSample(&ss->sample, NULL, w->sample);
	}
}

static void instrumentSamplePressPreviewActive(size_t note) { previewNote(note, w->instrument, 0); }
static void instrumentSampleReleasePreviewActive(size_t note) { previewNote(note, w->instrument, 1); }


static void samplerInput(Inst *iv)
{
	InstSamplerState *s = iv->state;

	addTooltipBind("sample left"      , ControlMask, XK_Left     , TT_DRAW, (void(*)(void*))samplerSampleLeft     , NULL);
	addTooltipBind("sample right"     , ControlMask, XK_Right    , TT_DRAW, (void(*)(void*))samplerSampleRight    , NULL);

	if ((*s->sample)[w->sample])
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
				addTooltipBind("delete sample", 0, XK_d, TT_DRAW, (void(*)(void*))samplerInputDelete, NULL);
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

	switch (button)
	{
		case BUTTON1:
			if (y == TRACK_ROW+2)
			{
				w->fxoffset = x - ((INSTRUMENT_INDEX_COLS-1) + (((ws.ws_col - (INSTRUMENT_INDEX_COLS-1)) - 3)>>1) - 1);
				/* TODO: rewrite this logic without the conditional, maybe abuse casts? */
				if (w->fxoffset < 0)
					w->fxoffset -= 5; /* gotta love SIGNED DIVISION, everyone! */
				w->fxoffset /= 5;
				p->redraw = 1;
				return;
			} break;
		case BUTTON_RELEASE:
			w->sample = MAX(0, MIN(SAMPLE_MAX-1, w->sample + w->fxoffset));
			w->fxoffset = 0;
			p->redraw = 1;
			break;
		case WHEEL_UP:
			if (y == TRACK_ROW+2)
				w->sample = MAX(0, MIN(SAMPLE_MAX-1, (short)w->sample - 1));
			break;
		case WHEEL_DOWN:
			if (y == TRACK_ROW+2)
				w->sample = MAX(0, MIN(SAMPLE_MAX-1, (short)w->sample + 1));
			break;
		default: break;
	}

	if (!(*s->sample)[w->sample])
	{
		short cols = getMaxInstUICols(&cyclicInstUI, ws.ws_col - (INSTRUMENT_INDEX_COLS - 1));
		short rows = getInstUIRows(&cyclicInstUI, cols);

		short wh = MAX((ws.ws_row - 1 - (TRACK_ROW+1)) - rows, INSTUI_WAVEFORM_MIN);

		if (y > TRACK_ROW + 2 && y < TRACK_ROW + wh - 1)
			browserMouse(fbstate, button, x, y);
	}
	mouseControls(button, x, y);
}
