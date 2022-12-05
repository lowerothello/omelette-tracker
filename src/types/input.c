/* *note is allowed to be null             */
/* returns true if a valid pad was pressed */
int charToKmode(int key, bool linknibbles, uint8_t *macrov, uint8_t *note)
{
	if (note) *note = NOTE_C5;
	switch (key)
	{
		case 'z':                                       *macrov = 0x00; return 1;  case 'Z': *macrov = ((*macrov>>4)<<4) + 0x0; return 1;
		case 'x': if (linknibbles) *macrov = 0x11; else *macrov = 0x10; return 1;  case 'X': *macrov = ((*macrov>>4)<<4) + 0x1; return 1;
		case 'c': if (linknibbles) *macrov = 0x22; else *macrov = 0x20; return 1;  case 'C': *macrov = ((*macrov>>4)<<4) + 0x2; return 1;
		case 'v': if (linknibbles) *macrov = 0x33; else *macrov = 0x30; return 1;  case 'V': *macrov = ((*macrov>>4)<<4) + 0x3; return 1;
		case 'a': if (linknibbles) *macrov = 0x44; else *macrov = 0x40; return 1;  case 'A': *macrov = ((*macrov>>4)<<4) + 0x4; return 1;
		case 's': if (linknibbles) *macrov = 0x55; else *macrov = 0x50; return 1;  case 'S': *macrov = ((*macrov>>4)<<4) + 0x5; return 1;
		case 'd': if (linknibbles) *macrov = 0x66; else *macrov = 0x60; return 1;  case 'D': *macrov = ((*macrov>>4)<<4) + 0x6; return 1;
		case 'f': if (linknibbles) *macrov = 0x77; else *macrov = 0x70; return 1;  case 'F': *macrov = ((*macrov>>4)<<4) + 0x7; return 1;
		case 'q': if (linknibbles) *macrov = 0x88; else *macrov = 0x80; return 1;  case 'Q': *macrov = ((*macrov>>4)<<4) + 0x8; return 1;
		case 'w': if (linknibbles) *macrov = 0x99; else *macrov = 0x90; return 1;  case 'W': *macrov = ((*macrov>>4)<<4) + 0x9; return 1;
		case 'e': if (linknibbles) *macrov = 0xaa; else *macrov = 0xa0; return 1;  case 'E': *macrov = ((*macrov>>4)<<4) + 0xa; return 1;
		case 'r': if (linknibbles) *macrov = 0xbb; else *macrov = 0xb0; return 1;  case 'R': *macrov = ((*macrov>>4)<<4) + 0xb; return 1;
		case '1': if (linknibbles) *macrov = 0xcc; else *macrov = 0xc0; return 1;  case '!': *macrov = ((*macrov>>4)<<4) + 0xc; return 1;
		case '2': if (linknibbles) *macrov = 0xdd; else *macrov = 0xd0; return 1;  case '@': *macrov = ((*macrov>>4)<<4) + 0xd; return 1;
		case '3': if (linknibbles) *macrov = 0xee; else *macrov = 0xe0; return 1;  case '#': *macrov = ((*macrov>>4)<<4) + 0xe; return 1;
		case '4': if (linknibbles) *macrov = 0xff; else *macrov = 0xf0; return 1;  case '$': *macrov = ((*macrov>>4)<<4) + 0xf; return 1;

		case ' ': if (note) *note = NOTE_OFF;  return 0;
		default : if (note) *note = NOTE_VOID; return 0;
	}
}

