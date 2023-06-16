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

short getTrackWidth(Track *cv) { return 9 + 4*(cv->pattern->macroc+1); }

/* generate sfx using dynamic width tracks */
short genSfx(short viewport)
{
	short x = 0;
	short ret = 0;
	short trackw;

	for (int i = 0; i < s->track->c; i++)
	{
		trackw = getTrackWidth(s->track->v[i]);
		x += trackw;
		if (i == w->track) ret = x - (trackw>>1); /* should only be set once */
		/* keep iterating so x is the full width of all tracks */
	}
	if (x > viewport)
	{
		ret = (viewport>>1) - ret;
		ret = MIN(ret, 0);
		ret = MAX(ret, -(x - viewport));
	} else
		ret = ((viewport - x)>>1);
	return ret;
}
/* generate sfx using constant width tracks */
short genConstSfx(short trackw, short viewportwidth)
{
	short x = 0;
	short ret = 0;

	for (int i = 0; i < s->track->c; i++)
	{
		x += trackw;
		if (i == w->track) ret = x - (trackw>>1); /* should only be set once */
		/* keep iterating so x is the full width of all tracks */
	}
	if (x > viewportwidth)
	{
		ret = (viewportwidth>>1) - ret;
		ret = MIN(ret, 0);
		ret = MAX(ret, -(x - viewportwidth));
	} else
		ret = ((viewportwidth - x)>>1);
	return ret;
}

static void setRowIntensity(bool mute, bool pattern, int i)
{
	if (mute)
		printf("\033[2m");
	else if (pattern && getPatternChainIndex(w->trackerfy) != getPatternChainIndex(i))
		printf("\033[2m");
	else if (w->playing && w->playfy == i)
		printf("\033[1m");
}

static void drawLineNumbers(short x, short minx, short maxx)
{
	char buffer[5];
	int i;
	for (short y = TRACK_ROW+1; y < ws.ws_row; y++)
	{
		i = y - (w->centre - w->trackerfy);
		if (i < 0) continue;
		if (getPatternChainIndex(w->trackerfy) == getPatternChainIndex(i))
		{
			snprintf(buffer, 5, "%02x", getPatternIndex(i));
			setRowIntensity(0, 1, i);
			printCulling(buffer, x, y, minx, maxx);
			printf("\033[22m");
		}
	}
}

