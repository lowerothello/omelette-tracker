#define NULL_EFFECT_HEIGHT 2
#define NULL_EFFECT_TEXT "NO EFFECTS"

/* selected is whether or not this chain should have bold outlines drawn */
void drawEffects(EffectChain *chain, ControlState *cc, bool selected, short x, short width, short y)
{
	if (x > ws.ws_col+1 || x+width < 1) return;
	if (chain->c)
	{
		uint8_t selectedindex = getEffectFromCursor(chain, cc->cursor);
		short maxheight      = 0;
		short selstartheight = 0;
		short selendheight   = 0;
		for (int i = 0; i < chain->c; i++)
		{
			if (i == selectedindex)
			{
				selstartheight = maxheight;
				maxheight += getEffectHeight(&chain->v[i]);
				selendheight = maxheight;
			} else maxheight += getEffectHeight(&chain->v[i]);
		}

		/* content height - viewport height = max scroll offset */
		w->effectscroll = MIN(w->effectscroll, maxheight - (ws.ws_row - y));
		w->effectscroll = MAX(w->effectscroll, 0);
		/* make sure the selected effect is fully on screen */
		w->effectscroll = MIN(w->effectscroll, selstartheight);
		w->effectscroll = MAX(w->effectscroll, selendheight - (ws.ws_row - y));

		short ty = y - w->effectscroll;

		for (uint8_t i = 0; i < chain->c; i++)
			ty += drawEffect(&chain->v[i], cc,
					selected && selectedindex == i, x, width,
					ty+1, y, ws.ws_row-1);
	} else
	{
		w->effectscroll = 0;

		if (selected) printf("\033[1m");
		drawBoundingBox(x, y, width, NULL_EFFECT_HEIGHT, 1, ws.ws_col, 1, ws.ws_row);
		printf("\033[m");

		x += ((width - (short)strlen(NULL_EFFECT_TEXT))>>1);
		printCulling(NULL_EFFECT_TEXT, x, y+1, 1, ws.ws_col);

		printf("\033[%d;%dH", y+1, MAX(1, MIN(ws.ws_col, x))); /* visual cursor */
	}
}
