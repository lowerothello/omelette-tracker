/* https://stackoverflow.com/a/32936928 */
size_t count_utf8_code_points(const char *s)
{
	size_t count = 0;
	while (*s) {
		count += (*s++ & 0xC0) != 0x80;
	}
	return count;
}


void effectRedraw(void)
{
	if (w->popup && w->popup > 2)
	{
		switch (w->mode)
		{
			case 1:
				printf("\033[%d;0H\033[1m-- MOUSEADJUST --\033[m", ws.ws_row);
				w->command.error[0] = '\0';
				break;
		}

		unsigned short x, y;
		x = w->instrumentcelloffset; /* reused instrument vars */
		y = w->instrumentrowoffset;
		switch (w->popup)
		{
			case 3:
				printf("\033[%d;%dH┌─────────── <- [EFFECT %02x] -> ───────────┐", y - 1, x, w->effect);
				for (int i = 0; i < INSTRUMENT_BODY_ROWS; i++)
					printf("\033[%d;%dH│                                         │", y + i, x);
				printf("\033[%d;%dH└─────────────────────────────────────────┘", y + INSTRUMENT_BODY_ROWS, x);

				effect *ev = &s->effectv[w->effect];
				if (!ev->type)
				{
					printf("\033[%d;%dH            (no effect loaded)           ", y + 1, x + 1);
					printf("\033[%d;%dH", y - 1, x + 17 + w->fieldpointer);
					w->effectindex = 0;
				} else switch (ev->type)
				{
					case 2:
						printf("\033[%d;%dH(%.*s)", y + 1, x + maxshort(2, (INSTRUMENT_TYPE_COLS - strlen(ev->name)) / 2), INSTRUMENT_TYPE_COLS - 4, ev->name);

						unsigned short maxnamewidth = 0, maxvaluewidth = 1;
						char valuebuffer[128];
						for (uint32_t i = 0; i < ev->controlc; i++)
						{
							if (strlen(ev->controlv[i].name) > maxnamewidth)
								maxnamewidth = strlen(ev->controlv[i].name);

							if (!ev->controlv[i].toggled)
							{
								if (ev->controlv[i].enumerate)
								{
									for (unsigned int j = 0; j < ev->controlv[i].enumerate; j++)
										if (strlen(ev->controlv[i].scalelabel[j]) > maxvaluewidth)
											maxvaluewidth = count_utf8_code_points(ev->controlv[i].scalelabel[j]);
								} else
								{
									if (ev->controlv[i].integer)
										snprintf(valuebuffer, 128, ev->controlv[i].format, (int)ev->controlv[i].value);
									else
										snprintf(valuebuffer, 128, ev->controlv[i].format, ev->controlv[i].value);

									if (strlen(valuebuffer) > maxvaluewidth)
										maxvaluewidth = strlen(valuebuffer);
								}
							}
						}

						const unsigned char maxwidth = 37;
						signed char spacing = 5;
						if (maxvaluewidth + maxnamewidth > maxwidth - spacing)
						{
							for (spacing = 4; spacing > 0; spacing--)
							{
								if (!(maxvaluewidth + maxnamewidth > maxwidth - spacing))
									break;
							}
							if (maxvaluewidth + maxnamewidth > maxwidth - spacing)
							{
								if (maxnamewidth > 20)
									for (unsigned short i = maxnamewidth; i > 20; i--)
									{
										if (!(maxvaluewidth + maxnamewidth > maxwidth - spacing))
											break;
									}

								if (maxvaluewidth + maxnamewidth > maxwidth - spacing)
									maxvaluewidth = maxwidth - maxnamewidth;
							}
						}

						unsigned short namex, valuex;
						namex = (INSTRUMENT_TYPE_COLS - maxnamewidth - maxvaluewidth - spacing) / 2;
						valuex = namex + maxnamewidth + spacing;

						int drawninputs = 0;
						for (uint32_t i = 0; i < ev->controlc; i++)
						{
							if (drawninputs - w->effectoffset < 0)
							{
								drawninputs++;
								continue;
							}

							if (drawninputs - w->effectoffset < INSTRUMENT_BODY_ROWS - 3)
							{
								printf("\033[%d;%dH%s\033[%d;%dH",
									y + 3 + drawninputs - w->effectoffset, x + namex,
									ev->controlv[i].name,
									y + 3 + drawninputs - w->effectoffset, x + valuex);

								if (ev->controlv[i].toggled)
								{
									if ((int)ev->controlv[i].value)
										printf("%*s", maxvaluewidth + 2, "[X]");
									else
										printf("%*s", maxvaluewidth + 2, "[ ]");
								} else
								{
									if (ev->controlv[i].enumerate)
									{
										for (unsigned int j = 0; j < ev->controlv[i].enumerate; j++)
											if (ev->controlv[i].scalevalue[j] == ev->controlv[i].value)
											{
												int o = maxvaluewidth - count_utf8_code_points(ev->controlv[i].scalelabel[j]);
												if (o > 0) printf("[\033[%dC%s]", o, ev->controlv[i].scalelabel[j]);
												else       printf("[%.*s]", maxvaluewidth, ev->controlv[i].scalelabel[j]);
												break;
											}
									} else
									{
										float displayvalue = ev->controlv[i].value;

										if (ev->controlv[i].integer)
											snprintf(valuebuffer, 128, ev->controlv[i].format, (int)displayvalue);
										else
											snprintf(valuebuffer, 128, ev->controlv[i].format, displayvalue);

										printf("[%*s]", maxvaluewidth, valuebuffer);
									}
								}
								drawninputs++;
							}
						}

						switch (w->effectindex)
						{
							case MIN_EFFECT_INDEX:
								printf("\033[%d;%dH", y - 1, x + 17 + w->fieldpointer);
								break;
							default:
								if (2 + w->effectindex - w->effectoffset > 2
										&& 2 + w->effectindex - w->effectoffset < INSTRUMENT_BODY_ROWS)
								{
									if (ev->controlv[w->effectindex - 1].toggled)
										printf("\033[%d;%dH",
											y + 2 + w->effectindex - w->effectoffset, x + valuex + maxvaluewidth + w->fieldpointer);
									else
										printf("\033[%d;%dH",
											y + 2 + w->effectindex - w->effectoffset, x + 1 + valuex + w->fieldpointer);
								} else printf("\033[%d;%dH", ws.ws_row, ws.ws_col); /* put the cursor in the bottom right if it's off the screen */
						}
						break;
				}
				break;
			case 4:
				printf("\033[%d;%dH┌────────────── [lv2 index] ──────────────┐", y - 1, x);
				for (int i = 0; i < INSTRUMENT_BODY_ROWS; i++)
				{
					printf("\033[%d;%dH│                                         │", y + i, x);
				}

				unsigned char cursoroffset = 10;
				for (int i = 0; i < lv2.pluginc; i++)
				{
					if (1 + i - w->pluginindex + cursoroffset > 20) break;
					if (1 + i - w->pluginindex + cursoroffset > 0)
					{
						printf("\033[%d;%dH%.*s", y + i - w->pluginindex + cursoroffset, x + 2, INSTRUMENT_TYPE_COLS - 2, w->pluginlist[i]);
					}
				}

				printf("\033[%d;%dH└─────────────────────────────────────────┘", y + INSTRUMENT_BODY_ROWS, x);

				printf("\033[%d;%dH", y + cursoroffset + w->fyoffset, x + 2);
				break;
		}
	}
}

