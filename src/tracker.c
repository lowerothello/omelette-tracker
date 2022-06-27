#define T_MODE_NORMAL 0
#define T_MODE_VISUAL 1
#define T_MODE_SONG 2
#define T_MODE_MOUSEADJUST 3
#define T_MODE_INSERT 4
#define T_MODE_ARROWVISUAL 5
#define T_MODE_VISUALLINE 6

/* will populate buffer, buffer should be at least length 4 */
void noteToString(uint8_t note, char *buffer)
{
	if (note == 0)   { snprintf(buffer, 4, "..."); return; }
	if (note == 255) { snprintf(buffer, 4, "OFF"); return; }

	note = note - 1;

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
void changeMacro(int input, char *dest)
{
	if (isdigit(input)) *dest = input;
	else switch (input)
	{
		case 'b': *dest = 'B'; break; /* bpm         */
		case 'c': *dest = 'C'; break; /* cut         */
		case 'd': *dest = 'D'; break; /* delay       */
		case 'D': *dest = 'd'; break; /* fine delay  */
		case 'g': *dest = 'G'; break; /* gain        */
		case 'G': *dest = 'g'; break; /* smooth gain */
		case 'p': *dest = 'P'; break; /* portamento  */
		case 'r': *dest = 'R'; break; /* retrigger   */
		case 'v': *dest = 'V'; break; /* vibrato     */
		case 'w': *dest = 'W'; break; /* waveshaper  */
	}
}

int drawPatternLineNumbers(uint8_t pattern)
{
	for (int i = 0; i <= s->patternv[s->patterni[pattern]]->rowc; i++)
		if (w->centre - w->trackerfy + i > 5 && w->centre - w->trackerfy + i < ws.ws_row)
		{
			printf("\033[%d;%dH", w->centre - w->trackerfy + i, w->trackercelloffset);
			printf("%02x", i);
			if (s->playing == PLAYING_CONT && s->songp == w->songfx && s->songr == i)
				printf(" - ");
			else if (s->rowhighlight && !(i % s->rowhighlight))
				printf(" * ");
			else
				printf("   ");
		}
	return 0;
}
void drawSong(void)
{
	for (unsigned short i = 0; i < w->songvisible; i++)
	{
		unsigned short x = w->songcelloffset + SONG_COLS * i;

		if (s->songf[i + w->songoffset])
			switch (s->songf[i + w->songoffset])
			{
				case 1:  printf("\033[3;%dHloop", x); break;
				default: printf("\033[3;%dH????", x); break; /* corrupt file */
			}
		if (w->songnext == i + w->songoffset + 1)
			printf("\033[4;%dHnext", x);

		if (s->playing && i + w->songoffset == s->songp) printf("\033[7m");
		if (i + w->songoffset == w->songfx) printf("\033[1m");
		if (s->songi[i + w->songoffset] == 255)
			printf("\033[2;%dH == ", x);
		else
			printf("\033[2;%dH %02x ", x, s->songi[i + w->songoffset]);
		printf("\033[m ");
	}
}

int drawChannel(uint8_t channel, uint8_t screenpos)
{
	if (s->patterni[s->songi[w->songfx]] < 1) return 1; /* invalid pattern */
	if (channel >= s->channelc) return 1;

	const unsigned short x = w->trackercelloffset + LINENO_COLS + ROW_COLS * screenpos;
	if (s->channelv[channel].mute) // mute
		printf("\033[5;%dH \033[2m<CHANNEL %02x>\033[m", x, channel);
	else
		printf("\033[5;%dH <CHANNEL %02x>", x, channel);

	row r;
	int i, c;
	char buffer[16], altbuffer[6];

	for (i = 0; i <= s->patternv[s->patterni[s->songi[w->songfx]]]->rowc; i++)
		if (w->centre - w->trackerfy + i > 5 && w->centre - w->trackerfy + i < ws.ws_row)
		{
			printf("\033[%d;%dH", w->centre - w->trackerfy + i, x);

			void startVisual(signed char fieldpointer)
			{
				if (!w->popup && channel == MIN(w->visualchannel, w->channel))
				{
					switch (w->mode)
					{
						case T_MODE_VISUAL:
						case T_MODE_ARROWVISUAL:
							if (w->visualchannel == w->channel)
							{
								if (i >= MIN(w->visualfy, w->trackerfy)
										&& i <= MAX(w->visualfy, w->trackerfy)
										&& MIN(w->visualfx, tfxToVfx(w->trackerfx)) == fieldpointer)
									printf("\033[2;7m");
							} else if (i >= MIN(w->visualfy, w->trackerfy)
									&& i <= MAX(w->visualfy, w->trackerfy)
									&& (w->visualchannel <= w->channel ? w->visualfx : tfxToVfx(w->trackerfx)) == fieldpointer)
								printf("\033[2;7m");
							break;
						case T_MODE_VISUALLINE:
							if (i >= MIN(w->visualfy, w->trackerfy)
									&& i <= MAX(w->visualfy, w->trackerfy)
									&& fieldpointer == 0)
								printf("\033[2;7m");
							break;
					}
				}
			}
			void stopVisual(signed char fieldpointer)
			{
				if (!w->popup && (w->mode == T_MODE_VISUAL
							|| w->mode == T_MODE_ARROWVISUAL
							|| w->mode == T_MODE_VISUALLINE)
						&& channel == MAX(w->visualchannel, w->channel))
				{
					switch (w->mode)
					{
						case T_MODE_VISUAL:
						case T_MODE_ARROWVISUAL:
							if (w->visualchannel == w->channel)
							{
								if (i >= MIN(w->visualfy, w->trackerfy)
										&& i <= MAX(w->visualfy, w->trackerfy)
										&& MAX(w->visualfx, tfxToVfx(w->trackerfx)) == fieldpointer)
								{
									printf("\033[27m");
									if (!s->channelv[channel].mute) printf("\033[22m");
								}
							} else if (i >= MIN(w->visualfy, w->trackerfy)
									&& i <= MAX(w->visualfy, w->trackerfy)
									&& (w->visualchannel >= w->channel ? w->visualfx : tfxToVfx(w->trackerfx)) == fieldpointer)
							{
								printf("\033[27m");
								if (!s->channelv[channel].mute) printf("\033[22m");
							}
							break;
						case T_MODE_VISUALLINE:
							if (i >= MIN(w->visualfy, w->trackerfy)
									&& i <= MAX(w->visualfy, w->trackerfy)
									&& fieldpointer == 3)
							{
								printf("\033[27m");
								if (!s->channelv[channel].mute) printf("\033[22m");
							}
							break;
					}
				}
			}
			int ifVisual(signed char fieldpointer)
			{
				if (!w->popup && (w->mode == T_MODE_VISUAL
							|| w->mode == T_MODE_ARROWVISUAL
							|| w->mode == T_MODE_VISUALLINE)
						&& channel >= MIN(w->visualchannel, w->channel)
						&& channel <= MAX(w->visualchannel, w->channel))
				{
					switch (w->mode)
					{
						case T_MODE_VISUAL:
						case T_MODE_ARROWVISUAL:
							if (w->visualchannel == w->channel)
							{
								if (       i >= MIN(w->visualfy, w->trackerfy)
										&& i <= MAX(w->visualfy, w->trackerfy)
										&& fieldpointer >= MIN(w->visualfx, tfxToVfx(w->trackerfx))
										&& fieldpointer <= MAX(w->visualfx, tfxToVfx(w->trackerfx)))
									return 1;
							} else
							{
								if (channel > MIN(w->visualchannel, w->channel)
										&& channel < MAX(w->visualchannel, w->channel))
									return 1;
								else if (channel == MIN(w->visualchannel, w->channel))
								{
									if (       i >= MIN(w->visualfy, w->trackerfy)
											&& i <= MAX(w->visualfy, w->trackerfy)
											&& fieldpointer >= (w->visualchannel <= w->channel ? w->visualfx : tfxToVfx(w->trackerfx)))
										return 1;
								} else if (channel == MAX(w->visualchannel, w->channel))
								{
									if (       i >= MIN(w->visualfy, w->trackerfy)
											&& i <= MAX(w->visualfy, w->trackerfy)
											&& fieldpointer <= (w->visualchannel >= w->channel ? w->visualfx : tfxToVfx(w->trackerfx)))
										return 1;
								}
							}
							break;
						case T_MODE_VISUALLINE:
							if (i >= MIN(w->visualfy, w->trackerfy)
									&& i <= MAX(w->visualfy, w->trackerfy))
								return 1;
							break;
					}
				}
				return 0;
			}

			/* special attributes */
			if (s->channelv[channel].mute) printf("\033[2m");
			if (!w->popup && (w->mode == T_MODE_VISUAL
						|| w->mode == T_MODE_ARROWVISUAL
						|| w->mode == T_MODE_VISUALLINE)
					&& channel > MIN(w->visualchannel, w->channel)
					&& channel <= MAX(w->visualchannel, w->channel)
					&& i >= MIN(w->visualfy, w->trackerfy)
					&& i <= MAX(w->visualfy, w->trackerfy))
				printf("\033[2;7m");

			r = s->patternv[s->patterni[s->songi[w->songfx]]]->rowv[channel][i];

			startVisual(0);
			noteToString(r.note, altbuffer);
			if (r.note == 255) snprintf(buffer, 16, "\033[31m%s\033[37m", altbuffer);
			else               snprintf(buffer, 16, "\033[32m%s\033[37m", altbuffer);

			if (r.note)
			{
				if (ifVisual(0) && !s->channelv[channel].mute)
				{
					printf("\033[22m");
					printf(buffer);
					printf("\033[2m");
				} else printf(buffer);
			} else printf("...");
			stopVisual(0);

			printf(" ");

			startVisual(1);
			snprintf(buffer, 16, "\033[33m%02x\033[37m", r.inst);
			if (r.inst < 255)
			{
				if (ifVisual(1) && !s->channelv[channel].mute)
				{
					printf("\033[22m");
					printf(buffer);
					printf("\033[2m");
				} else printf(buffer);
			} else printf("..");
			stopVisual(1);

			printf(" ");

			startVisual(2);
			if (r.macroc[0])
			{
				if (isdigit(r.macroc[0])) /* different colour for instrument macros */
					snprintf(buffer, 16, "\033[31m%c%02x\033[37m", r.macroc[0], r.macrov[0]);
				else
					snprintf(buffer, 16, "\033[34m%c%02x\033[37m", r.macroc[0], r.macrov[0]);

				if (ifVisual(2) && !s->channelv[channel].mute)
				{
					printf("\033[22m");
					printf(buffer);
					printf("\033[2m");
				} else printf(buffer);
			} else printf("...");
			stopVisual(2);

			printf(" ");

			startVisual(3);
			if (r.macroc[1])
			{
				if (isdigit(r.macroc[1])) /* different colour for instrument macros */
					snprintf(buffer, 16, "\033[31m%c%02x\033[37m", r.macroc[1], r.macrov[1]);
				else
					snprintf(buffer, 16, "\033[35m%c%02x\033[37m", r.macroc[1], r.macrov[1]);

				if (ifVisual(3) && !s->channelv[channel].mute)
				{
					printf("\033[22m");
					printf(buffer);
					printf("\033[2m");
				} else printf(buffer);
			} else printf("...");
			stopVisual(3);

			printf("\033[m"); /* always clear all attributes */

			if (s->playing == PLAYING_CONT && s->songp == w->songfx && s->songr == i)
				printf(" - ");
			else if (s->rowhighlight && !(i % s->rowhighlight))
				printf(" * ");
			else
				printf("   ");
		}
	if (w->centre - w->trackerfy - 1 > 4 && w->songfx > 0 && s->songi[w->songfx - 1] != 255)
	{
		c = 0;
		for (i = w->centre - w->trackerfy - 1; i > 5; i--)
		{
			if (c > s->patternv[s->patterni[s->songi[w->songfx - 1]]]->rowc) break;
			printf("\033[%d;%dH\033[2m", i, x);
			r = s->patternv[s->patterni[s->songi[w->songfx - 1]]]->rowv[channel]
				[s->patternv[s->patterni[s->songi[w->songfx - 1]]]->rowc - c];

			noteToString(r.note, buffer);
			if (r.note) printf(buffer); else printf("...");
			printf(" ");
			snprintf(buffer, 16, "%02x", r.inst);
			if (r.inst < 255) printf(buffer); else printf("..");
			printf(" ");
			snprintf(buffer, 16, "%c", r.macroc[0]);
			if (r.macroc[0]) printf(buffer); else printf(".");
			snprintf(buffer, 16, "%02x", r.macrov[0]);
			if (r.macroc[0]) printf(buffer); else printf("..");
			printf(" ");
			snprintf(buffer, 16, "%c", r.macroc[1]);
			if (r.macroc[1]) printf(buffer); else printf(".");
			snprintf(buffer, 16, "%02x", r.macrov[1]);
			if (r.macroc[1]) printf(buffer); else printf("..");

			printf("\033[m");

			c++;
		}
	}
	if (w->centre - w->trackerfy + s->patternv[s->patterni[s->songi[w->songfx]]]->rowc + 1 < ws.ws_row
		&& w->songfx < 254 && s->songi[w->songfx + 1] != 255)
	{
		c = 0;
		for (i = w->centre - w->trackerfy + s->patternv[s->patterni[s->songi[w->songfx]]]->rowc + 1; i < ws.ws_row; i++)
		{
			if (c > s->patternv[s->patterni[s->songi[w->songfx + 1]]]->rowc) break;
			printf("\033[%d;%dH\033[2m", i, x);
			r = s->patternv[s->patterni[s->songi[w->songfx + 1]]]->rowv[channel][c];

			noteToString(r.note, buffer);
			if (r.note) printf(buffer); else printf("...");
			printf(" ");
			snprintf(buffer, 16, "%02x", r.inst);
			if (r.inst < 255) printf(buffer); else printf("..");
			printf(" ");
			snprintf(buffer, 16, "%c", r.macroc[0]);
			if (r.macroc[0]) printf(buffer); else printf(".");
			snprintf(buffer, 16, "%02x", r.macrov[0]);
			if (r.macroc[0]) printf(buffer); else printf("..");
			printf(" ");
			snprintf(buffer, 16, "%c", r.macroc[1]);
			if (r.macroc[1]) printf(buffer); else printf(".");
			snprintf(buffer, 16, "%02x", r.macrov[1]);
			if (r.macroc[1]) printf(buffer); else printf("..");

			printf("\033[m");

			c++;
		}
	}

	return 0;
}

void descMacro(char c, uint8_t v)
{
	switch (c)
	{
		case 'B': /* bpm         */
			printf("\033[%d;%ldH%s", ws.ws_row, (ws.ws_col - strlen("BPM")) / 2, "BPM"); break;
		case 'C': /* cut         */
			printf("\033[%d;%ldH%s", ws.ws_row, (ws.ws_col - strlen("NOTE CUT")) / 2, "NOTE CUT"); break;
		case 'D': /* delay       */
			printf("\033[%d;%ldH%s", ws.ws_row, (ws.ws_col - strlen("NOTE DELAY")) / 2, "NOTE DELAY"); break;
		case 'd': /* fine delay  */
			printf("\033[%d;%ldH%s", ws.ws_row, (ws.ws_col - strlen("FINE NOTE DELAY")) / 2, "FINE NOTE DELAY"); break;
		case 'G': /* gain        */
			printf("\033[%d;%ldH%s", ws.ws_row, (ws.ws_col - strlen("GAIN/PAN")) / 2, "GAIN/PAN"); break;
		case 'g': /* smooth gain */
			printf("\033[%d;%ldH%s", ws.ws_row, (ws.ws_col - strlen("SMOOTH GAIN/PAN")) / 2, "SMOOTH GAIN/PAN"); break;
		case 'P': /* portamento  */
			printf("\033[%d;%ldH%s", ws.ws_row, (ws.ws_col - strlen("PITCH SLIDE")) / 2, "PITCH SLIDE"); break;
		case 'R': /* retrigger   */
			printf("\033[%d;%ldH%s", ws.ws_row, (ws.ws_col - strlen("RETRIGGER")) / 2, "RETRIGGER"); break;
		case 'V': /* vibrato     */
			printf("\033[%d;%ldH%s", ws.ws_row, (ws.ws_col - strlen("VIBRATO")) / 2, "VIBRATO"); break;
		case 'W': /* waveshaper  */
			switch (v>>4)
			{
				case 0:
					printf("\033[%d;%ldH%s", ws.ws_row, (ws.ws_col - strlen("HARD CLIPPING")) / 2, "HARD CLIPPING"); break;
				case 1:
					printf("\033[%d;%ldH%s", ws.ws_row, (ws.ws_col - strlen("SOFT CLIPPING (odd harmonics)")) / 2, "SOFT CLIPPING (odd harmonics)"); break;
				case 2:
					printf("\033[%d;%ldH%s", ws.ws_row, (ws.ws_col - strlen("RECTIFIER (even harmonics)")) / 2, "RECTIFIER (even harmonics)"); break;
				case 3:
					printf("\033[%d;%ldH%s", ws.ws_row, (ws.ws_col - strlen("RECTIFIER x2 (octave)")) / 2, "RECTIFIER x2 (octave)"); break;
				case 4:
					printf("\033[%d;%ldH%s", ws.ws_row, (ws.ws_col - strlen("WAVEFOLDER")) / 2, "WAVEFOLDER"); break;
				case 5:
					printf("\033[%d;%ldH%s", ws.ws_row, (ws.ws_col - strlen("WAVEWRAPPER")) / 2, "WAVEWRAPPER"); break;
				case 6:
					printf("\033[%d;%ldH%s", ws.ws_row, (ws.ws_col - strlen("SIGNED UNSIGNED CONVERSION")) / 2, "SIGNED UNSIGNED CONVERSION"); break;
			}
			break;
	}
}

void drawTracker(void)
{
	unsigned char i;
	unsigned short x, y;

	while ( // try to show s indices
		w->songfx < w->songoffset + 2
		&& w->songoffset > 0
	) w->songoffset--;
	while (
		w->songfx > w->songoffset + w->songvisible - 3
		&& w->songoffset + w->songvisible < 255
	) w->songoffset++;

	drawSong();
	if (s->songi[w->songfx] != 255)
	{
		drawPatternLineNumbers(s->songi[w->songfx]);

		if (w->visiblechannels > 1)
		{ // try to follow the focus in a smooth way
			while (w->channel < w->channeloffset + 1
					&& w->channeloffset > 0)
				w->channeloffset--;
			while (w->channel > w->channeloffset + w->visiblechannels - 2
					&& w->channeloffset + w->visiblechannels < s->channelc)
				w->channeloffset++;
		} else w->channeloffset = w->channel; // if only one channel is visible then pin it

		uint8_t drawn = 0;
		for (i = 0; i <= s->channelc; i++)
			if (i >= w->channeloffset && i < w->channeloffset + w->visiblechannels)
			{
				drawChannel(i, drawn);
				drawn++;
			}
	} else printf("\033[%d;%dH%s", w->centre, (ws.ws_col - (unsigned short)strlen("(invalid pattern)")) / 2, "(invalid pattern)");

	y = w->centre + w->fyoffset;
	x = w->trackercelloffset + LINENO_COLS + ROW_COLS * (w->channel - w->channeloffset);

	/* top ruler */
	printf("\033[0;0H\033[1momelette tracker\033[0;%dHv%d.%2d      %d\033[m",
			ws.ws_col - 19, MAJOR, MINOR, DEBUG);
	/* bottom ruler */
	if (w->mode < 255)
	{
		if (w->instrumentrecv == INST_REC_LOCK_CONT)
		{
			if (w->recptr == 0)
				printf("\033[%d;%dH\033[3m{REC   0s}\033[m", ws.ws_row, ws.ws_col - 50);
			else
				printf("\033[%d;%dH\033[3m{REC %3ds}\033[m", ws.ws_row, ws.ws_col - 50, w->recptr / samplerate + 1);
		}
		if (w->chord)
			printf("\033[%d;%dH%c", ws.ws_row, ws.ws_col - 23, w->chord);
		printf("\033[%d;%dH", ws.ws_row, ws.ws_col - 17);
		if (s->playing == PLAYING_STOP)
			printf("STOP  ");
		else
			printf("PLAY  ");
		printf("&%d +%x  B%02x", w->octave, w->step, s->songbpm);
	}

	if (w->trackerfx > 1)
	{
		row *r = &s->patternv[s->patterni[s->songi[w->songfx]]]->rowv[w->channel][w->trackerfy];
		switch (w->trackerfx)
		{
			case 2: case 3: descMacro(r->macroc[0], r->macrov[0]); break;
			case 4: case 5: descMacro(r->macroc[1], r->macrov[1]); break;
		}
	}

	if (s->songi[w->songfx] == 255)
	{
		printf("\033[%d;%ldH", w->centre, (ws.ws_col - strlen("(invalid pattern)")) / 2 + 1);
	}

	switch (w->mode) /* cursor */
	{
		case T_MODE_SONG: /* song */
			printf("\033[%d;0H\033[1m-- SONG --\033[m", ws.ws_row);
			w->command.error[0] = '\0';
			printf("\033[2;%dH", w->songcelloffset + (w->songfx - w->songoffset) * SONG_COLS + 2);
			break;
		default:
			if (w->mode == T_MODE_VISUAL)
			{
				printf("\033[%d;0H\033[1m-- VISUAL --\033[m", ws.ws_row);
				printf("\033[0 q");
				w->command.error[0] = '\0';
			} else if (w->mode == T_MODE_ARROWVISUAL)
			{
				printf("\033[%d;0H\033[1m-- ARROW VISUAL --\033[m", ws.ws_row);
				printf("\033[0 q");
				w->command.error[0] = '\0';
			} else if (w->mode == T_MODE_VISUALLINE)
			{
				printf("\033[%d;0H\033[1m-- VISUAL LINE --\033[m", ws.ws_row);
				printf("\033[0 q");
				w->command.error[0] = '\0';
			} else if (w->mode == T_MODE_MOUSEADJUST)
			{
				printf("\033[%d;0H\033[1m-- MOUSE ADJUST --\033[m", ws.ws_row);
				printf("\033[0 q");
				w->command.error[0] = '\0';
			} else if (w->mode == T_MODE_INSERT)
			{
				printf("\033[%d;0H\033[1m-- INSERT --\033[m", ws.ws_row);
				printf("\033[3 q");
				w->command.error[0] = '\0';
			} else
			{
				printf("\033[0 q");
			}

			if (s->songi[w->songfx] == 255)
			{
				printf("\033[%d;%ldH", w->centre, (ws.ws_col - strlen("(invalid pattern)")) / 2 + 1);
			} else
				switch (w->trackerfx)
				{
					case 0: printf("\033[%d;%dH", y, x + 0);                    break;
					case 1: printf("\033[%d;%dH", y, x + 5 - w->fieldpointer);  break;
					case 2: printf("\033[%d;%dH", y, x + 7);                    break;
					case 3: printf("\033[%d;%dH", y, x + 9 - w->fieldpointer);  break;
					case 4: printf("\033[%d;%dH", y, x + 11);                   break;
					case 5: printf("\033[%d;%dH", y, x + 13 - w->fieldpointer); break;
				}
			break;
	}
}

void prunePattern(uint8_t index, short callingindex)
{
	if (index == 255) return; /* invalid index */
	if (s->patterni[index] == 0) return; /* pattern doesn't exist */

	/* don't remove if pattern is still referenced */
	for (short i = 0; i < 256; i++)
		if (s->songi[i] == index && i != callingindex)
			return;

	pattern *p = s->patternv[s->patterni[index]];
	/* don't remove if pattern is populated */
	for (short i = 0; i < p->rowc + 1; i++)
		for (short c = 0; c < s->channelc; c++)
		{
			row r = p->rowv[c][i];
			if (r.note || r.macroc[0] || r.macroc[1])
				return;
		}

	delPattern(index);
}
void inputSongHex(char value)
{
	prunePattern(s->songi[w->songfx], w->songfx);
	if (s->songi[w->songfx] == 255)
		s->songi[w->songfx] = 0;
	s->songi[w->songfx] <<= 4; s->songi[w->songfx] += value;
	addPattern(s->songi[w->songfx], 0);
}
void inputPatternHex(row *r, char value)
{
	switch (w->trackerfx)
	{
		case 1:
			if (r->inst == 255) r->inst = 0;
			r->inst <<= 4; r->inst += value;
			if (r->inst == 255) r->inst = 254;
			break;
		case 3: r->macrov[0] <<= 4; r->macrov[0] += value; break;
		case 5: r->macrov[1] <<= 4; r->macrov[1] += value; break;
	}
}
uint8_t changeNoteOctave(uint8_t octave, uint8_t note)
{
	w->octave = octave;

	if (!note) return 0;

	octave = octave * 12;
	while (note > octave + 12)
		note -= 12;
	while (note < octave + 1)
		note += 12;
	return note;
}

void trackerAdjustUp(void) /* mouse adjust only */
{
	row *r = &s->patternv[s->patterni[s->songi[w->songfx]]]->rowv[w->channel][w->trackerfy];
	switch (w->trackerfx)
	{
		case 0: if (r->note == 120) r->note = 255; else r->note++; break;
		case 1: if (w->fieldpointer) { if (r->inst < 255 - 16) r->inst+=16; else r->inst = 255; } else if (r->inst < 255) r->inst++; break;
		case 3: if (w->fieldpointer) { if (r->macrov[0] < 255 - 16) r->macrov[0]+=16; else r->macrov[0] = 255; } else if (r->macrov[0] < 255) r->macrov[0]++; break;
		case 5: if (w->fieldpointer) { if (r->macrov[1] < 255 - 16) r->macrov[1]+=16; else r->macrov[1] = 255; } else if (r->macrov[1] < 255) r->macrov[1]++; break;
	}
}
void trackerAdjustDown(void) /* mouse adjust only */
{
	row *r = &s->patternv[s->patterni[s->songi[w->songfx]]]->rowv[w->channel][w->trackerfy];
	switch (w->trackerfx)
	{
		case 0: if (r->note == 255) r->note = 120; else r->note--; break;
		case 1: if (w->fieldpointer) { if (r->inst > 16) r->inst-=16; else r->inst = 0; } else if (r->inst) r->inst--; break;
		case 3: if (w->fieldpointer) { if (r->macrov[0] > 16) r->macrov[0]-=16; else r->macrov[0] = 0; } else if (r->macrov[0]) r->macrov[0]--; break;
		case 5: if (w->fieldpointer) { if (r->macrov[1] > 16) r->macrov[1]-=16; else r->macrov[1] = 0; } else if (r->macrov[1]) r->macrov[1]--; break;
	}
}

int trackerInput(int input)
{
	switch (input)
	{
		case 12: /* ^L, toggle current pattern loop */
			s->songf[w->songfx] = !s->songf[w->songfx];
			redraw();
			break;
		case '\033': /* escape */
			previewNote(0, 255, w->channel, 1);
			switch (getchar())
			{
				case 'O':
					switch (getchar())
					{
						case 'P':
							w->mode = T_MODE_NORMAL;
							w->popup = 1;
							w->instrumentindex = MIN_INSTRUMENT_INDEX;
							break;
					}
					redraw();
					break;
				case '[':
					switch (getchar())
					{
						case 'A': /* up arrow */
							if (w->mode == T_MODE_ARROWVISUAL) w->mode = w->oldmode;
							if (w->mode == T_MODE_SONG || s->songi[w->songfx] == 255) break;
							w->trackerfy--;
							if (w->trackerfy < 0)
							{
								if (w->songfx > 0 && s->songi[w->songfx - 1] != 255)
								{
									w->songfx--;
									w->trackerfy = s->patternv[s->patterni[s->songi[w->songfx]]]->rowc;
								} else w->trackerfy = 0;
							}
							redraw();
							break;
						case 'B': /* down arrow */
							if (w->mode == T_MODE_ARROWVISUAL) w->mode = w->oldmode;
							if (w->mode == T_MODE_SONG) { w->mode = T_MODE_NORMAL; redraw(); break; }
							if (s->songi[w->songfx] == 255) break;
							w->trackerfy++;
							if (w->trackerfy > s->patternv[s->patterni[s->songi[w->songfx]]]->rowc)
							{
								if (w->songfx < 255 && s->songi[w->songfx + 1] != 255)
								{
									w->trackerfy = 0;
									w->songfx++;
								} else w->trackerfy = s->patternv[s->patterni[s->songi[w->songfx]]]->rowc;
							}
							redraw();
							break;
						case 'D': /* left arrow */
							if (w->mode == T_MODE_ARROWVISUAL) w->mode = w->oldmode;
							switch (w->mode)
							{
								case T_MODE_NORMAL:
								case T_MODE_VISUAL:
								case T_MODE_INSERT:
								case T_MODE_ARROWVISUAL:
								case T_MODE_VISUALLINE:
									if (s->songi[w->songfx] == 255) break;
									w->trackerfx--;
									if (w->trackerfx < 0)
									{
										if (w->channel > 0)
										{
											w->channel--;
											w->trackerfx = ROW_FIELDS - 1;
										} else w->trackerfx = 0;
									} break;
								case T_MODE_SONG:
									w->trackerfy = 0;
									if (w->songfx > 0)
										w->songfx--;
									break;
							}
							redraw();
							break;
						case 'C': /* right arrow */
							if (w->mode == T_MODE_ARROWVISUAL) w->mode = w->oldmode;
							switch (w->mode)
							{
								case T_MODE_NORMAL:
								case T_MODE_VISUAL:
								case T_MODE_INSERT:
								case T_MODE_ARROWVISUAL:
								case T_MODE_VISUALLINE:
									if (s->songi[w->songfx] == 255) break;

									w->trackerfx++;

									if (w->trackerfx > ROW_FIELDS - 1)
									{
										if (w->channel < s->channelc - 1)
										{
											w->channel++;
											w->trackerfx = 0;
										} else
											w->trackerfx = ROW_FIELDS - 1;
									} break;
								case 2:
									w->trackerfy = 0;
									w->songfx++;
									if (w->songfx > 254)
										w->songfx = 254;
									break;
							}
							redraw();
							break;
						case '1': /* mod+arrow / f5 - f8 */
							switch (getchar())
							{
								case '5': /* f5, play */
									if (w->mode != T_MODE_INSERT) /* suggest mode 0, but allow insert */
										w->mode = T_MODE_NORMAL;
									getchar();
									startPlayback();
									break;
								case '7': /* f6 (yes, f6 is '7'), stop */
									if (w->mode != T_MODE_INSERT) /* suggest mode 0, but allow insert */
										w->mode = T_MODE_NORMAL;
									getchar();
									stopPlayback();
									break;
								case ';': /* mod+arrow */
									switch (getchar())
									{
										case '5': /* ctrl+arrow */
											switch (getchar())
											{
												case 'D': /* left */
													if (w->mode != T_MODE_SONG && s->songi[w->songfx] != 255)
													{
														if (w->channel > 0)
															w->channel--;
														redraw();
													}
													break;
												case 'C': /* right */
													if (w->mode != T_MODE_SONG && s->songi[w->songfx] != 255)
													{
														if (w->channel < s->channelc - 1)
															w->channel++;
														redraw();
													}
													break;
											}
											break;
										case '2': /* shift+arrow */
											if (w->mode == T_MODE_VISUAL || w->mode == T_MODE_VISUALLINE)
											{
												w->mode = T_MODE_ARROWVISUAL;
											} else if (w->mode != T_MODE_ARROWVISUAL)
											{
												w->visualfx = tfxToVfx(w->trackerfx);
												w->visualfy = w->trackerfy;
												w->visualchannel = w->channel;
												w->oldmode = 0;
												w->mode = T_MODE_ARROWVISUAL;
											}
											switch (getchar())
											{
												case 'A': /* up arrow */
													if (s->songi[w->songfx] == 255) break;
													w->trackerfy--;
													if (w->trackerfy < 0)
													{
														if (w->songfx > 0 && s->songi[w->songfx - 1] != 255)
														{
															w->songfx--;
															w->trackerfy = s->patternv[s->patterni[s->songi[w->songfx]]]->rowc;
														} else w->trackerfy = 0;
													}
													redraw();
													break;
												case 'B': /* down arrow */
													if (s->songi[w->songfx] == 255) break;
													w->trackerfy++;
													if (w->trackerfy > s->patternv[s->patterni[s->songi[w->songfx]]]->rowc)
													{
														if (w->songfx < 255 && s->songi[w->songfx + 1] != 255)
														{
															w->trackerfy = 0;
															w->songfx++;
														} else w->trackerfy = s->patternv[s->patterni[s->songi[w->songfx]]]->rowc;
													}
													redraw();
													break;
												case 'D': /* left arrow */
													if (s->songi[w->songfx] == 255) break;
													w->trackerfx--;
													if (w->trackerfx < 0)
													{
														if (w->channel > 0)
														{
															w->channel--;
															w->trackerfx = ROW_FIELDS - 1;
														} else w->trackerfx = 0;
													}
													redraw();
													break;
												case 'C': /* right arrow */
													if (s->songi[w->songfx] == 255) break;

													w->trackerfx++;

													if (w->trackerfx > ROW_FIELDS - 1)
													{
														if (w->channel < s->channelc - 1)
														{
															w->channel++;
															w->trackerfx = 0;
														} else
															w->trackerfx = ROW_FIELDS - 1;
													}
													redraw();
													break;
											} break;
									} break;
							} break;
						case 'H': /* home */
							w->trackerfy = 0;
							redraw();
							break;
						case '4': /* end */
							getchar();
							w->trackerfy = s->patternv[s->patterni[s->songi[w->songfx]]]->rowc;
							redraw();
							break;
						case '5': /* page up */
							getchar();
							if (s->songi[w->songfx] == 255) break;
							w->trackerfy -= s->rowhighlight;
							if (w->trackerfy < 0)
							{
								if (w->songfx > 0 && s->songi[w->songfx - 1] != 255)
								{
									w->songfx--;
									w->trackerfy += s->patternv[s->patterni[s->songi[w->songfx]]]->rowc + 1;
								} else w->trackerfy = 0;
							}
							redraw();
							break;
						case '6': /* page down */
							getchar();
							if (s->songi[w->songfx] == 255) break;
							w->trackerfy += s->rowhighlight;
							if (w->trackerfy > s->patternv[s->patterni[s->songi[w->songfx]]]->rowc)
							{
								if (w->songfx < 255 && s->songi[w->songfx + 1] != 255)
								{
									w->songfx++;
									w->trackerfy -= s->patternv[s->patterni[s->songi[w->songfx]]]->rowc + 1;
								} else w->trackerfy = s->patternv[s->patterni[s->songi[w->songfx]]]->rowc;
							}
							redraw();
							break;
						case 'M': /* mouse */
							int button = getchar();
							int x = getchar() - 32;
							int y = getchar() - 32;
							switch (button)
							{
								case WHEEL_UP: case WHEEL_UP_CTRL: /* scroll up */
									if (s->songi[w->songfx] == 255) break;
									w->trackerfy -= WHEEL_SPEED;
									if (w->trackerfy < 0)
									{
										if (w->songfx > 0 && s->songi[w->songfx - 1] != 255)
										{
											w->songfx--;
											w->trackerfy += s->patternv[s->patterni[s->songi[w->songfx]]]->rowc + 1;
										} else w->trackerfy = 0;
									} break;
								case WHEEL_DOWN: case WHEEL_DOWN_CTRL: /* scroll down */
									if (s->songi[w->songfx] == 255) break;
									w->trackerfy += WHEEL_SPEED;
									if (w->trackerfy > s->patternv[s->patterni[s->songi[w->songfx]]]->rowc)
									{
										if (w->songfx < 255 && s->songi[w->songfx + 1] != 255)
										{
											w->trackerfy -= s->patternv[s->patterni[s->songi[w->songfx]]]->rowc + 1;
											w->songfx++;
										} else w->trackerfy = s->patternv[s->patterni[s->songi[w->songfx]]]->rowc;
									} break;
								case BUTTON_RELEASE: case BUTTON_RELEASE_CTRL: /* release click */
									if (s->songi[w->songfx] == 255) break;
									w->trackerfy += w->fyoffset;

									if (w->trackerfy < 0)
									{
										if (w->songfx > 0 && s->songi[w->songfx - 1] != 255)
										{
											w->songfx--;
											w->trackerfy += s->patternv[s->patterni[s->songi[w->songfx]]]->rowc + 1;
										} else w->trackerfy = 0;
									} else if (w->trackerfy > s->patternv[s->patterni[s->songi[w->songfx]]]->rowc)
									{
										if (w->songfx < 255 && s->songi[w->songfx + 1] != 255)
										{
											w->trackerfy -= s->patternv[s->patterni[s->songi[w->songfx]]]->rowc + 1;
											w->songfx++;
										} else w->trackerfy = s->patternv[s->patterni[s->songi[w->songfx]]]->rowc;
									}
									w->fyoffset = 0;
									w->fieldpointer = 0;

									if (w->mode == T_MODE_MOUSEADJUST) /* leave adjust mode */
										w->mode = w->oldmode;
									break;
								default: /* click / click+drag */
									/* adjust */
									if ((button == BUTTON1_HOLD || button == BUTTON1_HOLD_CTRL) && w->mode == T_MODE_MOUSEADJUST)
									{
										if      (y > w->mousey) trackerAdjustDown();
										else if (y < w->mousey) trackerAdjustUp();
										w->mousey = y;
									}
									/* song indices */
									else if (((button == BUTTON1 || button == BUTTON1_HOLD_CTRL
												|| button == BUTTON2 || button == BUTTON2_HOLD_CTRL
												|| button == BUTTON3 || button == BUTTON3_HOLD_CTRL)
											&& y < 4)
											|| ((button == BUTTON1_HOLD || button == BUTTON1_HOLD_CTRL) && w->mode == T_MODE_SONG))
									{
										w->mode = T_MODE_SONG; /* ensure song mode */
										uint8_t oldsongfx = w->songfx;
										if (x < w->songcelloffset) /* left */
											w->songfx = w->songoffset;
										else if (x > ws.ws_col - w->songcelloffset) /* right */
											w->songfx = MIN(255, w->songoffset + w->songvisible) - 1;
										else /* middle */
											w->songfx = (x + 1 - w->songcelloffset + w->songoffset * SONG_COLS) / SONG_COLS;

										if (oldsongfx != w->songfx) w->trackerfy = 0;

										switch (button)
										{
											case BUTTON2: case BUTTON2_CTRL:
												if (s->songi[w->songfx] != 255)
												{
													if (w->songnext == w->songfx + 1)
														w->songnext = 0;
													else
														w->songnext = w->songfx + 1;
												} break;
											case BUTTON3: case BUTTON3_CTRL:
												if (s->songi[w->songfx] != 255)
													s->songf[w->songfx] = !s->songf[w->songfx];
												break;
										}
									} else
									{
										if (y > ws.ws_row - 1) break; /* ignore clicking out of range */
										if (s->songi[w->songfx] == 255) break;

										/* channel row, for mute/solo */
										if (y < 6)
										{
											switch (button)
											{
												case BUTTON1: case BUTTON1_CTRL:
													if (w->mode != T_MODE_INSERT) /* suggest mode 0, but allow insert */
														w->mode = T_MODE_NORMAL;
													if (x > w->trackercelloffset + ROW_COLS * MIN(w->visiblechannels, s->channelc))
													{ /* right edge */
														w->channel = MIN(w->channeloffset + w->visiblechannels, s->channelc) - 1;
													} else if (x < w->trackercelloffset + LINENO_COLS - 3)
													{ /* left edge */
														w->channel = w->channeloffset;
													} else
													{ /* middle */
														w->channel = w->channeloffset + (x - w->trackercelloffset - LINENO_COLS + 2) / ROW_COLS;
													}
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
													if (w->mode == T_MODE_ARROWVISUAL)
													{
														w->mode = T_MODE_VISUAL;
													} else if (!(w->mode == T_MODE_VISUAL || w->mode == T_MODE_VISUALLINE))
													{
														w->visualfx = tfxToVfx(w->trackerfx);
														w->visualfy = w->trackerfy;
														w->visualchannel = w->channel;
														w->oldmode = w->mode;
														w->mode = T_MODE_VISUAL;
													}
													break;
											}

											short oldtrackerfx = w->trackerfx;
											uint8_t oldchannel = w->channel;

											if (x > w->trackercelloffset + ROW_COLS * MIN(w->visiblechannels, s->channelc))
											{ /* right edge */
												w->channel = MIN(w->channeloffset + w->visiblechannels, s->channelc) - 1;
												w->trackerfx = 5;
											} else if (x < w->trackercelloffset + LINENO_COLS - 3)
											{ /* left edge */
												w->channel = w->channeloffset;
												w->trackerfx = 0;
											} else
											{ /* middle */
												w->channel = w->channeloffset + (x - w->trackercelloffset - LINENO_COLS + 2) / ROW_COLS;
												unsigned char modulo = (x - w->trackercelloffset - LINENO_COLS) % ROW_COLS;
												if (modulo < 3 || modulo > 14) w->trackerfx = 0;
												else if (modulo < 6)           w->trackerfx = 1;
												else if (modulo < 8)           w->trackerfx = 2;
												else if (modulo < 10)          w->trackerfx = 3;
												else if (modulo < 12)          w->trackerfx = 4;
												else                           w->trackerfx = 5;
											}
											w->fyoffset = y - w->centre;

											/* enter adjust */
											if ((button == BUTTON1 || button == BUTTON1_CTRL)
													&& w->fyoffset == 0
													&& w->trackerfx == oldtrackerfx
													&& w->channel == oldchannel)
											{
												w->oldmode = w->mode;
												w->mode = T_MODE_MOUSEADJUST;
												w->mousey = y;

												unsigned char offset = (x - w->trackercelloffset - LINENO_COLS) % ROW_COLS;
												w->fieldpointer = 0;
												switch (w->trackerfx)
												{
													case 1: if (offset < 5)  w->fieldpointer = 1; break;
													case 3: if (offset < 9)  w->fieldpointer = 1; break;
													case 5: if (offset < 13) w->fieldpointer = 1; break;
												}
											}
										}
									}
									break;
							}
							redraw();
							break;
					} break;
				default: /* escape */
					if (w->mode == T_MODE_VISUAL || w->mode == T_MODE_ARROWVISUAL || w->mode == T_MODE_VISUALLINE)
						w->mode = w->oldmode;
					else if (w->mode)
						w->mode = T_MODE_NORMAL;

					redraw();
					break;
			} break;
		default:
			if (input == 10 || input == 13) /* RET mute */
			{
				s->channelv[w->channel].mute = !s->channelv[w->channel].mute;
				redraw();
				break;
			} else
			{
				row *r;
				switch (w->mode)
				{
					case T_MODE_VISUALLINE:
						if (input == 'v') /* normal visual */
						{
							w->mode = T_MODE_VISUAL;
							redraw();
							break;
						}
					case T_MODE_VISUAL:
					case T_MODE_ARROWVISUAL:
						switch (input)
						{
							case 'V': /* visual line */
								w->mode = T_MODE_VISUALLINE;
								redraw();
								break;
							case 1: /* ^a */
								addPartPattern(1,
										MIN(tfxToVfx(w->trackerfx), w->visualfx),
										MAX(tfxToVfx(w->trackerfx), w->visualfx),
										MIN(w->trackerfy, w->visualfy),
										MAX(w->trackerfy, w->visualfy),
										MIN(w->channel, w->visualchannel),
										MAX(w->channel, w->visualchannel));
								redraw();
								break;
							case 24: /* ^x */
								addPartPattern(-1,
										MIN(tfxToVfx(w->trackerfx), w->visualfx),
										MAX(tfxToVfx(w->trackerfx), w->visualfx),
										MIN(w->trackerfy, w->visualfy),
										MAX(w->trackerfy, w->visualfy),
										MIN(w->channel, w->visualchannel),
										MAX(w->channel, w->visualchannel));
								redraw();
								break;
							case 'v':
								w->mode = w->oldmode;
								redraw();
								break;
							case 'd': /* pattern cut */
								if (s->songi[w->songfx] == 255) break;
								yankPartPattern(
										MIN(tfxToVfx(w->trackerfx), w->visualfx),
										MAX(tfxToVfx(w->trackerfx), w->visualfx),
										MIN(w->trackerfy, w->visualfy),
										MAX(w->trackerfy, w->visualfy),
										MIN(w->channel, w->visualchannel),
										MAX(w->channel, w->visualchannel));
								delPartPattern(
										MIN(tfxToVfx(w->trackerfx), w->visualfx),
										MAX(tfxToVfx(w->trackerfx), w->visualfx),
										MIN(w->trackerfy, w->visualfy),
										MAX(w->trackerfy, w->visualfy),
										MIN(w->channel, w->visualchannel),
										MAX(w->channel, w->visualchannel));
								w->trackerfx = MIN(w->trackerfx, vfxToTfx(w->visualfx));
								w->trackerfy = MIN(w->trackerfy, w->visualfy);
								w->channel = MIN(w->channel, w->visualchannel);
								w->mode = T_MODE_NORMAL;
								redraw();
								break;
							case 'y': /* pattern copy */
								if (s->songi[w->songfx] == 255) break;
								yankPartPattern(
										MIN(tfxToVfx(w->trackerfx), w->visualfx),
										MAX(tfxToVfx(w->trackerfx), w->visualfx),
										MIN(w->trackerfy, w->visualfy),
										MAX(w->trackerfy, w->visualfy),
										MIN(w->channel, w->visualchannel),
										MAX(w->channel, w->visualchannel));
								w->trackerfx = MIN(w->trackerfx, vfxToTfx(w->visualfx));
								w->trackerfy = MIN(w->trackerfy, w->visualfy);
								w->channel = MIN(w->channel, w->visualchannel);
								w->mode = T_MODE_NORMAL;
								redraw();
								break;
							case 'h': /* left arrow */
								if (s->songi[w->songfx] == 255) break;
								w->trackerfx--;
								if (w->trackerfx < 0)
								{
									if (w->channel > 0)
									{
										w->channel--;
										w->trackerfx = ROW_FIELDS - 1;
									} else w->trackerfx = 0;
								}
								redraw();
								break;
							case 'j': /* down arrow */
								if (s->songi[w->songfx] == 255) break;
								w->trackerfy++;
								if (w->trackerfy > s->patternv[s->patterni[s->songi[w->songfx]]]->rowc)
								{
									if (w->songfx < 255 && s->songi[w->songfx + 1] != 255)
									{
										w->trackerfy = 0;
										w->songfx++;
									} else w->trackerfy = s->patternv[s->patterni[s->songi[w->songfx]]]->rowc;
								}
								redraw();
								break;
							case 'k': /* up arrow */
								if (s->songi[w->songfx] == 255) break;
								w->trackerfy--;
								if (w->trackerfy < 0)
								{
									if (w->songfx > 0 && s->songi[w->songfx - 1] != 255)
									{
										w->songfx--;
										w->trackerfy = s->patternv[s->patterni[s->songi[w->songfx]]]->rowc;
									} else w->trackerfy = 0;
								}
								redraw();
								break;
							case 'l': /* right arrow */
								if (s->songi[w->songfx] == 255) break;
								w->trackerfx++;
								if (w->trackerfx > ROW_FIELDS - 1)
								{
									if (w->channel < s->channelc - 1)
									{
										w->channel++;
										w->trackerfx = 0;
									} else
										w->trackerfx = ROW_FIELDS - 1;
								}
								redraw();
								break;
						}
						break;
					case T_MODE_NORMAL:
						if (w->chord)
							switch (w->chord)
							{
								case 'd': if (input == 'd')
									{
										if (s->songi[w->songfx] == 255) break;
										yankPartPattern(0, 3,
												w->trackerfy, w->trackerfy,
												w->channel, w->channel);
										delPartPattern(0, 3,
												w->trackerfy, w->trackerfy,
												w->channel, w->channel);
									}
									break;
								case 'y': if (input == 'y')
									{
										if (s->songi[w->songfx] == 255) break;
										yankPartPattern(0, 3,
												w->trackerfy, w->trackerfy,
												w->channel, w->channel);
									}
									break;
								case 'c':
									switch (input)
									{
										case 'a': /* add */
											w->channel++;
											addChannel(s, w->channel);
											break;
										case 'A': /* add before */
											addChannel(s, w->channel);
											break;
										case 'd': /* delete */
											delChannel(w->channel);
											if (w->channel > s->channelc - 1)
												w->channel--;
											break;
										case 'D': /* delete to end */
											if (w->channel == 0)
												w->channel++;
											for (uint8_t i = s->channelc; i > w->channel; i--)
												delChannel(i - 1);
											w->channel--;
											break;
										case 'y': /* yank */
											yankChannel(w->channel);
											break;
										case 'p': /* put */
											w->channel++;
											addChannel(s, w->channel);
											putChannel(w->channel);
											break;
									}
									break;
							}
						else
							switch (input)
							{
								case 9: /* TAB enter song mode */ w->mode = T_MODE_SONG; break;
								case 'i': /* enter record mode */ w->mode = T_MODE_INSERT; break;
								case 'v': /* enter visual mode */
									w->visualfx = tfxToVfx(w->trackerfx);
									w->visualfy = w->trackerfy;
									w->visualchannel = w->channel;
									w->oldmode = 0;
									w->mode = T_MODE_VISUAL;
									break;
								case 'V': /* enter visual line mode */
									w->visualfx = tfxToVfx(w->trackerfx);
									w->visualfy = w->trackerfy;
									w->visualchannel = w->channel;
									w->oldmode = 0;
									w->mode = T_MODE_VISUALLINE;
									break;
								case 'y': /* pattern copy */
									w->chord = 'y';
									redraw();
									goto t_afterchordunset;
								case 'd': /* pattern cut */
									w->chord = 'd';
									redraw();
									goto t_afterchordunset;
								case 'c': /* channel */
									w->chord = 'c';
									redraw();
									goto t_afterchordunset;
								case 'p': /* pattern paste */
									if (s->songi[w->songfx] == 255) break;
									if (w->mode == T_MODE_VISUAL || w->mode == T_MODE_ARROWVISUAL || w->mode == T_MODE_VISUALLINE)
									{
										w->trackerfx = MIN(w->trackerfx, vfxToTfx(w->visualfx));
										w->trackerfy = MIN(w->trackerfy, w->visualfy);
										w->channel = MIN(w->channel, w->visualchannel);
										w->mode = T_MODE_NORMAL;
									}
									if (w->mode == T_MODE_NORMAL || w->mode == T_MODE_INSERT)
									{
										putPartPattern();
										w->trackerfy = MIN(w->trackerfy + w->pbfy[1] - w->pbfy[0],
													s->patternv[s->patterni[s->songi[w->songfx]]]->rowc - 1);

										w->trackerfy++; /* go one further to line up consecutive pastes */
										if (w->trackerfy > s->patternv[s->patterni[s->songi[w->songfx]]]->rowc)
										{
											if (w->songfx < 255 && s->songi[w->songfx + 1] != 255)
											{
												w->trackerfy = 0;
												w->songfx++;
											} else w->trackerfy = s->patternv[s->patterni[s->songi[w->songfx]]]->rowc;
										}
									}
									break;
								case 's': w->mode = T_MODE_INSERT;
								case 'x':
									r = &s->patternv[s->patterni[s->songi[w->songfx]]]->rowv[w->channel][w->trackerfy];
									switch (w->trackerfx)
									{
										case 0: /* note */
											r->note = 0;
											r->inst = 255;
											w->trackerfy -= w->step;
											if (w->trackerfy < 0)
											{
												if (w->songfx > 0 && s->songi[w->songfx - 1] != 255)
												{
													w->songfx--;
													w->trackerfy = s->patternv[s->patterni[s->songi[w->songfx]]]->rowc;
												} else w->trackerfy = 0;
											}
											break;
										case 1: /* instrument */
											r->inst = 255;
											break;
										case 2: case 3: /* macro 1 */
											r->macroc[0] = 0;
											break;
										case 4: case 5: /* macro 2 */
											r->macroc[1] = 0;
											break;
									}
									break;

								case 'h': /* left arrow */
									if (s->songi[w->songfx] == 255) break;
									w->trackerfx--;
									if (w->trackerfx < 0)
									{
										if (w->channel > 0)
										{
											w->channel--;
											w->trackerfx = ROW_FIELDS - 1;
										} else w->trackerfx = 0;
									}
									break;
								case 'j': /* down arrow */
									if (s->songi[w->songfx] == 255) break;
									w->trackerfy++;
									if (w->trackerfy > s->patternv[s->patterni[s->songi[w->songfx]]]->rowc)
									{
										if (w->songfx < 255 && s->songi[w->songfx + 1] != 255)
										{
											w->trackerfy = 0;
											w->songfx++;
										} else w->trackerfy = s->patternv[s->patterni[s->songi[w->songfx]]]->rowc;
									}
									break;
								case 'k': /* up arrow */
									if (s->songi[w->songfx] == 255) break;
									w->trackerfy--;
									if (w->trackerfy < 0)
									{
										if (w->songfx > 0 && s->songi[w->songfx - 1] != 255)
										{
											w->songfx--;
											w->trackerfy = s->patternv[s->patterni[s->songi[w->songfx]]]->rowc;
										} else w->trackerfy = 0;
									}
									break;
								case 'l': /* right arrow */
									if (s->songi[w->songfx] == 255) break;

									w->trackerfx++;

									if (w->trackerfx > ROW_FIELDS - 1)
									{
										if (w->channel < s->channelc - 1)
										{
											w->channel++;
											w->trackerfx = 0;
										} else
											w->trackerfx = ROW_FIELDS - 1;
									}
									break;
								case '[': /* channel left */
									if (s->songi[w->songfx] != 255)
										if (w->channel > 0)
											w->channel--;
									break;
								case ']': /* right */
									if (s->songi[w->songfx] != 255)
										if (w->channel < s->channelc - 1)
											w->channel++;
									break;
								default: /* inc/dec */
									r = &s->patternv[s->patterni[s->songi[w->songfx]]]->rowv[w->channel][w->trackerfy];
									switch (w->trackerfx)
									{
										case 0: /* note */
											switch (input)
											{
												case 1: /* ^a */
													r->note++;
													break;
												case 24: /* ^x */
													r->note--;
													break;
											}
											break;
										case 1: /* instrument */
											switch (input)
											{
												case 1: /* ^a */
													r->inst++;
													if (r->inst == 255) r->inst = 0;
													if (w->instrumentrecv == INST_REC_LOCK_OK)
														w->instrument = r->inst;
													break;
												case 24: /* ^x */
													r->inst--;
													if (r->inst == 255) r->inst = 254;
													if (w->instrumentrecv == INST_REC_LOCK_OK)
														w->instrument = r->inst;
													break;
											}
											break;
										case 3: /* macro 1 value */
											switch (input)
											{
												case 1: /* ^a */
													r->macrov[0]++;
													break;
												case 24: /* ^x */
													r->macrov[0]--;
													break;
											}
											break;
										case 5: /* macro 2 value */
											switch (input)
											{
												case 1: /* ^a */
													r->macrov[1]++;
													break;
												case 24: /* ^x */
													r->macrov[1]--;
													break;
											}
											break;
									}
									break;
							}
						redraw();
						break;
					case 4: /* record */
						if (s->songi[w->songfx] == 255) break;
						r = &s->patternv[s->patterni[s->songi[w->songfx]]]->rowv[w->channel][w->trackerfy];
						switch (w->trackerfx)
						{
							case 0: /* note */
								switch (input)
								{
									case 1: /* ^a */
										r->note++;
										break;
									case 24: /* ^x */
										r->note--;
										break;
									case ' ': /* space */
										previewNote(255, 255, w->channel, 0);
										r->note = 255;
										w->trackerfy += w->step;
										if (w->trackerfy > s->patternv[s->patterni[s->songi[w->songfx]]]->rowc)
										{
											if (w->songfx < 255 && s->songi[w->songfx + 1] != 255)
											{
												w->trackerfy = 0;
												w->songfx++;
											} else w->trackerfy = s->patternv[s->patterni[s->songi[w->songfx]]]->rowc;
										} break;
									case 127: case 8: /* backspace */
										previewNote(0, 255, w->channel, 0);
										r->note = 0;
										r->inst = 255;
										w->trackerfy -= w->step;
										if (w->trackerfy < 0)
										{
											if (w->songfx > 0 && s->songi[w->songfx - 1] != 255)
											{
												w->songfx--;
												w->trackerfy = s->patternv[s->patterni[s->songi[w->songfx]]]->rowc;
											} else w->trackerfy = 0;
										} break;
									case '0': r->note = changeNoteOctave(0, r->note); if (r->note && w->instrumentrecv == INST_REC_LOCK_OK) r->inst = w->instrument; break;
									case '1': r->note = changeNoteOctave(1, r->note); if (r->note && w->instrumentrecv == INST_REC_LOCK_OK) r->inst = w->instrument; break;
									case '2': r->note = changeNoteOctave(2, r->note); if (r->note && w->instrumentrecv == INST_REC_LOCK_OK) r->inst = w->instrument; break;
									case '3': r->note = changeNoteOctave(3, r->note); if (r->note && w->instrumentrecv == INST_REC_LOCK_OK) r->inst = w->instrument; break;
									case '4': r->note = changeNoteOctave(4, r->note); if (r->note && w->instrumentrecv == INST_REC_LOCK_OK) r->inst = w->instrument; break;
									case '5': r->note = changeNoteOctave(5, r->note); if (r->note && w->instrumentrecv == INST_REC_LOCK_OK) r->inst = w->instrument; break;
									case '6': r->note = changeNoteOctave(6, r->note); if (r->note && w->instrumentrecv == INST_REC_LOCK_OK) r->inst = w->instrument; break;
									case '7': r->note = changeNoteOctave(7, r->note); if (r->note && w->instrumentrecv == INST_REC_LOCK_OK) r->inst = w->instrument; break;
									case '8': r->note = changeNoteOctave(8, r->note); if (r->note && w->instrumentrecv == INST_REC_LOCK_OK) r->inst = w->instrument; break;
									case '9': r->note = changeNoteOctave(9, r->note); if (r->note && w->instrumentrecv == INST_REC_LOCK_OK) r->inst = w->instrument; break;
									default:
										uint8_t note = charToNote(input, w->octave);
										if (note) /* ignore nothing */
										{
											r->note = note;
											if (w->instrumentrecv == INST_REC_LOCK_OK)
												r->inst = w->instrument;
											previewNote(note, r->inst, w->channel, 0);
										}
										w->trackerfy += w->step;
										if (w->trackerfy > s->patternv[s->patterni[s->songi[w->songfx]]]->rowc)
										{
											if (w->songfx < 255 && s->songi[w->songfx + 1] != 255)
											{
												w->trackerfy = 0;
												w->songfx++;
											} else w->trackerfy = s->patternv[s->patterni[s->songi[w->songfx]]]->rowc;
										} break;
								} break;
							case 1: /* instrument */
								switch (input)
								{
									case 1: /* ^a */
										r->inst++;
										if (r->inst == 255) r->inst = 0;
										if (w->instrumentrecv == INST_REC_LOCK_OK)
											w->instrument = r->inst;
										break;
									case 24: /* ^x */
										r->inst--;
										if (r->inst == 255) r->inst = 254;
										if (w->instrumentrecv == INST_REC_LOCK_OK)
											w->instrument = r->inst;
										break;
									case ' ':
										r->inst = w->instrument;
										break;
									case 127: case 8: /* backspace */
										r->inst = 255;
										break;
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
							case 2: /* macro 1 */
								switch (input)
								{
									case 127: case 8: /* backspace */
										r->macroc[0] = 0;
										break;
									default:
										if (!r->macroc[0])
											r->macrov[0] = 0;
										changeMacro(input, &r->macroc[0]);
										break;
								} break;
							case 3: /* macro 1 value */
								if (r->macroc[0])
									switch (input)
									{
										case 1: /* ^a */
											r->macrov[0]++;
											break;
										case 24: /* ^x */
											r->macrov[0]--;
											break;
										case 127: case 8: /* backspace */
											r->macroc[0] = 0;
											break;
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
									r->macrov[0] = 0;
									changeMacro(input, &r->macroc[0]);
								} break;
							case 4: /* macro 2 */
								switch (input)
								{
									case 127: case 8: /* backspace */
										r->macroc[1] = 0;
										break;
									default:
										if (!r->macroc[1])
											r->macrov[1] = 0;
										changeMacro(input, &r->macroc[1]);
										break;
								} break;
							case 5: /* macro 2 value */
								if (r->macroc[1])
									switch (input)
									{
										case 1: /* ^a */
											r->macrov[1]++;
											break;
										case 24: /* ^x */
											r->macrov[1]--;
											break;
										case 127: case 8: /* backspace */
											r->macroc[1] = 0;
											break;
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
									r->macrov[1] = 0;
									changeMacro(input, &r->macroc[1]);
								} break;
						}
						redraw();
						break;
					case 2: /* song list */
						switch (input)
						{
							case 1: /* ^a */
								if (s->songi[w->songfx] == 255)
								{
									s->songi[w->songfx] = 0;
									addPattern(0, 0);
									break;
								} else
								{
									prunePattern(s->songi[w->songfx], w->songfx);
									s->songi[w->songfx]++;
									addPattern(s->songi[w->songfx], 0);
								}
								break;
							case 24: /* ^x */
								if (s->songi[w->songfx] == 0)
								{
									if (w->songnext == w->songfx + 1)
										w->songnext = 0;
									s->songi[w->songfx] = 255;
									s->songf[w->songfx] = 0;
									prunePattern(0, w->songfx);
									break;
								} else
								{
									prunePattern(s->songi[w->songfx], w->songfx);
									s->songi[w->songfx]--;
									addPattern(s->songi[w->songfx], 0);
								}
								break;
							case '0':           inputSongHex(0);   break;
							case '1':           inputSongHex(1);   break;
							case '2':           inputSongHex(2);   break;
							case '3':           inputSongHex(3);   break;
							case '4':           inputSongHex(4);   break;
							case '5':           inputSongHex(5);   break;
							case '6':           inputSongHex(6);   break;
							case '7':           inputSongHex(7);   break;
							case '8':           inputSongHex(8);   break;
							case '9':           inputSongHex(9);   break;
							case 'A': case 'a': inputSongHex(10);  break;
							case 'B': case 'b': inputSongHex(11);  break;
							case 'C': case 'c': inputSongHex(12);  break;
							case 'D': case 'd': inputSongHex(13);  break;
							case 'E': case 'e': inputSongHex(14);  break;
							case 'F': case 'f': inputSongHex(15);  break;
							case 127: case 8: /* backspace */
								if (w->songnext == w->songfx + 1)
									w->songnext = 0;
								s->songi[w->songfx] = 255;
								s->songf[w->songfx] = 0;
								prunePattern(s->songi[w->songfx], w->songfx);
								break;
							case 'r': /* repeat ('l' is taken by vi-style directionals) */
								if (s->songi[w->songfx] != 255)
									s->songf[w->songfx] = !s->songf[w->songfx];
								break;
							case 'n':
								if (s->songi[w->songfx] != 255)
								{
									if (w->songnext == w->songfx + 1)
										w->songnext = 0;
									else
										w->songnext = w->songfx + 1;
								}
								break;
							case 'p':
								if (s->songi[w->songfx] != 255)
									s->songi[w->songfx] = duplicatePattern(s->songi[w->songfx]);
								break;

							case 'h': /* left arrow */
								w->trackerfy = 0;
								if (w->songfx > 0)
									w->songfx--;
								redraw();
								break;
							case 'j': /* down arrow */
								w->mode = T_MODE_NORMAL;
								redraw();
								break;
							case 'l': /* right arrow */
								w->trackerfy = 0;
								w->songfx++;
								if (w->songfx > 254)
									w->songfx = 254;
								redraw();
								break;
						}
						redraw();
						break;
				}
			}
			break;
	}
	if (w->chord)
	{
		w->chord = '\0';
		redraw();
	}
t_afterchordunset:

	return 0;
}
