/*
 * value:
 *   - a void pointer, should be cast from an integer type
 *   - the nibbles value should be consistent with the type cast from
 *   - set to NULL for this control to be a dummy (nibbles should be 0 in this case)
 *   - (*value < 0) is a special case, and will print '='
 *
 * nibbles:
 *   sets how many cells wide the control should be (drawn in hex)
 *
 *   input cases:
 *     0:    click/return to toggle, ignores other input
 *     2:    input shifts up, no fieldpointer handling
 *     3:    input shifts up, no fieldpointer handling, +/- to set the sign
 *     else: input sets the cell under fieldpointer and walks fieldpointer
 *   types:
 *     0: (bool)       shows either "X" or " "
 *     1: (int8_t)     shows -1 to 15, reads pretty names, (uint8_t)min should not equal max
 *     2: (uint8_t)    shows 0 to 0xff
 *     3: (int8_t)     shows 0 to 0x3f/0x40 and the sign bit
 *     4: (uint16_t)   shows 0 to 0xffff
 *     5: (int16_t)    shows 0 to 0x7fff/0x8000 and the sign bit, min should be absolute
 *     8: (uint32_t)   shows 0 to 0xffffffff
 *     else: undefined
 */
typedef struct
{
	short    x, y; /* position on the screen */
	void    *value;
	uint32_t min, max;
	uint32_t def;
	int8_t   nibbles;
	uint8_t  prettynamelen;
	char    *prettyname[16]; /* only actually read if (nibbles == 1) */
	int8_t   prettynameptr;

	void   (*callback)(void *cc); /* called when self->value is changed */
	void    *callbackarg;
} Control;

typedef struct
{
	uint8_t cursor;
	signed char fieldpointer;

	uint8_t controlc; /* how many controls are assigned */
	Control control[256];

	bool mouseadjust;
	bool keyadjust;
	bool resetadjust;
	short prevmousex;
} ControlState;


uint32_t _pow32(uint32_t a, uint32_t b)
{
	if (!b) return 1;
	uint32_t c = a;
	for (uint32_t i = 1; i < b; i++)
		c *= a;
	return c;
}


void clearControls(ControlState *cc)
{
	Control *c;
	for (int i = 0; i < cc->controlc; i++)
	{
		c = &cc->control[i];
		if (!c->value) continue;

		for (int j = 0; j < c->prettynameptr; j++)
		{
			if (c->prettyname[j]) free(c->prettyname[j]);
			c->prettyname[j] = NULL;
		}
		c->value = NULL;
		c->callback = NULL;
	}
	cc->controlc = 0;
}

void addControl(ControlState *cc,
		short x, short y,
		void *value, int8_t nibbles,
		uint32_t min, uint32_t max, uint32_t def,
		uint8_t prettynamelen,
		void (*callback)(void *), void *callbackarg)
{
	cc->control[cc->controlc].x = x;
	cc->control[cc->controlc].y = y;
	cc->control[cc->controlc].value = value;
	cc->control[cc->controlc].min = min;
	cc->control[cc->controlc].max = max;
	cc->control[cc->controlc].def = def;
	cc->control[cc->controlc].nibbles = nibbles;
	cc->control[cc->controlc].prettynamelen = prettynamelen;
	cc->control[cc->controlc].prettynameptr = 0;
	cc->control[cc->controlc].callback = callback;
	cc->control[cc->controlc].callbackarg = callbackarg;

	cc->controlc++;
}
/* applies retroactively to the previously registered control */
void setControlPrettyName(ControlState *cc, char *prettyname)
{
	Control *c = &cc->control[cc->controlc-1];
	c->prettyname[c->prettynameptr] = malloc(sizeof(char) * c->prettynamelen+1);
	strcpy(c->prettyname[c->prettynameptr], prettyname);
	c->prettynameptr++;
}

/*
 * dump state to the screen
 * leaves the cursor over the selected control
 */
