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


/* *note is allowed to be null             */
/* returns true if a valid pad was pressed */
int charToKmode(int key, char GXXstyle, uint8_t *macrov, uint8_t *note)
{
	if (note) *note = NOTE_C5;
	switch (key)
	{
		case 'z':                                    *macrov = 0x00; return 1;  case 'Z': *macrov = ((*macrov>>4)<<4) + 0x0; return 1;
		case 'x': if (GXXstyle) *macrov = 0x11; else *macrov = 0x10; return 1;  case 'X': *macrov = ((*macrov>>4)<<4) + 0x1; return 1;
		case 'c': if (GXXstyle) *macrov = 0x22; else *macrov = 0x20; return 1;  case 'C': *macrov = ((*macrov>>4)<<4) + 0x2; return 1;
		case 'v': if (GXXstyle) *macrov = 0x33; else *macrov = 0x30; return 1;  case 'V': *macrov = ((*macrov>>4)<<4) + 0x3; return 1;
		case 'a': if (GXXstyle) *macrov = 0x44; else *macrov = 0x40; return 1;  case 'A': *macrov = ((*macrov>>4)<<4) + 0x4; return 1;
		case 's': if (GXXstyle) *macrov = 0x55; else *macrov = 0x50; return 1;  case 'S': *macrov = ((*macrov>>4)<<4) + 0x5; return 1;
		case 'd': if (GXXstyle) *macrov = 0x66; else *macrov = 0x60; return 1;  case 'D': *macrov = ((*macrov>>4)<<4) + 0x6; return 1;
		case 'f': if (GXXstyle) *macrov = 0x77; else *macrov = 0x70; return 1;  case 'F': *macrov = ((*macrov>>4)<<4) + 0x7; return 1;
		case 'q': if (GXXstyle) *macrov = 0x88; else *macrov = 0x80; return 1;  case 'Q': *macrov = ((*macrov>>4)<<4) + 0x8; return 1;
		case 'w': if (GXXstyle) *macrov = 0x99; else *macrov = 0x90; return 1;  case 'W': *macrov = ((*macrov>>4)<<4) + 0x9; return 1;
		case 'e': if (GXXstyle) *macrov = 0xaa; else *macrov = 0xa0; return 1;  case 'E': *macrov = ((*macrov>>4)<<4) + 0xa; return 1;
		case 'r': if (GXXstyle) *macrov = 0xbb; else *macrov = 0xb0; return 1;  case 'R': *macrov = ((*macrov>>4)<<4) + 0xb; return 1;
		case '1': if (GXXstyle) *macrov = 0xcc; else *macrov = 0xc0; return 1;  case '!': *macrov = ((*macrov>>4)<<4) + 0xc; return 1;
		case '2': if (GXXstyle) *macrov = 0xdd; else *macrov = 0xd0; return 1;  case '@': *macrov = ((*macrov>>4)<<4) + 0xd; return 1;
		case '3': if (GXXstyle) *macrov = 0xee; else *macrov = 0xe0; return 1;  case '#': *macrov = ((*macrov>>4)<<4) + 0xe; return 1;
		case '4': if (GXXstyle) *macrov = 0xff; else *macrov = 0xf0; return 1;  case '$': *macrov = ((*macrov>>4)<<4) + 0xf; return 1;

		case ' ': if (note) *note = NOTE_OFF;  return 0;
		default:  if (note) *note = NOTE_VOID; return 0;
	}
}

