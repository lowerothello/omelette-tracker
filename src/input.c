/* *note is allowed to be null             */
/* returns true if a valid pad was pressed */
/* TODO: deprecated */
static int charToKmode(int key, bool linknibbles, uint8_t *macrov, uint8_t *note)
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
static void charToNote(int key, uint8_t *note)
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

static void _previewNote(UI *cw, int key, uint8_t inst)
{
	cw->previewrow.macro[0].c = '\0';
	cw->previewrow.inst = inst;

	if (cw->keyboardmacro)
	{
		if (charToKmode(key, MACRO_LINKNIBBLES(cw->keyboardmacro), &cw->previewrow.macro[0].v, &cw->previewrow.note))
			cw->previewrow.macro[0].c = cw->keyboardmacro;
	} else charToNote(key, &cw->previewrow.note);
}
void previewNote(int key, uint8_t inst)
{
	if (p->w->page == PAGE_VARIANT && p->s->playing) return;
	_previewNote(w, key, inst);
	w->previewtrigger = PTRIG_NORMAL;
}
void previewRow(Row *r)
{
	if (w->page == PAGE_VARIANT && s->playing) return;
	memcpy(&w->previewrow, r, sizeof(Row));
	w->previewtrigger = PTRIG_NORMAL;
}
void previewFileNote(UI *cw, int key)
{
	_previewNote(cw, key, 0); /* inst arg is unused so it doesn't matter */
	w->previewtrigger = PTRIG_FILE;
}

void incControlValueRedraw(ControlState *cc) { incControlValue (cc); p->redraw = 1; }
void decControlValueRedraw(ControlState *cc) { decControlValue (cc); p->redraw = 1; }
void toggleKeyControlRedraw(ControlState *cc) { toggleKeyControl(cc); p->redraw = 1; }
void revertKeyControlRedraw(ControlState *cc) { revertKeyControl(cc); p->redraw = 1; }


static enum InputMode getRawInputMode(void)
{
#ifdef DISABLE_RAW_INPUT
	return INPUMODE_NONE;
#endif
	if (getenv("OML_STDIN"))              return INPUMODE_NONE;
	if (!strcmp(getenv("TERM"), "LINUX")) return INPUMODE_RAW;
	if (getenv("DISPLAY"))                return INPUMODE_X;
	return INPUMODE_NONE; /* fallback */
}


Display *dpy;
pthread_t xeventthread_id;
Window wpy;
static void *XEventThread(PlaybackInfo *arg)
{
	Event  e;        /* omelette event */
	XEvent ev, *evp; /* Xorg events    */
	while (1)
	{
		XNextEvent(dpy, &ev);

		if (p->xeventthreadsem)
		{
			p->xeventthreadsem++;
			return NULL;
		}

		e.sem = M_SEM_INPUT;
		evp = malloc(sizeof(XEvent));
		memcpy(evp, &ev, sizeof(XEvent));
		e.callbackarg = evp;
		pushEvent(&e);
	}
	return NULL;
}

enum InputMode input_mode;

/* stdin is initialized by initTerminal() */
/* returns true for failure */
int initRawInput(void)
{
	input_mode = getRawInputMode();

	int revtoret;
	switch (input_mode)
	{
		case INPUMODE_RAW:
			// ioctl(0, KDSKBMODE, K_RAW); /* TODO: */
			break;
		case INPUMODE_X:
			if (!(dpy = XOpenDisplay(NULL))) return 1;
			XGetInputFocus(dpy, &wpy, &revtoret);
			XGrabKey(dpy, AnyKey, AnyModifier, wpy, 1, GrabModeAsync, GrabModeAsync);
			pthread_create(&xeventthread_id, NULL, (void*(*)(void*))XEventThread, &p);
			break;
		case INPUMODE_NONE: break;
	}
	return 0;
}

/* stdin is free'd by cleanupTerminal() */
void freeRawInput(void)
{
	switch (input_mode)
	{
		case INPUMODE_RAW:
			// ioctl(0, KDSKBMODE, K_XLATE);
			break;
		case INPUMODE_X:
			/* the xevent thread will die when this bit is set high after
			 * the next event is recieved. it's usually killed by the
			 * release event of whatever key triggered cleanup, but not
			 * always. sometimes it just stays alive for a bit. idk how
			 * to make it more consistent other than maybe faking an
			 * xevent? idk */
			p->xeventthreadsem = 1;

			pthread_join(xeventthread_id, NULL);

			XUngrabKey(dpy, AnyKey, AnyModifier, wpy);
			XCloseDisplay(dpy);
			break;
		case INPUMODE_NONE: break;
	}
}
