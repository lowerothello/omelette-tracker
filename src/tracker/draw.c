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

void drawGlobalLineNumbers(unsigned short x)
{
	for (int i = 0; i < s->songlen; i++)
		if (w->centre - w->trackerfy + i > CHANNEL_ROW && w->centre - w->trackerfy + i < ws.ws_row)
		{
			printf("\033[%d;%dH", w->centre - w->trackerfy + i, x);
			if (s->playing == PLAYING_CONT && s->playfy == i)                              printf("\033[1m");
			else if (i < STATE_ROWS || (s->loop[1] && (i < s->loop[0] || i > s->loop[1]))) printf("\033[2m");

			/* print as special if the row has a bpm change, or if the row hasn't been fully allocated yet */
			if (s->bpmcachelen <= i || s->bpmcache[i] != -1) printf("\033[33m");

			if (i < STATE_ROWS) printf("  -%x", STATE_ROWS - i);
			else                printf("%04x",  i - STATE_ROWS);
			printf("\033[m");
		}
}

/* VMO: visual macro order */
uint8_t tfxToVmo(ChannelData *cd, uint8_t tfx)
{
	if (tfx < 2) return tfx; /* no change for note and inst columns */

	if (tfx&0x1) /* macrov */ return (4 + (cd->macroc<<1)) - tfx;
	else         /* macroc */ return (2 + (cd->macroc<<1)) - tfx;
}
/* VMO: visual macro order */
uint8_t vfxToVmo(ChannelData *cd, uint8_t vfx)
{
	if (vfx < 2) return vfx; /* no change for note and inst columns */

	return (2 + (cd->macroc<<1)) - vfx;
}

