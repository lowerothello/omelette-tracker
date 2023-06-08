void previewNote(uint8_t note, uint8_t inst, bool release)
{
	if ((p->w->page == PAGE_VARIANT
	  || p->w->page == PAGE_PATTERN)
	  && p->w->playing)
		return;

	Event ev;
	ev.sem = M_SEM_PREVIEW;
	ev.arg1 = note;
	ev.arg2 = inst;
	ev.arg3 = release;
	pushEvent(&ev);
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
	if (!input_api.poly)
		return 0;

	int emptyslot = -1;

	int      oldestslot = -1;
	uint32_t oldestslotpointer = 0;

	for (int i = 0; i < PREVIEW_TRACKS; i++)
	{
		if (w->previewtrack[i]->r.note == note && !w->previewtrack[i]->release)
			return i;

		if (w->previewtrack[i]->r.note == NOTE_VOID || w->previewtrack[i]->release)
			emptyslot = i;
		else if (w->previewtrack[i]->pointer > oldestslotpointer)
		{
			oldestslot = i;
			oldestslotpointer = w->previewtrack[i]->pointer;
		}
	}

	if (release) return -1;

	/* use the first empty slot if there are any */
	if (emptyslot != -1) return emptyslot;

	/* use the oldest slot */
	return oldestslot;
}

#define INPUT_GET_API_TRY(backend) if (backend##_input_api.init && !backend##_input_api.init()) { input_api = backend##_input_api; return; }
void inputInitAPI(void)
{
#ifdef DISABLE_RAW_INPUT
	INPUT_GET_API_TRY(stdin);
	return;
#endif

	if (getenv("OML_STDIN"))
	{
		INPUT_GET_API_TRY(stdin);
		return;
	}

	INPUT_GET_API_TRY(raw);

#ifdef OML_X11
	INPUT_GET_API_TRY(x11);
#endif
	INPUT_GET_API_TRY(stdin);
}
