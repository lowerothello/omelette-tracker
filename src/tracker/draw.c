#define CHANNEL_TRIG_COLS 4
#define CHANNEL_LINENO_COLS (LINENO_COLS - 3)

/* will populate buffer, buffer should be at least length 4 */
void noteToString(uint8_t note, char *buffer)
{
	if (note == NOTE_VOID) { snprintf(buffer, 4, "..."); return; }
	if (note == NOTE_OFF)  { snprintf(buffer, 4, "==="); return; }

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
	snprintf(buffer, 4, "%s%c", strnote, octave);
}

/* void drawSongList(void)
{
	for (int i = 0; i < SONG_MAX; i++) // reserve 0xff
		if (w->centre - w->trackerfy + i > CHANNEL_ROW
				&& w->centre - w->trackerfy + i < ws.ws_row)
		{
			printf("\033[%d;%dH", w->centre - w->trackerfy + i, 1);
			if (s->trig[i].flags) printf("l");
			else                  printf(" ");

			switch (w->mode)
			{
				case T_MODE_VTRIG: case T_MODE_VTRIG_INSERT:
					if (s->playing && s->songp == i)      printf("\033[1m%02x\033[m", s->trig[i].index);
					else if (s->trig[i].index == PATTERN_VOID) printf("..");
					else                                  printf("%02x", s->trig[i].index);
					break;
				case T_MODE_VTRIG_VISUAL:
					if (
							i >= MIN(w->visualfy, w->trackerfy) &&
							i <= MAX(w->visualfy, w->trackerfy))
					{
						if (s->playing && s->songp == i)      printf("\033[2;7m%02x\033[m", s->trig[i].index);
						else if (s->trig[i].index == PATTERN_VOID) printf("\033[2;7m..\033[m");
						else                                  printf("\033[2;7m%02x\033[m", s->trig[i].index);
					} else
					{
						if (s->playing && s->songp == i)      printf("\033[1m%02x\033[m", s->trig[i].index);
						else if (s->trig[i].index == PATTERN_VOID) printf("..");
						else                                  printf("%02x", s->trig[i].index);
					} break;
				default:
					if (w->trackerfy == i && s->trig[i].index == PATTERN_VOID) printf("\033[7m..\033[m");
					else if (s->playing && s->songp == i && w->trackerfy == i) printf("\033[1;7m%02x\033[m", s->trig[i].index);
					else if (w->trackerfy == i)                                printf("\033[7m%02x\033[m",   s->trig[i].index);
					else if (s->playing && s->songp == i)                      printf("\033[1m%02x\033[m",   s->trig[i].index);
					else if (s->trig[i].index == PATTERN_VOID)                 printf("..");
					else                                                       printf("%02x", s->trig[i].index);
					break;
			}
		}
} */

void drawGlobalLineNumbers(unsigned short x)
{
	for (int i = 0; i < s->songlen; i++)
		if (w->centre - w->trackerfy + i > CHANNEL_ROW && w->centre - w->trackerfy + i < ws.ws_row)
		{
			printf("\033[%d;%dH", w->centre - w->trackerfy + i, x);
			if (s->playing == PLAYING_CONT && s->playfy == i) printf("\033[1m");
			else if (i < s->loop[0] || i > s->loop[1])        printf("\033[2m");
			if (i < STATE_ROWS) printf("  -%x", STATE_ROWS - i);
			else                printf("%04x",  i - STATE_ROWS);
			printf("\033[m");
		}
}

