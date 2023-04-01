typedef struct InstMidiState
{
	int8_t channel;
} InstMidiState;


void midiInit(Inst *iv);
void midiFree(Inst *iv);
void midiCopy(Inst *dest, Inst *src);
void midiGetIndexInfo(Inst *iv, char *buffer);
void midiDraw(Inst *iv, short x, short y, short width, short height, short minx, short maxx);
void midiInput(Inst *iv);
void midiMouse(Inst *iv, enum Button button, int x, int y);
void midiTriggerNote(uint32_t fptr, Inst *iv, Track *cv, uint8_t oldnote, uint8_t note, short inst);
void midiProcess(Inst *iv, Track *cv, float rp, uint32_t pointer, uint32_t pitchedpointer, float finetune, short *l, short *r);
void midiLookback(Inst *iv, Track *cv, uint16_t *spr);

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
	midiProcess,
	midiLookback,
	0,
};
