/* will populate buffer, buffer should be at least length 4 */
static void noteToString(uint8_t note, char *buffer)
{
	if (note == NOTE_VOID) { strcpy(buffer, "..."); return; }
	if (note == NOTE_OFF)  { strcpy(buffer, "==="); return; }

	char *strnote;
	char octave;
	switch (note % 12)
	{
		case 0:  strnote = "C-"; break;
		case 1:  strnote = "C#"; break;
		case 2:  strnote = "D-"; break;
		case 3:  strnote = "D#"; break;
		case 4:  strnote = "E-"; break;
		case 5:  strnote = "F-"; break;
		case 6:  strnote = "F#"; break;
		case 7:  strnote = "G-"; break;
		case 8:  strnote = "G#"; break;
		case 9:  strnote = "A-"; break;
		case 10: strnote = "A#"; break;
		case 11: strnote = "B-"; break;
		default: strnote = "?-"; break;
	}

	switch (note / 12)
	{
		case 0:  octave = '0'; break;
		case 1:  octave = '1'; break;
		case 2:  octave = '2'; break;
		case 3:  octave = '3'; break;
		case 4:  octave = '4'; break;
		case 5:  octave = '5'; break;
		case 6:  octave = '6'; break;
		case 7:  octave = '7'; break;
		case 8:  octave = '8'; break;
		case 9:  octave = '9'; break;
		default: octave = '?'; break;
	}
	snprintf(buffer, 5, "%s%c", strnote, octave);
}

/* generate sfx using dynamic width tracks */
short genSfx(short minx)
{
	short x = minx<<1;
	short ret = 0;
	short trackw;

	for (int i = 0; i < s->track->c; i++)
	{
		trackw = TRACK_TRIG_PAD + 11 + 4*(s->track->v[i].data.variant->macroc+1);
		x += trackw;
		if (i == w->track) ret = x - (trackw>>1); /* should only be set once */
		/* keep iterating so x is the full width of all tracks */
	}
	if (x > ws.ws_col)
	{
		ret = (ws.ws_col>>1) - ret;
		ret = MIN(ret, 0);
		ret = MAX(ret, -(x - ws.ws_col));
	} else
		ret = ((ws.ws_col - x)>>1);
	return ret;
}
/* generate sfx using constant width tracks */
short genConstSfx(short trackw)
{
	short x = 0;
	short ret = 0;

	for (int i = 0; i < s->track->c; i++)
	{
		x += trackw;
		if (i == w->track) ret = x - (trackw>>1); /* should only be set once */
		/* keep iterating so x is the full width of all tracks */
	}
	if (x > ws.ws_col)
	{
		ret = (ws.ws_col>>1) - ret;
		ret = MIN(ret, 0);
		ret = MAX(ret, -(x - ws.ws_col));
	} else
		ret = ((ws.ws_col - x)>>1);
	return ret;
}

static void drawGlobalLineNumbers(short x, short minx, short maxx)
{
	char buffer[5];
	short y;
	for (int i = 0; i < s->songlen; i++)
	{
		y = w->centre - w->trackerfy + i;
		if (y > TRACK_ROW && y < ws.ws_row)
		{
			if (s->playing == PLAYING_CONT && s->playfy == i)                              printf("\033[1m");
			else if (i < STATE_ROWS || (s->loop[1] && (i < s->loop[0] || i > s->loop[1]))) printf("\033[2m");

			/* print as special if the row has a bpm change, or if the row hasn't been fully allocated yet */
			if (s->bpmcachelen <= i || s->bpmcache[i] != -1) printf("\033[33m");

			if (i < STATE_ROWS) snprintf(buffer, 5, "  -%x", STATE_ROWS - i);
			else                snprintf(buffer, 5, "%04x",  i - STATE_ROWS);

			printCulling(buffer, x, y, minx, maxx);
			printf("\033[m");
		}
	}
}

