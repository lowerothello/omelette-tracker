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
		case ';': printf("\033[%d;%ldH%s", ws.ws_row, (ws.ws_col - strlen("MIDI CC TARGET")) / 2, "MIDI CC TARGET"); break;
		case '@': printf("\033[%d;%ldH%s", ws.ws_row, (ws.ws_col - strlen("MIDI PC")) / 2, "MIDI PC"); break;
		case '.': printf("\033[%d;%ldH%s", ws.ws_row, (ws.ws_col - strlen("MIDI CC")) / 2, "MIDI CC"); break;
		case ',': printf("\033[%d;%ldH%s", ws.ws_row, (ws.ws_col - strlen("SMOOTH MIDI CC")) / 2, "SMOOTH MIDI CC"); break;
		case '%': printf("\033[%d;%ldH%s", ws.ws_row, (ws.ws_col - strlen("CHANCE ROW")) / 2, "CHANCE ROW"); break;
		case 'B': printf("\033[%d;%ldH%s", ws.ws_row, (ws.ws_col - strlen("BPM")) / 2, "BPM"); break;
		case 'C': printf("\033[%d;%ldH%s", ws.ws_row, (ws.ws_col - strlen("NOTE CUT")) / 2, "NOTE CUT"); break;
		case 'D': printf("\033[%d;%ldH%s", ws.ws_row, (ws.ws_col - strlen("NOTE DELAY")) / 2, "NOTE DELAY"); break;
		case 'E': printf("\033[%d;%ldH%s", ws.ws_row, (ws.ws_col - strlen("LOCAL ENVELOPE TIME")) / 2, "LOCAL ENVELOPE TIME"); break;
		case 'F': printf("\033[%d;%ldH%s", ws.ws_row, (ws.ws_col - strlen("FILTER CUTOFF")) / 2, "FILTER CUTOFF"); break;
		case 'f': printf("\033[%d;%ldH%s", ws.ws_row, (ws.ws_col - strlen("SMOOTH FILTER CUTOFF")) / 2, "SMOOTH FILTER CUTOFF"); break;
		case 'G': printf("\033[%d;%ldH%s", ws.ws_row, (ws.ws_col - strlen("STEREO GAIN")) / 2, "STEREO GAIN"); break;
		case 'g': printf("\033[%d;%ldH%s", ws.ws_row, (ws.ws_col - strlen("SMOOTH STEREO GAIN")) / 2, "SMOOTH STEREO GAIN"); break;
		case 'H': printf("\033[%d;%ldH%s", ws.ws_row, (ws.ws_col - strlen("LOCAL PITCH SHIFT")) / 2, "LOCAL PITCH SHIFT"); break;
		case 'I': printf("\033[%d;%ldH%s", ws.ws_row, (ws.ws_col - strlen("CHANCE GAIN")) / 2, "CHANCE GAIN"); break;
		case 'i': printf("\033[%d;%ldH%s", ws.ws_row, (ws.ws_col - strlen("SMOOTH CHANCE GAIN")) / 2, "SMOOTH CHANCE GAIN"); break;
		case 'K': printf("\033[%d;%ldH%s", ws.ws_row, (ws.ws_col - strlen("COMPRESSOR COEFFICIENTS")) / 2, "COMPRESSOR COEFFICIENTS"); break;
		case 'k': printf("\033[%d;%ldH%s", ws.ws_row, (ws.ws_col - strlen("SMOOTH COMPRESSOR COEF.")) / 2, "SMOOTH COMPRESSOR COEF."); break;
		case 'L': printf("\033[%d;%ldH%s", ws.ws_row, (ws.ws_col - strlen("LOCAL CYCLELENGTH HIGH BYTE")) / 2, "LOCAL CYCLELENGTH HIGH BYTE"); break;
		case 'l': printf("\033[%d;%ldH%s", ws.ws_row, (ws.ws_col - strlen("LOCAL CYCLELENGTH LOW BYTE")) / 2, "LOCAL CYCLELENGTH LOW BYTE"); break;
		case 'M': printf("\033[%d;%ldH%s", ws.ws_row, (ws.ws_col - strlen("MICROTONAL OFFSET")) / 2, "MICROTONAL OFFSET"); break;
		case 'O': printf("\033[%d;%ldH%s", ws.ws_row, (ws.ws_col - strlen("NOTE OFFSET")) / 2, "NOTE OFFSET"); break;
		case 'o': printf("\033[%d;%ldH%s", ws.ws_row, (ws.ws_col - strlen("BACKWARDS NOTE OFFSET")) / 2, "BACKWARDS NOTE OFFSET"); break;
		case 'P': printf("\033[%d;%ldH%s", ws.ws_row, (ws.ws_col - strlen("PITCH SLIDE")) / 2, "PITCH SLIDE"); break;
		case 'Q': printf("\033[%d;%ldH%s", ws.ws_row, (ws.ws_col - strlen("RETRIGGER")) / 2, "RETRIGGER"); break;
		case 'q': printf("\033[%d;%ldH%s", ws.ws_row, (ws.ws_col - strlen("BACKWARDS RETRIGGER")) / 2, "BACKWARDS RETRIGGER"); break;
		case 'R': printf("\033[%d;%ldH%s", ws.ws_row, (ws.ws_col - strlen("BLOCK RETRIGGER")) / 2, "BLOCK RETRIGGER"); break;
		case 'r': printf("\033[%d;%ldH%s", ws.ws_row, (ws.ws_col - strlen("BACKWARDS BLOCK RETRIGGER")) / 2, "BACKWARDS BLOCK RETRIGGER"); break;
		case 'S': printf("\033[%d;%ldH%s", ws.ws_row, (ws.ws_col - strlen("SEND")) / 2, "SEND"); break;
		case 's': printf("\033[%d;%ldH%s", ws.ws_row, (ws.ws_col - strlen("SMOOTH SEND")) / 2, "SMOOTH SEND"); break;
		case 'U': printf("\033[%d;%ldH%s", ws.ws_row, (ws.ws_col - strlen("RANDOM NOTE OFFSET")) / 2, "RANDOM NOTE OFFSET"); break;
		case 'u': printf("\033[%d;%ldH%s", ws.ws_row, (ws.ws_col - strlen("RANDOM BACKWARDS NOTE OFFSET")) / 2, "RANDOM BACKWARDS NOTE OFFSET"); break;
		case 'V': printf("\033[%d;%ldH%s", ws.ws_row, (ws.ws_col - strlen("VIBRATO")) / 2, "VIBRATO"); break;
		case 'W': /* waveshapers */
			switch (v>>4)
			{
				case 0: printf("\033[%d;%ldH%s", ws.ws_row, (ws.ws_col - strlen("HARD CLIP")) / 2, "HARD CLIP"); break;
				case 1: printf("\033[%d;%ldH%s", ws.ws_row, (ws.ws_col - strlen("SOFT CLIP")) / 2, "SOFT CLIP"); break;
				case 2: printf("\033[%d;%ldH%s", ws.ws_row, (ws.ws_col - strlen("RECTIFY")) / 2, "RECTIFY"); break;
				case 3: printf("\033[%d;%ldH%s", ws.ws_row, (ws.ws_col - strlen("RECTIFYx2")) / 2, "RECTIFYx2"); break;
				case 4: printf("\033[%d;%ldH%s", ws.ws_row, (ws.ws_col - strlen("WAVEFOLD")) / 2, "WAVEFOLD"); break;
				case 5: printf("\033[%d;%ldH%s", ws.ws_row, (ws.ws_col - strlen("WAVEWRAP")) / 2, "WAVEWRAP"); break;
				case 6: printf("\033[%d;%ldH%s", ws.ws_row, (ws.ws_col - strlen("SIGN CONVERSION")) / 2, "SIGN CONVERSION"); break;
				case 7: printf("\033[%d;%ldH%s", ws.ws_row, (ws.ws_col - strlen("HARD GATE")) / 2, "HARD GATE"); break;
			}
			break;
		case 'w': /* smooth waveshapers */
			switch (v>>4)
			{
				case 0: printf("\033[%d;%ldH%s", ws.ws_row, (ws.ws_col - strlen("SMOOTH HARD CLIP")) / 2, "SMOOTH HARD CLIP"); break;
				case 1: printf("\033[%d;%ldH%s", ws.ws_row, (ws.ws_col - strlen("SMOOTH SOFT CLIP")) / 2, "SMOOTH SOFT CLIP"); break;
				case 2: printf("\033[%d;%ldH%s", ws.ws_row, (ws.ws_col - strlen("SMOOTH RECTIFY")) / 2, "SMOOTH RECTIFY"); break;
				case 3: printf("\033[%d;%ldH%s", ws.ws_row, (ws.ws_col - strlen("SMOOTH RECTIFYx2")) / 2, "SMOOTH RECTIFYx2"); break;
				case 4: printf("\033[%d;%ldH%s", ws.ws_row, (ws.ws_col - strlen("SMOOTH WAVEFOLD")) / 2, "SMOOTH WAVEFOLD"); break;
				case 5: printf("\033[%d;%ldH%s", ws.ws_row, (ws.ws_col - strlen("SMOOTH WAVEWRAP")) / 2, "SMOOTH WAVEWRAP"); break;
				case 6: printf("\033[%d;%ldH%s", ws.ws_row, (ws.ws_col - strlen("SMOOTH SIGN CONVERSION")) / 2, "SMOOTH SIGN CONVERSION"); break;
				case 7: printf("\033[%d;%ldH%s", ws.ws_row, (ws.ws_col - strlen("SMOOTH HARD GATE")) / 2, "SMOOTH HARD GATE"); break;
			}
			break;
		case 'Z': printf("\033[%d;%ldH%s", ws.ws_row, (ws.ws_col - strlen("FILTER RESONANCE")) / 2, "FILTER RESONANCE"); break;
		case 'z': printf("\033[%d;%ldH%s", ws.ws_row, (ws.ws_col - strlen("SMOOTH FILTER RESONANCE")) / 2, "SMOOTH FILTER RESONANCE"); break;
	}
}

