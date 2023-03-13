pthread_t dummyprocessthread;

uint32_t getSampleRate(void) { return DEBUG_DUMMY_SAMPLERATE; }
uint32_t getBufferSize(void) { return DEBUG_DUMMY_BUFFERSIZE; }

static void *_dummyProcess(void)
{
	struct timespec req;
	long nsec = (buffersize / samplerate) * 1000000;
	while (1)
	{
		processOutput(buffersize);
		processInput(buffersize);
		req.tv_sec = 0;
		req.tv_nsec = nsec;
		while(nanosleep(&req, &req) < 0);
	}
	return NULL;
}

int initAudio(void)
{
	return 0;
}

void startAudio(void)
{
	pthread_create(&dummyprocessthread, NULL, (void*(*)(void*))_dummyProcess, NULL);
}

void cleanAudio(void)
{
	pthread_cancel(dummyprocessthread);
	pthread_join(dummyprocessthread, NULL);
}

void writeMidiEvent(uint32_t buffersample, unsigned char *data, size_t datalength) { }
void writeAudioSample(uint32_t bufptr, float left, float right) { }
void readAudioSample(uint32_t bufptr, float *left, float *right) { }

int createRealtimeThread(pthread_t *thread, void *(*start_routine)(void*), void *arg)
{
	return pthread_create(thread, NULL, start_routine, arg);
}