void effectAdjustUp(window *w, effect *ev, short index) {}
void effectAdjustDown(window *w, effect *ev, short index) {}

void effectAdjustLeft(window *w, effect *ev, short index)
{
	if (index == MIN_EFFECT_INDEX && w->effect > 0)
		w->effect--;
	else if (ev->type)
	{
		lv2control *lc = &ev->controlv[index - 1];
		float stepsize, newvalue;

		if (lc->samplerate)
			newvalue = lc->value / samplerate;
		else newvalue = lc->value;

		if (lc->enumerate)
		{
			for (unsigned int i = 1; i < lc->enumerate; i++)
				if (lc->scalevalue[i] == newvalue)
				{
					newvalue = lc->scalevalue[i - 1];
					break;
				}
		} else if (lc->integer)
		{
			if (newvalue - 1 > lc->min)
				newvalue -= 1;
			else newvalue = lc->min;
		} else if (lc->logarithmic)
		{
			float currentstep = (lc->steps - 1.0) * logf(newvalue / lc->min) / logf(lc->max / lc->min);
			float nextstep = lc->min * powf(lc->max / lc->min, (currentstep - 1.0) / (lc->steps - 1.0));
			if (nextstep < lc->min) newvalue = lc->min;
			else newvalue = nextstep;
		} else
		{
			stepsize = (lc->max - lc->min) / lc->steps;
			if (newvalue - stepsize > lc->min)
				newvalue -= stepsize;
			else newvalue = lc->min;
		}

		if (lc->samplerate)
			lc->value = newvalue * samplerate;
		else lc->value = newvalue;
	}
}
void effectAdjustRight(window *w, effect *ev, short index)
{
	if (index == MIN_EFFECT_INDEX && w->effect < 15)
		w->effect++;
	else if (ev->type)
	{
		lv2control *lc = &ev->controlv[index - 1];
		float stepsize, newvalue;

		if (lc->samplerate)
			newvalue = lc->value / samplerate;
		else newvalue = lc->value;

		if (lc->enumerate)
		{
			for (unsigned int i = 0; i < lc->enumerate - 1; i++)
				if (lc->scalevalue[i] == newvalue)
				{
					newvalue = lc->scalevalue[i + 1];
					break;
				}
		} else if (lc->integer)
		{
			if (newvalue + 1 < lc->max)
				newvalue += 1;
			else newvalue = lc->max;
		} else if (lc->logarithmic)
		{
			float currentstep = (lc->steps - 1.0) * logf(newvalue / lc->min) / logf(lc->max / lc->min);
			float nextstep = lc->min * powf(lc->max / lc->min, (currentstep + 1.0) / (lc->steps - 1.0));
			DEBUG=currentstep;
			if (nextstep > lc->max) newvalue = lc->max;
			else newvalue = nextstep;
		} else
		{
			stepsize = (lc->max - lc->min) / lc->steps;
			if (newvalue + stepsize < lc->max)
				newvalue += stepsize;
			else newvalue = lc->max;
		}

		if (lc->samplerate)
			lc->value = newvalue * samplerate;
		else lc->value = newvalue;
	}
}

