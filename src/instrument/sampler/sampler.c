static void *samplerInit(void)
{
	InstSamplerState *ret = calloc(1, sizeof(InstSamplerState));

	ret->rateredux = 0xff;
	ret->bitredux = 0xf;
	ret->envelope = 0x00f0;

	ret->cyclelength = 0x3fff;
	ret->ramp = 8;

	return ret;
}

static void samplerFree(Inst *iv)
{
	freeWaveform();

	InstSamplerState *s = iv->state;
	if (s->sample) free(s->sample);
	free(s);
}

/* dest has already been free'd */
static void samplerCopy(Inst *dest, Inst *src)
{
	dest->type = INST_TYPE_SAMPLER;
	InstSamplerState *ds = dest->state = calloc(1, sizeof(InstSamplerState));
	InstSamplerState *ss = src->state;

	memcpy(ds, ss, sizeof(InstSamplerState));

	if (ss->sample)
		copySample(&ds->sample, ss->sample);
}

static void samplerGetIndexInfo(Inst *iv, char *buffer)
{
	InstSamplerState *s = iv->state;
	humanReadableSize(s->sample->length * s->sample->channels, buffer);
}

static void samplerTriggerNote(uint32_t fptr, Inst *iv, Track *cv, float oldnote, float note, short inst)
{
	InstSamplerPlaybackState *ps = cv->inststate;
	ps->pitchedpointer = 0; /* TODO: skip resetting this if legato */
	ps->transattackfollower = 0.0f;
}

static struct json_object *samplerSerialize(void *state, size_t *dataoffset)
{
	InstSamplerState *s = state;
	struct json_object *ret = json_object_new_object();

	json_object_object_add(ret, "sample", serializeSample(s->sample, dataoffset));
	json_object_object_add(ret, "channelmode", json_object_new_string(SampleChannelsString[s->channelmode]));
	json_object_object_add(ret, "rateredux", json_object_new_int(s->rateredux));
	json_object_object_add(ret, "bitredux", json_object_new_int(s->bitredux));
	json_object_object_add(ret, "interpolate", json_object_new_boolean(s->interpolate));
	json_object_object_add(ret, "frame", json_object_new_int(s->frame));
	json_object_object_add(ret, "envelope", json_object_new_int(s->envelope));
	json_object_object_add(ret, "gain", json_object_new_int(s->gain));
	json_object_object_add(ret, "reverse", json_object_new_boolean(s->reverse));
	json_object_object_add(ret, "ramp", json_object_new_int(s->ramp));
	json_object_object_add(ret, "cyclelength", json_object_new_int(s->cyclelength));
	json_object_object_add(ret, "cyclelengthjitter", json_object_new_int(s->cyclelengthjitter));
	json_object_object_add(ret, "transientsensitivity", json_object_new_int(s->transientsensitivity));
	json_object_object_add(ret, "timestretch", json_object_new_int(s->timestretch));
	json_object_object_add(ret, "notestretch", json_object_new_boolean(s->notestretch));
	json_object_object_add(ret, "pitchshift", json_object_new_int(s->pitchshift));
	json_object_object_add(ret, "pitchjitter", json_object_new_int(s->pitchjitter));
	json_object_object_add(ret, "formantshift", json_object_new_int(s->formantshift));
	json_object_object_add(ret, "formantstereo", json_object_new_int(s->formantstereo));

	return ret;
}

void samplerSerializeData(FILE *fp, void *state, size_t *dataoffset)
{
	InstSamplerState *s = state;
	serializeSampleData(fp, s->sample, dataoffset);
}

static void *samplerDeserialize(struct json_object *jso, void *data, double ratemultiplier)
{
	int i;
	InstSamplerState *ret = samplerInit();

	ret->sample = deserializeSample(json_object_object_get(jso, "sample"), data, ratemultiplier);

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
	ret->reverse = json_object_get_boolean(json_object_object_get(jso, "reverse"));
	ret->ramp = json_object_get_int(json_object_object_get(jso, "ramp"));
	ret->cyclelength = json_object_get_int(json_object_object_get(jso, "cyclelength"));
	ret->cyclelengthjitter = json_object_get_int(json_object_object_get(jso, "cyclelengthjitter"));
	ret->transientsensitivity = json_object_get_int(json_object_object_get(jso, "transientsensitivity"));
	ret->timestretch = json_object_get_int(json_object_object_get(jso, "timestretch"));
	ret->notestretch = json_object_get_boolean(json_object_object_get(jso, "notestretch"));
	ret->pitchshift = json_object_get_int(json_object_object_get(jso, "pitchshift"));
	ret->pitchjitter = json_object_get_int(json_object_object_get(jso, "pitchjitter"));
	ret->formantshift = json_object_get_int(json_object_object_get(jso, "formantshift"));
	ret->formantstereo = json_object_get_int(json_object_object_get(jso, "formantstereo"));

	return ret;
}

#include "commands.c"
#include "draw.c"
#include "input.c" /* depends on "draw.c", TODO: kinda dumb dependancy */
#include "process.c"
