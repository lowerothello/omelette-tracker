static void instrumentEscape(void *arg)
{
	setAutoRepeatOff();
	w->showfilebrowser = 0;
	previewNote(NOTE_OFF, INST_VOID, 0);
	cc.mouseadjust = cc.keyadjust = 0;
	w->mode = MODE_NORMAL;
}


#include "chord/add.c"


static void instrumentUpArrow(size_t count) { if (!instSafe(s->inst, w->instrument) || w->showfilebrowser) browserUpArrow  (fbstate, count); }
static void instrumentDnArrow(size_t count) { if (!instSafe(s->inst, w->instrument) || w->showfilebrowser) browserDownArrow(fbstate, count); }
static void instrumentHome(void) { if (!instSafe(s->inst, w->instrument) || w->showfilebrowser) browserHome(fbstate); }
static void instrumentEnd(void)  { if (!instSafe(s->inst, w->instrument) || w->showfilebrowser) browserEnd (fbstate); }
static void instrumentPgUp(void) { instrumentUpArrow((ws.ws_row>>1)); }
static void instrumentPgDn(void) { instrumentDnArrow((ws.ws_row>>1)); }
static void instrumentSearchStart(void) { if (!instSafe(s->inst, w->instrument) || w->showfilebrowser) browserSearchStart(fbstate); }
static void instrumentSearchNext(void) { if (!instSafe(s->inst, w->instrument) || w->showfilebrowser) browserSearchNext(fbstate, 0); }
static void instrumentSearchPrev(void) { if (!instSafe(s->inst, w->instrument) || w->showfilebrowser) browserSearchPrev(fbstate, 0); }

static void instrumentCtrlUpArrow(void *count)
{
	w->showfilebrowser = 0;
	w->instrument -= (size_t)count * MAX(1, w->count);
	if (w->instrument < 0)
		w->instrument = 0;
}
static void instrumentCtrlDownArrow(void *count)
{
	w->showfilebrowser = 0;
	w->instrument += (size_t)count * MAX(1, w->count);
	if (w->instrument > 254)
		w->instrument = 254;
}

static void instrumentSampleReturn(void)
{
	if (!instSafe(s->inst, w->instrument) || w->showfilebrowser)
		fbstate->commit(fbstate);
}
static void instrumentSampleBackspace(void)
{
	if (!instSafe(s->inst, w->instrument) || w->showfilebrowser)
		fileBrowserBackspace(fbstate);
}
static void instrumentSamplePressPreview(size_t note)
{
	if (!instSafe(s->inst, w->instrument) || w->showfilebrowser)
		fileBrowserPreview(fbstate, note, 0);
	else
		previewNote(note, w->instrument, 0);
}
static void instrumentSampleReleasePreview(size_t note)
{
	if (!instSafe(s->inst, w->instrument) || w->showfilebrowser)
		fileBrowserPreview(fbstate, note, 1);
	else
		previewNote(note, w->instrument, 1);
}

static void instrumentMouse(enum Button button, int x, int y)
{
	if (rulerMouse(button, x, y)) return;

	switch (button)
	{
		case BUTTON2_HOLD: case BUTTON2_HOLD_CTRL:
			break;
		default:
			if ((!instSafe(s->inst, w->instrument) || w->showfilebrowser)
					&& y > TRACK_ROW-2
					&& x >= INSTRUMENT_INDEX_COLS)
				browserMouse(fbstate, button, x, y);
			else if (cc.mouseadjust
					|| (y > TRACK_ROW-2
					&&  x >= INSTRUMENT_INDEX_COLS))
			{
				if (instSafe(s->inst, w->instrument))
				{
					Inst *iv = &s->inst->v[s->inst->i[w->instrument]];
					const InstAPI *api;
					if ((api = instGetAPI(iv->type))) api->mouse(iv, button, x, y);
				}
			} else
			{
				switch (button)
				{
					case WHEEL_UP: case WHEEL_UP_CTRL:
						if (w->instrument > WHEEL_SPEED) w->instrument -= WHEEL_SPEED;
						else                             w->instrument = 0;
						break;
					case WHEEL_DOWN: case WHEEL_DOWN_CTRL:
						if (w->instrument < 254 - WHEEL_SPEED) w->instrument += WHEEL_SPEED;
						else                                   w->instrument = 254;
						break;
					case BUTTON_RELEASE: case BUTTON_RELEASE_CTRL:
						if (w->fyoffset)
						{
							if ((short)w->instrument + w->fyoffset < 0)        w->instrument = 0;
							else if ((short)w->instrument + w->fyoffset > 254) w->instrument = 254;
							else                                               w->instrument += w->fyoffset;
							w->fyoffset = 0;
						} break;
					case BUTTON1_HOLD: case BUTTON1_HOLD_CTRL: break; /* ignore */
					default:
						switch (button)
						{
							case BUTTON2: case BUTTON2_CTRL:
								if (!(w->instrecv != INST_REC_LOCK_OK && w->instreci == w->instrument + y - w->centre))
								{
									yankInst(w->instrument + y - w->centre);
									delInst (w->instrument + y - w->centre);
								}
							case BUTTON1: case BUTTON1_CTRL:
								w->fyoffset = y - w->centre;
								break;
							default: break;
						}
						previewNote(NOTE_OFF, INST_VOID, 0);
						break;
				}
			}
			p->redraw = 1;
			break;
	}
}