void drawSongList(void)
{
	for (int i = 0; i < SONG_MAX; i++) /* reserve 0xff */
	{
		if (w->centre - w->songfy + i > CHANNEL_ROW
				&& w->centre - w->songfy + i < ws.ws_row)
		{
			printf("\033[%d;%dH", w->centre - w->songfy + i, 1);
			if (w->songnext-1 == i) printf(">");
			else if (s->songf[i])   printf("v");
			else                    printf(" ");

			switch (w->mode)
			{
				case T_MODE_SONG: case T_MODE_SONG_INSERT:
					if (s->playing && s->songp == i)      printf("\033[1m%02x\033[m", s->songi[i]);
					else if (s->songi[i] == PATTERN_VOID) printf("..");
					else                                  printf("%02x", s->songi[i]);
					break;
				case T_MODE_SONG_VISUAL:
					if (
							i >= MIN(w->visualfy, w->songfy) &&
							i <= MAX(w->visualfy, w->songfy))
					{
						if (s->playing && s->songp == i)      printf("\033[2;7m%02x\033[m", s->songi[i]);
						else if (s->songi[i] == PATTERN_VOID) printf("\033[2;7m..\033[m");
						else                                  printf("\033[2;7m%02x\033[m", s->songi[i]);
					} else
					{
						if (s->playing && s->songp == i)      printf("\033[1m%02x\033[m", s->songi[i]);
						else if (s->songi[i] == PATTERN_VOID) printf("..");
						else                                  printf("%02x", s->songi[i]);
					} break;
				default:
					if (w->songfy == i && s->songi[i] == PATTERN_VOID)      printf("\033[7m..\033[m");
					else if (s->playing && s->songp == i && w->songfy == i) printf("\033[1;7m%02x\033[m", s->songi[i]);
					else if (w->songfy == i)                                printf("\033[7m%02x\033[m", s->songi[i]);
					else if (s->playing && s->songp == i)                   printf("\033[1m%02x\033[m", s->songi[i]);
					else if (s->songi[i] == PATTERN_VOID)                   printf("..");
					else                                                    printf("%02x", s->songi[i]);
					break;
			}

			if (w->songnext-1 == i) printf(">");
			else if (s->songf[i])   printf("^");
			else                    printf(" ");
		}
	}
}