#define VISUAL_ON "\033[2;7m"
#define VISUAL_OFF "\033[22;27m"
static bool startVisual(uint8_t track, int i, int8_t fieldpointer)
{
	if (w->page != PAGE_VARIANT) return 0;
	switch (w->mode)
	{
		case MODE_VISUAL:
			if (track == MIN(w->visualtrack, w->track+w->trackoffset)
					&& i >= MIN(w->visualfy, w->trackerfy+w->fyoffset)
					&& i <= MAX(w->visualfy, w->trackerfy+w->fyoffset)
					&& ((w->visualtrack == w->track+w->trackoffset && fieldpointer == vfxVmoMin(w->visualfx, tfxToVfx(w->trackerfx)))
					||  (w->visualtrack != w->track+w->trackoffset && fieldpointer == ((w->visualtrack <= w->track+w->trackoffset) ? w->visualfx : tfxToVfx(w->trackerfx)))))
			{
					printf(VISUAL_ON);
					return 1;
			} break;
		case MODE_VISUALREPLACE:
			if (track == w->track+w->trackoffset && fieldpointer == tfxToVfx(w->trackerfx)
					&& i >= MIN(w->visualfy, w->trackerfy+w->fyoffset)
					&& i <= MAX(w->visualfy, w->trackerfy+w->fyoffset))
			{
				printf(VISUAL_ON);
				return 1;
			} break;
		case MODE_VISUALLINE:
			if (track == MIN(w->visualtrack, w->track+w->trackoffset) && fieldpointer == 0
					&& i >= MIN(w->visualfy, w->trackerfy+w->fyoffset)
					&& i <= MAX(w->visualfy, w->trackerfy+w->fyoffset))
			{
				printf(VISUAL_ON);
				return 1;
			} break;
		default: break;
	} return 0;
}
static bool stopVisual(uint8_t track, int i, int8_t fieldpointer)
{
	if (w->page != PAGE_VARIANT) return 0;
	switch (w->mode)
	{
		case MODE_VISUAL:
			if (track == MAX(w->visualtrack, w->track+w->trackoffset)
					&& i >= MIN(w->visualfy, w->trackerfy+w->fyoffset)
					&& i <= MAX(w->visualfy, w->trackerfy+w->fyoffset)
					&& ((w->visualtrack == w->track+w->trackoffset && fieldpointer == vfxVmoMax(w->visualfx, tfxToVfx(w->trackerfx)))
					||  (w->visualtrack != w->track+w->trackoffset && fieldpointer == ((w->visualtrack >= w->track+w->trackoffset) ? w->visualfx : tfxToVfx(w->trackerfx)))))
			{
				printf(VISUAL_OFF);
				return 1;
			} break;
		case MODE_VISUALREPLACE:
			if (track == w->track+w->trackoffset && fieldpointer == tfxToVfx(w->trackerfx)
					&& i >= MIN(w->visualfy, w->trackerfy+w->fyoffset)
					&& i <= MAX(w->visualfy, w->trackerfy+w->fyoffset))
			{
				printf(VISUAL_OFF);
				return 1;
			} break;
		default: break;
	} return 0;
}
static bool ifVisualOrder(uint8_t track, int i)
{
	if (w->page != PAGE_PATTERN) return 0;

	switch (w->mode)
	{
		case MODE_VISUAL:
		case MODE_VISUALREPLACE:
		case MODE_VISUALLINE:
			if ((
				   track >= MIN(w->visualtrack, w->track+w->trackoffset)
				&& track <= MAX(w->visualtrack, w->track+w->trackoffset)
				) && (
				   i >= MIN(w->visualfy/(s->plen+1), w->trackerfy/(s->plen+1)+w->fyoffset)
				&& i <= MAX(w->visualfy/(s->plen+1), w->trackerfy/(s->plen+1)+w->fyoffset)
				))
					return 1;
			break;
		default: break;
	}
	return 0;
}
static bool ifVisual(uint8_t track, int i, int8_t fieldpointer)
{
	if (w->page != PAGE_VARIANT) return 0;
	if (track >= MIN(w->visualtrack, w->track+w->trackoffset)
	 && track <= MAX(w->visualtrack, w->track+w->trackoffset))
	{
		switch (w->mode)
		{
			case MODE_VISUAL:
				if (i >= MIN(w->visualfy, w->trackerfy+w->fyoffset)
				 && i <= MAX(w->visualfy, w->trackerfy+w->fyoffset))
				{
					if (w->visualtrack == w->track+w->trackoffset)
					{
						if (vfxVmoRangeIncl(
								vfxVmoMin(w->visualfx, tfxToVfx(w->trackerfx)),
								vfxVmoMax(w->visualfx, tfxToVfx(w->trackerfx)),
								fieldpointer))
							return 1;
					} else
					{
						if (track > MIN(w->visualtrack, w->track+w->trackoffset)
						 && track < MAX(w->visualtrack, w->track+w->trackoffset)) /* middle track */
							return 1;
						else if (track == MIN(w->visualtrack, w->track+w->trackoffset)) /* first track */
						{
							if (vfxVmoRangeIncl(
									w->visualtrack == track ? w->visualfx : tfxToVfx(w->trackerfx),
									1 + s->track->v[track]->pattern->macroc,
									fieldpointer))
								return 1;
						} else if (track == MAX(w->visualtrack, w->track+w->trackoffset)) /* last track */
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
				if (track == w->track+w->trackoffset && fieldpointer == tfxToVfx(w->trackerfx)
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

static void drawStarColumn(short x, short minx, short maxx)
{
	char buffer[4];
	int i;
	for (short y = TRACK_ROW+1; y < ws.ws_row; y++)
	{
		i = y - (w->centre - w->trackerfy);
		if (i < 0) continue;
		if (i >= (s->plen+1)*PATTERN_VOID) break;

		if (w->playing && w->playfy == i)                   strcpy(buffer, "-");
		else if (s->rowhighlight && !(i % s->rowhighlight)) strcpy(buffer, "*");
		else                                                strcpy(buffer, " ");
		setRowIntensity(0, 1, i);
		printCulling(buffer, x, w->centre - w->trackerfy + i, minx, maxx);
		printf("\033[22m");
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

static void drawTrackSmallHeader(uint8_t track, short x, short minx, short maxx)
{
	Track *cv = s->track->v[track];

	if (cv->mute) printf("\033[2m");
	else          printf("\033[1m");

	char headerbuffer[3];

	if (cv->triggerflash) printf("\033[3%dm", track%6 + 1);
	if (track == w->track + w->trackoffset) printf("\033[7m");

	snprintf(headerbuffer, 3, "%02x", track);
	printCulling(headerbuffer, x, TRACK_ROW, minx, maxx);

	printf("\033[m");
}

static short drawTrack(uint8_t track, short bx, short minx, short maxx)
{
	if (track >= s->track->c) return 0;

	Row *r;
	char buffer[16];
	memset(buffer, 0, 16);

	Track *cv = s->track->v[track];

	short x;
	int i;

	if (bx <= maxx && bx + getTrackWidth(cv) >= minx)
	{
		for (short y = TRACK_ROW+1; y < ws.ws_row; y++)
		{
			i = y - (w->centre - w->trackerfy);
			if (i < 0) continue;
			if (i >= (s->plen+1)*PATTERN_VOID) break;
			x = bx;
			// lineno = getVariantChainVariant(NULL, cv->variant, i);

			//
			// if (cv->variant->trig[i].index == VARIANT_OFF)
			// {
			// 	printf("\033[1m");
			// 	strcpy(buffer, " ==  ");
			// } else if (cv->variant->trig[i].index != VARIANT_VOID)
			// {
			// 	printf("\033[1;3%dm", cv->variant->trig[i].index%6 + 1);
			// 	if (cv->variant->trig[i].flags&C_VTRIG_LOOP) snprintf(buffer, 6, "l%02x  ", cv->variant->trig[i].index);
			// 	else                                         snprintf(buffer, 6, " %02x  ", cv->variant->trig[i].index);
			// } else
			// {
			// 	printf("\033[2m");
			// 	if (lineno == -1) strcpy(buffer, " ..  ");
			// 	else              snprintf(buffer, 6, " %02x  ", lineno);
			// }
			// printCulling(buffer, x, y, minx, maxx);
			//
			// /* box drawing variant range thing, drawn separately cos multibyte chars break things */
			// if (x+5 < maxx && x+4 > minx && lineno != -1)
			// 	printf("\033[1D\033[22;3%dm│", cv->variant->trig[getVariantChainPrevVtrig(cv->variant, i)].index%6 + 1);
			//
			// stopVisual(track, i, -1);
			// printf("\033[22;37;40m");

			x++;

			r = getTrackRow(cv, i, 0);
			setRowIntensity(cv->mute, 1, i);
			if (ifVisual(track, i, 0)) printf("\033[2;7m");

			if (x <= maxx)
			{
				if (r && r->note != NOTE_VOID)
				{
					noteToString(r->note, buffer);
					if (cv->mute)
						printCulling(buffer, x, y, minx, maxx);
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
				setRowIntensity(cv->mute, 1, i);
			}

			x+=3;

			if (x <= maxx)
			{
				startVisual(track, i, 1);
				if (r && r->inst != INST_VOID)
				{
					snprintf(buffer, 4, " %02x", r->inst);
					if (!cv->mute)
					{
						if (ifVisual(track, i, 1))
						{
							printf("\033[22m");
							setRowIntensity(cv->mute, 1, i);
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
				setRowIntensity(cv->mute, 1, i);
			}

			x+=3;

			for (int j = cv->pattern->macroc; j >= 0; j--)
			{
				if (x <= maxx)
				{
					startVisual(track, i, 2+j);
					if (r && r->macro[j].c)
					{
						if (ifVisual(track, i, 2+j))
						{
							printf("\033[22m");
							setRowIntensity(cv->mute, 1, i);
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
					setRowIntensity(cv->mute, 1, i);
				}
				x += 4;
			}
			printf("\033[m"); /* clear attributes before row highlighting */
		}
	}
	return getTrackWidth(cv);
}

/* returns the xpos at the start of the cursor track */
static short drawTrackerBody(short sfx, short x, short minx, short maxx)
{
	short ret = 0;
	short y;
	int i;

	drawLineNumbers(MAX(x, MAX(sfx+1, 0) + minx), MAX(sfx+1, 0) + minx, minx+TRACK_LINENO_COLS+MAX(sfx+1, 0) + minx);
	x += TRACK_LINENO_COLS;
	drawStarColumn(x+sfx + 1, minx+TRACK_LINENO_COLS+2, maxx);
	x += 2;

	for (int i = 0; i < s->track->c; i++)
	{
		if (i == w->track + w->trackoffset)
			ret = x;

		x += drawTrack(i, x+sfx, minx+TRACK_LINENO_COLS+3, maxx);
		drawStarColumn(x+sfx - 1, minx+TRACK_LINENO_COLS+2, maxx);
	}
	if (sfx < 0)
	{
		for (y = TRACK_ROW+1; y < ws.ws_row; y++)
		{
			i = y - (w->centre - w->trackerfy);
			if (i < 0) continue;
			if (i >= (s->plen+1)*PATTERN_VOID) break;

			setRowIntensity(0, 1, i);
			printf("\033[%d;%dH^\033[22m", y, minx+TRACK_LINENO_COLS+2);
		}
	}
	if (x + sfx > maxx + 1)
	{
		for (y = TRACK_ROW+1; y < ws.ws_row; y++)
		{
			i = y - (w->centre - w->trackerfy);
			if (i < 0) continue;
			if (i >= (s->plen+1)*PATTERN_VOID) break;

			setRowIntensity(0, 1, i);
			printf("\033[%d;%dH$\033[22m", y, maxx);
		}
	}

	return ret;
}

#define PATTERN_GUTTER 4
void drawTracker(bool patternlist)
{
	if (w->page == PAGE_EFFECT)
	{
		if (cc.mouseadjust || cc.keyadjust) printf("\033[%d;0H\033[1m-- EFFECT ADJUST --\033[m", ws.ws_row);
		else                                printf("\033[%d;0H\033[1m-- EFFECT --\033[m"       , ws.ws_row);
		w->command.error[0] = '\0';

		clearControls();
		short x = genConstSfx(EFFECT_WIDTH, ws.ws_col);
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
	} else
	{
		short offset = MIN(s->track->c * 3, ws.ws_col>>2)+PATTERN_GUTTER;
		short sfx = genSfx(ws.ws_col - ((TRACK_LINENO_COLS<<1) + offset));
		short x = TRACK_LINENO_COLS + 3 + sfx + offset;
		short xpartition = MAX(offset, MAX(sfx, 0) + offset);
		short smallsfx = genConstSfx(3, offset-PATTERN_GUTTER);
		short smallx, y, smallsx = 0;
		int j;
		char smallbuffer[4];
		for (uint8_t i = 0; i < s->track->c; i++)
		{
			smallx = xpartition - TRACK_LINENO_COLS - (offset-PATTERN_GUTTER) + (i+1)*3 + smallsfx - 1;

			if (i == w->track + w->trackoffset)
				smallsx = smallx;

			drawTrackSmallHeader(i, smallx, PATTERN_GUTTER, xpartition - TRACK_LINENO_COLS);
			for (j = 0; j < PATTERN_VOID; j++)
			{
				y = w->centre - w->trackerfy/(s->plen+1) + j;
				if (y >= ws.ws_row) break;
				if (y > TRACK_ROW)
				{
					if (s->track->v[i]->pattern->order[j] == PATTERN_VOID)
						strcpy(smallbuffer, "-- ");
					else
						snprintf(smallbuffer, 4, "%02x ", s->track->v[i]->pattern->order[j]);

					if (s->track->v[i]->mute || j > s->slen)
						printf("\033[2m");
					else if (w->playing && getPatternChainIndex(w->playfy) == j)
						printf("\033[1m");

					if (ifVisualOrder(i, j))
						printf(VISUAL_ON);
					printCulling(smallbuffer, smallx, y, PATTERN_GUTTER, xpartition - TRACK_LINENO_COLS);
					printf(VISUAL_OFF);
				}
			}

			drawTrackHeader(i, x + 1, getTrackWidth(s->track->v[i]) - 3, xpartition + 4, ws.ws_col);
			x += getTrackWidth(s->track->v[i]);
		}

		if (smallsfx < 0)
		{
			for (y = TRACK_ROW+1; y < ws.ws_row; y++)
			{
				j = y - (w->centre - w->trackerfy/(s->plen+1));
				if (j < 0) continue;
				if (j >= PATTERN_VOID) break;

				setRowIntensity(0, 0, j);
				printf("\033[%d;%dH^\033[22m", y, PATTERN_GUTTER);
			}
		}
		if (smallsfx + (s->track->c+1)*3 + 1 > offset)
		{
			for (y = TRACK_ROW+1; y < ws.ws_row; y++)
			{
				j = y - (w->centre - w->trackerfy/(s->plen+1));
				if (j < 0) continue;
				if (j >= PATTERN_VOID) break;

				setRowIntensity(0, 0, j);
				printf("\033[%d;%dH$\033[22m", y, xpartition - TRACK_LINENO_COLS);
			}
		}

		/* pointers */
		if (w->playing)
		{
			y = w->centre - (w->trackerfy/(s->plen+1)) + (w->playfy/(s->plen+1));
			if (y > TRACK_ROW && y < ws.ws_row)
				printf("\033[%d;%dH>", y, xpartition - TRACK_LINENO_COLS - offset);
		}
		if (w->queue != -1)
		{
			y = w->centre - (w->trackerfy/(s->plen+1)) + w->queue;
			if (y > TRACK_ROW && y < ws.ws_row)
				printf("\033[%d;%dHQ", y, xpartition - TRACK_LINENO_COLS - offset + 1);
		}

		short sx = drawTrackerBody(sfx, offset + 1, offset, ws.ws_col);

		switch (w->mode)
		{
			case MODE_VISUAL:        printf("\033[%d;0H\033[1m-- VISUAL --\033[m",                 ws.ws_row); w->command.error[0] = '\0'; break;
			case MODE_VISUALLINE:    printf("\033[%d;0H\033[1m-- VISUAL LINE --\033[m\033[4 q",    ws.ws_row); w->command.error[0] = '\0'; break;
			case MODE_VISUALREPLACE: printf("\033[%d;0H\033[1m-- VISUAL REPLACE --\033[m\033[4 q", ws.ws_row); w->command.error[0] = '\0'; break;
			case MODE_MOUSEADJUST:   printf("\033[%d;0H\033[1m-- ADJUST --\033[m\033[4 q",         ws.ws_row); w->command.error[0] = '\0'; break;
			case MODE_INSERT:        printf("\033[%d;0H\033[1m-- INSERT --\033[m\033[6 q",         ws.ws_row); w->command.error[0] = '\0'; break;
			default: break;
		}

		/* position the tracker cursor */
		short macro;
		y = w->centre + w->fyoffset;
		if (patternlist)
			printf("\033[%d;%dH", y, smallsx + 1 - w->fieldpointer);
		else
			switch (w->trackerfx)
			{
				case  0: printf("\033[%d;%dH", y, sx+sfx + 1); break;
				case  1: printf("\033[%d;%dH", y, sx+sfx + 6 - w->fieldpointer); break;
				default: /* macro columns */
					macro = (w->trackerfx - 2)>>1;
					if (w->trackerfx % 2 == 0) printf("\033[%d;%dH", y, sx+sfx + 8  + (s->track->v[w->track+w->trackoffset]->pattern->macroc - macro)*4);
					else                       printf("\033[%d;%dH", y, sx+sfx + 10 + (s->track->v[w->track+w->trackoffset]->pattern->macroc - macro)*4 - w->fieldpointer);
					break;
			}
	}
}
