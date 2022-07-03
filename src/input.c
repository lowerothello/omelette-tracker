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
		case ' ': return 255; break;

		case 'q': return 1  +w->octave*12; break;
		case 'w': return 2  +w->octave*12; break;
		case 'e': return 3  +w->octave*12; break;
		case 'r': return 4  +w->octave*12; break;
		case 't': return 5  +w->octave*12; break;
		case 'y': return 6  +w->octave*12; break;
		case 'u': return 7  +w->octave*12; break;
		case 'i': return 8  +w->octave*12; break;
		case 'o': return 9  +w->octave*12; break;
		case 'p': return 10 +w->octave*12; break;
		case '[': return 11 +w->octave*12; break;
		case ']': return 12 +w->octave*12; break;

		case 'a': return 13 +w->octave*12; break;
		case 's': return 14 +w->octave*12; break;
		case 'd': return 15 +w->octave*12; break;
		case 'f': return 16 +w->octave*12; break;
		case 'g': return 17 +w->octave*12; break;
		case 'h': return 18 +w->octave*12; break;
		case 'j': return 19 +w->octave*12; break;
		case 'k': return 20 +w->octave*12; break;
		case 'l': return 21 +w->octave*12; break;
		case ';': return 22 +w->octave*12; break;
		case '\'':return 23 +w->octave*12; break;
		case '\\':return 24 +w->octave*12; break;

		case 'z': return 25 +w->octave*12; break;
		case 'x': return 26 +w->octave*12; break;
		case 'c': return 27 +w->octave*12; break;
		case 'v': return 28 +w->octave*12; break;
		case 'b': return 29 +w->octave*12; break;
		case 'n': return 30 +w->octave*12; break;
		case 'm': return 31 +w->octave*12; break;
		case ',': return 32 +w->octave*12; break;
		case '.': return 33 +w->octave*12; break;
		case '/': return 34 +w->octave*12; break;
	}
	return 0;
}

void previewNote(uint8_t note, uint8_t inst, uint8_t channel)
{
	w->previewmacro.c = '\0';
	w->previewinst = inst;

	if (note == 255)
		w->previewnote = 255;
	else switch (w->keyboardmacro)
	{
		case 0: w->previewnote = note; break;
		case 'G': /* m.x and m.y are note */
			w->previewnote = C5;
			note -= w->octave*12;
			if (note>=13 && note<=20)
			{
				w->previewmacro.c = w->keyboardmacro;
				w->previewmacro.v = (note - 13) * 16 + (note - 13);
			}
			if (note>=25 && note<=32)
			{
				w->previewmacro.c = w->keyboardmacro;
				w->previewmacro.v = (note - 17) * 16 + (note - 17);
			}
			break;
		default: /* m.x is note */
			w->previewnote = C5;
			note -= w->octave*12;
			if (note>=13 && note<=20)
			{
				w->previewmacro.c = w->keyboardmacro;
				w->previewmacro.v = (note - 13) * 16;
			}
			if (note>=25 && note<=32)
			{
				w->previewmacro.c = w->keyboardmacro;
				w->previewmacro.v = (note - 17) * 16;
			}
			break;
	}

	w->previewchannel = channel;
	w->previewtrigger = 1;
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
	*field <<= 4;
	*field += value;
}