/* change these constants for anything but a modplug-style keymap */
void charToNote(int key, uint8_t *note)
{
	switch (key)
	{
		case 'q' : *note = MIN(NOTE_A10-1, 0  +w->octave*12); return;  case 'Q': *note = MIN(NOTE_A10-1, 0  +w->octave*12); return;
		case 'w' : *note = MIN(NOTE_A10-1, 1  +w->octave*12); return;  case 'W': *note = MIN(NOTE_A10-1, 1  +w->octave*12); return;
		case 'e' : *note = MIN(NOTE_A10-1, 2  +w->octave*12); return;  case 'E': *note = MIN(NOTE_A10-1, 2  +w->octave*12); return;
		case 'r' : *note = MIN(NOTE_A10-1, 3  +w->octave*12); return;  case 'R': *note = MIN(NOTE_A10-1, 3  +w->octave*12); return;
		case 't' : *note = MIN(NOTE_A10-1, 4  +w->octave*12); return;  case 'T': *note = MIN(NOTE_A10-1, 4  +w->octave*12); return;
		case 'y' : *note = MIN(NOTE_A10-1, 5  +w->octave*12); return;  case 'Y': *note = MIN(NOTE_A10-1, 5  +w->octave*12); return;
		case 'u' : *note = MIN(NOTE_A10-1, 6  +w->octave*12); return;  case 'U': *note = MIN(NOTE_A10-1, 6  +w->octave*12); return;
		case 'i' : *note = MIN(NOTE_A10-1, 7  +w->octave*12); return;  case 'I': *note = MIN(NOTE_A10-1, 7  +w->octave*12); return;
		case 'o' : *note = MIN(NOTE_A10-1, 8  +w->octave*12); return;  case 'O': *note = MIN(NOTE_A10-1, 8  +w->octave*12); return;
		case 'p' : *note = MIN(NOTE_A10-1, 9  +w->octave*12); return;  case 'P': *note = MIN(NOTE_A10-1, 9  +w->octave*12); return;
		case '[' : *note = MIN(NOTE_A10-1, 10 +w->octave*12); return;  case '{': *note = MIN(NOTE_A10-1, 10 +w->octave*12); return;
		case ']' : *note = MIN(NOTE_A10-1, 11 +w->octave*12); return;  case '}': *note = MIN(NOTE_A10-1, 11 +w->octave*12); return;

		case 'a' : *note = MIN(NOTE_A10-1, 12 +w->octave*12); return;  case 'A': *note = MIN(NOTE_A10-1, 12 +w->octave*12); return;
		case 's' : *note = MIN(NOTE_A10-1, 13 +w->octave*12); return;  case 'S': *note = MIN(NOTE_A10-1, 13 +w->octave*12); return;
		case 'd' : *note = MIN(NOTE_A10-1, 14 +w->octave*12); return;  case 'D': *note = MIN(NOTE_A10-1, 14 +w->octave*12); return;
		case 'f' : *note = MIN(NOTE_A10-1, 15 +w->octave*12); return;  case 'F': *note = MIN(NOTE_A10-1, 15 +w->octave*12); return;
		case 'g' : *note = MIN(NOTE_A10-1, 16 +w->octave*12); return;  case 'G': *note = MIN(NOTE_A10-1, 16 +w->octave*12); return;
		case 'h' : *note = MIN(NOTE_A10-1, 17 +w->octave*12); return;  case 'H': *note = MIN(NOTE_A10-1, 17 +w->octave*12); return;
		case 'j' : *note = MIN(NOTE_A10-1, 18 +w->octave*12); return;  case 'J': *note = MIN(NOTE_A10-1, 18 +w->octave*12); return;
		case 'k' : *note = MIN(NOTE_A10-1, 19 +w->octave*12); return;  case 'K': *note = MIN(NOTE_A10-1, 19 +w->octave*12); return;
		case 'l' : *note = MIN(NOTE_A10-1, 20 +w->octave*12); return;  case 'L': *note = MIN(NOTE_A10-1, 20 +w->octave*12); return;
		case ';' : *note = MIN(NOTE_A10-1, 21 +w->octave*12); return;  case ':': *note = MIN(NOTE_A10-1, 21 +w->octave*12); return;
		case '\'': *note = MIN(NOTE_A10-1, 22 +w->octave*12); return;  case '"': *note = MIN(NOTE_A10-1, 22 +w->octave*12); return;
		case '\\': *note = MIN(NOTE_A10-1, 23 +w->octave*12); return;  case '|': *note = MIN(NOTE_A10-1, 23 +w->octave*12); return;

		case 'z' : *note = MIN(NOTE_A10-1, 24 +w->octave*12); return;  case 'Z': *note = MIN(NOTE_A10-1, 24 +w->octave*12); return;
		case 'x' : *note = MIN(NOTE_A10-1, 25 +w->octave*12); return;  case 'X': *note = MIN(NOTE_A10-1, 25 +w->octave*12); return;
		case 'c' : *note = MIN(NOTE_A10-1, 26 +w->octave*12); return;  case 'C': *note = MIN(NOTE_A10-1, 26 +w->octave*12); return;
		case 'v' : *note = MIN(NOTE_A10-1, 27 +w->octave*12); return;  case 'V': *note = MIN(NOTE_A10-1, 27 +w->octave*12); return;
		case 'b' : *note = MIN(NOTE_A10-1, 28 +w->octave*12); return;  case 'B': *note = MIN(NOTE_A10-1, 28 +w->octave*12); return;
		case 'n' : *note = MIN(NOTE_A10-1, 29 +w->octave*12); return;  case 'N': *note = MIN(NOTE_A10-1, 29 +w->octave*12); return;
		case 'm' : *note = MIN(NOTE_A10-1, 30 +w->octave*12); return;  case 'M': *note = MIN(NOTE_A10-1, 30 +w->octave*12); return;
		case ',' : *note = MIN(NOTE_A10-1, 31 +w->octave*12); return;  case '<': *note = MIN(NOTE_A10-1, 31 +w->octave*12); return;
		case '.' : *note = MIN(NOTE_A10-1, 32 +w->octave*12); return;  case '>': *note = MIN(NOTE_A10-1, 32 +w->octave*12); return;
		case '/' : *note = MIN(NOTE_A10-1, 33 +w->octave*12); return;  case '?': *note = MIN(NOTE_A10-1, 33 +w->octave*12); return;

		case ' ' : *note = NOTE_OFF;  return;
		default  : *note = NOTE_VOID; return;
	}
}

