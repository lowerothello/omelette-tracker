#define I_MODE_INDICES 0
#define I_MODE_NORMAL  1

void sampleApplyTrimming(Instrument *iv)
{
	if (iv->samplelength > 0)
	{
		// malloc a new buffer
		short *sampledata = malloc(sizeof(short) * iv->trimlength * iv->channels);
		if (sampledata == NULL)
		{
			strcpy(w->command.error, "failed to apply trim, out of memory");
			return;
		}

		memcpy(sampledata,
				iv->sampledata+(sizeof(short) * iv->trimstart),
				sizeof(short) * iv->trimlength * iv->channels);

		free(iv->sampledata); iv->sampledata = NULL;
		iv->sampledata = sampledata;
		iv->samplelength = iv->trimlength * iv->channels;
		iv->length = iv->trimlength;
		iv->trimstart = 0;
	}
}
int sampleExportCallback(char *command, unsigned char *mode)
{
	if (!instrumentSafe(s, w->instrument)) return 1;
	Instrument *iv = &s->instrument->v[s->instrument->i[w->instrument]];

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
	// if (s->instrument->i[w->instrument] < s->instrument->c)
	if (instrumentSafe(s, w->instrument))
	{
		w->waveformwidth = s->instrument->v[s->instrument->i[w->instrument]].length;
		w->waveformcursor = 0;
		w->waveformdrawpointer = 0;
		p->dirty = 1;
	}
}
void sampleLoadCallback(char *path)
{
	if (path) loadSample(w->instrument, path);

	w->page = PAGE_INSTRUMENT_SAMPLE;
	w->mode = w->oldmode;
	resetWaveform();
}

void instrumentModeToIndices(void) { w->mode = I_MODE_INDICES; }
void instrumentModeToNormal (void) { w->mode = I_MODE_NORMAL; }

void instrumentOpenFilebrowser(void)
{
	if (!instrumentSafe(s, w->instrument)) addInstrument(w->instrument);
	w->page = PAGE_FILEBROWSER;
	w->fyoffset = 0;
	w->oldmode = I_MODE_NORMAL;
	w->mode = 0;
	w->filebrowserCallback = &sampleLoadCallback;
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
	} p->dirty = 1;
}

void chordRecordToggle(void *_)
{
	if (w->instrumentrecv != INST_REC_LOCK_OK && w->instrumentreci != w->instrument)
	{ /* stop whichever instrument is already recording */
		toggleRecording(w->instrumentreci, 0);
	} else
	{
		if (!instrumentSafe(s, w->instrument)) addInstrument(w->instrument);
		toggleRecording(w->instrument, 0);
	}
}
void chordRecordCueToggle(void *_)
{
	if (w->instrumentrecv != INST_REC_LOCK_OK && w->instrumentreci != w->instrument)
	{ /* stop whichever instrument is already recording */
		toggleRecording(w->instrumentreci, 1);
	} else
	{
		if (!instrumentSafe(s, w->instrument)) addInstrument(w->instrument);
		toggleRecording(w->instrument, 1);
	}
}
void chordRecordCancel(void *_)
{
	if (w->instrumentrecv != INST_REC_LOCK_OK)
		w->instrumentrecv = INST_REC_LOCK_PREP_CANCEL;
}
void setChordRecord(void)
{
	clearTooltip(&tt);
	setTooltipTitle(&tt, "record");
	addTooltipBind(&tt, "toggle recording now", 'r', chordRecordToggle, NULL);
	addTooltipBind(&tt, "cue toggle recording", 'q', chordRecordCueToggle, NULL);
	addTooltipBind(&tt, "cancel recording    ", 'c', chordRecordCancel, NULL);
	w->chord = 'r';
}

/* void chordZoomReset(void *_)
{
	if (instrumentSafe(s, w->instrument))
	{
		w->waveformwidth = s->instrument->v[s->instrument->i[w->instrument]].length;
		w->waveformdrawpointer = 0;
	}
}
void chordZoomIn(void *_)
{
	if (instrumentSafe(s, w->instrument))
	{
		for (int i = MAX(1, w->count); i > 0; i--)
			w->waveformwidth /= 2;
		w->waveformdrawpointer = 0;
	}
}
void chordZoomOut(void *_)
{
	if (instrumentSafe(s, w->instrument))
	{
		for (int i = MAX(1, w->count); i > 0; i--)
			w->waveformwidth = MIN(s->instrument->v[s->instrument->i[w->instrument]].length, w->waveformwidth*2);
		w->waveformdrawpointer = 0;
	}
}
void setChordZoom(void)
{
	clearTooltip(&tt);
	setTooltipTitle(&tt, "zoom");
	addTooltipBind(&tt, "reset zoom", 'z', chordZoomReset, NULL);
	addTooltipBind(&tt, "zoom in   ", 'i', chordZoomIn, NULL);
	addTooltipBind(&tt, "zoom out  ", 'o', chordZoomOut, NULL);
	w->chord = 'z';
} */
