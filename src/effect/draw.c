/* won't centre properly if multibyte chars are present */
void drawCentreText(short x, short y, short w, const char *text)
{
	x += (w - MIN(w, (short)strlen(text)))>>1;
	w = MIN(w, (short)strlen(text));

	printCulling(text, x, y, 1, MIN(ws.ws_col, x + w));
}

#ifndef OMELETTE_EFFECT_NO_STRUCTS
static short getEffectHeight(Effect *e)
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

	if (ymin <= y-1 && ymax >= y-1)
	{
		addControlInt(x + width - 8, y-1, &e->bypass,    0,    0,   1, 0, 0, 0, NULL, NULL);
		addControlInt(x + width - 5, y-1, &e->inputgain, 3, -127, 127, 0, 0, 0, NULL, NULL);
	} else
	{
		addControlDummy(0, 0);
		addControlDummy(0, 0);
	}

	if (effect_api[e->type].draw)
		effect_api[e->type].draw(e->state, x, width, y, ymin, ymax);

	printf("\033[m");

	return ret;
}

static void drawEffectChain(uint8_t track, EffectChain *chain, short x, short width, short y)
{
	short focusindex;
	if (track == w->track) focusindex = getEffectFromCursor(track, chain, cc.cursor);
	else                   focusindex = -1;

	short bottom = (ws.ws_row - 4) - chain->sendc;
	short origwidth = width;

	if (chain->c)
	{
		short height = 0;
		short focusheight = 0;
		for (uint8_t i = 0; i < chain->c; i++)
		{
			height += getEffectHeight(&chain->v[i]);
			if (i == focusindex)
				focusheight = height;
		}

		short ty = y;
		if (bottom - height < y)
		{
			size_t chaincontrolc = 0;
			for (int i = 0; i < chain->c; i++)
				chaincontrolc += getEffectControlCount(chain, i);

			width--;
			if (focusindex > -1)
			{
				drawVerticalScrollbar(x + width + 1, y, bottom - y, chaincontrolc, MIN(chaincontrolc, cc.cursor - getCursorFromEffectTrack(track)));
				ty = MIN(y, bottom - focusheight);
			} else
				drawVerticalScrollbar(x + width + 1, y, bottom - y, chaincontrolc, 0);
		}

		for (uint8_t i = 0; i < chain->c; i++)
			ty += _drawEffect(&chain->v[i],
					focusindex == i, x, width,
					ty+1, y, bottom);
	} else
	{
		if (focusindex > -1) printf("\033[1;31m");
		drawBoundingBox(x, y, width, NULL_EFFECT_HEIGHT-1, 1, ws.ws_col, 1, bottom);
		printf("\033[m");

		addControlDummy(x + width - 8, y);

		printCulling(NULL_EFFECT_TEXT, x+((width - (short)strlen(NULL_EFFECT_TEXT))>>1), y+1, 1, ws.ws_col);
	}

	drawHorizontalLine(x, bottom+1, origwidth-1, 1, ws.ws_col, 1, ws.ws_row);

	for (uint8_t i = 0; i < chain->sendc; i++)
	{
		printCulling("TRACK      ->  ", x + 4, (ws.ws_row - 3) - i, 1, ws.ws_col);
		addControlInt(x + 10, bottom+2 + i, &chain->sendv[i].target,    2,    0, s->track->c - 1, 0, 0, 0, NULL, NULL);
		addControlInt(x + 20, bottom+2 + i, &chain->sendv[i].inputgain, 3, -128,             127, 0, 0, 0, NULL, NULL);
	}

	printCulling("MASTER  >  ", x + 4, ws.ws_row - 2, 1, ws.ws_col);
	addControlInt(x + 16, ws.ws_row - 2, &chain->panning, 3, -127, 127,   0, 0, 0, NULL, NULL);
	addControlInt(x + 21, ws.ws_row - 2, &chain->volume,  2,    0, 255, 255, 0, 0, NULL, NULL);
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

void drawEffect(void)
{
	
	if (cc.mouseadjust || cc.keyadjust)
	{
		printf("\033[%d;0H\033[1m-- ADJUST --\033[m", ws.ws_row);
		w->repl.error[0] = '\0';
	}

	clearControls();
	short x = genConstSfx(EFFECT_WIDTH, ws.ws_col);
	for (uint8_t i = 0; i < s->track->c; i++)
	{
		drawTrackHeader(i, x+1, EFFECT_WIDTH,
				1, ws.ws_col);

		drawEffectChain(i, s->track->v[i]->effect,
				x + 2,
				EFFECT_WIDTH - 2,
				TRACK_ROW + 2);

		x += EFFECT_WIDTH;
	}
	drawControls();
}
