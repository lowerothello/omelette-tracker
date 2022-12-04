void instrumentSamplerControlCallback(void *arg);
void resetWaveform(void);
void resizeWaveform(void);
void freeWaveform(void);
void drawMarker(uint32_t marker, size_t offset, size_t width);
void *walkWaveformRoutine(Instrument *iv);
void drawWaveform(Instrument *iv);