static void tooltipAddCount(void *add)
{
	w->count *= 10;
	w->count += (size_t)add;
	p->redraw = 1;
}
void addCountBinds(TooltipState *tt, bool draw)
{
	unsigned int flags = TT_DEAD;
	if (draw) flags |= TT_DRAW;
	addTooltipBind(tt, "add 0 to count", 0, XK_0, flags, tooltipAddCount, (void*)0);
	addTooltipBind(tt, "add 1 to count", 0, XK_1, flags, tooltipAddCount, (void*)1);
	addTooltipBind(tt, "add 2 to count", 0, XK_2, flags, tooltipAddCount, (void*)2);
	addTooltipBind(tt, "add 3 to count", 0, XK_3, flags, tooltipAddCount, (void*)3);
	addTooltipBind(tt, "add 4 to count", 0, XK_4, flags, tooltipAddCount, (void*)4);
	addTooltipBind(tt, "add 5 to count", 0, XK_5, flags, tooltipAddCount, (void*)5);
	addTooltipBind(tt, "add 6 to count", 0, XK_6, flags, tooltipAddCount, (void*)6);
	addTooltipBind(tt, "add 7 to count", 0, XK_7, flags, tooltipAddCount, (void*)7);
	addTooltipBind(tt, "add 8 to count", 0, XK_8, flags, tooltipAddCount, (void*)8);
	addTooltipBind(tt, "add 9 to count", 0, XK_9, flags, tooltipAddCount, (void*)9);
}

void _previewNote(UI *cw, int key, uint8_t inst)
{
	cw->previewrow.macro[0].c = '\0';
	cw->previewrow.inst = inst;

	if (cw->keyboardmacro)
	{
		if (charToKmode(key, linkMacroNibbles(cw->keyboardmacro), &cw->previewrow.macro[0].v, &cw->previewrow.note))
		{
			cw->previewrow.macro[0].c = cw->keyboardmacro;
			cw->previewrow.macro[0].alt = cw->keyboardmacroalt;
		}
	} else charToNote(key, &cw->previewrow.note);
}
void previewNote(int key, uint8_t inst)
{
	if (p->w->page == PAGE_TRACK_VARIANT && p->s->playing) return;
	_previewNote(w, key, inst);
	w->previewtrigger = PTRIG_NORMAL;
}
void previewRow(Row *r)
{
	if (w->page == PAGE_TRACK_VARIANT && s->playing) return;
	memcpy(&w->previewrow, r, sizeof(Row));
	w->previewtrigger = PTRIG_NORMAL;
}
void previewFileNote(UI *cw, int key)
{
	_previewNote(cw, key, 0); /* inst arg is unused so it doesn't matter */
	w->previewtrigger = PTRIG_FILE;
}


enum _INPUT_MODE {
	INPUT_MODE_NONE,  /* just interpret stdin        */
	INPUT_MODE_RAW,   /* use the raw driver, console */
	INPUT_MODE_X,     /* use an x hack, xterm        */
} INPUT_MODE;

enum _INPUT_MODE getRawInputMode(void)
{
#ifdef DISABLE_RAW_INPUT
	return INPUT_MODE_NONE;
#endif
	if (getenv("OML_STDIN"))              return INPUT_MODE_NONE;
	if (!strcmp(getenv("TERM"), "LINUX")) return INPUT_MODE_RAW;
	if (getenv("DISPLAY"))                return INPUT_MODE_X;
	return INPUT_MODE_NONE; /* fallback */
}


