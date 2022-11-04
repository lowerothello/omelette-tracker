/* won't centre properly if multibyte chars are present */
void drawCentreText(short x, short y, short w, const char *text)
{
	x += (w - MIN(w, (short)strlen(text)))>>1;
	w = MIN(w, (short)strlen(text));

	if      (x >= 1) printf("\033[%d;%dH%.*s", y, x, MAX(0, MIN(w, (ws.ws_col+1) - x)), text);
	else if (x > -w) printf("\033[%d;%dH%.*s", y, x, w-x, text-(x-1)); /* x should always be <= 0 */
}

void drawAutogenPluginLine(ControlState *cc, short x, short y, short w,
		short ymin, short ymax,
		const char *name, float *value,
		bool toggled, bool integer,
		float min, float max, float def)
{
	short controloffset = w;
	if (ymin <= y && ymax >= y)
	{
		if (toggled) /* boolean */
		{
			controloffset = w - 3;
			addControlFloat(cc, x + controloffset, y,
					value, CONTROL_NIBBLES_TOGGLED,
					min, max, def, 0, NULL, NULL);
		} else if (min != NAN && max != NAN && min < 0.0f)
		{ /* signed */
			if (integer) /* signed int */
			{
				controloffset = w - 3 - getPreRadixDigits(max);
				addControlFloat(cc, x + controloffset, y,
						value, CONTROL_NIBBLES_SIGNED_INT,
						min, max, def, 0, NULL, NULL);
			} else /* signed float */
			{
				controloffset = w - 10;
				addControlFloat(cc, x + controloffset, y,
						value, CONTROL_NIBBLES_SIGNED_FLOAT,
						min, max, def, 0, NULL, NULL);
			}
		} else
		{ /* unsigned */
			if (integer) /* unsigned int */
			{
				controloffset = w - 2 - getPreRadixDigits(max);
				addControlFloat(cc, x + controloffset, y,
						value, CONTROL_NIBBLES_UNSIGNED_INT,
						min, max, def, 0, NULL, NULL);
			} else /* unsigned float */
			{
				controloffset = w - 9;
				addControlFloat(cc, x + controloffset, y,
						value, CONTROL_NIBBLES_UNSIGNED_FLOAT,
						min, max, def, 0, NULL, NULL);
			}
		}

		controloffset -= 3; /* 2 for [], 1 for the whitespace separator */

		if (x+1 >= 1)
			printf("\033[%d;%dH%.*s", y, x+1, MIN(controloffset, ws.ws_col - x), name);
		else if (x+1 + MIN(controloffset, (int)strlen(name)) > 1)
			printf("\033[%d;%dH%.*s", y, x, controloffset-x, name-x);
	} else addControlInt(cc, 0, 0, NULL, 0, 0, 0, 0, 0, NULL, NULL);
}