void drawControls(ControlState *cc)
{
	Control *c;
	for (int i = 0; i < cc->controlc; i++)
	{
		c = &cc->control[i];
		if (!c->value) continue;

		printf("\033[%d;%dH", c->y, c->x);
		if (i == cc->cursor)
		{
			if (cc->mouseadjust || cc->keyadjust) printf("\033[1m");
			else if (cc->resetadjust)             printf("\033[34m");
		}

		switch (c->nibbles)
		{
			case 0:
				if (*(bool *)(c->value)) printf("X"); // Y
				else                     printf(" "); // N
				break;
			case 1:
				if (*(int8_t *)(c->value) < 0)                     printf("=");
				else if (*(int8_t *)(c->value) < c->prettynameptr) printf("%s", c->prettyname[*(int8_t *)(c->value)]);
				else                                               printf("%x", *(int8_t *)(c->value));
				break;
			case 2: printf("%02x", *(uint8_t *)(c->value)); break;
			case 3:
				if (*(int8_t *)(c->value) < 0) printf("-%02x", (short)(*(int8_t *)(c->value)) * -1);
				else                           printf("+%02x", *(int8_t *)(c->value));
				break;
			case 4: printf("%04x", *(uint16_t *)(c->value)); break;
			case 5:
				if (*(int16_t *)(c->value) < 0) printf("-%04x", (int)(*(int16_t *)(c->value)) * -1);
				else                            printf("+%04x", *(int16_t *)(c->value));
				break;
			case 8: printf("%08x", *(uint32_t *)(c->value)); break;
		} printf("\033[m");
	}
	if (cc->cursor < cc->controlc)
	{
		c = &cc->control[cc->cursor];
		if (c->nibbles) printf("\033[%d;%dH", c->y, c->x + c->nibbles - 1 - cc->fieldpointer + (MAX(1, c->prettynamelen)-1));
		else            printf("\033[%d;%dH", c->y, c->x);
	}
}

