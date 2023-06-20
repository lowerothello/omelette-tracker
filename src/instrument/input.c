void instrumentEscape(void *arg); /* funny lil prototype, TODO: get rid of :3 */

#include "chord/add.c"


void instrumentEscape(void *arg)
{
	if (input_api.autorepeatoff)
		input_api.autorepeatoff();

	previewNote(NOTE_OFF, INST_VOID, 0);
	cc.mouseadjust = cc.keyadjust = 0;
	w->mode = MODE_NORMAL;
}


static void instrumentSetIndex(short inst)
{
	if (inst >= 0 && inst <= 254)
	{
		w->instrument = inst;
		w->sample = 0;
		freeWaveform();
	}
}

static void instrumentCtrlUpArrow  (void *count) { instrumentSetIndex(w->instrument - (size_t)count * MAX(1, w->count)); }
static void instrumentCtrlDownArrow(void *count) { instrumentSetIndex(w->instrument + (size_t)count * MAX(1, w->count)); }

static void instrumentSampleBackspace(void) { fileBrowserBackspace(fbstate); }
static void instrumentSamplePressPreview(size_t note) { fileBrowserPreview(fbstate, note, 0); }
static void instrumentSampleReleasePreview(size_t note) { fileBrowserPreview(fbstate, note, 1); }

static void instrumentMouse(enum Button button, int x, int y)
{
	if (rulerMouse(button, x, y)) return;

	switch (button)
	{
		case BUTTON2_HOLD: case BUTTON2_HOLD_CTRL:
			break;
		default:
			if ((!instSafe(s->inst, w->instrument))
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
					case WHEEL_UP: case WHEEL_UP_CTRL: instrumentSetIndex(w->instrument - WHEEL_SPEED); break;
					case WHEEL_DOWN: case WHEEL_DOWN_CTRL: instrumentSetIndex(w->instrument + WHEEL_SPEED); break;
					case BUTTON_RELEASE: case BUTTON_RELEASE_CTRL:
						if (w->fyoffset)
							instrumentSetIndex(w->instrument + w->fyoffset);
						w->fyoffset = 0;
						break;
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
	if (input_api.autorepeatoff)
		input_api.autorepeatoff();

	w->mode = MODE_INSERT;
}

static void setInsertOctave(void *octave) { w->octave = (size_t)octave; }

static void emptyInstrumentIndex(void) { instrumentSetIndex(emptyInst(0)); }

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
	addBrowserBinds(fbstate);

	addTooltipBind("revert"        , 0          , XK_BackSpace, TT_DRAW, (void(*)(void*))instrumentSampleBackspace, NULL    );
	addTooltipBind("previous index", ControlMask, XK_Up       , TT_DRAW, instrumentCtrlUpArrow                    , (void*)1);
	addTooltipBind("next index"    , ControlMask, XK_Down     , TT_DRAW, instrumentCtrlDownArrow                  , (void*)1);
	addTooltipBind("return"        , 0          , XK_Escape   , 0      , instrumentEscape                         , NULL    );
	switch (w->mode)
	{
		case MODE_NORMAL:
			addCountBinds(0);
			addRulerBinds();
			addBrowserSearchBinds(fbstate);
			addTooltipBind("record sample"    , 0          , XK_r    , TT_DEAD|TT_DRAW, (void(*)(void*))setChordRecord       , NULL);
			addTooltipBind("empty instrument" , 0          , XK_e    , TT_DRAW        , (void(*)(void*))emptyInstrumentIndex , NULL);
			addTooltipBind("yank instrument"  , ControlMask, XK_y    , TT_DRAW        , (void(*)(void*))yankInstrumentInput  , NULL);
			addTooltipBind("put instrument"   , ControlMask, XK_p    , TT_DRAW        , (void(*)(void*))putInstrumentInput   , NULL);
			addTooltipBind("delete instrument", ControlMask, XK_d    , TT_DRAW        , (void(*)(void*))deleteInstrumentInput, NULL);
			addTooltipBind("enter insert mode", 0          , XK_i    , TT_DRAW        , instrumentEnterInsertMode            , NULL);
			break;
		case MODE_INSERT:
			addDecimalBinds("set octave", 0, setInsertOctave);
			addNotePressBinds("preview note", 0, w->octave, (void(*)(void*))instrumentSamplePressPreview);
			addNoteReleaseBinds("preview release", 0, w->octave, (void(*)(void*))instrumentSampleReleasePreview);
			break;
		default: break;
	}
}
