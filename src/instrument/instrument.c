#include "waveform.c"

static void __copyInstrument(Instrument *dest, Instrument *src) /* NOT atomic */
{
	if (dest->sample)
	{ free(dest->sample); dest->sample = NULL; }

	memcpy(dest, src, sizeof(Instrument));

	if (src->sample)
	{ /* only copy sampledata if it exists */
		dest->sample = malloc(sizeof(Sample) + sizeof(short)*src->sample->length*src->sample->channels);
		memcpy(dest->sample, src->sample, sizeof(Sample) + sizeof(short)*src->sample->length*src->sample->channels);
	}
}

static InstrumentChain *_copyInstrument(uint8_t index, Instrument *src)
{
	InstrumentChain *newinstrument = calloc(1, sizeof(InstrumentChain) + (s->instrument->c+1) * sizeof(Instrument));
	memcpy(newinstrument, s->instrument, sizeof(InstrumentChain) + s->instrument->c * sizeof(Instrument));

	__copyInstrument(&newinstrument->v[newinstrument->c], src);

	newinstrument->i[index] = newinstrument->c;
	newinstrument->c++;

	return newinstrument;
}


static void cb_copyInstrument(Event *e)
{
	if (instrumentSafe(e->src, (size_t)e->callbackarg))
		delInstrumentForce(&((InstrumentChain*)e->src)->v[(size_t)e->callbackarg]);

	free(e->src); e->src = NULL;

	w->mode = MODE_NORMAL;
	p->redraw = 1;
}
int copyInstrument(uint8_t index, Instrument *src)
{ /* fully atomic */
	if (instrumentSafe(s->instrument, index)) return 1; /* index occupied */
	Event e;
	e.sem = M_SEM_SWAP_REQ;
	e.dest = (void **)&s->instrument;
	e.src = _copyInstrument(index, src);
	e.callback = cb_copyInstrument;
	e.callbackarg = (void *)(size_t)index;
	pushEvent(&e);
	return 0;
}

/* frees the contents of an instrument */
void delInstrumentForce(Instrument *iv)
{
	freeWaveform();
	if (iv->sample) free(iv->sample);
	iv->sample = NULL;
}

/* is an instrument safe to use */
bool instrumentSafe(InstrumentChain *ic, short index)
{
	if (index < 0) return 0; /* special instruments should be handled separately */
	if (index != INSTRUMENT_MAX && ic->i[index] < ic->c)
		return 1;
	return 0;
}

/* take a Sample* and reparent it under instrument iv */
void reparentSample(Instrument *iv, Sample *sample)
{
	if (iv->sample) free(iv->sample);
	iv->sample = NULL;

	iv->sample = sample;

	iv->trimstart = 0;
	iv->trimlength = sample->length-1;
	iv->wavetable.framelength = (sample->length-1)>>8; /* /256 */
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
	} p->redraw = 1;
}

static void cb_addInstrument         (Event *e) { free(e->src); e->src = NULL; w->mode = MODE_NORMAL; p->redraw = 1; }
static void cb_addRecordInstrument   (Event *e) { free(e->src); e->src = NULL; toggleRecording((size_t)e->callbackarg, 0); p->redraw = 1; }
static void cb_addRecordCueInstrument(Event *e) { free(e->src); e->src = NULL; toggleRecording((size_t)e->callbackarg, 1); p->redraw = 1; }

/* __ layer of abstraction for initializing instrumentbuffer */
void __addInstrument(Instrument *iv, int8_t algorithm)
{
	iv->algorithm = algorithm;
	iv->samplerate = 0xff;
	iv->bitdepth = 0xf;
	iv->envelope = 0x00f0;

	iv->midi.channel = -1;

	iv->granular.cyclelength = 0x3fff;
	iv->granular.rampgrains = 8;
	iv->granular.beatsensitivity = 0x80;
	iv->granular.beatdecay = 0xff;

	iv->sample = calloc(1, sizeof(Sample));
}
InstrumentChain *_addInstrument(uint8_t index, int8_t algorithm)
{
	InstrumentChain *newinstrument = calloc(1, sizeof(InstrumentChain) + (s->instrument->c+1) * sizeof(Instrument));
	memcpy(newinstrument, s->instrument, sizeof(InstrumentChain) + s->instrument->c * sizeof(Instrument));

	__addInstrument(&newinstrument->v[newinstrument->c], algorithm);

	newinstrument->i[index] = newinstrument->c;
	newinstrument->c++;

	return newinstrument;
}
int addInstrument(uint8_t index, int8_t algorithm, void (*cb)(Event *))
{ /* fully atomic */
	if (instrumentSafe(s->instrument, index)) return 1; /* index occupied */
	freeWaveform();
	Event e;
	e.sem = M_SEM_SWAP_REQ;
	e.dest = (void **)&s->instrument;
	e.src = _addInstrument(index, algorithm);
	e.callback = cb;
	e.callbackarg = (void *)(size_t)index;
	pushEvent(&e);
	return 0;
}