void incControlValue(ControlState *cc)
{
	Control *c = &cc->control[cc->cursor];
	if (!c->value || !c->nibbles) return;

	uint32_t delta = _pow32(16, cc->fieldpointer);
	switch (c->nibbles)
	{
		case 1:
			if ((*(int8_t *)(c->value)) == (int)c->max) break;
			if ((*(int8_t *)(c->value)) == (int)c->min) /* hack */  (*(int8_t *)(c->value)) += delta;
			else if ((*(int8_t *)(c->value)) + delta > (int)c->max) (*(int8_t *)(c->value)) = (int)c->max;
			else                                                    (*(int8_t *)(c->value)) += delta;
			break;
		case 2:
			if ((*(uint8_t *)(c->value)) + delta > c->max) (*(uint8_t *)(c->value)) = c->max;
			else                                           (*(uint8_t *)(c->value)) += delta;
			break;
		case 3: (*(int8_t *)(c->value)) = MIN((*(int8_t *)(c->value)) + (int)delta, (int)c->max); break;
		case 4:
			if ((*(uint16_t *)(c->value)) + delta > c->max) (*(uint16_t *)(c->value)) = c->max;
			else                                            (*(uint16_t *)(c->value)) += delta;
			break;
		case 5:
			if (*(int16_t *)(c->value) > 0)
			{
				if ((*(int16_t *)(c->value)) + delta > SHRT_MAX) (*(int16_t *)(c->value)) = SHRT_MAX;
				else
				{
					(*(int16_t *)(c->value)) += delta;
					while (((*(int16_t *)(c->value)) & 0x000f) > (c->max & 0x000f) && (int)(*(int16_t *)(c->value) + 0x0001) < SHRT_MAX) (*(int16_t *)(c->value)) += 0x0001;
					while (((*(int16_t *)(c->value)) & 0x00f0) > (c->max & 0x00f0) && (int)(*(int16_t *)(c->value) + 0x0010) < SHRT_MAX) (*(int16_t *)(c->value)) += 0x0010;
					while (((*(int16_t *)(c->value)) & 0x0f00) > (c->max & 0x0f00) && (int)(*(int16_t *)(c->value) + 0x0100) < SHRT_MAX) (*(int16_t *)(c->value)) += 0x0100;
					while (((*(int16_t *)(c->value)) & 0xf000) > (c->max & 0xf000)) (*(int16_t *)(c->value)) -= 0x1000;
				}
			} else
			{
				(*(int16_t *)(c->value)) += delta;
				while ((abs(*(int16_t *)(c->value)) & 0x000f) > (c->min & 0x000f)) (*(int16_t *)(c->value)) += 0x0001;
				while ((abs(*(int16_t *)(c->value)) & 0x00f0) > (c->min & 0x00f0)) (*(int16_t *)(c->value)) += 0x0010;
				while ((abs(*(int16_t *)(c->value)) & 0x0f00) > (c->min & 0x0f00)) (*(int16_t *)(c->value)) += 0x0100;
				while ((abs(*(int16_t *)(c->value)) & 0xf000) > (c->min & 0xf000)) (*(int16_t *)(c->value)) += 0x1000;
			}
			break;
		case 8:
			if ((*(uint32_t *)(c->value)) + delta > c->max) (*(uint32_t *)(c->value)) = c->max;
			else                                            (*(uint32_t *)(c->value)) += delta;
			break;
	}
	if (c->callback) c->callback(c->callbackarg);
}
void decControlValue(ControlState *cc)
{
	Control *c = &cc->control[cc->cursor];
	if (!c->value || !c->nibbles) return;

	uint32_t delta = _pow32(16, cc->fieldpointer);
	switch (c->nibbles)
	{
		case 1:
			if ((*(int8_t *)(c->value)) == (int)c->min) break;
			if ((int)c->min + delta < (*(int8_t *)(c->value))) (*(int8_t *)(c->value)) -= delta;
			else                                               (*(int8_t *)(c->value)) = (int)c->min;
			break;
		case 2:
			if (c->min + delta < (*(uint8_t *)(c->value))) (*(uint8_t *)(c->value)) -= delta;
			else                                           (*(uint8_t *)(c->value)) = c->min;
			break;
		case 3: (*(int8_t *)(c->value)) = MAX((*(int8_t *)(c->value)) - (int)delta, (int)c->min); break;
		case 4:
			if (c->min + delta < (*(uint16_t *)(c->value))) (*(uint16_t *)(c->value)) -= delta;
			else                                            (*(uint16_t *)(c->value)) = c->min;
			break;
		case 5:
			if (*(int16_t *)(c->value) < 0)
			{
				if (SHRT_MIN + delta < (*(int16_t *)(c->value)))
				{
					(*(int16_t *)(c->value)) -= delta;
					while ((abs(*(int16_t *)(c->value)) & 0x000f) > (c->min & 0x000f) && (int)(*(int16_t *)(c->value)) - 0x0001 > SHRT_MIN) (*(int16_t *)(c->value)) -= 0x0001;
					while ((abs(*(int16_t *)(c->value)) & 0x00f0) > (c->min & 0x00f0) && (int)(*(int16_t *)(c->value)) - 0x0010 > SHRT_MIN) (*(int16_t *)(c->value)) -= 0x0010;
					while ((abs(*(int16_t *)(c->value)) & 0x0f00) > (c->min & 0x0f00) && (int)(*(int16_t *)(c->value)) - 0x0100 > SHRT_MIN) (*(int16_t *)(c->value)) -= 0x0100;
					while ((abs(*(int16_t *)(c->value)) & 0xf000) > (c->min & 0xf000)) (*(int16_t *)(c->value)) += 0x1000;
				} else (*(int16_t *)(c->value)) = SHRT_MIN;
			} else
			{
				(*(int16_t *)(c->value)) -= delta;
				if (*(int16_t *)(c->value) > 0)
				{
					while (((*(int16_t *)(c->value)) & 0x000f) > (c->max & 0x000f)) (*(int16_t *)(c->value)) -= 0x0001;
					while (((*(int16_t *)(c->value)) & 0x00f0) > (c->max & 0x00f0)) (*(int16_t *)(c->value)) -= 0x0010;
					while (((*(int16_t *)(c->value)) & 0x0f00) > (c->max & 0x0f00)) (*(int16_t *)(c->value)) -= 0x0100;
					while (((*(int16_t *)(c->value)) & 0xf000) > (c->max & 0xf000)) (*(int16_t *)(c->value)) -= 0x1000;
				}
			}
			break;
		case 8:
			if (c->min + delta < (*(uint32_t *)(c->value))) (*(uint32_t *)(c->value)) -= delta;
			else                                            (*(uint32_t *)(c->value)) = c->min;
			break;
	}
	if (c->callback) c->callback(c->callbackarg);
}