short drawPatternLineNumbers(uint8_t pattern, unsigned short x)
{
	for (int i = 0; i <= s->patternv[s->patterni[pattern]]->rowc; i++)
		if (w->centre - w->trackerfy + i > CHANNEL_ROW && w->centre - w->trackerfy + i < ws.ws_row)
			printf("\033[%d;%dH%02x", w->centre - w->trackerfy + i, x, i);
	return 2;
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
					{
						printf("\033[27m");
						if (!s->channelv[channel].flags&C_FLAG_MUTE) printf("\033[22m");
					}
				} else if (i >= MIN(w->visualfy, w->trackerfy+w->fyoffset) && i <= MAX(w->visualfy, w->trackerfy+w->fyoffset)
						&& fieldpointer == tfxToVfx(w->trackerfx))
				{
					printf("\033[27m");
					if (!s->channelv[channel].flags&C_FLAG_MUTE) printf("\033[22m");
				}
			} break;
		case T_MODE_VISUAL:
			if (channel == MAX(w->visualchannel, w->channel))
			{
				if (w->visualchannel == w->channel)
				{
					if (i >= MIN(w->visualfy, w->trackerfy+w->fyoffset) && i <= MAX(w->visualfy, w->trackerfy+w->fyoffset)
							&& fieldpointer == MAX(w->visualfx, tfxToVfx(w->trackerfx)))
					{
						printf("\033[27m");
						if (!s->channelv[channel].flags&C_FLAG_MUTE) printf("\033[22m");
					}
				} else if (i >= MIN(w->visualfy, w->trackerfy+w->fyoffset) && i <= MAX(w->visualfy, w->trackerfy+w->fyoffset)
						&& fieldpointer == (w->visualchannel >= w->channel ? w->visualfx : tfxToVfx(w->trackerfx)))
				{
					printf("\033[27m");
					if (!s->channelv[channel].flags&C_FLAG_MUTE) printf("\033[22m");
				}
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
			case T_MODE_VISUALLINE:
				if (i >= MIN(w->visualfy, w->trackerfy+w->fyoffset) && i <= MAX(w->visualfy, w->trackerfy+w->fyoffset))
					return 1;
		}
	} return 0;
}

