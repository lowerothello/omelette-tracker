static void __copyInstrument(Instrument *dest, Instrument *src) /* NOT atomic */
{
	FOR_SAMPLECHAIN(i, dest->sample)
		free((*dest->sample)[i]);

	SampleChain *dsamplechain = dest->sample;

	memcpy(dest, src, sizeof(Instrument));

	/* avoid unnecessarily reallocing the sample chain */
	dest->sample = dsamplechain;

	copySampleChain(dest->sample, src->sample);
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
	FOR_SAMPLECHAIN(i, iv->sample)
		free((*iv->sample)[i]);
	free(iv->sample);
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
void reparentSample(Instrument *iv, Sample *sample) /* TODO: remove */
{
	short index = getEmptySampleIndex(iv->sample);
	if (index == -1)
	{
		free(sample);
		return;
	}

	(*iv->sample)[index] = sample;
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

static void cb_addInstrument         (Event *e) { free(e->src); w->mode = MODE_NORMAL; p->redraw = 1; }
static void cb_addRecordInstrument   (Event *e) { free(e->src); toggleRecording((size_t)e->callbackarg, 0); p->redraw = 1; }
static void cb_addRecordCueInstrument(Event *e) { free(e->src); toggleRecording((size_t)e->callbackarg, 1); p->redraw = 1; }

/* __ layer of abstraction for initializing instrumentbuffer */
void __addInstrument(Instrument *iv, int8_t algorithm)
{
	iv->sample = calloc(1, sizeof(SampleChain));

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
