enum {
	CONTROL_NIBBLES_BOOL = 0,   /* shows either "X" or " ", click/return to toggle (ignores other input) */
	CONTROL_NIBBLES_PRETTY = 1, /* shows -1 to 15, reads pretty names */
	CONTROL_NIBBLES_UINT8 = 2,  /* shows 0 to 0xff */
	CONTROL_NIBBLES_INT8 = 3,   /* shows 0 to 0x3f/0x40 and the sign bit */
	CONTROL_NIBBLES_UINT16 = 4, /* shows 0 to 0xffff */
	CONTROL_NIBBLES_INT16 = 5,  /* shows 0 to 0x7fff/0x8000 and the sign bit, min and max are nibble-wise, min should be absolute */
	CONTROL_NIBBLES_UINT32 = 8, /* shows 0 to 0xffffffff */
	CONTROL_NIBBLES_UNSIGNED_FLOAT,
	CONTROL_NIBBLES_SIGNED_FLOAT,
	CONTROL_NIBBLES_UNSIGNED_INT,
	CONTROL_NIBBLES_SIGNED_INT,
	CONTROL_NIBBLES_TOGGLED,
} CONTROL_NIBBLES;

typedef union {
	float    f;
	uint32_t i;
} ControlRange;

typedef struct {
	ControlRange value;
	char        *label;
} ScalePoint;

typedef struct
{
	short        x, y; /* position on the screen */
	void        *value;
	ControlRange min, max, def;
	int8_t       nibbles;
	uint32_t     scalepointlen;
	ScalePoint  *scalepoint;
	uint32_t     scalepointptr;
	uint32_t     scalepointcount;

	void       (*callback)(void *arg); /* called when self->value is changed */
	void        *callbackarg;
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
ControlState cc;


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

		for (int j = 0; j < c->scalepointptr; j++)
			if (c->scalepoint[j].label)
				free(c->scalepoint[j].label);

		if (c->scalepoint) free(c->scalepoint);
		c->scalepoint = NULL;
		c->value = NULL;
		c->callback = NULL;
	}
	cc->controlc = 0;
}

/* min/max/def/nibbles should already be set */
static void _addControl(ControlState *cc, short x, short y,
		void *value, uint32_t scalepointlen, uint32_t scalepointcount,
		void (*callback)(void *), void *callbackarg)
{
	cc->control[cc->controlc].x = x;
	cc->control[cc->controlc].y = y;
	cc->control[cc->controlc].value = value;
	cc->control[cc->controlc].scalepointlen = scalepointlen;
	cc->control[cc->controlc].scalepointptr = 0;
	cc->control[cc->controlc].scalepointcount = scalepointcount;
	if (scalepointcount) cc->control[cc->controlc].scalepoint = calloc(scalepointcount, sizeof(ScalePoint));
	cc->control[cc->controlc].callback = callback;
	cc->control[cc->controlc].callbackarg = callbackarg;

	cc->controlc++;
}
void addControlInt(ControlState *cc, short x, short y, void *value, int8_t nibbles,
		uint32_t min, uint32_t max, uint32_t def,
		uint32_t scalepointlen, uint32_t scalepointcount,
		void (*callback)(void *), void *callbackarg)
{
	cc->control[cc->controlc].min.i = min;
	cc->control[cc->controlc].max.i = max;
	cc->control[cc->controlc].def.i = def;
	cc->control[cc->controlc].nibbles = nibbles;

	_addControl(cc, x, y, value, scalepointlen, scalepointcount, callback, callbackarg);
}
void addControlFloat(ControlState *cc, short x, short y, void *value, int8_t nibbles,
		float min, float max, float def,
		uint32_t scalepointlen, uint32_t scalepointcount,
		void (*callback)(void *), void *callbackarg)
{
	cc->control[cc->controlc].min.f = min;
	cc->control[cc->controlc].max.f = max;
	cc->control[cc->controlc].def.f = def;
	cc->control[cc->controlc].nibbles = nibbles;

	_addControl(cc, x, y, value, scalepointlen, scalepointcount, callback, callbackarg);
}

