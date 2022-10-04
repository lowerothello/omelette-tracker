#include "variantinput.c"

void trackerInput(int input)
{
	int button, x, y, i;
	ChannelData *cd = &s->channelv[w->channel].data;
	switch (input)
	{
		case '\033': /* escape */
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
					previewNote(' ', INST_VOID);
					switch (getchar())
					{
						case '[':
							switch (getchar())
							{
								case 'A': /* linux f1 */ showTracker(); break;
								case 'B': /* linux f2 */ showInstrument(); break;
								case 'D': /* linux f4 */ showMaster(); break;
								case 'E': /* linux f5 */ leaveSpecialModes(); startPlayback(); break;
							} p->dirty = 1; break;
						case 'A': /* up arrow    */ trackerUpArrow  (1); p->dirty = 1; break;
						case 'B': /* down arrow  */ trackerDownArrow(1); p->dirty = 1; break;
						case 'D': /* left arrow  */ trackerLeftArrow (); p->dirty = 1; break;
						case 'C': /* right arrow */ trackerRightArrow(); p->dirty = 1; break;
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
												case 'D': /* left  */ channelLeft (); p->dirty = 1; break;
												case 'C': /* right */ channelRight(); p->dirty = 1; break;
												case 'A': /* up    */ cycleUp     (); p->dirty = 1; break;
												case 'B': /* down  */ cycleDown   (); p->dirty = 1; break;
											} break;
										default: getchar(); break;
									} break;
								case '~': /* linux home */ trackerHome(); p->dirty = 1; break;
							} break;
						case 'H': /* xterm home */ trackerHome(); p->dirty = 1; break;
						case '4': /* end        */ if (getchar() == '~') { trackerEnd(); p->dirty = 1; } break;
						case '5': /* page up    */ trackerUpArrow  (s->rowhighlight); getchar(); p->dirty = 1; break;
						case '6': /* page down  */ trackerDownArrow(s->rowhighlight); getchar(); p->dirty = 1; break;
						case 'M': /* mouse */
							button = getchar();
							x = getchar() - 32;
							y = getchar() - 32;

							short oldtrackerfx = w->trackerfx;
							uint8_t oldchannel = w->channel;

							short tx = 1 + CHANNEL_LINENO_COLS + 2 + genSfx();

							switch (button)
							{
								case BUTTON2_HOLD: case BUTTON2_HOLD_CTRL:
								case BUTTON3_HOLD: case BUTTON3_HOLD_CTRL:
									break;
								case BUTTON_RELEASE: case BUTTON_RELEASE_CTRL:
									if (w->channeloffset < 0)
									{
										if (-w->channeloffset > w->channel) w->channel = 0;
										else                                w->channel += w->channeloffset;
									} else
									{
										if (w->channeloffset > s->channelc-1 - w->channel) w->channel = s->channelc-1;
										else                                               w->channel += w->channeloffset;
									}
									if (w->fyoffset < 0)
									{
										if (-w->fyoffset > w->trackerfy) w->trackerfy = 0;
										else                             w->trackerfy += w->fyoffset;
									} else
									{
										if (w->fyoffset > s->songlen-1 - w->trackerfy) w->trackerfy = s->songlen-1;
										else                                           w->trackerfy += w->fyoffset;
									}
									w->fyoffset = w->channeloffset = w->fieldpointer = 0;

									switch (w->mode)
									{ /* leave mouseadjust mode */
										case T_MODE_MOUSEADJUST: case T_MODE_VTRIG_MOUSEADJUST:
											w->mode = w->oldmode; break;
									} p->dirty = 1;
									/* falls through intentionally */
								default:
									switch (w->page)
									{
										case PAGE_CHANNEL_VARIANT:
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
															if      (x > w->mousex) { trackerAdjustRight(); regenGlobalRowc(s); }
															else if (x < w->mousex) { trackerAdjustLeft (); regenGlobalRowc(s); }
															break;
														case T_MODE_VTRIG_MOUSEADJUST:
															if (!s->playing)
															{
																if (x > w->mousex)
																{
																	if (w->fieldpointer) setChannelTrig(cd, w->trackerfy, cd->trig[w->trackerfy].index+16);
																	else                 setChannelTrig(cd, w->trackerfy, cd->trig[w->trackerfy].index+1);
																	regenGlobalRowc(s);
																}
																else if (x < w->mousex)
																{
																	if (w->fieldpointer) setChannelTrig(cd, w->trackerfy, cd->trig[w->trackerfy].index-16);
																	else                 setChannelTrig(cd, w->trackerfy, cd->trig[w->trackerfy].index-1);
																	regenGlobalRowc(s);
																}
															} break;
													} w->mousex = x; break;
												default: /* click */
													if (y == CHANNEL_ROW-2)
													{
														switch (w->page)
														{ /* hacky implementation */
															case PAGE_CHANNEL_VARIANT: if (x >= ((ws.ws_col-17)>>1) + 9) showTracker(); break;
															case PAGE_CHANNEL_EFFECT:  if (x <  ((ws.ws_col-17)>>1) + 9) showTracker(); break;
														} break;
													} else if (y < CHANNEL_ROW-2)
													{
														if (x >= ((ws.ws_col-17)>>1) + 7) showInstrument();
														break;
													} else if (y <= CHANNEL_ROW)
													{
														for (i = 0; i < s->channelc; i++)
														{
															tx += CHANNEL_TRIG_COLS;
															tx += 8 + 4*(s->channelv[i].data.macroc+1);
															if (tx > x)
															{
																switch (button)
																{
																	case BUTTON1: case BUTTON1_CTRL: w->channeloffset = i - w->channel; break;
																	case BUTTON2: case BUTTON2_CTRL: /* TODO: channel solo */ break;
																	case BUTTON3: case BUTTON3_CTRL: toggleChannelMute(i); break;
																} break;
															}
														} break;
													}

													if (y > ws.ws_row - 1) break; /* ignore clicking out of range */
													w->follow = 0;

													for (i = 0; i < s->channelc; i++)
													{
														tx += CHANNEL_TRIG_COLS;
														if (tx > x) /* clicked on the trig column */
														{
															switch (w->mode)
															{
																case T_MODE_NORMAL:      w->mode = T_MODE_VTRIG; break;
																case T_MODE_INSERT:      w->mode = T_MODE_VTRIG_INSERT; break;
																case T_MODE_MOUSEADJUST: w->mode = T_MODE_VTRIG_MOUSEADJUST; break;
															}
															if (x < tx - 2) w->fieldpointer = 1;
															else            w->fieldpointer = 0;

															switch (button)
															{
																case BUTTON2: case BUTTON2_CTRL:
																	if (w->mode != T_MODE_VTRIG_INSERT) w->mode = T_MODE_VTRIG;
																	setChannelTrig(&s->channelv[i].data, w->trackerfy + y - w->centre, VARIANT_VOID);
																	regenGlobalRowc(s); break;
																case BUTTON1: case BUTTON1_CTRL:
																	if (w->mode != T_MODE_VTRIG_INSERT) w->mode = T_MODE_VTRIG;
																	if (y - w->centre == 0)
																	{
																		w->oldmode = w->mode;
																		w->mode = T_MODE_VTRIG_MOUSEADJUST;
																		w->mousex = x;
																	} break;
																case BUTTON3: case BUTTON3_CTRL:
																	if (w->mode != T_MODE_VTRIG_VISUAL)
																	{
																		w->visualfy = w->trackerfy;
																		w->visualchannel = w->channel;
																		w->mode = T_MODE_VTRIG_VISUAL;
																	} break;
															}

															w->fyoffset = y - w->centre;
															w->channeloffset = i - w->channel;
															break;
														}

														tx += 8 + 4*(s->channelv[i].data.macroc+1);
														if (i == s->channelc-1 || tx > x) /* clicked on the tracker row or out of range */
														{
															switch (w->mode)
															{
																case T_MODE_VTRIG: w->mode = T_MODE_NORMAL; break;
																case T_MODE_VTRIG_INSERT: w->mode = T_MODE_INSERT; break;
																case T_MODE_VTRIG_MOUSEADJUST: w->mode = T_MODE_MOUSEADJUST; break;
															}

															if (button == BUTTON1_CTRL) { w->step = MIN(15, abs(y - w->centre)); break; }

															switch (button)
															{
																case BUTTON1:
																	if (w->mode != T_MODE_INSERT) /* suggest normal mode, but allow insert */
																		w->mode = T_MODE_NORMAL;
																	break;
																case BUTTON3: case BUTTON3_CTRL:
																	if (!(w->mode == T_MODE_VISUAL || w->mode == T_MODE_VISUALLINE || w->mode == T_MODE_VISUALREPLACE))
																	{
																		w->visualfx = tfxToVfx(oldtrackerfx);
																		w->visualfy = w->trackerfy;
																		w->visualchannel = oldchannel;
																		w->mode = T_MODE_VISUAL;
																	} break;
															}
															if (tx-x < 3) /* clicked out of range, on the star column */
															{
																w->trackerfx = 1+((s->channelv[i].data.macroc+1)<<1);
																w->fieldpointer = 0;
															} else if (tx-x < 3 + 4*(s->channelv[i].data.macroc+1)) /* a macro column was clicked */
															{
																w->trackerfx = 1+((s->channelv[i].data.macroc+1)<<1) - (((tx-x - 1)>>1) - 1);
																if (((tx-x - 1) - 1) % 2) w->fieldpointer = 0;
																else                      w->fieldpointer = 1;
															} else if (tx-x < 6 + 4*(s->channelv[i].data.macroc+1))
															{
																w->trackerfx = 1;
																if (tx-x < 4 + 4*(s->channelv[i].data.macroc+1)) w->fieldpointer = 0;
																else                                             w->fieldpointer = 1;
															} else w->trackerfx = 0;

															w->fyoffset = y - w->centre;
															w->channeloffset = i - w->channel;

															if (button == BUTTON2 || button == BUTTON2_CTRL)
															{
																if (w->trackerfx == 0)
																{
																	yankPartPattern(0, 1, w->trackerfy+w->fyoffset, w->trackerfy+w->fyoffset, w->channel+w->channeloffset, w->channel+w->channeloffset);
																	delPartPattern (0, 1, w->trackerfy+w->fyoffset, w->trackerfy+w->fyoffset, w->channel+w->channeloffset, w->channel+w->channeloffset);
																} else
																{
																	yankPartPattern(tfxToVfx(w->trackerfx), tfxToVfx(w->trackerfx), w->trackerfy+w->fyoffset, w->trackerfy+w->fyoffset, w->channel+w->channeloffset, w->channel+w->channeloffset);
																	delPartPattern (tfxToVfx(w->trackerfx), tfxToVfx(w->trackerfx), w->trackerfy+w->fyoffset, w->trackerfy+w->fyoffset, w->channel+w->channeloffset, w->channel+w->channeloffset);
																} break;
															}

															/* enter adjust */
															if ((button == BUTTON1 || button == BUTTON1_CTRL)
																	&& w->fyoffset == 0 && w->trackerfx == oldtrackerfx && w->channeloffset == 0)
															{
																w->oldmode = w->mode;
																w->mode = T_MODE_MOUSEADJUST;
																w->mousex = x;
															} break;
														}
													}
											} break;
										case PAGE_CHANNEL_EFFECT:
											switch (button)
											{
												case WHEEL_UP: case WHEEL_UP_CTRL:     effectCtrlUpArrow  (&cd->effect, 1); break;
												case WHEEL_DOWN: case WHEEL_DOWN_CTRL: effectCtrlDownArrow(&cd->effect, 1); break;
												default:
													if (button != BUTTON1_HOLD && button != BUTTON1_HOLD_CTRL)
													{
														if (y == CHANNEL_ROW-2)
														{
															switch (w->page)
															{ /* hacky implementation */
																case PAGE_CHANNEL_VARIANT: if (x >= ((ws.ws_col-17)>>1) + 9) showTracker(); break;
																case PAGE_CHANNEL_EFFECT:  if (x <  ((ws.ws_col-17)>>1) + 9) showTracker(); break;
															} break;
														} else if (y < CHANNEL_ROW-2)
														{
															if (x >= ((ws.ws_col-17)>>1) + 7) showInstrument();
															break;
														} else if (y <= CHANNEL_ROW)
														{
															for (i = 0; i < s->channelc; i++)
															{
																tx += CHANNEL_TRIG_COLS;
																tx += 8 + 4*(s->channelv[i].data.macroc+1);
																if (tx > x)
																{
																	switch (button)
																	{
																		case BUTTON1: case BUTTON1_CTRL: w->channeloffset = i - w->channel; break;
																		case BUTTON2: case BUTTON2_CTRL: /* TODO: channel solo */ break;
																		case BUTTON3: case BUTTON3_CTRL: toggleChannelMute(i); break;
																	} break;
																}
															} break;
														}
													} mouseControls(&cc, button, x, y);
													break;
											} break;
									} p->dirty = 1; break;
							} break;
					} break;
				/* alt+numbers, change step */
				case '0': w->step = 0; p->dirty = 1; break;
				case '1': w->step = 1; p->dirty = 1; break;
				case '2': w->step = 2; p->dirty = 1; break;
				case '3': w->step = 3; p->dirty = 1; break;
				case '4': w->step = 4; p->dirty = 1; break;
				case '5': w->step = 5; p->dirty = 1; break;
				case '6': w->step = 6; p->dirty = 1; break;
				case '7': w->step = 7; p->dirty = 1; break;
				case '8': w->step = 8; p->dirty = 1; break;
				case '9': w->step = 9; p->dirty = 1; break;
				default: /* escape */
					previewNote(' ', INST_VOID);
					cc.mouseadjust = cc.keyadjust = 0;
					w->page = PAGE_CHANNEL_VARIANT;
					switch (w->mode)
					{
						case T_MODE_VISUALREPLACE: w->mode = T_MODE_VISUAL; break;
						case T_MODE_VISUAL: case T_MODE_VISUALLINE: w->mode = T_MODE_NORMAL; break;
						case T_MODE_VTRIG_INSERT: case T_MODE_VTRIG_VISUAL: w->mode = T_MODE_VTRIG; break;
						case T_MODE_VTRIG: break;
						default: w->keyboardmacro = '\0'; w->mode = T_MODE_NORMAL; break;
					} p->dirty = 1; break;
			} break;
		default:
			switch (w->page)
			{
				case PAGE_CHANNEL_VARIANT: if (inputChannelVariant(input)) return; break;
				case PAGE_CHANNEL_EFFECT:  if (inputEffect(&cd->effect, input)) return; break;
			}
	}
	if (w->count) { w->count = 0; p->dirty = 1; }
	if (w->chord) { w->chord = '\0'; clearTooltip(&tt); p->dirty = 1; }
}
