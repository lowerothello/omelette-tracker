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

static void _previewNote(UI *cw, uint8_t note, uint8_t inst)
{
	cw->previewrow.macro[0].c = '\0';
	cw->previewrow.inst = inst;
	cw->previewrow.note = note;
}
void previewNote(uint8_t note, uint8_t inst)
{
	if (p->w->page == PAGE_VARIANT && p->s->playing) return;
	_previewNote(w, note, inst);
	w->previewtrigger = PTRIG_NORMAL;
}
void previewRow(Row *r)
{
	if (w->page == PAGE_VARIANT && s->playing) return;
	memcpy(&w->previewrow, r, sizeof(Row));
	w->previewtrigger = PTRIG_NORMAL;
}
void previewFileNote(UI *cw, uint8_t note)
{
	_previewNote(cw, note, 0); /* inst arg is unused so it doesn't matter */
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
