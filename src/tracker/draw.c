/* will populate buffer, buffer should be at least length 4 */
/* uses a short for note for safe arithmatic */
static void noteToString(short note, char *buffer)
{
	switch (note)
	{
		case NOTE_VOID: strcpy(buffer, "..."); return;
		case NOTE_OFF:  strcpy(buffer, "==="); return;
		case NOTE_CUT:  strcpy(buffer, "^^^"); return;
	}

	note -= 1; /* pop off NOTE_VOID */

	char *strnote;
	if (note < NOTE_MAX) /* mindful that note has been offset down once */
		switch (note % 12)
		{
			case 0:  strnote = "A-"; break;
			case 1:  strnote = "A#"; break;
			case 2:  strnote = "B-"; break;
			case 3:  strnote = "C-"; break;
			case 4:  strnote = "C#"; break;
			case 5:  strnote = "D-"; break;
			case 6:  strnote = "D#"; break;
			case 7:  strnote = "E-"; break;
			case 8:  strnote = "F-"; break;
			case 9:  strnote = "F#"; break;
			case 10: strnote = "G-"; break;
			case 11: strnote = "G#"; break;
			default: strnote = "?-"; break;
		}
	else /* smooth */
		switch (note % 12)
		{
			case 0:  strnote = "a-"; break;
			case 1:  strnote = "a#"; break;
			case 2:  strnote = "b-"; break;
			case 3:  strnote = "c-"; break;
			case 4:  strnote = "c#"; break;
			case 5:  strnote = "d-"; break;
			case 6:  strnote = "d#"; break;
			case 7:  strnote = "e-"; break;
			case 8:  strnote = "f-"; break;
			case 9:  strnote = "f#"; break;
			case 10: strnote = "g-"; break;
			case 11: strnote = "g#"; break;
			default: strnote = "/-"; break;
		}

	snprintf(buffer, 5, "%s%1x", strnote, (note%NOTE_MAX) / 12);
}

/* generate sfx using dynamic width tracks */
short genSfx(short minx)
{
	short x = minx<<1;
	short ret = 0;
	short trackw;

	for (int i = 0; i < s->track->c; i++)
	{
		trackw = TRACK_TRIG_PAD + 11 + 4*(s->track->v[i]->variant->macroc+1);
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
			if (w->playing && w->playfy == i)                                              printf("\033[1m");
			else if (i < STATE_ROWS || (s->loop[1] && (i < s->loop[0] || i > s->loop[1]))) printf("\033[2m");

			/* print as special if the row has a bpm change, or if the row hasn't been fully allocated yet (rows allocate pretty slowly sometimes) */
			if (w->bpmcachelen <= i || w->bpmcache[i] != -1) printf("\033[33m");

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
		case MODE_VISUAL:
			if (track == MIN(w->visualtrack, w->track)
					&& i >= MIN(w->visualfy, w->trackerfy+w->fyoffset)
					&& i <= MAX(w->visualfy, w->trackerfy+w->fyoffset)
					&& ((w->visualtrack == w->track && fieldpointer == vfxVmoMin(w->visualfx, tfxToVfx(w->trackerfx)))
					||  (w->visualtrack != w->track && fieldpointer == ((w->visualtrack <= w->track) ? w->visualfx : tfxToVfx(w->trackerfx)))))
			{
					printf("\033[2;7m");
					return 1;
			} break;
		case MODE_VISUALREPLACE:
			if (track == w->track && fieldpointer == tfxToVfx(w->trackerfx)
					&& i >= MIN(w->visualfy, w->trackerfy+w->fyoffset)
					&& i <= MAX(w->visualfy, w->trackerfy+w->fyoffset))
			{
				printf("\033[2;7m");
				return 1;
			} break;
		case MODE_VISUALLINE:
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
		case MODE_VISUAL:
			if (track == MAX(w->visualtrack, w->track)
					&& i >= MIN(w->visualfy, w->trackerfy+w->fyoffset)
					&& i <= MAX(w->visualfy, w->trackerfy+w->fyoffset)
					&& ((w->visualtrack == w->track && fieldpointer == vfxVmoMax(w->visualfx, tfxToVfx(w->trackerfx)))
					||  (w->visualtrack != w->track && fieldpointer == ((w->visualtrack >= w->track) ? w->visualfx : tfxToVfx(w->trackerfx)))))
			{
				printf("\033[22;27m");
				return 1;
			} break;
		case MODE_VISUALREPLACE:
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
			case MODE_VISUAL:
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
									1 + s->track->v[track]->variant->macroc,
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
			case MODE_VISUALREPLACE:
				if (track == w->track && fieldpointer == tfxToVfx(w->trackerfx)
						&& i >= MIN(w->visualfy, w->trackerfy+w->fyoffset)
						&& i <= MAX(w->visualfy, w->trackerfy+w->fyoffset))
					return 1;
				break;
			case MODE_VISUALLINE:
				if (fieldpointer != -1
						&& i >= MIN(w->visualfy, w->trackerfy+w->fyoffset)
						&& i <= MAX(w->visualfy, w->trackerfy+w->fyoffset))
					return 1;
				break;
			default: break;
		}
	} return 0;
}