bool startVisual(uint8_t channel, int i, int8_t fieldpointer)
{
	switch (w->mode)
	{
		case T_MODE_VISUAL:
			if (channel == MIN(w->visualchannel, w->channel)
					&& i >= MIN(w->visualfy, w->trackerfy+w->fyoffset)
					&& i <= MAX(w->visualfy, w->trackerfy+w->fyoffset))
			{
				if (w->visualchannel == w->channel)
				{
					if (fieldpointer == MIN(w->visualfx, tfxToVfx(w->trackerfx)))
					/* if (fieldpointer ==
							(vfxToVmo(&s->channel->v[channel].data, w->visualfx) < vfxToVmo(&s->channel->v[channel].data, tfxToVfx(w->trackerfx)))
							? w->visualfx : tfxToVfx(w->trackerfx)) */
					{
						printf("\033[2;7m");
						return 1;
					}
				} else if (fieldpointer == ((w->visualchannel <= w->channel) ? w->visualfx : tfxToVfx(w->trackerfx)))
				{
					printf("\033[2;7m");
					return 1;
				}
			} break;
		case T_MODE_VISUALREPLACE:
			if (channel == w->channel && fieldpointer == tfxToVfx(w->trackerfx)
					&& i >= MIN(w->visualfy, w->trackerfy+w->fyoffset)
					&& i <= MAX(w->visualfy, w->trackerfy+w->fyoffset))
			{ printf("\033[2;7m"); return 1; }
			break;
		case T_MODE_VISUALLINE:
			if (channel == MIN(w->visualchannel, w->channel) && fieldpointer == 0
					&& i >= MIN(w->visualfy, w->trackerfy+w->fyoffset)
					&& i <= MAX(w->visualfy, w->trackerfy+w->fyoffset))
			{ printf("\033[2;7m"); return 1; }
			break;
	} return 0;
}
bool stopVisual(uint8_t channel, int i, int8_t fieldpointer)
{
	switch (w->mode)
	{
		case T_MODE_VISUAL:
			if (channel == MAX(w->visualchannel, w->channel)
					&& i >= MIN(w->visualfy, w->trackerfy+w->fyoffset)
					&& i <= MAX(w->visualfy, w->trackerfy+w->fyoffset))
			{
				if (w->visualchannel == w->channel)
				{
					if (fieldpointer == MAX(w->visualfx, tfxToVfx(w->trackerfx)))
					/* if (fieldpointer ==
							(vfxToVmo(&s->channel->v[channel].data, w->visualfx) > vfxToVmo(&s->channel->v[channel].data, tfxToVfx(w->trackerfx)))
							? w->visualfx : tfxToVfx(w->trackerfx)) */
					{
						printf("\033[22;27m");
						return 1;
					}
				} else if (fieldpointer == ((w->visualchannel >= w->channel) ? w->visualfx : tfxToVfx(w->trackerfx)))
				{
					printf("\033[22;27m");
					return 1;
				}
			} break;
		case T_MODE_VISUALREPLACE:
			if (channel == w->channel
					&& i >= MIN(w->visualfy, w->trackerfy+w->fyoffset)
					&& i <= MAX(w->visualfy, w->trackerfy+w->fyoffset))
			{
				if (w->visualchannel == w->channel) { if (fieldpointer == tfxToVfx(w->trackerfx)) { printf("\033[22;27m"); return 1; } }
				else if (fieldpointer == tfxToVfx(w->trackerfx)) { printf("\033[22;27m"); return 1; }
			} break;
	} return 0;
}
int ifVisual(uint8_t channel, int i, int8_t fieldpointer)
{
	if (   channel >= MIN(w->visualchannel, w->channel)
	    && channel <= MAX(w->visualchannel, w->channel))
	{
		switch (w->mode)
		{
			case T_MODE_VISUAL:
				if (i >= MIN(w->visualfy, w->trackerfy+w->fyoffset)
				 && i <= MAX(w->visualfy, w->trackerfy+w->fyoffset))
				{
					if (w->visualchannel == w->channel)
					{
						if (       fieldpointer >= MIN(w->visualfx, tfxToVfx(w->trackerfx))
								&& fieldpointer <= MAX(w->visualfx, tfxToVfx(w->trackerfx)))
							return 1;
					} else
					{
						if (       channel > MIN(w->visualchannel, w->channel)
								&& channel < MAX(w->visualchannel, w->channel))
							return 1;
						else if (channel == MIN(w->visualchannel, w->channel)
								&& fieldpointer >= (w->visualchannel <= w->channel ? w->visualfx : tfxToVfx(w->trackerfx)))
							return 1;
						else if (channel == MAX(w->visualchannel, w->channel)
								&& fieldpointer <= (w->visualchannel >= w->channel ? w->visualfx : tfxToVfx(w->trackerfx)))
							return 1;
					}
				} break;
			case T_MODE_VISUALREPLACE:
				if (channel == w->channel
						&& i >= MIN(w->visualfy, w->trackerfy+w->fyoffset)
						&& i <= MAX(w->visualfy, w->trackerfy+w->fyoffset))
				{
					if (fieldpointer == tfxToVfx(w->trackerfx)) { printf("\033[22;27m"); return 1; }
				} break;
			case T_MODE_VISUALLINE:
				if (       i >= MIN(w->visualfy, w->trackerfy+w->fyoffset)
						&& i <= MAX(w->visualfy, w->trackerfy+w->fyoffset)
						&& fieldpointer != -1)
					return 1;
		}
	} return 0;
}