void addControlDummy(ControlState *cc, short x, short y)
{
	cc->control[cc->controlc].min.i = 0;
	cc->control[cc->controlc].max.i = 0;
	cc->control[cc->controlc].def.i = 0;
	cc->control[cc->controlc].nibbles = 0;

	_addControl(cc, x, y, NULL, 0, 0, NULL, NULL);
}

/* applies retroactively to the previously registered control */
void addScalePointInt(ControlState *cc, char *label, uint32_t value)
{
	Control *c = &cc->control[cc->controlc-1];
	if (c->scalepointptr < c->scalepointcount)
	{
		c->scalepoint[c->scalepointptr].label = strdup(label);
		c->scalepoint[c->scalepointptr].value.i = value;
		c->scalepointptr++;
	}
}
void addScalePointFloat(ControlState *cc, char *label, uint32_t value)
{
	Control *c = &cc->control[cc->controlc-1];
	if (c->scalepointptr < c->scalepointcount)
	{
		c->scalepoint[c->scalepointptr].label = strdup(label);
		c->scalepoint[c->scalepointptr].value.f = value;
		c->scalepointptr++;
	}
}

/* number of digits before the radix, up to 6 are checked for (float range) */
/* don't think there's a more efficient way to do this? there might well be */
int getPreRadixDigits(float x)
{
	if      (x >= 100000.0f) return 6;
	else if (x >= 10000.0f) return 5;
	else if (x >= 1000.0f) return 4;
	else if (x >= 100.0f) return 3;
	else if (x >= 10.0f) return 2;
	else                return 1;
}

short getControlWidth(Control *c)
{
	if (!c->value) return 0;
	if (c->scalepointptr) return c->scalepointlen + 2;
	switch (c->nibbles)
	{
		case 1: case 0: case CONTROL_NIBBLES_TOGGLED: return 3;
		case CONTROL_NIBBLES_UNSIGNED_FLOAT:          return 9;
		case CONTROL_NIBBLES_SIGNED_FLOAT:            return 10;
		case CONTROL_NIBBLES_UNSIGNED_INT:            return getPreRadixDigits(c->max.f) + 2;
		case CONTROL_NIBBLES_SIGNED_INT:              return getPreRadixDigits(c->max.f) + 3;
		default:                                      return c->nibbles + 2;
	}
}

/*
 * dump state to the screen
 * leaves the cursor over the selected control
 * screen-space culling
 */
