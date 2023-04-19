typedef struct InstMidiState
{
	int8_t channel;
} InstMidiState;


void *midiInit(void);
void midiFree(Inst *iv);
void midiCopy(Inst *dest, Inst *src);
void midiGetIndexInfo(Inst *iv, char *buffer);
void midiDraw(Inst *iv, short x, short y, short width, short height, short minx, short maxx);
void midiInput(Inst *iv);
void midiMouse(Inst *iv, enum Button button, int x, int y);
void midiTriggerNote(uint32_t fptr, Inst *iv, Track *cv, float oldnote, float note, short inst);
static struct json_object *midiSerialize(void *state, size_t *dataoffset);
static void *midiDeserialize(struct json_object *jso, void *data, double ratemultiplier);

const InstAPI midiAPI =
{
	midiInit,
	midiFree,
	midiCopy,
	midiGetIndexInfo,
	midiDraw,
	midiInput,
	midiMouse,
	midiTriggerNote,
	NULL, /* process */
	NULL, /* lookback */
	midiSerialize,
	NULL, /* serializedata */
	midiDeserialize,
	0,
};