typedef struct
{
	Sample *buffer;
	uint8_t index;
} InstrumentAddReparentArg;
static void cb_addReparentInstrument(Event *e)
{
	InstrumentAddReparentArg *castarg = e->callbackarg;
	free(e->src); e->src = NULL;
	reparentSample(&s->instrument->v[s->instrument->i[castarg->index]], castarg->buffer);
	free(e->callbackarg); e->callbackarg = NULL;
	p->redraw = 1;
}
int addReparentInstrument(uint8_t index, int8_t algorithm, Sample *buffer)
{ /* fully atomic */
	if (instrumentSafe(s->instrument, index)) return 1; /* index occupied */

	InstrumentAddReparentArg *arg = malloc(sizeof(InstrumentAddReparentArg));
	arg->buffer = buffer;
	arg->index = index;

	Event e;
	e.sem = M_SEM_SWAP_REQ;
	e.dest = (void **)&s->instrument;
	e.src = _addInstrument(index, algorithm);
	e.callback = cb_addReparentInstrument;
	e.callbackarg = arg;
	pushEvent(&e);
	return 0;
}

/* returns -1 if no instrument slots are free */
short emptyInstrument(uint8_t min)
{
	for (int i = min; i < INSTRUMENT_MAX; i++)
		if (!instrumentSafe(s->instrument, i)) return i;
	return -1;
}

void yankInstrument(uint8_t index)
{
	if (!instrumentSafe(s->instrument, index)) return; /* nothing to copy */
	__copyInstrument(&w->instrumentbuffer, &s->instrument->v[s->instrument->i[index]]);
}

void putInstrument(size_t index)
{
	copyInstrument(index, &w->instrumentbuffer);
	p->redraw = 1;
}

static void cb_delInstrument(Event *e)
{
	delInstrumentForce(&((InstrumentChain *)e->src)->v[(size_t)e->callbackarg]);
	free(e->src); e->src = NULL;
	p->redraw = 1;
}

int delInstrument(uint8_t index)
{ /* fully atomic */
	if (!instrumentSafe(s->instrument, index)) return 1; /* instrument doesn't exist */

	size_t cutindex = s->instrument->i[index]; /* cast to void* later */

	InstrumentChain *newinstrument = calloc(1, sizeof(InstrumentChain) + (s->instrument->c-1) * sizeof(Instrument));

	memcpy(newinstrument->i, s->instrument->i, sizeof(uint8_t) * INSTRUMENT_MAX);


	if (cutindex > 0)
		memcpy(&newinstrument->v[0],
				&s->instrument->v[0],
				sizeof(Instrument)*(cutindex));

	if (cutindex < s->instrument->c-1)
		memcpy(&newinstrument->v[cutindex],
				&s->instrument->v[cutindex+1],
				sizeof(Instrument)*(s->instrument->c - (cutindex+1)));

	newinstrument->i[index] = INSTRUMENT_VOID;
	// backref contiguity
	for (uint8_t i = 0; i < 255; i++)
		if (newinstrument->i[i] >= cutindex && newinstrument->i[i] < s->instrument->c)
			newinstrument->i[i]--;

	newinstrument->c = s->instrument->c - 1;

	Event e;
	e.sem = M_SEM_SWAP_REQ;
	e.dest = (void **)&s->instrument;
	e.src = newinstrument;
	e.callback = cb_delInstrument;
	e.callbackarg = (void *)cutindex;
	pushEvent(&e);
	return 0;
}

