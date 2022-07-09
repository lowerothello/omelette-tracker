#define T_MODE_NORMAL 0
#define T_MODE_VISUAL 1
#define T_MODE_MOUSEADJUST 2
#define T_MODE_INSERT 3
#define T_MODE_ARROWVISUAL 4
#define T_MODE_VISUALLINE 5
#define T_MODE_SONG 6
#define T_MODE_SONG_INSERT 7

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
	for (int i = 0; i < 255; i++) /* reserve 0xff */
	{
		if (w->centre - w->songfy + i > CHANNEL_ROW
				&& w->centre - w->songfy + i < ws.ws_row)
		{
			printf("\033[%d;%dH", w->centre - w->songfy + i, x);
			if (w->songnext - 1 == i) printf(" >");
			else if (s->songf[i])     printf(" v");
			else                      printf("  ");

			if (w->mode == T_MODE_SONG || w->mode == T_MODE_SONG_INSERT)
			{
				if (s->playing && s->songp == i)
					printf("\033[1m%02x\033[m", s->songi[i]);
				else if (s->songi[i] == 255)
					printf("..");
				else
					printf("%02x", s->songi[i]);
			} else
			{
				if (w->centre - w->songfy + i == w->centre && s->songi[i] == 255)
					printf("\033[7m..\033[m");
				else if (s->playing && s->songp == i && w->centre - w->songfy + i == w->centre)
					printf("\033[1;7m%02x\033[m", s->songi[i]);
				else if (w->centre - w->songfy + i == w->centre)
					printf("\033[7m%02x\033[m", s->songi[i]);
				else if (s->playing && s->songp == i)
					printf("\033[1m%02x\033[m", s->songi[i]);
				else if (s->songi[i] == 255)
					printf("..");
				else
					printf("%02x", s->songi[i]);
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
		if (w->centre - w->trackerfy + i > CHANNEL_ROW
				&& w->centre - w->trackerfy + i < ws.ws_row)
		{
			printf("\033[%d;%dH", w->centre - w->trackerfy + i, x - 1);
			printf(" %02x", i);
			if (s->playing == PLAYING_CONT && s->songp == w->songfy && s->songr == i)
				printf(" - ");
			else if (s->rowhighlight && !(i % s->rowhighlight))
				printf(" * ");
			else
				printf("   ");
		}
	if (w->centre - w->trackerfy - 1 > 4 && w->songfy > 0 && s->songi[w->songfy - 1] != 255)
	{
		c = 0;
		for (i = w->centre - w->trackerfy - 1; i > CHANNEL_ROW; i--)
		{
			if (c > s->patternv[s->patterni[s->songi[w->songfy - 1]]]->rowc) break;
			printf("\033[%d;%dH\033[2m", i, x+2);

			if (s->playing == PLAYING_CONT && s->songp == w->songfy - 1
					&& s->songr == s->patternv[s->patterni[s->songi[w->songfy - 1]]]->rowc - c)
				printf(" - ");
			else if (s->rowhighlight
					&& !((s->patternv[s->patterni[s->songi[w->songfy - 1]]]->rowc - c) % s->rowhighlight))
				printf(" * ");
			else
				printf("   ");

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

			if (s->playing == PLAYING_CONT && s->songp == w->songfy + 1
					&& s->songr == c)
				printf(" - ");
			else if (s->rowhighlight
					&& !(c % s->rowhighlight))
				printf(" * ");
			else
				printf("   ");

			printf("\033[m");
			c++;
		}
	}
}

void startVisual(uint8_t channel, int i, signed char fieldpointer)
{
	if ((w->mode == T_MODE_VISUAL
				|| w->mode == T_MODE_ARROWVISUAL
				|| w->mode == T_MODE_VISUALLINE)
			&& channel == MIN(w->visualchannel, w->channel))
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
void stopVisual(uint8_t channel, int i, signed char fieldpointer)
{
	if ((w->mode == T_MODE_VISUAL
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
				} break;
			case T_MODE_VISUALLINE:
				if (i >= MIN(w->visualfy, w->trackerfy)
						&& i <= MAX(w->visualfy, w->trackerfy)
						&& fieldpointer == 3)
				{
					printf("\033[27m");
					if (!s->channelv[channel].mute) printf("\033[22m");
				} break;
		}
	}
}
int ifVisual(uint8_t channel, int i, signed char fieldpointer)
{
	if ((w->mode == T_MODE_VISUAL
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
					if (i >= MIN(w->visualfy, w->trackerfy)
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
						if (i >= MIN(w->visualfy, w->trackerfy)
								&& i <= MAX(w->visualfy, w->trackerfy)
								&& fieldpointer >= (w->visualchannel <= w->channel ? w->visualfx : tfxToVfx(w->trackerfx)))
							return 1;
					} else if (channel == MAX(w->visualchannel, w->channel))
					{
						if (i >= MIN(w->visualfy, w->trackerfy)
								&& i <= MAX(w->visualfy, w->trackerfy)
								&& fieldpointer <= (w->visualchannel >= w->channel ? w->visualfx : tfxToVfx(w->trackerfx)))
							return 1;
					}
				} break;
			case T_MODE_VISUALLINE:
				if (i >= MIN(w->visualfy, w->trackerfy)
						&& i <= MAX(w->visualfy, w->trackerfy))
					return 1;
				break;
		}
	}
	return 0;
}


void drawChannel(uint8_t channel, unsigned short x)
{
	if (s->patterni[s->songi[w->songfy]] < 1) return; /* invalid pattern */
	if (channel >= s->channelc) return;

	char xoffset = (6 + 4*s->channelv[channel].macroc - 10) / 2;
	if (s->channelv[channel].mute)
		printf("\033[%d;%dH\033[2mCHANNEL %02x\033[m", CHANNEL_ROW, x+xoffset, channel);
	else
		printf("\033[%d;%dH\033[1mCHANNEL %02x\033[m", CHANNEL_ROW, x+xoffset, channel);

	row r; int c;
	char buffer[16], altbuffer[6];
	for (int i = 0; i <= s->patternv[s->patterni[s->songi[w->songfy]]]->rowc; i++)
		if (w->centre - w->trackerfy + i > CHANNEL_ROW && w->centre - w->trackerfy + i < ws.ws_row)
		{
			printf("\033[%d;%dH", w->centre - w->trackerfy + i, x);

			/* special attributes */
			if (s->channelv[channel].mute) printf("\033[2m");
			if ((w->mode == T_MODE_VISUAL
						|| w->mode == T_MODE_ARROWVISUAL
						|| w->mode == T_MODE_VISUALLINE)
					&& channel > MIN(w->visualchannel, w->channel)
					&& channel <= MAX(w->visualchannel, w->channel)
					&& i >= MIN(w->visualfy, w->trackerfy)
					&& i <= MAX(w->visualfy, w->trackerfy))
				printf("\033[2;7m");

			r = s->patternv[s->patterni[s->songi[w->songfy]]]->rowv[channel][i];

			startVisual(channel, i, 0);

			if (r.note)
			{
				noteToString(r.note, altbuffer);
				if (r.note == 255) snprintf(buffer, 16, "\033[31m%s\033[37m", altbuffer);
				else               snprintf(buffer, 16, "\033[32m%s\033[37m", altbuffer);
				if (ifVisual(channel, i, 0) && !s->channelv[channel].mute)
				{ printf("\033[22m%s\033[2m", buffer); }
				else printf(buffer);
			} else printf("...");
			stopVisual(channel, i, 0);

			printf(" ");

			startVisual(channel, i, 1);
			if (r.inst < 255)
			{
				snprintf(buffer, 16, "\033[33m%02x\033[37m", r.inst);
				if (ifVisual(channel, i, 1) && !s->channelv[channel].mute)
				{ printf("\033[22m%s\033[2m", buffer); }
				else printf(buffer);
			} else printf("..");
			stopVisual(channel, i, 1);

			for (int j = 0; j < s->channelv[channel].macroc; j++)
			{
				printf(" ");

				startVisual(channel, i, 2+j);
				if (r.macro[j].c)
				{
					if (ifVisual(channel, i, 2+j) && !s->channelv[channel].mute) printf("\033[22m");
					if (isdigit(r.macro[j].c)) /* different colour for instrument macros */
					{
						printf("\033[35m%c",   r.macro[j].c);
						printf("%02x\033[37m", r.macro[j].v);
					} else
					{
						printf("\033[36m%c",   r.macro[j].c);
						printf("%02x\033[37m", r.macro[j].v);
					}
					if (ifVisual(channel, i, 2+j) && !s->channelv[channel].mute) printf("\033[2m");
				} else printf("...");
				stopVisual(channel, i, 2+j);
			}

			printf("\033[m"); /* always clear all attributes */

			if (s->playing == PLAYING_CONT && s->songp == w->songfy && s->songr == i)
				printf(" - ");
			else if (s->rowhighlight && !(i % s->rowhighlight))
				printf(" * ");
			else
				printf("   ");
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
				[s->patternv[s->patterni[s->songi[w->songfy - 1]]]->rowc - c];

			noteToString(r.note, buffer);              if (r.note)       printf(buffer); else printf("...");
			printf(" ");
			snprintf(buffer, 16, "%02x", r.inst);      if (r.inst < 255) printf(buffer); else printf("..");
			for (int j = 0; j < s->channelv[channel].macroc; j++)
			{
				if (r.macro[j].c)
				{
					printf(" %c",  r.macro[j].c);
					printf("%02x", r.macro[j].v);
				} else printf(" ...");
			}

			if (s->playing == PLAYING_CONT && s->songp == w->songfy - 1
					&& s->songr == s->patternv[s->patterni[s->songi[w->songfy - 1]]]->rowc - c)
				printf(" - ");
			else if (s->rowhighlight
					&& !((s->patternv[s->patterni[s->songi[w->songfy - 1]]]->rowc - c) % s->rowhighlight))
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
			r = s->patternv[s->patterni[s->songi[w->songfy + 1]]]->rowv[channel][c];

			noteToString(r.note, buffer);              if (r.note)       printf(buffer); else printf("...");
			printf(" ");
			snprintf(buffer, 16, "%02x", r.inst);      if (r.inst < 255) printf(buffer); else printf("..");
			for (int j = 0; j < s->channelv[channel].macroc; j++)
			{
				if (r.macro[j].c)
				{
					printf(" %c",  r.macro[j].c);
					printf("%02x", r.macro[j].v);
				} else printf(" ...");
			}

			if (s->playing == PLAYING_CONT && s->songp == w->songfy + 1
					&& s->songr == c)
				printf(" - ");
			else if (s->rowhighlight
					&& !(c % s->rowhighlight))
				printf(" * ");
			else
				printf("   ");
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
		if (*maxwidth + 9 + 4*s->channelv[i+w->channeloffset].macroc > ws.ws_col - BORDER*2) break;
		*maxwidth += 9 + 4*s->channelv[i+w->channeloffset].macroc;
		w->visiblechannels++;
	}
	if (w->channeloffset)
		*prevwidth = 9 + 4*s->channelv[w->channeloffset-1].macroc;
	else
		*prevwidth = 0;
}

void drawTracker(void)
{
	unsigned short x, y;
	unsigned short maxwidth, prevwidth;

	printf("\033[%d;%dH\033[1mPATTERN\033[m \033[2mINSTRUMENT\033[m", CHANNEL_ROW-2, (ws.ws_col-17) / 2);

	unsigned short sx = 0;
	uint8_t oldvisiblechannels = w->visiblechannels;

	repeat(&maxwidth, &prevwidth);
	if (w->visiblechannels != oldvisiblechannels)
		repeat(&maxwidth, &prevwidth);

	while (w->channeloffset && prevwidth <= ws.ws_col - maxwidth - BORDER*2)
	{
		w->channeloffset--;
		uint8_t oldchannel = w->channel;
		w->channel-=2;
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
	} else
	{
		printf("\033[%d;%dH%s", w->centre, x + (maxwidth - (unsigned short)strlen(" (invalid pattern) ")) / 2, " (invalid pattern) ");
	}

	y = w->centre + w->fyoffset;

	if (w->trackerfx > 1 && w->mode == T_MODE_INSERT)
	{
		row *r = &s->patternv[s->patterni[s->songi[w->songfy]]]->rowv[w->channel][w->trackerfy];
		short macro = (w->trackerfx - 2) / 2;
		descMacro(r->macro[macro].c, r->macro[macro].v);
	}

	if (w->mode == T_MODE_VISUAL)
	{
		printf("\033[%d;0H\033[1m-- VISUAL --\033[m", ws.ws_row);
		printf("\033[0 q"); w->command.error[0] = '\0';
	} else if (w->mode == T_MODE_ARROWVISUAL)
	{
		printf("\033[%d;0H\033[1m-- ARROW VISUAL --\033[m", ws.ws_row);
		printf("\033[0 q"); w->command.error[0] = '\0';
	} else if (w->mode == T_MODE_VISUALLINE)
	{
		printf("\033[%d;0H\033[1m-- VISUAL LINE --\033[m", ws.ws_row);
		printf("\033[0 q"); w->command.error[0] = '\0';
	} else if (w->mode == T_MODE_MOUSEADJUST)
	{
		printf("\033[%d;0H\033[1m-- MOUSE ADJUST --\033[m", ws.ws_row);
		printf("\033[0 q"); w->command.error[0] = '\0';
	} else if (w->mode == T_MODE_INSERT || w->mode == T_MODE_SONG_INSERT)
	{
		printf("\033[%d;0H\033[1m-- INSERT --\033[m", ws.ws_row);
		printf("\033[3 q"); w->command.error[0] = '\0';
	} else
	{
		printf("\033[0 q");
	}

	if (w->mode == T_MODE_SONG || w->mode == T_MODE_SONG_INSERT)
		printf("\033[%d;%dH", w->centre, (ws.ws_col - maxwidth) / 2 + 4);
	else if (s->songi[w->songfy] == 255)
		printf("\033[%d;%dH", w->centre, x + (maxwidth - (unsigned short)strlen(" (invalid pattern) ")) / 2 + 2);
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
			if (r.note || r.macro[0].c || r.macro[1].c)
				return;
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

void trackerAdjustUp(void) /* mouse adjust only */
{
	row *r = &s->patternv[s->patterni[s->songi[w->songfy]]]->rowv[w->channel][w->trackerfy];
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
			}
			break;
	}
}
void trackerAdjustDown(void) /* mouse adjust only */
{
	short macro;
	row *r = &s->patternv[s->patterni[s->songi[w->songfy]]]->rowv[w->channel][w->trackerfy];
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
			}
			break;
	}
}

