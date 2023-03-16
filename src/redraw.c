#define RULER_WIDTH 19

/* globals instead of static to the mouse function so they can be checked against in the draw code */
int staging_octave = 0; bool reset_octave = 0;
int staging_step = 0; bool reset_step = 0;
int staging_play = 0;

static void drawRuler(void)
{
	/* top ruler */
	printf("\033[1m\033[0;%ldH%s\033[m", (ws.ws_col - strlen(PROGRAM_TITLE))>>1, PROGRAM_TITLE);
	printf("\033[1m\033[0;%dHv$%04x  %d\033[m", ws.ws_col - 15, version, DEBUG);

	int previewtracks = PREVIEW_TRACKS;
	if (input_mode == INPUTMODE_NONE)
		previewtracks = 1;

	printf("\033[%d;%dH", 2, ws.ws_col - previewtracks*4 - 2);
	char buffer[4];
	for (int i = 0; i < previewtracks; i++)
	{
		noteToString(w->previewtrack[i]->r.note, buffer);
		printf("%s ", buffer);
	}
	printf("%02x", w->instrument);

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

		printf("\033[%d;%dH", ws.ws_row, ws.ws_col - RULER_WIDTH);

		if (staging_play) printf(STAGING_FORMAT);
		if (w->follow) printf(">");
		else           printf(" ");

		if (s->playing) printf("PLAY");
		else            printf("STOP");

		if (w->follow) printf(">");
		else           printf(" ");
		printf("\033[m ");

		if (reset_octave) printf(RESET_FORMAT);
		else if (staging_octave) printf(STAGING_FORMAT);
		printf("&%d\033[m ", w->octave);

		if (reset_step) printf(RESET_FORMAT);
		else if (staging_step) printf(STAGING_FORMAT);
		printf("+%x\033[m  ", w->step);

		printf("\033[1m%3dBPM\033[m", s->songbpm);
	}
}

/* returns 0 to fall through */
int rulerMouse(enum Button button, int x, int y)
{
	switch (button)
	{
		case BUTTON1: /* fall through */
		case WHEEL_UP:
			if (y < ws.ws_row) return 0;
			if (x >= ws.ws_col - RULER_WIDTH && x <= ws.ws_col - RULER_WIDTH + 5) staging_play = 1;
			if (x >= ws.ws_col - RULER_WIDTH + 7 && x <= ws.ws_col - RULER_WIDTH + 8) staging_octave++;
			if (x >= ws.ws_col - RULER_WIDTH + 10 && x <= ws.ws_col - RULER_WIDTH + 11) staging_step++;
			p->redraw = 1;
			return 1;

		case BUTTON2:
			if (y < ws.ws_row) return 0;
			if (x >= ws.ws_col - RULER_WIDTH && x <= ws.ws_col - RULER_WIDTH + 5) staging_play = 2;
			if (x >= ws.ws_col - RULER_WIDTH + 7 && x <= ws.ws_col - RULER_WIDTH + 8) reset_octave = 1;
			if (x >= ws.ws_col - RULER_WIDTH + 10 && x <= ws.ws_col - RULER_WIDTH + 11) reset_step = 1;
			p->redraw = 1;
			return 1;

		case BUTTON3: /* fall through */
		case WHEEL_DOWN:
			if (y < ws.ws_row) return 0;
			if (x >= ws.ws_col - RULER_WIDTH && x <= ws.ws_col - RULER_WIDTH + 5) staging_play = 3;
			if (x >= ws.ws_col - RULER_WIDTH + 7 && x <= ws.ws_col - RULER_WIDTH + 8) staging_octave--;
			if (x >= ws.ws_col - RULER_WIDTH + 10 && x <= ws.ws_col - RULER_WIDTH + 11) staging_step--;
			p->redraw = 1;
			return 1;

		case BUTTON_RELEASE: case BUTTON_RELEASE_CTRL:
			switch (staging_play)
			{
				case 1: startPlayback(); staging_play = 0; break;
				case 2: toggleSongFollow(); staging_play = 0; break;
				case 3: stopPlayback(); staging_play = 0; break;
			}

			if (reset_octave)
			{
				w->octave = DEF_OCTAVE;
				reset_octave = 0;
				p->redraw = 1;
			}
			if (staging_octave)
			{
				addOctave(staging_octave);
				staging_octave = 0;
				p->redraw = 1;
			}

			if (reset_step)
			{
				w->step = DEF_STEP;
				reset_step = 0;
				p->redraw = 1;
			}
			if (staging_step)
			{
				addStep(staging_step);
				staging_step = 0;
				p->redraw = 1;
			}
			/* fall through */
		default:
			return 0;
	}
}

static void setBpmCount(void) { s->songbpm = MIN(255, MAX(32, w->count)); reapplyBpm(); p->redraw = 1; }
static void setRowHighlightCount(void) { s->rowhighlight = MIN(16, w->count); regenGlobalRowc(s); p->redraw = 1; }
static void setOctaveCount(void) { w->octave = MIN(w->count, MAX_OCTAVE); p->redraw = 1; }
static void setStepCount(void) { w->step = MIN(w->count, MAX_STEP); p->redraw = 1; }
static void setInstCount(void) { w->instrument = MIN(w->count, INSTRUMENT_MAX); p->redraw = 1; }

void addRulerBinds(void)
{
	addTooltipBind("count set bpm"          , 0, XK_B, 0, (void(*)(void*))setBpmCount         , NULL);
	addTooltipBind("count set row highlight", 0, XK_R, 0, (void(*)(void*))setRowHighlightCount, NULL);
	addTooltipBind("count set octave"       , 0, XK_O, 0, (void(*)(void*))setOctaveCount      , NULL);
	addTooltipBind("count set step"         , 0, XK_S, 0, (void(*)(void*))setStepCount        , NULL);
	addTooltipBind("count set instrument"   , 0, XK_I, 0, (void(*)(void*))setInstCount        , NULL);
}


void redraw(void)
{
	fcntl(0, F_SETFL, 0); /* blocking */

	/* "CSI 2 J"      clears the screen              */
	/* "CSI ? 2 5 h"  ensures the cursor is visible  */
	/* "CSI 2   q"    sets the cursor shape to block */
	printf("\033[2J\033[?25h\033[2 q");

	drawRuler();
	switch (w->page)
	{
		case PAGE_VARIANT:       drawTracker();        break;
		case PAGE_INSTRUMENT:    drawInstrument();     break;
		case PAGE_PLUGINBROWSER: drawBrowser(pbstate); break;
	}
	drawCommand(&w->command, w->mode);

	if (w->showtooltip) drawTooltip();

	fflush(stdout);
	fcntl(0, F_SETFL, O_NONBLOCK); /* non-blocking */
}
