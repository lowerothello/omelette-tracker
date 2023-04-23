/* won't centre properly if multibyte chars are present */
void drawCentreText(short x, short y, short w, const char *text)
{
	x += (w - MIN(w, (short)strlen(text)))>>1;
	w = MIN(w, (short)strlen(text));

	printCulling(text, x, y, 1, MIN(ws.ws_col, x + w));
}

#ifndef OMELETTE_EFFECT_NO_STRUCTS
short getEffectHeight(Effect *e)
{
	if (!e) return 0;

	if (effect_api[e->type].height)
		return effect_api[e->type].height(e->state);

	return 0;
}

char *stringToUpper(char *s)
{
	if (!s) return NULL;
	for (size_t i = 0; s[i]; i++)
		s[i] = toupper(s[i]);
	return s;
}

static short _drawEffect(Effect *e, bool selected, short x, short width, short y, short ymin, short ymax)
{
	if (!e) return 0;

	short ret = getEffectHeight(e);

	if (selected) printf("\033[1;31m");
	drawBoundingBox(x, y-1, width, ret-1, 1, ws.ws_col, ymin, ymax);

	printf("\033[7m");
	if (ymin <= y-1 && ymax >= y-1)
	{
		char *upperType = stringToUpper(strdup(EffectTypeString[e->type]));
		if (upperType)
		{
			printCulling(upperType, x+1, y-1, 1, ws.ws_col);
			free(upperType);
		}
	}
	printf("\033[27;37;40m");

	/* NOT in the y culling block, intentional! */
	addControlInt(x + width - 3, y-1, &e->bypass, 0, 0, 1, 0, 0, 0, NULL, NULL);

	if (effect_api[e->type].draw)
		effect_api[e->type].draw(e->state, x, width, y, ymin, ymax);

	printf("\033[m");

	return ret;
}

void drawEffectChain(uint8_t track, EffectChain *chain, short x, short width, short y)
{
	short focusindex;
	if (track == w->track) focusindex = getEffectFromCursor(track, chain, cc.cursor);
	else                   focusindex = -1;

	if (chain->c)
	{
		short height = 0;
		short focusheight = 0;
		short ty = y;
		for (uint8_t i = 0; i < chain->c; i++)
		{
			height += getEffectHeight(&chain->v[i]);
			if (i == focusindex)
				focusheight = height;
		}

		if ((ws.ws_row - 1) - height < y)
		{
			size_t chaincontrolc = 0;
			for (int i = 0; i < chain->c; i++)
				chaincontrolc += getEffectControlCount(&chain->v[i]);

			width--;
			if (focusindex > -1)
			{
				drawVerticalScrollbar(x + width + 1, y, ws.ws_row-1 - y, chaincontrolc, cc.cursor - getCursorFromEffectTrack(track));
				ty = MIN(y, (ws.ws_row - 1) - focusheight);
			} else
				drawVerticalScrollbar(x + width + 1, y, ws.ws_row-1 - y, chaincontrolc, 0);
		}

		for (uint8_t i = 0; i < chain->c; i++)
			ty += _drawEffect(&chain->v[i],
					focusindex == i, x, width,
					ty+1, y, ws.ws_row-1);
	} else
	{
		if (focusindex > -1) printf("\033[1;31m");
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
	if (
			ymin <= y &&
			ymax >= y &&
			x < ws.ws_col &&
			x + w > 1
		)
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
	} else addControlDummy(0, 0);
}
