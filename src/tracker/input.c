void trackerInput(int input)
{
	int button, x, y, i;
	short macro;
	row *r;
	channeldata *cd = &s->channelv[w->channel].data;
	switch (input)
	{
		case '\033': /* escape */
			switch (getchar())
			{
				case 'O':
					previewNote(' ', INST_VOID, w->channel);
					switch (getchar())
					{
						case 'P': /* xterm f1 */ showTracker(); break;
						case 'Q': /* xterm f2 */ showInstrument(); break;
					} redraw(); break;
				case '[':
					previewNote(' ', INST_VOID, w->channel);
					switch (getchar())
					{
						case '[':
							switch (getchar())
							{
								case 'A': /* linux f1 */ showTracker(); break;
								case 'B': /* linux f2 */ showInstrument(); break;
								case 'E': /* linux f5 */ leaveSpecialModes(); startPlayback(); break;
							} redraw(); break;
						case 'A': /* up arrow    */ trackerUpArrow(1); redraw(); break;
						case 'B': /* down arrow  */ trackerDownArrow(1); redraw(); break;
						case 'D': /* left arrow  */ trackerLeftArrow(); redraw(); break;
						case 'C': /* right arrow */ trackerRightArrow(); redraw(); break;
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
												case 'D': /* left  */ channelLeft(); redraw(); break;
												case 'C': /* right */ channelRight(); redraw(); break;
												case 'A': /* up    */ cycleUp(); redraw(); break;
												case 'B': /* down  */ cycleDown(); redraw(); break;
											} break;
										default: getchar(); break;
									} break;
								case '~': /* linux home */ trackerHome(); redraw(); break;
							} break;
						case 'H': /* xterm home */ trackerHome(); redraw(); break;
						case '4': /* end        */ if (getchar() == '~') { trackerEnd(); redraw(); } break;
						case '5': /* page up    */ trackerUpArrow(s->rowhighlight); getchar(); redraw(); break;
						case '6': /* page down  */ trackerDownArrow(s->rowhighlight); getchar(); redraw(); break;
						case 'M': /* mouse */
							button = getchar();
							x = getchar() - 32;
							y = getchar() - 32;

							short oldtrackerfx = w->trackerfx;
							uint8_t oldchannel = w->channel;

							short sfx, tx;
							switch (button)
							{
								case WHEEL_UP: case WHEEL_UP_CTRL:
									trackerUpArrow(WHEEL_SPEED);
									redraw(); break;
								case WHEEL_DOWN: case WHEEL_DOWN_CTRL:
									trackerDownArrow(WHEEL_SPEED);
									redraw(); break;
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
									} redraw(); break;
								case BUTTON1_HOLD:
									if (w->flags&W_FLAG_FOLLOW) w->flags ^= W_FLAG_FOLLOW;
									switch (w->mode)
									{
										case T_MODE_MOUSEADJUST:
											if      (x > w->mousex) { trackerAdjustRight(); regenGlobalRowc(s); redraw(); }
											else if (x < w->mousex) { trackerAdjustLeft (); regenGlobalRowc(s); redraw(); }
											break;
										case T_MODE_VTRIG_MOUSEADJUST:
											if (!s->playing)
											{
												if (x > w->mousex)
												{
													if (w->fieldpointer) setChannelTrig(cd, w->trackerfy, cd->trig[w->trackerfy].index+16);
													else                 setChannelTrig(cd, w->trackerfy, cd->trig[w->trackerfy].index+1);
													regenGlobalRowc(s); redraw();
												}
												else if (x < w->mousex)
												{
													if (w->fieldpointer) setChannelTrig(cd, w->trackerfy, cd->trig[w->trackerfy].index-16);
													else                 setChannelTrig(cd, w->trackerfy, cd->trig[w->trackerfy].index-1);
													regenGlobalRowc(s); redraw();
												}
											} break;
									} w->mousex = x; break;
								default: /* click */
									if (y <= CHANNEL_ROW-2)
									{
										if (x < (ws.ws_col-17) / 2 + 7) showTracker();
										else                            showInstrument();
										break;
									}
									if (y > ws.ws_row - 1) break; /* ignore clicking out of range */
									if (w->flags&W_FLAG_FOLLOW) w->flags ^= W_FLAG_FOLLOW;

									sfx = tx = 0; /* need to know sfx for screen scroll */
									for (i = 0; i < s->channelc; i++)
									{
										tx += 3 + 9 + 4*(s->channelv[i].data.macroc+1);
										if (i == w->channel)
										{
											sfx = tx;
											break;
										}
									} sfx = MIN(0, (ws.ws_col>>1) - sfx);
									tx = 1 + (LINENO_COLS-3) + 2; /* drawGlobalLineNumbers() */
									tx += sfx; /* could probably remove the sfx var and do it all in one go */

									for (i = 0; i < s->channelc; i++)
									{
										tx += 4; /* drawChannelTrigs() */
										if (tx > x) /* clicked on the trig column */
										{
											if (y <= CHANNEL_ROW) { toggleChannelMute(i); break; }

											switch (w->mode)
											{
												case T_MODE_NORMAL: w->mode = T_MODE_VTRIG; break;
												case T_MODE_INSERT: w->mode = T_MODE_VTRIG_INSERT; break;
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
											if (y <= CHANNEL_ROW) { toggleChannelMute(i); break; }

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
									} redraw();
							} break;
					} break;
				/* alt+numbers, change step */
				case '0': w->step = 0; redraw(); break;
				case '1': w->step = 1; redraw(); break;
				case '2': w->step = 2; redraw(); break;
				case '3': w->step = 3; redraw(); break;
				case '4': w->step = 4; redraw(); break;
				case '5': w->step = 5; redraw(); break;
				case '6': w->step = 6; redraw(); break;
				case '7': w->step = 7; redraw(); break;
				case '8': w->step = 8; redraw(); break;
				case '9': w->step = 9; redraw(); break;
				default: /* escape */
					previewNote(' ', INST_VOID, w->channel);
					switch (w->mode)
					{
						case T_MODE_VISUALREPLACE: w->mode = T_MODE_VISUAL; break;
						case T_MODE_VISUAL: case T_MODE_VISUALLINE: w->mode = T_MODE_NORMAL; break;
						case T_MODE_VTRIG_INSERT: case T_MODE_VTRIG_VISUAL: w->mode = T_MODE_VTRIG; break;
						case T_MODE_VTRIG: break;
						default: w->keyboardmacro = '\0'; w->mode = T_MODE_NORMAL; break;
					} redraw(); break;
			} break;
		default:
			switch (w->mode)
			{
				case T_MODE_VISUALREPLACE:
					if (input == '\n' || input == '\r')
					{
						toggleChannelMute(w->channel); redraw();
					} else
					{
						switch (w->trackerfx)
						{
							case 0: /* note */
								for (i = MIN(w->trackerfy, w->visualfy); i <= MAX(w->trackerfy, w->visualfy); i++)
								{
									r = getChannelRow(cd, i);
									insertNote(r, input);
								}
								/* <preview> */
								r = getChannelRow(cd, w->trackerfy);
								previewNote(input, r->inst, w->channel);
								/* </preview> */
								break;
							case 1: /* inst */
								for (i = MIN(w->trackerfy, w->visualfy); i <= MAX(w->trackerfy, w->visualfy); i++)
								{
									r = getChannelRow(cd, i);
									insertInst(r, input);
								} break;
							default: /* macro */
								macro = (w->trackerfx - 2)>>1;
								if (!(w->trackerfx%2))
								{ /* macroc */
									for (i = MIN(w->trackerfy, w->visualfy); i <= MAX(w->trackerfy, w->visualfy); i++)
									{
										r = getChannelRow(cd, i);
										insertMacroc(r, macro, input);
									}
								} else
								{ /* macrov */
									for (i = MIN(w->trackerfy, w->visualfy); i <= MAX(w->trackerfy, w->visualfy); i++)
									{
										r = getChannelRow(cd, i);
										insertMacrov(r, macro, input);
									}
								} break;
						} regenGlobalRowc(s); redraw(); break;
					}
				case T_MODE_VISUAL: case T_MODE_VISUALLINE:
					switch (input)
					{
						case '\n': case '\r': toggleChannelMute(w->channel); redraw(); break;
						case 'v': case 22: /* visual */
							switch (w->mode)
							{
								case T_MODE_VISUAL:     w->mode = T_MODE_NORMAL; redraw(); break;
								case T_MODE_VISUALLINE: w->mode = T_MODE_VISUAL; redraw(); break;
							} break;
						case 'V': /* visual line */
							switch (w->mode)
							{
								case T_MODE_VISUAL:     w->mode = T_MODE_VISUALLINE; redraw(); break;
								case T_MODE_VISUALLINE: w->mode = T_MODE_NORMAL; redraw(); break;
							} break;
						case 'r': /* replace     */ w->mode = T_MODE_VISUALREPLACE; redraw(); break;;
						case 'k': /* up arrow    */ trackerUpArrow(1); redraw(); break;
						case 'j': /* down arrow  */ trackerDownArrow(1); redraw(); break;
						case 'h': /* left arrow  */ trackerLeftArrow(); redraw(); break;
						case 'l': /* right arrow */ trackerRightArrow(); redraw(); break;
						case '[': /* chnl left   */ channelLeft(); redraw(); break;
						case ']': /* chnl right  */ channelRight(); redraw(); break;
						case '{': /* cycle up    */ cycleUp(); redraw(); break;
						case '}': /* cycle down  */ cycleDown(); redraw(); break;
						case '~': /* vi tilde */
							switch (w->mode)
							{
								case T_MODE_VISUAL:     tildePartPattern(MIN(tfxToVfx(w->trackerfx), w->visualfx), MAX(tfxToVfx(w->trackerfx), w->visualfx), MIN(w->trackerfy, w->visualfy), MAX(w->trackerfy, w->visualfy), MIN(w->channel, w->visualchannel), MAX(w->channel, w->visualchannel)); redraw(); break;
								case T_MODE_VISUALLINE: tildePartPattern(0, 2+cd->macroc, MIN(w->trackerfy, w->visualfy), MAX(w->trackerfy, w->visualfy), MIN(w->channel, w->visualchannel), MAX(w->channel, w->visualchannel)); redraw(); break;
							} break;
						case 'i': /* interpolate */
							switch (w->mode)
							{
								case T_MODE_VISUAL:     interpolatePartPattern(MIN(tfxToVfx(w->trackerfx), w->visualfx), MAX(tfxToVfx(w->trackerfx), w->visualfx), MIN(w->trackerfy, w->visualfy), MAX(w->trackerfy, w->visualfy), MIN(w->channel, w->visualchannel), MAX(w->channel, w->visualchannel)); redraw(); break;
								case T_MODE_VISUALLINE: interpolatePartPattern(0, 2+cd->macroc, MIN(w->trackerfy, w->visualfy), MAX(w->trackerfy, w->visualfy), MIN(w->channel, w->visualchannel), MAX(w->channel, w->visualchannel)); redraw(); break;
							} regenGlobalRowc(s); break;
						case '%': /* random */
							switch (w->mode)
							{
								case T_MODE_VISUAL:     randPartPattern(MIN(tfxToVfx(w->trackerfx), w->visualfx), MAX(tfxToVfx(w->trackerfx), w->visualfx), MIN(w->trackerfy, w->visualfy), MAX(w->trackerfy, w->visualfy), MIN(w->channel, w->visualchannel), MAX(w->channel, w->visualchannel)); redraw(); break;
								case T_MODE_VISUALLINE: randPartPattern(0, 2+cd->macroc, MIN(w->trackerfy, w->visualfy), MAX(w->trackerfy, w->visualfy), MIN(w->channel, w->visualchannel), MAX(w->channel, w->visualchannel)); redraw(); break;
							} regenGlobalRowc(s); break;
						case 1: /* ^a */
							switch (w->mode)
							{
								case T_MODE_VISUAL:     addPartPattern(MAX(1, w->count), MIN(tfxToVfx(w->trackerfx), w->visualfx), MAX(tfxToVfx(w->trackerfx), w->visualfx), MIN(w->trackerfy, w->visualfy), MAX(w->trackerfy, w->visualfy), MIN(w->channel, w->visualchannel), MAX(w->channel, w->visualchannel)); redraw(); break;
								case T_MODE_VISUALLINE: addPartPattern(MAX(1, w->count), 0, 2+cd->macroc, MIN(w->trackerfy, w->visualfy), MAX(w->trackerfy, w->visualfy), MIN(w->channel, w->visualchannel), MAX(w->channel, w->visualchannel)); redraw(); break;
							} regenGlobalRowc(s); break;
						case 24: /* ^x */
							switch (w->mode)
							{
								case T_MODE_VISUAL:     addPartPattern(-MAX(1, w->count), MIN(tfxToVfx(w->trackerfx), w->visualfx), MAX(tfxToVfx(w->trackerfx), w->visualfx), MIN(w->trackerfy, w->visualfy), MAX(w->trackerfy, w->visualfy), MIN(w->channel, w->visualchannel), MAX(w->channel, w->visualchannel)); redraw(); break;
								case T_MODE_VISUALLINE: addPartPattern(-MAX(1, w->count), 0, 2+cd->macroc, MIN(w->trackerfy, w->visualfy), MAX(w->trackerfy, w->visualfy), MIN(w->channel, w->visualchannel), MAX(w->channel, w->visualchannel)); redraw(); break;
							} regenGlobalRowc(s); break;
						case 'd': case 'x': case 127: case '\b': /* pattern cut */
							switch (w->mode)
							{
								case T_MODE_VISUAL:
									yankPartPattern(MIN(tfxToVfx(w->trackerfx), w->visualfx), MAX(tfxToVfx(w->trackerfx), w->visualfx), MIN(w->trackerfy, w->visualfy), MAX(w->trackerfy, w->visualfy), MIN(w->channel, w->visualchannel), MAX(w->channel, w->visualchannel));
									delPartPattern (MIN(tfxToVfx(w->trackerfx), w->visualfx), MAX(tfxToVfx(w->trackerfx), w->visualfx), MIN(w->trackerfy, w->visualfy), MAX(w->trackerfy, w->visualfy), MIN(w->channel, w->visualchannel), MAX(w->channel, w->visualchannel));
									break;
								case T_MODE_VISUALLINE:
									yankPartPattern(0, 2+cd->macroc, MIN(w->trackerfy, w->visualfy), MAX(w->trackerfy, w->visualfy), MIN(w->channel, w->visualchannel), MAX(w->channel, w->visualchannel));
									delPartPattern (0, 2+cd->macroc, MIN(w->trackerfy, w->visualfy), MAX(w->trackerfy, w->visualfy), MIN(w->channel, w->visualchannel), MAX(w->channel, w->visualchannel));
									break;
							}
							w->trackerfx = MIN(w->trackerfx, vfxToTfx(w->visualfx));
							w->trackerfy = MIN(w->trackerfy, w->visualfy);
							w->channel = MIN(w->channel, w->visualchannel);
							w->mode = T_MODE_NORMAL;
							regenGlobalRowc(s); redraw(); break;
						case 'y': /* pattern copy */
							switch (w->mode)
							{
								case T_MODE_VISUAL:     yankPartPattern(MIN(tfxToVfx(w->trackerfx), w->visualfx), MAX(tfxToVfx(w->trackerfx), w->visualfx), MIN(w->trackerfy, w->visualfy), MAX(w->trackerfy, w->visualfy), MIN(w->channel, w->visualchannel), MAX(w->channel, w->visualchannel)); break;
								case T_MODE_VISUALLINE: yankPartPattern(0, 2+cd->macroc, MIN(w->trackerfy, w->visualfy), MAX(w->trackerfy, w->visualfy), MIN(w->channel, w->visualchannel), MAX(w->channel, w->visualchannel)); break;
							}
							w->trackerfx = MIN(w->trackerfx, vfxToTfx(w->visualfx));
							w->trackerfy = MIN(w->trackerfy, w->visualfy);
							w->channel = MIN(w->channel, w->visualchannel);
							w->mode = T_MODE_NORMAL;
							redraw(); break;
					} break;
				case T_MODE_VTRIG:
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
									case 'I': /* macro insert */ changeMacro(input, &w->keyboardmacro); w->mode = T_MODE_INSERT; break;
									case 'd': if (input == 'd') /* delete */
										{
											yankPartVtrig(w->trackerfy, w->trackerfy+MAX(1, w->count)-1, w->channel, w->channel);
											delPartVtrig (w->trackerfy, w->trackerfy+MAX(1, w->count)-1, w->channel, w->channel);
											trackerDownArrow(MAX(1, w->count));
											regenGlobalRowc(s); redraw();
										} break;
									case 'y': if (input == 'y') /* yank */
										{
											yankPartVtrig(w->trackerfy, w->trackerfy+MAX(1, w->count)-1, w->channel, w->channel);
											trackerDownArrow(MAX(1, w->count));
											redraw();
										} break;
									case 'g': /* graphic */ if (input == 'g') w->trackerfy = 0; redraw(); break;
									case 'r': /* row     */ chordRow(cd, input); regenGlobalRowc(s); redraw(); break;
									case ';': /* loop    */ chordLoop(cd, input); redraw(); break;
								} w->count = 0; redraw();
							} else
								switch (input)
								{
									case '\t': /* leave vtrig mode   */ w->mode = T_MODE_NORMAL; redraw(); break;
									case 'f':  /* toggle song follow */ w->flags ^= W_FLAG_FOLLOW; if (s->playing) w->trackerfy = s->playfy; redraw(); break;
									case 'k':  /* up arrow           */ trackerUpArrow(1); redraw(); break;
									case 'j':  /* down arrow         */ trackerDownArrow(1); redraw(); break;
									case 'h':  /* left arrow         */ trackerLeftArrow(); redraw(); break;
									case 'l':  /* right arrow        */ trackerRightArrow(); redraw(); break;
									case '[':  /* chnl left          */ channelLeft(); redraw(); break;
									case ']':  /* chnl right         */ channelRight(); redraw(); break;
									case '{':  /* cycle up           */ cycleUp(); redraw(); break;
									case '}':  /* cycle down         */ cycleDown(); redraw(); break;
									case 'b':  /* bpm                */ if (w->count) s->songbpm = MIN(255, MAX(32, w->count)); w->request = REQ_BPM; redraw(); break;
									case 't':  /* row highlight      */ if (w->count) s->rowhighlight = MIN(16, w->count); regenGlobalRowc(s); redraw(); break;
									case 's':  /* step               */ w->step = MIN(15, w->count); redraw(); break;
									case 'o':  /* octave             */ w->octave = MIN(9, w->count); redraw(); break;
									case 'r':  /* row                */ w->chord = 'r'; redraw(); return;
									case ';':  /* loop               */ w->chord = ';'; redraw(); return;
									case 'g':  /* graphic misc       */ w->chord = 'g'; redraw(); return;
									case 'G':  /* graphic end        */ trackerEnd(); redraw(); break;
									case 'y':  /* copy               */ w->chord = 'y'; redraw(); return;
									case 'd':  /* cut                */ w->chord = 'd'; redraw(); return;
									case 'I':          /* macro insert mode */ w->chord = 'I'; redraw(); return;
									case 'i':          /* enter insert mode */ w->mode = T_MODE_VTRIG_INSERT; redraw(); break;
									case 'v': case 22: /* enter visual mode */
										w->visualfy = w->trackerfy;
										w->visualchannel = w->channel;
										w->mode = T_MODE_VTRIG_VISUAL;
										redraw(); break;
									case 1:  /* ^a */         setChannelTrig(cd, w->trackerfy, cd->trig[w->trackerfy].index+MAX(1, w->count)-1); regenGlobalRowc(s); redraw(); break;
									case 24: /* ^x */         setChannelTrig(cd, w->trackerfy, cd->trig[w->trackerfy].index-MAX(1, w->count)+1); regenGlobalRowc(s); redraw(); break;
									case 'a': /* add empty */ setChannelTrig(cd, w->trackerfy, _getEmptyVariantIndex(cd, cd->trig[w->trackerfy].index)); regenGlobalRowc(s); redraw(); break;
									/* clone is broken currently */
									// case 'c': /* clone */     setChannelTrig(cd, w->trackerfy, duplicateVariant(cd, cd->trig[w->trackerfy].index)); redraw(); break;
									case 'p': /* pattern put */ /* TODO: count */
										putPartVtrig();
										if (w->vbchannelc)
											trackerDownArrow(w->vbrowc);
										regenGlobalRowc(s); redraw(); break;
									case 'P': /* vtrig put before */ mixPutPartVtrig(); regenGlobalRowc(s); redraw(); break;
									case 'x': case 127: case '\b': /* vtrig delete */
										yankPartVtrig (w->trackerfy, w->trackerfy, w->channel, w->channel);
										setChannelTrig(cd, w->trackerfy, VARIANT_VOID);
										regenGlobalRowc(s); redraw(); break;
									case '.': /* vtrig loop */
										i = getPrevVtrig(cd, w->trackerfy);
										if (i != -1)
										{
											cd->trig[i].flags ^= C_VTRIG_LOOP;
											regenGlobalRowc(s); redraw();
										} break;
								} break;
					} break;
				case T_MODE_NORMAL:
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
									case 'd': if (input == 'd') /* delete */
										{
											yankPartPattern(0, 2+cd->macroc, w->trackerfy, w->trackerfy+MAX(1, w->count)-1, w->channel, w->channel);
											delPartPattern (0, 2+cd->macroc, w->trackerfy, w->trackerfy+MAX(1, w->count)-1, w->channel, w->channel);
											trackerDownArrow(MAX(1, w->count));
											regenGlobalRowc(s); redraw();
										} break;
									case 'y': if (input == 'y') /* yank */
										{
											yankPartPattern(0, 2+cd->macroc, w->trackerfy, w->trackerfy+MAX(1, w->count)-1, w->channel, w->channel);
											trackerDownArrow(MAX(1, w->count));
											redraw();
										} break;
									case 'c': /* channel      */ chordChannel(cd, input); regenGlobalRowc(s); resize(0); break;
									case 'm': /* macro        */ chordMacro(cd, input); regenGlobalRowc(s); resize(0); break;
									case 'I': /* macro insert */ changeMacro(input, &w->keyboardmacro); w->mode = T_MODE_INSERT; redraw(); break;
									case 'g': /* graphic      */ if (input == 'g') w->trackerfy = 0; redraw(); break;
									case 'r': /* row          */ chordRow(cd, input); regenGlobalRowc(s); redraw(); break;
									case ';': /* loop         */ chordLoop(cd, input); redraw(); break;
								} w->count = 0;
							} else
							{
								r = getChannelRow(cd, w->trackerfy);
								switch (input)
								{
									case '\n': case '\r': toggleChannelMute(w->channel); redraw(); break;
									case 'f': /* toggle song follow */ w->flags ^= W_FLAG_FOLLOW; if (s->playing) w->trackerfy = s->playfy; redraw(); break;
									case 'I': /* macro insert mode  */ w->chord = 'I'; redraw(); return;
									case 'i': /* enter insert mode  */ w->mode = T_MODE_INSERT; redraw(); break;
									// case 'u': /* undo               */ popPatternHistory(s->patterni[s->trig[w->songfy].index]); redraw(); break;
									// case 18:  /* redo               */ unpopPatternHistory(s->patterni[s->trig[w->songfy].index]); redraw(); break;
									case 'k': /* up arrow           */ trackerUpArrow(1); redraw(); break;
									case 'j': /* down arrow         */ trackerDownArrow(1); redraw(); break;
									case 'h': /* left arrow         */ trackerLeftArrow(); redraw(); break;
									case 'l': /* right arrow        */ trackerRightArrow(); redraw(); break;
									case '[': /* chnl left          */ channelLeft(); redraw(); break;
									case ']': /* chnl right         */ channelRight(); redraw(); break;
									case '{': /* cycle up           */ cycleUp(); redraw(); break;
									case '}': /* cycle down         */ cycleDown(); redraw(); break;
									case 'v': case 22: /* enter visual mode */
										w->visualfx = tfxToVfx(w->trackerfx);
										w->visualfy = w->trackerfy;
										w->visualchannel = w->channel;
										w->mode = T_MODE_VISUAL;
										redraw(); break;
									case 'V': /* enter visual line mode */
										w->visualfx = tfxToVfx(w->trackerfx);
										w->visualfy = w->trackerfy;
										w->visualchannel = w->channel;
										w->mode = T_MODE_VISUALLINE;
										redraw(); break;
									case '\t': /* enter vtrig mode */ w->mode = T_MODE_VTRIG; redraw(); break;
									case 'y':  /* pattern copy     */ w->chord = 'y'; redraw(); return;
									case 'd':  /* pattern cut      */ w->chord = 'd'; redraw(); return;
									case 'c':  /* channel          */ w->chord = 'c'; redraw(); return;
									case 'm':  /* macro            */ w->chord = 'm'; redraw(); return;
									case 'r':  /* row              */ w->chord = 'r'; redraw(); return;
									case ';':  /* loop             */ w->chord = ';'; redraw(); return;
									case 'g':  /* graphic misc     */ w->chord = 'g'; redraw(); return;
									case 'G':  /* graphic end      */ trackerEnd(); redraw(); break;
									case 'b':  /* bpm              */ if (w->count) s->songbpm = MIN(255, MAX(32, w->count)); w->request = REQ_BPM; redraw(); break;
									case 't':  /* row highlight    */ if (w->count) s->rowhighlight = MIN(16, w->count); regenGlobalRowc(s); redraw(); break;
									case 's':  /* step             */ w->step = MIN(15, w->count); redraw(); break;
									case 'o':  /* octave           */
										if (!w->trackerfx) r->note = changeNoteOctave(MIN(9, w->count), r->note);
										else               w->octave = MIN(9, w->count);
										redraw(); break;
									case 'p':  /* pattern put */ /* TODO: count */
										putPartPattern();
										if (w->pbchannelc)
											trackerDownArrow(w->pbvariantv[0]->rowc);
										regenGlobalRowc(s); redraw(); break;
									case 'P': /* pattern put before */
										mixPutPartPattern();
										regenGlobalRowc(s); redraw(); break;
									case 'x': case 127: case '\b': /* backspace */
										if (w->trackerfx == 0)
										{
											yankPartPattern(0, 1, w->trackerfy, w->trackerfy, w->channel, w->channel);
											delPartPattern (0, 1, w->trackerfy, w->trackerfy, w->channel, w->channel);
										} else
										{
											yankPartPattern(tfxToVfx(w->trackerfx), tfxToVfx(w->trackerfx), w->trackerfy, w->trackerfy, w->channel, w->channel);
											delPartPattern (tfxToVfx(w->trackerfx), tfxToVfx(w->trackerfx), w->trackerfy, w->trackerfy, w->channel, w->channel);
										} regenGlobalRowc(s); redraw(); break;
									case '%': /* random */
										randPartPattern(tfxToVfx(w->trackerfx), tfxToVfx(w->trackerfx), w->trackerfy, w->trackerfy, w->channel, w->channel);
										regenGlobalRowc(s); redraw(); break;
									default: /* column specific */
										switch (w->trackerfx)
										{
											case 0: /* note */
												switch (input)
												{
													case 1:  /* ^a */ r->note+=MAX(1, w->count); break;
													case 24: /* ^x */ r->note-=MAX(1, w->count); break;
												} break;
											case 1: /* instrument */
												switch (input)
												{
													case 1: /* ^a */
														r->inst+=MAX(1, w->count);
														if (w->instrumentrecv == INST_REC_LOCK_OK) w->instrument = r->inst;
														break;
													case 24: /* ^x */
														r->inst-=MAX(1, w->count);
														if (w->instrumentrecv == INST_REC_LOCK_OK) w->instrument = r->inst;
														break;
												} break;
											default:
												macro = (w->trackerfx - 2)>>1;
												switch (input)
												{
													case 1:  /* ^a */
														switch (r->macro[macro].c)
														{
															case 'G': case 'g': case 'K': case 'k': r->macro[macro].v += MAX(1, w->count)*16;
															default:                                r->macro[macro].v += MAX(1, w->count);
														} break;
													case 24: /* ^x */
														switch (r->macro[macro].c)
														{
															case 'G': case 'g': case 'K': case 'k': r->macro[macro].v -= MAX(1, w->count)*16;
															default:                                r->macro[macro].v -= MAX(1, w->count);
														} break;
													case '~': /* toggle case */
														if      (isupper(r->macro[macro].c)) changeMacro(r->macro[macro].c, &r->macro[macro].c);
														else if (islower(r->macro[macro].c)) changeMacro(r->macro[macro].c, &r->macro[macro].c);
														break;
												} break;
										} regenGlobalRowc(s); redraw(); break;
								}
							} break;
					} break;
				case T_MODE_VTRIG_VISUAL:
					switch (input)
					{
						case 'v': case 22: w->mode = T_MODE_VTRIG; redraw(); break;
						case 'k': /* up arrow    */ trackerUpArrow(1); redraw(); break;
						case 'j': /* down arrow  */ trackerDownArrow(1); redraw(); break;
						case 'h': /* left arrow  */ trackerLeftArrow(); redraw(); break;
						case 'l': /* right arrow */ trackerRightArrow(); redraw(); break;
						case '[': /* chnl left   */ channelLeft(); redraw(); break;
						case ']': /* chnl right  */ channelRight(); redraw(); break;
						case '{': /* cycle up    */ cycleUp(); redraw(); break;
						case '}': /* cycle down  */ cycleDown(); redraw(); break;
						case 'd': case 'x': case 127: case '\b': /* cut */
							yankPartVtrig(MIN(w->trackerfy, w->visualfy), MAX(w->trackerfy, w->visualfy), MIN(w->channel, w->visualchannel), MAX(w->channel, w->visualchannel));
							delPartVtrig (MIN(w->trackerfy, w->visualfy), MAX(w->trackerfy, w->visualfy), MIN(w->channel, w->visualchannel), MAX(w->channel, w->visualchannel));

							w->trackerfy = MIN(w->trackerfy, w->visualfy);
							w->channel = MIN(w->channel, w->visualchannel);
							w->mode = T_MODE_VTRIG;
							regenGlobalRowc(s); redraw(); break;
						case 'y': /* pattern copy */
							yankPartVtrig(MIN(w->trackerfy, w->visualfy), MAX(w->trackerfy, w->visualfy), MIN(w->channel, w->visualchannel), MAX(w->channel, w->visualchannel));

							w->trackerfy = MIN(w->trackerfy, w->visualfy);
							w->channel = MIN(w->channel, w->visualchannel);
							w->mode = T_MODE_VTRIG;
							redraw(); break;
						case '.': /* vtrig loop */
							loopPartVtrig(MIN(w->trackerfy, w->visualfy), MAX(w->trackerfy, w->visualfy), MIN(w->channel, w->visualchannel), MAX(w->channel, w->visualchannel));
							w->mode = T_MODE_VTRIG;
							regenGlobalRowc(s); redraw(); break;
					} break;
				case T_MODE_VTRIG_INSERT:
					switch (input)
					{
						case '0':           inputChannelTrig(cd, w->trackerfy, 0);  regenGlobalRowc(s); redraw(); break;
						case '1':           inputChannelTrig(cd, w->trackerfy, 1);  regenGlobalRowc(s); redraw(); break;
						case '2':           inputChannelTrig(cd, w->trackerfy, 2);  regenGlobalRowc(s); redraw(); break;
						case '3':           inputChannelTrig(cd, w->trackerfy, 3);  regenGlobalRowc(s); redraw(); break;
						case '4':           inputChannelTrig(cd, w->trackerfy, 4);  regenGlobalRowc(s); redraw(); break;
						case '5':           inputChannelTrig(cd, w->trackerfy, 5);  regenGlobalRowc(s); redraw(); break;
						case '6':           inputChannelTrig(cd, w->trackerfy, 6);  regenGlobalRowc(s); redraw(); break;
						case '7':           inputChannelTrig(cd, w->trackerfy, 7);  regenGlobalRowc(s); redraw(); break;
						case '8':           inputChannelTrig(cd, w->trackerfy, 8);  regenGlobalRowc(s); redraw(); break;
						case '9':           inputChannelTrig(cd, w->trackerfy, 9);  regenGlobalRowc(s); redraw(); break;
						case 'A': case 'a': inputChannelTrig(cd, w->trackerfy, 10); regenGlobalRowc(s); redraw(); break;
						case 'B': case 'b': inputChannelTrig(cd, w->trackerfy, 11); regenGlobalRowc(s); redraw(); break;
						case 'C': case 'c': inputChannelTrig(cd, w->trackerfy, 12); regenGlobalRowc(s); redraw(); break;
						case 'D': case 'd': inputChannelTrig(cd, w->trackerfy, 13); regenGlobalRowc(s); redraw(); break;
						case 'E': case 'e': inputChannelTrig(cd, w->trackerfy, 14); regenGlobalRowc(s); redraw(); break;
						case 'F': case 'f': inputChannelTrig(cd, w->trackerfy, 15); regenGlobalRowc(s); redraw(); break;
						case ' ': setChannelTrig(cd, w->trackerfy, VARIANT_OFF); regenGlobalRowc(s); redraw(); break;
						case 127: case '\b': /* backspace */ setChannelTrig(cd, w->trackerfy, VARIANT_VOID); regenGlobalRowc(s); redraw(); break;
						case 1:  /* ^a */                    setChannelTrig(cd, w->trackerfy, cd->trig[w->trackerfy].index+MAX(1, w->count)-1); regenGlobalRowc(s); redraw(); break;
						case 24: /* ^x */                    setChannelTrig(cd, w->trackerfy, cd->trig[w->trackerfy].index-MAX(1, w->count)+1); regenGlobalRowc(s); redraw(); break;
					} break;
				case T_MODE_INSERT:
					r = getChannelRow(cd, w->trackerfy);
					if (input == '\n' || input == '\r') { toggleChannelMute(w->channel); redraw(); }
					else
						switch (w->trackerfx)
						{
							case 0: /* note */
								switch (input)
								{
									case 1:  /* ^a */ r->note+=MAX(1, w->count); break;
									case 24: /* ^x */ r->note-=MAX(1, w->count); break;
									case 127: case '\b': /* backspace */
										r->note = NOTE_VOID;
										r->inst = INST_VOID;
										trackerUpArrow(w->step);
										break;
									/* case '0': r->note = changeNoteOctave(0, r->note); break;
									case '1': r->note = changeNoteOctave(1, r->note); break;
									case '2': r->note = changeNoteOctave(2, r->note); break;
									case '3': r->note = changeNoteOctave(3, r->note); break;
									case '4': r->note = changeNoteOctave(4, r->note); break;
									case '5': r->note = changeNoteOctave(5, r->note); break;
									case '6': r->note = changeNoteOctave(6, r->note); break;
									case '7': r->note = changeNoteOctave(7, r->note); break;
									case '8': r->note = changeNoteOctave(8, r->note); break;
									case '9': r->note = changeNoteOctave(9, r->note); break; */
									default:
										insertNote(r, input);
										previewNote(input, r->inst, w->channel);
										trackerDownArrow(w->step);
										break;
								} break;
							case 1: /* instrument */ insertInst(r, input); break;
							default: /* macros */
								macro = (w->trackerfx - 2)>>1;
								switch (input)
								{
									case '~': /* toggle case */
										if      (isupper(r->macro[macro].c)) r->macro[macro].c += 32;
										else if (islower(r->macro[macro].c)) r->macro[macro].c -= 32;
										break;
									default:
										if (!(w->trackerfx%2)) insertMacroc(r, macro, input);
										else                   insertMacrov(r, macro, input);
										break;
								} break;
						} regenGlobalRowc(s); redraw(); break;
			} break;
	}
	if (w->count) { w->count = 0; redraw(); }
	if (w->chord) { w->chord = '\0'; redraw(); }
	// pushPatternHistoryIfNew(s->patternv[s->patterni[s->trig[w-> songfy].index]]);
}