static bool startVisual(uint8_t track, int i, int8_t fieldpointer)
{
	switch (w->mode)
	{
		case T_MODE_VISUAL:
			if (track == MIN(w->visualtrack, w->track)
					&& i >= MIN(w->visualfy, w->trackerfy+w->fyoffset)
					&& i <= MAX(w->visualfy, w->trackerfy+w->fyoffset)
					&& ((w->visualtrack == w->track && fieldpointer == vfxVmoMin(w->visualfx, tfxToVfx(w->trackerfx)))
					||  (w->visualtrack != w->track && fieldpointer == ((w->visualtrack <= w->track) ? w->visualfx : tfxToVfx(w->trackerfx)))))
			{
					printf("\033[2;7m");
					return 1;
			} break;
		case T_MODE_VISUALREPLACE:
			if (track == w->track && fieldpointer == tfxToVfx(w->trackerfx)
					&& i >= MIN(w->visualfy, w->trackerfy+w->fyoffset)
					&& i <= MAX(w->visualfy, w->trackerfy+w->fyoffset))
			{
				printf("\033[2;7m");
				return 1;
			} break;
		case T_MODE_VISUALLINE:
			if (track == MIN(w->visualtrack, w->track) && fieldpointer == 0
					&& i >= MIN(w->visualfy, w->trackerfy+w->fyoffset)
					&& i <= MAX(w->visualfy, w->trackerfy+w->fyoffset))
			{
				printf("\033[2;7m");
				return 1;
			} break;
		default: break;
	} return 0;
}
static bool stopVisual(uint8_t track, int i, int8_t fieldpointer)
{
	switch (w->mode)
	{
		case T_MODE_VISUAL:
			if (track == MAX(w->visualtrack, w->track)
					&& i >= MIN(w->visualfy, w->trackerfy+w->fyoffset)
					&& i <= MAX(w->visualfy, w->trackerfy+w->fyoffset)
					&& ((w->visualtrack == w->track && fieldpointer == vfxVmoMax(w->visualfx, tfxToVfx(w->trackerfx)))
					||  (w->visualtrack != w->track && fieldpointer == ((w->visualtrack >= w->track) ? w->visualfx : tfxToVfx(w->trackerfx)))))
			{
				printf("\033[22;27m");
				return 1;
			} break;
		case T_MODE_VISUALREPLACE:
			if (track == w->track && fieldpointer == tfxToVfx(w->trackerfx)
					&& i >= MIN(w->visualfy, w->trackerfy+w->fyoffset)
					&& i <= MAX(w->visualfy, w->trackerfy+w->fyoffset))
			{
				printf("\033[22;27m");
				return 1;
			} break;
		default: break;
	} return 0;
}
static bool ifVisual(uint8_t track, int i, int8_t fieldpointer)
{
	if (track >= MIN(w->visualtrack, w->track)
	 && track <= MAX(w->visualtrack, w->track))
	{
		switch (w->mode)
		{
			case T_MODE_VISUAL:
				if (i >= MIN(w->visualfy, w->trackerfy+w->fyoffset)
				 && i <= MAX(w->visualfy, w->trackerfy+w->fyoffset))
				{
					if (w->visualtrack == w->track)
					{
						if (vfxVmoRangeIncl(
								vfxVmoMin(w->visualfx, tfxToVfx(w->trackerfx)),
								vfxVmoMax(w->visualfx, tfxToVfx(w->trackerfx)),
								fieldpointer))
							return 1;
					} else
					{
						if (track > MIN(w->visualtrack, w->track)
						 && track < MAX(w->visualtrack, w->track)) /* middle track */
							return 1;
						else if (track == MIN(w->visualtrack, w->track)) /* first track */
						{
							if (vfxVmoRangeIncl(
									w->visualtrack == track ? w->visualfx : tfxToVfx(w->trackerfx),
									1 + s->track->v[track].data.variant->macroc,
									fieldpointer))
								return 1;
						} else if (track == MAX(w->visualtrack, w->track)) /* last track */
						{
							if (vfxVmoRangeIncl(
									TRACKERFX_MIN,
									w->visualtrack == track ? w->visualfx : tfxToVfx(w->trackerfx),
									fieldpointer))
								return 1;
						}
					}
				} break;
			case T_MODE_VISUALREPLACE:
				if (track == w->track && fieldpointer == tfxToVfx(w->trackerfx)
						&& i >= MIN(w->visualfy, w->trackerfy+w->fyoffset)
						&& i <= MAX(w->visualfy, w->trackerfy+w->fyoffset))
					return 1;
				break;
			case T_MODE_VISUALLINE:
				if (fieldpointer != -1
						&& i >= MIN(w->visualfy, w->trackerfy+w->fyoffset)
						&& i <= MAX(w->visualfy, w->trackerfy+w->fyoffset))
					return 1;
				break;
			default: break;
		}
	} return 0;
}

