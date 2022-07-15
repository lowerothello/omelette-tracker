/* void waveformInput(int input)
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
						case 'D': // left arrow
							if (w->waveformoffset > delta)
								w->waveformoffset -= delta;
							else
								w->waveformoffset = 0;
							resizeWaveform();
							redraw();
							break;
						case 'C': // right arrow
							if (w->waveformwidth < iv->length
									&& w->waveformoffset < iv->length - w->waveformwidth - delta)
								w->waveformoffset += delta;
							else
								w->waveformoffset = iv->length - w->waveformwidth;
							resizeWaveform();
							redraw();
							break;
						case 'H': // home
							w->waveformoffset = 0;
							resizeWaveform();
							redraw();
							break;
						case '4': // end
							if (getchar() == '~')
							{
								w->waveformoffset = iv->length - w->waveformwidth;
								resizeWaveform();
								redraw();
							}
							break;
						case '5': // page up
							getchar();
							if (w->waveformwidth > delta+delta)
								w->waveformwidth -= delta;
							else
								w->waveformwidth = delta;
							resizeWaveform();
							redraw();
							break;
						case '6': // page down
							getchar();
							if (w->waveformwidth < iv->length - w->waveformoffset - delta)
								w->waveformwidth += delta;
							else
								w->waveformwidth = iv->length - w->waveformoffset;
							resizeWaveform();
							redraw();
							break;
						case 'M': // mouse
							getchar(); // button
							getchar(); // xpos +32
							getchar(); // ypos +32
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
} */
