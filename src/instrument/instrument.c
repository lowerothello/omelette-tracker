#define I_MODE_INDICES 0
#define I_MODE_NORMAL 1
#define I_MODE_PREVIEW 2
#define I_MODE_VISUAL 5
#define I_MODE_INDICES_PREVIEW 6

#define MAX_INSTRUMENT_INDEX 15

#define INSTRUMENT_BODY_COLS 70
#define INSTRUMENT_BODY_ROWS 20
#define INSTRUMENT_TYPE_ROWS 14

void sampleApplyTrimming(instrument *iv)
{
	if (iv->samplelength > 0)
	{
		uint32_t newlen
			= MAX(iv->trim[0], iv->trim[1])
			- MIN(iv->trim[0], iv->trim[1]) + 1;

		// malloc a new buffer
		short *sampledata = malloc(sizeof(short) * newlen * iv->channels);
		if (sampledata == NULL)
		{
			strcpy(w->command.error, "failed to apply trim, out of memory");
			return;
		}

		uint32_t startOffset = MIN(iv->trim[0], iv->trim[1]);
		memcpy(sampledata,
				iv->sampledata+(sizeof(short) * startOffset),
				sizeof(short) * newlen * iv->channels);

		free(iv->sampledata); iv->sampledata = NULL;
		iv->sampledata = sampledata;
		iv->samplelength = newlen * iv->channels;
		iv->length = newlen;
		iv->trim[0] = iv->trim[0] - startOffset;
		iv->trim[1] = iv->trim[1] - startOffset;
	}
}
int sampleExportCallback(char *command, unsigned char *mode)
{
	if (s->instrumenti[w->instrument] >= s->instrumentc) return 1;
	instrument *iv = &s->instrumentv[s->instrumenti[w->instrument]];

	sampleApplyTrimming(iv);
	if (!iv->sampledata) return 1;

	char *buffer = malloc(strlen(command) + 1);
	wordSplit(buffer, command, 0);

	SNDFILE *sndfile;
	SF_INFO sfinfo;
	memset(&sfinfo, 0, sizeof(sfinfo));

	sfinfo.samplerate = iv->c5rate;
	sfinfo.frames = iv->length;
	sfinfo.channels = iv->channels;

	sfinfo.format = SF_FORMAT_WAV | SF_FORMAT_PCM_16;
	sndfile = sf_open(fileExtension(buffer, ".wav"), SFM_WRITE, &sfinfo);
	if (sndfile == NULL) { free(buffer); return 1; }

	// write the sample data to disk
	sf_writef_short(sndfile, iv->sampledata, iv->length);
	sf_close(sndfile);

	free(buffer);
	return 0;
}
void resetWaveform(void)
{
	if (s->instrumenti[w->instrument] < s->instrumentc)
	{
		w->waveformwidth = s->instrumentv[s->instrumenti[w->instrument]].length;
		w->waveformcursor = 0;
		w->waveformdrawpointer = 0;
		redraw();
	}
}
void sampleLoadCallback(char *path)
{
	loadSample(w->instrument, path);
	w->popup = 1;
	w->mode = w->oldmode;
	resetWaveform();
}

