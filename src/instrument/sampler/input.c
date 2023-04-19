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
	if (!(w->instrecv != INST_REC_LOCK_OK && w->instreci == w->instrument)
			&& instSafe(s->inst, w->instrument))
	{
		freeWaveform();
		attachSample(&((InstSamplerState*)s->inst->v[s->inst->i[w->instrument]].state)->sample, NULL, w->sample);
	}
}

static void samplerInput(Inst *iv)
{
	InstSamplerState *s = iv->state;

	addTooltipBind("sample left"      , ControlMask, XK_Left     , TT_DRAW, (void(*)(void*))samplerSampleLeft     , NULL);
	addTooltipBind("sample right"     , ControlMask, XK_Right    , TT_DRAW, (void(*)(void*))samplerSampleRight    , NULL);
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

	if ((*s->sample)[w->sample] && cc.cursor < sampleInstUI.controls)
		switch (w->mode)
		{
			case MODE_NORMAL:
				addTooltipBind("delete sample", 0, XK_d, TT_DRAW, (void(*)(void*))samplerInputDelete, NULL);
				break;
			default: break;
		}
}

static void samplerMouse(Inst *iv, enum Button button, int x, int y)
{
	mouseControls(button, x, y);
}