static void instrumentEnterInsertMode(void *arg)
{
	setAutoRepeatOff();
	w->mode = MODE_INSERT;
}

static void setInsertOctave(void *octave) { w->octave = (size_t)octave; }

static void emptyInstrumentIndex(void) { w->instrument = emptyInst(0); }

static void yankInstrumentInput(void) { yankInst(w->instrument); }
static void putInstrumentInput(void) { putInst(w->instrument); }

static void deleteInstrumentInput(void)
{
	if (!(w->instrecv != INST_REC_LOCK_OK && w->instreci == w->instrument)
			&& instSafe(s->inst, w->instrument))
	{
		yankInst(w->instrument);
		delInst (w->instrument);
	}
}

void initInstrumentInput(void)
{
	setTooltipTitle("instrument");


	if (instSafe(s->inst, w->instrument))
	{
		Inst *iv = &s->inst->v[s->inst->i[w->instrument]];
		const InstAPI *api;
		if ((api = instGetAPI(iv->type))) api->input(iv);
	}

	setTooltipMouseCallback(instrumentMouse);
	addTooltipBind("cursor up"     , 0          , XK_Up       , 0      , (void(*)(void*))instrumentUpArrow        , (void*)1);
	addTooltipBind("cursor down"   , 0          , XK_Down     , 0      , (void(*)(void*))instrumentDnArrow        , (void*)1);
	// addTooltipBind("cursor left"   , 0          , XK_Left     , 0      , (void(*)(void*))instrumentLeftArrow      , NULL    );
	// addTooltipBind("cursor right"  , 0          , XK_Right    , 0      , (void(*)(void*))instrumentRightArrow     , NULL    );
	addTooltipBind("cursor home"   , 0          , XK_Home     , 0      , (void(*)(void*))instrumentHome           , NULL    );
	addTooltipBind("cursor end"    , 0          , XK_End      , 0      , (void(*)(void*))instrumentEnd            , NULL    );
	addTooltipBind("cursor pgup"   , 0          , XK_Page_Up  , 0      , (void(*)(void*))instrumentPgUp           , NULL    );
	addTooltipBind("cursor pgdn"   , 0          , XK_Page_Down, 0      , (void(*)(void*))instrumentPgDn           , NULL    );
	addTooltipBind("previous index", ControlMask, XK_Up       , TT_DRAW, instrumentCtrlUpArrow                    , (void*)1);
	addTooltipBind("next index"    , ControlMask, XK_Down     , TT_DRAW, instrumentCtrlDownArrow                  , (void*)1);
	addTooltipBind("return"        , 0          , XK_Escape   , 0      , instrumentEscape                         , NULL    );
	addTooltipBind("commit"        , 0          , XK_Return   , TT_DRAW, (void(*)(void*))instrumentSampleReturn   , NULL    );
	addTooltipBind("revert"        , 0          , XK_BackSpace, TT_DRAW, (void(*)(void*))instrumentSampleBackspace, NULL    );
	addTooltipBind("search start"  , 0          , XK_slash    , TT_DRAW, (void(*)(void*))instrumentSearchStart    , NULL    );
	addTooltipBind("search next"   , 0          , XK_n        , TT_DRAW, (void(*)(void*))instrumentSearchNext     , NULL    );
	addTooltipBind("search prev"   , 0          , XK_N        , TT_DRAW, (void(*)(void*))instrumentSearchPrev     , NULL    );
	switch (w->mode)
	{
		case MODE_NORMAL:
			addCountBinds(0);
			addRulerBinds();
			addTooltipBind("record"           , 0, XK_r, TT_DEAD|TT_DRAW, (void(*)(void*))setChordRecord       , NULL);
			addTooltipBind("empty index"      , 0, XK_e, TT_DRAW        , (void(*)(void*))emptyInstrumentIndex , NULL);
			addTooltipBind("yank"             , 0, XK_y, TT_DRAW        , (void(*)(void*))yankInstrumentInput  , NULL);
			addTooltipBind("put"              , 0, XK_p, TT_DRAW        , (void(*)(void*))putInstrumentInput   , NULL);
			addTooltipBind("delete"           , 0, XK_d, 0              , (void(*)(void*))deleteInstrumentInput, NULL);
			addTooltipBind("enter insert mode", 0, XK_i, TT_DRAW        , instrumentEnterInsertMode            , NULL);
			break;
		case MODE_INSERT:
			addDecimalBinds("set octave", 0, setInsertOctave);
			addNotePressBinds("preview note", 0, w->octave, (void(*)(void*))instrumentSamplePressPreview);
			addNoteReleaseBinds("preview release", 0, w->octave, (void(*)(void*))instrumentSampleReleasePreview);
			break;
		default: break;
	}
}