Display *dpy;
pthread_t xeventthread_id;
Window wpy;
static void *XEventThread(void *arg)
{
	Event  e;        /* omelette event */
	XEvent ev, *evp; /* Xorg events    */
	while (1)
	{
		XNextEvent(dpy, &ev);

		e.sem = M_SEM_INPUT;
		evp = malloc(sizeof(XEvent));
		memcpy(evp, &ev, sizeof(XEvent));
		e.callbackarg = evp;
		pushEvent(&e);
	}
	return NULL;
}
/* stdin is always initialized */
/* returns true for failure */
int initRawInput(enum _INPUT_MODE mode)
{
	int revtoret;
	switch (mode)
	{
		case INPUT_MODE_RAW:
			// ioctl(0, KDSKBMODE, K_RAW); /* TODO: */
			break;
		case INPUT_MODE_X:
			if (!(dpy = XOpenDisplay(NULL))) return 1;
			XGetInputFocus(dpy, &wpy, &revtoret);
			XGrabKey(dpy, AnyKey, AnyModifier, wpy, 1, GrabModeAsync, GrabModeAsync);
			pthread_create(&xeventthread_id, NULL, XEventThread, NULL);
			break;
		case INPUT_MODE_NONE: break;
	}
	return 0;
}

void freeRawInput(enum _INPUT_MODE mode)
{
	switch (mode)
	{
		case INPUT_MODE_RAW:
			// ioctl(0, KDSKBMODE, K_XLATE);
			break;
		case INPUT_MODE_X:
			/* TODO: cleanly exit xeventthread_id */
			XUngrabKey(dpy, AnyKey, AnyModifier, wpy);
			XCloseDisplay(dpy);
			break;
		case INPUT_MODE_NONE: break;
	}
}

static unsigned int ansiToModState(int input)
{
	unsigned int ret = 0;
	input -= 0x31; /* '2' => 1 */
	if (input&1) ret |= ShiftMask;
	if (input&2) ret |= Mod1Mask;
	if (input&4) ret |= ControlMask;
	return ret;
}

/* TODO: doesn't really belong here, idk a better way to handle this */
void setDefaultColour(int colour, unsigned int r, unsigned int g, unsigned int b)
{
	tc.colour[colour].r = r;
	tc.colour[colour].g = g;
	tc.colour[colour].b = b;

	/* trigger a redraw when all 8 colours have been read in */
	tc.semaphore++;
	if (tc.semaphore >= 8)
	{
		p->redraw = 1;
		tc.semaphore -= 8;
	}
}