void drawControls(ControlState *cc)
{
	short cw;
	Control *c;
	char *buffer;
	for (int i = 0; i < cc->controlc; i++)
	{
		c = &cc->control[i];
		if (!c->value) continue; /* dummy control */
		cw = getControlWidth(c);
		if (c->x - 1 + cw < 2)      continue; /* off the left edge */
		if (c->x - 1 > ws.ws_col+1) continue; /* off the right edge */

		if (c->x - 1 > 0) printf("\033[%d;%dH[", c->y, c->x - 1);

		if (i == cc->cursor)
		{
			if (cc->mouseadjust || cc->keyadjust) printf("\033[1m");
			else if (cc->resetadjust)             printf("\033[34m");
		}

		buffer = calloc(cw - 1, sizeof(char)); /* doesn't need to be big enough to hold the leading and trailing square brackets */

		if (c->scalepointptr)
		{
			switch (c->nibbles)
			{
				case CONTROL_NIBBLES_TOGGLED:
				case CONTROL_NIBBLES_UNSIGNED_FLOAT:
				case CONTROL_NIBBLES_SIGNED_FLOAT:
				case CONTROL_NIBBLES_UNSIGNED_INT:
				case CONTROL_NIBBLES_SIGNED_INT:
					for (uint32_t j = 0; j < c->scalepointptr; j++) { if (c->scalepoint[j].value.f == *(float *)c->value) { strcpy(buffer, c->scalepoint[j].label); break; } } break;
				case 1: case 3: for (uint32_t j = 0; j < c->scalepointptr; j++) { if ((int8_t  )c->scalepoint[j].value.i == *(int8_t   *)c->value) { strcpy(buffer, c->scalepoint[j].label); break; } } break;
				case 2:         for (uint32_t j = 0; j < c->scalepointptr; j++) { if ((uint8_t )c->scalepoint[j].value.i == *(uint8_t  *)c->value) { strcpy(buffer, c->scalepoint[j].label); break; } } break;
				case 4:         for (uint32_t j = 0; j < c->scalepointptr; j++) { if ((uint16_t)c->scalepoint[j].value.i == *(uint16_t *)c->value) { strcpy(buffer, c->scalepoint[j].label); break; } } break;
				case 8:         for (uint32_t j = 0; j < c->scalepointptr; j++) { if ((uint32_t)c->scalepoint[j].value.i == *(uint32_t *)c->value) { strcpy(buffer, c->scalepoint[j].label); break; } } break;
				case 5:         for (uint32_t j = 0; j < c->scalepointptr; j++) { if ((int16_t )c->scalepoint[j].value.i == *(int16_t  *)c->value) { strcpy(buffer, c->scalepoint[j].label); break; } } break;
			}
		} else
			switch (c->nibbles)
			{
				case 0:
					if (*(bool *)c->value) strcpy(buffer, "X"); // Y
					else                   strcpy(buffer, " "); // N
					break;
				case CONTROL_NIBBLES_TOGGLED:
					if (*(float *)c->value > 0.0f) strcpy(buffer, "X"); // Y
					else                           strcpy(buffer, " "); // N
					break;
				case 1:
					if (*(int8_t *)c->value < 0) strcpy(buffer, "=");
					else                         sprintf(buffer, "%x", *(int8_t *)c->value);
					break;
				case 2: sprintf(buffer, "%02x", *(uint8_t *)c->value); break;
				case 3:
					if (*(int8_t *)c->value < 0) sprintf(buffer, "-%02x", (short)(*(int8_t *)c->value) * -1);
					else                         sprintf(buffer, "+%02x", *(int8_t *)c->value);
					break;
				case 4: sprintf(buffer, "%04x", *(uint16_t *)c->value); break;
				case 5:
					if (*(int16_t *)c->value < 0) sprintf(buffer, "-%04x", (int)(*(int16_t *)c->value) * -1);
					else                          sprintf(buffer, "+%04x", *(int16_t *)c->value);
					break;
				case 8: sprintf(buffer, "%08x", *(uint32_t *)c->value); break;
				case CONTROL_NIBBLES_UNSIGNED_FLOAT: sprintf(buffer,  "%0*.*f", 7, 6-getPreRadixDigits(c->max.f), *(float *)c->value); break;
				case CONTROL_NIBBLES_SIGNED_FLOAT:   sprintf(buffer, "%+0*.*f", 8, 6-getPreRadixDigits(c->max.f), *(float *)c->value); break; /* TODO: c->min can have more pre-radix digits than c->max */
				case CONTROL_NIBBLES_UNSIGNED_INT:   sprintf(buffer,  "%0*.0f",      getPreRadixDigits(c->max.f), *(float *)c->value); break;
				case CONTROL_NIBBLES_SIGNED_INT:     sprintf(buffer, "%+0*.0f",    1+getPreRadixDigits(c->max.f), *(float *)c->value); break;
			}
		// if (c->x - 1 > 0) printf("\033[%d;%dH[", c->y, c->x - 1);
		if (c->x < 1) { if (c->x > 1 - (cw-2)) printf("\033[%d;%dH%s\033[m", c->y, 1, buffer+(1 - c->x)); }
		else                                   printf("\033[%d;%dH%.*s\033[m", c->y, c->x, (ws.ws_col+1) - c->x, buffer);
		free(buffer);

		if (c->x + (cw-2) < ws.ws_col+1) printf("\033[%d;%dH]", c->y, c->x - 2 + cw);
	}
	if (cc->cursor < cc->controlc)
	{
		c = &cc->control[cc->cursor];
		switch (c->nibbles)
		{
			case 0: case CONTROL_NIBBLES_TOGGLED: printf("\033[%d;%dH", c->y, c->x); break;
			case CONTROL_NIBBLES_UNSIGNED_FLOAT:
				if (cc->fieldpointer < 6-getPreRadixDigits(c->max.f)) printf("\033[%d;%dH", c->y, c->x + 6 - cc->fieldpointer);
				else                                                  printf("\033[%d;%dH", c->y, c->x + 5 - cc->fieldpointer);
				break;
			case CONTROL_NIBBLES_SIGNED_FLOAT:
				if (cc->fieldpointer < 6-getPreRadixDigits(c->max.f)) printf("\033[%d;%dH", c->y, c->x + 7 - cc->fieldpointer);
				else                                                  printf("\033[%d;%dH", c->y, c->x + 6 - cc->fieldpointer);
				break;
			case CONTROL_NIBBLES_UNSIGNED_INT: printf("\033[%d;%dH", c->y, c->x + getPreRadixDigits(c->max.f) - cc->fieldpointer - 1); break;
			case CONTROL_NIBBLES_SIGNED_INT:   printf("\033[%d;%dH", c->y, c->x + getPreRadixDigits(c->max.f) - cc->fieldpointer    ); break;
			default: printf("\033[%d;%dH", c->y, c->x + c->nibbles - 1 - cc->fieldpointer + (MAX(1, c->scalepointlen)-1)); break;
		}
	}
}

