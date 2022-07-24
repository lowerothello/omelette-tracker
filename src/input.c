#define BUTTON1 32
#define BUTTON2 33
#define BUTTON3 34
#define BUTTON_RELEASE 35
#define BUTTON1_CTRL BUTTON1 + 16
#define BUTTON2_CTRL BUTTON2 + 16
#define BUTTON3_CTRL BUTTON3 + 16
#define BUTTON_RELEASE_CTRL BUTTON_RELEASE + 16

#define BUTTON1_HOLD BUTTON1 + 32
#define BUTTON2_HOLD BUTTON2 + 32
#define BUTTON3_HOLD BUTTON3 + 32
#define BUTTON1_HOLD_CTRL BUTTON1_CTRL + 32
#define BUTTON2_HOLD_CTRL BUTTON2_CTRL + 32
#define BUTTON3_HOLD_CTRL BUTTON3_CTRL + 32

#define WHEEL_UP BUTTON1 + 64
#define WHEEL_DOWN BUTTON2 + 64
#define WHEEL_UP_CTRL BUTTON1_CTRL + 64
#define WHEEL_DOWN_CTRL BUTTON2_CTRL + 64


/* change these constants for a different keymap */
uint8_t charToNote(int key)
{
	switch (key)
	{
		case ' ': return NOTE_OFF; break;

		case 'q': case 'Q': return MIN(A10-1, 0  +w->octave*12); break;
		case 'w': case 'W': return MIN(A10-1, 1  +w->octave*12); break;
		case 'e': case 'E': return MIN(A10-1, 2  +w->octave*12); break;
		case 'r': case 'R': return MIN(A10-1, 3  +w->octave*12); break;
		case 't': case 'T': return MIN(A10-1, 4  +w->octave*12); break;
		case 'y': case 'Y': return MIN(A10-1, 5  +w->octave*12); break;
		case 'u': case 'U': return MIN(A10-1, 6  +w->octave*12); break;
		case 'i': case 'I': return MIN(A10-1, 7  +w->octave*12); break;
		case 'o': case 'O': return MIN(A10-1, 8  +w->octave*12); break;
		case 'p': case 'P': return MIN(A10-1, 9  +w->octave*12); break;
		case '[': case '{': return MIN(A10-1, 10 +w->octave*12); break;
		case ']': case '}': return MIN(A10-1, 11 +w->octave*12); break;

		case 'a': case 'A': return MIN(A10-1, 12 +w->octave*12); break;
		case 's': case 'S': return MIN(A10-1, 13 +w->octave*12); break;
		case 'd': case 'D': return MIN(A10-1, 14 +w->octave*12); break;
		case 'f': case 'F': return MIN(A10-1, 15 +w->octave*12); break;
		case 'g': case 'G': return MIN(A10-1, 16 +w->octave*12); break;
		case 'h': case 'H': return MIN(A10-1, 17 +w->octave*12); break;
		case 'j': case 'J': return MIN(A10-1, 18 +w->octave*12); break;
		case 'k': case 'K': return MIN(A10-1, 19 +w->octave*12); break;
		case 'l': case 'L': return MIN(A10-1, 20 +w->octave*12); break;
		case ';': case ':': return MIN(A10-1, 21 +w->octave*12); break;
		case '\'':case '"': return MIN(A10-1, 22 +w->octave*12); break;
		case '\\':case '|': return MIN(A10-1, 23 +w->octave*12); break;

		case 'z': case 'Z': return MIN(A10-1, 24 +w->octave*12); break;
		case 'x': case 'X': return MIN(A10-1, 25 +w->octave*12); break;
		case 'c': case 'C': return MIN(A10-1, 26 +w->octave*12); break;
		case 'v': case 'V': return MIN(A10-1, 27 +w->octave*12); break;
		case 'b': case 'B': return MIN(A10-1, 28 +w->octave*12); break;
		case 'n': case 'N': return MIN(A10-1, 29 +w->octave*12); break;
		case 'm': case 'M': return MIN(A10-1, 30 +w->octave*12); break;
		case ',': case '<': return MIN(A10-1, 31 +w->octave*12); break;
		case '.': case '>': return MIN(A10-1, 32 +w->octave*12); break;
		case '/': case '?': return MIN(A10-1, 33 +w->octave*12); break;
	}
	return NOTE_VOID;
}

void previewNote(uint8_t note, uint8_t inst, uint8_t channel)
{
	w->previewrow.macro[0].c = '\0';
	w->previewrow.inst = inst;
	w->previewchannelsrc = channel;

	if (note == NOTE_OFF)
	{
		if (w->previewrow.note != NOTE_OFF)
		{
			w->previewrow.note = NOTE_OFF;
			w->previewtrigger = 1;
		}
	} else
	{
		switch (w->keyboardmacro)
		{
			case 0: w->previewrow.note = note; break;
			case 'G': /* m.x and m.y are note */
				w->previewrow.note = C5;
				note -= w->octave*12;
				if (note>=12 && note<=19)
				{
					w->previewrow.macro[0].c = w->keyboardmacro;
					w->previewrow.macro[0].v = (note - 12) * 16 + (note - 12);
				}
				if (note>=24 && note<=31)
				{
					w->previewrow.macro[0].c = w->keyboardmacro;
					w->previewrow.macro[0].v = (note - 16) * 16 + (note - 16);
				}
				break;
			default: /* m.x is note */
				w->previewrow.note = C5;
				note -= w->octave*12;
				if (note>=12 && note<=19)
				{
					w->previewrow.macro[0].c = w->keyboardmacro;
					w->previewrow.macro[0].v = (note - 12) * 16;
				}
				if (note>=24 && note<=31)
				{
					w->previewrow.macro[0].c = w->keyboardmacro;
					w->previewrow.macro[0].v = (note - 16) * 16;
				}
				break;
		} w->previewtrigger = 1;
	}

}

void incField(signed char fieldpointer, uint32_t *value, uint32_t max)
{
	uint32_t oldval = *value;
	*value += pow32(16, fieldpointer);
	if (oldval > *value) *value = max;
}

void decField(signed char fieldpointer, uint32_t *value)
{
	uint32_t oldval = *value;
	*value -= pow32(16, fieldpointer);
	if (oldval < *value) *value = 0;
}

void updateField(signed char fieldpointer, uint32_t *value, char modifier)
{
	uint32_t oldDigit, newDigit;
	if (*value > 0)
		oldDigit = pow32(16, fieldpointer) * hexDigit32(*value, fieldpointer);
	else
		oldDigit = 0;
	newDigit = pow32(16, fieldpointer) * modifier;
	*value = *value - oldDigit + newDigit;
}
void updateFieldPush(uint8_t *field, char value)
{
	*field<<=4;
	*field+=value;
}
