#define M_MODE_NORMAL 0
#define M_MODE_EDIT 1

void drawMaster(void)
{
}

void masterUpArrow  (int count) { decControlCursor(&cc, count); }
void masterDownArrow(int count) { incControlCursor(&cc, count); }
void masterLeftArrow (void) { incControlFieldpointer(&cc); }
void masterRightArrow(void) { decControlFieldpointer(&cc); }
void masterHome(void) { setControlCursor(&cc, 0);             }
void masterEnd (void) { setControlCursor(&cc, cc.controlc-1); }

void masterInput(int input)
{
	switch (input)
	{
		case '\033':
			switch (getchar())
			{
				case 'O':
					switch (getchar())
					{
						case 'P': /* xterm f1 */ showTracker(); break;
						case 'Q': /* xterm f2 */ showInstrument(); break;
						case 'S': /* xterm f4 */ showMaster(); break;
					} redraw(); break;
				case '[': /* CSI */
					switch (getchar())
					{
						case '[':
							switch (getchar())
							{
								case 'A': /* linux f1 */ showTracker(); break;
								case 'B': /* linux f2 */ showInstrument(); break;
								case 'D': /* linux f4 */ showMaster(); break;
								case 'E': /* linux f5 */ startPlayback(); break;
							} redraw(); break;
						case 'A': /* up arrow    */ masterUpArrow(1); redraw(); break;
						case 'B': /* down arrow  */ masterDownArrow(1); redraw(); break;
						case 'D': /* left arrow  */ masterLeftArrow(); redraw(); break;
						case 'C': /* right arrow */ masterRightArrow(); redraw(); break;
						case '1': /* mod+arrow / f5 - f8 */
							switch (getchar())
							{
								case '5': /* xterm f5 */ startPlayback(); getchar(); break;
								case '7': /* f6       */ stopPlayback(); getchar(); break;
								case ';': /* mod+arrow */
									switch (getchar())
									{
										case '5': /* ctrl+arrow */
											switch (getchar())
											{
												// case 'D': /* left  */ channelLeft(); redraw(); break;
												// case 'C': /* right */ channelRight(); redraw(); break;
												// case 'A': /* up    */ cycleUp(); redraw(); break;
												// case 'B': /* down  */ cycleDown(); redraw(); break;
											} break;
										default: getchar(); break;
									} break;
								case '~': /* linux home */ masterHome(); redraw(); break;
							} break;
						case 'H': /* xterm home */ masterHome(); redraw(); break;
						case '4': /* end        */ if (getchar() == '~') { masterEnd(); redraw(); } break;
						// case '5': /* page up    */ trackerUpArrow(s->rowhighlight); getchar(); redraw(); break;
						// case '6': /* page down  */ trackerDownArrow(s->rowhighlight); getchar(); redraw(); break;
					} break;
			}
	}
}