static void setRowIntensity(TrackData *cd, int i)
{
	if (cd->mute || i < STATE_ROWS || (s->loop[1] && (i < s->loop[0] || i > s->loop[1]))) printf("\033[2m");
	else if (s->playing == PLAYING_CONT && s->playfy == i)                                printf("\033[1m");
}

static void drawStarColumn(short x, short minx, short maxx)
{
	char buffer[4];
	for (int i = 0; i < s->songlen; i++)
		if (w->centre - w->trackerfy + i > TRACK_ROW && w->centre - w->trackerfy + i < ws.ws_row)
		{
			if (i < STATE_ROWS)                                              strcpy(buffer, " ");
			else if (s->playing == PLAYING_CONT && s->playfy == i)           strcpy(buffer, "-");
			else if (s->rowhighlight && !((i-STATE_ROWS) % s->rowhighlight)) strcpy(buffer, "*");
			else                                                             strcpy(buffer, " ");
			printCulling(buffer, x, w->centre - w->trackerfy + i, minx, maxx);
		}
}

/* minx is the xpos where the start of the track will get clipped at */
#define TRACK_HEADER_LEN 8
static void drawTrackHeader(uint8_t track, short x, short minx, short maxx)
{
	TrackData *cd = &s->track->v[track].data;

	char headerbuffer[9];
	snprintf(headerbuffer, 9, "TRACK %02x", track);

	if (cd->mute) printf("\033[2m");
	else          printf("\033[1m");
	if (s->track->v[track].triggerflash) printf("\033[3%dm", track%6 + 1);
	if (track == w->track + w->trackoffset) printf("\033[7m");
	printCulling(headerbuffer, x, TRACK_ROW, minx, maxx);

	printf("\033[m");
}