void incControlCursor(ControlState *cc, uint8_t count)
{
	if (cc->keyadjust || cc->mouseadjust)
		decControlValue(cc);
	else
	{
		if (cc->cursor + count < cc->controlc)
			cc->cursor += count;
		cc->fieldpointer = 0;
	}
}
void decControlCursor(ControlState *cc, uint8_t count)
{
	if (cc->keyadjust || cc->mouseadjust)
		incControlValue(cc);
	else
	{
		if (cc->cursor > count - 1)
			cc->cursor -= count;
		cc->fieldpointer = 0;
	}
}
void setControlCursor(ControlState *cc, uint8_t newcursor)
{
	cc->cursor = newcursor;
	cc->fieldpointer = 0;
}

void incControlFieldpointer(ControlState *cc)
{
	Control *c = &cc->control[cc->cursor];
	switch (c->nibbles)
	{
		case 0: case 1: cc->fieldpointer = 0; break;
		case 2: case 3: cc->fieldpointer = 1; break;
		case 4: case 5: cc->fieldpointer++; if (cc->fieldpointer > 3) cc->fieldpointer = 0; break;
		case 8: cc->fieldpointer++; if (cc->fieldpointer > 7) cc->fieldpointer = 0; break;
	}
}
void decControlFieldpointer(ControlState *cc)
{
	Control *c = &cc->control[cc->cursor];
	switch (c->nibbles)
	{
		case 0: case 1: cc->fieldpointer = 0; break;
		case 2: case 3: cc->fieldpointer = 0; break;
		case 4: case 5: cc->fieldpointer--; if (cc->fieldpointer < 0) cc->fieldpointer = 3; break;
		case 8: cc->fieldpointer--; if (cc->fieldpointer < 0) cc->fieldpointer = 7; break;
	}
}

