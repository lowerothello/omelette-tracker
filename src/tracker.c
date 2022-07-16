#define T_MODE_NORMAL 0
#define T_MODE_VISUAL 1
#define T_MODE_MOUSEADJUST 2
#define T_MODE_INSERT 3
#define T_MODE_VISUALLINE 5
#define T_MODE_SONG 6
#define T_MODE_SONG_INSERT 7
#define T_MODE_SONG_VISUAL 8

#include "trackerdraw.c"

void changeMacro(int input, char *dest)
{
	if (isdigit(input)) *dest = input;
	else switch (input)
	{
		case '%': *dest = '%'; break; /* note chance       */
		case 'b': *dest = 'B'; break; /* band pass filter  */
		case 'B': *dest = 'b'; break; /* bpm               */
		case 'c': *dest = 'C'; break; /* note cut          */
		case 'd': *dest = 'D'; break; /* note delay        */
		case 'g': *dest = 'G'; break; /* gain              */
		case 'G': *dest = 'g'; break; /* smooth gain       */
		case 'h': *dest = 'H'; break; /* high pass filter  */
		case 'l': *dest = 'L'; break; /* low pass filter   */
		case 'm': *dest = 'M'; break; /* microtonal offset */
		case 'n': *dest = 'N'; break; /* notch filter      */
		case 'o': *dest = 'O'; break; /* offset            */
		case 'O': *dest = 'o'; break; /* backwards offset  */
		case 'p': *dest = 'P'; break; /* pitch slide       */
		case 'r': *dest = 'R'; break; /* retrigger         */
		case 'T': *dest = 't'; break; /* gate              */
		case 'v': *dest = 'V'; break; /* vibrato           */
		case 'w': *dest = 'W'; break; /* waveshaper        */
	}
}

void prunePattern(uint8_t index, short callingindex)
{
	if (index == 255) return; /* invalid index */
	if (s->patterni[index] == 0) return; /* pattern doesn't exist */

	/* don't remove if pattern is still referenced */
	for (short i = 0; i < 256; i++) if (s->songi[i] == index && i != callingindex) return;

	pattern *p = s->patternv[s->patterni[index]];
	/* don't remove if pattern is populated */
	for (short c = 0; c < s->channelc; c++)
		for (short i = 0; i < p->rowcc[c] + 1; i++)
		{
			row r = p->rowv[c][i];
			if (r.note || r.macro[0].c || r.macro[1].c) return;
		}
	delPattern(index);
}
void inputPatternHex(row *r, char value)
{
	short macro;
	switch (w->trackerfx)
	{
		case 1: if (r->inst == 255) r->inst = 0; r->inst <<= 4; r->inst += value; break;
		default:
			macro = (w->trackerfx - 2) / 2;
			if (w->trackerfx % 2 == 1) { r->macro[macro].v <<= 4; r->macro[macro].v += value; }
			break;
	}
}
uint8_t changeNoteOctave(uint8_t octave, uint8_t note)
{
	w->octave = octave;
	if (!note) return 0;

	octave = octave * 12;
	while (note > octave + 12) note -= 12;
	while (note < octave + 1)  note += 12;
	return note;
}

void trackerAdjustRight(void) /* mouse adjust only */
{
	uint8_t modulorow = w->trackerfy % s->patternv[s->patterni[s->songi[w->songfy]]]->rowcc[w->channel];
	row *r = &s->patternv[s->patterni[s->songi[w->songfy]]]->rowv[w->channel][modulorow];
	short macro;
	switch (w->trackerfx)
	{
		case 0: if (r->note == 120) r->note = 255; else r->note++; break;
		case 1: if (w->fieldpointer) { if (r->inst < 255 - 16) r->inst+=16; else r->inst = 255; } else if (r->inst < 255) r->inst++; break;
		default:
			macro = (w->trackerfx - 2) / 2;
			if (w->trackerfx % 2 == 1)
			{
				if (w->fieldpointer)
				{
					if (r->macro[macro].v < 255 - 16)
						r->macro[macro].v+=16;
					else
						r->macro[macro].v = 255;
				} else if (r->macro[macro].v < 255)
					r->macro[macro].v++;
			} break;
	}
}
void trackerAdjustLeft(void) /* mouse adjust only */
{
	short macro;
	uint8_t modulorow = w->trackerfy % s->patternv[s->patterni[s->songi[w->songfy]]]->rowcc[w->channel];
	row *r = &s->patternv[s->patterni[s->songi[w->songfy]]]->rowv[w->channel][modulorow];
	switch (w->trackerfx)
	{
		case 0: if (r->note == 255) r->note = 120; else r->note--; break;
		case 1: if (w->fieldpointer) { if (r->inst > 16) r->inst-=16; else r->inst = 0; } else if (r->inst) r->inst--; break;
		default:
			macro = (w->trackerfx - 2) / 2;
			if (w->trackerfx % 2 == 1)
			{
				if (w->fieldpointer)
				{
					if (r->macro[macro].v > 16)
						r->macro[macro].v-=16;
					else
						r->macro[macro].v = 0;
				}
				else if (r->macro[macro].v)
					r->macro[macro].v--;
			} break;
	}
}