void startVisual(uint8_t channel, int i, signed char fieldpointer)
{
	switch (w->mode)
	{
		case T_MODE_VISUAL:
			if (channel == MIN(w->visualchannel, w->channel))
			{
				if (w->visualchannel == w->channel)
				{
					if (i >= MIN(w->visualfy, w->trackerfy+w->fyoffset) && i <= MAX(w->visualfy, w->trackerfy+w->fyoffset)
							&& fieldpointer == MIN(w->visualfx, tfxToVfx(w->trackerfx)))
						printf("\033[2;7m");
				} else if (i >= MIN(w->visualfy, w->trackerfy+w->fyoffset) && i <= MAX(w->visualfy, w->trackerfy+w->fyoffset)
						&& fieldpointer == (w->visualchannel <= w->channel ? w->visualfx : tfxToVfx(w->trackerfx)))
					printf("\033[2;7m");
			} break;
		case T_MODE_VISUALLINE:
			if (channel == MIN(w->visualchannel, w->channel)
					&& i >= MIN(w->visualfy, w->trackerfy+w->fyoffset) && i <= MAX(w->visualfy, w->trackerfy+w->fyoffset)
					&& fieldpointer == 0)
				printf("\033[2;7m");
			break;
		case T_MODE_VISUALREPLACE:
			if (channel == w->channel
					&& i >= MIN(w->visualfy, w->trackerfy+w->fyoffset) && i <= MAX(w->visualfy, w->trackerfy+w->fyoffset)
					&& fieldpointer == tfxToVfx(w->trackerfx))
				printf("\033[2;7m");
			break;
	}
}
void stopVisual(uint8_t channel, int i, signed char fieldpointer)
{
	switch (w->mode)
	{
		case T_MODE_VISUALREPLACE:
			if (channel == w->channel)
			{
				if (w->visualchannel == w->channel)
				{
					if (i >= MIN(w->visualfy, w->trackerfy+w->fyoffset) && i <= MAX(w->visualfy, w->trackerfy+w->fyoffset)
							&& fieldpointer == tfxToVfx(w->trackerfx))
						printf("\033[22;27m");
				} else if (i >= MIN(w->visualfy, w->trackerfy+w->fyoffset) && i <= MAX(w->visualfy, w->trackerfy+w->fyoffset)
						&& fieldpointer == tfxToVfx(w->trackerfx))
					printf("\033[22;27m");
			} break;
		case T_MODE_VISUAL:
			if (channel == MAX(w->visualchannel, w->channel))
			{
				if (w->visualchannel == w->channel)
				{
					if (i >= MIN(w->visualfy, w->trackerfy+w->fyoffset) && i <= MAX(w->visualfy, w->trackerfy+w->fyoffset)
							&& fieldpointer == MAX(w->visualfx, tfxToVfx(w->trackerfx)))
						printf("\033[22;27m");
				} else if (i >= MIN(w->visualfy, w->trackerfy+w->fyoffset) && i <= MAX(w->visualfy, w->trackerfy+w->fyoffset)
						&& fieldpointer == (w->visualchannel >= w->channel ? w->visualfx : tfxToVfx(w->trackerfx)))
					printf("\033[22;27m");
			} break;
	}
}
int ifVisual(uint8_t channel, int i, signed char fieldpointer)
{
	if (   channel >= MIN(w->visualchannel, w->channel)
	    && channel <= MAX(w->visualchannel, w->channel))
	{
		switch (w->mode)
		{
			case T_MODE_VISUAL: case T_MODE_VISUALREPLACE:
				if (w->visualchannel == w->channel)
				{
					if (i >= MIN(w->visualfy, w->trackerfy+w->fyoffset) && i <= MAX(w->visualfy, w->trackerfy+w->fyoffset)
							&& fieldpointer >= MIN(w->visualfx, tfxToVfx(w->trackerfx))
							&& fieldpointer <= MAX(w->visualfx, tfxToVfx(w->trackerfx)))
						return 1;
				} else
				{
					if (channel > MIN(w->visualchannel, w->channel) && channel < MAX(w->visualchannel, w->channel)) return 1;
					else if (channel == MIN(w->visualchannel, w->channel))
					{
						if (i >= MIN(w->visualfy, w->trackerfy+w->fyoffset) && i <= MAX(w->visualfy, w->trackerfy+w->fyoffset)
								&& fieldpointer >= (w->visualchannel <= w->channel ? w->visualfx : tfxToVfx(w->trackerfx)))
							return 1;
					} else if (channel == MAX(w->visualchannel, w->channel))
					{
						if (i >= MIN(w->visualfy, w->trackerfy+w->fyoffset) && i <= MAX(w->visualfy, w->trackerfy+w->fyoffset)
								&& fieldpointer <= (w->visualchannel >= w->channel ? w->visualfx : tfxToVfx(w->trackerfx)))
							return 1;
					}
				} break;
			case T_MODE_VTRIG_VISUAL: case T_MODE_VISUALLINE:
				if (i >= MIN(w->visualfy, w->trackerfy+w->fyoffset) && i <= MAX(w->visualfy, w->trackerfy+w->fyoffset))
					return 1;
		}
	} return 0;
}