void setRowIntensity(ChannelData *cd, int i)
{
	if (cd->mute || i < STATE_ROWS || (s->loop[1] && (i < s->loop[0] || i > s->loop[1]))) printf("\033[2m");
	else if (s->playing == PLAYING_CONT && s->playfy == i)                                printf("\033[1m");
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
	x += CHANNEL_TRIG_COLS;

	char headerbuffer[11], variantbuffer[3];

	ChannelData *cd = &s->channel->v[channel].data;

	/* TODO: channel line underline */
	snprintf(headerbuffer, 11, "CHANNEL %02x", channel);

	int prevVtrig = getPrevVtrig(cd, w->trackerfy);
	int c;
	if (prevVtrig != -1 && cd->trig[prevVtrig].index != VARIANT_OFF)
	{
		if (cd->trig[prevVtrig].flags&C_VTRIG_LOOP || cd->variantv[cd->varianti[cd->trig[prevVtrig].index]]->rowc >= w->trackerfy - prevVtrig)
		{
			/* gcc likes to make variantbuffer static (fuck gcc), so be super careful about not using it uninitialized so non-conformant compilers don't crash and burn here */
			snprintf(variantbuffer, 3, "%02x", cd->trig[prevVtrig].index);
			c = x - 6;
			printf("\033[3m");
			if (c <= ws.ws_col)
			{
				if (x < LINENO_COLS + 2) { if (x > LINENO_COLS) printf("\033[%d;%dH%s", CHANNEL_ROW, LINENO_COLS, variantbuffer+(LINENO_COLS - x - 2)); }
				else                                            printf("\033[%d;%dH%.*s", CHANNEL_ROW, c, ws.ws_col - c, variantbuffer);
			}
			printf("\033[m");
		}
	}

	if (cd->mute) printf("\033[2m");
	else          printf("\033[1m");
	if (s->channel->v[channel].triggerflash) printf("\033[3%dm", channel%6 + 1);
	if (channel == w->channel + w->channeloffset) printf("\033[7m");
	c = x + ((6 + 4*(cd->macroc+1) - 10)>>1);
	if (c <= ws.ws_col)
	{
		if (x < LINENO_COLS - 2) { if (x > LINENO_COLS - 12) printf("\033[%d;%dH%s", CHANNEL_ROW, LINENO_COLS, headerbuffer+(LINENO_COLS - x - 2)); }
		else                                                 printf("\033[%d;%dH%.*s", CHANNEL_ROW, c, ws.ws_col - c, headerbuffer);
	}

	printf("\033[m");
	return 8 + 4*(cd->macroc+1) + CHANNEL_TRIG_COLS;
}
short drawChannel(uint8_t channel, short bx)
{
	if (channel >= s->channel->c) return 0;

	Row *r;
	char buffer[16];

	Variant *v;
	int gcvret;
	ChannelData *cd = &s->channel->v[channel].data;

	for (int i = 0; i < s->songlen; i++)
		if (w->centre - w->trackerfy + i > CHANNEL_ROW && w->centre - w->trackerfy + i < ws.ws_row)

	_drawChannelHeader(channel, bx);

	short x;
	if (bx <= ws.ws_col && bx + CHANNEL_TRIG_COLS + 9 + 4*(cd->macroc+1) > LINENO_COLS)
	{
		for (int i = 0; i < s->songlen; i++)
			if (w->centre - w->trackerfy + i > CHANNEL_ROW && w->centre - w->trackerfy + i < ws.ws_row)
			{
				x = bx;

				printf("\033[%d;%dH", w->centre - w->trackerfy + i, MAX(LINENO_COLS, x));

				if (ifVisual(channel, i, -1)) printf("\033[2;7m");

				if (cd->trig[i].index == VARIANT_OFF) { printf("\033[1m"); strcpy(buffer, " ==  "); }
				else if (cd->trig[i].index != VARIANT_VOID)
				{
					printf("\033[1m");
					if (cd->trig[i].flags&C_VTRIG_LOOP) snprintf(buffer, 6, "l%02x  ", cd->trig[i].index);
					else                                snprintf(buffer, 6, " %02x  ", cd->trig[i].index);
				} else
				{
					printf("\033[2m");
					int lineno = getChannelVariant(NULL, cd, i);
					if (lineno == -1) strcpy(buffer, " ..  ");
					else              snprintf(buffer, 6, " %02x  ", lineno);
				}

				if (x < LINENO_COLS) { if (x > LINENO_COLS - 5) printf("%s", buffer+(LINENO_COLS - x)); }
				else                                            printf("%.*s", ws.ws_col - x, buffer);

				stopVisual(channel, i, -1);
				printf("\033[22m");

				x += CHANNEL_TRIG_COLS;

				r = getChannelRow(cd, i);
				setRowIntensity(cd, i);

				/* use underline to split up variants more visually */
				gcvret = getChannelVariant(&v, cd, i);
				if ((i < s->songlen-1 && (cd->trig[i+1].index < VARIANT_MAX || (cd->trig[i+1].index == VARIANT_OFF && gcvret != -1)))
						|| (gcvret != -1 && (gcvret%(v->rowc+1) == v->rowc)))
					printf("\033[4m");

				if (ifVisual(channel, i, 0)) printf("\033[2;7m");

				x += 4;

				if (x <= ws.ws_col)
				{
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
					setRowIntensity(cd, i);
				}

				x+=4;

				if (x <= ws.ws_col)
				{
					if (x-1 >= LINENO_COLS) printf(" ");
					snprintf(buffer, 3, "%02x", r->inst);
					startVisual(channel, i, 1);
					if (r->inst != INST_VOID)
					{
						if (!cd->mute)
						{
							if (ifVisual(channel, i, 1))
							{
								printf("\033[22m");
								setRowIntensity(cd, i);
							}
							if (instrumentSafe(s, r->inst)) printf("\033[3%dm", r->inst%6 + 1);
							else                            printf("\033[37m");
							if (x < LINENO_COLS) { if (x > LINENO_COLS - 2) printf("%s", buffer+(LINENO_COLS - x)); }
							else                                            printf("%.*s", ws.ws_col - x, buffer);
							printf("\033[22;37m");
							if (ifVisual(channel, i, 1)) printf("\033[2m");
						} else
						{
							if (x < LINENO_COLS) { if (x > LINENO_COLS - 2) printf("%s", buffer+(LINENO_COLS - x)); }
							else                                            printf("%.*s", ws.ws_col - x, buffer);
						}
					} else
					{
						if (x < LINENO_COLS) { if (x > LINENO_COLS - 2) printf("%s", ".."+(LINENO_COLS - x)); }
						else                                            printf("%.*s", ws.ws_col - x, "..");
					}
					stopVisual(channel, i, 1);
					setRowIntensity(cd, i);
				}

				x+=3;

				for (int j = cd->macroc; j >= 0; j--)
				{
					if (x <= ws.ws_col)
					{
						if (x-1 >= LINENO_COLS) printf(" ");
						startVisual(channel, i, 2+j);
						if (r->macro[j].c)
						{
							if (ifVisual(channel, i, 2+j))
							{
								printf("\033[22m");
								setRowIntensity(cd, i);
								   if (r->macro[j].alt) printf("\033[3;27m");
							} else if (r->macro[j].alt) printf("\033[3;7m");

							snprintf(buffer, 3, "%02x", r->macro[j].v);
							if (cd->mute)
							{
								if (x   >= LINENO_COLS) printf("%c", r->macro[j].c);
								if (x+1 <  LINENO_COLS) { if (x+1 > LINENO_COLS - 2) printf("%s", buffer+(LINENO_COLS - (x+1))); }
								else                                                 printf("%.*s", ws.ws_col - (x+1), buffer);
							} else
							{
								/* TODO: macro colour groups */
								printf("\033[35m");
								if (x   >= LINENO_COLS) printf("%c", r->macro[j].c);
								if (x+1 <  LINENO_COLS) { if (x+1 > LINENO_COLS - 2) printf("%s", buffer+(LINENO_COLS - (x+1))); }
								else                                                 printf("%.*s", ws.ws_col - (x+1), buffer);
							}

							if (ifVisual(channel, i, 2+j)) printf("\033[23;2;7;37m");
							else                           printf("\033[23;27;37m");
						} else
						{
							if (x < LINENO_COLS) { if (x > LINENO_COLS - 3) printf("%s", "..."+(LINENO_COLS - x)); }
							else                                            printf("%.*s", ws.ws_col - x, "...");
						}
						stopVisual(channel, i, 2+j);
						setRowIntensity(cd, i);
					}
					x += 4;
				}

				printf("\033[m"); /* clear attributes before row highlighting */

				x--;

				if (x <= ws.ws_col)
				{
					if (s->playing == PLAYING_CONT && s->playfy == i)
					{
						if (x < LINENO_COLS) { if (x > LINENO_COLS - 2)
							printf("%s", " - "+(LINENO_COLS - x)); }
						else printf("%.*s", ws.ws_col - x, " - ");
					} else if (s->rowhighlight && i >= STATE_ROWS && !((i-STATE_ROWS) % s->rowhighlight))
					{
						if (x < LINENO_COLS) { if (x > LINENO_COLS - 2)
							printf("%s", " * "+(LINENO_COLS - x)); }
						else printf("%.*s", ws.ws_col - x, " * ");
					} else
					{
						if (x < LINENO_COLS) { if (x > LINENO_COLS - 2)
							printf("%s", "   "+(LINENO_COLS - x)); }
						else printf("%.*s", ws.ws_col - x, "   ");
					}
				}

				printf(" ");
			}
	} return CHANNEL_TRIG_COLS + 8 + 4*(cd->macroc+1);
}

