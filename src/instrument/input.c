static void instrumentEscape(void *arg)
{
	setAutoRepeatOff();
	w->showfilebrowser = 0;
	previewNote(NOTE_OFF, INST_VOID, 0);
	cc.mouseadjust = cc.keyadjust = 0;
	w->mode = MODE_NORMAL;
	p->redraw = 1;
}


#include "chord/add.c"


static void instrumentUpArrow(size_t count)
{
	if (!instSafe(s->inst, w->instrument) || w->showfilebrowser)
		browserUpArrow(fbstate, count);
	p->redraw = 1;
}
static void instrumentDnArrow(size_t count)
{
	if (!instSafe(s->inst, w->instrument) || w->showfilebrowser)
		browserDownArrow(fbstate, count);
	p->redraw = 1;
}
static void instrumentHome(void)
{
	if (!instSafe(s->inst, w->instrument) || w->showfilebrowser)
		browserHome(fbstate);
	p->redraw = 1;
}
static void instrumentEnd(void)
{
	if (!instSafe(s->inst, w->instrument) || w->showfilebrowser)
		browserEnd(fbstate);
	p->redraw = 1;
}

static void instrumentPgUp(void *count) { instrumentUpArrow((ws.ws_row>>1) * (size_t)count); p->redraw = 1; }
static void instrumentPgDn(void *count) { instrumentDnArrow((ws.ws_row>>1) * (size_t)count); p->redraw = 1; }

static void instrumentCtrlUpArrow(void *count)
{
	w->showfilebrowser = 0;
	w->instrument -= (size_t)count * MAX(1, w->count);
	if (w->instrument < 0)
		w->instrument = 0;
	p->redraw = 1;
}
static void instrumentCtrlDownArrow(void *count)
{
	w->showfilebrowser = 0;
	w->instrument += (size_t)count * MAX(1, w->count);
	if (w->instrument > 254)
		w->instrument = 254;
	p->redraw = 1;
}

static void instrumentSampleReturn(void)
{
	if (!instSafe(s->inst, w->instrument) || w->showfilebrowser)
		fbstate->commit(fbstate);
	p->redraw = 1;
}
static void instrumentSampleBackspace(void)
{
	if (!instSafe(s->inst, w->instrument) || w->showfilebrowser)
		fileBrowserBackspace(fbstate);
	p->redraw = 1;
}
static void instrumentSamplePressPreview(size_t note)
{
	if (!instSafe(s->inst, w->instrument) || w->showfilebrowser)
		fileBrowserPreview(fbstate, note, 0);
	else
		previewNote(note, w->instrument, 0);
	p->redraw = 1;
}
static void instrumentSampleReleasePreview(size_t note)
{
	if (!instSafe(s->inst, w->instrument) || w->showfilebrowser)
		fileBrowserPreview(fbstate, note, 1);
	else
		previewNote(note, w->instrument, 1);
	p->redraw = 1;
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
			p->redraw = 1; break;
	}
}

static void instrumentEnterInsertMode(void *arg)
{
	setAutoRepeatOff();
	w->mode = MODE_INSERT;
	p->redraw = 1;
}

static void setInsertOctave(void *octave)
{
	w->octave = (size_t)octave;
	p->redraw = 1;
}

static void emptyInstrumentIndex(void)
{
	w->instrument = emptyInst(0);
	p->redraw = 1;
}

static void yankInstrumentInput(void)
{
	yankInst(w->instrument);
	p->redraw = 1;
}
static void putInstrumentInput(void)
{
	putInst(w->instrument);
	p->redraw = 1;
}

static void deleteInstrumentInput(void)
{
	if (!(w->instrecv != INST_REC_LOCK_OK && w->instreci == w->instrument)
			&& instSafe(s->inst, w->instrument))
	{
		yankInst(w->instrument);
		delInst (w->instrument);
		p->redraw = 1;
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
	addTooltipBind("cursor pgup"   , 0          , XK_Page_Up  , 0      , instrumentPgUp                           , (void*)1);
	addTooltipBind("cursor pgdn"   , 0          , XK_Page_Down, 0      , instrumentPgDn                           , (void*)1);
	addTooltipBind("previous index", ControlMask, XK_Up       , TT_DRAW, instrumentCtrlUpArrow                    , (void*)1);
	addTooltipBind("next index"    , ControlMask, XK_Down     , TT_DRAW, instrumentCtrlDownArrow                  , (void*)1);
	addTooltipBind("return"        , 0          , XK_Escape   , 0      , instrumentEscape                         , NULL    );
	addTooltipBind("commit"        , 0          , XK_Return   , TT_DRAW, (void(*)(void*))instrumentSampleReturn   , NULL    );
	addTooltipBind("revert"        , 0          , XK_BackSpace, TT_DRAW, (void(*)(void*))instrumentSampleBackspace, NULL    );
	switch (w->mode)
	{
		case MODE_NORMAL:
			addCountBinds(0);
			addRulerBinds();
			addTooltipBind("record"           , 0, XK_r, TT_DEAD|TT_DRAW, (void(*)(void*))setChordRecord       , NULL);
			addTooltipBind("empty index"      , 0, XK_e, TT_DRAW        , (void(*)(void*))emptyInstrumentIndex , NULL);
			addTooltipBind("yank"             , 0, XK_y, TT_DEAD|TT_DRAW, (void(*)(void*))yankInstrumentInput  , NULL);
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