void drawChannelTrigs(uint8_t channel, short x)
{
	ChannelData *cd = &s->channelv[channel].data;
	char buffer[5];

	for (int i = 0; i < s->songlen; i++)
		if (w->centre - w->trackerfy + i > CHANNEL_ROW && w->centre - w->trackerfy + i < ws.ws_row)
		{
			printf("\033[%d;%dH", w->centre - w->trackerfy + i, MAX(LINENO_COLS, x));
			if (w->mode == T_MODE_VTRIG_VISUAL && ifVisual(channel, i, 0)) printf("\033[7m");

			if (cd->trig[i].index == VARIANT_OFF)
			{
				printf("\033[1m"); strcpy(buffer, " == ");
			} else if (cd->trig[i].index != VARIANT_VOID)
			{
				printf("\033[1m");
				if (cd->trig[i].flags&C_VTRIG_LOOP) snprintf(buffer, 5, "l%02x ", cd->trig[i].index);
				else                                snprintf(buffer, 5, " %02x ", cd->trig[i].index);
			} else
			{
				printf("\033[2m");
				int lineno = getChannelVariant(NULL, cd, i);
				if (lineno == -1) strcpy(buffer, " .. ");
				else              snprintf(buffer, 5, " %02x ", lineno);
			}

			if (x < LINENO_COLS)
			{
				if (x > LINENO_COLS-4)
					printf("%s", &buffer[LINENO_COLS - x]);
			} else printf("%.*s", MAX(0, ws.ws_col - x), buffer);
			printf("\033[m");
		}
}

void drawStarColumn(unsigned short x)
{
	for (int i = 0; i < s->songlen; i++)
		if (w->centre - w->trackerfy + i > CHANNEL_ROW && w->centre - w->trackerfy + i < ws.ws_row)
		{
			printf("\033[%d;%dH", w->centre - w->trackerfy + i, x);
			if (i < STATE_ROWS)                                              printf("   ");
			else if (s->playing == PLAYING_CONT && s->playfy == i)           printf(" - ");
			else if (s->rowhighlight && !((i-STATE_ROWS) % s->rowhighlight)) printf(" * ");
			else                                                             printf("   ");
		}
}