static short drawTrack(uint8_t track, short bx, short minx, short maxx)
{
	if (track >= s->track->c) return 0;

	Row *r;
	// char buffer[16];
	char *buffer = calloc(16, sizeof(char));

	// Variant *v;
	// int gcvret;
	TrackData *cd = &s->track->v[track].data;

	short x, y;
	int lineno;
	if (bx <= maxx && bx + TRACK_TRIG_PAD + 12 + 4*(cd->variant->macroc+1) > minx)
	{
		for (int i = 0; i < s->songlen; i++)
		{
			y = w->centre - w->trackerfy + i;
			if (y > TRACK_ROW && y < ws.ws_row)
			{
				x = bx;
				lineno = getVariantChainVariant(NULL, cd->variant, i);

				if (ifVisual(track, i, -1)) printf("\033[2;7m");

				if (cd->variant->trig[i].index == VARIANT_OFF)
				{
					printf("\033[1m");
					strcpy(buffer, " ==  ");
				} else if (cd->variant->trig[i].index != VARIANT_VOID)
				{
					printf("\033[1;3%dm", cd->variant->trig[i].index%6 + 1);
					if (cd->variant->trig[i].flags&C_VTRIG_LOOP) snprintf(buffer, 6, "l%02x  ", cd->variant->trig[i].index);
					else                                         snprintf(buffer, 6, " %02x  ", cd->variant->trig[i].index);
				} else
				{
					printf("\033[2m");
					lineno = getVariantChainVariant(NULL, cd->variant, i);
					if (lineno == -1) strcpy(buffer, " ..  ");
					else              snprintf(buffer, 6, " %02x  ", lineno);
				}
				printCulling(buffer, x, y, minx, maxx);

				if (x+5 < maxx && x+4 > minx)
				{ /* box drawing variant range thing idk what to call it tbh */
				  /* drawn separately cos multibyte chars break things       */
				  /* TODO: should use printCulling() or otherwise have a \033[y;xH write */
					if (lineno != -1)
					{
						// bgTuple(&tc, greyscaleTuple(&tc, lineno*DIV256));
						printf("\033[1D\033[22;3%dm│", cd->variant->trig[getVariantChainPrevVtrig(cd->variant, i)].index%6 + 1);
					}
				}

				stopVisual(track, i, -1);
				printf("\033[22;37;40m");

				x += 3 + TRACK_TRIG_PAD;

				r = getTrackRow(cd, i);
				setRowIntensity(cd, i);

				/* use underline to split up variants more visually */
				// gcvret = getVariantChainVariant(&v, cd->variant, i);
				// if ((i < s->songlen-1 && (cd->variant->trig[i+1].index < VARIANT_MAX || (cd->variant->trig[i+1].index == VARIANT_OFF && gcvret != -1)))
				// 		|| (gcvret != -1 && (gcvret%(v->rowc+1) == v->rowc)))
				// 	printf("\033[1;%d]\033[4m", cd->variant->trig[i+1].index%6 + 1); /* linux console-specific underline colour setting, TODO: untested */

				// if (lineno != -1)
				// {
				// 	if (i >= STATE_ROWS && s->rowhighlight && !((i-STATE_ROWS) % s->rowhighlight))
				// 		bgTuple(&tc, lerpTuple(0.1f, greyscaleTuple(&tc, 0.05f), tc.colour[cd->variant->trig[getVariantChainPrevVtrig(cd->variant, i)].index%6 + 1]));
				// 	else
				// 		bgTuple(&tc, lerpTuple(0.1f, tc.colour[0], tc.colour[cd->variant->trig[getVariantChainPrevVtrig(cd->variant, i)].index%6 + 1]));
				// } else
				// 	if (i >= STATE_ROWS && s->rowhighlight && !((i-STATE_ROWS) % s->rowhighlight))
				// 		bgTuple(&tc, greyscaleTuple(&tc, 0.05f));

				if (ifVisual(track, i, 0)) printf("\033[2;7m");

				if (x <= maxx)
				{
					if (r->note != NOTE_VOID)
					{
						noteToString(r->note, buffer);
						if (cd->mute) printCulling(buffer, x, y, minx, maxx);
						else
						{
							if (ifVisual(track, i, 0))
							{
								printf("\033[22m");
								if (s->playing == PLAYING_CONT && s->playfy == i) printf("\033[1m");
							}
							if (instrumentSafe(s->instrument, r->inst)) printf("\033[3%dm", r->inst%6 + 1);
							else                            printf("\033[37m");
							printCulling(buffer, x, y, minx, maxx);
							printf("\033[37m");
							if (ifVisual(track, i, 0)) printf("\033[2m");
						}
					} else
					{
						strcpy(buffer, "...");
						printCulling(buffer, x, y, minx, maxx);
					}
					stopVisual(track, i, 0);
					setRowIntensity(cd, i);
				}

				x+=3;

				if (x <= maxx)
				{
					snprintf(buffer, 4, " %02x", r->inst);
					startVisual(track, i, 1);
					if (r->inst != INST_VOID)
					{
						if (!cd->mute)
						{
							if (ifVisual(track, i, 1))
							{
								printf("\033[22m");
								setRowIntensity(cd, i);
							}
							if (instrumentSafe(s->instrument, r->inst)) printf("\033[3%dm", r->inst%6 + 1);
							else                            printf("\033[37m");
							printCulling(buffer, x, y, minx, maxx);
							printf("\033[22;37m");
							if (ifVisual(track, i, 1)) printf("\033[2m");
						} else printCulling(buffer, x, y, minx, maxx);
					} else
					{
						strcpy(buffer, " ..");
						printCulling(buffer, x, y, minx, maxx);
					}
					stopVisual(track, i, 1);
					setRowIntensity(cd, i);
				}

				x+=3;

				for (int j = cd->variant->macroc; j >= 0; j--)
				{
					if (x <= maxx)
					{
						startVisual(track, i, 2+j);
						if (r->macro[j].c)
						{
							if (ifVisual(track, i, 2+j))
							{
								printf("\033[22m");
								setRowIntensity(cd, i);
							}

							snprintf(buffer, 5, " %c%02x", r->macro[j].c, r->macro[j].v);
							if (cd->mute)
								printCulling(buffer, x, y, minx, maxx);
							else
							{
								/* TODO: macro colour groups */
								printf("\033[35m");
								printCulling(buffer, x, y, minx, maxx);
							}

							if (ifVisual(track, i, 2+j)) printf("\033[23;2;7;37m");
							else                         printf("\033[23;27;37m");
						} else
						{
							strcpy(buffer, " ...");
							printCulling(buffer, x, y, minx, maxx);
						}
						stopVisual(track, i, 2+j);
						setRowIntensity(cd, i);
					}
					x += 4;
				}

				printf("\033[m"); /* clear attributes before row highlighting */
			}
		}
	}
	free(buffer);
	return TRACK_TRIG_PAD + 11 + 4*(cd->variant->macroc+1);
}

