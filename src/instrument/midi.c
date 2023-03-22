typedef struct InstMidiState
{
	int8_t channel;
} InstMidiState;

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

void midiGetIndexInfo(Inst *iv, char *buffer)
{
	snprintf(buffer, 9, "- MIDI -");
}