void incControlValue(ControlState *cc)
{
	Control *c = &cc->control[cc->cursor];
	if (!c->value || !c->nibbles || c->nibbles == CONTROL_NIBBLES_TOGGLED) return;

	switch (c->nibbles)
	{
		case CONTROL_NIBBLES_UNSIGNED_FLOAT:
			(*(float *)c->value) = MIN((*(float *)c->value) + powf(10.0f, cc->fieldpointer - (6-getPreRadixDigits(c->max.f))), c->max.f); break;
		case CONTROL_NIBBLES_SIGNED_FLOAT:
			(*(float *)c->value) = MIN((*(float *)c->value) + powf(10.0f, cc->fieldpointer - (6-getPreRadixDigits(c->max.f))), c->max.f); break;
		case CONTROL_NIBBLES_UNSIGNED_INT:
		case CONTROL_NIBBLES_SIGNED_INT:
			(*(float *)c->value) = MIN((*(float *)c->value) + powf(10.0f, cc->fieldpointer), c->max.f); break;
		case 1: case 3: (*(int8_t   *)c->value) = MIN((*(int8_t   *)c->value) + (int8_t  )_pow32(16, cc->fieldpointer), (int8_t  )c->max.i); break;
		case 2:         (*(uint8_t  *)c->value) = MIN((*(uint8_t  *)c->value) + (uint8_t )_pow32(16, cc->fieldpointer), (uint8_t )c->max.i); break;
		case 4:         (*(uint16_t *)c->value) = MIN((*(uint16_t *)c->value) + (uint16_t)_pow32(16, cc->fieldpointer), (uint16_t)c->max.i); break;
		case 8:         (*(uint32_t *)c->value) = MIN((*(uint32_t *)c->value) + (uint32_t)_pow32(16, cc->fieldpointer), (uint32_t)c->max.i); break;
		case 5:
			if (*(int16_t *)c->value > 0)
			{
				if ((*(int16_t *)c->value) + _pow32(16, cc->fieldpointer) > SHRT_MAX) (*(int16_t *)c->value) = SHRT_MAX;
				else
				{
					(*(int16_t *)c->value) += _pow32(16, cc->fieldpointer);
					while (((*(int16_t *)c->value) & 0x000f) > ((int16_t)c->max.i & 0x000f) && (int)(*(int16_t *)c->value) + 0x0001 < SHRT_MAX) (*(int16_t *)c->value) += 0x0001;
					while (((*(int16_t *)c->value) & 0x00f0) > ((int16_t)c->max.i & 0x00f0) && (int)(*(int16_t *)c->value) + 0x0010 < SHRT_MAX) (*(int16_t *)c->value) += 0x0010;
					while (((*(int16_t *)c->value) & 0x0f00) > ((int16_t)c->max.i & 0x0f00) && (int)(*(int16_t *)c->value) + 0x0100 < SHRT_MAX) (*(int16_t *)c->value) += 0x0100;
					while (((*(int16_t *)c->value) & 0xf000) > ((int16_t)c->max.i & 0xf000))                                                    (*(int16_t *)c->value) -= 0x1000;
				}
			} else
			{
				(*(int16_t *)c->value) += _pow32(16, cc->fieldpointer);
				while ((abs(*(int16_t *)c->value) & 0x000f) > ((int16_t)c->min.i & 0x000f)) (*(int16_t *)c->value) += 0x0001;
				while ((abs(*(int16_t *)c->value) & 0x00f0) > ((int16_t)c->min.i & 0x00f0)) (*(int16_t *)c->value) += 0x0010;
				while ((abs(*(int16_t *)c->value) & 0x0f00) > ((int16_t)c->min.i & 0x0f00)) (*(int16_t *)c->value) += 0x0100;
				while ((abs(*(int16_t *)c->value) & 0xf000) > ((int16_t)c->min.i & 0xf000)) (*(int16_t *)c->value) += 0x1000;
			}
			break;
	}
	if (c->callback) c->callback(c->callbackarg);
}
void decControlValue(ControlState *cc)
{
	Control *c = &cc->control[cc->cursor];
	if (!c->value || !c->nibbles || c->nibbles == CONTROL_NIBBLES_TOGGLED) return;

	switch (c->nibbles)
	{
		case CONTROL_NIBBLES_UNSIGNED_FLOAT:
			(*(float *)c->value) = MAX((*(float *)c->value) - powf(10.0f, cc->fieldpointer - (6-getPreRadixDigits(c->max.f))), c->min.f); break;
		case CONTROL_NIBBLES_SIGNED_FLOAT:
			(*(float *)c->value) = MAX((*(float *)c->value) - powf(10.0f, cc->fieldpointer - (6-getPreRadixDigits(c->max.f))), c->min.f); break;
		case CONTROL_NIBBLES_UNSIGNED_INT:
		case CONTROL_NIBBLES_SIGNED_INT:
			(*(float *)c->value) = MAX((*(float *)c->value) - powf(10.0f, cc->fieldpointer), c->min.f); break;
		case 1: case 3: (*(int8_t   *)c->value) = MAX((*(int8_t   *)c->value) - (int8_t  )_pow32(16, cc->fieldpointer), (int8_t  )c->min.i); break;
		case 2:         (*(uint8_t  *)c->value) = MAX((*(uint8_t  *)c->value) - (uint8_t )_pow32(16, cc->fieldpointer), (uint8_t )c->min.i); break;
		case 4:         (*(uint16_t *)c->value) = MAX((*(uint16_t *)c->value) - (uint16_t)_pow32(16, cc->fieldpointer), (uint16_t)c->min.i); break;
		case 8:         (*(uint32_t *)c->value) = MAX((*(uint32_t *)c->value) - (uint32_t)_pow32(16, cc->fieldpointer), (uint32_t)c->min.i); break;
		case 5:
			if (*(int16_t *)c->value < 0)
			{
				if (SHRT_MIN + _pow32(16, cc->fieldpointer) < (*(int16_t *)c->value))
				{
					(*(int16_t *)c->value) -= _pow32(16, cc->fieldpointer);
					while ((abs(*(int16_t *)c->value) & 0x000f) > ((int16_t)c->min.i & 0x000f) && (int)(*(int16_t *)c->value) - 0x0001 > SHRT_MIN) (*(int16_t *)c->value) -= 0x0001;
					while ((abs(*(int16_t *)c->value) & 0x00f0) > ((int16_t)c->min.i & 0x00f0) && (int)(*(int16_t *)c->value) - 0x0010 > SHRT_MIN) (*(int16_t *)c->value) -= 0x0010;
					while ((abs(*(int16_t *)c->value) & 0x0f00) > ((int16_t)c->min.i & 0x0f00) && (int)(*(int16_t *)c->value) - 0x0100 > SHRT_MIN) (*(int16_t *)c->value) -= 0x0100;
					while ((abs(*(int16_t *)c->value) & 0xf000) > ((int16_t)c->min.i & 0xf000))                                                    (*(int16_t *)c->value) += 0x1000;
				} else (*(int16_t *)c->value) = SHRT_MIN;
			} else
			{
				(*(int16_t *)c->value) -= _pow32(16, cc->fieldpointer);
				if (*(int16_t *)c->value > 0)
				{
					while (((*(int16_t *)c->value) & 0x000f) > ((int16_t)c->max.i & 0x000f)) (*(int16_t *)c->value) -= 0x0001;
					while (((*(int16_t *)c->value) & 0x00f0) > ((int16_t)c->max.i & 0x00f0)) (*(int16_t *)c->value) -= 0x0010;
					while (((*(int16_t *)c->value) & 0x0f00) > ((int16_t)c->max.i & 0x0f00)) (*(int16_t *)c->value) -= 0x0100;
					while (((*(int16_t *)c->value) & 0xf000) > ((int16_t)c->max.i & 0xf000)) (*(int16_t *)c->value) -= 0x1000;
				}
			}
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
		case 0: case 1: case CONTROL_NIBBLES_TOGGLED: cc->fieldpointer = 0; break;
		case 2: case 3: cc->fieldpointer = 1; break;
		case 4: case 5: cc->fieldpointer++; if (cc->fieldpointer > 3) cc->fieldpointer = 0; break;
		case 8: cc->fieldpointer++; if (cc->fieldpointer > 7) cc->fieldpointer = 0; break;
		case CONTROL_NIBBLES_SIGNED_FLOAT: case CONTROL_NIBBLES_UNSIGNED_FLOAT:
			cc->fieldpointer++; if (cc->fieldpointer > 5) cc->fieldpointer = 0; break;
		case CONTROL_NIBBLES_SIGNED_INT: case CONTROL_NIBBLES_UNSIGNED_INT:
			cc->fieldpointer++; if (cc->fieldpointer > getPreRadixDigits(c->max.f)-1) cc->fieldpointer = 0; break;
	}
}
void decControlFieldpointer(ControlState *cc)
{
	Control *c = &cc->control[cc->cursor];
	switch (c->nibbles)
	{
		case 0: case 1: case CONTROL_NIBBLES_TOGGLED: /* fall through */
		case 2: case 3: cc->fieldpointer = 0; break;
		case 4: case 5: cc->fieldpointer--; if (cc->fieldpointer < 0) cc->fieldpointer = 7; break;
		case 8: cc->fieldpointer--; if (cc->fieldpointer < 0) cc->fieldpointer = 7; break;
		case CONTROL_NIBBLES_SIGNED_FLOAT: case CONTROL_NIBBLES_UNSIGNED_FLOAT:
			cc->fieldpointer--; if (cc->fieldpointer < 0) cc->fieldpointer = 5; break;
		case CONTROL_NIBBLES_SIGNED_INT: case CONTROL_NIBBLES_UNSIGNED_INT:
			cc->fieldpointer--; if (cc->fieldpointer < 0) cc->fieldpointer = getPreRadixDigits(c->max.f)-1; break;
	}
}

