Display *dpy;
pthread_t xeventthread_id;
Window wpy;

void inputX11AutoRepeatOn(void) { XAutoRepeatOn(dpy); }
void inputX11AutoRepeatOff(void) { XAutoRepeatOff(dpy); }

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

bool inputX11Init(void)
{
	if (!(dpy = XOpenDisplay(NULL))) return 1;
	int revtoret;
	XGetInputFocus(dpy, &wpy, &revtoret);
	XGrabKey(dpy, AnyKey, AnyModifier, wpy, 1, GrabModeAsync, GrabModeAsync);
	pthread_create(&xeventthread_id, NULL, (void*(*)(void*))XEventThread, &p);
	return 0;
}

void inputX11Free(void)
{
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
	inputX11AutoRepeatOn();
	XCloseDisplay(dpy);
}

const InputAPI x11_input_api =
{
	inputX11Init,
	inputX11Free,
	inputX11AutoRepeatOn,
	inputX11AutoRepeatOff,
	1
};
