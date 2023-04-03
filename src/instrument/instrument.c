#include "sampler/sampler.c"
#include "midi/midi.c"

/* TODO: should just be an array lookup */
const InstAPI *instGetAPI(InstType type)
{
	switch (type)
	{
		case INST_TYPE_SAMPLER: return &samplerAPI;
		case INST_TYPE_MIDI: return &midiAPI;
		default: return NULL;
	}
}

/* this implementation must be realtime safe */
size_t instGetPlaybackStateSize(void)
{
	return MAX(samplerAPI.statesize, midiAPI.statesize);
}

static InstChain *_copyInst(uint8_t index, Inst *src)
{
	uint8_t newcount = s->inst->c;
	if (!instSafe(s->inst, index))
		newcount++;

	InstChain *newinst = calloc(1, sizeof(InstChain) + newcount * sizeof(Inst));
	memcpy(newinst, s->inst, sizeof(InstChain) + s->inst->c * sizeof(Inst));
	newinst->c = newcount;

	uint8_t affectindex = newcount - 1;
	if (instSafe(newinst, index))
	{
		affectindex = newinst->i[index];
		memset(&newinst->v[affectindex], 0, sizeof(Inst));
	}

	const InstAPI *api;
	if ((api = instGetAPI(src->type))) api->copy(&newinst->v[affectindex], src);

	newinst->i[index] = affectindex;

	return newinst;
}


static void cb_copyInst(Event *e)
{
	InstChain *ivc = e->src;
	if (instSafe(ivc, (size_t)e->callbackarg))
	{
		const InstAPI *api;
		Inst *iv = &ivc->v[ivc->i[(size_t)e->callbackarg]];
		if ((api = instGetAPI(iv->type))) api->free(iv);
	}

	free(e->src); e->src = NULL;

	w->mode = MODE_NORMAL;
	p->redraw = 1;
}

int copyInst(uint8_t index, Inst *src) /* TODO: overwrite paste */
{ /* fully atomic */
	Event e;
	e.sem = M_SEM_SWAP_REQ;
	e.dest = (void **)&s->inst;
	e.src = _copyInst(index, src);
	e.callback = cb_copyInst;
	e.callbackarg = (void *)(size_t)index;
	pushEvent(&e);
	return 0;
}

/* is an inst safe to use */
bool instSafe(InstChain *ic, short index)
{
	if (index < 0) return 0; /* special instruments should be handled separately */
	if (index != INSTRUMENT_MAX && ic->i[index] < ic->c)
		return 1;
	return 0;
}

/* take a Sample* and reparent it under .iv */
void reparentSample(Inst *iv, Sample *sample) /* TODO: remove */
{
	InstSamplerState *s = iv->state; /* TODO: TEMPORARY!! */

	short index = getEmptySampleIndex(s->sample);
	if (index == -1)
	{
		free(sample);
		return;
	}

	(*s->sample)[index] = sample;
}

void toggleRecording(uint8_t inst, char cue)
{
	if (w->instrecv == INST_REC_LOCK_OK) w->instreci = inst;
	if (w->instreci == inst)
	{
		switch (w->instrecv)
		{
			case INST_REC_LOCK_OK:
				w->recbuffer = malloc(sizeof(short) * RECORD_LENGTH * samplerate * 2);
				if (!w->recbuffer)
				{
					strcpy(w->command.error, "failed to start recording, out of memory");
					break;
				}
				w->recptr = 0;
				if (cue) w->instrecv = INST_REC_LOCK_CUE_START;
				else     w->instrecv = INST_REC_LOCK_START;
				break;
			default: w->instrecv = INST_REC_LOCK_PREP_END; break;
		}
	} p->redraw = 1;
}

static void cb_addInst         (Event *e) { free(e->src); w->mode = MODE_NORMAL; p->redraw = 1; }
static void cb_addRecordInst   (Event *e) { free(e->src); toggleRecording((size_t)e->callbackarg, 0); p->redraw = 1; }
static void cb_addRecordCueInst(Event *e) { free(e->src); toggleRecording((size_t)e->callbackarg, 1); p->redraw = 1; }

InstChain *_addInst(uint8_t index, InstType type)
{
	InstChain *newinst = calloc(1, sizeof(InstChain) + (s->inst->c+1) * sizeof(Inst));
	memcpy(newinst, s->inst, sizeof(InstChain) + s->inst->c * sizeof(Inst));

	newinst->v[newinst->c].type = type;
	const InstAPI *api;
	if ((api = instGetAPI(type)))
		newinst->v[newinst->c].state = api->init();

	newinst->i[index] = newinst->c;
	newinst->c++;

	return newinst;
}
int addInst(uint8_t index, InstType algorithm, void (*cb)(Event *), void *cbarg)
{ /* fully atomic */
	if (instSafe(s->inst, index)) return 1; /* index occupied */
	freeWaveform();
	Event e;
	e.sem = M_SEM_SWAP_REQ;
	e.dest = (void**)&s->inst;
	e.src = _addInst(index, algorithm);
	e.callback = cb;
	// e.callbackarg = (void*)(size_t)index;
	e.callbackarg = cbarg;
	pushEvent(&e);
	return 0;
}

int addReparentInst(uint8_t index, InstType type, Sample *buffer)
{ /* fully atomic */
	if (instSafe(s->inst, index)) return 1; /* index occupied */

	InstChain *newchain = _addInst(index, type);

	Event e;
	e.sem = M_SEM_SWAP_REQ;
	e.dest = (void **)&s->inst;
	e.src = newchain;
	reparentSample(&newchain->v[newchain->i[index]], buffer);
	e.callback = cb_addInst;
	// e.callbackarg = arg;
	pushEvent(&e);
	return 0;
}