short genSfx(void)
{
	short x = 0;
	short ret = 0;

	for (int i = 0; i < s->channel->c; i++)
	{
		x += CHANNEL_TRIG_COLS + 8 + 4*(s->channel->v[i].data.macroc+1);
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

	printf("\033[%d;%dH\033[1mchannel\033[m \033[2mINSTRUMENT\033[m", 2, (ws.ws_col-18)>>1);

	sfx = genSfx();

	switch (w->page)
	{
		case PAGE_CHANNEL_VARIANT:
			printf("\033[%d;%dH\033[3mvariant\033[m \033[3;2mEFFECT\033[m", 3, (ws.ws_col-14)>>1);

			drawGlobalLineNumbers(x + MAX(sfx, 0));
			x += CHANNEL_LINENO_COLS;
			if (sfx >= 0) drawStarColumn(x + MAX(sfx, 0));
			x += 2;

			for (int i = 0; i < s->channel->c; i++)
			{
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
				Row *r = getChannelRow(&s->channel->v[w->channel].data, w->trackerfy);
				short macro = (w->trackerfx - 2)>>1;
				descMacro(r->macro[macro].c, r->macro[macro].v, r->macro[macro].alt);
			}

			switch (w->mode)
			{
				case T_MODE_VISUAL:        printf("\033[%d;0H\033[1m-- VISUAL --\033[m",         ws.ws_row); w->command.error[0] = '\0'; break;
				case T_MODE_VISUALLINE:    printf("\033[%d;0H\033[1m-- VISUAL LINE --\033[m",    ws.ws_row); w->command.error[0] = '\0'; break;
				case T_MODE_VISUALREPLACE: printf("\033[%d;0H\033[1m-- VISUAL REPLACE --\033[m", ws.ws_row); w->command.error[0] = '\0'; break;
				case T_MODE_MOUSEADJUST:   printf("\033[%d;0H\033[1m-- MOUSE ADJUST --\033[m",   ws.ws_row); w->command.error[0] = '\0'; break;
				case T_MODE_INSERT:
					if (w->keyboardmacro)
					{
						if (w->keyboardmacroalt) printf("\033[%d;0H\033[1m-- INSERT (\033[3;7m%cxx\033[23;27m) --\033[m", ws.ws_row, w->keyboardmacro);
						else                     printf("\033[%d;0H\033[1m-- INSERT (%cxx) --\033[m", ws.ws_row, w->keyboardmacro);
					} else printf("\033[%d;0H\033[1m-- INSERT --\033[m", ws.ws_row);
					w->command.error[0] = '\0'; break;
			}

			switch (w->trackerfx)
			{
				case -1: printf("\033[%d;%dH", y, sx + 2  + sfx - w->fieldpointer); break;
				case  0: printf("\033[%d;%dH", y, sx + 5  + sfx); break;
				case  1: printf("\033[%d;%dH", y, sx + 10 + sfx - w->fieldpointer); break;
				default: /* macro columns */
					macro = (w->trackerfx - 2)>>1;
					if (w->trackerfx % 2 == 0) printf("\033[%d;%dH", y, sx + 12 + (s->channel->v[w->channel].data.macroc - macro)*4 + sfx);
					else printf("\033[%d;%dH", y, sx + 14 + (s->channel->v[w->channel].data.macroc - macro)*4 - w->fieldpointer + sfx);
					break;
			} break;
		case PAGE_CHANNEL_EFFECT:
			printf("\033[%d;%dH\033[3;2mVARIANT\033[m \033[3meffect\033[m", 3, (ws.ws_col-14)>>1);

			x += CHANNEL_LINENO_COLS + 2;

			for (int i = 0; i < s->channel->c; i++)
				x += _drawChannelHeader(i, x+sfx);

			if (sfx < 0)             printf("\033[%d;%dH^", CHANNEL_ROW, LINENO_COLS - 1);
			if (x + sfx > ws.ws_col) printf("\033[%d;%dH$", CHANNEL_ROW, ws.ws_col);

			drawEffects(s->channel->v[w->channel].data.effect, (INSTRUMENT_INDEX_COLS+1)>>1, ws.ws_col - (INSTRUMENT_INDEX_COLS+1), CHANNEL_ROW + 2); break;
			break;
	}

}
