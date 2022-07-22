/* will populate buffer, buffer should be at least length 4 */
void noteToString(uint8_t note, char *buffer)
{
	if (note == NOTE_VOID) { snprintf(buffer, 4, "..."); return; }
	if (note == NOTE_OFF)  { snprintf(buffer, 4, "OFF"); return; }

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

void descMacro(char c, uint8_t v)
{
	switch (c)
	{
		case '%': printf("\033[%d;%ldH%s", ws.ws_row, (ws.ws_col - strlen("NOTE CHANCE")) / 2, "NOTE CHANCE"); break;
		case 'B': printf("\033[%d;%ldH%s", ws.ws_row, (ws.ws_col - strlen("BAND-PASS FILTER")) / 2, "BAND-PASS FILTER"); break;
		case 'b': printf("\033[%d;%ldH%s", ws.ws_row, (ws.ws_col - strlen("BPM")) / 2, "BPM"); break;
		case 'C': printf("\033[%d;%ldH%s", ws.ws_row, (ws.ws_col - strlen("NOTE CUT")) / 2, "NOTE CUT"); break;
		case 'D': printf("\033[%d;%ldH%s", ws.ws_row, (ws.ws_col - strlen("NOTE DELAY")) / 2, "NOTE DELAY"); break;
		case 'G': printf("\033[%d;%ldH%s", ws.ws_row, (ws.ws_col - strlen("STEREO GAIN")) / 2, "STEREO GAIN"); break;
		case 'g': printf("\033[%d;%ldH%s", ws.ws_row, (ws.ws_col - strlen("SMOOTH STEREO GAIN")) / 2, "SMOOTH STEREO GAIN"); break;
		case 'H': printf("\033[%d;%ldH%s", ws.ws_row, (ws.ws_col - strlen("HIGH-PASS FILTER")) / 2, "HIGH-PASS FILTER"); break;
		case 'L': printf("\033[%d;%ldH%s", ws.ws_row, (ws.ws_col - strlen("LOW-PASS FILTER")) / 2, "LOW-PASS FILTER"); break;
		case 'M': printf("\033[%d;%ldH%s", ws.ws_row, (ws.ws_col - strlen("MICROTONAL OFFSET")) / 2, "MICROTONAL OFFSET"); break;
		case 'N': printf("\033[%d;%ldH%s", ws.ws_row, (ws.ws_col - strlen("NOTCH FILTER")) / 2, "NOTCH FILTER"); break;
		case 'O': printf("\033[%d;%ldH%s", ws.ws_row, (ws.ws_col - strlen("NOTE OFFSET")) / 2, "NOTE OFFSET"); break;
		case 'o': printf("\033[%d;%ldH%s", ws.ws_row, (ws.ws_col - strlen("BACKWARDS NOTE OFFSET")) / 2, "BACKWARDS NOTE OFFSET"); break;
		case 'P': printf("\033[%d;%ldH%s", ws.ws_row, (ws.ws_col - strlen("PITCH SLIDE")) / 2, "PITCH SLIDE"); break;
		case 'R': printf("\033[%d;%ldH%s", ws.ws_row, (ws.ws_col - strlen("BLOCK RETRIGGER")) / 2, "BLOCK RETRIGGER"); break;
		case 't': printf("\033[%d;%ldH%s", ws.ws_row, (ws.ws_col - strlen("GATE")) / 2, "GATE"); break;
		case 'V': printf("\033[%d;%ldH%s", ws.ws_row, (ws.ws_col - strlen("VIBRATO")) / 2, "VIBRATO"); break;
		case 'W': /* waveshapers */
			switch (v>>4)
			{
				case 0: printf("\033[%d;%ldH%s", ws.ws_row, (ws.ws_col - strlen("HARD CLIPPER")) / 2, "HARD CLIPPER"); break;
				case 1: printf("\033[%d;%ldH%s", ws.ws_row, (ws.ws_col - strlen("SOFT CLIPPER")) / 2, "SOFT CLIPPER"); break;
				case 2: printf("\033[%d;%ldH%s", ws.ws_row, (ws.ws_col - strlen("RECTIFIER")) / 2, "RECTIFIER"); break;
				case 3: printf("\033[%d;%ldH%s", ws.ws_row, (ws.ws_col - strlen("RECTIFIERx2")) / 2, "RECTIFIERx2"); break;
				case 4: printf("\033[%d;%ldH%s", ws.ws_row, (ws.ws_col - strlen("WAVEFOLDER")) / 2, "WAVEFOLDER"); break;
				case 5: printf("\033[%d;%ldH%s", ws.ws_row, (ws.ws_col - strlen("WAVEWRAPPER")) / 2, "WAVEWRAPPER"); break;
				case 6: printf("\033[%d;%ldH%s", ws.ws_row, (ws.ws_col - strlen("SIGN CONVERSION")) / 2, "SIGN CONVERSION"); break;
			}
			break;
	}
}

void drawSongList(unsigned short x)
{
	for (int i = 0; i < 256; i++) /* reserve 0xff */
	{
		if (w->centre - w->songfy + i > CHANNEL_ROW
				&& w->centre - w->songfy + i < ws.ws_row)
		{
			printf("\033[%d;%dH", w->centre - w->songfy + i, x);
			if (w->songnext - 1 == i) printf(">");
			else if (s->songf[i])     printf("v");
			else                      printf(" ");

			switch (w->mode)
			{
				case T_MODE_SONG: case T_MODE_SONG_INSERT:
					if (s->playing && s->songp == i) printf("\033[1m%02x\033[m", s->songi[i]);
					else if (s->songi[i] == 255)     printf("..");
					else                             printf("%02x", s->songi[i]);
					break;
				case T_MODE_SONG_VISUAL:
					if (
							i >= MIN(w->visualfy, w->songfy) &&
							i <= MAX(w->visualfy, w->songfy))
					{
						if (s->playing && s->songp == i) printf("\033[2;7m%02x\033[m", s->songi[i]);
						else if (s->songi[i] == 255)     printf("\033[2;7m..\033[m");
						else                             printf("\033[2;7m%02x\033[m", s->songi[i]);
					} else
					{
						if (s->playing && s->songp == i) printf("\033[1m%02x\033[m", s->songi[i]);
						else if (s->songi[i] == 255)     printf("..");
						else                             printf("%02x", s->songi[i]);
					} break;
				default:
					if (w->songfy == i && s->songi[i] == 255)               printf("\033[7m..\033[m");
					else if (s->playing && s->songp == i && w->songfy == i) printf("\033[1;7m%02x\033[m", s->songi[i]);
					else if (w->songfy == i)                                printf("\033[7m%02x\033[m", s->songi[i]);
					else if (s->playing && s->songp == i)                   printf("\033[1m%02x\033[m", s->songi[i]);
					else if (s->songi[i] == 255)                            printf("..");
					else                                                    printf("%02x", s->songi[i]);
					break;
			}

			if (w->songnext - 1 == i) printf(">");
			else if (s->songf[i])     printf("^");
			else                      printf(" ");
		}
	}
}

void drawPatternLineNumbers(uint8_t pattern, unsigned short x)
{
	int i, c;
	for (i = 0; i <= s->patternv[s->patterni[pattern]]->rowc; i++)
		if (w->centre - w->trackerfy + i > CHANNEL_ROW && w->centre - w->trackerfy + i < ws.ws_row)
		{
			printf("\033[%d;%dH", w->centre - w->trackerfy + i, x - 1);
			printf(" %02x", i);
			if (s->playing == PLAYING_CONT && s->songp == w->songfy && s->songr == i) printf(" - ");
			else if (s->rowhighlight && !(i % s->rowhighlight))                       printf(" * ");
			else                                                                      printf("   ");
		}
	if (w->centre - w->trackerfy - 1 > 4 && w->songfy > 0 && s->songi[w->songfy - 1] != 255)
	{
		c = 0;
		for (i = w->centre - w->trackerfy - 1; i > CHANNEL_ROW; i--)
		{
			if (c > s->patternv[s->patterni[s->songi[w->songfy - 1]]]->rowc) break;
			printf("\033[%d;%dH\033[2m", i, x+2);
			if (s->playing == PLAYING_CONT && s->songp == w->songfy-1 && s->songr == s->patternv[s->patterni[s->songi[w->songfy-1]]]->rowc - c) printf(" - ");
			else if (s->rowhighlight && !((s->patternv[s->patterni[s->songi[w->songfy-1]]]->rowc - c) % s->rowhighlight))                       printf(" * ");
			else                                                                                                                                printf("   ");
			printf("\033[m");
			c++;
		}
	}
	if (w->centre - w->trackerfy + s->patternv[s->patterni[s->songi[w->songfy]]]->rowc + 1 < ws.ws_row
		&& w->songfy < 254 && s->songi[w->songfy + 1] != 255)
	{
		c = 0;
		for (i = w->centre - w->trackerfy + s->patternv[s->patterni[s->songi[w->songfy]]]->rowc + 1; i < ws.ws_row; i++)
		{
			if (c > s->patternv[s->patterni[s->songi[w->songfy + 1]]]->rowc) break;
			printf("\033[%d;%dH\033[2m", i, x+2);
			if (s->playing == PLAYING_CONT && s->songp == w->songfy + 1 && s->songr == c) printf(" - ");
			else if (s->rowhighlight && !(c % s->rowhighlight))                           printf(" * ");
			else                                                                          printf("   ");
			printf("\033[m");
			c++;
		}
	}
}

void startVisual(char *buffer, uint8_t channel, int i, signed char fieldpointer)
{
	if ((w->mode == T_MODE_VISUAL || w->mode == T_MODE_VISUALLINE) && channel == MIN(w->visualchannel, w->channel))
	{
		switch (w->mode)
		{
			case T_MODE_VISUAL:
				if (w->visualchannel == w->channel)
				{
					if (i >= MIN(w->visualfy, w->trackerfy) && i <= MAX(w->visualfy, w->trackerfy)
							&& MIN(w->visualfx, tfxToVfx(w->trackerfx)) == fieldpointer)
						strcat(buffer, "\033[2;7m");
				} else if (i >= MIN(w->visualfy, w->trackerfy) && i <= MAX(w->visualfy, w->trackerfy)
						&& (w->visualchannel <= w->channel ? w->visualfx : tfxToVfx(w->trackerfx)) == fieldpointer)
					strcat(buffer, "\033[2;7m");
				break;
			case T_MODE_VISUALLINE:
				if (i >= MIN(w->visualfy, w->trackerfy) && i <= MAX(w->visualfy, w->trackerfy) && fieldpointer == 0)
					strcat(buffer, "\033[2;7m");
				break;
		}
	}
}
void stopVisual(char *buffer, uint8_t channel, int i, signed char fieldpointer)
{
	if ((w->mode == T_MODE_VISUAL || w->mode == T_MODE_VISUALLINE) && channel == MAX(w->visualchannel, w->channel))
	{
		switch (w->mode)
		{
			case T_MODE_VISUAL:
				if (w->visualchannel == w->channel)
				{
					if (i >= MIN(w->visualfy, w->trackerfy) && i <= MAX(w->visualfy, w->trackerfy)
							&& MAX(w->visualfx, tfxToVfx(w->trackerfx)) == fieldpointer)
					{
						strcat(buffer, "\033[27m");
						if (!s->channelv[channel].mute) strcat(buffer, "\033[22m");
					}
				} else if (i >= MIN(w->visualfy, w->trackerfy) && i <= MAX(w->visualfy, w->trackerfy)
						&& (w->visualchannel >= w->channel ? w->visualfx : tfxToVfx(w->trackerfx)) == fieldpointer)
				{
					strcat(buffer, "\033[27m");
					if (!s->channelv[channel].mute) strcat(buffer, "\033[22m");
				} break;
			case T_MODE_VISUALLINE:
				if (i >= MIN(w->visualfy, w->trackerfy) && i <= MAX(w->visualfy, w->trackerfy) && fieldpointer == 3)
				{
					strcat(buffer, "\033[27m");
					if (!s->channelv[channel].mute) strcat(buffer, "\033[22m");
				} break;
		}
	}
}
int ifVisual(uint8_t channel, int i, signed char fieldpointer)
{
	if ((w->mode == T_MODE_VISUAL || w->mode == T_MODE_VISUALLINE)
			&& channel >= MIN(w->visualchannel, w->channel)
			&& channel <= MAX(w->visualchannel, w->channel))
	{
		switch (w->mode)
		{
			case T_MODE_VISUAL:
				if (w->visualchannel == w->channel)
				{
					if (i >= MIN(w->visualfy, w->trackerfy) && i <= MAX(w->visualfy, w->trackerfy)
							&& fieldpointer >= MIN(w->visualfx, tfxToVfx(w->trackerfx))
							&& fieldpointer <= MAX(w->visualfx, tfxToVfx(w->trackerfx)))
						return 1;
				} else
				{
					if (channel > MIN(w->visualchannel, w->channel) && channel < MAX(w->visualchannel, w->channel)) return 1;
					else if (channel == MIN(w->visualchannel, w->channel))
					{
						if (i >= MIN(w->visualfy, w->trackerfy) && i <= MAX(w->visualfy, w->trackerfy)
								&& fieldpointer >= (w->visualchannel <= w->channel ? w->visualfx : tfxToVfx(w->trackerfx)))
							return 1;
					} else if (channel == MAX(w->visualchannel, w->channel))
					{
						if (i >= MIN(w->visualfy, w->trackerfy) && i <= MAX(w->visualfy, w->trackerfy)
								&& fieldpointer <= (w->visualchannel >= w->channel ? w->visualfx : tfxToVfx(w->trackerfx)))
							return 1;
					}
				} break;
			case T_MODE_VISUALLINE: if (i >= MIN(w->visualfy, w->trackerfy) && i <= MAX(w->visualfy, w->trackerfy)) return 1;
		}
	} return 0;
}


void drawChannel(uint8_t channel, unsigned short x)
{
	if (s->patterni[s->songi[w->songfy]] < 1) return; /* invalid pattern */
	if (channel >= s->channelc) return;

	char xoffset = (6 + 4*s->channelv[channel].macroc - 10) / 2;
	if (s->channelv[channel].mute) printf("\033[%d;%dH\033[2mCHANNEL %02x\033[m", CHANNEL_ROW, x+xoffset, channel);
	else                           printf("\033[%d;%dH\033[1mCHANNEL %02x\033[m", CHANNEL_ROW, x+xoffset, channel);

	row r; int c;
	char buffer[16], altbuffer[6];
	char rowbuffer[256]; /* arbitrary size */
	unsigned short rowcc = s->patternv[s->patterni[s->songi[w->songfy]]]->rowcc[channel]+1;
	for (int i = 0; i <= s->patternv[s->patterni[s->songi[w->songfy]]]->rowc; i++)
		if (w->centre - w->trackerfy + i > CHANNEL_ROW && w->centre - w->trackerfy + i < ws.ws_row)
		{
			rowbuffer[0] = '\0';
			uint8_t polycutoff = w->trackerfy - w->trackerfy % rowcc;

			r = s->patternv[s->patterni[s->songi[w->songfy]]]->rowv[channel][i%rowcc];

			if (s->channelv[channel].mute || i < polycutoff || i > polycutoff + rowcc - 1) strcat(rowbuffer, "\033[2m");
			if ((w->mode == T_MODE_VISUAL || w->mode == T_MODE_VISUALLINE)
					&& channel > MIN(w->visualchannel, w->channel) && channel <= MAX(w->visualchannel, w->channel)
					&& i >= MIN(w->visualfy, w->trackerfy) && i <= MAX(w->visualfy, w->trackerfy))
				strcat(rowbuffer, "\033[2;7m");

			startVisual(rowbuffer, channel, i, 0);
			if (r.note != NOTE_VOID)
			{
				noteToString(r.note, altbuffer);
				if (s->channelv[channel].mute) strcat(rowbuffer, altbuffer);
				else
				{
					if (ifVisual(channel, i, 0)) strcat(rowbuffer, "\033[22m");
					if (r.note == NOTE_OFF) strcat(rowbuffer, "\033[31m");
					else                    strcat(rowbuffer, "\033[32m");
					strcat(rowbuffer, altbuffer); strcat(rowbuffer, "\033[37m");
					if (ifVisual(channel, i, 0)) strcat(rowbuffer, "\033[2m");
				}
			} else strcat(rowbuffer, "...");
			stopVisual(rowbuffer, channel, i, 0);

			strcat(rowbuffer, " ");

			startVisual(rowbuffer, channel, i, 1);
			if (r.inst != INST_VOID)
			{
				snprintf(buffer, 3, "%02x", r.inst);
				if (!s->channelv[channel].mute)
				{
					if (ifVisual(channel, i, 1)) strcat(rowbuffer, "\033[22m");
					strcat(rowbuffer, "\033[33m"); strcat(rowbuffer, buffer); strcat(rowbuffer, "\033[37m");
					if (ifVisual(channel, i, 1)) strcat(rowbuffer, "\033[2m");
				} else strcat(rowbuffer, buffer);
			} else strcat(rowbuffer, "..");
			stopVisual(rowbuffer, channel, i, 1);

			for (int j = 0; j < s->channelv[channel].macroc; j++)
			{
				strcat(rowbuffer, " "); startVisual(rowbuffer, channel, i, 2+j);
				if (r.macro[j].c)
				{
					if (s->channelv[channel].mute)
					{
						snprintf(buffer, 2, "%c", r.macro[j].c);   strcat(rowbuffer, buffer);
						snprintf(buffer, 3, "%02x", r.macro[j].v); strcat(rowbuffer, buffer);
					} else
					{
						if (ifVisual(channel, i, 2+j)) strcat(rowbuffer, "\033[22m");
						if (isdigit(r.macro[j].c)) /* different colour for numerical macros */ /* TODO: more colour groups */
						{
							strcat(rowbuffer, "\033[35m");
							snprintf(buffer, 2, "%c", r.macro[j].c);   strcat(rowbuffer, buffer);
							snprintf(buffer, 3, "%02x", r.macro[j].v); strcat(rowbuffer, buffer);
							strcat(rowbuffer, "\033[37m");
						} else
						{
							strcat(rowbuffer, "\033[36m");
							snprintf(buffer, 2, "%c", r.macro[j].c);   strcat(rowbuffer, buffer);
							snprintf(buffer, 3, "%02x", r.macro[j].v); strcat(rowbuffer, buffer);
							strcat(rowbuffer, "\033[37m");
						}
						if (ifVisual(channel, i, 2+j)) strcat(rowbuffer, "\033[2m");
					}
				} else strcat(rowbuffer, "...");
				stopVisual(rowbuffer, channel, i, 2+j);
			} strcat(rowbuffer, "\033[m"); /* clear attributes before row highlighting */

			if (s->playing == PLAYING_CONT && s->songp == w->songfy && s->songr == i) strcat(rowbuffer, " - ");
			else if (s->rowhighlight && !(i % s->rowhighlight))                       strcat(rowbuffer, " * ");
			else                                                                      strcat(rowbuffer, "   ");

			if (x < ws.ws_col)
				// printf("\033[%d;%dH%.*s\033[m", w->centre - w->trackerfy + i, x, ws.ws_col - x, rowbuffer);
				printf("\033[%d;%dH%.*s\033[m", w->centre - w->trackerfy + i, x, 256, rowbuffer);
		}

	printf("\033[2m");
	if (w->centre - w->trackerfy - 1 > 4 && w->songfy && s->songi[w->songfy - 1] != 255)
	{
		c = 0;
		for (int i = w->centre - w->trackerfy - 1; i > CHANNEL_ROW; i--)
		{
			if (c > s->patternv[s->patterni[s->songi[w->songfy - 1]]]->rowc) break;
			printf("\033[%d;%dH", i, x);
			r = s->patternv[s->patterni[s->songi[w->songfy - 1]]]->rowv[channel]
				[(s->patternv[s->patterni[s->songi[w->songfy - 1]]]->rowc - c)%rowcc];

			noteToString(r.note, buffer); if (r.note != NOTE_VOID) printf(buffer); else printf("...");
			printf(" ");
			snprintf(buffer, 16, "%02x", r.inst); if (r.inst < INST_VOID) printf(buffer); else printf("..");
			for (int j = 0; j < s->channelv[channel].macroc; j++)
			{
				if (r.macro[j].c)
				{
					printf(" %c",  r.macro[j].c);
					printf("%02x", r.macro[j].v);
				} else printf(" ...");
			}

			if (s->playing == PLAYING_CONT && s->songp == w->songfy - 1 && s->songr == s->patternv[s->patterni[s->songi[w->songfy - 1]]]->rowc - c)
				printf(" - ");
			else if (s->rowhighlight && !((s->patternv[s->patterni[s->songi[w->songfy - 1]]]->rowc - c) % s->rowhighlight))
				printf(" * ");
			else
				printf("   ");
			c++;
		}
	}
	if (w->centre - w->trackerfy + s->patternv[s->patterni[s->songi[w->songfy]]]->rowc + 1 < ws.ws_row
		&& w->songfy < 254 && s->songi[w->songfy + 1] != 255)
	{
		c = 0;
		for (int i = w->centre - w->trackerfy + s->patternv[s->patterni[s->songi[w->songfy]]]->rowc + 1; i < ws.ws_row; i++)
		{
			if (c > s->patternv[s->patterni[s->songi[w->songfy + 1]]]->rowc) break;
			printf("\033[%d;%dH", i, x);
			r = s->patternv[s->patterni[s->songi[w->songfy + 1]]]->rowv[channel][c%rowcc];

			noteToString(r.note, buffer); if (r.note != NOTE_VOID) printf(buffer); else printf("...");
			printf(" ");
			snprintf(buffer, 16, "%02x", r.inst); if (r.inst < INST_VOID) printf(buffer); else printf("..");
			for (int j = 0; j < s->channelv[channel].macroc; j++)
			{
				if (r.macro[j].c)
				{
					printf(" %c",  r.macro[j].c);
					printf("%02x", r.macro[j].v);
				} else printf(" ...");
			}

			if (s->playing == PLAYING_CONT && s->songp == w->songfy + 1 && s->songr == c) printf(" - ");
			else if (s->rowhighlight && !(c % s->rowhighlight))                           printf(" * ");
			else                                                                          printf("   ");
			c++;
		}
	}
	printf("\033[m");
}

void repeat(unsigned short *maxwidth, unsigned short *prevwidth)
{
	*maxwidth = LINENO_COLS + SONGLIST_COLS;
	w->visiblechannels = 0;
	for (int i = 0; i < s->channelc; i++)
	{
		if (i+w->channeloffset > s->channelc - 1) break;
		if (*maxwidth + 9 + 4*s->channelv[i+w->channeloffset].macroc > ws.ws_col) break;
		*maxwidth += 9 + 4*s->channelv[i+w->channeloffset].macroc;
		w->visiblechannels++;
	}
	if (w->channeloffset) *prevwidth = 9 + 4*s->channelv[w->channeloffset-1].macroc;
	else                  *prevwidth = 0;
}

void drawTracker(void)
{
	unsigned short x, y;
	unsigned short maxwidth, prevwidth;

	printf("\033[%d;%dH\033[1mPATTERN\033[m \033[2mINSTRUMENT\033[m", CHANNEL_ROW-2, (ws.ws_col-18) / 2);

	unsigned short sx = 0;
	uint8_t oldvisiblechannels = w->visiblechannels;

	repeat(&maxwidth, &prevwidth);
	if (w->visiblechannels != oldvisiblechannels)
		repeat(&maxwidth, &prevwidth);

	while (w->channeloffset && prevwidth <= ws.ws_col - maxwidth)
	{
		w->channeloffset--;
		uint8_t oldchannel = w->channel;
		w->channel-=2; /* hack */
		repeat(&maxwidth, &prevwidth);
		w->channel = oldchannel;
	}

	x = (ws.ws_col - maxwidth) / 2 + 1;
	drawSongList(x); x += SONGLIST_COLS;
	if (s->songi[w->songfy] != 255)
	{
		drawPatternLineNumbers(s->songi[w->songfy], x); x += LINENO_COLS;
		for (int i = 0; i < w->visiblechannels; i++)
		{
			if (i+w->channeloffset == w->channel) sx = x;
			drawChannel(i+w->channeloffset, x);
			x += 9 + 4*s->channelv[i+w->channeloffset].macroc;
		}
	} else printf("\033[%d;%dH%s", w->centre, x + (maxwidth - 19) / 2, " [INVALID PATTERN] ");

	y = w->centre + w->fyoffset;

	if (w->trackerfx > 1 && w->mode == T_MODE_INSERT)
	{
		row *r = &s->patternv[s->patterni[s->songi[w->songfy]]]->rowv[w->channel][w->trackerfy];
		short macro = (w->trackerfx - 2) / 2;
		descMacro(r->macro[macro].c, r->macro[macro].v);
	}

	switch (w->mode)
	{
		case T_MODE_VISUAL: case T_MODE_SONG_VISUAL: printf("\033[%d;0H\033[1m-- VISUAL --\033[m\033[0 q", ws.ws_row); w->command.error[0] = '\0'; break;
		case T_MODE_VISUALLINE:                      printf("\033[%d;0H\033[1m-- VISUAL LINE --\033[m\033[0 q", ws.ws_row); w->command.error[0] = '\0'; break;
		case T_MODE_MOUSEADJUST:                     printf("\033[%d;0H\033[1m-- MOUSE ADJUST --\033[m\033[0 q", ws.ws_row); w->command.error[0] = '\0'; break;
		case T_MODE_INSERT: case T_MODE_SONG_INSERT: printf("\033[%d;0H\033[1m-- INSERT --\033[m\033[3 q", ws.ws_row); w->command.error[0] = '\0'; break;
		default: printf("\033[0 q"); break;
	}

	if (w->mode == T_MODE_SONG || w->mode == T_MODE_SONG_INSERT || w->mode == T_MODE_SONG_VISUAL)
		printf("\033[%d;%dH", w->centre + w->fyoffset, (ws.ws_col - maxwidth) / 2 + 3);
	else if (s->songi[w->songfy] == 255)
		printf("\033[%d;%dH", w->centre, x + (maxwidth - 19) / 2 + 2);
	else
	{
		short macro;
		switch (w->trackerfx)
		{
			case 0: printf("\033[%d;%dH", y, sx + 0);                   break;
			case 1: printf("\033[%d;%dH", y, sx + 5 - w->fieldpointer); break;
			default: /* macro columns */
				macro = (w->trackerfx - 2) / 2;
				if (w->trackerfx % 2 == 0) printf("\033[%d;%dH", y, sx + 7 + macro*4);
				else printf("\033[%d;%dH", y, sx + 9 + macro*4 - w->fieldpointer);
				break;
		}
	}
}