int effectListSearchEnd(char *input, unsigned char *mode)
{
	if (w->search) free(w->search);
	w->search = malloc(strlen(input));
	strcpy(w->search, input);

	char *buffer = malloc(strlen(w->search) + 1);
	strcpy(buffer, "/"); strcat(buffer, w->search);
	strcpy(w->command.error, buffer);
	free(buffer);
	return 0;
}

void effectListSearch(char *input, short index)
{
	/* look for input from the cursor to the bottom */
	for (int i = 0; i < lv2.pluginc; i++)
	{
		if (i + 1 < index) continue;

		if (strstr(w->pluginlist[i], input))
		{
			w->pluginindex = i;
			return;
		}
	}
	/* no occurrance from cursor to bottom, get the first occurrance */
	for (int i = 0; i < lv2.pluginc; i++)
	{
		if (strstr(w->pluginlist[i], input))
		{
			w->pluginindex = i;
			return;
		}
	}
	/* no occurrances */
}
void effectListRevSearch(char *input, short index)
{
	int hold = -1;
	/* look for input from the top to the cursor */
	for (int i = 0; i < lv2.pluginc; i++)
	{
		if (i < index)
		{
			if (strstr(w->pluginlist[i], input))
				hold = i;
		} else
		{
			if (hold >= 0)
			{
				w->pluginindex = hold;
				return;
			} else break;
		}
	}
	/* no occurrance from top to cursor, get the last occurrance */
	for (int i = 0; i < lv2.pluginc; i++)
	{
		if (strstr(w->pluginlist[i], input))
			hold = i;
	}
	if (hold >= 0)
	{
		w->pluginindex = hold;
		return;
	}
	/* no occurrances */
}
inline static void effectListSearchChar(char *input)
{
	effectListSearch(input, w->pluginindex);
}