void drawStarColumn(uint8_t pattern, unsigned short x)
{
	int i, c;
	for (i = 0; i <= s->patternv[s->patterni[pattern]]->rowc; i++)
		if (w->centre - w->trackerfy + i > CHANNEL_ROW && w->centre - w->trackerfy + i < ws.ws_row)
		{
			printf("\033[%d;%dH", w->centre - w->trackerfy + i, x);
			if (s->playing == PLAYING_CONT && s->songp == w->songfy && s->songr == i) printf(" - ");
			else if (s->rowhighlight && !(i % s->rowhighlight))                       printf(" * ");
			else                                                                      printf("   ");
		}
	if (w->centre - w->trackerfy - 1 > 4 && w->songfy && s->songi[w->songfy-1] != PATTERN_VOID)
	{
		c = 0;
		for (i = w->centre - w->trackerfy - 1; i > CHANNEL_ROW; i--)
		{
			if (c > s->patternv[s->patterni[s->songi[w->songfy-1]]]->rowc) break;
			printf("\033[%d;%dH\033[2m", i, x);
			if (s->playing == PLAYING_CONT && s->songp == w->songfy-1 && s->songr == s->patternv[s->patterni[s->songi[w->songfy-1]]]->rowc - c) printf(" - ");
			else if (s->rowhighlight && !((s->patternv[s->patterni[s->songi[w->songfy-1]]]->rowc - c) % s->rowhighlight))                       printf(" * ");
			else                                                                                                                                printf("   ");
			printf("\033[m");
			c++;
		}
	}
	if (w->centre - w->trackerfy + s->patternv[s->patterni[s->songi[w->songfy]]]->rowc+1 < ws.ws_row
		&& w->songfy < 254 && s->songi[w->songfy+1] != PATTERN_VOID)
	{
		c = 0;
		for (i = w->centre - w->trackerfy + s->patternv[s->patterni[s->songi[w->songfy]]]->rowc+1; i < ws.ws_row; i++)
		{
			if (c > s->patternv[s->patterni[s->songi[w->songfy+1]]]->rowc) break;
			printf("\033[%d;%dH\033[2m", i, x);
			if (s->playing == PLAYING_CONT && s->songp == w->songfy + 1 && s->songr == c) printf(" - ");
			else if (s->rowhighlight && !(c % s->rowhighlight))                           printf(" * ");
			else                                                                          printf("   ");
			printf("\033[m");
			c++;
		}
	}
}

