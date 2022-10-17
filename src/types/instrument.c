void copyInstrument(Instrument *dest, Instrument *src) /* TODO: should be atomic */
{
	if (dest->sample.data)
	{ free(dest->sample.data); dest->sample.data = NULL; }

	for (uint8_t i = 0; i < dest->effect.c; i++)
		freeEffect(&dest->effect.v[i]);

	memcpy(dest, src, sizeof(Instrument));

	if (src->sample.data)
	{ /* only copy sampledata if it exists */
		dest->sample.data = malloc(sizeof(short) * src->sample.length * src->sample.channels);
		memcpy(dest->sample.data, src->sample.data, sizeof(short) * src->sample.length * src->sample.channels);
	}

	for (uint8_t i = 0; i < src->effect.c; i++)
		copyEffect(&dest->effect.v[i], &src->effect.v[i]);
}

/* frees the contents of an instrument */
void _delInstrument(Instrument *iv)
{
	if (iv->sample.data)
	{
		free(iv->sample.data);
		iv->sample.data = NULL;
	}

	for (uint8_t i = 0; i < iv->effect.c; i++)
		freeEffect(&iv->effect.v[i]);
	iv->effect.c = 0;
}

bool instrumentSafe(Song *cs, short index)
{
	if (index < 0) return 0; /* special instruments should be handled separately */
	if (index != INSTRUMENT_MAX && cs->instrument->i[index] < cs->instrument->c)
		return 1;
	return 0;
}

