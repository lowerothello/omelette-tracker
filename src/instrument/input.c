#include "chord/add.c"
#include "chord/yank.c"
#include "chord/delete.c"

static void instrumentUpArrow(size_t count)
{
	if (!instrumentSafe(s->instrument, w->instrument)) return;
	if (w->showfilebrowser)
		browserUpArrow(fbstate, count);
	else
		decControlCursor(&cc, count*MAX(1, w->count));
	p->redraw = 1;
}
static void instrumentDownArrow(size_t count)
{
	if (!instrumentSafe(s->instrument, w->instrument)) return;
	if (w->showfilebrowser)
		browserDownArrow(fbstate, count);
	else
		incControlCursor(&cc, count*MAX(1, w->count));
	p->redraw = 1;
}
static void instrumentLeftArrow(void)
{
	if (!instrumentSafe(s->instrument, w->instrument)) return;
	if (!w->showfilebrowser)
		incControlFieldpointer(&cc);
	p->redraw = 1;
}
static void instrumentRightArrow(void)
{
	if (!instrumentSafe(s->instrument, w->instrument)) return;
	if (!w->showfilebrowser)
		decControlFieldpointer(&cc);
	p->redraw = 1;
}
static void instrumentHome(void)
{
	if (!instrumentSafe(s->instrument, w->instrument)) return;
	if (w->showfilebrowser)
		browserHome(fbstate);
	else
		setControlCursor(&cc, 0);
	p->redraw = 1;
}
static void instrumentEnd(void)
{
	if (!instrumentSafe(s->instrument, w->instrument)) return;
	if (w->showfilebrowser)
		browserEnd(fbstate);
	else
		setControlCursor(&cc, cc.controlc-1);
	p->redraw = 1;
}

static void instrumentEscape(void *arg)
{
	previewNote(' ', INST_VOID);
	cc.mouseadjust = cc.keyadjust = 0;
	w->mode = MODE_NORMAL;
	p->redraw = 1;
}

static void instrumentPgUp(void *count) { instrumentUpArrow  ((ws.ws_row>>1) * (size_t)count); p->redraw = 1; }
static void instrumentPgDn(void *count) { instrumentDownArrow((ws.ws_row>>1) * (size_t)count); p->redraw = 1; }

static void instrumentCtrlUpArrow(void *count)
{
	w->instrument -= (size_t)count * MAX(1, w->count);
	if (w->instrument < 0) w->instrument = 0;
	resetWaveform();
	p->redraw = 1;
}
static void instrumentCtrlDownArrow(void *count)
{
	w->instrument += (size_t)count * MAX(1, w->count);
	if (w->instrument > 254) w->instrument = 254;
	resetWaveform();
	p->redraw = 1;
}

static void instrumentSampleReturn(void *arg)
{
	if (!instrumentSafe(s->instrument, w->instrument)) return;
	if (w->showfilebrowser)
		fbstate->commit(fbstate);
	else
		toggleKeyControl(&cc);
	p->redraw = 1;
}
static void instrumentSampleBackspace(void *arg)
{
	if (!instrumentSafe(s->instrument, w->instrument)) return;
	if (w->showfilebrowser)
		fileBrowserBackspace(fbstate);
	else
		revertKeyControl(&cc);
	p->redraw = 1;
}
static void instrumentSamplePreview(void *input)
{
	if (!instrumentSafe(s->instrument, w->instrument)) return;
	if (w->showfilebrowser)
		fileBrowserPreview(fbstate, (int)(size_t)input);
	else
		previewNote((size_t)input, w->instrument);
	p->redraw = 1;
}

static void instrumentMouse(enum Button button, int x, int y)
{
	switch (button)
	{
		case BUTTON2_HOLD: case BUTTON2_HOLD_CTRL:
		case BUTTON3_HOLD: case BUTTON3_HOLD_CTRL:
			break;
		default:
			if (instrumentSafe(s->instrument, w->instrument) && w->showfilebrowser
					&& y > TRACK_ROW-2 && x >= INSTRUMENT_INDEX_COLS)
				browserMouse(fbstate, button, x, y);
			else if (cc.mouseadjust || (instrumentSafe(s->instrument, w->instrument)
						&& y > TRACK_ROW-2 && x >= INSTRUMENT_INDEX_COLS))
			{
				switch (button)
				{
					case WHEEL_UP: case WHEEL_UP_CTRL:     instrumentPgUp((void*)1); break;
					case WHEEL_DOWN: case WHEEL_DOWN_CTRL: instrumentPgDn((void*)1); break;
					default: mouseControls(&cc, button, x, y); break;
				}
			} else
			{
				switch (button)
				{
					case WHEEL_UP: case WHEEL_UP_CTRL:
						if (w->instrument > WHEEL_SPEED) w->instrument -= WHEEL_SPEED;
						else                             w->instrument = 0;
						resetWaveform();
						break;
					case WHEEL_DOWN: case WHEEL_DOWN_CTRL:
						if (w->instrument < 254 - WHEEL_SPEED) w->instrument += WHEEL_SPEED;
						else                                   w->instrument = 254;
						resetWaveform();
						break;
					case BUTTON_RELEASE: case BUTTON_RELEASE_CTRL:
						if (w->fyoffset)
						{
							if ((short)w->instrument + w->fyoffset < 0)        w->instrument = 0;
							else if ((short)w->instrument + w->fyoffset > 254) w->instrument = 254;
							else                                               w->instrument += w->fyoffset;
							resetWaveform();
							w->fyoffset = 0;
						} break;
					case BUTTON1_HOLD: case BUTTON1_HOLD_CTRL: break; /* ignore */
					default:
						switch (button)
						{
							case BUTTON2: case BUTTON2_CTRL:
								if (!(w->instrumentrecv != INST_REC_LOCK_OK && w->instrumentreci == w->instrument + y - w->centre))
								{
									yankInstrument(w->instrument + y - w->centre);
									delInstrument (w->instrument + y - w->centre);
									resetWaveform();
								}
							case BUTTON1: case BUTTON1_CTRL:
								w->fyoffset = y - w->centre;
								break;
							default: break;
						}
						previewNote(' ', INST_VOID);
						break;
				}
			} p->redraw = 1; break;
	}
}