void effectInput(int input)
{
	switch (w->popup)
	{
		case 3: /* effect */
			switch (input)
			{
				case 10: case 13: /* return */
					switch (w->effectindex)
					{
						case MIN_EFFECT_INDEX:
							/* break;
						case MIN_EFFECT_INDEX + 1: */
							w->popup = 4;
							w->command.error[0] = '\0';
							redraw();
							break;
						default:
							/* handle toggle buttons */
							effect *le = &s->effectv[w->effect];
							if (le->controlv[w->effectindex - 1].toggled)
							{
								if (le->controlv[w->effectindex - 1].value > 0.5)
									le->controlv[w->effectindex - 1].value = 0.0;
								else
									le->controlv[w->effectindex - 1].value = 1.0;
							}
							redraw();
							break;
					}
					break;
				case '\033':
					switch (getchar())
					{
						case 'O':
							switch (getchar())
							{
								case 'P':
									w->popup = 1;
									w->mode = 0;
									w->instrumentindex = MIN_INSTRUMENT_INDEX;
									break;
								case 'Q':
									w->popup = 0;
									break;
							}
							redraw();
							break;
						case '[':
							effect *ev = &s->effectv[w->effect];
							switch (getchar())
							{
								case 'A': /* up arrow */
									w->fieldpointer = 0;
									w->effectindex--;
									if (w->effectindex < MIN_EFFECT_INDEX)
										w->effectindex = MIN_EFFECT_INDEX;
									if (ev->indexc > 8 + 9 && w->effectindex > 9)
									{
										w->effectoffset = w->effectindex - 9;
										if (ev->indexc - w->effectindex < 8) w->effectoffset = ev->indexc - 8 - 9;
									} else w->effectoffset = 0;
									redraw();
									break;
								case 'B': /* down arrow */
									w->fieldpointer = 0;
									w->effectindex++;
									if (w->effectindex > ev->indexc)
										w->effectindex = ev->indexc;
									if (ev->indexc > 8 + 9 && w->effectindex > 9)
									{
										w->effectoffset = w->effectindex - 9;
										if (ev->indexc - w->effectindex < 8) w->effectoffset = ev->indexc - 8 - 9;
									} else w->effectoffset = 0;
									redraw();
									break;
								case 'D': /* left arrow */
									effectAdjustLeft(w, ev, w->effectindex);
									redraw();
									break;
								case 'C': /* right arrow */
									effectAdjustRight(w, ev, w->effectindex);
									redraw();
									break;
								case '1': /* mod+arrow / f5 - f8 */
									switch (getchar())
									{
										case '5': /* f5, play */
											getchar(); /* extraneous tilde */
											startPlayback();
											break;
										case '7': /* f6 (yes, f6 is '7'), stop */
											getchar(); /* extraneous tilde */
											stopPlayback();
											break;
										case ';': /* mod+arrow */
											getchar();
											getchar();
											break;
									}
									break;
								case 'H': /* home */
									if (w->effectindex == MIN_EFFECT_INDEX)
									{
										w->effect = 0;
										w->effectoffset = 0;
									} else w->effectindex = MIN_EFFECT_INDEX;
									redraw();
									break;
								case '4': /* end */
									getchar(); /* eat the trailing tilde */
									w->effectindex = ev->indexc;
									if (ev->indexc > 8 + 9)
									{
										w->effectoffset = ev->indexc - 8 - 9;
									} else w->effectoffset = 0;
									redraw();
									break;
								case '5': /* page up */
									getchar(); /* burn through the tilde */
									break;
								case '6': /* page down */
									getchar(); /* burn through the tilde */
									break;
								case 'M': /* mouse */
									w->fieldpointer = 0;
									int button = getchar();
									int x = getchar() - 32;
									int y = getchar() - 32;
									switch (button)
									{
										case 32 + 64: /* scroll up */
											w->effectoffset -= 3;
											if (w->effectoffset < 0) w->effectoffset = 0;
											break;
										case 33 + 64: /* scroll down */
											w->effectoffset += 3;
											if (ev->indexc < 8 + 9) w->effectoffset = 0;
											else if (w->effectoffset > ev->indexc - 8 - 9) w->effectoffset = ev->indexc - 8 - 9;
											break;
										case 35: /* release click */
											/* leave adjust mode */
											if (w->mode > 0) w->mode = 0;
											break;
										case BUTTON1:
											if (x < w->instrumentcelloffset
													|| x > w->instrumentcelloffset + INSTRUMENT_BODY_COLS - 1
													|| y < w->instrumentrowoffset - 1
													|| y > w->instrumentrowoffset + INSTRUMENT_BODY_ROWS)
											{
												w->popup = 0;
												break;
											}

											if (ev->type)
											{
												w->effectindex = w->effectoffset + (y - w->instrumentrowoffset - 2);
												if (w->effectindex < 0)
													w->effectindex = 0;
												else if (w->effectindex > ev->indexc)
													w->effectindex = ev->indexc;

												/* handle toggle buttons */
												effect *le = &s->effectv[w->effect];
												if (le->controlv[w->effectindex - 1].toggled)
												{
													if (le->controlv[w->effectindex - 1].value > 0.5)
														le->controlv[w->effectindex - 1].value = 0.0;
													else
														le->controlv[w->effectindex - 1].value = 1.0;
												}
											}
											/* enter mouse adjust mode */
											w->mode = 1;
											w->mousey = y;
											w->mousex = x;
											break;
										case BUTTON1 + 32:
											if (w->mode > 0) // mouse adjust
											{
												if      (x > w->mousex) effectAdjustRight(w, ev, w->effectindex);
												else if (x < w->mousex) effectAdjustLeft(w, ev, w->effectindex);

												if      (y > w->mousey) effectAdjustDown(w, ev, w->effectindex);
												else if (y < w->mousey) effectAdjustUp(w, ev, w->effectindex);

												w->mousey = y;
												w->mousex = x;
											}
											break;
									}
									redraw();
									break;
							}
							break;
						default:
							w->fieldpointer = 0;
							w->popup = 0;
							redraw();
							break;
					}
					break;
			}
			break;
		case 4: /* effect loading */
			switch (input)
			{
				case '/': /* search */
					setCommand(&w->command, &effectListSearchEnd, &effectListSearchChar, 0, "/", "");
					w->mode = 255;
					redraw();
					break;
				case 'n': /* next search */
					if (w->search)
					{
						effectListSearch(w->search, w->pluginindex + 2);
						char *buffer = malloc(strlen(w->search) + 1);
						strcpy(buffer, "/"); strcat(buffer, w->search);
						strcpy(w->command.error, buffer);
						free(buffer);
					} else strcpy(w->command.error, "no search term");
					redraw();
					break;
				case 'N': /* prev search */
					if (w->search)
					{
						effectListRevSearch(w->search, w->pluginindex);
						char *buffer = malloc(strlen(w->search) + 1);
						strcpy(buffer, "?"); strcat(buffer, w->search);
						strcpy(w->command.error, buffer);
						free(buffer);
					} else strcpy(w->command.error, "no search term");
					redraw();
					break;
				case 10: case 13: /* return */
					short xc = 0;
					unsigned char cursoroffset = 10;
					LILV_FOREACH(plugins, i, lv2.plugins) {
						if (xc - w->pluginindex + cursoroffset > 20 || xc > lv2.pluginc) break;
						if (xc == w->pluginindex)
						{
							loadLv2Effect(s, w, w->effect, lilv_plugins_get(lv2.plugins, i));
							break;
						}
						xc++;
					}
					w->popup = 3;
					redraw();
					break;
				case '\033':
					switch (getchar())
					{
						case 'O':
							switch (getchar())
							{
								case 'P':
									w->popup = 1;
									w->mode = 0;
									w->instrumentindex = MIN_INSTRUMENT_INDEX;
									break;
								case 'Q':
									w->popup = 0;
									w->mode = 0;
									break;
							}
							redraw();
							break;
						case '[':
							switch (getchar())
							{
								case 'A': /* up arrow */
									w->pluginindex--;
									if (w->pluginindex < 0)
										w->pluginindex = 0;
									redraw();
									break;
								case 'B': /* down arrow */
									w->pluginindex++;
									if (w->pluginindex > lv2.pluginc - 1)
										w->pluginindex = lv2.pluginc - 1;
									redraw();
									break;
								case 'D': case 'C': /* left/right arrow */
									break;
								case '1': /* mod+arrow / f5 - f8 */
									switch (getchar())
									{
										case '5': /* f5, play */
											getchar(); /* extraneous tilde */
											startPlayback();
											break;
										case '7': /* f6 (yes, f6 is '7'), stop */
											getchar(); /* extraneous tilde */
											stopPlayback();
											break;
										case ';': /* mod+arrow */
											getchar();
											getchar();
											break;
									}
									break;
								case 'H': /* home */
									w->pluginindex = 0;
									redraw();
									break;
								case '4': /* end */
									getchar(); /* eat the trailing tilde */
									w->pluginindex = lv2.pluginc - 1;
									redraw();
									break;
								case '5': /* page up */
									getchar(); /* burn through the tilde */
									w->pluginindex-=16;
									if (w->pluginindex < 0)
										w->pluginindex = 0;
									redraw();
									break;
								case '6': /* page down */
									getchar(); /* burn through the tilde */
									w->pluginindex+=16;
									if (w->pluginindex > lv2.pluginc - 1)
										w->pluginindex = lv2.pluginc - 1;
									redraw();
									break;
								case 'M': /* mouse */
									int button = getchar();
									int x = getchar() - 32;
									int y = getchar() - 32;
									switch (button)
									{
										case 32 + 64: /* scroll up */
											w->pluginindex -= 3;
											if (w->pluginindex < 0)
												w->pluginindex = 0;
											break;
										case 33 + 64: /* scroll down */
											w->pluginindex += 3;
											if (w->pluginindex > lv2.pluginc - 1)
												w->pluginindex = lv2.pluginc - 1;
											break;
										case 35: /* release click */
											w->pluginindex += w->fyoffset;
											w->fyoffset = 0;
											/* leave adjust mode */
											if (w->mode > 0) w->mode = 0;
											break;
										case BUTTON1:
											/* if (y < w->instrumentrowoffset + 2
													|| y > w->instrumentrowoffset + INSTRUMENT_TYPE_ROWS + 4)
											{
												w->popup = 1;
												w->instrumentindex = 0;
											} */
											if (x < w->instrumentcelloffset
													|| x > w->instrumentcelloffset + INSTRUMENT_BODY_COLS
													|| y < w->instrumentrowoffset - 1
													|| y > w->instrumentrowoffset + INSTRUMENT_BODY_ROWS)
											{
												w->popup = 0;
												break;
											}
										case BUTTON1 + 32:
											if (y > ws.ws_row - 1 || y < 3) break; /* ignore clicking out of range */
											w->fyoffset = y - (w->instrumentrowoffset + 10); /* magic number */
											if (w->fyoffset + w->pluginindex < 0)
												w->fyoffset -= w->pluginindex + w->fyoffset;
											if (w->fyoffset + w->pluginindex > lv2.pluginc - 1)
												w->fyoffset -= w->pluginindex + w->fyoffset - lv2.pluginc + 1;
											break;
									}
									redraw();
									break;
							}
							break;
						default:
							w->popup = 3;
							redraw();
							break;
					}
					break;
			}
			break;
	}
}
