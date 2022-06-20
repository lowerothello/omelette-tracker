#define BUTTON1 32
#define BUTTON2 33
#define BUTTON3 34
#define BUTTON_RELEASE 35

#define BUTTON1_HOLD BUTTON1 + 32
#define BUTTON2_HOLD BUTTON2 + 32
#define BUTTON3_HOLD BUTTON3 + 32

#define WHEEL_UP BUTTON1 + 64
#define WHEEL_DOWN BUTTON2 + 64
#define WHEEL_SPEED 3 /* how many lines the wheel should scroll */


/* change these constants for a different keymap */
uint8_t charToNote(int key, uint8_t octave)
{
	octave = octave*12;
	switch (key)
	{
		case ' ': return 255; break;

		case 'q': return 1  +octave; break;
		case 'w': return 2  +octave; break;
		case 'e': return 3  +octave; break;
		case 'r': return 4  +octave; break;
		case 't': return 5  +octave; break;
		case 'y': return 6  +octave; break;
		case 'u': return 7  +octave; break;
		case 'i': return 8  +octave; break;
		case 'o': return 9  +octave; break;
		case 'p': return 10 +octave; break;
		case '[': return 11 +octave; break;
		case ']': return 12 +octave; break;

		case 'a': return 13 +octave; break;
		case 's': return 14 +octave; break;
		case 'd': return 15 +octave; break;
		case 'f': return 16 +octave; break;
		case 'g': return 17 +octave; break;
		case 'h': return 18 +octave; break;
		case 'j': return 19 +octave; break;
		case 'k': return 20 +octave; break;
		case 'l': return 21 +octave; break;
		case ';': return 22 +octave; break;
		case '\'':return 23 +octave; break;
		case '\\':return 24 +octave; break;

		case 'z': return 25 +octave; break;
		case 'x': return 26 +octave; break;
		case 'c': return 27 +octave; break;
		case 'v': return 28 +octave; break;
		case 'b': return 29 +octave; break;
		case 'n': return 30 +octave; break;
		case 'm': return 31 +octave; break;
		case ',': return 32 +octave; break;
		case '.': return 33 +octave; break;
		case '/': return 34 +octave; break;
	}
	return 0;
}

void previewNote(uint8_t note, uint8_t inst, uint8_t channel, char interface)
{
	if (interface && !note)
	{
		if (s->playing == PLAYING_STOP)
		{
			w->previewchannel = channel;
			w->previewnote = note;
			w->previewinst = inst;
			w->previewtrigger = 1;
		}
	} else
	{
		w->previewchannel = channel;
		w->previewnote = note;
		w->previewinst = inst;
		w->previewtrigger = 1;
	}
}


void updateField(signed char fieldpointer, char fieldsize, uint32_t *value, char modifier)
{
	uint32_t multiplier = fieldsize - 1 - fieldpointer;
	uint32_t oldDigit, newDigit;
	if (*value > 0)
		oldDigit = pow32(16, multiplier) * hexDigit32(*value, multiplier);
	else
		oldDigit = 0;
	newDigit = pow32(16, multiplier) * modifier;
	*value = *value - oldDigit + newDigit;
}
void updateFieldPush(uint8_t *field, char value)
{
	*field <<= 4;
	*field += value;
}
