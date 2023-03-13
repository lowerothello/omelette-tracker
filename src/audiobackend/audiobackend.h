/* return true if initializing failed */
int initAudio(void);   /* initialize backend state    */
void startAudio(void); /* start processing buffers    */
void cleanAudio(void); /* stop processing and cleanup */

uint32_t getSampleRate(void);
uint32_t getBufferSize(void);

/* called inside process() */
void writeMidiEvent(uint32_t bufptr, unsigned char *data, size_t datalength);
void writeAudioSample(uint32_t bufptr, float left, float right);
void readAudioSample(uint32_t bufptr, float *left, float *right);

int createRealtimeThread(pthread_t *thread, void *(*start_routine)(void*), void *arg);

/* defined in "process.c" */
/* .buffersize is the number of samples to process */
void processOutput(uint32_t buffersize);
void processInput(uint32_t buffersize);

/* use jack if it's available, else use the dummy */
/* TODO: this should be done at runtime with a case switch i think? */
#ifdef DEBUG_DISABLE_AUDIO_OUTPUT
#include "dummy.c"
#else
#ifdef OML_JACK
#include "jack.c"
#else
#include "dummy.c"
#endif /* OML_JACK */
#endif /* DEBUG_DISABLE_AUDIO_OUTPUT */