/* change these constants for anything but a modplug-style keymap */
/* returns true if the key is capitalized */
int charToNote(int key, uint8_t *note)
{
	switch (key)
	{
		case 'q':  *note = MIN(NOTE_A10-1, 0  +w->octave*12); return 0;  case 'Q': *note = MIN(NOTE_A10-1, 0  +w->octave*12); return 1;
		case 'w':  *note = MIN(NOTE_A10-1, 1  +w->octave*12); return 0;  case 'W': *note = MIN(NOTE_A10-1, 1  +w->octave*12); return 1;
		case 'e':  *note = MIN(NOTE_A10-1, 2  +w->octave*12); return 0;  case 'E': *note = MIN(NOTE_A10-1, 2  +w->octave*12); return 1;
		case 'r':  *note = MIN(NOTE_A10-1, 3  +w->octave*12); return 0;  case 'R': *note = MIN(NOTE_A10-1, 3  +w->octave*12); return 1;
		case 't':  *note = MIN(NOTE_A10-1, 4  +w->octave*12); return 0;  case 'T': *note = MIN(NOTE_A10-1, 4  +w->octave*12); return 1;
		case 'y':  *note = MIN(NOTE_A10-1, 5  +w->octave*12); return 0;  case 'Y': *note = MIN(NOTE_A10-1, 5  +w->octave*12); return 1;
		case 'u':  *note = MIN(NOTE_A10-1, 6  +w->octave*12); return 0;  case 'U': *note = MIN(NOTE_A10-1, 6  +w->octave*12); return 1;
		case 'i':  *note = MIN(NOTE_A10-1, 7  +w->octave*12); return 0;  case 'I': *note = MIN(NOTE_A10-1, 7  +w->octave*12); return 1;
		case 'o':  *note = MIN(NOTE_A10-1, 8  +w->octave*12); return 0;  case 'O': *note = MIN(NOTE_A10-1, 8  +w->octave*12); return 1;
		case 'p':  *note = MIN(NOTE_A10-1, 9  +w->octave*12); return 0;  case 'P': *note = MIN(NOTE_A10-1, 9  +w->octave*12); return 1;
		case '[':  *note = MIN(NOTE_A10-1, 10 +w->octave*12); return 0;  case '{': *note = MIN(NOTE_A10-1, 10 +w->octave*12); return 1;
		case ']':  *note = MIN(NOTE_A10-1, 11 +w->octave*12); return 0;  case '}': *note = MIN(NOTE_A10-1, 11 +w->octave*12); return 1;

		case 'a':  *note = MIN(NOTE_A10-1, 12 +w->octave*12); return 0;  case 'A': *note = MIN(NOTE_A10-1, 12 +w->octave*12); return 1;
		case 's':  *note = MIN(NOTE_A10-1, 13 +w->octave*12); return 0;  case 'S': *note = MIN(NOTE_A10-1, 13 +w->octave*12); return 1;
		case 'd':  *note = MIN(NOTE_A10-1, 14 +w->octave*12); return 0;  case 'D': *note = MIN(NOTE_A10-1, 14 +w->octave*12); return 1;
		case 'f':  *note = MIN(NOTE_A10-1, 15 +w->octave*12); return 0;  case 'F': *note = MIN(NOTE_A10-1, 15 +w->octave*12); return 1;
		case 'g':  *note = MIN(NOTE_A10-1, 16 +w->octave*12); return 0;  case 'G': *note = MIN(NOTE_A10-1, 16 +w->octave*12); return 1;
		case 'h':  *note = MIN(NOTE_A10-1, 17 +w->octave*12); return 0;  case 'H': *note = MIN(NOTE_A10-1, 17 +w->octave*12); return 1;
		case 'j':  *note = MIN(NOTE_A10-1, 18 +w->octave*12); return 0;  case 'J': *note = MIN(NOTE_A10-1, 18 +w->octave*12); return 1;
		case 'k':  *note = MIN(NOTE_A10-1, 19 +w->octave*12); return 0;  case 'K': *note = MIN(NOTE_A10-1, 19 +w->octave*12); return 1;
		case 'l':  *note = MIN(NOTE_A10-1, 20 +w->octave*12); return 0;  case 'L': *note = MIN(NOTE_A10-1, 20 +w->octave*12); return 1;
		case ';':  *note = MIN(NOTE_A10-1, 21 +w->octave*12); return 0;  case ':': *note = MIN(NOTE_A10-1, 21 +w->octave*12); return 1;
		case '\'': *note = MIN(NOTE_A10-1, 22 +w->octave*12); return 0;  case '"': *note = MIN(NOTE_A10-1, 22 +w->octave*12); return 1;
		case '\\': *note = MIN(NOTE_A10-1, 23 +w->octave*12); return 0;  case '|': *note = MIN(NOTE_A10-1, 23 +w->octave*12); return 1;

		case 'z':  *note = MIN(NOTE_A10-1, 24 +w->octave*12); return 0;  case 'Z': *note = MIN(NOTE_A10-1, 24 +w->octave*12); return 1;
		case 'x':  *note = MIN(NOTE_A10-1, 25 +w->octave*12); return 0;  case 'X': *note = MIN(NOTE_A10-1, 25 +w->octave*12); return 1;
		case 'c':  *note = MIN(NOTE_A10-1, 26 +w->octave*12); return 0;  case 'C': *note = MIN(NOTE_A10-1, 26 +w->octave*12); return 1;
		case 'v':  *note = MIN(NOTE_A10-1, 27 +w->octave*12); return 0;  case 'V': *note = MIN(NOTE_A10-1, 27 +w->octave*12); return 1;
		case 'b':  *note = MIN(NOTE_A10-1, 28 +w->octave*12); return 0;  case 'B': *note = MIN(NOTE_A10-1, 28 +w->octave*12); return 1;
		case 'n':  *note = MIN(NOTE_A10-1, 29 +w->octave*12); return 0;  case 'N': *note = MIN(NOTE_A10-1, 29 +w->octave*12); return 1;
		case 'm':  *note = MIN(NOTE_A10-1, 30 +w->octave*12); return 0;  case 'M': *note = MIN(NOTE_A10-1, 30 +w->octave*12); return 1;
		case ',':  *note = MIN(NOTE_A10-1, 31 +w->octave*12); return 0;  case '<': *note = MIN(NOTE_A10-1, 31 +w->octave*12); return 1;
		case '.':  *note = MIN(NOTE_A10-1, 32 +w->octave*12); return 0;  case '>': *note = MIN(NOTE_A10-1, 32 +w->octave*12); return 1;
		case '/':  *note = MIN(NOTE_A10-1, 33 +w->octave*12); return 0;  case '?': *note = MIN(NOTE_A10-1, 33 +w->octave*12); return 1;

		case ' ': *note = NOTE_OFF;  return 0;
		default:  *note = NOTE_VOID; return 0;
	}
}

void previewNote(int key, uint8_t inst, uint8_t channel)
{
	w->previewrow.macro[0].c = '\0';
	w->previewrow.inst = inst;
	w->previewchannelsrc = channel;

	switch (w->keyboardmacro)
	{
		case 0: charToNote(key, &w->previewrow.note); break;
		case 'G':
			if (charToKmode(key, 1, &w->previewrow.macro[0].v, &w->previewrow.note))
				w->previewrow.macro[0].c = w->keyboardmacro;
			break;
		default:
			if (charToKmode(key, 0, &w->previewrow.macro[0].v, &w->previewrow.note))
				w->previewrow.macro[0].c = w->keyboardmacro;
			break;
	} w->previewtrigger = 1;
}
