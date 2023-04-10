typedef struct AudioAPI
{
	int           (*init)(void); /* initialize backend state, should set samplerate and buffersize */
	void         (*start)(void); /* start processing buffers */
	void         (*clean)(void); /* stop processing and cleanup */
	void     (*writeMidi)(uint32_t bufptr, unsigned char *data, size_t datalen);
	void    (*writeAudio)(uint32_t bufptr, float left, float right);
	void     (*readAudio)(uint32_t bufptr, float *left, float *right);
	int (*realtimeThread)(pthread_t *thread, void *(*start_routine)(void*), void *arg); /* TODO: should be standardized */
} AudioAPI;
AudioAPI audio_api;

/* defined in "process.c" */
/* .buffersize is the number of samples to process */
void processOutput(uint32_t buffersize);
void processInput(uint32_t buffersize);


#include "dummy.c"
#ifdef OML_JACK
#include "jack.c"
#endif

/* returns true if no apis could be initialized */
bool audioInitAPI(void);