void hexControlValue(ControlState *cc, char value)
{
	Control *c = &cc->control[cc->cursor];
	if (!c->value) return;

	switch (c->nibbles)
	{
		case 0: (*(bool *)c->value) = value; break;
		case 1:
			(*(int8_t *)c->value) = value;
			(*(int8_t *)c->value) = MIN((int8_t)c->max.i, *(int8_t *)c->value);
			(*(int8_t *)c->value) = MAX((int8_t)c->min.i, *(int8_t *)c->value);
			break;
		case 2:
			(*(uint8_t *)c->value) <<= 4;
			(*(uint8_t *)c->value) += value;
			(*(uint8_t *)c->value) = MIN((uint8_t)c->max.i, *(uint8_t *)c->value);
			(*(uint8_t *)c->value) = MAX((uint8_t)c->min.i, *(uint8_t *)c->value);
			break;
		case 3:
			if (*(int8_t *)c->value < 0)
			{
				(*(int8_t *)c->value) <<= 4;
				(*(int8_t *)c->value) -= value;
			} else
			{
				(*(int8_t *)c->value) <<= 4;
				(*(int8_t *)c->value) += value;
			}
			(*(int8_t *)c->value) = MIN(SCHAR_MAX, *(int8_t *)c->value);
			(*(int8_t *)c->value) = MAX(SCHAR_MIN, *(int8_t *)c->value);
			break;
		case 4:
			(*(uint16_t *)c->value) -= _pow32(16, cc->fieldpointer) * ((*(uint16_t *)c->value) / _pow32(16, cc->fieldpointer)%16);
			(*(uint16_t *)c->value) += _pow32(16, cc->fieldpointer) * value;
			(*(uint16_t *)c->value) = MIN((uint16_t)c->max.i, *(uint16_t *)c->value);
			(*(uint16_t *)c->value) = MAX((uint16_t)c->min.i, *(uint16_t *)c->value);
			break;
		case 5:
			/* TODO: pretty broken when numbers >= 0x8000 are involved */
			if (*(int16_t *)c->value < 0)
			{
				(*(int16_t *)c->value) += _pow32(16, cc->fieldpointer) * (((*(int16_t *)c->value)*-1) / _pow32(16, cc->fieldpointer)%16);
				(*(int16_t *)c->value) -= _pow32(16, cc->fieldpointer) * value;

				/* TODO: untested */
				while (((*(int16_t *)c->value) & 0x000f) > ((int16_t)c->max.i & 0x000f) && (int)(*(int16_t *)c->value - 0x0001) > SHRT_MIN) (*(int16_t *)c->value) -= 0x0001;
				while (((*(int16_t *)c->value) & 0x00f0) > ((int16_t)c->max.i & 0x00f0) && (int)(*(int16_t *)c->value - 0x0010) > SHRT_MIN) (*(int16_t *)c->value) -= 0x0010;
				while (((*(int16_t *)c->value) & 0x0f00) > ((int16_t)c->max.i & 0x0f00) && (int)(*(int16_t *)c->value - 0x0100) > SHRT_MIN) (*(int16_t *)c->value) -= 0x0100;
				while (((*(int16_t *)c->value) & 0xf000) > ((int16_t)c->max.i & 0xf000))                                                    (*(int16_t *)c->value) += 0x1000;
			} else
			{
				(*(int16_t *)c->value) -= _pow32(16, cc->fieldpointer) * ((*(int16_t *)c->value) / _pow32(16, cc->fieldpointer)%16);
				(*(int16_t *)c->value) += _pow32(16, cc->fieldpointer) * value;

				/* TODO: untested */
				while (((*(int16_t *)c->value) & 0x000f) > ((int16_t)c->max.i & 0x000f) && (int)(*(int16_t *)c->value + 0x0001) < SHRT_MAX) (*(int16_t *)c->value) += 0x0001;
				while (((*(int16_t *)c->value) & 0x00f0) > ((int16_t)c->max.i & 0x00f0) && (int)(*(int16_t *)c->value + 0x0010) < SHRT_MAX) (*(int16_t *)c->value) += 0x0010;
				while (((*(int16_t *)c->value) & 0x0f00) > ((int16_t)c->max.i & 0x0f00) && (int)(*(int16_t *)c->value + 0x0100) < SHRT_MAX) (*(int16_t *)c->value) += 0x0100;
				while (((*(int16_t *)c->value) & 0xf000) > ((int16_t)c->max.i & 0xf000))                                                    (*(int16_t *)c->value) -= 0x1000;
			}
			(*(int16_t *)c->value) = MIN(SHRT_MAX, *(int16_t *)c->value);
			(*(int16_t *)c->value) = MAX(SHRT_MIN, *(int16_t *)c->value);
			break;
		case 8:
			(*(uint32_t *)c->value) -= _pow32(16, cc->fieldpointer) * ((*(uint32_t *)c->value) / _pow32(16, cc->fieldpointer)%16);
			(*(uint32_t *)c->value) += _pow32(16, cc->fieldpointer) * value;
			(*(uint32_t *)c->value) = MIN((uint32_t)c->max.i, *(uint32_t *)c->value);
			(*(uint32_t *)c->value) = MAX((uint32_t)c->min.i, *(uint32_t *)c->value);
			break;
	}
	if (c->callback) c->callback(c->callbackarg);
	decControlFieldpointer(cc);
}
void toggleKeyControl(ControlState *cc)
{
	Control *c = &cc->control[cc->cursor];
	if (!c->value) return;

	switch (c->nibbles)
	{
		case 0:
			(*(bool *)c->value) = !(*(bool *)c->value);
			if (c->callback) c->callback(c->callbackarg);
			break;
		case CONTROL_NIBBLES_TOGGLED:
			if (*(float *)c->value > 0.0f) (*(float *)c->value) = 0.0f;
			else                           (*(float *)c->value) = 1.0f;
			if (c->callback) c->callback(c->callbackarg);
			break;
		default:
			cc->keyadjust = !cc->keyadjust;
			break;
	}
}
void revertKeyControl(ControlState *cc)
{
	Control *c = &cc->control[cc->cursor];
	if (!c->value) return;

	switch (c->nibbles)
	{
		case 0: *(bool     *)c->value = (bool    )c->def.i; break;
		case 1: *(int8_t   *)c->value = (int8_t  )c->def.i; break;
		case 2: *(uint8_t  *)c->value = (uint8_t )c->def.i; break;
		case 3: *(int8_t   *)c->value = (int8_t  )c->def.i; break;
		case 4: *(uint16_t *)c->value = (uint16_t)c->def.i; break;
		case 5: *(int16_t  *)c->value = (int16_t )c->def.i; break;
		case 8: *(uint32_t *)c->value = (uint32_t)c->def.i; break;
	}
	if (c->callback) c->callback(c->callbackarg);
}

