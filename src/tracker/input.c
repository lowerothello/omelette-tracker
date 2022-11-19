#include "variantinput.c"
#include "effectinput.c"

void trackerInput(int input)
{
	int button, x, y, i, j;
	short chanw;
	TrackData *cd = &s->track->v[w->track].data;
	switch (input)
	{
		case '\033': /* escape */
			switch ((input = getchar()))
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
					previewNote(' ', INST_VOID);
					switch (getchar())
					{
						case '[':
							switch (getchar())
							{
								case 'A': /* linux f1 */ showTracker   (); break;
								case 'B': /* linux f2 */ showInstrument(); break;
								case 'C': /* linux f3 */ showMaster    (); break;
								case 'E': /* linux f5 */ leaveSpecialModes(); startPlayback(); break;
							} p->redraw = 1; break;
						case 'A': /* up arrow    */ trackerUpArrow  (MAX(1, w->count)); p->redraw = 1; break;
						case 'B': /* down arrow  */ trackerDownArrow(MAX(1, w->count)); p->redraw = 1; break;
						case 'D': /* left arrow  */ trackerLeftArrow (MAX(1, w->count)); p->redraw = 1; break;
						case 'C': /* right arrow */ trackerRightArrow(MAX(1, w->count)); p->redraw = 1; break;
						case '1': /* mod+arrow / f5 - f8 */
							switch (getchar())
							{
								case '5': /* xterm f5 */ leaveSpecialModes(); startPlayback(); getchar(); break;
								case '7': /* f6       */ leaveSpecialModes(); stopPlayback(); getchar(); break;
								case ';': /* mod+arrow */
									switch (getchar())
									{
										case '5': /* ctrl+arrow */
											switch (getchar())
											{
												case 'D': /* left  */ trackLeft (MAX(1, w->count)); p->redraw = 1; break;
												case 'C': /* right */ trackRight(MAX(1, w->count)); p->redraw = 1; break;
												case 'A': /* up    */ cycleUp  (MAX(1, w->count)); p->redraw = 1; break;
												case 'B': /* down  */ cycleDown(MAX(1, w->count)); p->redraw = 1; break;
											} break;
										case '2': /* shift+arrow */
											switch (getchar())
											{
												case 'A': /* up    */ shiftUp  (MAX(1, w->count)); p->redraw = 1; break;
												case 'B': /* down  */ shiftDown(MAX(1, w->count)); p->redraw = 1; break;
											} break;
										default: getchar(); break;
									} break;
								case '~': /* linux home */ trackerHome(); p->redraw = 1; break;
							} break;
						case 'H': /* xterm home */ trackerHome(); p->redraw = 1; break;
						case '4': /* end        */ if (getchar() == '~') { trackerEnd(); p->redraw = 1; } break;
						case '5': /* page up / shift+scrollup */
							switch (getchar())
							{
								case '~': /* page up */ trackerPgUp(1); p->redraw = 1; break;
								case ';': /* shift+scrollup */
									getchar(); /* 2 */
									getchar(); /* ~ */
									// trackerPgUp(1);
									p->redraw = 1; break;
							} break;
						case '6': /* page dn / shift+scrolldn */
							switch (getchar())
							{
								case '~': /* page dn */ trackerPgDn(1); p->redraw = 1; break;
								case ';': /* shift+scrolldn */
									getchar(); /* 2 */
									getchar(); /* ~ */
									// trackerPgDn(1);
									p->redraw = 1; break;
							} break;
						case 'M': /* mouse */
							button = getchar();
							x = getchar() - 32;
							y = getchar() - 32;

							short oldtrackerfx = w->trackerfx;
							uint8_t oldtrack = w->track;

							short tx = 1 + TRACK_LINENO_COLS + 2 + genSfx(TRACK_LINENO_COLS);

							switch (button)
							{
								case BUTTON2_HOLD: case BUTTON2_HOLD_CTRL:
								case BUTTON3_HOLD: case BUTTON3_HOLD_CTRL:
									break;
								case BUTTON_RELEASE: case BUTTON_RELEASE_CTRL:
									if      (w->trackoffset < 0) trackLeft (-w->trackoffset);
									else if (w->trackoffset > 0) trackRight( w->trackoffset);

									if      (w->fyoffset < 0) trackerUpArrow  (-w->fyoffset);
									else if (w->fyoffset > 0) trackerDownArrow( w->fyoffset);

									if      (w->shiftoffset < 0) shiftUp  (-w->shiftoffset);
									else if (w->shiftoffset > 0) shiftDown( w->shiftoffset);
									w->fyoffset = w->shiftoffset = w->trackoffset = w->fieldpointer = 0;

									switch (w->mode)
									{ /* leave mouseadjust mode */
										case T_MODE_MOUSEADJUST:
											w->mode = w->oldmode; break;
									} p->redraw = 1;
									/* falls through intentionally */
								default:
									switch (w->page)
									{
										case PAGE_TRACK_VARIANT:
											switch (button)
											{
												case BUTTON_RELEASE: case BUTTON_RELEASE_CTRL:
													break;
												case WHEEL_UP: case WHEEL_UP_CTRL:     trackerUpArrow  (WHEEL_SPEED); break;
												case WHEEL_DOWN: case WHEEL_DOWN_CTRL: trackerDownArrow(WHEEL_SPEED); break;
												case BUTTON1_HOLD: case BUTTON1_HOLD_CTRL:
													w->follow = 0;
													switch (w->mode)
													{
														case T_MODE_MOUSEADJUST:
															if      (x > w->mousex) { trackerAdjustRight(cd); }
															else if (x < w->mousex) { trackerAdjustLeft (cd); }
															break;
													} w->mousex = x; break;
												default: /* click */
													if (trackerMouseHeader(button, x, y, &tx)) break;

													if (y > ws.ws_row - 1) break; /* ignore clicking out of range */
													w->follow = 0;

													for (i = 0; i < s->track->c; i++)
													{
														chanw = TRACK_TRIG_COLS + 8 + 4*(s->track->v[i].data.variant->macroc+1);
														if (i == s->track->c-1 || tx+chanw > x) /* clicked on track i */
														{
															if (button == BUTTON1_CTRL) { w->step = MIN(15, abs(y - w->centre)); break; }

															switch (button)
															{
																case BUTTON1:
																	if (w->mode != T_MODE_INSERT) /* suggest normal mode, but allow insert */
																		w->mode = T_MODE_NORMAL;
																	break;
																case BUTTON3:
																	if (!(w->mode == T_MODE_VISUAL || w->mode == T_MODE_VISUALLINE || w->mode == T_MODE_VISUALREPLACE))
																	{
																		w->visualfx = tfxToVfx(oldtrackerfx);
																		w->visualfy = w->trackerfy;
																		w->visualtrack = oldtrack;
																		w->mode = T_MODE_VISUAL;
																	} break;
															}
															if (x-tx < TRACK_TRIG_COLS-1) /* vtrig column */
															{
																w->trackerfx = -1;
																if (x-tx < 2) w->fieldpointer = 1;
																else          w->fieldpointer = 0;
															} else if (x-tx < TRACK_TRIG_COLS + 3) /* note column */
																w->trackerfx = 0;
															else if (x-tx < TRACK_TRIG_COLS + 6) /* inst column */
															{
																w->trackerfx = 1;
																if (x-tx < TRACK_TRIG_COLS + 5) w->fieldpointer = 1;
																else                              w->fieldpointer = 0;
															} else if (x-tx > TRACK_TRIG_COLS + 5 + 4*(s->track->v[i].data.variant->macroc+1)) /* star column */
															{
																w->trackerfx = 3;
																w->fieldpointer = 0;
															} else /* macro column */
															{
																j = x-tx - (TRACK_TRIG_COLS + 6);
																if ((j>>1)&0x1) w->trackerfx = 3 + ((s->track->v[i].data.variant->macroc - (j>>2))<<1)+0;
																else            w->trackerfx = 3 + ((s->track->v[i].data.variant->macroc - (j>>2))<<1)-1;
																if (j&0x1) w->fieldpointer = 0;
																else       w->fieldpointer = 1;
															}

															w->trackoffset = i - w->track;
															if (button == BUTTON3_CTRL) w->shiftoffset = y - w->centre;
															else                        w->fyoffset    = y - w->centre;

															if (button == BUTTON2 || button == BUTTON2_CTRL)
															{
																if (w->trackerfx == 0)
																{
																	yankPartPattern(0, 1, w->trackerfy+w->fyoffset, w->trackerfy+w->fyoffset, w->track+w->trackoffset, w->track+w->trackoffset);
																	delPartPattern (0, 1, w->trackerfy+w->fyoffset, w->trackerfy+w->fyoffset, w->track+w->trackoffset, w->track+w->trackoffset);
																} else
																{
																	yankPartPattern(tfxToVfx(w->trackerfx), tfxToVfx(w->trackerfx), w->trackerfy+w->fyoffset, w->trackerfy+w->fyoffset, w->track+w->trackoffset, w->track+w->trackoffset);
																	delPartPattern (tfxToVfx(w->trackerfx), tfxToVfx(w->trackerfx), w->trackerfy+w->fyoffset, w->trackerfy+w->fyoffset, w->track+w->trackoffset, w->track+w->trackoffset);
																} break;
															}

															/* enter adjust */
															if ((button == BUTTON1 || button == BUTTON1_CTRL)
																	&& w->fyoffset == 0 && w->trackerfx == oldtrackerfx && w->trackoffset == 0)
															{
																w->oldmode = w->mode;
																w->mode = T_MODE_MOUSEADJUST;
																w->mousex = x;
															} break;
														}
														tx += chanw;
													}
											} break;
										case PAGE_TRACK_EFFECT:
											switch (button)
											{
												case WHEEL_UP: case WHEEL_UP_CTRL:     effectPgUp(cd->effect, 1); break;
												case WHEEL_DOWN: case WHEEL_DOWN_CTRL: effectPgDn(cd->effect, 1); break;
												default: mouseControls(&cc, button, x, y); break;
											} break;
									} p->redraw = 1; break;
							} break;
					} break;
				/* alt+numbers, change step */
				case '0': w->step = 0; p->redraw = 1; break;
				case '1': w->step = 1; p->redraw = 1; break;
				case '2': w->step = 2; p->redraw = 1; break;
				case '3': w->step = 3; p->redraw = 1; break;
				case '4': w->step = 4; p->redraw = 1; break;
				case '5': w->step = 5; p->redraw = 1; break;
				case '6': w->step = 6; p->redraw = 1; break;
				case '7': w->step = 7; p->redraw = 1; break;
				case '8': w->step = 8; p->redraw = 1; break;
				case '9': w->step = 9; p->redraw = 1; break;
				default: /* escape */
					i = 0; /* silly way to finagle an exit status out of a case switch */
					switch (w->page)
					{
						case PAGE_TRACK_VARIANT: i = inputTrackVariantEscape(input); break;
						// case PAGE_TRACK_EFFECT:  i = inputTrackEffectEscape(cd, input); break;
					} if (i) break;

					previewNote(' ', INST_VOID);
					cc.mouseadjust = cc.keyadjust = 0;
					if (w->page == PAGE_TRACK_VARIANT)
						switch (w->mode)
						{
							case T_MODE_VISUALREPLACE: w->mode = T_MODE_VISUAL; break;
							case T_MODE_VISUAL: case T_MODE_VISUALLINE: w->mode = T_MODE_NORMAL; break;
							default: w->keyboardmacro = '\0'; w->mode = T_MODE_NORMAL; break;
						}
					p->redraw = 1; break;
			} break;
		default:
			if (w->chord == 'I')
			{ /* macro insert edge case */
				changeMacro(input, &w->keyboardmacro, &w->keyboardmacroalt, 0);
				w->mode = T_MODE_INSERT;
			} else if (w->chord)
			{
				p->redraw = 1;
				if (inputTooltip(&tt, input)) return;
			} else
			{
				if (w->mode != T_MODE_INSERT && w->mode != T_MODE_VISUALREPLACE)
					switch (input) /* handle counts and insrument macros if allowed */
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

						case 'b': /* bpm                */ if (w->count) { s->songbpm = MIN(255, MAX(32, w->count)); reapplyBpm(); } w->count = 0; p->redraw = 1; return;
						// case 'f': /* toggle inst browsr */ w->instrument = emptyInstrument(0); chordAddSample(0); return;
						// case 'r': /* record inst        */ setChordRecord(); p->redraw = 1; return;
						// case 'a': /* add empty inst     */ w->instrument = emptyInstrument(0); setChordAddInst(); p->redraw = 1; return;
						// case 'e': /* add empty inst     */                                     setChordAddInst(); p->redraw = 1; return;
					}

				switch (w->page)
				{
					case PAGE_TRACK_VARIANT: if (inputTrackVariant(input)) return; break;
					case PAGE_TRACK_EFFECT:  if (inputTrackEffect(cd, input)) return; break;
				}
			} break;
	}
	if (w->count) { w->count = 0; p->redraw = 1; }
	if (w->chord) { w->chord = '\0'; clearTooltip(&tt); p->redraw = 1; }
}
