void resizeWaveform(void)
{
	instrument *iv = s->instrumentv[s->instrumenti[w->instrument]];
	if (iv->samplelength)
	{
		fill(w->waveformcanvas, 0);

		/* draw sample data */
		uint32_t maxj = w->waveformw * w->waveformh * WAVEFORM_OVERSAMPLING;
		for (uint8_t i = 0; i < iv->channels; i++) /* mix all channels */
			for (uint32_t j = 0; j < maxj; j++)
			{
				size_t k = (float)j / maxj * w->waveformwidth;
				size_t x = ((float)k / w->waveformwidth) * w->waveformw;
				float sample = iv->sampledata[w->waveformoffset + k * iv->channels + i]*DIVSHRT;
				float offset = (w->waveformh-2)*0.5; // leave space for the scrollbar
				set_pixel_unsafe(w->waveformcanvas, 1, x, sample * offset + offset);
			}

		/* draw scrollbar */
		size_t scrollbarstart = w->waveformoffset / (float)iv->length * w->waveformw;
		size_t scrollbarend = scrollbarstart + w->waveformwidth / (float)iv->length * w->waveformw;
		for (size_t i = scrollbarstart; i < scrollbarend; i++)
			set_pixel_unsafe(w->waveformcanvas, 1, i, w->waveformh-1);
	}
}
void drawWaveform(void)
{
	printf("\033[?25l\033[%d;%dH\033[1mWAVEFORM\033[m", CHANNEL_ROW-2, (ws.ws_col - 8) / 2);

	instrument *iv = s->instrumentv[s->instrumenti[w->instrument]];
	if (!iv->samplelength)
	{
		printf("\033[%d;%dH[NO SAMPLE]\033[10D", w->centre, (ws.ws_col - 11) / 2);
	} else
	{
		draw(w->waveformcanvas, w->waveformbuffer);
		printf("\033[2;0H");
		for (size_t i = 0; w->waveformbuffer[i] != NULL; i++)
			printf("%s\n", w->waveformbuffer[i]);
	}
}

void waveformInput(int input)
{
	instrument *iv = s->instrumentv[s->instrumenti[w->instrument]];
	uint32_t delta = iv->length / 100;
	switch (input)
	{
		case '\033':
			switch (getchar())
			{
				case 'O':
					switch (getchar())
					{
						case 'P':
							w->instrumentindex = 0;
							w->popup = 0;
							w->mode = 0;
							break;
						case 'Q':
							w->popup = 3;
							w->mode = 0;
							break;
					}
					redraw();
					break;
				case '[':
					switch (getchar())
					{
						case 'D': /* left arrow */
							if (w->waveformoffset > delta)
								w->waveformoffset -= delta;
							else
								w->waveformoffset = 0;
							resizeWaveform();
							redraw();
							break;
						case 'C': /* right arrow */
							if (w->waveformwidth < iv->length
									&& w->waveformoffset < iv->length - w->waveformwidth - delta)
								w->waveformoffset += delta;
							else
								w->waveformoffset = iv->length - w->waveformwidth;
							resizeWaveform();
							redraw();
							break;
						case 'H': /* home */
							w->waveformoffset = 0;
							resizeWaveform();
							redraw();
							break;
						case '4': /* end */
							if (getchar() == '~')
							{
								w->waveformoffset = iv->length - w->waveformwidth;
								resizeWaveform();
								redraw();
							}
							break;
						case '5': /* page up */
							getchar();
							if (w->waveformwidth > delta+delta)
								w->waveformwidth -= delta;
							else
								w->waveformwidth = delta;
							resizeWaveform();
							redraw();
							break;
						case '6': /* page down */
							getchar();
							if (w->waveformwidth < iv->length - w->waveformoffset - delta)
								w->waveformwidth += delta;
							else
								w->waveformwidth = iv->length - w->waveformoffset;
							resizeWaveform();
							redraw();
							break;
						case 'M': /* mouse */
							getchar(); /* button   */
							getchar(); /* xpos +32 */
							getchar(); /* ypos +32 */
							break;
					}
					break;
				default:
					w->popup = 1;
					w->instrumentindex = 0;
					redraw();
					break;
			} break;
	}
}