Sample *_loadSample(char *path)
{
	fcntl(0, F_SETFL, 0); /* blocking */
	SF_INFO sfinfo;
	memset(&sfinfo, 0, sizeof(SF_INFO));

	SNDFILE *sndfile = sf_open(path, SFM_READ, &sfinfo);

	Sample *ret;

	if (!sndfile)
	{ /* raw file */
		struct stat buf;
		if (stat(path, &buf) == -1)
		{
			fcntl(0, F_SETFL, O_NONBLOCK); /* non-blocking */
			return NULL;
		}

		ret = malloc(sizeof(Sample) + buf.st_size - buf.st_size % sizeof(short));
		if (!ret) /* malloc failed */
		{
			fcntl(0, F_SETFL, O_NONBLOCK);
			return NULL;
		} else
		{
			/* read the whole file into memory */
			FILE *fp = fopen(path, "r");
			fread(&ret->data, sizeof(short), buf.st_size / sizeof(short), fp);
			fclose(fp);

			ret->channels = 1;
			ret->length = buf.st_size / sizeof(short);
			ret->rate = ret->defrate = 12000;
		}
	} else /* audio file */
	{
		ret = malloc(sizeof(Sample) + sizeof(short)*sfinfo.frames*sfinfo.channels);
		if (!ret) // malloc failed
		{
			sf_close(sndfile);
			fcntl(0, F_SETFL, O_NONBLOCK); /* non-blocking */
			return NULL;
		} else
		{
			/* read the whole file into memory */
			sf_readf_short(sndfile, ret->data, sfinfo.frames);
			ret->length = sfinfo.frames;
			ret->channels = sfinfo.channels;
			ret->rate = ret->defrate = sfinfo.samplerate;
			sf_close(sndfile);
		}
	}
	fcntl(0, F_SETFL, O_NONBLOCK); /* non-blocking */
	return ret;
}
void loadSample(uint8_t index, char *path) /* TODO: atomicity */
{
	if (!instrumentSafe(s->instrument, index)) return; /* instrument doesn't exist */
	freeWaveform();
	Instrument *iv = &s->instrument->v[s->instrument->i[index]];
	Sample *newsample = _loadSample(path);
	if (!newsample)
	{
		strcpy(w->command.error, "failed to load sample, out of memory");
		return;
	}

	/* unload any present sample data */
	if (iv->sample) free(iv->sample);
	iv->sample = newsample;
	iv->trimstart = 0;
	iv->trimlength = newsample->length-1;
	iv->wavetable.framelength = (newsample->length-1) / 256;
	iv->looplength = 0;
	iv->samplerate = 0xff;
	iv->bitdepth = 0xf;
}

/* TODO: sample could already be loaded into p->semarg, reparent if so */
void sampleLoadCallback(char *path) /* TODO: atomicity */
{
	if (path) loadSample(w->instrument, path);

	w->page = PAGE_INSTRUMENT;
	w->mode = MODE_NORMAL;
	w->showfilebrowser = 0;
	resetWaveform();
}

// int sampleExportCallback(char *command, unsigned char *mode) /* TODO: unmaintained */
// {
// 	if (!instrumentSafe(s->instrument, w->instrument)) return 1;
// 	Instrument *iv = &s->instrument->v[s->instrument->i[w->instrument]];
//
// 	if (!iv->sample->length) return 1;
//
// 	char *buffer = malloc(strlen(command) + 1);
// 	wordSplit(buffer, command, 0);
//
// 	SNDFILE *sndfile;
// 	SF_INFO sfinfo;
// 	memset(&sfinfo, 0, sizeof(sfinfo));
//
// 	sfinfo.samplerate = iv->sample->rate;
// 	sfinfo.frames = iv->sample->length;
// 	sfinfo.channels = iv->sample->channels;
//
// 	sfinfo.format = SF_FORMAT_WAV | SF_FORMAT_PCM_16;
// 	sndfile = sf_open(fileExtension(buffer, ".wav"), SFM_WRITE, &sfinfo);
// 	if (sndfile == NULL) { free(buffer); return 1; }
//
// 	// write the sample data to disk
// 	sf_writef_short(sndfile, iv->sample->data, iv->sample->length * iv->sample->channels);
// 	sf_close(sndfile);
//
// 	free(buffer);
// 	return 0;
// }

