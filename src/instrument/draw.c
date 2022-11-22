#include "sampledraw.c"

/* negative x values are unsupported, no reason to make this fully scrollable (yet) */
void drawInstrumentIndex(short bx)
{
	Instrument *iv;
	char buffer[11];
	short x;
	for (int i = 0; i < INSTRUMENT_MAX; i++)
		if (w->centre - w->instrument + i > TRACK_ROW && w->centre - w->instrument + i < ws.ws_row)
		{
			x = bx;

			if (instrumentSafe(s, i))
			{
				iv = &s->instrument->v[s->instrument->i[i]];
				if (iv->triggerflash) printf("\033[3%dm", i%6+1);
			}
			if (w->instrument + w->fyoffset == i)
			{
				printf("\033[7m");

				if (x <= ws.ws_col)
				{
					snprintf(buffer, 4, "%02x ", i);
					printf("\033[%d;%dH%.*s", w->centre - w->instrument + i, x, ws.ws_col - x, buffer);
				} x += 3;

				if (x <= ws.ws_col)
				{
					snprintf(buffer, 4, "%02x ", s->instrument->i[i]);
					printf("\033[%d;%dH%.*s", w->centre - w->instrument + i, x, ws.ws_col - x, buffer);
				} x += 3;
			} else
			{
				if (x <= ws.ws_col)
				{
					snprintf(buffer, 4, "%02x ", i);
					printf("\033[%d;%dH%.*s", w->centre - w->instrument + i, x, ws.ws_col - x, buffer);
				} x += 3;

				if (x <= ws.ws_col)
				{
					snprintf(buffer, 4, "%02x ", s->instrument->i[i]);
					printf("\033[2m");
					printf("\033[%d;%dH%.*s", w->centre - w->instrument + i, x, ws.ws_col - x, buffer);
					printf("\033[22m");
				} x += 3;
			}

			if (x <= ws.ws_col)
			{
				if (instrumentSafe(s, i))
				{
					iv = &s->instrument->v[s->instrument->i[i]];
					printf("\033[1m");
					if (iv->algorithm == INST_ALG_MIDI) snprintf(buffer, 11, "-  MIDI  -");
					else if (iv->sample)                snprintf(buffer, 11, "<%08x>", iv->sample->length);
					else                                snprintf(buffer, 11, "<%08x>", 0);
				} else snprintf(buffer, 11, " ........ ");
				printf("\033[%d;%dH%.*s", w->centre - w->instrument + i, x, ws.ws_col - x, buffer);
			}
			x += 10;

			printf("\033[40;37;22;27m");
		}

}

void drawInstrument(void)
{
	switch (w->mode)
	{
		case I_MODE_INSERT:
			if (cc.mouseadjust || cc.keyadjust) printf("\033[%d;0H\033[1m-- INSERT ADJUST --\033[m\033[4 q", ws.ws_row);
			else                                printf("\033[%d;0H\033[1m-- INSERT --\033[m\033[6 q",        ws.ws_row);
			w->command.error[0] = '\0';
			break;
		default:
			if (cc.mouseadjust || cc.keyadjust) { printf("\033[%d;0H\033[1m-- ADJUST --\033[m\033[4 q", ws.ws_row); w->command.error[0] = '\0'; }
			break;
	}

	drawInstrumentIndex(1);

	Instrument *iv;
	if (instrumentSafe(s, w->instrument))
	{
		iv = &s->instrument->v[s->instrument->i[w->instrument]];
		drawInstrumentSampler(iv);
	} else
	{
		const char *text = "PRESS 'a' TO ADD AN INSTRUMENT";
		printf("\033[%d;%dH%s", w->centre, INSTRUMENT_INDEX_COLS + ((ws.ws_col - INSTRUMENT_INDEX_COLS - (short)strlen(text))>>1), text);
	}

	// if (w->mode == I_MODE_INDICES) printf("\033[%d;%dH", w->centre + w->fyoffset, 8);
}