/* returns -1 if no inst slots are free */
short emptyInst(uint8_t min)
{
	for (int i = min; i < INSTRUMENT_MAX; i++)
		if (!instSafe(s->inst, i)) return i;
	return -1;
}

void yankInst(uint8_t index)
{
	if (!instSafe(s->inst, index)) return; /* nothing to copy */

	Inst *iv = &s->inst->v[s->inst->i[index]];
	const InstAPI *api;
	if ((api = instGetAPI(w->instbuffer.type))) api->free(&w->instbuffer);
	if ((api = instGetAPI(iv->type)))           api->copy(&w->instbuffer, iv);
}

void putInst(size_t index)
{
	copyInst(index, &w->instbuffer);
}

static void cb_delInst(Event *e)
{
	Inst *iv = &((InstChain*)e->src)->v[(size_t)e->callbackarg];

	const InstAPI *api;
	if ((api = instGetAPI(iv->type))) api->free(iv);

	free(e->src); e->src = NULL;
	p->redraw = 1;
}

int delInst(uint8_t index)
{ /* fully atomic */
	if (!instSafe(s->inst, index)) return 1; /* inst doesn't exist */

	size_t cutindex = s->inst->i[index]; /* cast to void* later */

	InstChain *newinst = calloc(1, sizeof(InstChain) + (s->inst->c-1) * sizeof(Inst));

	memcpy(newinst->i, s->inst->i, sizeof(uint8_t) * INSTRUMENT_MAX);


	if (cutindex > 0)
		memcpy(&newinst->v[0],
				&s->inst->v[0],
				sizeof(Inst)*(cutindex));

	if (cutindex < s->inst->c-1)
		memcpy(&newinst->v[cutindex],
				&s->inst->v[cutindex+1],
				sizeof(Inst)*(s->inst->c - (cutindex+1)));

	newinst->i[index] = INSTRUMENT_VOID;
	// backref contiguity
	for (uint8_t i = 0; i < 255; i++)
		if (newinst->i[i] >= cutindex && newinst->i[i] < s->inst->c)
			newinst->i[i]--;

	newinst->c = s->inst->c - 1;

	Event e;
	e.sem = M_SEM_SWAP_REQ;
	e.dest = (void **)&s->inst;
	e.src = newinst;
	e.callback = cb_delInst;
	e.callbackarg = (void *)cutindex;
	pushEvent(&e);
	return 0;
}

// int sampleExportCallback(char *command, unsigned char *mode) /* TODO: unmaintained */
// {
// 	if (!instSafe(s->inst, w->inst)) return 1;
// 	Inst *iv = &s->inst->v[s->inst->i[w->inst]];
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

void instControlCallback(void) { p->redraw = 1; }

struct json_object *serializeInst(Inst *inst, size_t *dataoffset)
{
	struct json_object *jso = json_object_new_object();
	json_object_object_add(jso, "type", json_object_new_int(inst->type));

	const InstAPI *api;
	if ((api = instGetAPI(inst->type)))
		json_object_object_add(jso, "state", api->serialize(inst->state, dataoffset));
	else
		json_object_object_add(jso, "state", NULL);

	return jso;
}
void serializeInstData(FILE *fp, Inst *inst, size_t *dataoffset)
{
	const InstAPI *api;
	if ((api = instGetAPI(inst->type)))
		api->serializedata(fp, inst->state, dataoffset);
}

Inst deserializeInst(struct json_object *jso, void *data, double ratemultiplier)
{
	Inst ret;
	ret.type = json_object_get_int(json_object_object_get(jso, "type"));

	const InstAPI *api;
	if ((api = instGetAPI(ret.type)))
		ret.state = api->deserialize(json_object_object_get(jso, "state"), data, ratemultiplier);

	return ret;
}

struct json_object *serializeInstChain(InstChain *chain)
{
	int i;
	size_t dataoffset = 0;
	struct json_object *ret = json_object_new_object();
	struct json_object *array;

	array = json_object_new_array_ext(INSTRUMENT_MAX);
	for (i = 0; i < INSTRUMENT_MAX; i++)
		json_object_array_add(array, json_object_new_int(chain->i[i]));
	json_object_object_add(ret, "index", array);

	array = json_object_new_array_ext(chain->c);
	for (i = 0; i < chain->c; i++)
		json_object_array_add(array, serializeInst(&chain->v[i], &dataoffset));
	json_object_object_add(ret, "data", array);

	return ret;
}

void serializeInstChainData(FILE *fp, InstChain *chain)
{
	size_t dataoffset = 0;
	for (int i = 0; i < chain->c; i++)
		serializeInstData(fp, &chain->v[i], &dataoffset);
}

InstChain *deserializeInstChain(struct json_object *jso, void *data, double ratemultiplier)
{
	int i;
	InstChain *ret = calloc(1, sizeof(InstChain) + json_object_array_length(json_object_object_get(jso, "data")) * sizeof(Inst));

	ret->c = json_object_array_length(json_object_object_get(jso, "data"));
	for (i = 0; i < INSTRUMENT_MAX; i++)
		ret->i[i] = json_object_get_int(json_object_array_get_idx(json_object_object_get(jso, "index"), i));

	for (i = 0; i < ret->c; i++)
		ret->v[i] = deserializeInst(json_object_array_get_idx(json_object_object_get(jso, "data"), i), data, ratemultiplier);

	return ret;
}

#include "input.c"
#include "waveform.c"
#include "draw.c"
