static void *samplerInit(void)
{
	InstSamplerState *ret = calloc(1, sizeof(InstSamplerState));
	ret->sample = calloc(1, sizeof(SampleChain));

	ret->rateredux = 0xff;
	ret->bitredux = 0xf;
	ret->envelope = 0x00f0;

	ret->granular.cyclelength = 0x3fff;
	ret->granular.rampgrains = 8;
	ret->granular.beatsensitivity = 0x80;
	ret->granular.beatdecay = 0xff;

	return ret;
}

static void samplerFree(Inst *iv)
{
	freeWaveform();

	InstSamplerState *s = iv->state;
	FOR_SAMPLECHAIN(i, s->sample)
		free((*s->sample)[i]);
	free(s->sample);
	free(s);
}

/* dest has already been free'd */
static void samplerCopy(Inst *dest, Inst *src)
{
	dest->type = INST_TYPE_SAMPLER;
	InstSamplerState *s = dest->state = calloc(1, sizeof(InstSamplerState));

	memcpy(dest->state, src->state, sizeof(InstSamplerState));

	s->sample = calloc(1, sizeof(SampleChain));
	copySampleChain(s->sample, ((InstSamplerState*)src->state)->sample);
}

static void samplerGetIndexInfo(Inst *iv, char *buffer)
{
	uint32_t samplesize = 0;
	InstSamplerState *s = iv->state;
	FOR_SAMPLECHAIN(i, s->sample)
		samplesize +=
				(*s->sample)[i]->length *
				(*s->sample)[i]->channels;
	humanReadableSize(samplesize, buffer);
}

static void samplerTriggerNote(uint32_t fptr, Inst *iv, Track *cv, uint8_t oldnote, uint8_t note, short inst)
{
	InstSamplerState *s = iv->state;
	InstSamplerPlaybackState *ps = cv->inststate;
	ps->sampleslot = s->samplemap[cv->r.note];
}

static struct json_object *samplerSerialize(void *state, size_t *dataoffset)
{
	int i;
	InstSamplerState *s = state;
	struct json_object *ret = json_object_new_object();

	struct json_object *array;

	array = json_object_new_array_ext(SAMPLE_MAX);
	for (i = 0; i < SAMPLE_MAX; i++)
		json_object_array_add(array, serializeSample((*s->sample)[i], dataoffset));

	json_object_object_add(ret, "sample", array);
	array = NULL;

	array = json_object_new_array_ext(NOTE_MAX);
	for (i = 0; i < NOTE_MAX; i++)
		json_object_array_add(array, json_object_new_int(s->samplemap[i]));
	json_object_object_add(ret, "samplemap", array);
	array = NULL;

	json_object_object_add(ret, "channelmode", json_object_new_string(SampleChannelsString[s->channelmode]));

	json_object_object_add(ret, "rateredux", json_object_new_int(s->rateredux));
	json_object_object_add(ret, "bitredux", json_object_new_int(s->bitredux));
	json_object_object_add(ret, "interpolate", json_object_new_boolean(s->interpolate));
	json_object_object_add(ret, "frame", json_object_new_int(s->frame));
	json_object_object_add(ret, "envelope", json_object_new_int(s->envelope));
	json_object_object_add(ret, "gain", json_object_new_int(s->gain));

	return ret;
}

void samplerSerializeData(FILE *fp, void *state, size_t *dataoffset)
{
	InstSamplerState *s = state;

	for (int i = 0; i < SAMPLE_MAX; i++)
		serializeSampleData(fp, (*s->sample)[i], dataoffset);
}

static void *samplerDeserialize(struct json_object *jso, void *data, double ratemultiplier)
{
	int i;
	InstSamplerState *ret = samplerInit();

	for (i = 0; i < SAMPLE_MAX; i++)
		(*ret->sample)[i] = deserializeSample(json_object_array_get_idx(json_object_object_get(jso, "sample"), i), data, ratemultiplier);

	for (i = 0; i < NOTE_MAX; i++)
		ret->samplemap[i] = json_object_get_int(json_object_array_get_idx(json_object_object_get(jso, "samplemap"), i));

	const char *string = json_object_get_string(json_object_object_get(jso, "channelmode"));
	for (i = 0; i < SAMPLE_CHANNELS_MAX; i++)
		if (!strcmp(string, SampleChannelsString[i]))
		{
			ret->channelmode = i;
			break;
		}

	ret->rateredux = json_object_get_int(json_object_object_get(jso, "rateredux"));
	ret->bitredux = json_object_get_int(json_object_object_get(jso, "bitredux"));
	ret->interpolate = json_object_get_boolean(json_object_object_get(jso, "interpolate"));
	ret->frame = json_object_get_int(json_object_object_get(jso, "frame"));
	ret->envelope = json_object_get_int(json_object_object_get(jso, "envelope"));
	ret->gain = json_object_get_int(json_object_object_get(jso, "gain"));

	return ret;
}

#include "macros.c"
#include "input.c"
#include "draw.c"
#include "process.c"