/* returns the xpos at the start of the cursor track */
static short drawTrackerBody(short sfx, short x, short minx, short maxx)
{
	short ret = 0;

	drawGlobalLineNumbers(x+MAX(sfx, 0), 1+MAX(sfx, 0), minx+MAX(sfx, 0));
	x += TRACK_LINENO_COLS;
	drawStarColumn(x+sfx + 1, minx, maxx);
	x += 2;

	for (int i = 0; i < s->track->c; i++)
	{
		if (i == w->track + w->trackoffset)
			ret = x;

		x += drawTrack(i, x+sfx, minx, maxx);
		drawStarColumn(x+sfx - 1, minx, maxx);
	}
	if (sfx < 0)
	{
		for (int i = 0; i < s->songlen; i++)
			if (w->centre - w->trackerfy + i > TRACK_ROW && w->centre - w->trackerfy + i < ws.ws_row)
				printf("\033[%d;%dH^", w->centre - w->trackerfy + i, minx);
	}
	if (x + sfx > maxx)
	{
		for (int i = 0; i < s->songlen; i++)
			if (w->centre - w->trackerfy + i > TRACK_ROW && w->centre - w->trackerfy + i < ws.ws_row)
				printf("\033[%d;%dH$", w->centre - w->trackerfy + i, maxx);
	}

	return ret;
}