void mouseControls(ControlState *cc, int button, int x, int y)
{
	Control *c;
	int i, preradixdigits;
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

				if (!c->value) break;
				if (!c->nibbles)
				{
					(*(bool *)c->value) = !(*(bool *)c->value);
					if (c->callback) c->callback(c->callbackarg);
				} else if (c->nibbles == CONTROL_NIBBLES_TOGGLED)
				{
					if (*(float *)c->value > 0.0f) (*(float *)c->value) = 0.0f;
					else                           (*(float *)c->value) = 1.0f;
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
				if (y == c->y && x >= c->x -1 && x <= c->x + MAX(1, c->nibbles) + (MAX(1, c->scalepointlen)-1))
				{
					cc->cursor = i;
					cc->prevmousex = x;
					cc->mouseadjust = 1;
					switch (c->nibbles)
					{
						case 0: case CONTROL_NIBBLES_TOGGLED: cc->fieldpointer = 0; break;
						case CONTROL_NIBBLES_UNSIGNED_FLOAT:
							if      (x < c->x)     cc->fieldpointer = 5;
							else if (x > c->x + 5) cc->fieldpointer = 0;
							else if (x < c->x + getPreRadixDigits(c->max.f)) cc->fieldpointer = 5 - (x - c->x);
							else                   cc->fieldpointer = 6 - (x - c->x);
							break;
						case CONTROL_NIBBLES_SIGNED_FLOAT:
							if      (x < c->x+1)   cc->fieldpointer = 5;
							else if (x > c->x + 6) cc->fieldpointer = 0;
							else if (x < c->x + 1 + getPreRadixDigits(c->max.f)) cc->fieldpointer = 5 - (x - (c->x+1));
							else                   cc->fieldpointer = 6 - (x - (c->x+1));
							break;
						case CONTROL_NIBBLES_UNSIGNED_INT:
							preradixdigits = getPreRadixDigits(c->max.f);
							if      (x < c->x)                   cc->fieldpointer = preradixdigits-1;
							else if (x >= c->x + preradixdigits) cc->fieldpointer = 0;
							else                                 cc->fieldpointer = (preradixdigits-1) - (x - c->x);
							break;
						case CONTROL_NIBBLES_SIGNED_INT:
							preradixdigits = getPreRadixDigits(c->max.f);
							if      (x < c->x+1)                   cc->fieldpointer = preradixdigits-1;
							else if (x >= c->x+1 + preradixdigits) cc->fieldpointer = 0;
							else                                   cc->fieldpointer = (preradixdigits-1) - (x - (c->x+1));
							break;
						case 3: case 5: /* use c->nibbles - 1 */
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
				if (y == c->y && x >= c->x -1 && x <= c->x + MAX(1, c->nibbles) + (MAX(1, c->scalepointlen)-1))
				{
					cc->cursor = i;
					cc->resetadjust = 1;
				}
			} break;
	}
}