static void toggleBrowser(void *arg)
{
	w->showfilebrowser = !w->showfilebrowser;
	p->redraw = 1;
}
static void instrumentEnterInsertMode(void *arg)
{
	w->mode = MODE_INSERT;
	p->redraw = 1;
}

static void setInsertOctave(void *octave)
{
	w->octave = (size_t)octave;
	p->redraw = 1;
}

void initInstrumentInput(TooltipState *tt)
{
	setTooltipTitle(tt, "instrument");
	setTooltipMouseCallback(tt, instrumentMouse);
	addTooltipBind(tt, "cursor up"        , 0          , XK_Up       , 0      , (void(*)(void*))instrumentUpArrow    , (void*)1);
	addTooltipBind(tt, "cursor down"      , 0          , XK_Down     , 0      , (void(*)(void*))instrumentDownArrow  , (void*)1);
	addTooltipBind(tt, "cursor left"      , 0          , XK_Left     , 0      , (void(*)(void*))instrumentLeftArrow  , NULL    );
	addTooltipBind(tt, "cursor right"     , 0          , XK_Right    , 0      , (void(*)(void*))instrumentRightArrow , NULL    );
	addTooltipBind(tt, "cursor home"      , 0          , XK_Home     , 0      , (void(*)(void*))instrumentHome       , NULL    );
	addTooltipBind(tt, "cursor end"       , 0          , XK_End      , 0      , (void(*)(void*))instrumentEnd        , NULL    );
	addTooltipBind(tt, "cursor pgup"      , 0          , XK_Page_Up  , 0      , instrumentPgUp                       , (void*)1);
	addTooltipBind(tt, "cursor pgdn"      , 0          , XK_Page_Down, 0      , instrumentPgDn                       , (void*)1);
	addTooltipBind(tt, "previous index"   , ControlMask, XK_Up       , 0      , instrumentCtrlUpArrow                , (void*)1);
	addTooltipBind(tt, "next index"       , ControlMask, XK_Down     , 0      , instrumentCtrlDownArrow              , (void*)1);
	addTooltipBind(tt, "return"           , 0          , XK_Escape   , 0      , instrumentEscape                     , NULL    );
	addTooltipBind(tt, "increment cell"   , ControlMask, XK_A        , TT_DRAW, (void(*)(void*))incControlValueRedraw, &cc     );
	addTooltipBind(tt, "decrement cell"   , ControlMask, XK_X        , TT_DRAW, (void(*)(void*))decControlValueRedraw, &cc     );
	addTooltipBind(tt, "toggle"           , 0          , XK_Return   , TT_DRAW, instrumentSampleReturn               , NULL    );
	addTooltipBind(tt, "revert to default", 0          , XK_BackSpace, TT_DRAW, instrumentSampleBackspace            , NULL    );
	switch (w->mode)
	{
		case MODE_NORMAL:
			addCountBinds(tt, 0);
			addTooltipBind(tt, "set bpm"          , 0, XK_b, 0      , (void(*)(void*))setBpmCount  , NULL                        );
			addTooltipBind(tt, "record"           , 0, XK_r, TT_DEAD, setChordRecord               , tt                          );
			addTooltipBind(tt, "add"              , 0, XK_a, TT_DEAD, setChordAddInst              , tt                          );
			addTooltipBind(tt, "add empty"        , 0, XK_e, TT_DEAD, setChordEmptyInst            , tt                          );
			addTooltipBind(tt, "yank"             , 0, XK_y, TT_DEAD, setChordYankInstrument       , tt                          );
			addTooltipBind(tt, "put"              , 0, XK_p, 0      , (void(*)(void*))putInstrument, (void*)(size_t)w->instrument);
			addTooltipBind(tt, "delete"           , 0, XK_d, TT_DEAD, setChordDeleteInstrument     , tt                          );
			addTooltipBind(tt, "delete (no chord)", 0, XK_x, TT_DEAD, chordDeleteInstrument        , NULL                        );
			addTooltipBind(tt, "toggle browser"   , 0, XK_f, 0      , toggleBrowser                , NULL                        );
			addTooltipBind(tt, "enter insert mode", 0, XK_i, 0      , instrumentEnterInsertMode    , NULL                        );
			break;
		case MODE_INSERT:
			addDecimalBinds(tt, "set octave"  , 0, setInsertOctave        );
			addNoteBinds   (tt, "preview note", 0, instrumentSamplePreview);
			break;
		default: break;
	}
}
