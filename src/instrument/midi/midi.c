void midiInit(Inst *iv)
{
	iv->type = INST_TYPE_MIDI;

	InstMidiState *s = iv->state = calloc(1, sizeof(InstMidiState));

	s->channel = -1;
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

void midiProcess(Inst *iv, Track *cv, float rp, uint32_t pointer, uint32_t pitchedpointer, float finetune, short *l, short *r)
{ }

void midiLookback(Inst *iv, Track *cv, uint16_t *spr)
{ }

#include "draw.c"
#include "input.c"
