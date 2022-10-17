void sampleApplyTrimming(Instrument *iv)
{
	if (iv->sample.length > 0)
	{
		// malloc a new buffer
		short *sampledata = malloc(sizeof(short) * iv->trimlength * iv->sample.channels);
		if (sampledata == NULL)
		{
			strcpy(w->command.error, "failed to apply trim, out of memory");
			return;
		}

		memcpy(sampledata,
				iv->sample.data+(sizeof(short) * iv->trimstart),
				sizeof(short) * iv->trimlength * iv->sample.channels);

		free(iv->sample.data); iv->sample.data = NULL;
		iv->sample.data = sampledata;
		iv->sample.length = iv->trimlength;
		iv->trimstart = 0;
	}
}
int sampleExportCallback(char *command, unsigned char *mode)
{
	if (!instrumentSafe(s, w->instrument)) return 1;
	Instrument *iv = &s->instrument->v[s->instrument->i[w->instrument]];

	sampleApplyTrimming(iv);
	if (!iv->sample.data) return 1;

	char *buffer = malloc(strlen(command) + 1);
	wordSplit(buffer, command, 0);

	SNDFILE *sndfile;
	SF_INFO sfinfo;
	memset(&sfinfo, 0, sizeof(sfinfo));

	sfinfo.samplerate = iv->sample.rate;
	sfinfo.frames = iv->sample.length;
	sfinfo.channels = iv->sample.channels;

	sfinfo.format = SF_FORMAT_WAV | SF_FORMAT_PCM_16;
	sndfile = sf_open(fileExtension(buffer, ".wav"), SFM_WRITE, &sfinfo);
	if (sndfile == NULL) { free(buffer); return 1; }

	// write the sample data to disk
	sf_writef_short(sndfile, iv->sample.data, iv->sample.length * iv->sample.channels);
	sf_close(sndfile);

	free(buffer);
	return 0;
}
void resetWaveform(void)
{
	// if (s->instrument->i[w->instrument] < s->instrument->c)
	if (instrumentSafe(s, w->instrument))
	{
		w->waveformdrawpointer = 0;
		p->dirty = 1;
	}
}
/* TODO: sample could already be loaded into p->semarg, reparent if so */
void sampleLoadCallback(char *path)
{
	if (path) loadSample(w->instrument, path);

	w->page = PAGE_INSTRUMENT_SAMPLE;
	w->mode = I_MODE_NORMAL;
	resetWaveform();
}

void instrumentModeToIndices(void) { w->mode = I_MODE_INDICES; }
void instrumentModeToNormal (void) { w->mode = I_MODE_NORMAL; }

/* void instrumentOpenFilebrowser(void)
{
	if (!instrumentSafe(s, w->instrument)) addInstrument(w->instrument, 0, cb_addInstrument);
	w->page = PAGE_FILEBROWSER;
	w->fyoffset = 0;
	w->oldmode = I_MODE_NORMAL;
	w->mode = 0;
	w->dirx = w->diry = 0;
	w->dirh = ws.ws_row - 1;
	w->dirw = ws.ws_col;
	changeDirectory();
	w->filebrowserCallback = &sampleLoadCallback;
} */

void chordRecordToggle(void *_)
{
	if (w->instrumentrecv != INST_REC_LOCK_OK && w->instrumentreci != w->instrument)
	{ /* stop whichever instrument is already recording */
		toggleRecording(w->instrumentreci, 0);
	} else
	{
		if (!instrumentSafe(s, w->instrument)) addInstrument(w->instrument, 0, cb_addRecordInstrument);
		else                                   toggleRecording(w->instrument, 0);
	}
}
void chordRecordCueToggle(void *_)
{
	if (w->instrumentrecv != INST_REC_LOCK_OK && w->instrumentreci != w->instrument)
	{ /* stop whichever instrument is already recording */
		toggleRecording(w->instrumentreci, 1);
	} else
	{
		if (!instrumentSafe(s, w->instrument)) addInstrument(w->instrument, 0, cb_addRecordCueInstrument);
		else                                   toggleRecording(w->instrument, 1);
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
	addTooltipBind(&tt, "cue recording       ", 'q', chordRecordCueToggle, NULL);
	addTooltipBind(&tt, "cancel recording    ", 'c', chordRecordCancel, NULL);
	w->chord = 'r';
}
/* void setChordSample(void)
{
	clearTooltip(&tt);
	setTooltipTitle(&tt, "sample");
	addTooltipBind(&tt, "record new sample   ", 'r', chordRecordToggle, NULL);
	addTooltipBind(&tt, "cue toggle recording", 'q', chordRecordCueToggle, NULL);
	addTooltipBind(&tt, "cancel recording    ", 'c', chordRecordCancel, NULL);
	w->chord = 's';
} */

void chordAddSample(void *_) { addInstrument(w->instrument, INST_ALG_SIMPLE, cb_addInstrument); p->dirty = 1; }
void chordAddMIDI  (void *_) { addInstrument(w->instrument, INST_ALG_MIDI,   cb_addInstrument); p->dirty = 1; }

void setChordAddInst(void)
{
	clearTooltip(&tt);
	setTooltipTitle(&tt, "add");
	addTooltipBind(&tt, "sample", 'a', chordAddSample, NULL);
	addTooltipBind(&tt, "MIDI  ", 'm', chordAddMIDI, NULL);
	w->chord = 'a';
}