void handleStdin(TooltipState *tt)
{
	unsigned int mod = 0;
	static int input;
	while (1)
		switch (input = getchar())
		{
			case EOF: return;
			case '\033': /* escape */ switch (input = getchar()) {
					case EOF: inputTooltip(tt, mod, XK_Escape, 0); return;
					case ']': /* OSC */ switch (input = getchar()) {
							case '4': {
									int colour;
									unsigned int r, g, b;
									scanf(";%d;rgb:%02x%*02x/%02x%*02x/%02x%*02x\007",
											&colour, &r, &g, &b);
									setDefaultColour(colour, r, g, b);
								} break;
						} break;
					case 'O': /* SS3 */ switch (input = getchar()) {
							case 'P': inputTooltip(tt, mod, XK_F1, 0); break;
							case 'Q': inputTooltip(tt, mod, XK_F2, 0); break;
							case 'R': inputTooltip(tt, mod, XK_F3, 0); break;
							case 'S': inputTooltip(tt, mod, XK_F4, 0); break;
						} break;
					case '[': /* CSI */ switch (input = getchar()) {
							case '[': switch (input = getchar()) {
									case 'A': inputTooltip(tt, mod, XK_F1, 0); break;
									case 'B': inputTooltip(tt, mod, XK_F2, 0); break;
									case 'C': inputTooltip(tt, mod, XK_F3, 0); break;
									case 'D': inputTooltip(tt, mod, XK_F4, 0); break;
									case 'E': inputTooltip(tt, mod, XK_F5, 0); break;
								} break;
							case 'A': inputTooltip(tt, mod, XK_Up   , 0); break;
							case 'B': inputTooltip(tt, mod, XK_Down , 0); break;
							case 'D': inputTooltip(tt, mod, XK_Left , 0); break;
							case 'C': inputTooltip(tt, mod, XK_Right, 0); break;
							case '1': switch (input = getchar()) {
									case '5': switch (input = getchar()) {
											case '~': inputTooltip(tt, mod, XK_F5, 0); break;
											case ';': inputTooltip(tt, mod|ansiToModState(getchar()), XK_F5, 0); getchar(); break;
										} break;
									case '7': switch (input = getchar()) {
											case '~': inputTooltip(tt, mod, XK_F6, 0); break;
											case ';': inputTooltip(tt, mod|ansiToModState(getchar()), XK_F6, 0); getchar(); break;
										} break;
									case '8': switch (input = getchar()) {
											case '~': inputTooltip(tt, mod, XK_F7, 0); break;
											case ';': inputTooltip(tt, mod|ansiToModState(getchar()), XK_F7, 0); getchar(); break;
										} break;
									case '9': switch (input = getchar()) {
											case '~': inputTooltip(tt, mod, XK_F8, 0); break;
											case ';': inputTooltip(tt, mod|ansiToModState(getchar()), XK_F8, 0); getchar(); break;
										} break;
									case ';':
										mod |= ansiToModState(getchar());
										switch (input = getchar()) {
											case 'A': inputTooltip(tt, mod, XK_Up   , 0); break;
											case 'B': inputTooltip(tt, mod, XK_Down , 0); break;
											case 'D': inputTooltip(tt, mod, XK_Left , 0); break;
											case 'C': inputTooltip(tt, mod, XK_Right, 0); break;
											case 'P': inputTooltip(tt, mod, XK_F1, 0); break;
											case 'Q': inputTooltip(tt, mod, XK_F2, 0); break;
											case 'R': inputTooltip(tt, mod, XK_F3, 0); break;
											case 'S': inputTooltip(tt, mod, XK_F4, 0); break;
										} break;
									case '~': inputTooltip(tt, mod, XK_Home, 0); break;
								} break;
							case '2': switch (input = getchar()) {
									case '0': switch (input = getchar()) {
											case '~': inputTooltip(tt, mod, XK_F9, 0); break;
											case ';': inputTooltip(tt, mod|ansiToModState(getchar()), XK_F9, 0); getchar(); break;
										} break;
									case '1': switch (input = getchar()) {
											case '~': inputTooltip(tt, mod, XK_F10, 0); break;
											case ';': inputTooltip(tt, mod|ansiToModState(getchar()), XK_F10, 0); getchar(); break;
										} break;
									case '3': switch (input = getchar()) {
											case '~': inputTooltip(tt, mod, XK_F11, 0); break;
											case ';': inputTooltip(tt, mod|ansiToModState(getchar()), XK_F11, 0); getchar(); break;
										} break;
									case '4': switch (input = getchar()) {
											case '~': inputTooltip(tt, mod, XK_F12, 0); break;
											case ';': inputTooltip(tt, mod|ansiToModState(getchar()), XK_F12, 0); getchar(); break;
										} break;
									case 'J': inputTooltip(tt, mod|ShiftMask, XK_Home  , 0); break;
									case 'K': inputTooltip(tt, mod|ShiftMask, XK_Delete, 0); break;
								} break;
							case '4': if (getchar() == '~') inputTooltip(tt, mod, XK_End, 0); break;
							case '5': switch (input = getchar()) {
									case '~': inputTooltip(tt, mod, XK_Page_Up, 0); break;
									case ';': inputTooltip(tt, mod|ansiToModState(getchar()), XK_Page_Up, 0); getchar(); break;
								} break;
							case '6': switch (input = getchar()) {
									case '~': inputTooltip(tt, mod, XK_Page_Down, 0); getchar(); break;
									case ';': inputTooltip(tt, mod|ansiToModState(getchar()), XK_Page_Down, 0); getchar(); break;
								} break;
							case 'H': inputTooltip(tt, mod, XK_Home, 0); break;
							case 'J': inputTooltip(tt, mod|ControlMask, XK_End, 0); break;
							case 'K': inputTooltip(tt, mod|ShiftMask  , XK_End, 0); break;
							case 'M': { /* TODO: ctrl+delete maps to CSI M, which conflicts */
									enum _BUTTON button = getchar();
									int x = getchar() - 32;
									int y = getchar() - 32;
									tt->mousecallback(button, x, y);
									resetInput();
								} break;
							case 'P': inputTooltip(tt, mod, XK_Delete, 0); break;
						} break;
					default:
						if (input <= 0x20) inputTooltip(tt, mod|Mod1Mask|ControlMask, input+0x40, 0);
						else               inputTooltip(tt, mod|Mod1Mask            , input     , 0);
						break;
				} break;
			case '\n': case '\r': inputTooltip(tt, mod, XK_Return, 0); break;
			case '\b': case 127:  inputTooltip(tt, mod, XK_BackSpace, 0); break;
			default:
				if (input <= 0x20) inputTooltip(tt, mod|ControlMask, input+0x40, 0);
				else               inputTooltip(tt, mod            , input     , 0);
				break;
		}
}
