/* will populate output, output should be at least length 4 */
void noteToString(uint8_t note, char *buffer)
{
	if (note == 0)   { snprintf(buffer, 4, "..."); return; }
	if (note == 255) { snprintf(buffer, 4, "==="); return; }
	if (note == 254) { snprintf(buffer, 4, "^^^"); return; }

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
		case 'B': case 'b': *dest = 'B'; break;
		case 'E': case 'e': *dest = 'E'; break;
		case 'M': case 'm': *dest = 'M'; break;
		case 'O': case 'o': *dest = 'O'; break;
		case 'P': case 'p': *dest = 'P'; break;
		case 'R': case 'r': *dest = 'R'; break;
		case 'S': case 's': *dest = 'S'; break;
	}
}

int drawPatternLineNumbers(uint8_t pattern, char rightside)
{
	if (s->patterni[pattern] < 1) return 1; /* invalid pattern */

	/* if (!rightside)
	{
		printf("\033[%d;%dH[%02x]", 5, w->trackercelloffset - 1, pattern);
	} */

	for (int i = 0; i <= s->patternv[s->patterni[pattern]]->rowc; i++)
	{
		if (w->centre - w->trackerfy + i > 4 && w->centre - w->trackerfy + i < ws.ws_row - 1)
		{
			if (rightside)
			{
				printf("\033[%d;%dH", w->centre - w->trackerfy + i,
					w->trackercelloffset + LINENO_COLS + ROW_COLS * w->visiblechannels);
				printf("%02x", i);
			}
			else
			{
				printf("\033[%d;%dH", w->centre - w->trackerfy + i, w->trackercelloffset);
				printf("%02x", i);
				if (s->rowhighlight > 0 && i % s->rowhighlight == 0)
					printf(" * ");
				else
					printf("   ");
			}
		}
	}
	return 0;
}
void drawSong(void)
{
	unsigned short x;
	for (unsigned short i = 0; i < w->songvisible; i++)
	{
		if (i >= 255) break; /* no indices past this point */
		x = w->songcelloffset + SONG_COLS * i;

		if (s->songa[i + w->songoffset])
			switch (s->songa[i + w->songoffset])
			{
				case 1:  printf("\033[1;%dHloop", x); break;
				default: printf("\033[1;%dH????", x); break; /* likely a corrupt file */
			}
		if (w->songnext == i + w->songoffset + 1)
			printf("\033[3;%dHnext", x);

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
		printf("\033[4;%dH \033[2m<CHANNEL %02x>\033[m", x, channel);
	else
		printf("\033[4;%dH <CHANNEL %02x>", x, channel);

	row r;
	int i, c;
	char buffer[16], altbuffer[6];

	for (i = 0; i <= s->patternv[s->patterni[s->songi[w->songfx]]]->rowc; i++)
	{
		if (w->centre - w->trackerfy + i > 4 && w->centre - w->trackerfy + i < ws.ws_row - 1)
		{
			printf("\033[%d;%dH", w->centre - w->trackerfy + i, x);

			void startVisual(signed char fieldpointer)
			{
				if (w->mode == 1 && channel == MIN(w->visualchannel, w->channel))
				{
					if (w->visualchannel == w->channel)
					{
						if (       i >= MIN(w->visualfy, w->trackerfy)
								&& i <= MAX(w->visualfy, w->trackerfy)
								&& MIN(w->visualfx, tfxToVfx(w->trackerfx)) == fieldpointer)
							printf("\033[2;7m");
					} else if (i >= MIN(w->visualfy, w->trackerfy)
							&& i <= MAX(w->visualfy, w->trackerfy)
							&& (w->visualchannel <= w->channel ? w->visualfx : tfxToVfx(w->trackerfx)) == fieldpointer)
						printf("\033[2;7m");
				}
			}
			void stopVisual(signed char fieldpointer)
			{
				if (w->mode == 1 && channel == MAX(w->visualchannel, w->channel))
				{
					if (w->visualchannel == w->channel)
					{
						if (       i >= MIN(w->visualfy, w->trackerfy)
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
				}
			}
			int ifVisual(signed char fieldpointer)
			{
				if (w->mode == 1
						&& channel >= MIN(w->visualchannel, w->channel)
						&& channel <= MAX(w->visualchannel, w->channel))
				{
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
				}
				return 0;
			}

			/* special attributes */
			if (s->channelv[channel].mute) printf("\033[2m");
			if (w->mode == 1
					&& channel > MIN(w->visualchannel, w->channel)
					&& channel <= MAX(w->visualchannel, w->channel)
					&& i >= MIN(w->visualfy, w->trackerfy)
					&& i <= MAX(w->visualfy, w->trackerfy))
				printf("\033[2;7m");

			r = s->patternv[s->patterni[s->songi[w->songfx]]]->rowv[channel][i];

			startVisual(0);
			noteToString(r.note, altbuffer);
			snprintf(buffer, 16, "\033[32m%s\033[37m", altbuffer);
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

			if (s->rowhighlight > 0 && i % s->rowhighlight == 0)
				printf(" * ");
			else
				printf("   ");
		}
	}
	if (w->centre - w->trackerfy - 1 > 4 && w->songfx > 0 && s->songi[w->songfx - 1] != 255)
	{
		c = 0;
		for (i = w->centre - w->trackerfy - 1; i > 4; i--)
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
	if (w->centre - w->trackerfy + s->patternv[s->patterni[s->songi[w->songfx]]]->rowc + 1 < ws.ws_row - 1
		&& w->songfx < 254 && s->songi[w->songfx + 1] != 255)
	{
		c = 0;
		for (i = w->centre - w->trackerfy + s->patternv[s->patterni[s->songi[w->songfx]]]->rowc + 1; i < ws.ws_row - 1; i++)
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

	if (s->playing && s->channelv[channel].r.inst < 255 && s->instrumenti[s->channelv[channel].r.inst])
	{
		instrument *iv = s->instrumentv[s->instrumenti[s->channelv[channel].r.inst]];
		if (iv->type < INSTRUMENT_TYPE_COUNT && t->f[iv->type].getOffset != NULL)
			printf("\033[%d;%dHO%02x", ws.ws_row - 1, x + 5, t->f[iv->type].getOffset(iv, &s->channelv[channel]));
		else
			printf("\033[%d;%dHO00", ws.ws_row - 1, x + 5);
	} else printf("\033[%d;%dHO00", ws.ws_row - 1, x + 5);
	
	return 0;
}

void trackerRedraw(void)
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
		drawPatternLineNumbers(s->songi[w->songfx], 0);
		// drawPatternLineNumbers(s->songi[w->songfx], 1);

		if (w->visiblechannels > 1)
		{ // try to follow the focus in a smooth way (broken with 2 channels)
			while (
				w->channel < w->channeloffset + 1
				&& w->channeloffset > 0
			) w->channeloffset--;
			while (
				w->channel > w->channeloffset + w->visiblechannels - 2
				&& w->channeloffset + w->visiblechannels < s->channelc
			) w->channeloffset++;
		} else w->channeloffset = w->channel; // if only one channel is visible then pin it

		uint8_t drawn = 0;
		for (i = 0; i <= s->channelc; i++)
		{
			if (i >= w->channeloffset && i < w->channeloffset + w->visiblechannels)
			{
				drawChannel(i, drawn);
				drawn++;
			}
		}
	} else printf("\033[%d;%dH%s", w->centre, (ws.ws_col - (unsigned short)strlen("(invalid pattern)")) / 2, "(invalid pattern)");

	y = w->centre + w->fyoffset;
	x = w->trackercelloffset + LINENO_COLS + ROW_COLS * (w->channel - w->channeloffset) + w->fieldpointer;

	/* ruler */
	printf("\033[%d;%dH[%02x] %02x,%02x  ", ws.ws_row, ws.ws_col - 43,
			w->songfx, w->trackerfy, w->channel);
	if (s->playing == PLAYING_STOP)
		printf("(stopped)    ");
	else
		printf("([%02x] %02x)    ", s->songp, s->songr);
	printf("&%d +%x   B%02x  (B%02x)", w->octave, 0, s->songbpm, s->bpm);

	switch (w->mode)
	{
		case 1: /* visual */
			printf("\033[%d;0H\033[1m-- VISUAL --\033[m", ws.ws_row);
			w->command.error[0] = '\0';
		case 0: /* pattern */
			if (s->songi[w->songfx] != 255)
				switch (w->trackerfx)
				{
					case 0: printf("\033[%d;%dH", y, x + 0);  break;
					case 1: printf("\033[%d;%dH", y, x + 4);  break;
					case 2: printf("\033[%d;%dH", y, x + 7);  break;
					case 3: printf("\033[%d;%dH", y, x + 8);  break;
					case 4: printf("\033[%d;%dH", y, x + 11); break;
					case 5: printf("\033[%d;%dH", y, x + 12); break;
				}
			else printf("\033[%d;%ldH", w->centre, (ws.ws_col - strlen("(invalid pattern)")) / 2 + 1);
			break;
		case 2: /* song */
			printf("\033[%d;0H\033[1m-- SONG --\033[m", ws.ws_row);
			w->command.error[0] = '\0';
			printf("\033[2;%dH", w->songcelloffset + (w->songfx - w->songoffset) * SONG_COLS + 1 + w->fieldpointer);
			break;
	}
}

void prunePattern(uint8_t index, short callingindex)
{
	if (index == 255) return; /* invalid index */
	// if (index == 0) return; /* protected index */
	if (s->patterni[index] == 0) return; /* pattern doesn't exist */

	short i;
	/* don't remove if pattern is still referenced */
	for (i = 0; i < 256; i++)
		if (s->songi[i] == index && i != callingindex)
			return;

	pattern *p = s->patternv[s->patterni[index]];
	/* don't remove if pattern is populated */
	for (i = 0; i < p->rowc + 1; i++)
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
	updateField(w->fieldpointer, 2, (uint32_t *)&s->songi[w->songfx], value);
	if (s->songi[w->songfx] == 255)
		s->songi[w->songfx] = 254;
	addPattern(s->songi[w->songfx], 0);
}
void inputPatternHex(row *r, char value) /* TODO: don't base off of trackerfx */
{
	switch (w->trackerfx)
	{
		case 1:
			if (r->inst == 255) r->inst = 0;
			updateField(w->fieldpointer, 2, (uint32_t *)&r->inst, value);
			if (r->inst == 255) r->inst = 254;
			break;
		case 3: updateField(w->fieldpointer, 2, (uint32_t *)&r->macrov[0], value); break;
		case 5: updateField(w->fieldpointer, 2, (uint32_t *)&r->macrov[1], value); break;
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

int changeBpmCallback(char *command, unsigned char *mode)
{
	char *buffer = malloc(strlen(command) + 1);
	wordSplit(buffer, command, 0);
	char update = 0;
	if (s->songbpm == s->bpm) update = 1;
	s->songbpm = MIN(MAX(strtol(buffer, NULL, 0), 32), 255);
	if (update) w->request = REQ_BPM; /* update if playing */
	free(buffer); buffer = NULL;
	return 0;
}
int changePatternLengthCallback(char *command, unsigned char *mode)
{
	char *buffer = malloc(strlen(command) + 1);
	wordSplit(buffer, command, 0);
	s->patternv[s->patterni[s->songi[w->songfx]]]->rowc = (uint8_t)strtol(buffer, NULL, 0);
	s->defpatternlength = s->patternv[s->patterni[s->songi[w->songfx]]]->rowc;
	if (w->trackerfy > s->patternv[s->patterni[s->songi[w->songfx]]]->rowc)
		w->trackerfy = s->patternv[s->patterni[s->songi[w->songfx]]]->rowc;
	free(buffer); buffer = NULL;
	return 0;
}
int changeRowHighlightCallback(char *command, unsigned char *mode)
{
	char *buffer = malloc(strlen(command) + 1);
	wordSplit(buffer, command, 0);
	s->rowhighlight = strtol(buffer, NULL, 0);
	free(buffer); buffer = NULL;
	return 0;
}

int trackerInput(int input)
{
	char buffer[COMMAND_LENGTH];
	switch (input)
	{
		case 2: /* ^B, change bpm */
			snprintf(buffer, COMMAND_LENGTH, "0x%x", s->songbpm);
			setCommand(&w->command, &changeBpmCallback, NULL, 0, "New bpm: ", buffer);
			w->mode = 255;
			redraw();
			break;
		case 12: /* ^L, change pattern length */
			snprintf(buffer, COMMAND_LENGTH, "0x%x",
				s->patternv[s->patterni[s->songi[w->songfx]]]->rowc);
			setCommand(&w->command, &changePatternLengthCallback, NULL, 0, "Pattern length: ", buffer);
			w->mode = 255;
			redraw();
			break;
		case 8: /* ^H, change row highlight */
			snprintf(buffer, COMMAND_LENGTH, "%d", s->rowhighlight);
			setCommand(&w->command, &changeRowHighlightCallback, NULL, 0, "Row highlight: ", buffer);
			w->mode = 255;
			redraw();
			break;
		case 15: /* ^O, toggle current pattern loop */
			s->songa[w->songfx] = !s->songa[w->songfx];
			redraw();
			break;
		case 9: /* tab */
			if (w->mode == 2) w->mode = 0;
			else              w->mode = 2;
			redraw();
			break;
		case '\033': /* escape */
			w->previewchanneltrigger = 0;
			switch (getchar())
			{
				case 'O':
					switch (getchar())
					{
						case 'P':
							w->popup = 1;
							w->mode = 0;
							w->instrumentindex = MIN_INSTRUMENT_INDEX;
							break;
						case 'Q':
							w->popup = 3;
							w->mode = 0;
							w->effectindex = MIN_EFFECT_INDEX;
							w->effectoffset = 0;
							break;
						// case 'R': w->tab = 3; break;
						// case 'S': w->tab = 4; break;
					}
					redraw();
					break;
				case '[':
					switch (getchar())
					{
						case 'A': /* up arrow */
							if (w->mode == 2 || s->songi[w->songfx] == 255) break;
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
							if (w->mode == 2) { w->mode = 0; redraw(); break; }
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
							switch (w->mode)
							{
								case 0: case 1:
									if (s->songi[w->songfx] == 255) break;

									if (w->fieldpointer) w->fieldpointer--;
									else
									{
										w->trackerfx--;
										switch (w->trackerfx)
										{
											case 0: case 2: case 4: /* 1 cell wide */
												w->fieldpointer = 0; break;
											case 1: case 3: case 5: /* 2 cells wide */
												w->fieldpointer = 1; break;
										}
									}

									if (w->trackerfx < 0)
									{
										if (w->channel > 0)
										{
											w->channel--;
											w->trackerfx = ROW_FIELDS - 1;
											w->fieldpointer = 1;
										} else w->trackerfx = 0;
									}
									break;
								case 2:
									w->trackerfy = 0;
									if (w->songfx > 0)
										w->songfx--;
									break;
							}
							redraw();
							break;
						case 'C': /* right arrow */
							switch (w->mode)
							{
								case 0: case 1:
									if (s->songi[w->songfx] == 255) break;

									switch (w->trackerfx)
									{
										case 0: case 2: case 4: /* 1 cell wide */
											w->trackerfx++;
											break;
										case 1: case 3: case 5: /* 2 cells wide */
											if (w->fieldpointer) { w->trackerfx++; w->fieldpointer = 0; }
											else                   w->fieldpointer++;
											break;
									}

									if (w->trackerfx > ROW_FIELDS - 1)
									{
										if (w->channel < s->channelc - 1)
										{
											w->channel++;
											w->trackerfx = 0;
											w->fieldpointer = 0;
										} else
										{
											w->trackerfx = ROW_FIELDS - 1;
											w->fieldpointer = 1;
										}
									}
									break;
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
									getchar(); /* extraneous tilde */
									startPlayback();
									break;
								case '7': /* f6 (yes, f6 is '7'), stop */
									getchar(); /* extraneous tilde */
									stopPlayback();
									break;
								case ';': /* mod+arrow */
									switch (getchar())
									{
										case '5': /* ctrl+arrow */
											switch (getchar())
											{
												case 'D': /* left */
													if (w->mode != 2 && s->songi[w->songfx] != 255)
													{
														if (w->channel > 0)
															w->channel--;
														redraw();
													}
													break;
												case 'C': /* right */
													if (w->mode != 2 && s->songi[w->songfx] != 255)
													{
														if (w->channel < s->channelc - 1)
															w->channel++;
														redraw();
													}
													break;
											}
											break;
									}
									break;
							}
							break;
						case 'H': /* home */
							w->trackerfx = 0;
							w->fieldpointer = 0;
							redraw();
							break;
						case '4': /* end */
							getchar(); /* burn through the tilde */
							break;
						case '5': /* page up */
							getchar(); /* burn through the tilde */
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
							getchar(); /* burn through the tilde */
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
								case 32 + 64: /* scroll up */
									if (s->songi[w->songfx] == 255) break;
									w->trackerfy -= 3;
									if (w->trackerfy < 0)
									{
										if (w->songfx > 0 && s->songi[w->songfx - 1] != 255)
										{
											w->songfx--;
											w->trackerfy += s->patternv[s->patterni[s->songi[w->songfx]]]->rowc + 1;
										} else w->trackerfy = 0;
									}
									break;
								case 33 + 64: /* scroll down */
									if (s->songi[w->songfx] == 255) break;
									w->trackerfy += 3;
									if (w->trackerfy > s->patternv[s->patterni[s->songi[w->songfx]]]->rowc)
									{
										if (w->songfx < 255 && s->songi[w->songfx + 1] != 255)
										{
											w->trackerfy -= s->patternv[s->patterni[s->songi[w->songfx]]]->rowc + 1;
											w->songfx++;
										} else w->trackerfy = s->patternv[s->patterni[s->songi[w->songfx]]]->rowc;
									}
									break;
								case 35: /* release click */
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
									break;
								default: /* click / drag */
									/* song indices */
									if ((button == BUTTON1 && y < 4) || (button == BUTTON1 + 32 && w->mode == 2))
									{
										w->mode = 2; /* force enter song mode */
										if (x < w->songcelloffset) /* left */
											w->songfx = w->songoffset;
										else if (x > ws.ws_col - w->songcelloffset) /* right */
											w->songfx = MIN(255, w->songoffset + w->songvisible) - 1;
										else /* middle */
											w->songfx = (x + 1 - w->songcelloffset + w->songoffset * SONG_COLS) / SONG_COLS;
										break;
									}

									if (y > ws.ws_row - 2 || y < 2) break; /* ignore clicking out of range */
									if (s->songi[w->songfx] == 255) break;

									if (button == BUTTON1 || button == BUTTON3)
									{
										switch (button)
										{
											case BUTTON1: w->mode = 0; break;
											case BUTTON3:
												if (w->mode != 1)
												{
													w->visualfx = tfxToVfx(w->trackerfx);
													w->visualfy = w->trackerfy;
													w->visualchannel = w->channel;
												}
												w->mode = 1;
												break;
										}

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
											if (modulo < 3 || modulo > 14) { w->trackerfx = 0; w->fieldpointer = 0; }
											else if (modulo < 5)           { w->trackerfx = 1; w->fieldpointer = 0; }
											else if (modulo < 6)           { w->trackerfx = 1; w->fieldpointer = 1; }
											else if (modulo < 8)           { w->trackerfx = 2; w->fieldpointer = 0; }
											else if (modulo < 9)           { w->trackerfx = 3; w->fieldpointer = 0; }
											else if (modulo < 10)          { w->trackerfx = 3; w->fieldpointer = 1; }
											else if (modulo < 12)          { w->trackerfx = 4; w->fieldpointer = 0; }
											else if (modulo < 13)          { w->trackerfx = 5; w->fieldpointer = 0; }
											else                           { w->trackerfx = 5; w->fieldpointer = 1; }
										}
										w->fyoffset = y - w->centre;
									}

									/* channel row, for mute and solo */
									if (y < 5)
									{
										switch (button)
										{
											case BUTTON1:
												w->mode = 0;
												w->trackerfx = 0;
												break;
											case BUTTON3:
												w->mode = 0;
												w->trackerfx = 0;
												s->channelv[w->channel].mute = !s->channelv[w->channel].mute;
												break;
										}
										break;
									}
									break;
							}
							redraw();
							break;
					}
					break;
				/* alt binds */
				case 'v': /* toggle visual mode */
					if (s->songi[w->songfx] < 255)
					{
						if (w->mode == 0)
						{
							w->visualfx = tfxToVfx(w->trackerfx);
							w->visualfy = w->trackerfy;
							w->visualchannel = w->channel;
							w->mode = 1;
						} else if (w->mode == 1)
							w->mode = 0;
					}
					redraw();
					break;
				case 'd': /* pattern cut */
					if (s->songi[w->songfx] == 255 && w->mode > 1) break;
					switch (w->mode)
					{
						case 0: /* normal */
							switch (w->trackerfx)
							{
								case 0: case 1: /* whole row */
									yankPartPattern(0, 3,
											w->trackerfy, w->trackerfy,
											w->channel, w->channel);
									delPartPattern(0, 3,
											w->trackerfy, w->trackerfy,
											w->channel, w->channel);
									break;
								case 2: case 3: /* macro[0] */
									yankPartPattern(2, 2,
											w->trackerfy, w->trackerfy,
											w->channel, w->channel);
									delPartPattern(2, 2,
											w->trackerfy, w->trackerfy,
											w->channel, w->channel);
									break;
								case 4: case 5: /* macro[1] */
									yankPartPattern(3, 3,
											w->trackerfy, w->trackerfy,
											w->channel, w->channel);
									delPartPattern(3, 3,
											w->trackerfy, w->trackerfy,
											w->channel, w->channel);
									break;
							}
							break;
						case 1: /* visual */
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
							w->mode = 0;
							break;
					}
					redraw();
					break;
				case 'y': /* pattern copy */
					if (s->songi[w->songfx] == 255 && w->mode > 1) break;
					switch (w->mode)
					{
						case 0: /* normal */
							switch (w->trackerfx)
							{
								case 0: case 1: /* whole row */
									yankPartPattern(0, 3,
											w->trackerfy, w->trackerfy,
											w->channel, w->channel);
									break;
								case 2: case 3: /* macro[0] */
									yankPartPattern(2, 2,
											w->trackerfy, w->trackerfy,
											w->channel, w->channel);
									break;
								case 4: case 5: /* macro[1] */
									yankPartPattern(3, 3,
											w->trackerfy, w->trackerfy,
											w->channel, w->channel);
									break;
							}
							break;
						case 1: /* visual */
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
							w->mode = 0;
							break;
					}
					redraw();
					break;
				case 'p': /* pattern paste */
					if (s->songi[w->songfx] == 255 && w->mode > 1) break;
					if (w->mode == 1)
					{
						w->trackerfx = MIN(w->trackerfx, vfxToTfx(w->visualfx));
						w->trackerfy = MIN(w->trackerfy, w->visualfy);
						w->channel = MIN(w->channel, w->visualchannel);
						w->mode = 0;
					}
					if (w->mode == 0)
					{
						putPartPattern();
						w->trackerfy = MIN(w->trackerfy + s->pbfy[1] - s->pbfy[0],
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
					redraw();
					break;
				default: /* escape */
					if (w->mode)
					{
						w->mode = 0;
						redraw();
					}
					break;
			}
			break;
		default:
			switch (w->mode)
			{
				case 0: /* normal */
					if (s->songi[w->songfx] == 255) break;
					row *r = &s->patternv[s->patterni[s->songi[w->songfx]]]->rowv[w->channel][w->trackerfy];
					switch (input)
					{
						case 14: /* ^N, new (^A is taken by increment) */
							w->channel++;
							addChannel(w->channel);
							break;
						// case 'A': /* add before */
							addChannel(w->channel);
							break;
						case 4: /* ^D, delete */
							delChannel(w->channel);
							if (w->channel > s->channelc - 1)
								w->channel--;
							break;
						// case 'D': /* delete to end */
							if (w->channel == 0)
								w->channel++;
							for (uint8_t i = s->channelc; i > w->channel; i--)
								delChannel(i - 1);
							w->channel--;
							break;
						case 25: /* ^Y, yank */
							yankChannel(w->channel);
							break;
						case 16: /* ^P, put */
							w->channel++;
							addChannel(w->channel);
							putChannel(w->channel);
							break;
						case 10: case 13: /* ^M, mute */
							s->channelv[w->channel].mute = !s->channelv[w->channel].mute;
							break;
						default:
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
										case ' ':
											previewNote(255, 0);
											r->note = 255;
											break;
										case 127: /* backspace */
											previewNote(254, 0);
											r->note = 0;
											r->inst = 255;
											break;
										case '=':
											previewNote(255, 0);
											r->note = 255;
											break;
										case '^':
											previewNote(254, 0);
											r->note = 254;
											break;
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
											uint8_t note = charToNote(input, w->octave);
											if (note) /* ignore nothing */
											{
												if (r->inst == 255)
													r->inst = w->instrument;
												r->note = note;
												previewNote(note, r->inst);
											}
											break;
									}
									break;
								case 1: /* instrument */
									switch (input)
									{
										case 1: /* ^a */
											r->inst++;
											if (r->inst == 255) r->inst = 0;
											break;
										case 24: /* ^x */
											r->inst--;
											if (r->inst == 255) r->inst = 254;
											break;
										case ' ':
											r->inst = w->instrument;
											break;
										case 127: /* backspace */
											r->inst = 255;
											w->fieldpointer = 0;
											break;
										case '0':           inputPatternHex(r, 0);  w->fieldpointer = !w->fieldpointer; break;
										case '1':           inputPatternHex(r, 1);  w->fieldpointer = !w->fieldpointer; break;
										case '2':           inputPatternHex(r, 2);  w->fieldpointer = !w->fieldpointer; break;
										case '3':           inputPatternHex(r, 3);  w->fieldpointer = !w->fieldpointer; break;
										case '4':           inputPatternHex(r, 4);  w->fieldpointer = !w->fieldpointer; break;
										case '5':           inputPatternHex(r, 5);  w->fieldpointer = !w->fieldpointer; break;
										case '6':           inputPatternHex(r, 6);  w->fieldpointer = !w->fieldpointer; break;
										case '7':           inputPatternHex(r, 7);  w->fieldpointer = !w->fieldpointer; break;
										case '8':           inputPatternHex(r, 8);  w->fieldpointer = !w->fieldpointer; break;
										case '9':           inputPatternHex(r, 9);  w->fieldpointer = !w->fieldpointer; break;
										case 'A': case 'a': inputPatternHex(r, 10); w->fieldpointer = !w->fieldpointer; break;
										case 'B': case 'b': inputPatternHex(r, 11); w->fieldpointer = !w->fieldpointer; break;
										case 'C': case 'c': inputPatternHex(r, 12); w->fieldpointer = !w->fieldpointer; break;
										case 'D': case 'd': inputPatternHex(r, 13); w->fieldpointer = !w->fieldpointer; break;
										case 'E': case 'e': inputPatternHex(r, 14); w->fieldpointer = !w->fieldpointer; break;
										case 'F': case 'f': inputPatternHex(r, 15); w->fieldpointer = !w->fieldpointer; break;
									}
									break;
								case 2: /* macro 1 */
									switch (input)
									{
										case 127: /* backspace */
											r->macroc[0] = 0;
											break;
										default:
											if (!r->macroc[0])
												r->macrov[0] = 0;
											changeMacro(input, &r->macroc[0]);
											break;
									}
									break;
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
											case 127: /* backspace */
												r->macroc[0] = 0;
												w->fieldpointer = 0;
												break;
											case '0':           inputPatternHex(r, 0);  w->fieldpointer = !w->fieldpointer; break;
											case '1':           inputPatternHex(r, 1);  w->fieldpointer = !w->fieldpointer; break;
											case '2':           inputPatternHex(r, 2);  w->fieldpointer = !w->fieldpointer; break;
											case '3':           inputPatternHex(r, 3);  w->fieldpointer = !w->fieldpointer; break;
											case '4':           inputPatternHex(r, 4);  w->fieldpointer = !w->fieldpointer; break;
											case '5':           inputPatternHex(r, 5);  w->fieldpointer = !w->fieldpointer; break;
											case '6':           inputPatternHex(r, 6);  w->fieldpointer = !w->fieldpointer; break;
											case '7':           inputPatternHex(r, 7);  w->fieldpointer = !w->fieldpointer; break;
											case '8':           inputPatternHex(r, 8);  w->fieldpointer = !w->fieldpointer; break;
											case '9':           inputPatternHex(r, 9);  w->fieldpointer = !w->fieldpointer; break;
											case 'A': case 'a': inputPatternHex(r, 10); w->fieldpointer = !w->fieldpointer; break;
											case 'B': case 'b': inputPatternHex(r, 11); w->fieldpointer = !w->fieldpointer; break;
											case 'C': case 'c': inputPatternHex(r, 12); w->fieldpointer = !w->fieldpointer; break;
											case 'D': case 'd': inputPatternHex(r, 13); w->fieldpointer = !w->fieldpointer; break;
											case 'E': case 'e': inputPatternHex(r, 14); w->fieldpointer = !w->fieldpointer; break;
											case 'F': case 'f': inputPatternHex(r, 15); w->fieldpointer = !w->fieldpointer; break;
										}
									else
									{
										r->macrov[0] = 0;
										changeMacro(input, &r->macroc[0]);
									}
									break;
								case 4: /* macro 2 */
									switch (input)
									{
										case 127: /* backspace */
											r->macroc[1] = 0;
											break;
										default:
											if (!r->macroc[1])
												r->macrov[1] = 0;
											changeMacro(input, &r->macroc[1]);
											break;
									}
									break;
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
											case 127: /* backspace */
												r->macroc[1] = 0;
												w->fieldpointer = 0;
												break;
											case '0':           inputPatternHex(r, 0);  w->fieldpointer = !w->fieldpointer; break;
											case '1':           inputPatternHex(r, 1);  w->fieldpointer = !w->fieldpointer; break;
											case '2':           inputPatternHex(r, 2);  w->fieldpointer = !w->fieldpointer; break;
											case '3':           inputPatternHex(r, 3);  w->fieldpointer = !w->fieldpointer; break;
											case '4':           inputPatternHex(r, 4);  w->fieldpointer = !w->fieldpointer; break;
											case '5':           inputPatternHex(r, 5);  w->fieldpointer = !w->fieldpointer; break;
											case '6':           inputPatternHex(r, 6);  w->fieldpointer = !w->fieldpointer; break;
											case '7':           inputPatternHex(r, 7);  w->fieldpointer = !w->fieldpointer; break;
											case '8':           inputPatternHex(r, 8);  w->fieldpointer = !w->fieldpointer; break;
											case '9':           inputPatternHex(r, 9);  w->fieldpointer = !w->fieldpointer; break;
											case 'A': case 'a': inputPatternHex(r, 10); w->fieldpointer = !w->fieldpointer; break;
											case 'B': case 'b': inputPatternHex(r, 11); w->fieldpointer = !w->fieldpointer; break;
											case 'C': case 'c': inputPatternHex(r, 12); w->fieldpointer = !w->fieldpointer; break;
											case 'D': case 'd': inputPatternHex(r, 13); w->fieldpointer = !w->fieldpointer; break;
											case 'E': case 'e': inputPatternHex(r, 14); w->fieldpointer = !w->fieldpointer; break;
											case 'F': case 'f': inputPatternHex(r, 15); w->fieldpointer = !w->fieldpointer; break;
										}
									else
									{
										r->macrov[1] = 0;
										changeMacro(input, &r->macroc[1]);
									}
									break;
							}
							break;
					}
					redraw();
					break;
				case 2:
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
								s->songa[w->songfx] = 0;
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
						case 127: /* backspace */
							if (w->songnext == w->songfx + 1)
								w->songnext = 0;
							s->songi[w->songfx] = 255;
							s->songa[w->songfx] = 0;
							prunePattern(s->songi[w->songfx], w->songfx);
							break;
						case 'l':
							if (s->songi[w->songfx] != 255)
								s->songa[w->songfx] = !s->songa[w->songfx];
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
					}
					redraw();
					break;
			}
			break;
	}

	return 0;
}