void instrumentUpArrow(int count)
{
	switch (w->mode)
	{
		case I_MODE_INDICES: case I_MODE_INDICES_PREVIEW:
			w->instrument -= count;
			if (w->instrument < 0) w->instrument = 0;
			resetWaveform();
			break;
		case I_MODE_VISUAL: w->mode = I_MODE_NORMAL; w->waveformdrawpointer = 0; redraw();
		case I_MODE_NORMAL: case I_MODE_PREVIEW:
			decControlCursor(&cc, count);
			if (!cc.cursor) w->waveformdrawpointer = 0;
			break;
	}
}
void instrumentDownArrow(int count)
{
	switch (w->mode)
	{
		case I_MODE_INDICES: case I_MODE_INDICES_PREVIEW:
			w->instrument += count;
			if (w->instrument > 254) w->instrument = 254;
			resetWaveform();
			break;
		case I_MODE_VISUAL: w->mode = I_MODE_NORMAL; w->waveformdrawpointer = 0; redraw();
		case I_MODE_NORMAL: case I_MODE_PREVIEW:
			incControlCursor(&cc, count);
			break;
	}
}
void instrumentLeftArrow(void)
{
	uint32_t delta;
	switch (w->mode)
	{
		case I_MODE_NORMAL: case I_MODE_PREVIEW: case I_MODE_VISUAL:
			if (!cc.cursor)
			{
				if (s->instrumenti[w->instrument] < s->instrumentc)
				{
					delta = w->waveformwidth / WAVEFORM_COARSE_SLICES;
					if (delta > w->waveformcursor) w->waveformcursor = 0;
					else                           w->waveformcursor -= delta;
					w->waveformdrawpointer = 0; redraw();
				}
			} else incControlFieldpointer(&cc);
			break;
	}
}
void instrumentRightArrow(void)
{
	uint32_t delta;
	switch (w->mode)
	{
		case I_MODE_NORMAL: case I_MODE_PREVIEW: case I_MODE_VISUAL:
			if (!cc.cursor)
			{
				if (s->instrumenti[w->instrument] < s->instrumentc)
				{
					delta = w->waveformwidth / WAVEFORM_COARSE_SLICES;
					if (delta > s->instrumentv[s->instrumenti[w->instrument]].length-1 - w->waveformcursor)
						w->waveformcursor = s->instrumentv[s->instrumenti[w->instrument]].length-1;
					else
						w->waveformcursor += delta;
					w->waveformdrawpointer = 0; redraw();
				}
			} else decControlFieldpointer(&cc);
			break;
	}
}
void instrumentCtrlLeftArrow(void)
{
	uint32_t delta;
	if (!cc.cursor && s->instrumenti[w->instrument] < s->instrumentc)
		switch (w->mode)
		{
			case I_MODE_NORMAL: case I_MODE_PREVIEW: case I_MODE_VISUAL:
				delta = w->waveformwidth / WAVEFORM_FINE_SLICES;
				if (delta > w->waveformcursor) w->waveformcursor = 0;
				else                           w->waveformcursor -= delta;
				w->waveformdrawpointer = 0; redraw(); break;
		}
}
void instrumentCtrlRightArrow(void)
{
	uint32_t delta;
	if (!cc.cursor && s->instrumenti[w->instrument] < s->instrumentc)
		switch (w->mode)
		{
			case I_MODE_NORMAL: case I_MODE_PREVIEW: case I_MODE_VISUAL:
				delta = w->waveformwidth / WAVEFORM_FINE_SLICES;
				if (delta > s->instrumentv[s->instrumenti[w->instrument]].length-1 - w->waveformcursor)
					w->waveformcursor = s->instrumentv[s->instrumenti[w->instrument]].length-1;
				else
					w->waveformcursor += delta;
				w->waveformdrawpointer = 0; redraw(); break;
		}
}
void instrumentHome(void)
{
	switch (w->mode)
	{
		case I_MODE_INDICES: case I_MODE_INDICES_PREVIEW:
			w->instrument = 0;
			resetWaveform();
			break;
		case I_MODE_NORMAL: case I_MODE_PREVIEW: case I_MODE_VISUAL:
			if (cc.cursor) setControlCursor(&cc, 1);
			else if (s->instrumenti[w->instrument] < s->instrumentc)
			{
				w->waveformcursor = 0;
				w->waveformdrawpointer = 0;
			} redraw(); break;
	}
}
void instrumentEnd(void)
{
	switch (w->mode)
	{
		case I_MODE_INDICES: case I_MODE_INDICES_PREVIEW:
			w->instrument = 254;
			resetWaveform();
			break;
		case I_MODE_NORMAL: case I_MODE_PREVIEW: case I_MODE_VISUAL:
			if (cc.cursor) setControlCursor(&cc, cc.controlc-1);
			else if (s->instrumenti[w->instrument] < s->instrumentc)
			{
				w->waveformcursor = s->instrumentv[s->instrumenti[w->instrument]].length-1;
				w->waveformdrawpointer = 0;
			} redraw(); break;
	}
}