// void serializeInstrument(Instrument *iv, FILE *fp)
// {
// 	fwrite(&iv->sample->length, sizeof(uint32_t), 1, fp);
// 	fwrite(&iv->sample->channels, sizeof(uint8_t), 1, fp);
// 	fwrite(&iv->channelmode, sizeof(int8_t), 1, fp);
// 	fwrite(&iv->sample->rate, sizeof(uint32_t), 1, fp);
// 	fwrite(&iv->sample->defrate, sizeof(uint32_t), 1, fp);
// 	fwrite(&iv->samplerate, sizeof(uint8_t), 1, fp);
// 	fwrite(&iv->bitdepth, sizeof(int8_t), 1, fp);
// 	fwrite(&iv->interpolate, sizeof(bool), 1, fp);
// 	fwrite(&iv->trimstart, sizeof(uint32_t), 1, fp);
// 	fwrite(&iv->trimlength, sizeof(uint32_t), 1, fp);
// 	fwrite(&iv->looplength, sizeof(uint32_t), 1, fp);
// 	fwrite(&iv->envelope, sizeof(uint16_t), 1, fp);
// 	fwrite(&iv->gain, sizeof(uint8_t), 1, fp);
// 	fwrite(&iv->invert, sizeof(bool), 1, fp);
// 	fwrite(&iv->pingpong, sizeof(bool), 1, fp);
// 	fwrite(&iv->loopramp, sizeof(uint8_t), 1, fp);
//
// 	fwrite(&iv->algorithm, sizeof(int8_t), 1, fp);
//
// 	/* midi */
// 	fwrite(&iv->midi.channel, sizeof(int8_t), 1, fp);
//
// 	/* granular */
// 	fwrite(&iv->granular.cyclelength, sizeof(uint16_t), 1, fp);
// 	fwrite(&iv->granular.reversegrains, sizeof(bool), 1, fp);
// 	fwrite(&iv->granular.rampgrains, sizeof(int8_t), 1, fp);
// 	fwrite(&iv->granular.timestretch, sizeof(int16_t), 1, fp);
// 	fwrite(&iv->granular.notestretch, sizeof(bool), 1, fp);
// 	fwrite(&iv->granular.pitchshift, sizeof(int16_t), 1, fp);
// 	fwrite(&iv->granular.pitchstereo, sizeof(int8_t), 1, fp);
//
// 	/* wavetable */
// 	fwrite(&iv->wavetable.framelength, sizeof(uint32_t), 1, fp);
// 	fwrite(&iv->wavetable.wtpos, sizeof(uint8_t), 1, fp);
// 	fwrite(&iv->wavetable.syncoffset, sizeof(int8_t), 1, fp);
// 	fwrite(&iv->wavetable.pulsewidth, sizeof(int8_t), 1, fp);
// 	fwrite(&iv->wavetable.phasedynamics, sizeof(int8_t), 1, fp);
// 	fwrite(&iv->wavetable.envelope, sizeof(uint16_t), 1, fp);
// 	fwrite(&iv->wavetable.lfospeed, sizeof(uint8_t), 1, fp);
// 	fwrite(&iv->wavetable.lfoduty, sizeof(int8_t), 1, fp);
// 	fwrite(&iv->wavetable.lfoshape, sizeof(bool), 1, fp);
// 	fwrite(&iv->wavetable.env.wtpos, sizeof(int8_t), 1, fp);
// 	fwrite(&iv->wavetable.env.sync, sizeof(int8_t), 1, fp);
// 	fwrite(&iv->wavetable.env.cutoff, sizeof(int8_t), 1, fp);
// 	fwrite(&iv->wavetable.env.phase, sizeof(int8_t), 1, fp);
// 	fwrite(&iv->wavetable.env.pwm, sizeof(int8_t), 1, fp);
// 	fwrite(&iv->wavetable.env.pdyn, sizeof(int8_t), 1, fp);
// 	fwrite(&iv->wavetable.lfo.gain, sizeof(uint8_t), 1, fp);
// 	fwrite(&iv->wavetable.lfo.wtpos, sizeof(int8_t), 1, fp);
// 	fwrite(&iv->wavetable.lfo.sync, sizeof(int8_t), 1, fp);
// 	fwrite(&iv->wavetable.lfo.cutoff, sizeof(int8_t), 1, fp);
// 	fwrite(&iv->wavetable.lfo.phase, sizeof(int8_t), 1, fp);
// 	fwrite(&iv->wavetable.lfo.pwm, sizeof(int8_t), 1, fp);
// 	fwrite(&iv->wavetable.lfo.pdyn, sizeof(int8_t), 1, fp);
//
// 	if (iv->sample->length)
// 		fwrite(iv->sample->data, sizeof(short), iv->sample->length * iv->sample->channels, fp);
// }
// void deserializeInstrument(Instrument *iv, FILE *fp, double ratemultiplier, uint8_t major, uint8_t minor)
// {
// 	Sample *newsample = malloc(sizeof(Sample));
// 	if (major == 0 && minor < 99) fseek(fp, sizeof(uint32_t), SEEK_CUR);
// 	fread(&newsample->length, sizeof(uint32_t), 1, fp);
// 	fread(&newsample->channels, sizeof(uint8_t), 1, fp);
// 	fread(&iv->channelmode, sizeof(int8_t), 1, fp);
// 	fread(&newsample->rate, sizeof(uint32_t), 1, fp);    newsample->rate *= ratemultiplier;
// 	fread(&newsample->defrate, sizeof(uint32_t), 1, fp); newsample->defrate *= ratemultiplier;
// 	fread(&iv->samplerate, sizeof(uint8_t), 1, fp);
// 	fread(&iv->bitdepth, sizeof(int8_t), 1, fp);
// 	fread(&iv->interpolate, sizeof(bool), 1, fp);
// 	fread(&iv->trimstart, sizeof(uint32_t), 1, fp);
// 	fread(&iv->trimlength, sizeof(uint32_t), 1, fp);
// 	fread(&iv->looplength, sizeof(uint32_t), 1, fp);
// 	fread(&iv->envelope, sizeof(uint16_t), 1, fp);
// 	fread(&iv->gain, sizeof(uint8_t), 1, fp);
// 	fread(&iv->invert, sizeof(bool), 1, fp);
// 	fread(&iv->pingpong, sizeof(bool), 1, fp);
// 	fread(&iv->loopramp, sizeof(uint8_t), 1, fp);
//
// 	fread(&iv->algorithm, sizeof(int8_t), 1, fp);
//
// 	/* midi */
// 	fread(&iv->midi.channel, sizeof(int8_t), 1, fp);
//
// 	/* granular */
// 	fread(&iv->granular.cyclelength, sizeof(uint16_t), 1, fp);
// 	fread(&iv->granular.reversegrains, sizeof(bool), 1, fp);
// 	fread(&iv->granular.rampgrains, sizeof(int8_t), 1, fp);
// 	fread(&iv->granular.timestretch, sizeof(int16_t), 1, fp);
// 	fread(&iv->granular.notestretch, sizeof(bool), 1, fp);
// 	fread(&iv->granular.pitchshift, sizeof(int16_t), 1, fp);
// 	fread(&iv->granular.pitchstereo, sizeof(int8_t), 1, fp);
//
// 	/* wavetable */
// 	fread(&iv->wavetable.framelength, sizeof(uint32_t), 1, fp);
// 	fread(&iv->wavetable.wtpos, sizeof(uint8_t), 1, fp);
// 	fread(&iv->wavetable.syncoffset, sizeof(int8_t), 1, fp);
// 	fread(&iv->wavetable.pulsewidth, sizeof(int8_t), 1, fp);
// 	fread(&iv->wavetable.phasedynamics, sizeof(int8_t), 1, fp);
// 	fread(&iv->wavetable.envelope, sizeof(uint16_t), 1, fp);
// 	fread(&iv->wavetable.lfospeed, sizeof(uint8_t), 1, fp);
// 	fread(&iv->wavetable.lfoduty, sizeof(int8_t), 1, fp);
// 	fread(&iv->wavetable.lfoshape, sizeof(bool), 1, fp);
// 	fread(&iv->wavetable.env.wtpos, sizeof(int8_t), 1, fp);
// 	fread(&iv->wavetable.env.sync, sizeof(int8_t), 1, fp);
// 	fread(&iv->wavetable.env.cutoff, sizeof(int8_t), 1, fp);
// 	fread(&iv->wavetable.env.phase, sizeof(int8_t), 1, fp);
// 	fread(&iv->wavetable.env.pwm, sizeof(int8_t), 1, fp);
// 	fread(&iv->wavetable.env.pdyn, sizeof(int8_t), 1, fp);
// 	fread(&iv->wavetable.lfo.gain, sizeof(uint8_t), 1, fp);
// 	fread(&iv->wavetable.lfo.wtpos, sizeof(int8_t), 1, fp);
// 	fread(&iv->wavetable.lfo.sync, sizeof(int8_t), 1, fp);
// 	fread(&iv->wavetable.lfo.cutoff, sizeof(int8_t), 1, fp);
// 	fread(&iv->wavetable.lfo.phase, sizeof(int8_t), 1, fp);
// 	fread(&iv->wavetable.lfo.pwm, sizeof(int8_t), 1, fp);
// 	fread(&iv->wavetable.lfo.pdyn, sizeof(int8_t), 1, fp);
//
// 	newsample = realloc(newsample, sizeof(Sample) + sizeof(short)*newsample->length*newsample->channels);
// 	if (newsample->length)
// 		fread(newsample->data, sizeof(short), newsample->length*newsample->channels, fp);
// 	iv->sample = newsample;
// }

void instrumentControlCallback(void)
{
	p->redraw = 1;
}