void upArrow(void)
{
	int i;
	switch (w->mode)
	{
		case T_MODE_SONG: case T_MODE_SONG_INSERT: case T_MODE_SONG_VISUAL:
			for (i = 0; i < MAX(1, w->count); i++)
				if (w->songfy)
					w->songfy--;
			break;
		default:
			for (i = 0; i < MAX(1, w->count); i++)
			{
				if (s->songi[w->songfy] == 255) return;
				w->trackerfy--;
				if (w->trackerfy < 0)
				{
					if (w->songfy > 0 && s->songi[w->songfy - 1] != 255)
					{
						w->songfy--;
						w->trackerfy = s->patternv[s->patterni[s->songi[w->songfy]]]->rowc;
					} else w->trackerfy = 0;
				}
			} break;
	}
}
void downArrow(void)
{
	int i;
	switch (w->mode)
	{
		case T_MODE_SONG: case T_MODE_SONG_INSERT: case T_MODE_SONG_VISUAL:
			for (i = 0; i < MAX(1, w->count); i++)
				if (w->songfy < 254)
					w->songfy++;
			break;
		default:
			for (i = 0; i < MAX(1, w->count); i++)
			{
				if (s->songi[w->songfy] == 255) return;
				w->trackerfy++;
				if (w->trackerfy > s->patternv[s->patterni[s->songi[w->songfy]]]->rowc)
				{
					if (w->songfy < 255 && s->songi[w->songfy + 1] != 255)
					{
						w->trackerfy = 0;
						w->songfy++;
					} else w->trackerfy = s->patternv[s->patterni[s->songi[w->songfy]]]->rowc;
				}
			} break;
	}
}
void cycleUp(short bound)
{
	switch (w->mode)
	{
		case T_MODE_SONG: case T_MODE_SONG_INSERT: case T_MODE_SONG_VISUAL: break;
		default:
			for (int j = 0; j < MAX(1, w->count); j++)
			{
				pattern *p = s->patternv[s->patterni[s->songi[w->songfy]]];
				row hold = p->rowv[w->channel][bound]; /* hold the first row */
				for (int i = bound; i < p->rowcc[w->channel]; i++) /* signed */
					p->rowv[w->channel][i] = p->rowv[w->channel][i+1];
				p->rowv[w->channel][p->rowcc[w->channel]] = hold; }
			break;
	}
}
void cycleDown(short bound)
{
	switch (w->mode)
	{
		case T_MODE_SONG: case T_MODE_SONG_INSERT: case T_MODE_SONG_VISUAL: break;
		default:
			for (int j = 0; j < MAX(1, w->count); j++)
			{
				pattern *p = s->patternv[s->patterni[s->songi[w->songfy]]];
				row hold = p->rowv[w->channel][p->rowcc[w->channel]]; /* hold the last row */
				for (int i = p->rowcc[w->channel] - 1; i >= bound; i--) /* signed */
					p->rowv[w->channel][i+1] = p->rowv[w->channel][i];
				p->rowv[w->channel][bound] = hold;
			} break;
	}
}
void leftArrow(void)
{
	switch (w->mode)
	{
		case T_MODE_SONG: case T_MODE_SONG_INSERT: case T_MODE_SONG_VISUAL: break;
		default:
			for (int i = 0; i < MAX(1, w->count); i++)
			{
				if (s->songi[w->songfy] == 255) return;
				w->trackerfx--;
				if (w->trackerfx < 0)
				{
					if (w->channel > 0)
					{
						w->channel--;
						w->trackerfx = 1 + s->channelv[w->channel].macroc * 2;
					} else w->trackerfx = 0;
					if (w->visiblechannels % 2 == 0)
					{
						if (w->channeloffset
								&& w->channel < s->channelc - w->visiblechannels / 2)
							w->channeloffset--;
					} else if (w->channeloffset
							&& w->channel < s->channelc - 1 - w->visiblechannels / 2)
						w->channeloffset--;
				}
			} break;
	}
}
void channelLeft(void)
{
	switch (w->mode)
	{
		case T_MODE_SONG: case T_MODE_SONG_INSERT: case T_MODE_SONG_VISUAL: break;
		default:
			for (int i = 0; i < MAX(1, w->count); i++)
			{
				if (s->songi[w->songfy] != 255)
				{
					if (w->channel > 0)
					{
						w->channel--;
						if (w->trackerfx > 1 + s->channelv[w->channel].macroc * 2)
							w->trackerfx = 1 + s->channelv[w->channel].macroc * 2;
					}
					if (w->visiblechannels % 2 == 0)
					{
						if (w->channeloffset
								&& w->channel < s->channelc - w->visiblechannels / 2)
							w->channeloffset--;
					} else if (w->channeloffset
							&& w->channel < s->channelc - 1 - w->visiblechannels / 2)
						w->channeloffset--;
				}
			} break;
	}
}
void rightArrow(void)
{
	switch (w->mode)
	{
		case T_MODE_SONG: case T_MODE_SONG_INSERT: case T_MODE_SONG_VISUAL: break;
		default:
			for (int i = 0; i < MAX(1, w->count); i++)
			{
				if (s->songi[w->songfy] == 255) return;
				w->trackerfx++;
				if (w->trackerfx > 1 + s->channelv[w->channel].macroc * 2)
				{
					if (w->channel < s->channelc - 1)
					{
						w->channel++;
						w->trackerfx = 0;
					} else w->trackerfx = 1 + s->channelv[w->channel].macroc * 2;
					if (w->channeloffset + w->visiblechannels < s->channelc
							&& w->channel > w->visiblechannels / 2)
						w->channeloffset++;
				}
			} break;
	}
}
void channelRight(void)
{
	switch (w->mode)
	{
		case T_MODE_SONG: case T_MODE_SONG_INSERT: case T_MODE_SONG_VISUAL: break;
		default:
			for (int i = 0; i < MAX(1, w->count); i++)
			{
				if (s->songi[w->songfy] == 255) return;
				if (w->channel < s->channelc - 1)
				{
					w->channel++;
					if (w->trackerfx > 1 + s->channelv[w->channel].macroc * 2)
						w->trackerfx = 1 + s->channelv[w->channel].macroc * 2;
				}
				if (w->channeloffset + w->visiblechannels < s->channelc
						&& w->channel > w->visiblechannels / 2)
					w->channeloffset++;
			} break;
	}
}

void inputSongHex(char value)
{
	prunePattern(s->songi[w->songfy], w->songfy);
	if (s->songi[w->songfy] == 255)
		s->songi[w->songfy] = 0;
	s->songi[w->songfy] <<= 4; s->songi[w->songfy] += value;
	addPattern(s->songi[w->songfy], 0);
	redraw();
}