short _drawChannelHeader(uint8_t channel, short x)
{
	if (channel >= s->channelc) return 0;
	char buffer[16];

	ChannelData *cd = &s->channelv[channel].data;

	snprintf(buffer, 11, "CHANNEL %02x", channel);

	int c = x + (6 + 4*(cd->macroc+1) - 10)/2;
	if (c <= ws.ws_col)
	{
		/* TODO: channel line underline is more complicated than this, not sure how to approach it */
		/* if (w->centre - w->trackerfy == CHANNEL_ROW)
		{
			gcvret = getChannelVariant(&v, cd, i);
			if ((i < s->songlen-1 && (cd->trig[i+1].index < VARIANT_MAX || (cd->trig[i+1].index == VARIANT_OFF && gcvret != -1)))
					|| (gcvret != -1 && (gcvret%(v->rowc+1) == v->rowc)))
				printf("\033[4m");
		} */
		if (cd->mute) printf("\033[2m");
		else          printf("\033[1m");
		if (channel == w->channel + w->channeloffset) printf("\033[7m");
		if (x < LINENO_COLS - 2) { if (x > LINENO_COLS - 12) printf("\033[%d;%dH%s", CHANNEL_ROW, LINENO_COLS, buffer+(LINENO_COLS - x - 2)); }
		else                                                 printf("\033[%d;%dH%.*s", CHANNEL_ROW, c, ws.ws_col - c, buffer);
		printf("\033[m");
	} return 8 + 4*(cd->macroc+1);
}
short drawChannel(uint8_t channel, short x)
{
	if (channel >= s->channelc) return 0;

	Row *r;
	char buffer[16];

	Variant *v;
	int gcvret;
	ChannelData *cd = &s->channelv[channel].data;

	_drawChannelHeader(channel, x);


	if (x <= ws.ws_col && x + 3 + 7 + 4*(cd->macroc+1) -1 > LINENO_COLS)
	{
		for (int i = 0; i < s->songlen; i++)
			if (w->centre - w->trackerfy + i > CHANNEL_ROW && w->centre - w->trackerfy + i < ws.ws_row)
			{
				if (x < LINENO_COLS) printf("\033[%d;%dH", w->centre - w->trackerfy + i, LINENO_COLS);
				else                 printf("\033[%d;%dH", w->centre - w->trackerfy + i, x);

				r = getChannelRow(cd, i);

				if (cd->mute || i < STATE_ROWS)                        printf("\033[2m");
				else if (s->playing == PLAYING_CONT && s->playfy == i) printf("\033[1m");

				gcvret = getChannelVariant(&v, cd, i);
				if ((i < s->songlen-1 && (cd->trig[i+1].index < VARIANT_MAX || (cd->trig[i+1].index == VARIANT_OFF && gcvret != -1)))
						|| (gcvret != -1 && (gcvret%(v->rowc+1) == v->rowc)))
					printf("\033[4m");

				/* start visual for channels that are part of the selection but not the first channel of it */
				if ((w->mode == T_MODE_VISUAL || w->mode == T_MODE_VISUALLINE)
						&& channel > MIN(w->visualchannel, w->channel) && channel <= MAX(w->visualchannel, w->channel)
						&& i >= MIN(w->visualfy, w->trackerfy+w->fyoffset) && i <= MAX(w->visualfy, w->trackerfy+w->fyoffset))
					printf("\033[2;7m");

				startVisual(channel, i, 0);
				if (r->note != NOTE_VOID)
				{
					noteToString(r->note, buffer);
					if (cd->mute)
					{
						if (x < LINENO_COLS) { if (x > LINENO_COLS - 3) printf("%s", buffer+(LINENO_COLS - x)); }
						else                                            printf("%.*s", ws.ws_col - x, buffer);
					} else
					{
						if (ifVisual(channel, i, 0))
						{
							printf("\033[22m");
							if (s->playing == PLAYING_CONT && s->playfy == i) printf("\033[1m");
						}
						if (r->note == NOTE_OFF) printf("\033[31m");
						else                     printf("\033[32m");
						if (x < LINENO_COLS) { if (x > LINENO_COLS - 3) printf("%s", buffer+(LINENO_COLS - x)); }
						else                                            printf("%.*s", ws.ws_col - x, buffer);
						printf("\033[37m");
						if (ifVisual(channel, i, 0)) printf("\033[2m");
					}
				} else
				{
					if (x < LINENO_COLS) { if (x > LINENO_COLS - 3) printf("%s", "..."+(LINENO_COLS - x)); }
					else                                            printf("%.*s", ws.ws_col - x, "...");
				}
				stopVisual(channel, i, 0);
				if (!cd->mute && s->playing == PLAYING_CONT && s->playfy == i) printf("\033[1m");

				if (x+4 <= ws.ws_col)
				{
					if (x+4 -1 >= LINENO_COLS) printf(" ");
					snprintf(buffer, 3, "%02x", r->inst);
					startVisual(channel, i, 1);
					if (r->inst != INST_VOID)
					{
						if (!cd->mute)
						{
							if (ifVisual(channel, i, 1))
							{
								printf("\033[22m");
								if (s->playing == PLAYING_CONT && s->playfy == i) printf("\033[1m");
							}
							if (s->instrumenti[r->inst] < s->instrumentc) printf("\033[3%dm", r->inst%6 + 1);
							else                                          printf("\033[2;37m");
							if (x+4 < LINENO_COLS) { if (x+4 > LINENO_COLS - 2) printf("%s", buffer+(LINENO_COLS - (x+4))); }
							else                                                printf("%.*s", ws.ws_col - (x+4), buffer);
							printf("\033[22;37m");
							if (ifVisual(channel, i, 1)) printf("\033[2m");
						} else
						{
							if (x+4 < LINENO_COLS) { if (x+4 > LINENO_COLS - 2) printf("%s", buffer+(LINENO_COLS - (x+4))); }
							else                                                printf("%.*s", ws.ws_col - (x+4), buffer);
						}
					} else
					{
						if (x+4 < LINENO_COLS) { if (x+4 > LINENO_COLS - 2) printf("%s", ".."+(LINENO_COLS - (x+4))); }
						else                                                printf("%.*s", ws.ws_col - (x+4), "..");
					}
					stopVisual(channel, i, 1);
					if (!cd->mute && s->playing == PLAYING_CONT && s->playfy == i) printf("\033[1m");
				}

				for (int j = 0; j <= cd->macroc; j++)
				{
					if (x+7 + 4*j <= ws.ws_col)
					{
						if (x+7 + 4*j -1 >= LINENO_COLS) printf(" ");
						snprintf(buffer, 3, "%02x", r->macro[j].v);
						startVisual(channel, i, 2+j);
						if (r->macro[j].c)
						{
							if (cd->mute)
							{
								if (x+7 + 4*j >= LINENO_COLS) printf("%c", r->macro[j].c);
								if (x+7 + 4*j +1 < LINENO_COLS) { if (x+7 + 4*j +1 > LINENO_COLS - 2) printf("%s", buffer+(LINENO_COLS - (x+7 + 4*j +1))); }
								else                                                                  printf("%.*s", ws.ws_col - (x+7 + 4*j +1), buffer);
							} else
							{
								if (ifVisual(channel, i, 2+j))
								{
									printf("\033[22m");
									if (s->playing == PLAYING_CONT && s->playfy == i) printf("\033[1m");
								}
								if (isdigit(r->macro[j].c)) /* different colour for numerical macros */ /* TODO: more colour groups */
								{
									printf("\033[35m");
									if (x+7 + 4*j >= LINENO_COLS) printf("%c", r->macro[j].c);
									if (x+7 + 4*j +1 < LINENO_COLS) { if (x+7 + 4*j +1 > LINENO_COLS - 2) printf("%s", buffer+(LINENO_COLS - (x+7 + 4*j +1))); }
									else                                                                  printf("%.*s", ws.ws_col - (x+7 + 4*j +1), buffer);
									printf("\033[37m");
								} else
								{
									printf("\033[36m");
									if (x+7 + 4*j >= LINENO_COLS) printf("%c", r->macro[j].c);
									if (x+7 + 4*j +1 < LINENO_COLS) { if (x+7 + 4*j +1 > LINENO_COLS - 2) printf("%s", buffer+(LINENO_COLS - (x+7 + 4*j +1))); }
									else                                                                  printf("%.*s", ws.ws_col - (x+7 + 4*j +1), buffer);
									printf("\033[37m");
								} if (ifVisual(channel, i, 2+j)) printf("\033[2m");
							}
						} else
						{
							if (x+7 + 4*j < LINENO_COLS) { if (x+7 + 4*j > LINENO_COLS - 3) printf("%s", "..."+(LINENO_COLS - (x+7 + 4*j))); }
							else                                                            printf("%.*s", ws.ws_col - (x+7 + 4*j), "...");
						}
						stopVisual(channel, i, 2+j);
						if (!cd->mute && s->playing == PLAYING_CONT && s->playfy == i) printf("\033[1m");
					}
				}

				printf("\033[m"); /* clear attributes before row highlighting */

				if (x+7 + 4*(cd->macroc+1) -1 <= ws.ws_col)
				{
					if (s->playing == PLAYING_CONT && s->playfy == i)
					{
						if (x+7 + 4*(cd->macroc+1) -1 < LINENO_COLS) { if (x+7 + 4*(cd->macroc+1) -1 > LINENO_COLS - 2)
							printf("%s", " - "+(LINENO_COLS - (x+7 + 4*(cd->macroc+1) -1))); }
						else printf("%.*s", ws.ws_col - (x+7 + 4*(cd->macroc+1) -1), " - ");
					} else if (s->rowhighlight && i >= STATE_ROWS && !((i-STATE_ROWS) % s->rowhighlight))
					{
						if (x+7 + 4*(cd->macroc+1) -1 < LINENO_COLS) { if (x+7 + 4*(cd->macroc+1) -1 > LINENO_COLS - 2)
							printf("%s", " * "+(LINENO_COLS - (x+7 + 4*(cd->macroc+1) -1))); }
						else printf("%.*s", ws.ws_col - (x+7 + 4*(cd->macroc+1) -1), " * ");
					} else
					{
						if (x+7 + 4*(cd->macroc+1) -1 < LINENO_COLS) { if (x+7 + 4*(cd->macroc+1) -1 > LINENO_COLS - 2)
							printf("%s", "   "+(LINENO_COLS - (x+7 + 4*(cd->macroc+1) -1))); }
						else printf("%.*s", ws.ws_col - (x+7 + 4*(cd->macroc+1) -1), "   ");
					}
				}
			}
	} return 8 + 4*(cd->macroc+1);
}

