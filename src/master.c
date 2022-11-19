#define M_MODE_NORMAL 0
#define M_MODE_EDIT 1

void drawMaster(void)
{
	switch (w->page)
	{
		case PAGE_EFFECT_MASTER: drawEffects(s->master, &cc, 1, (INSTRUMENT_INDEX_COLS+1)>>1, ws.ws_col - (INSTRUMENT_INDEX_COLS+1), TRACK_ROW + 2); break;
		case PAGE_EFFECT_SEND:   drawEffects(s->send,   &cc, 1, (INSTRUMENT_INDEX_COLS+1)>>1, ws.ws_col - (INSTRUMENT_INDEX_COLS+1), TRACK_ROW + 2); break;
	}
}

void masterUpArrow  (int count) { decControlCursor(&cc, count); }
void masterDownArrow(int count) { incControlCursor(&cc, count); }
void masterLeftArrow (void) { incControlFieldpointer(&cc); }
void masterRightArrow(void) { decControlFieldpointer(&cc); }
void masterHome(void) { setControlCursor(&cc, 0);             }
void masterEnd (void) { setControlCursor(&cc, cc.controlc-1); }

void masterPgUp(int count)
{
	switch (w->page)
	{
		case PAGE_EFFECT_MASTER: effectPgUp(s->master, count); break;
		case PAGE_EFFECT_SEND:   effectPgUp(s->send,   count); break;
	}
}
void masterPgDn(int count)
{
	switch (w->page)
	{
		case PAGE_EFFECT_MASTER: effectPgDn(s->master, count); break;
		case PAGE_EFFECT_SEND:   effectPgDn(s->send,   count); break;
	}
}

void masterInput(int input)
{
	int button, x, y;
	switch (input)
	{
		case '\033':
			switch (getchar())
			{
				case 'O':
					switch (getchar())
					{
						case 'P': /* xterm f1 */ showTracker   (); break;
						case 'Q': /* xterm f2 */ showInstrument(); break;
						case 'R': /* xterm f3 */ showMaster    (); break;
					} p->redraw = 1; break;
				case '[': /* CSI */
					switch (getchar())
					{
						case '[':
							switch (getchar())
							{
								case 'A': /* linux f1 */ showTracker   (); break;
								case 'B': /* linux f2 */ showInstrument(); break;
								case 'C': /* linux f3 */ showMaster    (); break;
								case 'E': /* linux f5 */ startPlayback(); break;
							} p->redraw = 1; break;
						case 'A': /* up arrow    */ masterUpArrow  (1); p->redraw = 1; break;
						case 'B': /* down arrow  */ masterDownArrow(1); p->redraw = 1; break;
						case 'D': /* left arrow  */ masterLeftArrow (); p->redraw = 1; break;
						case 'C': /* right arrow */ masterRightArrow(); p->redraw = 1; break;
						case '1': /* mod+arrow / f5 - f8 */
							switch (getchar())
							{
								case '5': /* xterm f5 */ startPlayback(); getchar(); break;
								case '7': /* f6       */ stopPlayback(); getchar(); break;
								case ';': /* mod+arrow */
									getchar();
									getchar();
									break;
								case '~': /* linux home */                 masterHome(); p->redraw = 1;   break;
							} break;
						case 'H':         /* xterm home */                 masterHome(); p->redraw = 1;   break;
						case '4': /* end        */ if (getchar() == '~') { masterEnd (); p->redraw = 1; } break;
						case '5': /* page up / shift+scrollup */
							switch (getchar())
							{
								case '~': /* page up */ masterPgUp(1); p->redraw = 1; break;
								case ';': /* shift+scrollup */
									getchar(); /* 2 */
									getchar(); /* ~ */
									p->redraw = 1; break;
							} break;
						case '6': /* page dn / shift+scrolldn */
							switch (getchar())
							{
								case '~': /* page up */ masterPgDn(1); p->redraw = 1; break;
								case ';': /* shift+scrollup */
									getchar(); /* 2 */
									getchar(); /* ~ */
									p->redraw = 1; break;
							} break;
						case 'M': /* mouse */
							button = getchar();
							x = getchar() - 32;
							y = getchar() - 32;

							switch (button)
							{
								case WHEEL_UP: case WHEEL_UP_CTRL:     masterPgUp(1); break;
								case WHEEL_DOWN: case WHEEL_DOWN_CTRL: masterPgDn(1); break;
								default:
									/* if (button != BUTTON1_HOLD && button != BUTTON1_HOLD_CTRL)
										if (masterMouseHeader(button, x, y, &tx)) break; */
									mouseControls(&cc, button, x, y);
									break;
							} break;
					} break;
			} break;
		default:
			switch (w->page)
			{
				case PAGE_EFFECT_MASTER: if (inputEffect(&s->master, input)) return; break;
				case PAGE_EFFECT_SEND:   if (inputEffect(&s->send,   input)) return; break;
			} break;
	}
	if (w->count) { w->count = 0; p->redraw = 1; }
	if (w->chord) { w->chord = '\0'; clearTooltip(&tt); p->redraw = 1; }
}