void hexControlValue(ControlState *cc, char value)
{
	Control *c = &cc->control[cc->cursor];
	if (!c->value) return;

	switch (c->nibbles)
	{
		case 0: (*(bool *)(c->value)) = value; break;
		case 1:
			(*(int8_t *)(c->value)) = value;
			(*(int8_t *)(c->value)) = MIN((int8_t)c->max, *(int8_t *)(c->value));
			(*(int8_t *)(c->value)) = MAX((int8_t)c->min, *(int8_t *)(c->value));
			break;
		case 2:
			(*(uint8_t *)(c->value)) <<= 4;
			(*(uint8_t *)(c->value)) += value;
			(*(uint8_t *)(c->value)) = MIN(c->max, *(uint8_t *)(c->value));
			(*(uint8_t *)(c->value)) = MAX(c->min, *(uint8_t *)(c->value));
			break;
		case 3:
			if (*(int8_t *)(c->value) < 0)
			{
				(*(int8_t *)(c->value)) <<= 4;
				(*(int8_t *)(c->value)) -= value;
			} else
			{
				(*(int8_t *)(c->value)) <<= 4;
				(*(int8_t *)(c->value)) += value;
			}
			(*(int8_t *)(c->value)) = MIN(SCHAR_MAX, *(int8_t *)(c->value));
			(*(int8_t *)(c->value)) = MAX(SCHAR_MIN, *(int8_t *)(c->value));
			break;
		case 4:
			(*(uint16_t *)(c->value)) -= _pow32(16, cc->fieldpointer) * ((*(uint16_t *)(c->value)) / _pow32(16, cc->fieldpointer)%16);
			(*(uint16_t *)(c->value)) += _pow32(16, cc->fieldpointer) * value;
			(*(uint16_t *)(c->value)) = MIN(c->max, *(uint16_t *)(c->value));
			(*(uint16_t *)(c->value)) = MAX(c->min, *(uint16_t *)(c->value));
			break;
		case 5:
			/* TODO: pretty broken when numbers >= 0x8000 are involved */
			if (*(int16_t *)(c->value) < 0)
			{
				(*(int16_t *)(c->value)) += _pow32(16, cc->fieldpointer) * (((*(int16_t *)(c->value))*-1) / _pow32(16, cc->fieldpointer)%16);
				(*(int16_t *)(c->value)) -= _pow32(16, cc->fieldpointer) * value;

				/* TODO: untested */
				while (((*(int16_t *)(c->value)) & 0x000f) > (c->max & 0x000f) && (int)(*(int16_t *)(c->value) - 0x0001) > SHRT_MIN) (*(int16_t *)(c->value)) -= 0x0001;
				while (((*(int16_t *)(c->value)) & 0x00f0) > (c->max & 0x00f0) && (int)(*(int16_t *)(c->value) - 0x0010) > SHRT_MIN) (*(int16_t *)(c->value)) -= 0x0010;
				while (((*(int16_t *)(c->value)) & 0x0f00) > (c->max & 0x0f00) && (int)(*(int16_t *)(c->value) - 0x0100) > SHRT_MIN) (*(int16_t *)(c->value)) -= 0x0100;
				while (((*(int16_t *)(c->value)) & 0xf000) > (c->max & 0xf000)) (*(int16_t *)(c->value)) += 0x1000;
			} else
			{
				(*(int16_t *)(c->value)) -= _pow32(16, cc->fieldpointer) * ((*(int16_t *)(c->value)) / _pow32(16, cc->fieldpointer)%16);
				(*(int16_t *)(c->value)) += _pow32(16, cc->fieldpointer) * value;

				/* TODO: untested */
				while (((*(int16_t *)(c->value)) & 0x000f) > (c->max & 0x000f) && (int)(*(int16_t *)(c->value) + 0x0001) < SHRT_MAX) (*(int16_t *)(c->value)) += 0x0001;
				while (((*(int16_t *)(c->value)) & 0x00f0) > (c->max & 0x00f0) && (int)(*(int16_t *)(c->value) + 0x0010) < SHRT_MAX) (*(int16_t *)(c->value)) += 0x0010;
				while (((*(int16_t *)(c->value)) & 0x0f00) > (c->max & 0x0f00) && (int)(*(int16_t *)(c->value) + 0x0100) < SHRT_MAX) (*(int16_t *)(c->value)) += 0x0100;
				while (((*(int16_t *)(c->value)) & 0xf000) > (c->max & 0xf000)) (*(int16_t *)(c->value)) -= 0x1000;
			}
			(*(int16_t *)(c->value)) = MIN(SHRT_MAX, *(int16_t *)(c->value));
			(*(int16_t *)(c->value)) = MAX(SHRT_MIN, *(int16_t *)(c->value));
			break;
		case 8:
			(*(uint32_t *)(c->value)) -= _pow32(16, cc->fieldpointer) * ((*(uint32_t *)(c->value)) / _pow32(16, cc->fieldpointer)%16);
			(*(uint32_t *)(c->value)) += _pow32(16, cc->fieldpointer) * value;
			(*(uint32_t *)(c->value)) = MIN(c->max, *(uint32_t *)(c->value));
			(*(uint32_t *)(c->value)) = MAX(c->min, *(uint32_t *)(c->value));
			break;
	}
	if (c->callback) c->callback(c->callbackarg);
	decControlFieldpointer(cc);
}
void toggleKeyControl(ControlState *cc)
{
	Control *c = &cc->control[cc->cursor];
	if (!c->value) return;

	if (!c->nibbles)
	{
		(*(bool *)(c->value)) = !(*(bool *)(c->value));
		if (c->callback) c->callback(c->callbackarg);
	} else
		cc->keyadjust = !cc->keyadjust;
}
void revertKeyControl(ControlState *cc)
{
	Control *c = &cc->control[cc->cursor];
	if (!c->value) return;

	switch (c->nibbles)
	{
		case 0: (*(bool *)(c->value)) = c->def; break;
		case 1: (*(int8_t *)(c->value)) = c->def; break;
		case 2: (*(uint8_t *)(c->value)) = c->def; break;
		case 3: (*(int8_t *)(c->value)) = c->def; break;
		case 4: (*(uint16_t *)(c->value)) = c->def; break;
		case 5: (*(int16_t *)(c->value)) = c->def; break;
		case 8: (*(uint32_t *)(c->value)) = c->def; break;
	}
	if (c->callback) c->callback(c->callbackarg);
}