short genSfx(void)
{
	short x = 0;
	short ret = 0;

	for (int i = 0; i < s->channelc; i++)
	{
		x += 3 + 9 + 4*(s->channelv[i].data.macroc+1);
		if (i == w->channel) ret = x; /* should only be set once */
		/* keep iterating so x is the full width of all channels */
	}
	if (x > (ws.ws_col-LINENO_COLS))
	{
		ret = (ws.ws_col>>1) - ret;
		ret = MIN(ret, 0);
		ret = MAX(ret, -(x - (ws.ws_col-LINENO_COLS)));
	} else
		ret = (((ws.ws_col-LINENO_COLS) - x)>>1);
	return ret;
}

void drawTracker(void)
{
	short x = 1;
	short sx = 0;
	short sfx = 0;
	short y, macro;

	printf("\033[%d;%dH\033[1mCHANNEL\033[m \033[2mINSTRUMENT\033[m", 2, (ws.ws_col-18)>>1);

	sfx = genSfx();

	switch (w->page)
	{
		case PAGE_CHANNEL_VARIANT:
			printf("\033[%d;%dH\033[3mvariant\033[m \033[3;2meffect\033[m", 3, (ws.ws_col-14)>>1);

			drawGlobalLineNumbers(x + MAX(sfx, 0));
			x += CHANNEL_LINENO_COLS;
			if (sfx >= 0) drawStarColumn(x + MAX(sfx, 0));
			x += 2;

			for (int i = 0; i < s->channelc; i++)
			{
				drawChannelTrigs(i, x+sfx);
				x += CHANNEL_TRIG_COLS;

				if (i == w->channel + w->channeloffset)
					sx = x;

				x += drawChannel(i, x+sfx);
			}
			if (sfx < 0)
			{
				printf("\033[%d;%dH^", CHANNEL_ROW, LINENO_COLS - 1);
				for (int i = 0; i < s->songlen; i++)
					if (w->centre - w->trackerfy + i > CHANNEL_ROW && w->centre - w->trackerfy + i < ws.ws_row)
						printf("\033[%d;%dH^", w->centre - w->trackerfy + i, LINENO_COLS - 1);
			}
			if (x + sfx > ws.ws_col)
			{
				printf("\033[%d;%dH$", CHANNEL_ROW, ws.ws_col);
				for (int i = 0; i < s->songlen; i++)
					if (w->centre - w->trackerfy + i > CHANNEL_ROW && w->centre - w->trackerfy + i < ws.ws_row)
						printf("\033[%d;%dH$", w->centre - w->trackerfy + i, ws.ws_col);
			}

			y = w->centre + w->fyoffset;

			if (w->trackerfx > 1 && w->mode == T_MODE_INSERT)
			{
				Row *r = getChannelRow(&s->channelv[w->channel].data, w->trackerfy);
				short macro = (w->trackerfx - 2)>>1;
				descMacro(r->macro[macro].c, r->macro[macro].v);
			}

			switch (w->mode)
			{
				case T_MODE_VISUAL: case T_MODE_VTRIG_VISUAL:           printf("\033[%d;0H\033[1m-- VISUAL --\033[m",         ws.ws_row); w->command.error[0] = '\0'; break;
				case T_MODE_VISUALLINE:                                 printf("\033[%d;0H\033[1m-- VISUAL LINE --\033[m",    ws.ws_row); w->command.error[0] = '\0'; break;
				case T_MODE_VISUALREPLACE:                              printf("\033[%d;0H\033[1m-- VISUAL REPLACE --\033[m", ws.ws_row); w->command.error[0] = '\0'; break;
				case T_MODE_MOUSEADJUST: case T_MODE_VTRIG_MOUSEADJUST: printf("\033[%d;0H\033[1m-- MOUSE ADJUST --\033[m",   ws.ws_row); w->command.error[0] = '\0'; break;
				case T_MODE_INSERT: case T_MODE_VTRIG_INSERT:
					if (w->keyboardmacro) printf("\033[%d;0H\033[1m-- INSERT (%cxx) --\033[m", ws.ws_row, w->keyboardmacro);
					else                  printf("\033[%d;0H\033[1m-- INSERT --\033[m", ws.ws_row);
					w->command.error[0] = '\0'; break;
			}

			/* cursor position */
			if (w->mode == T_MODE_VTRIG || w->mode == T_MODE_VTRIG_INSERT || w->mode == T_MODE_VTRIG_VISUAL || w->mode == T_MODE_VTRIG_MOUSEADJUST)
				printf("\033[%d;%dH", y, sx - 2 - w->fieldpointer + sfx);
			else
			{
				switch (w->trackerfx)
				{
					case 0: printf("\033[%d;%dH", y, sx + 0 + sfx);                   break;
					case 1: printf("\033[%d;%dH", y, sx + 5 - w->fieldpointer + sfx); break;
					default: /* macro columns */
						macro = (w->trackerfx - 2)>>1;
						if (w->trackerfx % 2 == 0) printf("\033[%d;%dH", y, sx + 7 + macro*4 + sfx);
						else printf("\033[%d;%dH", y, sx + 9 + macro*4 - w->fieldpointer + sfx);
						break;
				}
			} break;
		case PAGE_CHANNEL_EFFECT:
			printf("\033[%d;%dH\033[3;2mvariant\033[m \033[3meffect\033[m", 3, (ws.ws_col-14)>>1);

			x += CHANNEL_LINENO_COLS + 2;

			for (int i = 0; i < s->channelc; i++)
			{
				x += CHANNEL_TRIG_COLS;
				x += _drawChannelHeader(i, x+sfx);
			}
			if (sfx < 0)             printf("\033[%d;%dH^", CHANNEL_ROW, LINENO_COLS - 1);
			if (x + sfx > ws.ws_col) printf("\033[%d;%dH$", CHANNEL_ROW, ws.ws_col);

			drawEffects(&s->channelv[w->channel].data.effect, (INSTRUMENT_INDEX_COLS+1)>>1, ws.ws_col - (INSTRUMENT_INDEX_COLS+1), CHANNEL_ROW + 2); break;
			break;
	}

}
