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
			case PAGE_INSTRUMENT_EFFECT: effectCtrlUpArrow(s->instrument->v[s->instrument->i[w->instrument]].effect, count); break;
		}
}
void instrumentCtrlDownArrow(int count)
{
	if (w->mode != I_MODE_INDICES)
		switch (w->page)
		{
			case PAGE_INSTRUMENT_SAMPLE: break;
			case PAGE_INSTRUMENT_EFFECT: effectCtrlDownArrow(s->instrument->v[s->instrument->i[w->instrument]].effect, count); break;
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
void instrumentCtrlLeftArrow(int count)
{
	w->instrument -= count;
	if (w->instrument < 0) w->instrument = 0;
	w->effectscroll = 0;
	if (!instrumentSafe(s, w->instrument))
		w->mode = I_MODE_INDICES;
	resetWaveform();
}
void instrumentCtrlRightArrow(int count)
{
	w->instrument += count;
	if (w->instrument > 254) w->instrument = 254;
	w->effectscroll = 0;
	if (!instrumentSafe(s, w->instrument))
		w->mode = I_MODE_INDICES;
	resetWaveform();
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
						case 'P': /* xterm f1 */ showTracker   (); break;
						case 'Q': /* xterm f2 */ showInstrument(); break;
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
								case 'E': /* linux f5 */ startPlayback(); break;
							} p->redraw = 1; break;
						case 'A': /* up arrow    */ instrumentUpArrow  (1); p->redraw = 1; break;
						case 'B': /* down arrow  */ instrumentDownArrow(1); p->redraw = 1; break;
						case 'D': /* left arrow  */ instrumentLeftArrow (); p->redraw = 1; break;
						case 'C': /* right arrow */ instrumentRightArrow(); p->redraw = 1; break;
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
												case 'A': /* up    */ instrumentCtrlUpArrow   (1); p->redraw = 1; break;
												case 'B': /* down  */ instrumentCtrlDownArrow (1); p->redraw = 1; break;
												case 'C': /* right */ instrumentCtrlRightArrow(1); p->redraw = 1; break;
												case 'D': /* left  */ instrumentCtrlLeftArrow (1); p->redraw = 1; break;
											} break;
										default: getchar(); break;
									} break;
								case '~': /* linux home */ instrumentHome(); p->redraw = 1; break;
							} break;
						case 'H': /* xterm home */         instrumentHome(); p->redraw = 1; break;
						case '4': /* end */ if (getchar() == '~') { instrumentEnd (); p->redraw = 1; } break;
						case '5': /* page up / shift+scrollup */ getchar(); instrumentUpArrow  (ws.ws_row>>1); p->redraw = 1; break;
						case '6': /* page dn / shift+scrolldn */ getchar(); instrumentDownArrow(ws.ws_row>>1); p->redraw = 1; break;
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
									if (instrumentSafe(s, w->instrument) && !s->instrument->v[s->instrument->i[w->instrument]].sample->length
											&& y > CHANNEL_ROW-2 && x >= INSTRUMENT_INDEX_COLS)
										filebrowserMouse(button, x, y);
									else if (cc.mouseadjust || (instrumentSafe(s, w->instrument)
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
												w->effectscroll = 0; resetWaveform();
												break;
											case WHEEL_DOWN: case WHEEL_DOWN_CTRL:
												if (w->instrument < 254 - WHEEL_SPEED) w->instrument += WHEEL_SPEED;
												else                                   w->instrument = 254;
												w->effectscroll = 0; resetWaveform();
												break;
											case BUTTON_RELEASE: case BUTTON_RELEASE_CTRL:
												switch (w->mode)
												{
													case I_MODE_INDICES:
														if ((short)w->instrument + w->fyoffset < 0)        w->instrument = 0;
														else if ((short)w->instrument + w->fyoffset > 254) w->instrument = 254;
														else                                               w->instrument += w->fyoffset;
														w->effectscroll = 0; resetWaveform();
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
					w->page = PAGE_INSTRUMENT_SAMPLE;
					/* NOTE: escape mode handling goes here */
					p->redraw = 1; break;
			} break;
		default:
			switch (w->mode)
			{
				case I_MODE_INDICES:
					switch (input)
					{ /* set count first */
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
						default:
							if (w->chord)
							{
								w->count = MIN(256, w->count);
								switch (w->chord)
								{
									case 'r': /* record     */ inputTooltip(&tt, input); break;
									case 'a': /* add inst   */ inputTooltip(&tt, input); break;
									case 'e': /* empty inst */ inputTooltip(&tt, input); break;
									case 'y': /* yank       */ inputTooltip(&tt, input); break;
									case 'd': /* delete     */ inputTooltip(&tt, input); break;
								} w->count = 0; p->redraw = 1;
							} else
								switch (input)
								{
									case '\t': /* not indices    */ if (instrumentSafe(s, w->instrument)) { w->mode = I_MODE_NORMAL; p->redraw = 1; } break;
									case 'b':  /* bpm            */ if (w->count) { s->songbpm = MIN(255, MAX(32, w->count)); reapplyBpm(); } p->redraw = 1; break;
									case 'r':  /* record         */ setChordRecord(); p->redraw = 1; return;
									case 'a':  /* add            */
										if (instrumentSafe(s, w->instrument)) { previewNote('a', w->instrument); }
										else                                  { setChordAddInst(); p->redraw = 1; return; }
										break;
									case 'e': /* add empty */
										w->instrument = emptyInstrument(0);
										setChordAddInst(); w->chord = 'e';
										p->redraw = 1; return;
									case ' ': if (instrumentSafe(s, w->instrument)) previewNote(' ', w->instrument); break;
									case 'x': case 127: case '\b': /* delete (no chord) */ chordDeleteInstrument(0); break;
									case 'd': /* delete   */ setChordDeleteInstrument(); p->redraw = 1; return;
									case 'y': /* yank     */ setChordYankInstrument  (); p->redraw = 1; return;
									case 'p': /* put      */ putInstrument(w->instrument); p->redraw = 1; break;
									/* case 'e': // export
										setCommand(&w->command, &sampleExportCallback, NULL, NULL, 0, "File name: ", "");
										w->mode = 255;
										p->redraw = 1; break; */
								}
							break;
					} break;
				default:
					if (input == '\t') { w->mode = I_MODE_INDICES; p->redraw = 1; }
					else
						switch (w->page)
						{
							case PAGE_INSTRUMENT_SAMPLE: if (inputInstrumentSample(input)) return; break;
							case PAGE_INSTRUMENT_EFFECT: if (inputEffect(&s->instrument->v[s->instrument->i[w->instrument]].effect, input)) return; break;
						}
					break;
			} break;
	}
	if (w->count) { w->count = 0; p->redraw = 1; }
	if (w->chord) { w->chord = '\0'; clearTooltip(&tt); p->redraw = 1; }
}
