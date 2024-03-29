void setBpm(uint16_t *spr, uint8_t newbpm);

/* freewheel to fill up the ramp buffer */
void ramp(uint32_t fptr, uint16_t *spr, uint32_t sprp, float rowprogress, Track *cv, uint8_t realinstrument);

void midiNoteOn (uint32_t fptr, uint8_t miditrack, float note, uint8_t velocity);
void midiNoteOff(uint32_t fptr, uint8_t miditrack, float note, uint8_t velocity);
void midiPC(uint32_t fptr, uint8_t miditrack, uint8_t program);
void midiCC(uint32_t fptr, uint8_t miditrack, uint8_t controller, uint8_t value);

void midiMute(Inst *iv, Track *cv);
bool triggerMidi(uint32_t fptr, InstMidiState *ims, Track *cv, float oldnote, float note, uint8_t inst);
void triggerNote(uint32_t fptr, Track *cv, float oldnote, float note, short inst);

void processRow(uint32_t fptr, uint16_t *spr, bool midi, Track *cv, Row *r);
void postSampler(uint32_t fptr, Track *cv, float rp, float lf, float rf);

void playTrackLookback(uint32_t fptr, uint16_t *spr, Track *cv);
void playTrack(uint32_t fptr, uint16_t *spr, uint32_t sprp, Track *cv);
void lookback(uint32_t fptr, uint16_t *spr, uint16_t playfy, Track *cv);
