void setBpm(uint16_t *spr, uint8_t newbpm);

/* freewheel to fill up the ramp buffer */
void ramp(Track *cv, float rp, uint8_t realinstrument);

void midiNoteOn (jack_nframes_t fptr, uint8_t miditrack, uint8_t note, uint8_t velocity);
void midiNoteOff(jack_nframes_t fptr, uint8_t miditrack, uint8_t note, uint8_t velocity);
void midiPC(jack_nframes_t fptr, uint8_t miditrack, uint8_t program);
void midiCC(jack_nframes_t fptr, uint8_t miditrack, uint8_t controller, uint8_t value);

bool triggerMidi(jack_nframes_t fptr, Track *cv, uint8_t oldnote, uint8_t note, uint8_t inst);
void triggerNote(jack_nframes_t fptr, Track *cv, uint8_t oldnote, uint8_t note, short inst);

void processRow(jack_nframes_t fptr, uint16_t *spr, bool midi, Track *cv, Row r);
void postSampler(jack_nframes_t fptr, Track *cv, float rp, float lf, float rf);

void playTrackLookback(jack_nframes_t fptr, uint16_t *spr, Track *cv);
void playTrack(jack_nframes_t fptr, uint16_t *spr, jack_nframes_t sprp, Track *cv);
void lookback(jack_nframes_t fptr, uint16_t *spr, uint16_t playfy, Track *cv);

void *dummyProcess(PlaybackInfo *p); /* cast to (void*(*)(void*)) */
int process(jack_nframes_t nfptr, PlaybackInfo *p); /* cast to (int(*)(jack_nframes_t, void*)) */