void instrumentModeToIndices(void)
{
	switch (w->mode)
	{
		case I_MODE_PREVIEW: case I_MODE_INDICES_PREVIEW:
			w->mode = I_MODE_INDICES_PREVIEW; break;
		default:
			w->mode = I_MODE_INDICES; break;
	}
}
void instrumentModeToNormal(void)
{
	switch (w->mode)
	{
		case I_MODE_PREVIEW: case I_MODE_INDICES_PREVIEW:
			w->mode = I_MODE_PREVIEW; break;
		case I_MODE_INDICES:
			w->mode = I_MODE_NORMAL; break;
	}
}

void toggleRecording(uint8_t inst, char cue)
{
	if (w->instrumentrecv == INST_REC_LOCK_OK) w->instrumentreci = inst;
	if (w->instrumentreci == inst)
	{
		switch (w->instrumentrecv)
		{
			case INST_REC_LOCK_OK:
				w->recbuffer = malloc(sizeof(short) * RECORD_LENGTH * samplerate * 2);
				if (!w->recbuffer)
				{
					strcpy(w->command.error, "failed to start recording, out of memory");
					break;
				}
				w->recptr = 0;
				if (cue) w->instrumentrecv = INST_REC_LOCK_CUE_START;
				else     w->instrumentrecv = INST_REC_LOCK_START;
				break;
			default: w->instrumentrecv = INST_REC_LOCK_PREP_END; break;
		}
	} redraw();
}

void chordRecordToggle(void)
{
	if (w->instrumentrecv != INST_REC_LOCK_OK && w->instrumentreci != w->instrument)
	{ /* stop whichever instrument is already recording */
		toggleRecording(w->instrumentreci, 0);
	} else
	{
		if (s->instrumenti[w->instrument] >= s->instrumentc) addInstrument(w->instrument);
		toggleRecording(w->instrument, 0);
	}
}
void chordRecordCueToggle(void)
{
	if (w->instrumentrecv != INST_REC_LOCK_OK && w->instrumentreci != w->instrument)
	{ /* stop whichever instrument is already recording */
		toggleRecording(w->instrumentreci, 1);
	} else
	{
		if (s->instrumenti[w->instrument] >= s->instrumentc) addInstrument(w->instrument);
		toggleRecording(w->instrument, 1);
	}
}
void chordRecordCancel(void)
{
	if (w->instrumentrecv != INST_REC_LOCK_OK)
		w->instrumentrecv = INST_REC_LOCK_PREP_CANCEL;
}
void setChordRecord(void)
{
	clearTooltip(&tt);
	setTooltipTitle(&tt, "record");
	addTooltipBind(&tt, "toggle recording now", 'r', chordRecordToggle);
	addTooltipBind(&tt, "cue toggle recording", 'q', chordRecordCueToggle);
	addTooltipBind(&tt, "cancel recording    ", 'c', chordRecordCancel);
	w->chord = 'r';
}

void chordZoomReset(void)
{
	if (s->instrumenti[w->instrument] < s->instrumentc)
	{
		w->waveformwidth = s->instrumentv[s->instrumenti[w->instrument]].length;
		w->waveformdrawpointer = 0;
	}
}
void chordZoomIn(void)
{
	if (s->instrumenti[w->instrument] < s->instrumentc)
	{
		for (int i = MAX(1, w->count); i > 0; i--)
			w->waveformwidth /= 2;
		w->waveformdrawpointer = 0;
	}
}
void chordZoomOut(void)
{
	if (s->instrumenti[w->instrument] < s->instrumentc)
	{
		for (int i = MAX(1, w->count); i > 0; i--)
			w->waveformwidth = MIN(s->instrumentv[s->instrumenti[w->instrument]].length, w->waveformwidth*2);
		w->waveformdrawpointer = 0;
	}
}
void setChordZoom(void)
{
	clearTooltip(&tt);
	setTooltipTitle(&tt, "zoom");
	addTooltipBind(&tt, "reset zoom", 'z', chordZoomReset);
	addTooltipBind(&tt, "zoom in   ", 'i', chordZoomIn);
	addTooltipBind(&tt, "zoom out  ", 'o', chordZoomOut);
	w->chord = 'z';
}
