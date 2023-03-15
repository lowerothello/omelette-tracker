pthread_t dummyprocessthread;

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

static int initDummyAudio(void)
{
	samplerate = DEBUG_DUMMY_SAMPLERATE;
	buffersize = DEBUG_DUMMY_BUFFERSIZE;
	return 0;
}

static void startDummyAudio(void)
{
	pthread_create(&dummyprocessthread, NULL, (void*(*)(void*))_dummyProcess, NULL);
}

static void cleanDummyAudio(void)
{
	pthread_cancel(dummyprocessthread);
	pthread_join(dummyprocessthread, NULL);
}

static void writeDummyMidiEvent(uint32_t buffersample, unsigned char *data, size_t datalength) { }
static void writeDummyAudioSample(uint32_t bufptr, float left, float right) { }
static void readDummyAudioSample(uint32_t bufptr, float *left, float *right) { }

static int createDummyRealtimeThread(pthread_t *thread, void *(*start_routine)(void*), void *arg)
{
	return pthread_create(thread, NULL, start_routine, arg);
}

const AudioAPI dummy_audio_api =
{
	initDummyAudio,
	startDummyAudio,
	cleanDummyAudio,
	writeDummyMidiEvent,
	writeDummyAudioSample,
	readDummyAudioSample,
	createDummyRealtimeThread,
};
