void *midiInit(void)
{
	InstMidiState *ret = calloc(1, sizeof(InstMidiState));
	ret->channel = -1;

	return ret;
}
void midiFree(Inst *iv)
{
	free(iv->state);
}

void midiCopy(Inst *dest, Inst *src)
{
	dest->type = INST_TYPE_MIDI;
	dest->state = calloc(1, sizeof(InstMidiState));

	memcpy(dest->state, src->state, sizeof(InstMidiState));
}

void midiGetIndexInfo(Inst *iv, char *buffer)
{
	snprintf(buffer, 9, "- MIDI -");
}

void midiTriggerNote(uint32_t fptr, Inst *iv, Track *cv, uint8_t oldnote, uint8_t note, short inst)
{
	triggerMidi(fptr, iv->state, cv, oldnote, note, inst);
}

static struct json_object *midiSerialize(void *state, size_t *dataoffset)
{
	InstMidiState *s = state;
	return json_object_new_int(s->channel);
}

static void *midiDeserialize(struct json_object *jso, void *data, double ratemultiplier)
{
	InstMidiState *ret = midiInit();
	ret->channel = json_object_get_int(jso);
	return ret;
}

#include "draw.c"
#include "input.c"
