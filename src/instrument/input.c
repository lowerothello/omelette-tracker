void instrumentInput(int input)
{
	instrument *iv;
	int button, x, y;
	unsigned short yo;
	if (s->instrumenti[w->instrument] >= s->instrumentc) setControlCursor(&cc, 0);
	switch (input)
	{
		case '\033':
			switch (getchar())
			{
				case 'O':
					previewNote(' ', INST_VOID, w->channel);
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
							previewNote(' ', INST_VOID, w->channel);
							switch (getchar())
							{
								case 'A': /* linux f1 */ showTracker(); break;
								case 'B': /* linux f2 */ showInstrument(); break;
								case 'D': /* linux f4 */ showMaster(); break;
								case 'E': /* linux f5 */ startPlayback(); break;
							} redraw(); break;
						case 'A': /* up arrow    */ instrumentUpArrow(1);   redraw(); break;
						case 'B': /* down arrow  */ instrumentDownArrow(1); redraw(); break;
						case 'D': /* left arrow  */ instrumentLeftArrow();  redraw(); break;
						case 'C': /* right arrow */ instrumentRightArrow(); redraw(); break;
						case '1': /* mod+arrow / f5 - f8 */
							switch (getchar())
							{
								case '5': /* xterm f5, play  */ startPlayback(); getchar(); break;
								case '7': /* f6, stop        */
									previewNote(' ', INST_VOID, w->channel);
									stopPlayback(); getchar(); break;
								case ';': /* mod+arrow */
									switch (getchar())
									{
										case '5': /* ctrl+arrow */
											switch (getchar())
											{
												case 'D': /* left  */ instrumentCtrlLeftArrow();  redraw(); break;
												case 'C': /* right */ instrumentCtrlRightArrow(); redraw(); break;
											} break;
									} break;
								case '~': /* linux home */ instrumentHome(); redraw(); break;
							} break;
						case 'H': /* xterm home */ instrumentHome(); redraw(); break;
						case '4': /* end        */ if (getchar() == '~') { instrumentEnd(); redraw(); } break;
						case '5':
							switch (getchar())
							{
								case '~': /* page up        */ instrumentUpArrow(ws.ws_row>>1); redraw(); break;
								case ';': /* shift+scrollup */ getchar(); break;
							} break;
						case '6':
							switch (getchar())
							{
								case '~': /* page down      */ instrumentDownArrow(ws.ws_row>>1); redraw(); break;
								case ';': /* shift+scrolldn */ getchar(); break;
							} break;
						case 'M': /* mouse */
							button = getchar();
							x = getchar() - 32;
							y = getchar() - 32;
							yo = ws.ws_row - INSTRUMENT_CONTROL_ROW;
							if (cc.mouseadjust || (s->instrumenti[w->instrument] < s->instrumentc
										&& y > CHANNEL_ROW-2 && x >= INSTRUMENT_INDEX_COLS))
							{
								instrumentModeToNormal();
								iv = &s->instrumentv[s->instrumenti[w->instrument]];
								mouseControls(&cc, button, x, y);
								if (y < yo && !cc.mouseadjust)
								{
									setControlCursor(&cc, 0);
									uint32_t offset;
									switch (button)
									{
										case WHEEL_UP: case WHEEL_UP_CTRL:
											w->waveformwidth /= 2;
											w->waveformdrawpointer = 0;
											break;
										case WHEEL_DOWN: case WHEEL_DOWN_CTRL:
											w->waveformwidth = MIN(iv->length, w->waveformwidth*2);
											w->waveformdrawpointer = 0;
											break;
										case BUTTON1: case BUTTON1_CTRL:
											if (w->mode == I_MODE_VISUAL) w->mode = I_MODE_NORMAL;

											if (w->waveformcursor < (w->waveformwidth>>1))                   offset = 0;
											else if (w->waveformcursor > iv->length - (w->waveformwidth>>1)) offset = (iv->length - w->waveformwidth)<<1;
											else                                                             offset = (w->waveformcursor<<1) - w->waveformwidth;

											w->waveformcursor = MIN(iv->length-1, (uint32_t)((offset>>2) + (float)(x - INSTRUMENT_INDEX_COLS) / (float)w->waveformw * w->waveformwidth)<<1);
											w->waveformdrawpointer = 0;
											break;
										case BUTTON3: case BUTTON3_CTRL:
											if (w->mode != I_MODE_VISUAL) { w->mode = I_MODE_VISUAL; w->waveformvisual = w->waveformcursor; }

											if (w->waveformcursor < (w->waveformwidth>>1))                   offset = 0;
											else if (w->waveformcursor > iv->length - (w->waveformwidth>>1)) offset = (iv->length - w->waveformwidth)<<1;
											else                                                             offset = (w->waveformcursor<<1) - w->waveformwidth;

											w->waveformcursor = MIN(iv->length-1, (uint32_t)((offset>>2) + (float)(x - INSTRUMENT_INDEX_COLS) / (float)w->waveformw * w->waveformwidth)<<1);
											w->waveformdrawpointer = 0;
											break;
									}
								}
							} else
							{
								instrumentModeToIndices();
								switch (button)
								{
									case WHEEL_UP: case WHEEL_UP_CTRL:
										if (w->instrument > WHEEL_SPEED) w->instrument -= WHEEL_SPEED;
										else                             w->instrument = 0;
										resetWaveform(); break;
									case WHEEL_DOWN: case WHEEL_DOWN_CTRL:
										if (w->instrument < 254 - WHEEL_SPEED) w->instrument += WHEEL_SPEED;
										else                                   w->instrument = 254;
										resetWaveform(); break;
									case BUTTON_RELEASE: case BUTTON_RELEASE_CTRL:
										switch (w->mode)
										{
											case I_MODE_INDICES: case I_MODE_INDICES_PREVIEW:
												if ((short)w->instrument + w->fyoffset < 0)        w->instrument = 0;
												else if ((short)w->instrument + w->fyoffset > 254) w->instrument = 254;
												else                                               w->instrument += w->fyoffset;
												resetWaveform();
												w->fyoffset = 0;
												break;
										} break;
									default:
										if (y <= CHANNEL_ROW-2)
										{
											if (x < (ws.ws_col-17) / 2 + 7) showTracker();
											else                            showInstrument();
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

												if (s->instrumenti[w->instrument] >= s->instrumentc) addInstrument(w->instrument);
												w->popup = 15;
												w->fyoffset = 0;
												w->oldmode = w->mode;
												w->mode = 0;
												w->filebrowserCallback = &sampleLoadCallback;
												break;
										}
										previewNote(' ', INST_VOID, w->channel);
										break;
								}
							} redraw(); break;
					} break;
				default:
					previewNote(' ', INST_VOID, w->channel);
					cc.mouseadjust = cc.keyadjust = 0;
					switch (w->mode)
					{
						case I_MODE_VISUAL: w->waveformdrawpointer = 0; /* fall through */
						case I_MODE_PREVIEW:
							w->mode = I_MODE_NORMAL; break;
						case I_MODE_INDICES_PREVIEW:
							w->mode = I_MODE_INDICES; break;
					} redraw(); break;
			} break;
		default:
			switch (w->mode)
			{
				case I_MODE_INDICES:
					switch (input)
					{ /* set count first */
						case '0': w->count *= 10; w->count += 0; redraw(); return;
						case '1': w->count *= 10; w->count += 1; redraw(); return;
						case '2': w->count *= 10; w->count += 2; redraw(); return;
						case '3': w->count *= 10; w->count += 3; redraw(); return;
						case '4': w->count *= 10; w->count += 4; redraw(); return;
						case '5': w->count *= 10; w->count += 5; redraw(); return;
						case '6': w->count *= 10; w->count += 6; redraw(); return;
						case '7': w->count *= 10; w->count += 7; redraw(); return;
						case '8': w->count *= 10; w->count += 8; redraw(); return;
						case '9': w->count *= 10; w->count += 9; redraw(); return;
						default:
							if (w->chord)
							{
								w->count = MIN(256, w->count);
								switch (w->chord)
								{
									case 'r': /* record        */ inputTooltip(&tt, input); break;
									case 'I': /* macro preview */ changeMacro(input, &w->keyboardmacro); w->mode = I_MODE_INDICES_PREVIEW; redraw(); break;
								} w->count = 0; redraw();
							} else
							{
								switch (input)
								{
									case '\t': /* not indices    */ w->mode = I_MODE_NORMAL; resetWaveform(); redraw(); break;
									case 'i':  /* preview        */ setControlCursor(&cc, 0); w->mode = I_MODE_INDICES_PREVIEW; redraw(); break;
									case 'I':  /* macro prevew   */ w->chord = 'I'; redraw(); return;
									case 'r':  /* record         */ setChordRecord(); redraw(); return;
									case 'k':  /* up arrow       */ instrumentUpArrow(1);   redraw(); break;
									case 'j':  /* down arrow     */ instrumentDownArrow(1); redraw(); break;
									case 'h':  /* left arrow     */ instrumentLeftArrow();  redraw(); break;
									case 'l':  /* right arrow    */ instrumentRightArrow(); redraw(); break;
									case 'a':  /* add            */
										if (s->instrumenti[w->instrument] >= s->instrumentc) { addInstrument(w->instrument); redraw(); }
										else                                                 { previewNote('a', w->instrument, w->channel); }
										break;
									case ' ': if (s->instrumenti[w->instrument] < s->instrumentc) previewNote(' ', w->instrument, w->channel); break;
									case 'd': case 'x': case 127: case '\b': /* delete */
										if (!(w->instrumentrecv != INST_REC_LOCK_OK && w->instrumentreci == w->instrument) && s->instrumenti[w->instrument] < s->instrumentc)
										{
											yankInstrument(w->instrument);
											delInstrument (w->instrument);
										} redraw(); break;
									case 'y':  /* yank    */ yankInstrument(w->instrument); redraw(); break;
									case 'p':  /* put     */ putInstrument(w->instrument); redraw(); break;
									case '\n': case '\r':
										if (s->instrumenti[w->instrument] >= s->instrumentc) addInstrument(w->instrument);
										w->popup = 15;
										w->fyoffset = 0;
										w->oldmode = w->mode;
										w->mode = 0;
										w->filebrowserCallback = &sampleLoadCallback;
										redraw(); break;
									case 'e': /* export   */
										setCommand(&w->command, &sampleExportCallback, NULL, NULL, 0, "File name: ", "");
										w->mode = 255;
										redraw(); break;
								}
							} break;
					} break;
				case I_MODE_VISUAL:
					switch (input)
					{ /* set count first */
						case '0': w->count *= 10; w->count += 0; redraw(); return;
						case '1': w->count *= 10; w->count += 1; redraw(); return;
						case '2': w->count *= 10; w->count += 2; redraw(); return;
						case '3': w->count *= 10; w->count += 3; redraw(); return;
						case '4': w->count *= 10; w->count += 4; redraw(); return;
						case '5': w->count *= 10; w->count += 5; redraw(); return;
						case '6': w->count *= 10; w->count += 6; redraw(); return;
						case '7': w->count *= 10; w->count += 7; redraw(); return;
						case '8': w->count *= 10; w->count += 8; redraw(); return;
						case '9': w->count *= 10; w->count += 9; redraw(); return;
						default:
							if (w->chord)
							{
								w->count = MIN(256, w->count);
								switch (w->chord)
								{
									case 'z': /* zoom   */ inputTooltip(&tt, input); break;
								} w->count = 0; redraw();
							} else
								switch (input)
								{
									case 'v':  /* normal           */ w->mode = I_MODE_NORMAL; redraw(); break;
									case '\t': /* indices          */ w->mode = I_MODE_INDICES; resetWaveform(); redraw(); break;
									case 'i':  /* preview          */ w->mode = I_MODE_PREVIEW; w->waveformdrawpointer = 0; redraw(); break;
									case 'z':  /* zoom             */ if (!cc.cursor) { setChordZoom(); redraw(); return; } break;
									case '+': case '=': /* zoom in */ if (s->instrumenti[w->instrument] < s->instrumentc) { w->waveformwidth /= 2; w->waveformdrawpointer = 0; redraw(); } break;
									case '-':  /* zoom out         */ if (s->instrumenti[w->instrument] < s->instrumentc) { w->waveformwidth = MIN(s->instrumentv[s->instrumenti[w->instrument]].length, w->waveformwidth*2); w->waveformdrawpointer = 0; redraw(); } break;
									case 'k':  /* up arrow         */ instrumentUpArrow(1);   redraw(); break;
									case 'j':  /* down arrow       */ instrumentDownArrow(1); redraw(); break;
									case 'h':  /* left arrow       */ instrumentLeftArrow();  redraw(); break;
									case 'l':  /* right arrow      */ instrumentRightArrow(); redraw(); break;
									case 't':  /* trim             */
										if (s->instrumenti[w->instrument] < s->instrumentc)
										{
											s->instrumentv[s->instrumenti[w->instrument]].trim[0] = MIN(w->waveformcursor, w->waveformvisual);
											s->instrumentv[s->instrumenti[w->instrument]].trim[1] = MAX(w->waveformcursor, w->waveformvisual);
											w->mode = I_MODE_NORMAL; w->waveformdrawpointer = 0; redraw();
										} break;
								}
							break;
					} break;
				case I_MODE_NORMAL:
					if (!cc.cursor)
						switch (input)
						{ /* check counts first */
							case '0': w->count *= 10; w->count += 0; redraw(); return;
							case '1': w->count *= 10; w->count += 1; redraw(); return;
							case '2': w->count *= 10; w->count += 2; redraw(); return;
							case '3': w->count *= 10; w->count += 3; redraw(); return;
							case '4': w->count *= 10; w->count += 4; redraw(); return;
							case '5': w->count *= 10; w->count += 5; redraw(); return;
							case '6': w->count *= 10; w->count += 6; redraw(); return;
							case '7': w->count *= 10; w->count += 7; redraw(); return;
							case '8': w->count *= 10; w->count += 8; redraw(); return;
							case '9': w->count *= 10; w->count += 9; redraw(); return;
						}

					if (w->chord)
					{
						w->count = MIN(256, w->count);
						switch (w->chord)
						{
							case 'r': /* record */ inputTooltip(&tt, input); break;
							case 'z': /* zoom   */ inputTooltip(&tt, input); break;
							case 'I': /* macro preview */ changeMacro(input, &w->keyboardmacro); w->mode = I_MODE_INDICES_PREVIEW; redraw(); break;
							case 'd': /* delete */
								if (input == 'd')
								{
									if (s->instrumenti[w->instrument] < s->instrumentc)
									{
										if (s->instrumentv[s->instrumenti[w->instrument]].samplelength)
											free(s->instrumentv[s->instrumenti[w->instrument]].sampledata);
										s->instrumentv[s->instrumenti[w->instrument]].sampledata = NULL;
										s->instrumentv[s->instrumenti[w->instrument]].samplelength = 0;
										s->instrumentv[s->instrumenti[w->instrument]].channels = 0;
										s->instrumentv[s->instrumenti[w->instrument]].length = 0;
										s->instrumentv[s->instrumenti[w->instrument]].c5rate = 0;
										s->instrumentv[s->instrumenti[w->instrument]].trim[0] = 0;
										s->instrumentv[s->instrumenti[w->instrument]].trim[1] = 0;
										s->instrumentv[s->instrumenti[w->instrument]].loop = 0;
									}
								} break;
						} w->count = 0; redraw();
					} else
					{
						if (cc.cursor)
							switch (input)
							{
								case '\n': case '\r':             toggleKeyControl(&cc); redraw(); break;
								case '\t': /* indices          */ w->mode = I_MODE_INDICES; resetWaveform(); redraw(); break;
								case 'i':  /* preview          */ w->mode = I_MODE_PREVIEW; redraw(); break;
								case 'I':  /* macro prevew     */ w->chord = 'I'; redraw(); return;
								case 'r':  /* record           */ setChordRecord(); redraw(); return;
								case 'k':  /* up arrow         */ instrumentUpArrow(1);   redraw(); break;
								case 'j':  /* down arrow       */ instrumentDownArrow(1); redraw(); break;
								case 'h':  /* left arrow       */ instrumentLeftArrow();  redraw(); break;
								case 'l':  /* right arrow      */ instrumentRightArrow(); redraw(); break;
								case 127: case '\b': /* backspace */ decControlFieldpointer(&cc); redraw(); break;
								case ' ':            /* space     */ incControlFieldpointer(&cc); redraw(); break;
								case 1:  /* ^a */
									if (s->instrumenti[w->instrument] < s->instrumentc)
									{
										incControlValue(&cc);
										redraw();
									} break;
								case 24: /* ^x */
									if (s->instrumenti[w->instrument] < s->instrumentc)
									{
										decControlValue(&cc);
										redraw();
									} break;
								case '0':           if (s->instrumenti[w->instrument] < s->instrumentc) { hexControlValue(&cc, 0); redraw();  } break;
								case '1':           if (s->instrumenti[w->instrument] < s->instrumentc) { hexControlValue(&cc, 1); redraw();  } break;
								case '2':           if (s->instrumenti[w->instrument] < s->instrumentc) { hexControlValue(&cc, 2); redraw();  } break;
								case '3':           if (s->instrumenti[w->instrument] < s->instrumentc) { hexControlValue(&cc, 3); redraw();  } break;
								case '4':           if (s->instrumenti[w->instrument] < s->instrumentc) { hexControlValue(&cc, 4); redraw();  } break;
								case '5':           if (s->instrumenti[w->instrument] < s->instrumentc) { hexControlValue(&cc, 5); redraw();  } break;
								case '6':           if (s->instrumenti[w->instrument] < s->instrumentc) { hexControlValue(&cc, 6); redraw();  } break;
								case '7':           if (s->instrumenti[w->instrument] < s->instrumentc) { hexControlValue(&cc, 7); redraw();  } break;
								case '8':           if (s->instrumenti[w->instrument] < s->instrumentc) { hexControlValue(&cc, 8); redraw();  } break;
								case '9':           if (s->instrumenti[w->instrument] < s->instrumentc) { hexControlValue(&cc, 9); redraw();  } break;
								case 'A': case 'a': if (s->instrumenti[w->instrument] < s->instrumentc) { hexControlValue(&cc, 10); redraw(); } break;
								case 'B': case 'b': if (s->instrumenti[w->instrument] < s->instrumentc) { hexControlValue(&cc, 11); redraw(); } break;
								case 'C': case 'c': if (s->instrumenti[w->instrument] < s->instrumentc) { hexControlValue(&cc, 12); redraw(); } break;
								case 'D': case 'd': if (s->instrumenti[w->instrument] < s->instrumentc) { hexControlValue(&cc, 13); redraw(); } break;
								case 'E': case 'e': if (s->instrumenti[w->instrument] < s->instrumentc) { hexControlValue(&cc, 14); redraw(); } break;
								case 'F': case 'f': if (s->instrumenti[w->instrument] < s->instrumentc) { hexControlValue(&cc, 15); redraw(); } break;
							}
						else
							switch (input)
							{
								case '\n': case '\r':
									if (s->instrumenti[w->instrument >= s->instrumentc])
										addInstrument(w->instrument);
									w->popup = 15;
									w->fyoffset = 0;
									w->oldmode = w->mode;
									w->mode = 0;
									w->filebrowserCallback = &sampleLoadCallback;
									redraw(); break;
								case 'v':  /* visual               */ w->mode = I_MODE_VISUAL; w->waveformvisual = w->waveformcursor; w->waveformdrawpointer = 0; redraw(); break;
								case 'z':  /* zoom                 */ setChordZoom(); redraw(); return;
								case '+': case '=': /* zoom in     */ if (s->instrumenti[w->instrument] < s->instrumentc) { w->waveformwidth /= 2; w->waveformdrawpointer = 0; redraw(); } break;
								case '-':  /* zoom out             */ if (s->instrumenti[w->instrument] < s->instrumentc) { w->waveformwidth = MIN(s->instrumentv[s->instrumenti[w->instrument]].length, w->waveformwidth*2); w->waveformdrawpointer = 0; redraw(); } break;
								case 's':  /* trim start to cursor */ if (s->instrumenti[w->instrument] < s->instrumentc) { s->instrumentv[s->instrumenti[w->instrument]].trim[0] = w->waveformcursor; w->waveformdrawpointer = 0; redraw(); } break;
								case 'e':  /* trim end to cursor   */ if (s->instrumenti[w->instrument] < s->instrumentc) { s->instrumentv[s->instrumenti[w->instrument]].trim[1] = w->waveformcursor; w->waveformdrawpointer = 0; redraw(); } break;
								case 'l':  /* loop to cursor       */ if (s->instrumenti[w->instrument] < s->instrumentc) { s->instrumentv[s->instrumenti[w->instrument]].loop    = w->waveformcursor; w->waveformdrawpointer = 0; redraw(); } break;
								case 'd':  /* delete               */ w->chord = 'd'; redraw(); return;
								case '\t': /* indices              */ w->mode = I_MODE_INDICES; resetWaveform(); redraw(); break;
								case 'i':  /* preview              */ w->mode = I_MODE_PREVIEW; redraw(); break;
								case 'I':  /* macro prevew         */ w->chord = 'I'; redraw(); return;
								case 'r':  /* record               */ setChordRecord(); redraw(); return;
								// case 'k':  /* up arrow             */ if (s->instrumenti[w->instrument] < s->instrumentc) { instrumentUpArrow(1, &s->instrumentv[s->instrumenti[w->instrument]]); redraw();   } break;
								// case 'j':  /* down arrow           */ if (s->instrumenti[w->instrument] < s->instrumentc) { instrumentDownArrow(1, &s->instrumentv[s->instrumenti[w->instrument]]); redraw(); } break;
								// case 'h':  /* left arrow           */ if (s->instrumenti[w->instrument] < s->instrumentc) { instrumentLeftArrow(&s->instrumentv[s->instrumenti[w->instrument]]); redraw();    } break;
								// case 'l':  /* right arrow          */ if (s->instrumenti[w->instrument] < s->instrumentc) { instrumentRightArrow(is->instrumentv[s->instrumenti[w->instrument]]); redraw();   } break; /* conflicts with setting the loop mark */
								case 'a': if (s->instrumenti[w->instrument] < s->instrumentc) previewNote('a', w->instrument, w->channel); break;
								case ' ': if (s->instrumenti[w->instrument] < s->instrumentc) previewNote(' ', w->instrument, w->channel); break;
							}
					} break;
				case I_MODE_PREVIEW: case I_MODE_INDICES_PREVIEW:
					switch (input)
					{
						case '\n': case '\r':
							switch (w->mode)
							{
								case I_MODE_PREVIEW:
									if (!cc.cursor)
									{
										if (s->instrumenti[w->instrument] >= s->instrumentc) addInstrument(w->instrument);
										w->popup = 15;
										w->fyoffset = 0;
										w->oldmode = I_MODE_NORMAL;
										w->mode = 0;
										w->filebrowserCallback = &sampleLoadCallback;
									} else toggleKeyControl(&cc);
									redraw(); break;
								case I_MODE_INDICES_PREVIEW:
									if (s->instrumenti[w->instrument] >= s->instrumentc) addInstrument(w->instrument);
									w->popup = 15;
									w->fyoffset = 0;
									w->oldmode = I_MODE_NORMAL;
									w->mode = 0;
									w->filebrowserCallback = &sampleLoadCallback;
									redraw(); break;
							} break;
						case '\t':
							switch (w->mode)
							{
								case I_MODE_PREVIEW: w->mode = I_MODE_INDICES_PREVIEW; break;
								case I_MODE_INDICES_PREVIEW: w->mode = I_MODE_PREVIEW; break;
							} resetWaveform(); redraw(); break;
						case '0': w->octave = 0; redraw(); break;
						case '1': w->octave = 1; redraw(); break;
						case '2': w->octave = 2; redraw(); break;
						case '3': w->octave = 3; redraw(); break;
						case '4': w->octave = 4; redraw(); break;
						case '5': w->octave = 5; redraw(); break;
						case '6': w->octave = 6; redraw(); break;
						case '7': w->octave = 7; redraw(); break;
						case '8': w->octave = 8; redraw(); break;
						case '9': w->octave = 9; redraw(); break;
						default: previewNote(input, w->instrument, w->channel); break;
					} break;
			} break;
	}
	if (w->count) { w->count = 0; redraw(); }
	if (w->chord) { w->chord = '\0'; clearTooltip(&tt); redraw(); }
}
