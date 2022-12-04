/* undefined behaviour if multibyte chars are printed  */
/* only culls the x axis, culling the y axis is simple */
void printCulling(char *s, short x, short y, short minx, short maxx)
{
	if (x < minx) { if (x > minx - strlen(s)) printf("\033[%d;%dH%s", y, minx, s+MIN(minx - x, strlen(s))); }
	else if (x < maxx)                        printf("\033[%d;%dH%.*s", y, x, maxx - x, s);
}

void drawRuler(void)
{
	printf("\033[1m\033[0;%ldH%s\033[m", (ws.ws_col - strlen(PROGRAM_TITLE))>>1, PROGRAM_TITLE);
	printf("\033[1m\033[0;%dHv%d.%03d  %d\033[m", ws.ws_col - 15, MAJOR, MINOR, DEBUG);

	/* bottom ruler */
	if (w->mode < 255)
	{
		switch (w->instrumentrecv)
		{
			case INST_REC_LOCK_CONT: case INST_REC_LOCK_START:
				if (w->recptr == 0) printf("\033[%d;%dH\033[3m{REC %02x   0s}\033[m", ws.ws_row, ws.ws_col - 50, w->instrumentreci);
				else                printf("\033[%d;%dH\033[3m{REC %02x %3ds}\033[m", ws.ws_row, ws.ws_col - 50, w->instrumentreci, w->recptr / samplerate + 1);
				break;
			case INST_REC_LOCK_CUE_CONT: case INST_REC_LOCK_CUE_START:
				if (w->recptr == 0) printf("\033[%d;%dH\033[3m[cue]{REC %02x   0s}\033[m", ws.ws_row, ws.ws_col - 55, w->instrumentreci);
				else                printf("\033[%d;%dH\033[3m[cue]{REC %02x %3ds}\033[m", ws.ws_row, ws.ws_col - 55, w->instrumentreci, w->recptr / samplerate + 1);
				break;
		}

		if (w->count) printf("\033[%d;%dH%3d", ws.ws_row, ws.ws_col - 29, w->count);
		if (w->chord) printf("\033[%d;%dH%c", ws.ws_row, ws.ws_col - 26, w->chord);

		printf("\033[%d;%dH", ws.ws_row, ws.ws_col - 19);

		if (w->follow) printf(">");
		else           printf(" ");

		if (s->playing == PLAYING_STOP) printf("STOP");
		else                            printf("PLAY");

		if (w->follow) printf(">");
		else           printf(" ");

		printf(" &%d +%x  \033[1m%3dBPM\033[m", w->octave, w->step, s->songbpm);
	}
}

void redraw(void)
{
	fcntl(0, F_SETFL, 0); /* blocking */
	/* "CSI 2 J"     clears the screen              */
	/* "CSI ? 2 5 h"  ensures the cursor is visible  */
	/* "CSI 2   q" sets the cursor shape to block */
	puts("\033[2J\033[?25h\033[2 q");

	if (ws.ws_row < 14 + TRACK_ROW || ws.ws_col < 38 + INSTRUMENT_INDEX_COLS - 1)
	{
		printf("\033[%d;%dH%s", w->centre, (ws.ws_col - (unsigned short)strlen("(terminal too small)")) / 2, "(terminal too small)");
	} else
	{
		drawRuler();
		switch (w->page)
		{
			case PAGE_TRACK_VARIANT: case PAGE_TRACK_EFFECT: drawTracker();        break;
			case PAGE_INSTRUMENT:                            drawInstrument(&cc);  break;
			case PAGE_EFFECT_MASTER: case PAGE_EFFECT_SEND:  drawMaster();         break;
			case PAGE_PLUGINBROWSER:                         drawBrowser(pbstate); break;
		}
		drawCommand(&w->command, w->mode);
	}

	if (w->showtooltip) drawTooltip(&tt);

	fflush(stdout);
	fcntl(0, F_SETFL, O_NONBLOCK); /* non-blocking */
}