static void setRowIntensity(bool mute, int i)
{
	if (mute || i < STATE_ROWS || (s->loop[1] && (i < s->loop[0] || i > s->loop[1]))) printf("\033[2m");
	else if (w->playing && w->playfy == i)                                            printf("\033[1m");
}

static void drawStarColumn(short x, short minx, short maxx)
{
	char buffer[4];
	for (int i = 0; i < s->songlen; i++)
		if (w->centre - w->trackerfy + i > TRACK_ROW && w->centre - w->trackerfy + i < ws.ws_row)
		{
			if (i < STATE_ROWS)                                              strcpy(buffer, " ");
			else if (w->playing && w->playfy == i)                           strcpy(buffer, "-");
			else if (s->rowhighlight && !((i-STATE_ROWS) % s->rowhighlight)) strcpy(buffer, "*");
			else                                                             strcpy(buffer, " ");
			printCulling(buffer, x, w->centre - w->trackerfy + i, minx, maxx);
		}
}

#define TRACK_HEADER_LEN 8
static void drawTrackHeader(uint8_t track, short x, short width, short minx, short maxx)
{
	Track *cv = s->track->v[track];

	if (cv->mute) printf("\033[2m");
	else          printf("\033[1m");

	char headerbuffer[MAX(TRACK_HEADER_LEN, width)];
	snprintf(headerbuffer, width+1, "%.*s", width, cv->name);
	printCulling(headerbuffer, x, TRACK_ROW - 1, minx, maxx);

	if (cv->triggerflash) printf("\033[3%dm", track%6 + 1);

	if (track == w->track + w->trackoffset) printf("\033[7m");

	snprintf(headerbuffer, TRACK_HEADER_LEN+1, "TRACK %02x", track);
	printCulling(headerbuffer, x + ((width - TRACK_HEADER_LEN)>>1), TRACK_ROW, minx, maxx);

	printf("\033[m");
}

