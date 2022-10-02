#include "sampleinput.c"

void instrumentUpArrow(int count)
{
	if (w->mode == I_MODE_INDICES)
	{
		w->instrument -= count;
		if (w->instrument < 0) w->instrument = 0;
		w->effectscroll = 0;
		resetWaveform();
	} else
		switch (w->page)
		{
			case PAGE_INSTRUMENT_SAMPLE: instrumentSampleUpArrow(count); break;
			case PAGE_INSTRUMENT_EFFECT: effectUpArrow(count); break;
		}
}
void instrumentDownArrow(int count)
{
	if (w->mode == I_MODE_INDICES)
	{
		w->instrument += count;
		if (w->instrument > 254) w->instrument = 254;
		w->effectscroll = 0;
		resetWaveform();
	} else
		switch (w->page)
		{
			case PAGE_INSTRUMENT_SAMPLE: instrumentSampleDownArrow(count); break;
			case PAGE_INSTRUMENT_EFFECT: effectDownArrow(count); break;
		}
}
void instrumentCtrlUpArrow(int count)
{
	if (w->mode != I_MODE_INDICES)
		switch (w->page)
		{
			case PAGE_INSTRUMENT_SAMPLE: break;
			case PAGE_INSTRUMENT_EFFECT: effectCtrlUpArrow(&s->instrumentv[s->instrumenti[w->instrument]].effect, count); break;
		}
}
void instrumentCtrlDownArrow(int count)
{
	if (w->mode != I_MODE_INDICES)
		switch (w->page)
		{
			case PAGE_INSTRUMENT_SAMPLE: break;
			case PAGE_INSTRUMENT_EFFECT: effectCtrlDownArrow(&s->instrumentv[s->instrumenti[w->instrument]].effect, count); break;
		}
}
void instrumentLeftArrow(void)
{
	if (w->mode != I_MODE_INDICES)
		switch (w->page)
		{
			case PAGE_INSTRUMENT_SAMPLE: instrumentSampleLeftArrow(); break;
			case PAGE_INSTRUMENT_EFFECT: effectLeftArrow(); break;
		}
}
void instrumentRightArrow(void)
{
	if (w->mode != I_MODE_INDICES)
		switch (w->page)
		{
			case PAGE_INSTRUMENT_SAMPLE: instrumentSampleRightArrow(); break;
			case PAGE_INSTRUMENT_EFFECT: effectRightArrow(); break;
		}
}
void instrumentHome(void)
{
	if (w->mode == I_MODE_INDICES)
	{
		w->instrument = 0;
		w->effectscroll = 0;
		resetWaveform();
	} else
		switch (w->page)
		{
			case PAGE_INSTRUMENT_SAMPLE: instrumentSampleHome(); break;
			case PAGE_INSTRUMENT_EFFECT: effectHome(); break;
		}
}
void instrumentEnd(void)
{
	if (w->mode == I_MODE_INDICES)
	{
		w->instrument = 254;
		w->effectscroll = 0;
		resetWaveform();
	} else
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
						case 'P': /* xterm f1 */ showTracker(); break;
						case 'Q': /* xterm f2 */ showInstrument(); break;
						case 'S': /* xterm f4 */ showMaster(); break;
					} p->dirty = 1; break;
				case '[': /* CSI */
					switch (getchar())
					{
						case '[':
							previewNote(' ', INST_VOID);
							switch (getchar())
							{
								case 'A': /* linux f1 */ showTracker(); break;
								case 'B': /* linux f2 */ showInstrument(); break;
								case 'D': /* linux f4 */ showMaster(); break;
								case 'E': /* linux f5 */ startPlayback(); break;
							} p->dirty = 1; break;
						case 'A': /* up arrow    */ instrumentUpArrow  (1); p->dirty = 1; break;
						case 'B': /* down arrow  */ instrumentDownArrow(1); p->dirty = 1; break;
						case 'D': /* left arrow  */ instrumentLeftArrow (); p->dirty = 1; break;
						case 'C': /* right arrow */ instrumentRightArrow(); p->dirty = 1; break;
						case '1': /* mod+arrow / f5 - f8 */
							switch (getchar())
							{
								case '5': /* xterm f5, play  */ startPlayback(); getchar(); break;
								case '7': /* f6, stop        */
									previewNote(' ', INST_VOID);
									stopPlayback(); getchar(); break;
								case ';': /* mod+arrow */
									switch (getchar())
									{
										case '5': /* ctrl+arrow */
											switch (getchar())
											{
												case 'A': /* up   */ instrumentCtrlUpArrow  (1); p->dirty = 1; break;
												case 'B': /* down */ instrumentCtrlDownArrow(1); p->dirty = 1; break;
											} break;
										default: getchar(); break;
									} break;
								case '~': /* linux home */          instrumentHome(); p->dirty = 1;   break;
							} break;
						case 'H': /* xterm home */                  instrumentHome(); p->dirty = 1;   break;
						case '4': /* end */ if (getchar() == '~') { instrumentEnd (); p->dirty = 1; } break;
						case '5':
							switch (getchar())
							{
								case '~': /* page up        */ instrumentUpArrow(ws.ws_row>>1); p->dirty = 1; break;
								case ';': /* shift+scrollup */ getchar(); break;
							} break;
						case '6':
							switch (getchar())
							{
								case '~': /* page down      */ instrumentDownArrow(ws.ws_row>>1); p->dirty = 1; break;
								case ';': /* shift+scrolldn */ getchar(); break;
							} break;
						case 'M': /* mouse */
							button = getchar();
							x = getchar() - 32;
							y = getchar() - 32;
							switch (button)
							{
								case BUTTON2_HOLD: case BUTTON2_HOLD_CTRL:
								case BUTTON3_HOLD: case BUTTON3_HOLD_CTRL:
									break;
								default:
									if (button != BUTTON1_HOLD && button != BUTTON1_HOLD_CTRL) instrumentModeToNormal();
									if (cc.mouseadjust || (instrumentSafe(s, w->instrument)
												&& y > CHANNEL_ROW-2 && x >= INSTRUMENT_INDEX_COLS))
									{
										if (button != BUTTON1_HOLD && button != BUTTON1_HOLD_CTRL) instrumentModeToNormal();
										switch (button)
										{
											case WHEEL_UP:   case WHEEL_UP_CTRL:   instrumentCtrlUpArrow  (1); break;
											case WHEEL_DOWN: case WHEEL_DOWN_CTRL: instrumentCtrlDownArrow(1); break;
											default: instrumentModeToNormal(); mouseControls(&cc, button, x, y); break;
										}
									} else
									{
										if (button != BUTTON1_HOLD && button != BUTTON1_HOLD_CTRL) instrumentModeToIndices();
										switch (button)
										{
											case WHEEL_UP: case WHEEL_UP_CTRL:
												if (w->instrument > WHEEL_SPEED) w->instrument -= WHEEL_SPEED;
												else                             w->instrument = 0;
												w->effectscroll = 0;
												break;
											case WHEEL_DOWN: case WHEEL_DOWN_CTRL:
												if (w->instrument < 254 - WHEEL_SPEED) w->instrument += WHEEL_SPEED;
												else                                   w->instrument = 254;
												w->effectscroll = 0;
												break;
											case BUTTON_RELEASE: case BUTTON_RELEASE_CTRL:
												switch (w->mode)
												{
													case I_MODE_INDICES:
														if ((short)w->instrument + w->fyoffset < 0)        w->instrument = 0;
														else if ((short)w->instrument + w->fyoffset > 254) w->instrument = 254;
														else                                               w->instrument += w->fyoffset;
														w->effectscroll = 0;
														w->fyoffset = 0;
														break;
												} break;
											case BUTTON1_HOLD: case BUTTON1_HOLD_CTRL: break; /* ignore */
											default:
												if (y == CHANNEL_ROW-2)
												{
													switch (w->page)
													{ /* hacky implementation */
														case PAGE_INSTRUMENT_SAMPLE: if (x >= ((ws.ws_col-17)>>1) + 9) showInstrument(); break;
														case PAGE_INSTRUMENT_EFFECT: if (x <  ((ws.ws_col-17)>>1) + 9) showInstrument(); break;
													} break;
												} else if (y < CHANNEL_ROW-2)
												{
													if (x < ((ws.ws_col-17)>>1) + 7) showTracker();
													break;
												}

												switch (button)
												{
													case BUTTON2: case BUTTON2_CTRL:
														if (!(w->instrumentrecv != INST_REC_LOCK_OK && w->instrumentreci == w->instrument + y - w->centre))
														{
															yankInstrument(w->instrument + y - w->centre);
															delInstrument (w->instrument + y - w->centre);
														}
													case BUTTON1: case BUTTON1_CTRL:
														w->fyoffset = y - w->centre;
														break;
													case BUTTON3: case BUTTON3_CTRL:
														if ((short)w->instrument + (y - w->centre) < 0)        w->instrument = 0;
														else if ((short)w->instrument + (y - w->centre) > 254) w->instrument = 254;
														else                                                   w->instrument += y - w->centre;
														w->effectscroll = 0;

														instrumentOpenFilebrowser();
														break;
												}
												previewNote(' ', INST_VOID);
												break;
										}
									} p->dirty = 1; break;
									break;
							}
					} break;
				default:
					previewNote(' ', INST_VOID);
					cc.mouseadjust = cc.keyadjust = 0;
					/* NOTE: escape mode handling goes here */
					p->dirty = 1; break;
			} break;
		default:
			switch (w->mode)
			{
				case I_MODE_INDICES:
					switch (input)
					{ /* set count first */
						case '0': w->count *= 10; w->count += 0; p->dirty = 1; return;
						case '1': w->count *= 10; w->count += 1; p->dirty = 1; return;
						case '2': w->count *= 10; w->count += 2; p->dirty = 1; return;
						case '3': w->count *= 10; w->count += 3; p->dirty = 1; return;
						case '4': w->count *= 10; w->count += 4; p->dirty = 1; return;
						case '5': w->count *= 10; w->count += 5; p->dirty = 1; return;
						case '6': w->count *= 10; w->count += 6; p->dirty = 1; return;
						case '7': w->count *= 10; w->count += 7; p->dirty = 1; return;
						case '8': w->count *= 10; w->count += 8; p->dirty = 1; return;
						case '9': w->count *= 10; w->count += 9; p->dirty = 1; return;
						default:
							if (w->chord)
							{
								w->count = MIN(256, w->count);
								switch (w->chord)
								{
									case 'r': /* record        */ inputTooltip(&tt, input); break;
								} w->count = 0; p->dirty = 1;
							} else
								switch (input)
								{
									case '\t': /* not indices    */ if (instrumentSafe(s, w->instrument)) { w->mode = I_MODE_NORMAL; p->dirty = 1; } break;
									case 'r':  /* record         */ setChordRecord(); p->dirty = 1; return;
									case 'a':  /* add            */
										if (instrumentSafe(s, w->instrument)) { previewNote('a', w->instrument); }
										else                                  { addInstrument(w->instrument); p->dirty = 1; }
										break;
									case ' ': if (instrumentSafe(s, w->instrument)) previewNote(' ', w->instrument); break;
									case 'd': case 'x': case 127: case '\b': /* delete */
										if (!(w->instrumentrecv != INST_REC_LOCK_OK && w->instrumentreci == w->instrument) && instrumentSafe(s, w->instrument))
										{
											yankInstrument(w->instrument);
											delInstrument (w->instrument);
										} p->dirty = 1; break;
									case 'y':  /* yank    */ yankInstrument(w->instrument); p->dirty = 1; break;
									case 'p':  /* put     */ putInstrument (w->instrument); p->dirty = 1; break;
									case 'o': case '\n': case '\r': instrumentOpenFilebrowser(); p->dirty = 1; break;
									case 'e': /* export   */
										setCommand(&w->command, &sampleExportCallback, NULL, NULL, 0, "File name: ", "");
										w->mode = 255;
										p->dirty = 1; break;
								}
							break;
					} break;
				default:
					if (input == '\t') { w->mode = I_MODE_INDICES; p->dirty = 1; }
					else
						switch (w->page)
						{
							case PAGE_INSTRUMENT_SAMPLE: if (inputInstrumentSample(input)) return; break;
							case PAGE_INSTRUMENT_EFFECT: if (inputEffect(&s->instrumentv[s->instrumenti[w->instrument]].effect, input)) return; break;
						}
					break;
			} break;
	}
	if (w->count) { w->count = 0; p->dirty = 1; }
	if (w->chord) { w->chord = '\0'; clearTooltip(&tt); p->dirty = 1; }
}
