#ifdef DEBUG_DISABLE_AUDIO_OUTPUT
pthread_t dummyprocessthread;
#endif

jack_client_t *client;

void cleanup(int ret);
void init(int argc, char *argv[]);