/* take a short* and reparent it under instrument iv               */
/* bufferlen is how many stereo pairs of samples are in the buffer */
void reparentSample(Instrument *iv, short *buffer, jack_nframes_t bufferlen, uint32_t rate)
{
	if (iv->sample.data) { free(iv->sample.data); iv->sample.data = NULL; }

	iv->sample.data = buffer;

	iv->sample.length = bufferlen;
	iv->sample.channels = 2;
	iv->sample.rate = iv->sample.defrate = rate; /* assume the buffer is at the system sample rate */
	iv->trimstart = 0;
	iv->trimlength = bufferlen-1;
	iv->wavetable.framelength = (bufferlen-1) / 256;
	iv->looplength = 0;
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

void cb_addInstrument         (Event *e) { free(e->swap2); e->swap2 = NULL; if (w->page == PAGE_INSTRUMENT_SAMPLE) w->mode = I_MODE_NORMAL; p->dirty = 1; }
void cb_addRecordInstrument   (Event *e) { free(e->swap2); e->swap2 = NULL; toggleRecording((size_t)e->callbackarg, 0); p->dirty = 1; }
void cb_addRecordCueInstrument(Event *e) { free(e->swap2); e->swap2 = NULL; toggleRecording((size_t)e->callbackarg, 1); p->dirty = 1; }
void cb_addPutInstrument      (Event *e) { free(e->swap2); e->swap2 = NULL; copyInstrument(&s->instrument->v[s->instrument->i[(size_t)e->callbackarg]], &w->instrumentbuffer); p->dirty = 1; }
InstrumentChain *_addInstrument(uint8_t index, int8_t algorithm)
{
	InstrumentChain *newinstrument = calloc(1, sizeof(InstrumentChain) + (s->instrument->c+1) * sizeof(Instrument));
	memcpy(newinstrument, s->instrument, sizeof(InstrumentChain) + s->instrument->c * sizeof(Instrument));

	newinstrument->v[newinstrument->c].algorithm = algorithm;
	newinstrument->v[newinstrument->c].samplerate = 0xff;
	newinstrument->v[newinstrument->c].bitdepth = 0xf;
	newinstrument->v[newinstrument->c].envelope = 0x00f0;
	newinstrument->v[newinstrument->c].filtercutoff = 0xff;

	newinstrument->v[newinstrument->c].granular.cyclelength = 0x3fff;
	newinstrument->v[newinstrument->c].granular.rampgrains = 8;

	newinstrument->i[index] = newinstrument->c;
	newinstrument->c++;

	return newinstrument;
}
int addInstrument(uint8_t index, int8_t algorithm, void (*cb)(Event *))
{ /* fully atomic */
	if (instrumentSafe(s, index)) return 1; /* index occupied */
	Event e;
	e.sem = M_SEM_SWAP_REQ;
	e.swap1 = s->instrument;
	e.swap2 = _addInstrument(index, algorithm);
	e.callback = cb;
	e.callbackarg = (void *)(size_t)index;
	pushEvent(&e);
	return 0;
}

typedef struct
{
	short         *buffer;
	jack_nframes_t buflen;
	uint32_t       rate;
	uint8_t        index;
} InstrumentAddReparentArg;
void cb_addReparentInstrument(Event *e)
{
	InstrumentAddReparentArg *castarg = e->callbackarg;
	free(e->swap2); e->swap2 = NULL;
	reparentSample(&s->instrument->v[s->instrument->i[castarg->index]], castarg->buffer, castarg->buflen, castarg->rate);
	free(e->callbackarg); e->callbackarg = NULL;
	p->dirty = 1;
}
int addReparentInstrument(uint8_t index, int8_t algorithm, short *buffer, jack_nframes_t buflen, uint32_t rate)
{ /* fully atomic */
	if (instrumentSafe(s, index)) return 1; /* index occupied */

	InstrumentAddReparentArg *arg = malloc(sizeof(InstrumentAddReparentArg));
	arg->buffer = buffer;
	arg->buflen = buflen;
	arg->rate = rate;
	arg->index = index;

	Event e;
	e.sem = M_SEM_SWAP_REQ;
	e.swap1 = s->instrument;
	e.swap2 = _addInstrument(index, algorithm);
	e.callback = cb_addReparentInstrument;
	e.callbackarg = arg;
	pushEvent(&e);
	return 0;
}

/* returns -1 if no instrument slots are free */
short emptyInstrument(uint8_t min)
{
	for (int i = min; i < INSTRUMENT_MAX; i++)
		if (!instrumentSafe(s, i)) return i;
	return -1;
}

void yankInstrument(uint8_t index)
{
	if (!instrumentSafe(s, index)) return; /* nothing to copy */
	_delInstrument(&w->instrumentbuffer);
	copyInstrument(&w->instrumentbuffer, &s->instrument->v[s->instrument->i[index]]);
}

void putInstrument(uint8_t index)
{
	if (s->instrument->i[index] >= s->instrument->c) addInstrument(index, 0, cb_addPutInstrument);
	else copyInstrument(&s->instrument->v[s->instrument->i[index]], &w->instrumentbuffer);
}

void cb_delInstrument(Event *e)
{
	_delInstrument(&((InstrumentChain *)e->swap2)->v[(size_t)e->callbackarg]);
	free(e->swap2); e->swap2 = NULL;
	p->dirty = 1;
}
int delInstrument(uint8_t index)
{ /* fully atomic */
	if (!instrumentSafe(s, index)) return 1; /* instrument doesn't exist */

	size_t cutindex = s->instrument->i[index]; /* cast to void* later */
	_delInstrument(&s->instrument->v[cutindex]);

	InstrumentChain *newinstrument = calloc(1, sizeof(InstrumentChain) + s->instrument->c * sizeof(Instrument));
	memcpy(newinstrument, s->instrument, sizeof(InstrumentChain)); /* copy just the header */

	if (cutindex > 0)
		memcpy(newinstrument->v,
				s->instrument->v,
				sizeof(Instrument)*(cutindex+1));

	if (cutindex < s->instrument->c-1)
		memcpy(&newinstrument->v[cutindex],
				&s->instrument->v[cutindex+1],
				sizeof(Instrument)*(s->instrument->c-(cutindex+1)));

	newinstrument->i[index] = INSTRUMENT_VOID;
	/* backref contiguity */
	for (uint8_t i = 0; i < 255; i++)
		if (newinstrument->i[i] >= cutindex && newinstrument->i[i] < newinstrument->c)
			newinstrument->i[i]--;

	newinstrument->c--;

	Event e;
	e.sem = M_SEM_SWAP_REQ;
	e.swap1 = s->instrument;
	e.swap2 = newinstrument;
	e.callback = cb_delInstrument;
	e.callbackarg = (void *)cutindex;
	pushEvent(&e);
	return 0;
}

short *_loadSample(char *path, SF_INFO *sfinfo)
{
	fcntl(0, F_SETFL, 0); /* blocking */
	memset(sfinfo, 0, sizeof(SF_INFO));

	SNDFILE *sndfile = sf_open(path, SFM_READ, sfinfo);
	short *ptr;

	if (!sndfile)
	{ /* raw file */
		struct stat buf;
		if (stat(path, &buf) == -1)
		{
			fcntl(0, F_SETFL, O_NONBLOCK); /* non-blocking */
			return NULL;
		}

		ptr = malloc(buf.st_size - buf.st_size % sizeof(short));
		if (!ptr) // malloc failed
		{
			fcntl(0, F_SETFL, O_NONBLOCK); /* non-blocking */
			return NULL;
		} else
		{
			/* read the whole file into memory */
			FILE *fp = fopen(path, "r");
			fread(ptr, sizeof(short), buf.st_size / sizeof(short), fp);
			fclose(fp);

			/* spoof data */
			sfinfo->channels = 1;
			sfinfo->frames = buf.st_size / sizeof(short);
			sfinfo->samplerate = 12000;
		}
	} else /* audio file */
	{
		if (sfinfo->channels > 2) /* fail on high channel files */
		{
			sf_close(sndfile);
			fcntl(0, F_SETFL, O_NONBLOCK); /* non-blocking */
			return NULL;
		}

		ptr = malloc(sizeof(short) * sfinfo->frames * sfinfo->channels);
		if (!ptr) // malloc failed
		{
			sf_close(sndfile);
			fcntl(0, F_SETFL, O_NONBLOCK); /* non-blocking */
			return NULL;
		} else
		{
			/* read the whole file into memory */
			sf_readf_short(sndfile, ptr, sfinfo->frames);
			sf_close(sndfile);
		}
	}
	fcntl(0, F_SETFL, O_NONBLOCK); /* non-blocking */
	return ptr;
}
void loadSample(uint8_t index, char *path)
{
	if (!instrumentSafe(s, index)) return; /* instrument doesn't exist */
	Instrument *iv = &s->instrument->v[s->instrument->i[index]];
	SF_INFO sfinfo;
	short *sampledata = _loadSample(path, &sfinfo);
	if (!sampledata)
	{
		strcpy(w->command.error, "failed to load sample, out of memory");
		return;
	}

	/* unload any present sample data */
	if (iv->sample.data) free(iv->sample.data);
	iv->sample.data = sampledata;
	iv->sample.length = sfinfo.frames;
	iv->sample.channels = sfinfo.channels;
	iv->sample.rate = iv->sample.defrate = sfinfo.samplerate;
	iv->trimstart = 0;
	iv->trimlength = sfinfo.frames-1;
	iv->wavetable.framelength = (sfinfo.frames-1) / 256;
	iv->looplength = 0;
	iv->samplerate = 0xff;
	iv->bitdepth = 0xf;
}

void serializeInstrument(Instrument *iv, FILE *fp)
{
	fwrite(&iv->sample.length, sizeof(uint32_t), 1, fp);
	fwrite(&iv->sample.channels, sizeof(uint8_t), 1, fp);
	fwrite(&iv->channelmode, sizeof(int8_t), 1, fp);
	fwrite(&iv->sample.rate, sizeof(uint32_t), 1, fp);
	fwrite(&iv->sample.defrate, sizeof(uint32_t), 1, fp);
	fwrite(&iv->samplerate, sizeof(uint8_t), 1, fp);
	fwrite(&iv->bitdepth, sizeof(int8_t), 1, fp);
	fwrite(&iv->trimstart, sizeof(uint32_t), 1, fp);
	fwrite(&iv->trimlength, sizeof(uint32_t), 1, fp);
	fwrite(&iv->looplength, sizeof(uint32_t), 1, fp);
	fwrite(&iv->envelope, sizeof(uint16_t), 1, fp);
	fwrite(&iv->gain, sizeof(uint8_t), 1, fp);
	fwrite(&iv->invert, sizeof(bool), 1, fp);
	fwrite(&iv->pingpong, sizeof(bool), 1, fp);
	fwrite(&iv->loopramp, sizeof(uint8_t), 1, fp);

	fwrite(&iv->filtermode, sizeof(int8_t), 1, fp);
	fwrite(&iv->filtercutoff, sizeof(uint8_t), 1, fp);
	fwrite(&iv->filterresonance, sizeof(uint8_t), 1, fp);

	fwrite(&iv->algorithm, sizeof(int8_t), 1, fp);

	/* midi */
	fwrite(&iv->midi.channel, sizeof(int8_t), 1, fp);

	/* granular */
	fwrite(&iv->granular.cyclelength, sizeof(uint16_t), 1, fp);
	fwrite(&iv->granular.reversegrains, sizeof(bool), 1, fp);
	fwrite(&iv->granular.rampgrains, sizeof(int8_t), 1, fp);
	fwrite(&iv->granular.timestretch, sizeof(int16_t), 1, fp);
	fwrite(&iv->granular.notestretch, sizeof(bool), 1, fp);
	fwrite(&iv->granular.pitchshift, sizeof(int16_t), 1, fp);
	fwrite(&iv->granular.pitchstereo, sizeof(int8_t), 1, fp);

	/* wavetable */
	fwrite(&iv->wavetable.framelength, sizeof(uint32_t), 1, fp);
	fwrite(&iv->wavetable.wtpos, sizeof(uint8_t), 1, fp);
	fwrite(&iv->wavetable.syncoffset, sizeof(int8_t), 1, fp);
	fwrite(&iv->wavetable.pulsewidth, sizeof(int8_t), 1, fp);
	fwrite(&iv->wavetable.phasedynamics, sizeof(int8_t), 1, fp);
	fwrite(&iv->wavetable.lfospeed, sizeof(uint8_t), 1, fp);
	fwrite(&iv->wavetable.lfoduty, sizeof(int8_t), 1, fp);
	fwrite(&iv->wavetable.lfoshape, sizeof(bool), 1, fp);
	fwrite(&iv->wavetable.env.wtpos, sizeof(int8_t), 1, fp);
	fwrite(&iv->wavetable.env.sync, sizeof(int8_t), 1, fp);
	fwrite(&iv->wavetable.env.cutoff, sizeof(int8_t), 1, fp);
	fwrite(&iv->wavetable.env.phase, sizeof(int8_t), 1, fp);
	fwrite(&iv->wavetable.env.pwm, sizeof(int8_t), 1, fp);
	fwrite(&iv->wavetable.env.pdyn, sizeof(int8_t), 1, fp);
	fwrite(&iv->wavetable.lfo.gain, sizeof(uint8_t), 1, fp);
	fwrite(&iv->wavetable.lfo.wtpos, sizeof(int8_t), 1, fp);
	fwrite(&iv->wavetable.lfo.sync, sizeof(int8_t), 1, fp);
	fwrite(&iv->wavetable.lfo.cutoff, sizeof(int8_t), 1, fp);
	fwrite(&iv->wavetable.lfo.phase, sizeof(int8_t), 1, fp);
	fwrite(&iv->wavetable.lfo.pwm, sizeof(int8_t), 1, fp);
	fwrite(&iv->wavetable.lfo.pdyn, sizeof(int8_t), 1, fp);

	if (iv->sample.length)
		fwrite(iv->sample.data, sizeof(short), iv->sample.length * iv->sample.channels, fp);

	fwrite(&iv->effect.c, sizeof(uint8_t), 1, fp);
	for (int i = 0; i < iv->effect.c; i++)
		serializeEffect(&iv->effect.v[i], fp);
}
void deserializeInstrument(Instrument *iv, FILE *fp, double ratemultiplier, uint8_t major, uint8_t minor)
{
	if (major == 0 && minor < 99) fseek(fp, sizeof(uint32_t), SEEK_CUR);
	fread(&iv->sample.length, sizeof(uint32_t), 1, fp);
	fread(&iv->sample.channels, sizeof(uint8_t), 1, fp);
	fread(&iv->channelmode, sizeof(int8_t), 1, fp);
	fread(&iv->sample.rate, sizeof(uint32_t), 1, fp); iv->sample.rate *= ratemultiplier;
	if (!(major == 0 && minor < 101)) { fread(&iv->sample.defrate, sizeof(uint32_t), 1, fp); iv->sample.defrate *= ratemultiplier; }
	else                              iv->sample.defrate = iv->sample.rate;
	fread(&iv->samplerate, sizeof(uint8_t), 1, fp);
	fread(&iv->bitdepth, sizeof(int8_t), 1, fp);
	fread(&iv->trimstart, sizeof(uint32_t), 1, fp);
	fread(&iv->trimlength, sizeof(uint32_t), 1, fp);
	fread(&iv->looplength, sizeof(uint32_t), 1, fp);
	if (major == 0 && minor < 100) { fread(&iv->envelope, sizeof( uint8_t), 1, fp); iv->envelope <<= 8; }
	else                             fread(&iv->envelope, sizeof(uint16_t), 1, fp);
	fread(&iv->gain, sizeof(uint8_t), 1, fp);
	fread(&iv->invert, sizeof(bool), 1, fp);
	fread(&iv->pingpong, sizeof(bool), 1, fp);
	fread(&iv->loopramp, sizeof(uint8_t), 1, fp);

	if (!(major == 0 && minor < 101))
	{
		fread(&iv->filtermode, sizeof(int8_t), 1, fp);
		fread(&iv->filtercutoff, sizeof(uint8_t), 1, fp);
		fread(&iv->filterresonance, sizeof(uint8_t), 1, fp);
	} else
	{
		iv->filtermode = 0;
		iv->filtercutoff = 0xff;
		iv->filterresonance = 0x0;
	}

	if (!(major == 0 && minor < 100)) fread(&iv->algorithm, sizeof(int8_t), 1, fp);

	/* midi */
	fread(&iv->midi.channel, sizeof(int8_t), 1, fp);

	/* granular */
	fread(&iv->granular.cyclelength, sizeof(uint16_t), 1, fp);
	if (major == 0 && minor < 100) fseek(fp, sizeof(uint8_t), SEEK_CUR);
	fread(&iv->granular.reversegrains, sizeof(bool), 1, fp);
	if (!(major == 0 && minor < 100)) fread(&iv->granular.rampgrains, sizeof(int8_t), 1, fp);
	fread(&iv->granular.timestretch, sizeof(int16_t), 1, fp);
	fread(&iv->granular.notestretch, sizeof(bool), 1, fp);
	fread(&iv->granular.pitchshift, sizeof(int16_t), 1, fp);
	fread(&iv->granular.pitchstereo, sizeof(int8_t), 1, fp);

	/* wavetable */
	if (!(major == 0 && minor < 100))
	{
		fread(&iv->wavetable.framelength, sizeof(uint32_t), 1, fp);
		fread(&iv->wavetable.wtpos, sizeof(uint8_t), 1, fp);
		fread(&iv->wavetable.syncoffset, sizeof(int8_t), 1, fp);
		fread(&iv->wavetable.pulsewidth, sizeof(int8_t), 1, fp);
		fread(&iv->wavetable.phasedynamics, sizeof(int8_t), 1, fp);
		fread(&iv->wavetable.lfospeed, sizeof(uint8_t), 1, fp);
		fread(&iv->wavetable.lfoduty, sizeof(int8_t), 1, fp);
		fread(&iv->wavetable.lfoshape, sizeof(bool), 1, fp);
		fread(&iv->wavetable.env.wtpos, sizeof(int8_t), 1, fp);
		fread(&iv->wavetable.env.sync, sizeof(int8_t), 1, fp);
		fread(&iv->wavetable.env.cutoff, sizeof(int8_t), 1, fp);
		fread(&iv->wavetable.env.phase, sizeof(int8_t), 1, fp);
		fread(&iv->wavetable.env.pwm, sizeof(int8_t), 1, fp);
		fread(&iv->wavetable.env.pdyn, sizeof(int8_t), 1, fp);
		fread(&iv->wavetable.lfo.gain, sizeof(uint8_t), 1, fp);
		fread(&iv->wavetable.lfo.wtpos, sizeof(int8_t), 1, fp);
		fread(&iv->wavetable.lfo.sync, sizeof(int8_t), 1, fp);
		fread(&iv->wavetable.lfo.cutoff, sizeof(int8_t), 1, fp);
		fread(&iv->wavetable.lfo.phase, sizeof(int8_t), 1, fp);
		fread(&iv->wavetable.lfo.pwm, sizeof(int8_t), 1, fp);
		fread(&iv->wavetable.lfo.pdyn, sizeof(int8_t), 1, fp);
	}

	if (iv->sample.length)
	{
		iv->sample.data = malloc(sizeof(short) * iv->sample.length * iv->sample.channels);
		fread(iv->sample.data, sizeof(short), iv->sample.length * iv->sample.channels, fp);
	}

	if (!(major == 1 && minor < 97))
	{
		fread(&iv->effect.c, sizeof(uint8_t), 1, fp);
		for (int i = 0; i < iv->effect.c; i++)
			deserializeEffect(&iv->effect, &iv->effect.v[i], fp, major, minor);
	}
}
