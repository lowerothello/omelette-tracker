#define SONGLIST_ROWS 20
#define SONGLIST_COLS 15


void inputSongHex(char value)
{
	prunePattern(s->songi[w->songfy], w->songfy);
	if (s->songi[w->songfy] == 255)
		s->songi[w->songfy] = 0;
	s->songi[w->songfy] <<= 4; s->songi[w->songfy] += value;
	addPattern(s->songi[w->songfy], 0);
	redraw();
}

void drawSong(void)
{
	printf("\033[%d;%dH\033[1mSONG\033[m", CHANNEL_ROW-2, (ws.ws_col - 4) / 2);

	unsigned char visiblechannels = MIN((ws.ws_col - 16) / 9, s->channelc);
	unsigned short x = (ws.ws_col - MIN(ws.ws_col, 13 + 9*visiblechannels)) / 2;

	printf("\033[1m\033[%d;%dH┌\033[%d;%dH┐\033[m",
			CHANNEL_ROW-1, x+16,
			CHANNEL_ROW-1, x+16 + visiblechannels*9 - 4);

	printf("\033[%d;%dH\033[1mINDICES\033[m", CHANNEL_ROW, x+4);
	printf("\033[%d;%dH", CHANNEL_ROW, x+16);
	for (unsigned short j = 0; j < visiblechannels; j++)
		if (s->channelv[j].mute) printf("\033[2mCHNL%02x\033[m   ", j);
		else                     printf("\033[1mCHNL%02x\033[m   ", j);

	for (int i = 0; i < 256; i++)
	{
		if (w->centre - w->songfy + i > CHANNEL_ROW
				&& w->centre - w->songfy + i < ws.ws_row)
		{
			printf("\033[%d;%dH%02x", w->centre - w->songfy + i, x, i);
			if (w->songnext - 1 == i)                             printf(" > ");
			else if (s->playing == PLAYING_CONT && i == s->songp) printf(" - ");
			else if (s->rowhighlight && !(i % s->rowhighlight))   printf(" * ");
			else                                                  printf("   ");

			if (s->songi[i] == 255) printf(".. ..");
			else printf("%02x %02x", s->songi[i], s->patternv[s->patterni[s->songi[i]]]->rowc);

			if (w->songnext - 1 == i)                             printf(" < ");
			else if (s->playing == PLAYING_CONT && i == s->songp) printf(" - ");
			else if (s->rowhighlight && !(i % s->rowhighlight))   printf(" * ");
			else                                                  printf("   ");

			printf("   ");
			for (unsigned short j = 0; j < visiblechannels; j++)
				if (s->channelv[j].mute) printf("\033[2m.. ...\033[m   ");
				else printf(".. ...   ");
		}
	}
	switch (w->songfx)
	{
		case 0: printf("\033[%d;%dH", w->centre + w->fyoffset, x+LINENO_COLS+1); break;
		case 1: printf("\033[%d;%dH", w->centre + w->fyoffset, x+LINENO_COLS+4); break;
	}
}

void songInput(int input)
{
	switch (input)
	{
		case '\033':
			switch (getchar())
			{
				case 'O':
					switch (getchar())
					{
						case 'P':
							w->instrumentindex = MIN_INSTRUMENT_INDEX;
							w->popup = 1;
							w->mode = 0;
							break;
						case 'Q':
							w->popup = 0;
							w->mode = 0;
							break;
					}
					redraw();
					break;
				case '[':
					switch (getchar())
					{
						case 'A': /* up arrow    */ if (w->songfy > 0) w->songfy--; redraw(); break;
						case 'B': /* down arrow  */ if (w->songfy < 255) w->songfy++; redraw(); break;
						case 'D': /* left arrow  */ break;
						case 'C': /* right arrow */ break;
						case 'H': /* home */
							w->songfy = 0;
							redraw();
							break;
						case '4': /* end */
							if (getchar() == '~')
							{
								w->songfy = 255;
								redraw();
							}
							break;
						case '1': /* mod+arrow / f5 - f8 */
							switch (getchar())
							{
								case '5': /* f5, play */
									getchar();
									startPlayback();
									break;
								case '7': /* f6, stop */
									getchar();
									stopPlayback();
									break;
								case ';': /* mod+arrow */
									getchar();
									getchar();
									break;
							}
							break;
						case 'M': /* mouse */
							int button = getchar();
							int x = getchar() - 32;
							int y = getchar() - 32;
							switch (button)
							{
								case WHEEL_UP: case WHEEL_UP_CTRL: /* scroll up */
									if (w->songfy > WHEEL_SPEED) w->songfy -= WHEEL_SPEED;
									else                         w->songfy = 0;
									break;
								case WHEEL_DOWN: case WHEEL_DOWN_CTRL: /* scroll up */
									if (w->songfy < 255 - WHEEL_SPEED) w->songfy += WHEEL_SPEED;
									else                               w->songfy = 255;
									break;
								case BUTTON_RELEASE: case BUTTON_RELEASE_CTRL: /* release click */
									w->songfy = MAX(0, MIN(255, w->songfy + w->fyoffset));
									w->fyoffset = 0;
									break;
								case BUTTON1: case BUTTON1_CTRL:
								case BUTTON3: case BUTTON3_CTRL:
									if (y > ws.ws_row - 1) break; /* ignore clicking out of range */
									if (y <= CHANNEL_ROW)
									{
										unsigned char visiblechannels = MIN((ws.ws_col - 16) / 9, s->channelc);
										unsigned short vx = (ws.ws_col - MIN(ws.ws_col, 13 + 9*visiblechannels)) / 2;
										uint8_t channel = MIN((x - vx - 15) / 9, visiblechannels);
										s->channelv[channel].mute = !s->channelv[channel].mute;
									} else
									{
										w->fyoffset = y - w->centre;
									}
									break;
							}
							redraw();
							break;
					}
					break;
				default:
					w->popup = 0;
					w->mode = 0;
					redraw();
					break;
			}
			break;
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
	}
}
