static void __copyInstrument(Instrument *dest, Instrument *src) /* NOT atomic */
{
	for (uint8_t i = 0; i < dest->samplecount; i++)
		free(dest->sample[i]);
	if (dest->sample) free(dest->sample);

	memcpy(dest, src, sizeof(Instrument));

	if (src->samplecount)
	{
		dest->sample = malloc(sizeof(Sample*) * src->samplecount);
		for (uint8_t i = 0; i < src->samplecount; i++)
		{
			dest->sample[i] = malloc(sizeof(Sample) + sizeof(short)*src->sample[i]->length*src->sample[i]->channels);
			memcpy(dest->sample[i], src->sample[i], sizeof(Sample) + sizeof(short)*src->sample[i]->length*src->sample[i]->channels);
		}
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
	for (uint8_t i = 0; i < iv->samplecount; i++)
		free(iv->sample[i]);

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
void reparentSample(Instrument *iv, Sample *sample) /* TODO: atomicity? */
{
	iv->samplecount++;
	iv->sample = realloc(iv->sample, sizeof(Sample*) * iv->samplecount);

	iv->sample[iv->samplecount - 1] = sample;
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
	SF_INFO sfinfo = { 0 };

	Sample *ret = NULL;

	SNDFILE *sndfile = sf_open(path, SFM_READ, &sfinfo);
	if (!sndfile)
	{ /* raw file */
		struct stat buf;
		if (stat(path, &buf) == -1) goto _loadSample_end;

		ret = calloc(1, sizeof(Sample) + buf.st_size - buf.st_size % sizeof(short));
		if (!ret) goto _loadSample_end;

		/* read the whole file into memory */
		FILE *fp = fopen(path, "r");
		fread(&ret->data, sizeof(short), buf.st_size / sizeof(short), fp);
		fclose(fp);

		ret->channels = 1;
		ret->length = buf.st_size / sizeof(short);
		ret->rate = ret->defrate = 12000;
	} else /* audio file */
	{
		ret = calloc(1, sizeof(Sample) + sizeof(short)*sfinfo.frames*sfinfo.channels);

		if (!ret) goto _loadSample_end;

		/* read the whole file into memory */
		sf_readf_short(sndfile, ret->data, sfinfo.frames);
		ret->length = sfinfo.frames;
		ret->channels = sfinfo.channels;
		ret->rate = ret->defrate = sfinfo.samplerate;
	}

	ret->trimstart = 0;
	ret->trimlength = ret->length-1;
	ret->looplength = 0;

_loadSample_end:
	if (sndfile) sf_close(sndfile);
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

	reparentSample(iv, newsample);
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

void instrumentControlCallback(void) { p->redraw = 1; }

#include "input.c"
#include "waveform.c"
#include "draw.c"
