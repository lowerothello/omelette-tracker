#define NULL_EFFECT_HEIGHT 2
#define NULL_EFFECT_TEXT "NO EFFECTS"

/* x = INSTRUMENT_INDEX_COLS+1;
width = ws.ws_col - x; */

void drawEffects(EffectChain *chain, short x, short width, short y)
{
	clearControls(&cc);

	if (chain->c)
	{
		uint8_t selectedindex = getEffectFromCursor(chain, cc.cursor);
		short maxheight      = 0;
		short selstartheight = 0;
		short selendheight   = 0;
		for (int i = 0; i < chain->c; i++)
		{
			if (i == selectedindex)
			{
				selstartheight = maxheight;
				maxheight += getEffectHeight(&chain->v[i], width);
				selendheight = maxheight;
			} else maxheight += getEffectHeight(&chain->v[i], width);
		}

		/* content height - viewport height = max scroll offset */
		w->effectscroll = MIN(w->effectscroll, maxheight - (ws.ws_row - y));
		w->effectscroll = MAX(w->effectscroll, 0);
		/* make sure the selected effect is fully on screen */
		w->effectscroll = MIN(w->effectscroll, selstartheight);
		w->effectscroll = MAX(w->effectscroll, selendheight - (ws.ws_row - y));

		short ty = y - w->effectscroll;

		for (uint8_t i = 0; i < chain->c; i++)
			ty += drawEffect(&chain->v[i], &cc,
					selectedindex == i, x, width,
					ty+1, y, ws.ws_row-1);
	} else
	{
		w->effectscroll = 0;
		for (int i = 0; i < width; i++)
			printf("\033[%d;%dH─", y, x+i);
		printf("\033[%d;%dH┌\033[%d;%dH┒",
				y, x-1,
				y, x+width);
		for (int i = 0; i < width; i++)
			printf("\033[%d;%dH━", y+NULL_EFFECT_HEIGHT, x+i);
		printf("\033[%d;%dH┕\033[%d;%dH┛",
				y+NULL_EFFECT_HEIGHT, x-1,
				y+NULL_EFFECT_HEIGHT, x+width);
		for (int i = 1; i < NULL_EFFECT_HEIGHT; i++)
			printf("\033[%d;%dH│\033[%d;%dH┃",
					y+i, x-1,
					y+i, x+width);

		printf("\033[%d;%dH%s", y+1, x + ((width - (short)strlen(NULL_EFFECT_TEXT))>>1), NULL_EFFECT_TEXT);
		printf("\033[%d;%dH",   y+1, x + ((width - (short)strlen(NULL_EFFECT_TEXT))>>1));
	}

	drawControls(&cc);
}