short drawChannel(uint8_t channel, short x)
{
	if (s->patterni[s->songi[w->songfy]] < 1) return 0; /* invalid pattern */
	if (channel >= s->channelc) return 0;

	const unsigned short llimit = SONGLIST_COLS + 2;

	row r; int c;
	char buffer[16];
	unsigned short rowcc = s->patternv[s->patterni[s->songi[w->songfy]]]->rowcc[channel]+1;

	snprintf(buffer, 11, "CHANNEL %02x", channel);
	c = x + (6 + 4*(s->channelv[channel].macroc+1) - 10)/2;
	if (c <= ws.ws_col)
	{
		if (s->channelv[channel].flags&C_FLAG_MUTE) printf("\033[2m");
		else                                        printf("\033[1m");
		if (x < llimit - 2) { if (x > llimit - 12) printf("\033[%d;%dH\033[2m%s\033[m", CHANNEL_ROW, llimit, buffer+(llimit - x - 2)); }
		else                                       printf("\033[%d;%dH\033[2m%.*s\033[m", CHANNEL_ROW, c, ws.ws_col - c, buffer);
	}

	if (x <= ws.ws_col && x + 3 + 7 + 4*(s->channelv[channel].macroc+1) -1 > llimit)
	{
		for (int i = 0; i <= s->patternv[s->patterni[s->songi[w->songfy]]]->rowc; i++)
			if (w->centre - w->trackerfy + i > CHANNEL_ROW && w->centre - w->trackerfy + i < ws.ws_row)
			{
				if (x < llimit) printf("\033[%d;%dH", w->centre - w->trackerfy + i, llimit);
				else            printf("\033[%d;%dH", w->centre - w->trackerfy + i, x);
				uint8_t polycutoff = w->trackerfy - w->trackerfy % rowcc;

				r = s->patternv[s->patterni[s->songi[w->songfy]]]->rowv[channel][i%rowcc];

				if (s->channelv[channel].flags&C_FLAG_MUTE || i < polycutoff || i > polycutoff + rowcc - 1) printf("\033[2m");
				/* start visual for channels that are part of the selection but not the first channel of it */
				if ((w->mode == T_MODE_VISUAL || w->mode == T_MODE_VISUALLINE)
						&& channel > MIN(w->visualchannel, w->channel) && channel <= MAX(w->visualchannel, w->channel)
						&& i >= MIN(w->visualfy, w->trackerfy+w->fyoffset) && i <= MAX(w->visualfy, w->trackerfy+w->fyoffset))
					printf("\033[2;7m");

				startVisual(channel, i, 0);
				if (r.note != NOTE_VOID)
				{
					noteToString(r.note, buffer);
					if (s->channelv[channel].flags&C_FLAG_MUTE)
					{
						if (x < llimit) { if (x > llimit - 3) printf("%s", buffer+(llimit - x)); }
						else                                  printf("%.*s", ws.ws_col - x, buffer);
					} else
					{
						if (ifVisual(channel, i, 0)) printf("\033[22m");
						if (r.note == NOTE_OFF) printf("\033[31m");
						else                    printf("\033[32m");
						if (x < llimit) { if (x > llimit - 3) printf("%s", buffer+(llimit - x)); }
						else                                  printf("%.*s", ws.ws_col - x, buffer);
						printf("\033[37m");
						if (ifVisual(channel, i, 0)) printf("\033[2m");
					}
				} else
				{
					if (x < llimit) { if (x > llimit - 3) printf("%s", "..."+(llimit - x)); }
					else                                  printf("%.*s", ws.ws_col - x, "...");
				} stopVisual(channel, i, 0);

				if (x+4 <= ws.ws_col)
				{
					if (x+4 -1 >= llimit) printf(" ");
					snprintf(buffer, 3, "%02x", r.inst);
					startVisual(channel, i, 1);
					if (r.inst != INST_VOID)
					{
						if (!(s->channelv[channel].flags&C_FLAG_MUTE))
						{
							if (ifVisual(channel, i, 1)) printf("\033[22m");
							if (x+4 < llimit) { if (x+4 > llimit - 2) printf("\033[33m%s", buffer+(llimit - (x+4))); }
							else                                      printf("\033[33m%.*s", ws.ws_col - (x+4), buffer);
							printf("\033[37m");
							if (ifVisual(channel, i, 1)) printf("\033[2m");
						} else
						{
							if (x+4 < llimit) { if (x+4 > llimit - 2) printf("%s", buffer+(llimit - (x+4))); }
							else                                      printf("%.*s", ws.ws_col - (x+4), buffer);
						}
					} else
					{
						if (x+4 < llimit) { if (x+4 > llimit - 2) printf("%s", ".."+(llimit - (x+4))); }
						else                                      printf("%.*s", ws.ws_col - (x+4), "..");
					} stopVisual(channel, i, 1);
				}

				for (int j = 0; j <= s->channelv[channel].macroc; j++)
				{
					if (x+7 + 4*j <= ws.ws_col)
					{
						if (x+7 + 4*j -1 >= llimit) printf(" ");
						snprintf(buffer, 3, "%02x", r.macro[j].v);
						startVisual(channel, i, 2+j);
						if (r.macro[j].c)
						{
							if (s->channelv[channel].flags&C_FLAG_MUTE)
							{
								if (x+7 + 4*j >= llimit) printf("%c", r.macro[j].c);
								if (x+7 + 4*j +1 < llimit) { if (x+7 + 4*j +1 > llimit - 2) printf("%s", buffer+(llimit - (x+7 + 4*j +1))); }
								else                                                        printf("%.*s", ws.ws_col - (x+7 + 4*j +1), buffer);
							} else
							{
								if (ifVisual(channel, i, 2+j)) printf("\033[22m");
								if (isdigit(r.macro[j].c)) /* different colour for numerical macros */ /* TODO: more colour groups */
								{
									printf("\033[35m");
									if (x+7 + 4*j >= llimit) printf("%c", r.macro[j].c);
									if (x+7 + 4*j +1 < llimit) { if (x+7 + 4*j +1 > llimit - 2) printf("%s", buffer+(llimit - (x+7 + 4*j +1))); }
									else                                                        printf("%.*s", ws.ws_col - (x+7 + 4*j +1), buffer);
									printf("\033[37m");
								} else
								{
									printf("\033[36m");
									if (x+7 + 4*j >= llimit) printf("%c", r.macro[j].c);
									if (x+7 + 4*j +1 < llimit) { if (x+7 + 4*j +1 > llimit - 2) printf("%s", buffer+(llimit - (x+7 + 4*j +1))); }
									else                                                        printf("%.*s", ws.ws_col - (x+7 + 4*j +1), buffer);
									printf("\033[37m");
								} if (ifVisual(channel, i, 2+j)) printf("\033[2m");
							}
						} else
						{
							if (x+7 + 4*j < llimit) { if (x+7 + 4*j > llimit - 3) printf("%s", "..."+(llimit - (x+7 + 4*j))); }
							else                                                  printf("%.*s", ws.ws_col - (x+7 + 4*j), "...");
						} stopVisual(channel, i, 2+j);
					}
				}

				printf("\033[m"); /* clear attributes before row highlighting */

				if (x+7 + 4*(s->channelv[channel].macroc+1) -1 <= ws.ws_col)
				{
					if (s->playing == PLAYING_CONT && s->songp == w->songfy && s->songr == i)
					{
						if (x+7 + 4*(s->channelv[channel].macroc+1) -1 < llimit) { if (x+7 + 4*(s->channelv[channel].macroc+1) -1 > llimit - 2) printf("%s", " - "+(llimit - (x+7 + 4*(s->channelv[channel].macroc+1) -1))); }
						else                                                                                                            printf("%.*s", ws.ws_col - (x+7 + 4*(s->channelv[channel].macroc+1) -1), " - ");
					} else if (s->rowhighlight && !(i % s->rowhighlight))
					{
						if (x+7 + 4*(s->channelv[channel].macroc+1) -1 < llimit) { if (x+7 + 4*(s->channelv[channel].macroc+1) -1 > llimit - 2) printf("%s", " * "+(llimit - (x+7 + 4*(s->channelv[channel].macroc+1) -1))); }
						else                                                                                                            printf("%.*s", ws.ws_col - (x+7 + 4*(s->channelv[channel].macroc+1) -1), " * ");
					} else
					{
						if (x+7 + 4*(s->channelv[channel].macroc+1) -1 < llimit) { if (x+7 + 4*(s->channelv[channel].macroc+1) -1 > llimit - 2) printf("%s", "   "+(llimit - (x+7 + 4*(s->channelv[channel].macroc+1) -1))); }
						else                                                                                                            printf("%.*s", ws.ws_col - (x+7 + 4*(s->channelv[channel].macroc+1) -1), "   ");
					}
				}
			}

		/* ghost patterns */
		printf("\033[2m");
		if (w->centre - w->trackerfy - 1 > 4 && w->songfy && s->songi[w->songfy - 1] != PATTERN_VOID)
		{
			rowcc = s->patternv[s->patterni[s->songi[w->songfy-1]]]->rowcc[channel]+1;
			c = 0;
			for (int i = w->centre - w->trackerfy - 1; i > CHANNEL_ROW; i--)
			{
				if (c > s->patternv[s->patterni[s->songi[w->songfy-1]]]->rowc) break;
				if (x < llimit) printf("\033[%d;%dH", i, llimit);
				else            printf("\033[%d;%dH", i, x);
				r = s->patternv[s->patterni[s->songi[w->songfy-1]]]->rowv[channel]
					[(s->patternv[s->patterni[s->songi[w->songfy-1]]]->rowc - c)%rowcc];

				if (r.note != NOTE_VOID)
				{
					noteToString(r.note, buffer);
					if (x < llimit) { if (x > llimit - 3) printf("%s", buffer+(llimit - x)); }
					else                                  printf("%.*s", ws.ws_col - x, buffer);
				} else
				{
					if (x < llimit) { if (x > llimit - 3) printf("%s", "..."+(llimit - x)); }
					else                                  printf("%.*s", ws.ws_col - x, "...");
				}

				if (x+4 <= ws.ws_col)
				{
					if (x+4 -1 >= llimit) printf(" ");
					snprintf(buffer, 3, "%02x", r.inst);
					if (r.inst != INST_VOID)
					{
						if (x+4 < llimit) { if (x+4 > llimit - 2) printf("%s", buffer+(llimit - (x+4))); }
						else                                      printf("%.*s", ws.ws_col - (x+4), buffer);
					} else
					{
						if (x+4 < llimit) { if (x+4 > llimit - 2) printf("%s", ".."+(llimit - (x+4))); }
						else                                      printf("%.*s", ws.ws_col - (x+4), "..");
					}
				}

				for (int j = 0; j <= s->channelv[channel].macroc; j++)
				{
					if (x+7 + 4*j <= ws.ws_col)
					{
						if (x+7 + 4*j -1 >= llimit) printf(" ");
						snprintf(buffer, 3, "%02x", r.macro[j].v);
						if (r.macro[j].c)
						{
							if (x+7 + 4*j >= llimit) printf("%c", r.macro[j].c);
							if (x+7 + 4*j +1 < llimit) { if (x+7 + 4*j +1 > llimit - 2) printf("%s", buffer+(llimit - (x+7 + 4*j +1))); }
							else                                                        printf("%.*s", ws.ws_col - (x+7 + 4*j +1), buffer);
						} else
						{
							if (x+7 + 4*j < llimit) { if (x+7 + 4*j > llimit - 3) printf("%s", "..."+(llimit - (x+7 + 4*j))); }
							else                                                  printf("%.*s", ws.ws_col - (x+7 + 4*j), "...");
						}
					}
				}

				if (x+7 + 4*(s->channelv[channel].macroc+1) -1 <= ws.ws_col)
				{
					if (s->playing == PLAYING_CONT && s->songp == w->songfy-1 && s->songr == s->patternv[s->patterni[s->songi[w->songfy-1]]]->rowc - c)
					{
						if (x+7 + 4*(s->channelv[channel].macroc+1)-1 < llimit) { if (x+7 + 4*(s->channelv[channel].macroc+1)-1 > llimit - 2) printf("%s", " - "+(llimit - (x+7 + 4*(s->channelv[channel].macroc+1)-1))); }
						else                                                                                                          printf("%.*s", ws.ws_col - (x+7 + 4*(s->channelv[channel].macroc+1) -1), " - ");
					} else if (s->rowhighlight && !((s->patternv[s->patterni[s->songi[w->songfy-1]]]->rowc - c) % s->rowhighlight))
					{
						if (x+7 + 4*(s->channelv[channel].macroc+1)-1 < llimit) { if (x+7 + 4*(s->channelv[channel].macroc+1)-1 > llimit - 2) printf("%s", " * "+(llimit - (x+7 + 4*(s->channelv[channel].macroc+1)-1))); }
						else                                                                                                          printf("%.*s", ws.ws_col - (x+7 + 4*(s->channelv[channel].macroc+1) -1), " * ");
					} else
					{
						if (x+7 + 4*(s->channelv[channel].macroc+1)-1 < llimit) { if (x+7 + 4*(s->channelv[channel].macroc+1)-1 > llimit - 2) printf("%s", "   "+(llimit - (x+7 + 4*(s->channelv[channel].macroc+1)-1))); }
						else                                                                                                          printf("%.*s", ws.ws_col - (x+7 + 4*(s->channelv[channel].macroc+1) -1), "   ");
					}
				} c++;
			}
		}
		if (w->centre - w->trackerfy + s->patternv[s->patterni[s->songi[w->songfy]]]->rowc + 1 < ws.ws_row
			&& w->songfy < 254 && s->songi[w->songfy+1] != PATTERN_VOID)
		{
			rowcc = s->patternv[s->patterni[s->songi[w->songfy+1]]]->rowcc[channel]+1;
			c = 0;
			for (int i = w->centre - w->trackerfy + s->patternv[s->patterni[s->songi[w->songfy]]]->rowc + 1; i < ws.ws_row; i++)
			{
				if (c > s->patternv[s->patterni[s->songi[w->songfy+1]]]->rowc) break;
				if (x < llimit) printf("\033[%d;%dH", i, llimit);
				else            printf("\033[%d;%dH", i, x);
				r = s->patternv[s->patterni[s->songi[w->songfy+1]]]->rowv[channel][c%rowcc];

				if (r.note != NOTE_VOID)
				{
					noteToString(r.note, buffer);
					if (x < llimit) { if (x > llimit - 3) printf("%s", buffer+(llimit - x)); }
					else                                  printf("%.*s", ws.ws_col - x, buffer);
				} else
				{
					if (x < llimit) { if (x > llimit - 3) printf("%s", "..."+(llimit - x)); }
					else                                  printf("%.*s", ws.ws_col - x, "...");
				}

				if (x+4 <= ws.ws_col)
				{
					if (x+4 -1 >= llimit) printf(" ");
					snprintf(buffer, 3, "%02x", r.inst);
					if (r.inst != INST_VOID)
					{
						if (x+4 < llimit) { if (x+4 > llimit - 2) printf("%s", buffer+(llimit - (x+4))); }
						else                                      printf("%.*s", ws.ws_col - (x+4), buffer);
					} else
					{
						if (x+4 < llimit) { if (x+4 > llimit - 2) printf("%s", ".."+(llimit - (x+4))); }
						else                                      printf("%.*s", ws.ws_col - (x+4), "..");
					}
				}

				for (int j = 0; j <= s->channelv[channel].macroc; j++)
				{
					if (x+7 + 4*j <= ws.ws_col)
					{
						if (x+7 + 4*j -1 >= llimit) printf(" ");
						snprintf(buffer, 3, "%02x", r.macro[j].v);
						if (r.macro[j].c)
						{
							if (x+7 + 4*j >= llimit) printf("%c", r.macro[j].c);
							if (x+7 + 4*j +1 < llimit) { if (x+7 + 4*j +1 > llimit - 2) printf("%s", buffer+(llimit - (x+7 + 4*j +1))); }
							else                                                        printf("%.*s", ws.ws_col - (x+7 + 4*j +1), buffer);
						} else
						{
							if (x+7 + 4*j < llimit) { if (x+7 + 4*j > llimit - 3) printf("%s", "..."+(llimit - (x+7 + 4*j))); }
							else                                                  printf("%.*s", ws.ws_col - (x+7 + 4*j), "...");
						}
					}
				}

				if (x+7 + 4*(s->channelv[channel].macroc+1) -1 <= ws.ws_col)
				{
					if (s->playing == PLAYING_CONT && s->songp == w->songfy+1 && s->songr == c)
					{
						if (x+7 + 4*(s->channelv[channel].macroc+1)-1 < llimit) { if (x+7 + 4*(s->channelv[channel].macroc+1)-1 > llimit - 2) printf("%s", " - "+(llimit - (x+7 + 4*(s->channelv[channel].macroc+1)-1))); }
						else                                                                                                          printf("%.*s", ws.ws_col - (x+7 + 4*(s->channelv[channel].macroc+1) -1), " - ");
					} else if (s->rowhighlight && !(c % s->rowhighlight))
					{
						if (x+7 + 4*(s->channelv[channel].macroc+1)-1 < llimit) { if (x+7 + 4*(s->channelv[channel].macroc+1)-1 > llimit - 2) printf("%s", " * "+(llimit - (x+7 + 4*(s->channelv[channel].macroc+1)-1))); }
						else                                                                                                          printf("%.*s", ws.ws_col - (x+7 + 4*(s->channelv[channel].macroc+1) -1), " * ");
					} else
					{
						if (x+7 + 4*(s->channelv[channel].macroc+1)-1 < llimit) { if (x+7 + 4*(s->channelv[channel].macroc+1)-1 > llimit - 2) printf("%s", "   "+(llimit - (x+7 + 4*(s->channelv[channel].macroc+1)-1))); }
						else                                                                                                          printf("%.*s", ws.ws_col - (x+7 + 4*(s->channelv[channel].macroc+1) -1), "   ");
					}
				} c++;
			}
		} printf("\033[m");
	}
	return 9 + 4*(s->channelv[channel].macroc+1);
}

