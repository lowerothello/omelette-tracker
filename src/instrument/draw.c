#include "sampledraw.c"

void drawInstrument(void)
{
	printf("\033[%d;%dH\033[2mCHANNEL\033[m \033[1minstrument\033[m", 2, (ws.ws_col-18)>>1);
	switch (w->page)
	{
		case PAGE_INSTRUMENT_SAMPLE: printf("\033[%d;%dH\033[3msample\033[m \033[3;2mEFFECT\033[m", 3, (ws.ws_col-13)>>1); break;
		case PAGE_INSTRUMENT_EFFECT: printf("\033[%d;%dH\033[3;2mSAMPLE\033[m \033[3meffect\033[m", 3, (ws.ws_col-13)>>1); break;
	}

	if (cc.mouseadjust || cc.keyadjust)
	{
		printf("\033[%d;0H\033[1m-- ADJUST --\033[m", ws.ws_row);
		w->command.error[0] = '\0';
	}

	Instrument *iv;

	for (int i = 0; i < INSTRUMENT_MAX; i++)
		if (w->centre - w->instrument + i >= CHANNEL_ROW && w->centre - w->instrument + i < ws.ws_row)
		{
			if (w->mode != I_MODE_INDICES && w->instrument == i)
			{
				printf("\033[7m");
				if (instrumentSafe(s, i))
				{
					iv = &s->instrument->v[s->instrument->i[i]];
					if (iv->triggerflash) printf("\033[4%dm", i%6+1);
					if (iv->algorithm == INST_ALG_MIDI) printf("\033[%d;1H%02x %02x \033[1m - MIDI - ", w->centre - w->instrument + i, i, s->instrument->i[i]);
					else if (iv->sample)                printf("\033[%d;1H%02x %02x \033[1m<%08x>",     w->centre - w->instrument + i, i, s->instrument->i[i], iv->sample->length);
					else                                printf("\033[%d;1H%02x %02x \033[1m<%08x>",     w->centre - w->instrument + i, i, s->instrument->i[i], 0);
				} else                                  printf("\033[%d;1H%02x %02x  ........ ",        w->centre - w->instrument + i, i, s->instrument->i[i]);
				printf("\033[40;22;27m");
			} else
			{
				if (instrumentSafe(s, i))
				{
					iv = &s->instrument->v[s->instrument->i[i]];
					if (iv->triggerflash) printf("\033[3%dm", i%6+1);
					if (iv->algorithm == INST_ALG_MIDI) printf("\033[%d;1H%02x \033[2m%02x\033[22m \033[1m - MIDI - ", w->centre - w->instrument + i, i, s->instrument->i[i]);
					else if (iv->sample)                printf("\033[%d;1H%02x \033[2m%02x\033[22m \033[1m<%08x>",     w->centre - w->instrument + i, i, s->instrument->i[i], iv->sample->length);
					else                                printf("\033[%d;1H%02x \033[2m%02x\033[22m \033[1m<%08x>",     w->centre - w->instrument + i, i, s->instrument->i[i], 0);
				} else                                  printf("\033[%d;1H%02x \033[2m%02x\033[22m  ........ ",        w->centre - w->instrument + i, i, s->instrument->i[i]);
				printf("\033[37;22m");
			}
		}

	if (instrumentSafe(s, w->instrument))
	{
		iv = &s->instrument->v[s->instrument->i[w->instrument]];
		switch (w->page)
		{
			case PAGE_INSTRUMENT_SAMPLE: drawInstrumentSampler(iv); break;
			case PAGE_INSTRUMENT_EFFECT: drawEffects(iv->effect, INSTRUMENT_INDEX_COLS+1, ws.ws_col - (INSTRUMENT_INDEX_COLS+1), CHANNEL_ROW); break;
		}
	} else
	{
		const char *text = "PRESS 'a' TO ADD AN INSTRUMENT";
		printf("\033[%d;%dH%s", w->centre, INSTRUMENT_INDEX_COLS + ((ws.ws_col - INSTRUMENT_INDEX_COLS - (short)strlen(text))>>1), text);
	}

	if (w->mode == I_MODE_INDICES) printf("\033[%d;%dH", w->centre + w->fyoffset, 8);
}