void trackerInput(int input)
{
	int button, x, y, i, j, k;
	short macro;
	row *r;
	pattern *p;
	uint8_t note, modulorow;
	switch (input)
	{
		case '\033': /* escape */
			switch (getchar())
			{
				case 'O': handleFKeys(getchar()); previewNote(255, 255, w->channel); redraw(); break;
				case '[':
					previewNote(255, 255, w->channel);
					switch (getchar())
					{
						case 'A': /* up arrow    */ upArrow(); redraw(); break;
						case 'B': /* down arrow  */ downArrow(); redraw(); break;
						case 'D': /* left arrow  */ leftArrow(); redraw(); break;
						case 'C': /* right arrow */ rightArrow(); redraw(); break;
						case '1': /* mod+arrow / f5 - f8 */
							switch (getchar())
							{
								case '5': /* f5, play */
									switch (w->mode)
									{
										case T_MODE_INSERT: case T_MODE_SONG: case T_MODE_SONG_INSERT: break;
										case T_MODE_SONG_VISUAL: w->mode = T_MODE_SONG; break;
										default:                 w->mode = T_MODE_NORMAL; break;
									}
									startPlayback();
									getchar(); break;
								case '7': /* f6, stop */
									switch (w->mode)
									{
										case T_MODE_INSERT: case T_MODE_SONG: case T_MODE_SONG_INSERT: break;
										case T_MODE_SONG_VISUAL: w->mode = T_MODE_SONG; break;
										default:                 w->mode = T_MODE_NORMAL; break;
									}
									stopPlayback();
									getchar(); break;
								case ';': /* mod+arrow */
									switch (getchar())
									{
										case '5': /* ctrl+arrow */
											switch (getchar())
											{
												case 'D': /* left  */ channelLeft(); redraw(); break;
												case 'C': /* right */ channelRight(); redraw(); break;
												case 'A': /* up    */ cycleUp(0);   redraw(); break;
												case 'B': /* down  */ cycleDown(0); redraw(); break;
											} break;
										case '6': /* ctrl+shift+arrow */
											switch (getchar())
											{
												case 'A': /* up    */
													cycleUp(w->trackerfy%(s->patternv[s->patterni[s->songi[w->songfy]]]->rowcc[w->channel]+1));
													redraw(); break;
												case 'B': /* down  */
													cycleDown(w->trackerfy%(s->patternv[s->patterni[s->songi[w->songfy]]]->rowcc[w->channel]+1));
													redraw(); break;
											} break;
										default: getchar(); break;
									} break;
							} break;
						case 'H': /* home */
							switch (w->mode)
							{
								case T_MODE_SONG: case T_MODE_SONG_INSERT: case T_MODE_SONG_VISUAL:
									w->songfy = 0;
									break;
								default:
									w->trackerfy = 0;
									break;
							} redraw(); break;
						case '4': /* end */
							if (getchar() == '~')
							{
								switch (w->mode)
								{
									case T_MODE_SONG: case T_MODE_SONG_INSERT: case T_MODE_SONG_VISUAL:
										w->songfy = 254;
										break;
									default:
										w->trackerfy = s->patternv[s->patterni[s->songi[w->songfy]]]->rowc;
										break;
								} redraw();
							} break;
						case '5': /* page up */
							switch (w->mode)
							{
								case T_MODE_SONG: case T_MODE_SONG_INSERT: case T_MODE_SONG_VISUAL:
									for (int i = 0; i < MAX(1, w->count); i++)
									{
										w->songfy -= s->rowhighlight;
										if (w->songfy < 0) { w->songfy = 0; break; }
									} break;
								default:
									for (int i = 0; i < MAX(1, w->count); i++)
									{
										if (s->songi[w->songfy] == 255) break;
										w->trackerfy -= s->rowhighlight;
										if (w->trackerfy < 0)
										{
											if (w->songfy > 0 && s->songi[w->songfy - 1] != 255)
											{
												w->songfy--;
												w->trackerfy += s->patternv[s->patterni[s->songi[w->songfy]]]->rowc + 1;
											} else w->trackerfy = 0;
										}
									} break;
							} getchar(); redraw(); break;
						case '6': /* page down */
							switch (w->mode)
							{
								case T_MODE_SONG: case T_MODE_SONG_INSERT: case T_MODE_SONG_VISUAL:
									for (int i = 0; i < MAX(1, w->count); i++)
									{
										w->songfy += s->rowhighlight;
										if (w->songfy >= 255)
										{
											w->songfy = 255;
											break;
										}
									} break;
								default:
									for (int i = 0; i < MAX(1, w->count); i++)
									{
										if (s->songi[w->songfy] == 255) break;
										w->trackerfy += s->rowhighlight;
										if (w->trackerfy > s->patternv[s->patterni[s->songi[w->songfy]]]->rowc)
										{
											if (w->songfy < 255 && s->songi[w->songfy + 1] != 255)
											{
												w->songfy++;
												w->trackerfy -= s->patternv[s->patterni[s->songi[w->songfy]]]->rowc + 1;
											} else w->trackerfy = s->patternv[s->patterni[s->songi[w->songfy]]]->rowc;
										}
									} break;
							} getchar(); redraw(); break;
						case 'M': /* mouse */
							button = getchar();
							x = getchar() - 32;
							y = getchar() - 32;
							switch (button)
							{
								case WHEEL_UP: case WHEEL_UP_CTRL: /* scroll up */
									if (s->songi[w->songfy] == 255) break;
									w->trackerfy -= WHEEL_SPEED;
									if (w->trackerfy < 0)
									{
										if (w->songfy > 0 && s->songi[w->songfy - 1] != 255)
										{
											w->songfy--;
											w->trackerfy += s->patternv[s->patterni[s->songi[w->songfy]]]->rowc + 1;
										} else w->trackerfy = 0;
									} break;
								case WHEEL_DOWN: case WHEEL_DOWN_CTRL: /* scroll down */
									if (s->songi[w->songfy] == 255) break;
									w->trackerfy += WHEEL_SPEED;
									if (w->trackerfy > s->patternv[s->patterni[s->songi[w->songfy]]]->rowc)
									{
										if (w->songfy < 255 && s->songi[w->songfy + 1] != 255)
										{
											w->trackerfy -= s->patternv[s->patterni[s->songi[w->songfy]]]->rowc + 1;
											w->songfy++;
										} else w->trackerfy = s->patternv[s->patterni[s->songi[w->songfy]]]->rowc;
									} break;
								case BUTTON_RELEASE: case BUTTON_RELEASE_CTRL: /* release click */
									switch (w->mode)
									{
										case T_MODE_SONG: case T_MODE_SONG_INSERT: case T_MODE_SONG_VISUAL:
											w->songfy += w->fyoffset;
											if (w->songfy < 0) w->songfy = 0;
											else if (w->songfy > 254) w->songfy = 254;
											break;
										default:
											if (s->songi[w->songfy] == 255) break;
											w->trackerfy += w->fyoffset;
											if (w->trackerfy < 0)
											{
												if (w->songfy > 0 && s->songi[w->songfy - 1] != 255)
												{
													w->songfy--;
													w->trackerfy += s->patternv[s->patterni[s->songi[w->songfy]]]->rowc + 1;
												} else w->trackerfy = 0;
											} else if (w->trackerfy > s->patternv[s->patterni[s->songi[w->songfy]]]->rowc)
											{
												if (w->songfy < 255 && s->songi[w->songfy + 1] != 255)
												{
													w->trackerfy -= s->patternv[s->patterni[s->songi[w->songfy]]]->rowc + 1;
													w->songfy++;
												} else w->trackerfy = s->patternv[s->patterni[s->songi[w->songfy]]]->rowc;
											}
											w->channeloffset =
												MAX(0, MIN(s->channelc - 1 - w->visiblechannels / 2,
															w->channel - w->visiblechannels / 2));
											break;
									}
									w->fyoffset = 0;
									w->fieldpointer = 0;
									if (w->mode == T_MODE_MOUSEADJUST) /* leave adjust mode */
										w->mode = w->oldmode;
									break;
								case BUTTON1_HOLD: case BUTTON1_HOLD_CTRL:
									if (w->mode == T_MODE_MOUSEADJUST)
									{
										if      (x > w->mousex) trackerAdjustRight();
										else if (x < w->mousex) trackerAdjustLeft();
										w->mousex = x;
									} break;
								default: /* click / click+drag */
									if (y == CHANNEL_ROW-2)
									{
										if (x < (ws.ws_col-17) / 2 + 7) handleFKeys('P');
										else                            handleFKeys('Q');
										break;
									}
									if (y > ws.ws_row - 1) break; /* ignore clicking out of range */

									short oldtrackerfx = w->trackerfx;
									uint8_t oldchannel = w->channel;
									unsigned short maxwidth = LINENO_COLS + SONGLIST_COLS;
									uint8_t maxchannels = 0;
									for (int i = 0; i < s->channelc; i++)
									{
										if (i+w->channeloffset > s->channelc - 1) break;
										if (maxwidth + 9 + 4*s->channelv[i+w->channeloffset].macroc > ws.ws_col - BORDER*2) break;
										maxwidth += 9 + 4*s->channelv[i+w->channeloffset].macroc;
										maxchannels++;
									}

									unsigned short dx = (ws.ws_col - maxwidth) / 2 + 1 + LINENO_COLS + SONGLIST_COLS;
									if (x < dx - LINENO_COLS)
									{ /* song list */
										switch (w->mode)
										{
											case T_MODE_SONG: case T_MODE_SONG_INSERT: case T_MODE_SONG_VISUAL: break;
											case T_MODE_INSERT: w->mode = T_MODE_SONG_INSERT; break;
											default: w->mode = T_MODE_SONG; break;
										}
										switch (button)
										{
											case BUTTON1: case BUTTON1_CTRL:
												if (w->mode != T_MODE_SONG_INSERT) w->mode = T_MODE_SONG;
												break;
											case BUTTON3: case BUTTON3_CTRL:
												if (w->mode != T_MODE_SONG_VISUAL)
												{
													w->visualfy = w->songfy;
													w->mode = T_MODE_SONG_VISUAL;
												} break;
										}
										w->fyoffset = y - w->centre;
									} else
									{ /* tracker channels */
										switch (w->mode)
										{
											case T_MODE_SONG: case T_MODE_SONG_VISUAL: w->mode = T_MODE_NORMAL; break;
											case T_MODE_SONG_INSERT: w->mode = T_MODE_INSERT; break;
										}
										if (s->songi[w->songfy] == 255) break;
										for (int i = 0; i < maxchannels; i++)
										{
											if (x < dx + 9 + 4*s->channelv[i+w->channeloffset].macroc)
											{
												w->channel = i+w->channeloffset;
												goto dxandwchannelset;
											} dx += 9 + 4*s->channelv[i+w->channeloffset].macroc;
										}
										w->channel = w->channeloffset + maxchannels-1;
										dx -= 9 + 4*s->channelv[w->channeloffset + maxchannels-1].macroc;
dxandwchannelset:
										/* channel row, for mute/solo */
										if (y <= CHANNEL_ROW)
										{
											switch (button)
											{
												case BUTTON1: case BUTTON1_CTRL:
													if (w->mode != T_MODE_INSERT) /* suggest mode 0, but allow insert */
														w->mode = T_MODE_NORMAL;
													s->channelv[w->channel].mute = !s->channelv[w->channel].mute;
													break;
											}
										} else if (button == BUTTON1 || button == BUTTON1_CTRL
												|| button == BUTTON3 || button == BUTTON3_CTRL)
										{
											switch (button)
											{
												case BUTTON1: case BUTTON1_CTRL:
													if (w->mode != T_MODE_INSERT) /* suggest mode 0, but allow insert */
														w->mode = T_MODE_NORMAL;
													break;
												case BUTTON3: case BUTTON3_CTRL:
													if (!(w->mode == T_MODE_VISUAL || w->mode == T_MODE_VISUALLINE))
													{
														w->visualfx = tfxToVfx(w->trackerfx);
														w->visualfy = w->trackerfy;
														w->visualchannel = w->channel;
														w->oldmode = w->mode;
														w->mode = T_MODE_VISUAL;
													} break;
											}
											if (x < dx + 3) w->trackerfx = 0;
											else if (x < dx + 6) w->trackerfx = 1;
											else w->trackerfx = (x - dx - 6) / 2 + 2;
											if (w->trackerfx > (s->channelv[w->channel].macroc + 1) * 2 - 1)
												w->trackerfx = (s->channelv[w->channel].macroc + 1) * 2 - 1;
											w->fyoffset = y - w->centre;
											/* enter adjust */
											if ((button == BUTTON1 || button == BUTTON1_CTRL)
													&& w->fyoffset == 0 && w->trackerfx == oldtrackerfx && w->channel == oldchannel)
											{
												w->oldmode = w->mode;
												w->mode = T_MODE_MOUSEADJUST;
												w->mousex = x;
												w->fieldpointer = 0;
												switch (w->trackerfx)
												{
													case 1: if (x - dx < 5) w->fieldpointer = 1; break;
													default:
														macro = (w->trackerfx - 2) / 2;
														if (w->trackerfx % 2 == 1 && x - dx < 9 + 4*macro)
															w->fieldpointer = 1;
														break;
												}
											}
										}
									} break;
							} redraw(); break;
					} break;
				/* alt+numbers, change step if in insert mode */
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
					previewNote(255, 255, w->channel);
					switch (w->mode)
					{
						case T_MODE_VISUAL: case T_MODE_VISUALLINE: w->mode = w->oldmode; break;
						case T_MODE_SONG_INSERT: case T_MODE_SONG_VISUAL: w->mode = T_MODE_SONG; break;
						case T_MODE_SONG: break;
						default: w->mode = T_MODE_NORMAL; break;
					} redraw(); break;
			} break;
		default:
			if (input == '\n' || input == '\r') /* RET mute */
			{
				s->channelv[w->channel].mute = !s->channelv[w->channel].mute;
				redraw(); break;
			} else
				switch (w->mode)
				{
					case T_MODE_VISUALLINE:
						switch (input)
						{
							case 'v': /* visual       */ w->mode = T_MODE_VISUAL; redraw(); break;
							case 'V': /* visual  line */ w->mode = w->oldmode; redraw(); break;
							case 1: /* ^a */
								addPartPattern(MAX(1, w->count),
										0, 1+s->channelv[w->channel].macroc,
										MIN(w->trackerfy, w->visualfy), MAX(w->trackerfy, w->visualfy),
										MIN(w->channel, w->visualchannel), MAX(w->channel, w->visualchannel));
								redraw(); break;
							case 24: /* ^x */
								addPartPattern(-MAX(1, w->count),
										0, 1+s->channelv[w->channel].macroc,
										MIN(w->trackerfy, w->visualfy), MAX(w->trackerfy, w->visualfy),
										MIN(w->channel, w->visualchannel), MAX(w->channel, w->visualchannel));
								redraw(); break;
							case 'x': case 'd': /* pattern cut */
								if (s->songi[w->songfy] == 255) break;
								yankPartPattern(
										0, 1+s->channelv[w->channel].macroc,
										MIN(w->trackerfy, w->visualfy), MAX(w->trackerfy, w->visualfy),
										MIN(w->channel, w->visualchannel), MAX(w->channel, w->visualchannel));
								delPartPattern(
										0, 1+s->channelv[w->channel].macroc,
										MIN(w->trackerfy, w->visualfy), MAX(w->trackerfy, w->visualfy),
										MIN(w->channel, w->visualchannel), MAX(w->channel, w->visualchannel));
								w->trackerfx = 0;
								w->trackerfy = MIN(w->trackerfy, w->visualfy);
								w->channel = MIN(w->channel, w->visualchannel);
								w->mode = T_MODE_NORMAL;
								redraw(); break;
							case 'y': /* pattern copy */
								if (s->songi[w->songfy] == 255) break;
								yankPartPattern(
										0, 1+s->channelv[w->channel].macroc,
										MIN(w->trackerfy, w->visualfy), MAX(w->trackerfy, w->visualfy),
										MIN(w->channel, w->visualchannel), MAX(w->channel, w->visualchannel));
								w->trackerfx = 0;
								w->trackerfy = MIN(w->trackerfy, w->visualfy);
								w->channel = MIN(w->channel, w->visualchannel);
								w->mode = T_MODE_NORMAL;
								redraw(); break;
						} break;
					case T_MODE_VISUAL:
						switch (input)
						{
							case 'v': /* visual      */ w->mode = w->oldmode; redraw(); break;
							case 'V': /* visual line */ w->mode = T_MODE_VISUALLINE; redraw(); break;
							case 1: /* ^a */
								addPartPattern(MAX(1, w->count),
										MIN(tfxToVfx(w->trackerfx), w->visualfx), MAX(tfxToVfx(w->trackerfx), w->visualfx),
										MIN(w->trackerfy, w->visualfy), MAX(w->trackerfy, w->visualfy),
										MIN(w->channel, w->visualchannel), MAX(w->channel, w->visualchannel));
								redraw(); break;
							case 24: /* ^x */
								addPartPattern(-MAX(1, w->count),
										MIN(tfxToVfx(w->trackerfx), w->visualfx), MAX(tfxToVfx(w->trackerfx), w->visualfx),
										MIN(w->trackerfy, w->visualfy), MAX(w->trackerfy, w->visualfy),
										MIN(w->channel, w->visualchannel), MAX(w->channel, w->visualchannel));
								redraw(); break;
							case 'x': case 'd': /* pattern cut */
								if (s->songi[w->songfy] == 255) break;
								yankPartPattern(
										MIN(tfxToVfx(w->trackerfx), w->visualfx), MAX(tfxToVfx(w->trackerfx), w->visualfx),
										MIN(w->trackerfy, w->visualfy), MAX(w->trackerfy, w->visualfy),
										MIN(w->channel, w->visualchannel), MAX(w->channel, w->visualchannel));
								delPartPattern(
										MIN(tfxToVfx(w->trackerfx), w->visualfx), MAX(tfxToVfx(w->trackerfx), w->visualfx),
										MIN(w->trackerfy, w->visualfy), MAX(w->trackerfy, w->visualfy),
										MIN(w->channel, w->visualchannel), MAX(w->channel, w->visualchannel));
								w->trackerfx = MIN(w->trackerfx, vfxToTfx(w->visualfx));
								w->trackerfy = MIN(w->trackerfy, w->visualfy);
								w->channel = MIN(w->channel, w->visualchannel);
								w->mode = T_MODE_NORMAL;
								redraw(); break;
							case 'y': /* pattern copy */
								if (s->songi[w->songfy] == 255) break;
								yankPartPattern(
										MIN(tfxToVfx(w->trackerfx), w->visualfx), MAX(tfxToVfx(w->trackerfx), w->visualfx),
										MIN(w->trackerfy, w->visualfy), MAX(w->trackerfy, w->visualfy),
										MIN(w->channel, w->visualchannel), MAX(w->channel, w->visualchannel));
								w->trackerfx = MIN(w->trackerfx, vfxToTfx(w->visualfx));
								w->trackerfy = MIN(w->trackerfy, w->visualfy);
								w->channel = MIN(w->channel, w->visualchannel);
								w->mode = T_MODE_NORMAL;
								redraw(); break;
						} break;
					case T_MODE_SONG:
						switch (input)
						{
							case '\t': /* leave song mode   */ w->mode = T_MODE_NORMAL; redraw(); break;
							case 'i':  /* enter insert mode */ w->mode = T_MODE_SONG_INSERT; redraw(); break;
							case 'v':  /* enter visual mode */
								w->visualfy = w->songfy;
								w->oldmode = T_MODE_SONG;
								w->mode = T_MODE_SONG_VISUAL;
								redraw(); break;
							case 1: /* ^a */
								prunePattern(s->songi[w->songfy], w->songfy);
								s->songi[w->songfy]+=MAX(1, w->count);
								addPattern(s->songi[w->songfy], 0);
								redraw(); break;
							case 24: /* ^x */
								prunePattern(s->songi[w->songfy], w->songfy);
								s->songi[w->songfy]-=MAX(1, w->count);
								addPattern(s->songi[w->songfy], 0);
								if (s->songi[w->songfy] == 255 && w->songnext == w->songfy + 1)
									w->songnext = 0;
								redraw(); break;
							case 'l': /* loop */
								if (s->songi[w->songfy] != 255)
									s->songf[w->songfy] = !s->songf[w->songfy];
								redraw(); break;
							case 'n':
								if (s->songi[w->songfy] != 255)
								{
									if (w->songnext == w->songfy + 1) w->songnext = 0;
									else                              w->songnext = w->songfy + 1;
								} redraw(); break;
							case 'c': /* clone */
								if (s->songi[w->songfy] != 255)
									s->songi[w->songfy] = duplicatePattern(s->songi[w->songfy]);
								redraw(); break;
							case 'p': /* song paste */
								if (w->songbufferlen)
								{
									memcpy(&s->songi[w->songfy], w->songibuffer, w->songbufferlen);
									memcpy(&s->songf[w->songfy], w->songfbuffer, w->songbufferlen);
									w->songfy += w->songbufferlen;
								} redraw(); break;
							case 'P': /* song paste above */
								if (w->songbufferlen)
								{
									memcpy(&s->songi[w->songfy], w->songibuffer, w->songbufferlen);
									memcpy(&s->songf[w->songfy], w->songfbuffer, w->songbufferlen);
								} redraw(); break;
							case 's': w->mode = T_MODE_SONG_INSERT;
							case 127: case 8: case 'x': /* backspace */
								if (w->songnext == w->songfy + 1)
									w->songnext = 0;
								s->songi[w->songfy] = 255;
								s->songf[w->songfy] = 0;
								prunePattern(s->songi[w->songfy], w->songfy);
								redraw(); break;
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
												if (s->songi[w->songfy] == 255) break;
												p = s->patternv[s->patterni[s->songi[w->songfy]]];
												j = MIN(p->rowc-1, w->trackerfy - 1 + MAX(1, w->count)) - w->trackerfy;
												yankPartPattern(0, 1+s->channelv[w->channel].macroc,
														w->trackerfy, w->trackerfy+j,
														w->channel, w->channel);
												delPartPattern(0, 1+s->channelv[w->channel].macroc,
														w->trackerfy, w->trackerfy+j,
														w->channel, w->channel);
												for (i = 0; i < j; i++) downArrow();
												redraw();
											} break;
										case 'y': if (input == 'y') /* yank */
											{
												if (s->songi[w->songfy] == 255) break;
												p = s->patternv[s->patterni[s->songi[w->songfy]]];
												j = MIN(p->rowc-1, w->trackerfy - 1 + MAX(1, w->count)) - w->trackerfy;
												yankPartPattern(0, 1+s->channelv[w->channel].macroc,
														w->trackerfy, w->trackerfy+j,
														w->channel, w->channel);
												for (i = 0; i < j; i++) downArrow();
												redraw();
											} break;
										case 'c': /* channel */
											switch (input)
											{
												case 'a': /* add */
													for (i = 0; i < MAX(1, w->count); i++)
													{
														if (s->channelc >= MAX_CHANNELS-1) break;
														addChannel(s, w->channel+1);
														w->channel++;
														if (w->channeloffset + w->visiblechannels < s->channelc
																&& w->channel > w->visiblechannels / 2)
															w->channeloffset++;
													} resize(0); break;
												case 'A': /* add before */
													for (i = 0; i < MAX(1, w->count); i++)
													{
														if (s->channelc >= MAX_CHANNELS-1) break;
														addChannel(s, w->channel);
													} resize(0); break;
												case 'd': /* delete */
													for (i = 0; i < MAX(1, w->count); i++)
													{
														if (delChannel(w->channel)) break;
														if (w->channel > s->channelc - 1)
															w->channel--;
													} resize(0); break;
												case 'D': /* delete to end */ /* ignores w->count */
													if (w->channel == 0) w->channel++;
													for (uint8_t i = s->channelc; i > w->channel; i--)
														delChannel(i - 1);
													w->channel--;
													resize(0); break;
												case 'y': /* yank */ /* ignores w->count */
													yankChannel(w->channel);
													redraw(); break;
												case 'p': /* put */
													for (i = 0; i < MAX(1, w->count); i++)
													{
														if (s->channelc >= MAX_CHANNELS-1) break;
														w->channel++;
														addChannel(s, w->channel);
														putChannel(w->channel);
													} resize(0); break;
												case 'P': /* put before */
													for (i = 0; i < MAX(1, w->count); i++)
													{
														if (s->channelc >= MAX_CHANNELS-1) break;
														addChannel(s, w->channel);
														putChannel(w->channel);
													} resize(0); break;
											} break;
										case 'm': /* macro */
											switch (input)
											{
												case 'a': /* add */
													for (i = 0; i < MAX(1, w->count); i++)
														if (s->channelv[w->channel].macroc < 8)
															s->channelv[w->channel].macroc++;
													resize(0); break;
												case 'd': /* delete */
													for (i = 0; i < MAX(1, w->count); i++)
													{
														if (s->channelv[w->channel].macroc > 1)
															s->channelv[w->channel].macroc--;
														if (w->trackerfx > 1 + s->channelv[w->channel].macroc * 2)
															w->trackerfx = s->channelv[w->channel].macroc * 2;
													} resize(0); break;
												case 'm': /* set */
													if (w->count) s->channelv[w->channel].macroc = MIN(8, w->count);
													else s->channelv[w->channel].macroc = 2;
													resize(0); break;
											} break;
										case 'k': /* keyboard macro */ /* ignores w->count */
											w->keyboardmacro = '\0';
											if (input != 'k') // kk just resets
												changeMacro(input, &w->keyboardmacro);
											redraw(); break;
										case 'r': /* row */
											p = s->patternv[s->patterni[s->songi[w->songfy]]];
											switch (input)
											{
												case 'r':
													j = p->rowcc[w->channel];
													if (w->count) p->rowcc[w->channel] = w->count - 1;
													else          p->rowcc[w->channel] = p->rowc;
													while (j < p->rowcc[w->channel])
														if (j < 127)
														{
															memcpy(&p->rowv[w->channel][j + 1], p->rowv[w->channel],
																	sizeof(row) * (j + 1));
															j = (j + 1) * 2 - 1;
														} else
														{
															memcpy(&p->rowv[w->channel][j + 1], p->rowv[w->channel],
																	sizeof(row) * (255 - j));
															j = 255;
															break;
														}
													redraw(); break;
												case 'd':
													for (i = 0; i < MAX(1, w->count); i++)
														if (p->rowcc[w->channel]) p->rowcc[w->channel]--;
													redraw(); break;
												case 'a':
													for (i = 0; i < MAX(1, w->count); i++)
														if (p->rowcc[w->channel] < 255)
														{
															p->rowcc[w->channel]++;
															memset(&p->rowv[w->channel][p->rowcc[w->channel]], 0, sizeof(row));
															p->rowv[w->channel][p->rowcc[w->channel]].inst = 255;
														} else break;
													redraw(); break;
												case '-':
													for (i = 0; i < MAX(1, w->count); i++)
														if (p->rowcc[w->channel] == 255)
															p->rowcc[w->channel] = 127;
														else if (p->rowcc[w->channel])
															p->rowcc[w->channel] = (p->rowcc[w->channel] + 1) / 2 - 1;
														else break;
													redraw(); break;
												case '+': case '=':
													for (i = 0; i < MAX(1, w->count); i++)
														if (p->rowcc[w->channel] < 127)
														{
															memcpy(&p->rowv[w->channel][p->rowcc[w->channel] + 1], p->rowv[w->channel],
																	sizeof(row) * (p->rowcc[w->channel] + 1));
															p->rowcc[w->channel] = (p->rowcc[w->channel] + 1) * 2 - 1;
														} else
														{
															memcpy(&p->rowv[w->channel][p->rowcc[w->channel] + 1], p->rowv[w->channel],
																	sizeof(row) * (255 - p->rowcc[w->channel]));
															p->rowcc[w->channel] = 255;
															break;
														}
													redraw(); break;
												case '/':
													for (i = 0; i < MAX(1, w->count); i++)
														if (p->rowcc[w->channel] == 255)
														{
															for (j = 0; j < 127; j++)
																p->rowv[w->channel][j] = p->rowv[w->channel][j*2];
															p->rowcc[w->channel] = 127;
														} else if (p->rowcc[w->channel])
														{
															for (j = 0; j < p->rowcc[w->channel] + 1; j++)
																p->rowv[w->channel][j] = p->rowv[w->channel][j*2];
															p->rowcc[w->channel] = (p->rowcc[w->channel] + 1) / 2 - 1;
														} else break;
													redraw(); break;
												case '*':
													for (i = 0; i < MAX(1, w->count); i++)
														if (p->rowcc[w->channel] < 127)
														{
															for (j = p->rowcc[w->channel] + 1; j > 0; j--)
															{
																p->rowv[w->channel][j*2] = p->rowv[w->channel][j];
																memset(&p->rowv[w->channel][j*2-1], 0, sizeof(row));
																p->rowv[w->channel][j*2-1].inst = 255;
															} p->rowcc[w->channel] = (p->rowcc[w->channel] + 1) * 2 - 1;
														} else if (p->rowcc[w->channel] < 255)
														{
															for (j = p->rowcc[w->channel] + 1; j > 0; j--)
															{
																p->rowv[w->channel][j*2] = p->rowv[w->channel][j];
																memset(&p->rowv[w->channel][j*2-1], 0, sizeof(row));
																p->rowv[w->channel][j*2-1].inst = 255;
															} p->rowcc[w->channel] = 255;
														} else break;
													redraw(); break;
											}
											p->rowc = 0;
											for (i = 0; i < MAX_CHANNELS; i++)
												p->rowc = MAX(p->rowc, p->rowcc[i]);
											w->trackerfy = MIN(p->rowc, w->trackerfy);
											break;
										case 'R': /* global row */
											p = s->patternv[s->patterni[s->songi[w->songfy]]];
											switch (input)
											{
												case 'r':
													k = p->rowcc[w->channel];
													if (w->count) for (i = 0; i < MAX_CHANNELS; i++) p->rowcc[i] = w->count - 1;
													else          for (i = 0; i < MAX_CHANNELS; i++) p->rowcc[i] = p->rowc;
													for (i = 0; i < MAX_CHANNELS; i++)
													{
														j = k;
														while (j < p->rowcc[i])
															if (j < 127)
															{
																memcpy(&p->rowv[i][j + 1], p->rowv[i],
																		sizeof(row) * (j + 1));
																i = (j + 1) * 2 - 1;
															} else
															{
																memcpy(&p->rowv[i][j + 1], p->rowv[i],
																		sizeof(row) * (255 - j));
																j = 255;
																break;
															}
													}
													redraw(); break;
												case 'd':
													for (i = 0; i < MAX_CHANNELS; i++)
														for (j = 0; j < MAX(1, w->count); j++) if (p->rowcc[i]) p->rowcc[i]--;
													redraw(); break;
												case 'a':
													for (i = 0; i < MAX_CHANNELS; i++)
														for (j = 0; j < MAX(1, w->count); j++)
															if (p->rowcc[i] < 255)
															{
																p->rowcc[i]++;
																memset(&p->rowv[i][p->rowcc[i]], 0, sizeof(row));
																p->rowv[i][p->rowcc[i]].inst = 255;
															} else break;
													redraw(); break;
												case '-':
													for (i = 0; i < MAX_CHANNELS; i++)
														for (j = 0; j < MAX(1, w->count); j++)
														{
															if (p->rowcc[i] == 255)
																p->rowcc[i] = 127;
															else if (p->rowcc[i])
																p->rowcc[i] = (p->rowcc[i] + 1) / 2 - 1;
															else break;
														}
													redraw(); break;
												case '+': case '=':
													for (i = 0; i < MAX_CHANNELS; i++)
														for (j = 0; j < MAX(1, w->count); j++)
															if (p->rowcc[i] < 127)
															{
																memcpy(&p->rowv[i][p->rowcc[i] + 1], p->rowv[i],
																		sizeof(row) * (p->rowcc[i] + 1));
																p->rowcc[i] = (p->rowcc[i] + 1) * 2 - 1;
															} else
															{
																memcpy(&p->rowv[w->channel][p->rowcc[i] + 1], p->rowv[w->channel],
																		sizeof(row) * (255 - p->rowcc[i]));
																p->rowcc[i] = 255;
																break;
															}
													redraw(); break;
												case '/':
													for (i = 0; i < MAX_CHANNELS; i++)
														for (k = 0; k < MAX(1, w->count); k++)
															if (p->rowcc[i] == 255)
															{
																for (j = 0; j < 127; j++)
																	p->rowv[i][j] = p->rowv[i][j*2];
																p->rowcc[i] = 127;
															} else if (p->rowcc[i])
															{
																for (j = 0; j < p->rowcc[i] + 1; j++)
																	p->rowv[i][j] = p->rowv[i][j*2];
																p->rowcc[i] = (p->rowcc[i] + 1) / 2 - 1;
															} else break;
													redraw(); break;
												case '*':
													for (i = 0; i < MAX_CHANNELS; i++)
														for (k = 0; k < MAX(1, w->count); k++)
															if (p->rowcc[i] < 127)
															{
																for (j = p->rowcc[i] + 1; j > 0; j--)
																{
																	p->rowv[i][j*2] = p->rowv[i][j];
																	memset(&p->rowv[i][j*2-1], 0, sizeof(row));
																	p->rowv[i][j*2-1].inst = 255;
																}
																p->rowcc[i] = (p->rowcc[i] + 1) * 2 - 1;
															} else if (p->rowcc[i] < 255)
															{
																for (j = p->rowcc[i] + 1; j > 0; j--)
																{
																	p->rowv[i][j*2] = p->rowv[i][j];
																	memset(&p->rowv[i][j*2-1], 0, sizeof(row));
																	p->rowv[i][j*2-1].inst = 255;
																}
																p->rowcc[i] = 255;
															} else break;
													redraw(); break;
											}
											p->rowc = 0;
											for (i = 0; i < MAX_CHANNELS; i++)
												p->rowc = MAX(p->rowc, p->rowcc[i]);
											w->trackerfy = MIN(p->rowc, w->trackerfy);
											break;
									} w->count = 0;
								} else
									switch (input)
									{
										case 'f': /* toggle song follow */ w->flags ^= 0b1; redraw(); break;
										case 'i': /* enter insert mode  */ w->mode = T_MODE_INSERT; redraw(); break;
										case 'v': /* enter visual mode  */
											w->visualfx = tfxToVfx(w->trackerfx);
											w->visualfy = w->trackerfy;
											w->visualchannel = w->channel;
											w->oldmode = T_MODE_NORMAL;
											w->mode = T_MODE_VISUAL;
											redraw(); break;
										case 'V': /* enter visual line mode */
											w->visualfx = tfxToVfx(w->trackerfx);
											w->visualfy = w->trackerfy;
											w->visualchannel = w->channel;
											w->oldmode = T_MODE_NORMAL;
											w->mode = T_MODE_VISUALLINE;
											redraw(); break;
										case '\t': /* enter song mode */ w->mode = T_MODE_SONG; redraw(); break;
										case 'y': /* pattern copy   */ w->chord = 'y'; redraw(); return;
										case 'd': /* pattern cut    */ w->chord = 'd'; redraw(); return;
										case 'c': /* channel        */ w->chord = 'c'; redraw(); return;
										case 'm': /* macro          */ w->chord = 'm'; redraw(); return;
										case 'k': /* keyboard macro */ w->chord = 'k'; redraw(); return;
										case 'r': /* row            */ w->chord = 'r'; redraw(); return;
										case 'R': /* global row     */ w->chord = 'R'; redraw(); return;
										case 'b': /* bpm            */ if (w->count) s->songbpm = MIN(255, MAX(32, w->count)); break;
										case 'h': /* row highlight  */ if (w->count) s->rowhighlight = MIN(16, w->count); break;
										case 'p': /* pattern put */ /* TODO: count */
											if (s->songi[w->songfy] == 255) break;
											putPartPattern();
											w->trackerfy = MIN(w->trackerfy + w->pbfy[1] - w->pbfy[0],
														s->patternv[s->patterni[s->songi[w->songfy]]]->rowc - 1);
											downArrow();
											redraw(); break;
										case 'P': /* pattern put before */ /* TODO: count */
											if (s->songi[w->songfy] == 255) break;
											putPartPattern();
											redraw(); break;
										case 's': w->mode = T_MODE_INSERT;
										case 'x': case 127: case 8: /* backspace */
											if (w->trackerfx == 0)
											{
												yankPartPattern(0, 1,
														w->trackerfy, w->trackerfy, w->channel, w->channel);
												delPartPattern(0, 1,
														w->trackerfy, w->trackerfy, w->channel, w->channel);
											} else
											{
												yankPartPattern(tfxToVfx(w->trackerfx), tfxToVfx(w->trackerfx),
														w->trackerfy, w->trackerfy, w->channel, w->channel);
												delPartPattern(tfxToVfx(w->trackerfx), tfxToVfx(w->trackerfx),
														w->trackerfy, w->trackerfy, w->channel, w->channel);
											}
											redraw(); break;
										default: /* column specific */
											modulorow = w->trackerfy % s->patternv[s->patterni[s->songi[w->songfy]]]->rowcc[w->channel];
											r = &s->patternv[s->patterni[s->songi[w->songfy]]]->rowv[w->channel][modulorow];
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
															if (r->inst == 255) r->inst = 0;
															if (w->instrumentrecv == INST_REC_LOCK_OK)
																w->instrument = r->inst;
															break;
														case 24: /* ^x */
															r->inst-=MAX(1, w->count);
															if (r->inst == 255) r->inst = 254;
															if (w->instrumentrecv == INST_REC_LOCK_OK)
																w->instrument = r->inst;
															break;
													} break;
												default:
													macro = (w->trackerfx - 2) / 2;
													switch (input)
													{
														case 1:  /* ^a */ r->macro[macro].v+=MAX(1, w->count); break;
														case 24: /* ^x */ r->macro[macro].v-=MAX(1, w->count); break;
														case '~': /* toggle case */
															if      (isupper(r->macro[macro].c)) r->macro[macro].c += 32;
															else if (islower(r->macro[macro].c)) r->macro[macro].c -= 32;
													} break;
											} redraw(); break;
									}
								break;
						} break;
					case T_MODE_SONG_VISUAL:
						switch (input)
						{
							case 'v': w->mode = T_MODE_SONG; redraw(); break;
							case 'y': /* song copy */
								w->songbufferlen = MAX(w->songfy, w->visualfy) - MIN(w->songfy, w->visualfy) +1;
								if (w->songbufferlen)
								{
									memcpy(w->songibuffer, &s->songi[MIN(w->songfy, w->visualfy)], w->songbufferlen);
									memcpy(w->songfbuffer, &s->songf[MIN(w->songfy, w->visualfy)], w->songbufferlen);
								}
								w->songfy = MIN(w->songfy, w->visualfy);
								w->mode = T_MODE_SONG;
								redraw(); break;
							case 'd': case 'x': /* song delete */
								w->songbufferlen = MAX(w->songfy, w->visualfy) - MIN(w->songfy, w->visualfy) +1;
								if (w->songbufferlen)
								{
									memcpy(w->songibuffer, &s->songi[MIN(w->songfy, w->visualfy)], w->songbufferlen);
									memcpy(w->songfbuffer, &s->songf[MIN(w->songfy, w->visualfy)], w->songbufferlen);
									for (i = 0; i < w->songbufferlen; i++)
									{
										s->songi[MIN(w->songfy, w->visualfy)+i] = 255;
										s->songf[MIN(w->songfy, w->visualfy)+i] = 0;
										if (w->songnext == MIN(w->songfy, w->visualfy)+i + 1)
											w->songnext = 0;
									}
								}
								w->songfy = MIN(w->songfy, w->visualfy);
								w->mode = T_MODE_SONG;
								redraw(); break;
							case 'l': /* block loop */
								for (i = 0; i < MAX(w->songfy, w->visualfy) - MIN(w->songfy, w->visualfy) +1; i++)
									if (s->songi[MIN(w->songfy, w->visualfy)+i] != 255)
										s->songf[MIN(w->songfy, w->visualfy)+i] = !s->songf[MIN(w->songfy, w->visualfy)+i];
								redraw(); break;
						} break;
					case T_MODE_SONG_INSERT:
						switch (input)
						{
							case '0':           inputSongHex(0);  break;
							case '1':           inputSongHex(1);  break;
							case '2':           inputSongHex(2);  break;
							case '3':           inputSongHex(3);  break;
							case '4':           inputSongHex(4);  break;
							case '5':           inputSongHex(5);  break;
							case '6':           inputSongHex(6);  break;
							case '7':           inputSongHex(7);  break;
							case '8':           inputSongHex(8);  break;
							case '9':           inputSongHex(9);  break;
							case 'A': case 'a': inputSongHex(10); break;
							case 'B': case 'b': inputSongHex(11); break;
							case 'C': case 'c': inputSongHex(12); break;
							case 'D': case 'd': inputSongHex(13); break;
							case 'E': case 'e': inputSongHex(14); break;
							case 'F': case 'f': inputSongHex(15); break;
							case 127: case 8: /* backspace */
								if (w->songnext == w->songfy + 1)
									w->songnext = 0;
								s->songi[w->songfy] = 255;
								s->songf[w->songfy] = 0;
								prunePattern(s->songi[w->songfy], w->songfy);
								redraw(); break;
						} break;
					case T_MODE_INSERT:
						if (s->songi[w->songfy] == 255) break;
						modulorow = w->trackerfy % (s->patternv[s->patterni[s->songi[w->songfy]]]->rowcc[w->channel]+1);
						r = &s->patternv[s->patterni[s->songi[w->songfy]]]->rowv[w->channel][modulorow];
						switch (w->trackerfx)
						{
							case 0: /* note */
								switch (input)
								{
									case 1:  /* ^a */ r->note+=MAX(1, w->count); break;
									case 24: /* ^x */ r->note-=MAX(1, w->count); break;
									case ' ': /* space */
										r->note = 255;
										w->trackerfy += w->step;
										if (w->trackerfy > s->patternv[s->patterni[s->songi[w->songfy]]]->rowc)
										{
											if (w->songfy < 255 && s->songi[w->songfy + 1] != 255)
											{
												w->trackerfy = 0;
												w->songfy++;
											} else w->trackerfy = s->patternv[s->patterni[s->songi[w->songfy]]]->rowc;
										} break;
									case 127: case 8: /* backspace */
										r->note = 0;
										r->inst = 255;
										w->trackerfy -= w->step;
										if (w->trackerfy < 0)
										{
											if (w->songfy > 0 && s->songi[w->songfy - 1] != 255)
											{
												w->songfy--;
												w->trackerfy = s->patternv[s->patterni[s->songi[w->songfy]]]->rowc;
											} else w->trackerfy = 0;
										} break;
									case '0': r->note = changeNoteOctave(0, r->note); break;
									case '1': r->note = changeNoteOctave(1, r->note); break;
									case '2': r->note = changeNoteOctave(2, r->note); break;
									case '3': r->note = changeNoteOctave(3, r->note); break;
									case '4': r->note = changeNoteOctave(4, r->note); break;
									case '5': r->note = changeNoteOctave(5, r->note); break;
									case '6': r->note = changeNoteOctave(6, r->note); break;
									case '7': r->note = changeNoteOctave(7, r->note); break;
									case '8': r->note = changeNoteOctave(8, r->note); break;
									case '9': r->note = changeNoteOctave(9, r->note); break;
									default:
										note = charToNote(input);
										if (note) /* ignore nothing */
										{
											if (w->instrumentrecv == INST_REC_LOCK_OK)
												r->inst = w->instrument;
											previewNote(note, r->inst, w->channel);
											if (note == 255) r->note = 255;
											else switch (w->keyboardmacro)
											{
												case 0: r->note = note; break;
												case 'G': /* m.x and m.y are note */
													r->note = C5;
													note -= w->octave*12;
													if (note>=13 && note<=20)
													{
														r->macro[0].c = w->keyboardmacro;
														r->macro[0].v = (note - 13) * 16 + (note - 13);
													}
													if (note>=25 && note<=32)
													{
														r->macro[0].c = w->keyboardmacro;
														r->macro[0].v = (note - 17) * 16 + (note - 17);
													}
													break;
												default: /* m.x is note */
													r->note = C5;
													note -= w->octave*12;
													if (note>=13 && note<=20)
													{
														r->macro[0].c = w->keyboardmacro;
														r->macro[0].v = (note - 13) * 16;
													}
													if (note>=25 && note<=32)
													{
														r->macro[0].c = w->keyboardmacro;
														r->macro[0].v = (note - 17) * 16;
													}
													break;
											}
										}
										w->trackerfy += w->step;
										if (w->trackerfy > s->patternv[s->patterni[s->songi[w->songfy]]]->rowc)
										{
											if (w->songfy < 255 && s->songi[w->songfy + 1] != 255)
											{
												w->trackerfy = 0;
												w->songfy++;
											} else w->trackerfy = s->patternv[s->patterni[s->songi[w->songfy]]]->rowc;
										} break;
								} break;
							case 1: /* instrument */
								switch (input)
								{
									case 1: /* ^a */ r->inst+=MAX(1, w->count);
										if (r->inst == 255) r->inst = 0;
										if (w->instrumentrecv == INST_REC_LOCK_OK)
											w->instrument = r->inst;
										break;
									case 24: /* ^x */ r->inst-=MAX(1, w->count);
										if (r->inst == 255) r->inst = 254;
										if (w->instrumentrecv == INST_REC_LOCK_OK)
											w->instrument = r->inst;
										break;
									case ' ':         /* space     */ r->inst = w->instrument; break;
									case 127: case 8: /* backspace */ r->inst = 255; break;
									case '0':           inputPatternHex(r, 0);  if (w->instrumentrecv == INST_REC_LOCK_OK) w->instrument = r->inst; break;
									case '1':           inputPatternHex(r, 1);  if (w->instrumentrecv == INST_REC_LOCK_OK) w->instrument = r->inst; break;
									case '2':           inputPatternHex(r, 2);  if (w->instrumentrecv == INST_REC_LOCK_OK) w->instrument = r->inst; break;
									case '3':           inputPatternHex(r, 3);  if (w->instrumentrecv == INST_REC_LOCK_OK) w->instrument = r->inst; break;
									case '4':           inputPatternHex(r, 4);  if (w->instrumentrecv == INST_REC_LOCK_OK) w->instrument = r->inst; break;
									case '5':           inputPatternHex(r, 5);  if (w->instrumentrecv == INST_REC_LOCK_OK) w->instrument = r->inst; break;
									case '6':           inputPatternHex(r, 6);  if (w->instrumentrecv == INST_REC_LOCK_OK) w->instrument = r->inst; break;
									case '7':           inputPatternHex(r, 7);  if (w->instrumentrecv == INST_REC_LOCK_OK) w->instrument = r->inst; break;
									case '8':           inputPatternHex(r, 8);  if (w->instrumentrecv == INST_REC_LOCK_OK) w->instrument = r->inst; break;
									case '9':           inputPatternHex(r, 9);  if (w->instrumentrecv == INST_REC_LOCK_OK) w->instrument = r->inst; break;
									case 'A': case 'a': inputPatternHex(r, 10); if (w->instrumentrecv == INST_REC_LOCK_OK) w->instrument = r->inst; break;
									case 'B': case 'b': inputPatternHex(r, 11); if (w->instrumentrecv == INST_REC_LOCK_OK) w->instrument = r->inst; break;
									case 'C': case 'c': inputPatternHex(r, 12); if (w->instrumentrecv == INST_REC_LOCK_OK) w->instrument = r->inst; break;
									case 'D': case 'd': inputPatternHex(r, 13); if (w->instrumentrecv == INST_REC_LOCK_OK) w->instrument = r->inst; break;
									case 'E': case 'e': inputPatternHex(r, 14); if (w->instrumentrecv == INST_REC_LOCK_OK) w->instrument = r->inst; break;
									case 'F': case 'f': inputPatternHex(r, 15); if (w->instrumentrecv == INST_REC_LOCK_OK) w->instrument = r->inst; break;
								} break;
							default:
								macro = (w->trackerfx - 2) / 2;
								if (w->trackerfx % 2 == 0)
									switch (input)
									{
										case 127: case 8: /* backspace */
											r->macro[macro].c = 0;
											break;
										default:
											if (!r->macro[macro].c)
												r->macro[macro].v = 0;
											changeMacro(input, &r->macro[macro].c);
											break;
									}
								else
								{
									if (r->macro[macro].c)
										switch (input)
										{
											case 1:  /* ^a */ r->macro[macro].v+=MAX(1, w->count); break;
											case 24: /* ^x */ r->macro[macro].v-=MAX(1, w->count); break;
											case 127: case 8: /* backspace */ r->macro[macro].c = 0; break;
											case '0':           inputPatternHex(r, 0);  break;
											case '1':           inputPatternHex(r, 1);  break;
											case '2':           inputPatternHex(r, 2);  break;
											case '3':           inputPatternHex(r, 3);  break;
											case '4':           inputPatternHex(r, 4);  break;
											case '5':           inputPatternHex(r, 5);  break;
											case '6':           inputPatternHex(r, 6);  break;
											case '7':           inputPatternHex(r, 7);  break;
											case '8':           inputPatternHex(r, 8);  break;
											case '9':           inputPatternHex(r, 9);  break;
											case 'A': case 'a': inputPatternHex(r, 10); break;
											case 'B': case 'b': inputPatternHex(r, 11); break;
											case 'C': case 'c': inputPatternHex(r, 12); break;
											case 'D': case 'd': inputPatternHex(r, 13); break;
											case 'E': case 'e': inputPatternHex(r, 14); break;
											case 'F': case 'f': inputPatternHex(r, 15); break;
										}
									else
									{
										r->macro[macro].v = 0;
										changeMacro(input, &r->macro[macro].c);
									}
								} break;
						} redraw(); break;
				} break;
	}
	w->count = 0;
	if (w->chord) { w->chord = '\0'; redraw(); }
}