void upArrow(void)
{
	switch (w->mode)
	{
		case T_MODE_SONG: case T_MODE_SONG_INSERT: if (w->songfy) { w->songfy--; } break;
		default:
			if (s->songi[w->songfy] == 255) return;
			w->trackerfy--;
			if (w->trackerfy < 0)
			{
				if (w->songfy > 0 && s->songi[w->songfy - 1] != 255)
				{
					w->songfy--;
					w->trackerfy = s->patternv[s->patterni[s->songi[w->songfy]]]->rowc;
				} else w->trackerfy = 0;
			} break;
	}
}
void downArrow(void)
{
	switch (w->mode)
	{
		case T_MODE_SONG: case T_MODE_SONG_INSERT: if (w->songfy < 254) { w->songfy++; } break;
		default:
			if (s->songi[w->songfy] == 255) return;
			w->trackerfy++;
			if (w->trackerfy > s->patternv[s->patterni[s->songi[w->songfy]]]->rowc)
			{
				if (w->songfy < 255 && s->songi[w->songfy + 1] != 255)
				{
					w->trackerfy = 0;
					w->songfy++;
				} else w->trackerfy = s->patternv[s->patterni[s->songi[w->songfy]]]->rowc;
			} break;
	}
}
void shiftUp(void)
{
	if (w->mode != T_MODE_SONG && w->mode != T_MODE_SONG_INSERT)
	{
		pattern *p = s->patternv[s->patterni[s->songi[w->songfy]]];
		row hold = p->rowv[w->channel][0]; /* hold the first row */
		for (int i = 0; i < p->rowc; i++) /* signed */
			p->rowv[w->channel][i] = p->rowv[w->channel][i+1];
		p->rowv[w->channel][p->rowc] = hold;
	}
}
void shiftDown(void)
{
	if (w->mode != T_MODE_SONG && w->mode != T_MODE_SONG_INSERT)
	{
		pattern *p = s->patternv[s->patterni[s->songi[w->songfy]]];
		row hold = p->rowv[w->channel][p->rowc]; /* hold the last row */
		for (int i = p->rowc - 1; i >= 0; i--) /* signed */
			p->rowv[w->channel][i+1] = p->rowv[w->channel][i];
		p->rowv[w->channel][0] = hold;
	}
}
void leftArrow(void)
{
	if (w->mode != T_MODE_SONG && w->mode != T_MODE_SONG_INSERT)
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
			} else
			{
				if (w->channeloffset
						&& w->channel < s->channelc - 1 - w->visiblechannels / 2)
					w->channeloffset--;
			}
		}
	}
}
void channelLeft(void)
{
	if (w->mode != T_MODE_SONG && w->mode != T_MODE_SONG_INSERT)
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
			} else
			{
				if (w->channeloffset
						&& w->channel < s->channelc - 1 - w->visiblechannels / 2)
					w->channeloffset--;
			}
		}
	}
}
void rightArrow(void)
{
	if (w->mode != T_MODE_SONG && w->mode != T_MODE_SONG_INSERT)
	{
		if (s->songi[w->songfy] == 255) return;
		w->trackerfx++;
		if (w->trackerfx > 1 + s->channelv[w->channel].macroc * 2)
		{
			if (w->channel < s->channelc - 1)
			{
				w->channel++;
				w->trackerfx = 0;
			} else
				w->trackerfx = 1 + s->channelv[w->channel].macroc * 2;
			if (w->channeloffset + w->visiblechannels < s->channelc
					&& w->channel > w->visiblechannels / 2)
				w->channeloffset++;
		}
	}
}
void channelRight(void)
{
	if (w->mode != T_MODE_SONG && w->mode != T_MODE_SONG_INSERT)
	{
		if (s->songi[w->songfy] != 255)
		{
			if (w->channel < s->channelc - 1)
			{
				w->channel++;
				if (w->trackerfx > 1 + s->channelv[w->channel].macroc * 2)
					w->trackerfx = 1 + s->channelv[w->channel].macroc * 2;
			}
			if (w->channeloffset + w->visiblechannels < s->channelc
					&& w->channel > w->visiblechannels / 2)
				w->channeloffset++;
		}
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
	int button, x, y;
	short macro;
	row *r;
	uint8_t note;
	switch (input)
	{
		case '\033': /* escape */
			switch (getchar())
			{
				case 'O':
					handleFKeys(getchar());
					redraw();
					break;
				case '[':
					switch (getchar())
					{
						case 'A': /* up arrow */
							if (w->mode == T_MODE_ARROWVISUAL) w->mode = w->oldmode;
							upArrow(); redraw(); break;
						case 'B': /* down arrow */
							if (w->mode == T_MODE_ARROWVISUAL) w->mode = w->oldmode;
							downArrow(); redraw(); break;
						case 'D': /* left arrow */
							if (w->mode == T_MODE_ARROWVISUAL) w->mode = w->oldmode;
							leftArrow(); redraw(); break;
						case 'C': /* right arrow */
							if (w->mode == T_MODE_ARROWVISUAL) w->mode = w->oldmode;
							rightArrow(); redraw(); break;
						case '1': /* mod+arrow / f5 - f8 */
							switch (getchar())
							{
								case '5': /* f5, play */
									if (w->mode != T_MODE_INSERT) /* suggest mode 0, but allow insert */
										w->mode = T_MODE_NORMAL;
									getchar();
									startPlayback();
									break;
								case '7': /* f6, stop */
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
												case 'D': /* left  */ if (w->mode == T_MODE_ARROWVISUAL) w->mode = w->oldmode; channelLeft(); redraw(); break;
												case 'C': /* right */ if (w->mode == T_MODE_ARROWVISUAL) w->mode = w->oldmode; channelRight(); redraw(); break;
												case 'A': /* up    */ shiftUp(); redraw(); break;
												case 'B': /* down  */ shiftDown(); redraw(); break;
											} break;
										case '6': /* ctrl+shift+arrow */
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
												case 'D': /* left  */ channelLeft(); redraw(); break;
												case 'C': /* right */ channelRight(); redraw(); break;
											} break;
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
												case 'A': /* up    */ upArrow(); redraw(); break;
												case 'B': /* down  */ downArrow(); redraw(); break;
												case 'D': /* left  */ leftArrow(); redraw(); break;
												case 'C': /* right */ rightArrow(); redraw(); break;
											} break;
										default:
											getchar();
											break;
									} break;
							} break;
						case 'H': /* home */
							if (w->mode != T_MODE_SONG && w->mode != T_MODE_SONG_INSERT)
								w->trackerfy = 0;
							redraw();
							break;
						case '4': /* end */
							if (getchar() == '~')
							{
								if (w->mode != T_MODE_SONG && w->mode != T_MODE_SONG_INSERT)
									w->trackerfy = s->patternv[s->patterni[s->songi[w->songfy]]]->rowc;
								redraw();
							}
							break;
						case '5': /* page up */
							getchar();
							if (w->mode != T_MODE_SONG && w->mode != T_MODE_SONG_INSERT)
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
							}
							redraw();
							break;
						case '6': /* page down */
							getchar();
							if (w->mode != T_MODE_SONG && w->mode != T_MODE_SONG_INSERT)
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
							}
							redraw();
							break;
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
										MAX(0,
											MIN(s->channelc - 1 - w->visiblechannels / 2,
												w->channel - w->visiblechannels / 2));

									w->fyoffset = 0;
									w->fieldpointer = 0;
									if (w->mode == T_MODE_MOUSEADJUST) /* leave adjust mode */
										w->mode = w->oldmode;
									break;
								case BUTTON1_HOLD: case BUTTON1_HOLD_CTRL:
									if (w->mode == T_MODE_MOUSEADJUST)
									{
										if      (y > w->mousey) trackerAdjustDown();
										else if (y < w->mousey) trackerAdjustUp();
										w->mousey = y;
									}
									break;
								default: /* click / click+drag */
									if (y == CHANNEL_ROW-2)
									{
										if (x < (ws.ws_col-17) / 2 + 7) handleFKeys('P');
										else                            handleFKeys('Q');
										break;
									}
									if (y > ws.ws_row - 1 || s->songi[w->songfy] == 255) break; /* ignore clicking out of range */

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
									for (int i = 0; i < maxchannels; i++)
									{
										if (x < dx + 9 + 4*s->channelv[i+w->channeloffset].macroc)
										{
											w->channel = i+w->channeloffset;
											goto dxandwchannelset;
										}
										dx += 9 + 4*s->channelv[i+w->channeloffset].macroc;
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
										if (x < dx + 3) w->trackerfx = 0;
										else if (x < dx + 6) w->trackerfx = 1;
										else w->trackerfx = (x - dx - 6) / 2 + 2;

										if (w->trackerfx > (s->channelv[w->channel].macroc + 1) * 2 - 1)
											w->trackerfx = (s->channelv[w->channel].macroc + 1) * 2 - 1;

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
									} break;
							}
							redraw();
							break;
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
					switch (w->mode)
					{
						case T_MODE_VISUAL: case T_MODE_ARROWVISUAL: case T_MODE_VISUALLINE:
							w->mode = w->oldmode; break;
						case T_MODE_SONG_INSERT:
							w->mode = T_MODE_SONG;
							break;
						case T_MODE_SONG:
							break;
						default:
							w->mode = T_MODE_NORMAL; break;
					}
					redraw();
					break;
			} break;
		default:
			if (input == '\n' || input == '\r') /* RET mute */
			{
				s->channelv[w->channel].mute = !s->channelv[w->channel].mute;
				redraw();
				break;
			} else if (input == '=') /* panic note off preview */
				previewNote(255, 255, w->channel);
			else
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
								if (s->songi[w->songfy] == 255) break;
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
								if (s->songi[w->songfy] == 255) break;
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
								case '[': /* channel left  */ channelLeft(); break;
								case ']': /* channel right */ channelRight(); break;
						} break;
					case T_MODE_SONG:
						switch (input)
						{
							case '\t': /* leave song mode   */ w->mode = T_MODE_NORMAL; break;
							case 'i': /* enter insert mode  */ w->mode = T_MODE_SONG_INSERT; break;
							case 1: /* ^a */
								if (s->songi[w->songfy] == 255)
								{
									s->songi[w->songfy] = 0;
									addPattern(0, 0);
								} else
								{
									prunePattern(s->songi[w->songfy], w->songfy);
									s->songi[w->songfy]++;
									addPattern(s->songi[w->songfy], 0);
								}
								redraw();
								break;
							case 24: /* ^x */
								if (s->songi[w->songfy] == 0)
								{
									if (w->songnext == w->songfy + 1)
										w->songnext = 0;
									s->songi[w->songfy] = 255;
									s->songf[w->songfy] = 0;
									prunePattern(0, w->songfy);
								} else
								{
									prunePattern(s->songi[w->songfy], w->songfy);
									s->songi[w->songfy]--;
									addPattern(s->songi[w->songfy], 0);
								}
								redraw();
								break;
							case 'l': /* loop */
								if (s->songi[w->songfy] != 255)
									s->songf[w->songfy] = !s->songf[w->songfy];
								DEBUG=94;
								redraw();
								break;
							case 'n':
								if (s->songi[w->songfy] != 255)
								{
									if (w->songnext == w->songfy + 1) w->songnext = 0;
									else                              w->songnext = w->songfy + 1;
								}
								redraw();
								break;
							case 'p':
								if (s->songi[w->songfy] != 255)
									s->songi[w->songfy] = duplicatePattern(s->songi[w->songfy]);
								redraw();
								break;
							case 's':
								w->mode = T_MODE_SONG_INSERT;
							case 127: case 8: case 'x': /* backspace */
								if (w->songnext == w->songfy + 1)
									w->songnext = 0;
								s->songi[w->songfy] = 255;
								s->songf[w->songfy] = 0;
								prunePattern(s->songi[w->songfy], w->songfy);
								redraw();
								break;
						}
						break;
					case T_MODE_NORMAL:
						if (w->chord)
							switch (w->chord)
							{
								case 'd': if (input == 'd') /* delete */
									{
										if (s->songi[w->songfy] == 255) break;
										yankPartPattern(0, 1+s->channelv[w->channel].macroc,
												w->trackerfy, w->trackerfy,
												w->channel, w->channel);
										delPartPattern(0, 1+s->channelv[w->channel].macroc,
												w->trackerfy, w->trackerfy,
												w->channel, w->channel);
										downArrow();
									} break;
								case 'y': if (input == 'y') /* yank */
									{
										if (s->songi[w->songfy] == 255) break;
										yankPartPattern(0, 1+s->channelv[w->channel].macroc,
												w->trackerfy, w->trackerfy,
												w->channel, w->channel);
										downArrow();
									} break;
								case 'c': /* channel */
									switch (input)
									{
										case 'a': /* add */
											if (s->channelc >= MAX_CHANNELS-1) break;
											addChannel(s, w->channel+1);
											w->channel++;
											if (w->channeloffset + w->visiblechannels < s->channelc
													&& w->channel > w->visiblechannels / 2)
												w->channeloffset++;
											resize(0);
											break;
										case 'A': /* add before */
											if (s->channelc >= MAX_CHANNELS-1) break;
											addChannel(s, w->channel);
											resize(0);
											break;
										case 'd': /* delete */
											delChannel(w->channel);
											if (w->channel > s->channelc - 1)
												w->channel--;
											resize(0);
											break;
										case 'D': /* delete to end */
											if (w->channel == 0)
												w->channel++;
											for (uint8_t i = s->channelc; i > w->channel; i--)
												delChannel(i - 1);
											w->channel--;
											resize(0);
											break;
										case 'y': /* yank */
											yankChannel(w->channel);
											break;
										case 'p': /* put */
											w->channel++;
											addChannel(s, w->channel);
											putChannel(w->channel);
											resize(0);
											break;
										case 'P': /* put before */
											addChannel(s, w->channel);
											putChannel(w->channel);
											resize(0);
											break;
									} break;
								case 'm': /* macro */
									switch (input)
									{
										case 'a': /* add */
											if (s->channelv[w->channel].macroc < 8)
												s->channelv[w->channel].macroc++;
											resize(0);
											break;
										case 'd': /* delete */
											if (s->channelv[w->channel].macroc > 1)
												s->channelv[w->channel].macroc--;
											if (w->trackerfx > 1 + s->channelv[w->channel].macroc * 2)
												w->trackerfx = s->channelv[w->channel].macroc * 2;
											resize(0);
											break;
									} break;
								case 'k': /* keyboard macro */
									w->keyboardmacro = '\0';
									if (input != 'k') // kk just resets
										changeMacro(input, &w->keyboardmacro);
									redraw();
									break;
							}
						else
							switch (input)
							{
								case 'f': /* toggle song follow */ w->flags ^= 0b1; break;
								case 'i': /* enter insert mode  */ w->mode = T_MODE_INSERT; break;
								case 'v': /* enter visual mode  */
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
								case '\t': /* enter song mode */ w->mode = T_MODE_SONG; break;
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
								case 'm': /* macro */
									w->chord = 'm';
									redraw();
									goto t_afterchordunset;
								case 'k': /* keyboard macro */
									w->chord = 'k';
									redraw();
									goto t_afterchordunset;
								case 'p': /* pattern put */
									if (s->songi[w->songfy] == 255) break;
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
													s->patternv[s->patterni[s->songi[w->songfy]]]->rowc - 1);
										downArrow();
									}
									break;
								case 'P': /* pattern put before */
									if (s->songi[w->songfy] == 255) break;
									if (w->mode == T_MODE_VISUAL || w->mode == T_MODE_ARROWVISUAL || w->mode == T_MODE_VISUALLINE)
									{
										w->trackerfx = MIN(w->trackerfx, vfxToTfx(w->visualfx));
										w->trackerfy = MIN(w->trackerfy, w->visualfy);
										w->channel = MIN(w->channel, w->visualchannel);
										w->mode = T_MODE_NORMAL;
									}
									if (w->mode == T_MODE_NORMAL || w->mode == T_MODE_INSERT)
										putPartPattern();
									break;
								case 's': w->mode = T_MODE_INSERT;
								case 'x': case 127: case 8: /* backspace */
									yankPartPattern(tfxToVfx(w->trackerfx), tfxToVfx(w->trackerfx),
											w->trackerfy, w->trackerfy,
											w->channel, w->channel);
									delPartPattern(tfxToVfx(w->trackerfx), tfxToVfx(w->trackerfx),
											w->trackerfy, w->trackerfy,
											w->channel, w->channel);
									break;
								case '[': /* channel left  */ channelLeft(); break;
								case ']': /* channel right */ channelRight(); break;
								default: /* column specific */
									r = &s->patternv[s->patterni[s->songi[w->songfy]]]->rowv[w->channel][w->trackerfy];
									switch (w->trackerfx)
									{
										case 0: /* note */
											switch (input)
											{
												case 1:  /* ^a */ r->note++; break;
												case 24: /* ^x */ r->note--; break;
											} break;
										case 1: /* instrument */
											switch (input)
											{
												case 1:  /* ^a */
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
											} break;
										default:
											macro = (w->trackerfx - 2) / 2;
											switch (input)
											{
												case 1:  /* ^a */ r->macro[macro].v++; break;
												case 24: /* ^x */ r->macro[macro].v--; break;
												case '~': /* toggle case */
													if      (isupper(r->macro[macro].c)) r->macro[macro].c += 32;
													else if (islower(r->macro[macro].c)) r->macro[macro].c -= 32;
											} break;
									} break;
							} redraw(); break;
					case T_MODE_SONG_INSERT:
						switch (input)
						{
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
								if (w->songnext == w->songfy + 1)
									w->songnext = 0;
								s->songi[w->songfy] = 255;
								s->songf[w->songfy] = 0;
								prunePattern(s->songi[w->songfy], w->songfy);
								redraw();
								break;
						}
						break;
					case T_MODE_INSERT:
						if (s->songi[w->songfy] == 255) break;
						r = &s->patternv[s->patterni[s->songi[w->songfy]]]->rowv[w->channel][w->trackerfy];
						switch (w->trackerfx)
						{
							case 0: /* note */
								switch (input)
								{
									case 1:  /* ^a */ r->note++; break;
									case 24: /* ^x */ r->note--; break;
									case ' ': /* space */
										previewNote(255, 255, w->channel);
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
										previewNote(255, 255, w->channel);
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

											if (note == 255)
												r->note = 255;
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
											case 1:  /* ^a */ r->macro[macro].v++; break;
											case 24: /* ^x */ r->macro[macro].v--; break;
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
						}
						redraw();
						break;
				}
			break;
	}
	if (w->chord)
	{
		w->chord = '\0';
		redraw();
	}
t_afterchordunset:
}
