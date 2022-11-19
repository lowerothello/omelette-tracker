#include "sampleinput.c"

void instrumentUpArrow(int count)
{
	switch (w->page)
	{
		case PAGE_INSTRUMENT_SAMPLE: instrumentSampleUpArrow(count); break;
		case PAGE_INSTRUMENT_EFFECT: effectUpArrow(count); break;
	}
}
void instrumentDownArrow(int count)
{
	switch (w->page)
	{
		case PAGE_INSTRUMENT_SAMPLE: instrumentSampleDownArrow(count); break;
		case PAGE_INSTRUMENT_EFFECT: effectDownArrow(count); break;
	}
}
void instrumentPgUp(int count)
{
	switch (w->page)
	{
		case PAGE_INSTRUMENT_SAMPLE: instrumentUpArrow(ws.ws_row>>1); break;
		case PAGE_INSTRUMENT_EFFECT: effectPgUp(s->instrument->v[s->instrument->i[w->instrument]].effect, count); break;
	}
}
void instrumentPgDn(int count)
{
	switch (w->page)
	{
		case PAGE_INSTRUMENT_SAMPLE: instrumentDownArrow(ws.ws_row>>1); break;
		case PAGE_INSTRUMENT_EFFECT: effectPgDn(s->instrument->v[s->instrument->i[w->instrument]].effect, count); break;
	}
}
void instrumentLeftArrow(void)
{
	switch (w->page)
	{
		case PAGE_INSTRUMENT_SAMPLE: instrumentSampleLeftArrow(); break;
		case PAGE_INSTRUMENT_EFFECT: effectLeftArrow(); break;
	}
}
void instrumentRightArrow(void)
{
	switch (w->page)
	{
		case PAGE_INSTRUMENT_SAMPLE: instrumentSampleRightArrow(); break;
		case PAGE_INSTRUMENT_EFFECT: effectRightArrow(); break;
	}
}
void instrumentCtrlUpArrow(int count)
{
	w->instrument -= count;
	if (w->instrument < 0) w->instrument = 0;
	w->effectscroll = 0;
	resetWaveform();
}
void instrumentCtrlDownArrow(int count)
{
	w->instrument += count;
	if (w->instrument > 254) w->instrument = 254;
	w->effectscroll = 0;
	resetWaveform();
}
void instrumentHome(void)
{
	switch (w->page)
	{
		case PAGE_INSTRUMENT_SAMPLE: instrumentSampleHome(); break;
		case PAGE_INSTRUMENT_EFFECT: effectHome(); break;
	}
}
void instrumentEnd(void)
{
	switch (w->page)
	{
		case PAGE_INSTRUMENT_SAMPLE: instrumentSampleEnd(); break;
		case PAGE_INSTRUMENT_EFFECT: effectEnd(); break;
	}
}