void mouseControls(ControlState *cc, int button, int x, int y)
{
	Control *c;
	int i;
	switch (button)
	{
		case BUTTON_RELEASE: case BUTTON_RELEASE_CTRL:
			if (cc->resetadjust)
			{
				cc->resetadjust = 0;
				revertKeyControl(cc);
			}
			if (cc->mouseadjust)
			{
				cc->mouseadjust = 0;
				cc->fieldpointer = 0;
				c = &cc->control[cc->cursor];
				if (c->value && !c->nibbles)
				{
					(*(bool *)(c->value)) = !(*(bool *)(c->value));
					if (c->callback) c->callback(c->callbackarg);
				}
			} break;
		case BUTTON1_HOLD: case BUTTON1_HOLD_CTRL:
			if (cc->mouseadjust)
			{
				if      (x > cc->prevmousex) incControlValue(cc);
				else if (x < cc->prevmousex) decControlValue(cc);
				cc->prevmousex = x;
			} break;
		case BUTTON1: case BUTTON1_CTRL:
			for (i = 0; i < cc->controlc; i++)
			{
				c = &cc->control[i];
				if (y == c->y && x >= c->x -1 && x <= c->x + MAX(1, c->nibbles) + (MAX(1, c->prettynamelen)-1))
				{
					cc->cursor = i;
					cc->prevmousex = x;
					cc->mouseadjust = 1;
					switch (c->nibbles)
					{
						case 0: cc->fieldpointer = 1; break;
						case 3: case 5: /* use c->nibbles-1 */
							if      (x < c->x+1)             cc->fieldpointer = c->nibbles-2;
							else if (x >= c->x + c->nibbles) cc->fieldpointer = 0;
							else                             cc->fieldpointer = (c->nibbles-2) - (x - (c->x+1));
							break;
						default:
							if      (x < c->x)               cc->fieldpointer = c->nibbles-1;
							else if (x >= c->x + c->nibbles) cc->fieldpointer = 0;
							else                             cc->fieldpointer = (c->nibbles-1) - (x - c->x);
							break;
					} break;
				}
			} break;
		case BUTTON2: case BUTTON2_CTRL:
			for (i = 0; i < cc->controlc; i++)
			{
				c = &cc->control[i];
				if (y == c->y && x >= c->x -1 && x <= c->x + MAX(1, c->nibbles) + (MAX(1, c->prettynamelen)-1))
				{
					cc->cursor = i;
					cc->resetadjust = 1;
				}
			} break;
	}
}
