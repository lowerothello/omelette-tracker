enum InputMode input_mode;

static void tooltipAddCount(void *add)
{
	w->count *= 10;
	w->count += (size_t)add;
	p->redraw = 1;
}
void addCountBinds(bool draw)
{
	unsigned int flags = TT_DEAD;
	if (draw) flags |= TT_DRAW;
	addTooltipBind("add 0 to count", 0, XK_0, flags, tooltipAddCount, (void*)0);
	addTooltipBind("add 1 to count", 0, XK_1, flags, tooltipAddCount, (void*)1);
	addTooltipBind("add 2 to count", 0, XK_2, flags, tooltipAddCount, (void*)2);
	addTooltipBind("add 3 to count", 0, XK_3, flags, tooltipAddCount, (void*)3);
	addTooltipBind("add 4 to count", 0, XK_4, flags, tooltipAddCount, (void*)4);
	addTooltipBind("add 5 to count", 0, XK_5, flags, tooltipAddCount, (void*)5);
	addTooltipBind("add 6 to count", 0, XK_6, flags, tooltipAddCount, (void*)6);
	addTooltipBind("add 7 to count", 0, XK_7, flags, tooltipAddCount, (void*)7);
	addTooltipBind("add 8 to count", 0, XK_8, flags, tooltipAddCount, (void*)8);
	addTooltipBind("add 9 to count", 0, XK_9, flags, tooltipAddCount, (void*)9);
}

void previewNote(uint8_t note, uint8_t inst, bool release)
{
	if (p->w->page == PAGE_VARIANT && p->s->playing) return;
	Event ev;
	ev.sem = M_SEM_PREVIEW;
	ev.arg1 = note;
	ev.arg2 = inst;
	ev.arg3 = release;
	pushEvent(&ev);
}
void previewRow(Row *r, bool release)
{
	previewNote(r->note, r->inst, release);
}
void previewFileNote(uint8_t note, bool release)
{
	Event ev;
	ev.sem = M_SEM_PREVIEW;
	ev.arg1 = note;
	ev.arg2 = -1;
	ev.arg3 = release;
	pushEvent(&ev);
}

/* returns -1 to do nothing */
int getPreviewVoice(uint8_t note, bool release)
{
	/* monophonic preview if key release events are unavailable */
	if (input_mode == INPUTMODE_NONE)
		return 0;

	int emptyslot = -1;

	int      oldestslot = -1;
	uint32_t oldestslotpointer = 0;

	for (int i = 0; i < PREVIEW_TRACKS; i++)
	{
		if (w->previewtrack[i].r.note == note)
			return i;

		if (w->previewtrack[i].r.note == NOTE_VOID)
			emptyslot = i;
		else if (w->previewtrack[i].pointer > oldestslotpointer)
		{
			oldestslot = i;
			oldestslotpointer = w->previewtrack[i].pointer;
		}
	}

	if (release) return -1;

	/* use the first empty slot if there are any */
	if (emptyslot != -1) return emptyslot;

	/* use the oldest slot */
	return oldestslot;
}

void incControlValueRedraw(void) { incControlValue(); p->redraw = 1; }
void decControlValueRedraw(void) { decControlValue(); p->redraw = 1; }
void toggleKeyControlRedraw(void) { toggleKeyControl(); p->redraw = 1; }
void revertKeyControlRedraw(void) { revertKeyControl(); p->redraw = 1; }


static enum InputMode getRawInputMode(void)
{
#ifdef DISABLE_RAW_INPUT
	return INPUTMODE_NONE;
#endif

	if (getenv("OML_STDIN"))              return INPUTMODE_NONE;
	if (!strcmp(getenv("TERM"), "LINUX")) return INPUTMODE_RAW;

#ifdef OML_X11
	if (getenv("DISPLAY"))                return INPUTMODE_X;
#endif

	return INPUTMODE_NONE; /* fallback */
}


#ifdef OML_X11
Display *dpy;
pthread_t xeventthread_id;
Window wpy;
static void *XEventThread(PlaybackInfo *arg)
{
	Event  e;        /* omelette event */
	XEvent ev, *evp; /* Xorg events    */

	int keyspressed = 0;

	e.sem = M_SEM_INPUT;
	while (1)
	{
		XNextEvent(dpy, &ev);

		if (p->xeventthreadsem)
		{
			p->xeventthreadsem++;
			return NULL;
		}

		switch (ev.type)
		{
			case KeyPress:
				if (!keyspressed)
					XGrabKeyboard(dpy, wpy, 1, GrabModeAsync, GrabModeAsync, CurrentTime);
				keyspressed++;
				break;
			case KeyRelease:
				keyspressed--;
				if (!keyspressed)
					XUngrabKeyboard(dpy, CurrentTime);
				break;
			case Expose:
				while (XCheckTypedEvent(dpy, Expose, &ev));
				continue;
			default: continue;
		}

		evp = malloc(sizeof(XEvent));
		memcpy(evp, &ev, sizeof(XEvent));
		e.callbackarg = evp;
		pushEvent(&e);
	}
	return NULL;
}

void setAutoRepeatOn(void)
{
	if (input_mode == INPUTMODE_X)
		XAutoRepeatOn(dpy); /* assume key repeat is wanted on, TODO: use whatever was set before */
}
void setAutoRepeatOff(void)
{
	if (input_mode == INPUTMODE_X)
		XAutoRepeatOff(dpy);
}
#else
void setAutoRepeatOn (void) { }
void setAutoRepeatOff(void) { }
#endif

/* stdin is initialized by initTerminal() */
/* returns true for failure */
int initRawInput(void)
{
	input_mode = getRawInputMode();

	switch (input_mode)
	{
		case INPUTMODE_RAW:
			// ioctl(0, KDSKBMODE, K_RAW); /* TODO: */
			break;

#ifdef OML_X11
		case INPUTMODE_X:
			if (!(dpy = XOpenDisplay(NULL))) return 1;
			int revtoret;
			XGetInputFocus(dpy, &wpy, &revtoret);
			XGrabKey(dpy, AnyKey, AnyModifier, wpy, 1, GrabModeAsync, GrabModeAsync);
			pthread_create(&xeventthread_id, NULL, (void*(*)(void*))XEventThread, &p);
			break;
#endif

		case INPUTMODE_NONE: break;
	}
	return 0;
}

/* stdin is free'd by cleanupTerminal() */
void freeRawInput(void)
{
	switch (input_mode)
	{
		case INPUTMODE_RAW:
			// ioctl(0, KDSKBMODE, K_XLATE);
			break;

#ifdef OML_X11
		case INPUTMODE_X:
			/* the xevent thread will die when this bit is set high after
			 * the next event is recieved. it's usually killed by the
			 * release event of whatever key triggered cleanup, but not
			 * always. sometimes it just stays alive for a bit. idk how
			 * to make it more consistent other than maybe faking an
			 * xevent? idk */
			p->xeventthreadsem = 1;

			pthread_join(xeventthread_id, NULL);

			XUngrabKey(dpy, AnyKey, AnyModifier, wpy);
			XUngrabKeyboard(dpy, CurrentTime);
			setAutoRepeatOn();
			XCloseDisplay(dpy);
			break;
#endif

		case INPUTMODE_NONE: break;
	}
}