void drawTracker(void)
{
	short x = 1;
	short sx = 0;
	short y, macro;

	short sfx;
	Row *r;

	switch (w->page)
	{
		case PAGE_TRACK_VARIANT:
			sfx = genSfx(TRACK_LINENO_COLS);
			x = 1 + TRACK_LINENO_COLS + 2 + sfx;
			for (uint8_t i = 0; i < s->track->c; i++)
			{
				drawTrackHeader(i, x + TRACK_TRIG_PAD+3 +((6 + 4*(s->track->v[i].data.variant->macroc+1) - TRACK_HEADER_LEN)>>1),
						LINENO_COLS, ws.ws_col);

				x += TRACK_TRIG_PAD + 11 + 4*(s->track->v[i].data.variant->macroc+1);
			}

			sx = drawTrackerBody(sfx, 1, LINENO_COLS-1, ws.ws_col); /* TODO: the -1 here is a dirty hack */
			// drawInstrumentIndex(ws.ws_col - 2);

			y = w->centre + w->fyoffset;

			if (w->trackerfx > 1 && w->mode == T_MODE_INSERT)
			{
				r = getTrackRow(&s->track->v[w->track].data, w->trackerfy);
				macro = (w->trackerfx - 2)>>1;
				descMacro(r->macro[macro].c, r->macro[macro].v);
			}

			switch (w->mode)
			{
				case T_MODE_VISUAL:        printf("\033[%d;0H\033[1m-- VISUAL --\033[m",                 ws.ws_row); w->command.error[0] = '\0'; break;
				case T_MODE_VISUALLINE:    printf("\033[%d;0H\033[1m-- VISUAL LINE --\033[m\033[4 q",    ws.ws_row); w->command.error[0] = '\0'; break;
				case T_MODE_VISUALREPLACE: printf("\033[%d;0H\033[1m-- VISUAL REPLACE --\033[m\033[4 q", ws.ws_row); w->command.error[0] = '\0'; break;
				case T_MODE_MOUSEADJUST:   printf("\033[%d;0H\033[1m-- MOUSE ADJUST --\033[m\033[4 q",   ws.ws_row); w->command.error[0] = '\0'; break;
				case T_MODE_INSERT:
					if (w->keyboardmacro)
						printf("\033[%d;0H\033[1m-- INSERT (%cxx) --\033[m", ws.ws_row, w->keyboardmacro);
					else
						printf("\033[%d;0H\033[1m-- INSERT --\033[m", ws.ws_row);
					printf("\033[6 q");
					w->command.error[0] = '\0'; break;
				default: break;
			}

			switch (w->trackerfx)
			{
				case -1: printf("\033[%d;%dH", y, sx+sfx + 2 - w->fieldpointer); break;
				case  0: printf("\033[%d;%dH", y, sx+sfx + 3 + TRACK_TRIG_PAD); break;
				case  1: printf("\033[%d;%dH", y, sx+sfx + 8 + TRACK_TRIG_PAD - w->fieldpointer); break;
				default: /* macro columns */
					macro = (w->trackerfx - 2)>>1;
					if (w->trackerfx % 2 == 0) printf("\033[%d;%dH", y, sx+sfx + 10 + TRACK_TRIG_PAD + (s->track->v[w->track+w->trackoffset].data.variant->macroc - macro)*4);
					else                       printf("\033[%d;%dH", y, sx+sfx + 12 + TRACK_TRIG_PAD + (s->track->v[w->track+w->trackoffset].data.variant->macroc - macro)*4 - w->fieldpointer);
					break;
			} break;
		case PAGE_TRACK_EFFECT:
			x = genConstSfx(MIN_EFFECT_WIDTH)+2;

			clearControls(&cc);

			for (int i = 0; i < s->track->c; i++)
			{
				drawTrackHeader(i, x + ((MIN_EFFECT_WIDTH - TRACK_HEADER_LEN)>>1)-1, 1, ws.ws_col);
				if (i == w->track+w->trackoffset)
					sx = x;
				else
					drawEffects(s->track->v[i].data.effect, &cc, 0, x, MIN_EFFECT_WIDTH-2, TRACK_ROW + 1);

				x += MIN_EFFECT_WIDTH;
			}

			drawEffects(s->track->v[w->track+w->trackoffset].data.effect, &cc, 1, sx, MIN_EFFECT_WIDTH-2, TRACK_ROW + 1);

			drawControls(&cc);

			break;
		default: break;
	}
}