static short drawTrack(uint8_t track, short bx, short minx, short maxx)
{
	if (track >= s->track->c) return 0;

	Row *r;
	char buffer[16];
	memset(buffer, 0, 16);

	Track *cv = s->track->v[track];

	short x, y;
	int lineno;

	if (bx <= maxx && bx + TRACK_TRIG_PAD + 12 + 4*(cv->variant->macroc+1) > minx)
	{
		for (int i = 0; i < s->songlen; i++)
		{
			y = w->centre - w->trackerfy + i;
			if (y > TRACK_ROW && y < ws.ws_row)
			{
				x = bx;
				lineno = getVariantChainVariant(NULL, cv->variant, i);

				if (ifVisual(track, i, -1)) printf("\033[2;7m");

				if (cv->variant->trig[i].index == VARIANT_OFF)
				{
					printf("\033[1m");
					strcpy(buffer, " ==  ");
				} else if (cv->variant->trig[i].index != VARIANT_VOID)
				{
					printf("\033[1;3%dm", cv->variant->trig[i].index%6 + 1);
					if (cv->variant->trig[i].flags&C_VTRIG_LOOP) snprintf(buffer, 6, "l%02x  ", cv->variant->trig[i].index);
					else                                         snprintf(buffer, 6, " %02x  ", cv->variant->trig[i].index);
				} else
				{
					printf("\033[2m");
					lineno = getVariantChainVariant(NULL, cv->variant, i);
					if (lineno == -1) strcpy(buffer, " ..  ");
					else              snprintf(buffer, 6, " %02x  ", lineno);
				}
				printCulling(buffer, x, y, minx, maxx);

				/* box drawing variant range thing, drawn separately cos multibyte chars break things */
				if (x+5 < maxx && x+4 > minx && lineno != -1)
					printf("\033[1D\033[22;3%dm│", cv->variant->trig[getVariantChainPrevVtrig(cv->variant, i)].index%6 + 1);

				stopVisual(track, i, -1);
				printf("\033[22;37;40m");

				x += 3 + TRACK_TRIG_PAD;

				r = getTrackRow(cv, i);
				setRowIntensity(cv->mute, i);

				if (ifVisual(track, i, 0)) printf("\033[2;7m");

				if (x <= maxx)
				{
					if (r->note != NOTE_VOID)
					{
						noteToString(r->note, buffer);
						if (cv->mute) printCulling(buffer, x, y, minx, maxx);
						else
						{
							if (ifVisual(track, i, 0))
							{
								printf("\033[22m");
								if (w->playing && w->playfy == i) printf("\033[1m");
							}
							if (instSafe(s->inst, r->inst)) printf("\033[3%dm", r->inst%6 + 1);
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
					setRowIntensity(cv->mute, i);
				}

				x+=3;

				if (x <= maxx)
				{
					snprintf(buffer, 4, " %02x", r->inst);
					startVisual(track, i, 1);
					if (r->inst != INST_VOID)
					{
						if (!cv->mute)
						{
							if (ifVisual(track, i, 1))
							{
								printf("\033[22m");
								setRowIntensity(cv->mute, i);
							}
							if (instSafe(s->inst, r->inst)) printf("\033[3%dm", r->inst%6 + 1);
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
					setRowIntensity(cv->mute, i);
				}

				x+=3;

				for (int j = cv->variant->macroc; j >= 0; j--)
				{
					if (x <= maxx)
					{
						startVisual(track, i, 2+j);
						if (r->macro[j].c)
						{
							if (ifVisual(track, i, 2+j))
							{
								printf("\033[22m");
								setRowIntensity(cv->mute, i);
							}

							snprintf(buffer, 5, " %c%02x", r->macro[j].c, r->macro[j].v);
							switch (r->macro[j].t>>4)
							{
								case 1: buffer[2] = '~'; break;
								case 2: buffer[2] = '+'; break;
								case 3: buffer[2] = '-'; break;
								case 4: buffer[2] = '%'; break;
							}
							switch (r->macro[j].t&15)
							{
								case 1: buffer[3] = '~'; break;
								case 2: buffer[3] = '+'; break;
								case 3: buffer[3] = '-'; break;
								case 4: buffer[3] = '%'; break;
							}

							if (cv->mute)
								printCulling(buffer, x, y, minx, maxx);
							else
							{
								printf("\033[3%dm", MACRO_COLOUR(r->macro[j].c) + 1);
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
						setRowIntensity(cv->mute, i);
					}
					x += 4;
				}

				printf("\033[m"); /* clear attributes before row highlighting */
			}
		}
	}
	return TRACK_TRIG_PAD + 11 + 4*(cv->variant->macroc+1);
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
	short x;

	if (w->mode == MODE_EFFECT)
	{
		if (cc.mouseadjust || cc.keyadjust) printf("\033[%d;0H\033[1m-- EFFECT ADJUST --\033[m", ws.ws_row);
		else                                printf("\033[%d;0H\033[1m-- EFFECT --\033[m"       , ws.ws_row);
		w->command.error[0] = '\0';

		clearControls();
		x = genConstSfx(EFFECT_WIDTH);
		for (uint8_t i = 0; i < s->track->c; i++)
		{
			drawTrackHeader(i, x+1, EFFECT_WIDTH,
					1, ws.ws_col);

			drawEffectChain(i, s->track->v[i]->effect,
					x + 2,
					EFFECT_WIDTH - 2,
					TRACK_ROW + 2);

			x += EFFECT_WIDTH;
		}
		drawControls();
	} else if (w->mode == MODE_SETTINGS)
	{
		if (cc.mouseadjust || cc.keyadjust) printf("\033[%d;0H\033[1m-- SETTINGS ADJUST --\033[m", ws.ws_row);
		else                                printf("\033[%d;0H\033[1m-- SETTINGS --\033[m"       , ws.ws_row);
		w->command.error[0] = '\0';

		clearControls();
		x = genConstSfx(SETTINGS_WIDTH);
		for (uint8_t i = 0; i < s->track->c; i++)
		{
			if (i) drawVerticalLine(x+1, TRACK_ROW+2, ws.ws_row - TRACK_ROW - 3, 1, ws.ws_col, 1, ws.ws_row);

			drawTrackHeader(i, x+2, SETTINGS_WIDTH,
					1, ws.ws_col);

			printCulling("transpose:",     x + 4, TRACK_ROW+2 + 0, 1, ws.ws_col); addControlInt(x + SETTINGS_WIDTH - 5, TRACK_ROW+2 + 0, &s->track->v[i]->transpose,          3, -128, 127, 0,   0, 0, NULL, NULL);
			printCulling("pattern scale:", x + 4, TRACK_ROW+2 + 1, 1, ws.ws_col); addControlInt(x + SETTINGS_WIDTH - 3, TRACK_ROW+2 + 1, &s->track->v[i]->patternlengthscale, 1, 0,    8,   1,   0, 0, NULL, NULL);

			x += SETTINGS_WIDTH;
		}
		drawControls();
	} else
	{
		short sfx = genSfx(TRACK_LINENO_COLS);
		x = 1 + TRACK_LINENO_COLS + 2 + sfx;
		for (uint8_t i = 0; i < s->track->c; i++)
		{
			drawTrackHeader(i, x + TRACK_TRIG_PAD+3, 6 + 4*(s->track->v[i]->variant->macroc+1),
					LINENO_COLS, ws.ws_col);

			x += TRACK_TRIG_PAD + 11 + 4*(s->track->v[i]->variant->macroc+1);
		}

		short sx = drawTrackerBody(sfx, 1, LINENO_COLS-1, ws.ws_col); /* TODO: the -1 here is a dirty hack */

		switch (w->mode)
		{
			case MODE_VISUAL:        printf("\033[%d;0H\033[1m-- VISUAL --\033[m",                 ws.ws_row); w->command.error[0] = '\0'; break;
			case MODE_VISUALLINE:    printf("\033[%d;0H\033[1m-- VISUAL LINE --\033[m\033[4 q",    ws.ws_row); w->command.error[0] = '\0'; break;
			case MODE_VISUALREPLACE: printf("\033[%d;0H\033[1m-- VISUAL REPLACE --\033[m\033[4 q", ws.ws_row); w->command.error[0] = '\0'; break;
			case MODE_MOUSEADJUST:   printf("\033[%d;0H\033[1m-- ADJUST --\033[m\033[4 q",         ws.ws_row); w->command.error[0] = '\0'; break;
			case MODE_INSERT:
				printf("\033[%d;0H\033[1m-- INSERT --\033[m", ws.ws_row);
				printf("\033[6 q");
				w->command.error[0] = '\0'; break;
			default: break;
		}

		/* position the tracker cursor */
		short macro;
		short y = w->centre + w->fyoffset;
		switch (w->trackerfx)
		{
			case -1: printf("\033[%d;%dH", y, sx+sfx + 2 - w->fieldpointer); break;
			case  0: printf("\033[%d;%dH", y, sx+sfx + 3 + TRACK_TRIG_PAD); break;
			case  1: printf("\033[%d;%dH", y, sx+sfx + 8 + TRACK_TRIG_PAD - w->fieldpointer); break;
			default: /* macro columns */
				macro = (w->trackerfx - 2)>>1;
				if (w->trackerfx % 2 == 0) printf("\033[%d;%dH", y, sx+sfx + 10 + TRACK_TRIG_PAD + (s->track->v[w->track+w->trackoffset]->variant->macroc - macro)*4);
				else                       printf("\033[%d;%dH", y, sx+sfx + 12 + TRACK_TRIG_PAD + (s->track->v[w->track+w->trackoffset]->variant->macroc - macro)*4 - w->fieldpointer);
				break;
		}
	}
}
