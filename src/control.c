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
 *   cases:
 *     0: boolean control, value should be a bool
 *     2: input shifts up, no fieldpointer handling
 *     else: input sets the cell under fieldpointer and walks fieldpointer
 *   types:
 *     0: (bool)
 *     1: (int8_t)
 *     2: (uint8_t)
 *     4: (uint16_t)
 *     8: (uint32_t)
 *     else: undefined
 */
typedef struct
{
	short    x, y; /* position on the screen */
	void    *value;
	uint32_t min, max;
	int8_t   nibbles;
	uint8_t  prettynamelen;
	char    *prettyname[16]; /* only actually read if (nibbles == 1) */
	int8_t   prettynameptr;
} Control;

typedef struct
{
	uint8_t cursor;
	signed char fieldpointer;

	uint8_t controlc; /* how many controls are assigned */
	Control control[256];

	bool mouseadjust;
	bool keyadjust;
	short prevmousex;
} ControlState;


uint32_t _pow32(uint32_t a, uint32_t b)
{
	if (!b) return 1;
	uint32_t c = a;
	while (b)
	{
		b--;
		c *= a;
	}
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
	} cc->controlc = 0;
}

void addControl(ControlState *cc,
		short x, short y,
		void *value, int8_t nibbles,
		uint32_t min, uint32_t max,
		uint8_t prettynamelen)
{
	cc->control[cc->controlc].x = x;
	cc->control[cc->controlc].y = y;
	cc->control[cc->controlc].value = value;
	cc->control[cc->controlc].min = min;
	cc->control[cc->controlc].max = max;
	cc->control[cc->controlc].nibbles = nibbles;
	cc->control[cc->controlc].prettynamelen = prettynamelen;
	cc->control[cc->controlc].prettynameptr = 0;

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
			case 4: printf("%04x", *(uint16_t *)(c->value)); break;
			case 8: printf("%08x", *(uint32_t *)(c->value)); break;
		}
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
			if ((*(int8_t *)(c->value)) == c->max) break;
			if ((*(int8_t *)(c->value)) == c->min) /* hack */  (*(int8_t *)(c->value)) += delta;
			else if ((*(int8_t *)(c->value)) + delta > c->max) (*(int8_t *)(c->value)) = c->max;
			else                                               (*(int8_t *)(c->value)) += delta;
			break;
		case 2:
			if ((*(uint8_t *)(c->value)) + delta > c->max) (*(uint8_t *)(c->value)) = c->max;
			else                                           (*(uint8_t *)(c->value)) += delta;
			break;
		case 4:
			if ((*(uint16_t *)(c->value)) + delta > c->max) (*(uint16_t *)(c->value)) = c->max;
			else                                            (*(uint16_t *)(c->value)) += delta;
			break;
		case 8:
			if ((*(uint32_t *)(c->value)) + delta > c->max) (*(uint32_t *)(c->value)) = c->max;
			else                                            (*(uint32_t *)(c->value)) += delta;
			break;
	}
}
void decControlValue(ControlState *cc)
{
	Control *c = &cc->control[cc->cursor];
	if (!c->value || !c->nibbles) return;

	uint32_t delta = _pow32(16, cc->fieldpointer);
	switch (c->nibbles)
	{
		case 1:
			if ((*(int8_t *)(c->value)) == c->min) break;
			if (c->min + delta < (*(int8_t *)(c->value))) (*(int8_t *)(c->value)) -= delta;
			else                                          (*(int8_t *)(c->value)) = c->min;
			break;
		case 2:
			if (c->min + delta < (*(uint8_t *)(c->value))) (*(uint8_t *)(c->value)) -= delta;
			else                                           (*(uint8_t *)(c->value)) = c->min;
			break;
		case 4:
			if (c->min + delta < (*(uint16_t *)(c->value))) (*(uint16_t *)(c->value)) -= delta;
			else                                            (*(uint16_t *)(c->value)) = c->min;
			break;
		case 8:
			if (c->min + delta < (*(uint32_t *)(c->value))) (*(uint32_t *)(c->value)) -= delta;
			else                                            (*(uint32_t *)(c->value)) = c->min;
			break;
	}
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
		case 2: cc->fieldpointer = 1; break;
		case 4: cc->fieldpointer++; if (w->fieldpointer > 3) w->fieldpointer = 0; break;
		case 8: cc->fieldpointer++; if (w->fieldpointer > 7) w->fieldpointer = 0; break;
	}
}
void decControlFieldpointer(ControlState *cc)
{
	Control *c = &cc->control[cc->cursor];
	switch (c->nibbles)
	{
		case 0: case 1: cc->fieldpointer = 0; break;
		case 2: cc->fieldpointer = 0; break;
		case 4: cc->fieldpointer--; if (w->fieldpointer < 0) w->fieldpointer = 3; break;
		case 8: cc->fieldpointer--; if (w->fieldpointer < 0) w->fieldpointer = 7; break;
	}
}

void hexControlValue(ControlState *cc, char value)
{
	Control *c = &cc->control[cc->cursor];
	if (!c->value) return;

	switch (c->nibbles)
	{
		case 0: (*(bool *)(c->value)) = value; break;
		case 1: (*(int8_t *)(c->value)) = value; break;
		case 2:
			(*(uint8_t *)(c->value)) <<= 4;
			(*(uint8_t *)(c->value)) += value;
			break;
		case 4:
			(*(uint16_t *)(c->value)) -= _pow32(16, cc->fieldpointer) * ((*(uint16_t *)(c->value)) / _pow32(16, cc->fieldpointer)%16);
			(*(uint16_t *)(c->value)) += _pow32(16, cc->fieldpointer) * value;
			break;
		case 8:
			(*(uint32_t *)(c->value)) -= _pow32(16, cc->fieldpointer) * ((*(uint32_t *)(c->value)) / _pow32(16, cc->fieldpointer)%16);
			(*(uint32_t *)(c->value)) += _pow32(16, cc->fieldpointer) * value;
			break;
	}
	decControlFieldpointer(cc);
}
void toggleKeyControl(ControlState *cc)
{
	Control *c = &cc->control[cc->cursor];
	if (!c->value) return;

	if (!c->nibbles) (*(bool *)(c->value)) = !(*(bool *)(c->value));
	else cc->keyadjust = !cc->keyadjust;
}

void mouseControls(ControlState *cc, int button, int x, int y)
{
	Control *c;
	int i;
	switch (button)
	{
		case BUTTON_RELEASE: case BUTTON_RELEASE_CTRL:
			cc->mouseadjust = 0;
			cc->fieldpointer = 0;
			c = &cc->control[cc->cursor];
			if (c->value && !c->nibbles) (*(bool *)(c->value)) = !(*(bool *)(c->value));
			break;
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
					if (x < c->x)                            cc->fieldpointer = MAX(1, c->nibbles)-1;
					else if (x >= c->x + MAX(1, c->nibbles)) cc->fieldpointer = 0;
					else                                     cc->fieldpointer = (MAX(1, c->nibbles)-1) - (x - c->x);
					cc->prevmousex = x;
					cc->mouseadjust = 1;
					break;
				}
			} break;
	}
}