void instrumentInput(int input)
{
	int button, x, y;
	if (!instrumentSafe(s, w->instrument)) setControlCursor(&cc, 0);
	switch (input)
	{
		case '\033':
			switch (getchar())
			{
				case 'O':
					previewNote(' ', INST_VOID);
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
							previewNote(' ', INST_VOID);
							switch (getchar())
							{
								case 'A': /* linux f1 */ showTracker   (); break;
								case 'B': /* linux f2 */ showInstrument(); break;
								case 'C': /* linux f2 */ showMaster    (); break;
								case 'E': /* linux f5 */ startPlayback(); break;
							} p->redraw = 1; break;
						case 'A': /* up arrow    */ instrumentUpArrow  (MAX(1, w->count)); p->redraw = 1; break;
						case 'B': /* down arrow  */ instrumentDownArrow(MAX(1, w->count)); p->redraw = 1; break;
						case 'D': /* left arrow  */ instrumentLeftArrow                (); p->redraw = 1; break;
						case 'C': /* right arrow */ instrumentRightArrow               (); p->redraw = 1; break;
						case '1': /* mod+arrow / f5 - f8 */
							switch (getchar())
							{
								case '5': /* xterm f5 */ startPlayback(); getchar(); break;
								case '7': /*       f6 */
									previewNote(' ', INST_VOID);
									stopPlayback(); getchar(); break;
								case ';': /* mod+arrow */
									switch (getchar())
									{
										case '5': /* ctrl+arrow */
											switch (getchar())
											{
												case 'A': /* up    */ instrumentCtrlUpArrow  (MAX(1, w->count)); p->redraw = 1; break;
												case 'B': /* down  */ instrumentCtrlDownArrow(MAX(1, w->count)); p->redraw = 1; break;
											} break;
										default: getchar(); break;
									} break;
								case '~': /* linux home */          instrumentHome(); p->redraw = 1; break;
							} break;
						case 'H': /* xterm home */                  instrumentHome(); p->redraw = 1; break;
						case '4': /* end */ if (getchar() == '~') { instrumentEnd (); p->redraw = 1; } break;
						case '5': /* page up / shift+scrollup */
							switch (getchar())
							{
								case '~': /* page up */ instrumentPgUp(MAX(1, w->count)); p->redraw = 1; break;
								case ';': /* shift+scrollup */
									getchar(); /* 2 */
									getchar(); /* ~ */
									instrumentPgUp(MAX(1, w->count));
									p->redraw = 1; break;
							} break;
						case '6': /* page dn / shift+scrolldn */
							switch (getchar())
							{
								case '~': /* page dn */ instrumentPgDn(MAX(1, w->count)); p->redraw = 1; break;
								case ';': /* shift+scrolldn */
									getchar(); /* 2 */
									getchar(); /* ~ */
									instrumentPgDn(MAX(1, w->count));
									p->redraw = 1; break;
							} break;
						case 'M': /* mouse */
							button = getchar();
							x = getchar()-32;
							y = getchar()-32;
							switch (button)
							{
								case BUTTON2_HOLD: case BUTTON2_HOLD_CTRL:
								case BUTTON3_HOLD: case BUTTON3_HOLD_CTRL:
									break;
								default:
									if (w->page == PAGE_INSTRUMENT_SAMPLE && instrumentSafe(s, w->instrument) && w->showfilebrowser
											&& y > TRACK_ROW-2 && x >= INSTRUMENT_INDEX_COLS)
										browserMouse(fbstate, button, x, y);
									else if (cc.mouseadjust || (instrumentSafe(s, w->instrument)
												&& y > TRACK_ROW-2 && x >= INSTRUMENT_INDEX_COLS))
									{
										switch (button)
										{
											case WHEEL_UP: case WHEEL_UP_CTRL:     instrumentPgUp(1); break;
											case WHEEL_DOWN: case WHEEL_DOWN_CTRL: instrumentPgDn(1); break;
											default: mouseControls(&cc, button, x, y); break;
										}
									} else
									{
										switch (button)
										{
											case WHEEL_UP: case WHEEL_UP_CTRL:
												if (w->instrument > WHEEL_SPEED) w->instrument -= WHEEL_SPEED;
												else                             w->instrument = 0;
												w->effectscroll = 0; resetWaveform();
												break;
											case WHEEL_DOWN: case WHEEL_DOWN_CTRL:
												if (w->instrument < 254 - WHEEL_SPEED) w->instrument += WHEEL_SPEED;
												else                                   w->instrument = 254;
												w->effectscroll = 0; resetWaveform();
												break;
											case BUTTON_RELEASE: case BUTTON_RELEASE_CTRL:
												if (w->fyoffset)
												{
													if ((short)w->instrument + w->fyoffset < 0)        w->instrument = 0;
													else if ((short)w->instrument + w->fyoffset > 254) w->instrument = 254;
													else                                               w->instrument += w->fyoffset;
													w->effectscroll = 0; resetWaveform();
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
															w->effectscroll = 0; resetWaveform();
														}
													case BUTTON1: case BUTTON1_CTRL:
														w->fyoffset = y - w->centre;
														break;
												}
												previewNote(' ', INST_VOID);
												break;
										}
									} p->redraw = 1; break;
									break;
							}
					} break;
				default: /* escape */
					previewNote(' ', INST_VOID);
					cc.mouseadjust = cc.keyadjust = 0;
					w->mode = I_MODE_NORMAL;
					p->redraw = 1; break;
			} break;
		default:
			if (w->chord)
			{
				p->redraw = 1;
				if (inputTooltip(&tt, input)) return;
			} else
				switch (input)
				{
					case '0': w->count *= 10; w->count += 0; p->redraw = 1; return;
					case '1': w->count *= 10; w->count += 1; p->redraw = 1; return;
					case '2': w->count *= 10; w->count += 2; p->redraw = 1; return;
					case '3': w->count *= 10; w->count += 3; p->redraw = 1; return;
					case '4': w->count *= 10; w->count += 4; p->redraw = 1; return;
					case '5': w->count *= 10; w->count += 5; p->redraw = 1; return;
					case '6': w->count *= 10; w->count += 6; p->redraw = 1; return;
					case '7': w->count *= 10; w->count += 7; p->redraw = 1; return;
					case '8': w->count *= 10; w->count += 8; p->redraw = 1; return;
					case '9': w->count *= 10; w->count += 9; p->redraw = 1; return;

					case 'b': /* bpm               */ if (w->count) { s->songbpm = MIN(255, MAX(32, w->count)); reapplyBpm(); } w->count = 0; p->redraw = 1; break;
					case 'r': /* record            */ setChordRecord            (); p->redraw = 1; return;
					case 'a': /* add               */ setChordAddInst           (); p->redraw = 1; return;
					case 'e': /* add empty         */ setChordEmptyInst         (); p->redraw = 1; return;
					case 'y': /* yank              */ setChordYankInstrument    (); p->redraw = 1; return;
					case 'p': /* put               */ putInstrument(w->instrument); p->redraw = 1; break;
					case 'd': /* delete            */ setChordDeleteInstrument  (); p->redraw = 1; return;
					case 'x': /* delete (no chord) */ chordDeleteInstrument (NULL); break;
					case 'f': /* toggle browser    */ w->showfilebrowser = !w->showfilebrowser; p->redraw = 1; break;
					case 'i': w->mode = I_MODE_INSERT; p->redraw = 1; break;
					/* case 'e': // export
						setCommand(&w->command, &sampleExportCallback, NULL, NULL, 0, "File name: ", "");
						w->mode = 255;
						p->redraw = 1; break; */
					default:
						switch (w->page)
						{
							case PAGE_INSTRUMENT_SAMPLE: if (inputInstrumentSample(input)) return; break;
							case PAGE_INSTRUMENT_EFFECT: if (instrumentSafe(s, w->instrument) && inputEffect(&s->instrument->v[s->instrument->i[w->instrument]].effect, input)) return; break; /* shell-like && as a gate */
						} break;
				}
			break;
	}
	if (w->count) { w->count = 0; p->redraw = 1; }
	if (w->chord) { w->chord = '\0'; clearTooltip(&tt); p->redraw = 1; }
}
