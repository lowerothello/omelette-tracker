/* won't centre properly if multibyte chars are present */
void drawCentreText(short x, short y, short w, const char *text)
{
	x += (w - MIN(w, (short)strlen(text)))>>1;
	w = MIN(w, (short)strlen(text));

	if      (x >= 1) printf("\033[%d;%dH%.*s", y, x, MAX(0, MIN(w, (ws.ws_col+1) - x)), text);
	else if (x > -w) printf("\033[%d;%dH%.*s", y, x, w-x, text-(x-1)); /* x should always be <= 0 */
}

#ifndef OMELETTE_EFFECT_NO_STRUCTS
short getEffectHeight(Effect *e)
{
	if (!e) return 0;

	if (effect_api[e->type].height)
		effect_api[e->type].height(e->state);

	return 0;
}

static int _drawEffect(Effect *e, bool selected, short x, short width, short y, short ymin, short ymax)
{
	if (!e) return 0;

	short ret = getEffectHeight(e);

	if (selected) printf("\033[1;31m");
	drawBoundingBox(x, y-1, width, ret-1, 1, ws.ws_col, ymin, ymax);

	if (effect_api[e->type].draw)
		effect_api[e->type].draw(e->state, x, width, y, ymin, ymax);

	printf("\033[22;37m");

	return ret;
}

void drawEffectChain(EffectChain *chain, short x, short width, short y)
{
	if (x > ws.ws_col+1 || x+width < 1) return;

	uint8_t focusedindex = getEffectFromCursor(chain, cc.cursor);
	short ty = y + ((ws.ws_row-1 - y)>>1);

	if (chain->c)
	{
		for (uint8_t i = 0; i < focusedindex; i++)
			ty -= getEffectHeight(&chain->v[i]);

		ty -= getEffectHeight(&chain->v[focusedindex])>>1;

		for (uint8_t i = 0; i < chain->c; i++)
			ty += _drawEffect(&chain->v[i],
					focusedindex == i, x, width,
					ty+1, y, ws.ws_row-1);
		drawVerticalScrollbar(x + width + 2, y, ws.ws_row-1 - y, cc.controlc, cc.cursor);
	} else
	{
		drawBoundingBox(x, y, width, NULL_EFFECT_HEIGHT-1, 1, ws.ws_col, 1, ws.ws_row);
		printf("\033[m");

		x += ((width - (short)strlen(NULL_EFFECT_TEXT))>>1);
		printCulling(NULL_EFFECT_TEXT, x, y+1, 1, ws.ws_col);

		addControlDummy(MAX(1, MIN(ws.ws_col, x)), y+1);
	}
}
#endif

void drawAutogenPluginLine(short x, short y, short w,
		short ymin, short ymax,
		const char *name, float *value,
		bool toggled, bool integer,
		float min, float max, float def,
		char *prefix, char *postfix,
		uint32_t scalepointlen, uint32_t scalepointcount)
{
	short controloffset = w;
	if (ymin <= y && ymax >= y)
	{
		if (postfix)
		{
			controloffset -= strlen(postfix);
			if (controloffset >= 0)
				printf("\033[%d;%dH%s", y, x + controloffset - 1, postfix);
		}

		if (toggled) /* boolean */
		{
			controloffset -= 3;
			addControlFloat(x + controloffset, y,
					value, CONTROL_NIBBLES_TOGGLED,
					min, max, def, scalepointlen, scalepointcount, NULL, NULL);
		} else if (!isnan(min) && !isnan(max) && min < 0.0f)
		{ /* signed */
			if (integer) /* signed int */
			{
				controloffset -= 3 + getPreRadixDigits(max);
				addControlFloat(x + controloffset, y,
						value, CONTROL_NIBBLES_SIGNED_INT,
						min, max, def, scalepointlen, scalepointcount, NULL, NULL);
			} else /* signed float */
			{
				controloffset -= 10;
				addControlFloat(x + controloffset, y,
						value, CONTROL_NIBBLES_SIGNED_FLOAT,
						min, max, def, scalepointlen, scalepointcount, NULL, NULL);
			}
		} else
		{ /* unsigned */
			if (integer) /* unsigned int */
			{
				controloffset -= 2 + getPreRadixDigits(max);
				addControlFloat(x + controloffset, y,
						value, CONTROL_NIBBLES_UNSIGNED_INT,
						min, max, def, scalepointlen, scalepointcount, NULL, NULL);
			} else /* unsigned float */
			{
				controloffset -= 9;
				addControlFloat(x + controloffset, y,
						value, CONTROL_NIBBLES_UNSIGNED_FLOAT,
						min, max, def, scalepointlen, scalepointcount, NULL, NULL);
			}
		}

		controloffset -= 1;

		if (prefix)
		{
			controloffset -= strlen(prefix);
			if (controloffset >= 0)
				printf("\033[%d;%dH%s", y, x + controloffset, prefix);
		}

		controloffset -= 2;

		if (controloffset >= 0)
		{
			if (x+1 >= 1)
				printf("\033[%d;%dH%.*s", y, x+1, MIN(controloffset, ws.ws_col - x), name);
			else if (x+1 + MIN(controloffset, (int)strlen(name)) > 1)
				printf("\033[%d;%dH%.*s", y, x, controloffset-x, name-x);
		}
	} else addControlInt(0, 0, NULL, 0, 0, 0, 0, 0, 0, NULL, NULL);
}