void drawTracker(void)
{
	short x = 0;
	short sx = 0;
	short sfx = 0;
	short y, macro;

	printf("\033[%d;%dH\033[1mPATTERN\033[m \033[2mINSTRUMENT\033[m", CHANNEL_ROW-2, (ws.ws_col-18) / 2);

	for (int i = 0; i < s->channelc; i++)
	{
		if (i == w->channel)
		{
			sfx = x + ((9 + 4*(s->channelv[i].macroc+1) + LINENO_COLS + SONGLIST_COLS)>>1);
			break;
		} x += 9 + 4*(s->channelv[i].macroc+1);
	} sfx = MIN(0, (ws.ws_col>>1) - sfx);

	drawSongList(); x = SONGLIST_COLS;
	if (s->songi[w->songfy] != PATTERN_VOID)
	{
		x += drawPatternLineNumbers(s->songi[w->songfy], x);
		for (int i = 0; i < s->channelc; i++)
		{
			if (!i)
			{
				if (!sfx) drawStarColumn(s->songi[w->songfy], x);
				x += 3;
			}

			if (i == w->channel)
				sx = x;

			x += drawChannel(i, x + sfx);
		}
		if (sfx != 0)
		{
			printf("\033[%d;%dH^", CHANNEL_ROW, SONGLIST_COLS +2);
			for (int i = 0; i <= s->patternv[s->patterni[s->songi[w->songfy]]]->rowc; i++)
				if (w->centre - w->trackerfy + i > CHANNEL_ROW && w->centre - w->trackerfy + i < ws.ws_row)
					printf("\033[%d;%dH^", w->centre - w->trackerfy + i, SONGLIST_COLS +2);
		}
		if (x + sfx > ws.ws_col)
		{
			printf("\033[%d;%dH$", CHANNEL_ROW, ws.ws_col);
			for (int i = 0; i <= s->patternv[s->patterni[s->songi[w->songfy]]]->rowc; i++)
				if (w->centre - w->trackerfy + i > CHANNEL_ROW && w->centre - w->trackerfy + i < ws.ws_row)
					printf("\033[%d;%dH$", w->centre - w->trackerfy + i, ws.ws_col);
		}
	} else printf("\033[%d;%dH%s", w->centre, SONGLIST_COLS + (ws.ws_col - SONGLIST_COLS - 19)/2, " [INVALID PATTERN] ");

	y = w->centre + w->fyoffset;

	if (w->trackerfx > 1 && w->mode == T_MODE_INSERT)
	{
		row *r = &s->patternv[s->patterni[s->songi[w->songfy]]]->rowv[w->channel][w->trackerfy];
		short macro = (w->trackerfx - 2) / 2;
		descMacro(r->macro[macro].c, r->macro[macro].v);
	}

	switch (w->mode)
	{
		case T_MODE_VISUAL: case T_MODE_SONG_VISUAL: printf("\033[%d;0H\033[1m-- VISUAL --\033[m", ws.ws_row); w->command.error[0] = '\0'; break;
		case T_MODE_VISUALLINE:                      printf("\033[%d;0H\033[1m-- VISUAL LINE --\033[m", ws.ws_row); w->command.error[0] = '\0'; break;
		case T_MODE_VISUALREPLACE:                   printf("\033[%d;0H\033[1m-- VISUAL REPLACE --\033[m", ws.ws_row); w->command.error[0] = '\0'; break;
		case T_MODE_MOUSEADJUST:                     printf("\033[%d;0H\033[1m-- MOUSE ADJUST --\033[m", ws.ws_row); w->command.error[0] = '\0'; break;
		case T_MODE_INSERT: case T_MODE_SONG_INSERT: printf("\033[%d;0H\033[1m-- INSERT --\033[m", ws.ws_row); w->command.error[0] = '\0'; break;
	}

	if (w->mode == T_MODE_SONG || w->mode == T_MODE_SONG_INSERT || w->mode == T_MODE_SONG_VISUAL)
		printf("\033[%d;%dH", w->centre + w->fyoffset, 3);
	else if (s->songi[w->songfy] == PATTERN_VOID)
		printf("\033[%d;%dH", w->centre, x + (ws.ws_col - SONGLIST_COLS - 19)/2 + 2);
	else
	{
		switch (w->trackerfx)
		{
			case 0: printf("\033[%d;%dH", y, sx + 0 + sfx);                   break;
			case 1: printf("\033[%d;%dH", y, sx + 5 - w->fieldpointer + sfx); break;
			default: /* macro columns */
				macro = (w->trackerfx - 2) / 2;
				if (w->trackerfx % 2 == 0) printf("\033[%d;%dH", y, sx + 7 + macro*4 + sfx);
				else printf("\033[%d;%dH", y, sx + 9 + macro*4 - w->fieldpointer + sfx);
				break;
		}
	}
}
