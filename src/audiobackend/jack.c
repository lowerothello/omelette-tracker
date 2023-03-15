#include <jack/jack.h>     /* audio i/o                       */
#include <jack/midiport.h> /* midi i/o                        */
#include <jack/thread.h>   /* threading helper                */ /* TODO: remove, use raw pthreads */

typedef struct PortPair
{
	jack_port_t *l;
	jack_port_t *r;
} PortPair;
struct {
	jack_client_t *client;
	PortPair       in;
	PortPair       out;
	jack_port_t   *midiout;
	float *inbuffer[2];
	float *outbuffer[2];
	void *midibuffer;
} jackState;

/* stub jack error callback to hide errors, TODO: do something more useful */
static void _jackError(const char *message) { return; }

static void writeJackAudioSample(uint32_t bufptr, float left, float right)
{
	jackState.outbuffer[0][bufptr] = left;
	jackState.outbuffer[1][bufptr] = right;
}
static void readJackAudioSample(uint32_t bufptr, float *left, float *right)
{
	*left = jackState.inbuffer[0][bufptr];
	*right = jackState.inbuffer[0][bufptr];
}
static void writeJackMidiEvent(uint32_t bufptr, unsigned char *data, size_t datalength)
{
	jack_midi_event_write(jackState.midibuffer, bufptr, data, datalength);
}

static void *_jackProcess(jack_nframes_t nfptr)
{
	jackState.inbuffer [0] = jack_port_get_buffer(jackState.in.l, nfptr);
	jackState.inbuffer [1] = jack_port_get_buffer(jackState.in.r, nfptr);
	jackState.outbuffer[0] = jack_port_get_buffer(jackState.out.l, nfptr);
	jackState.outbuffer[1] = jack_port_get_buffer(jackState.out.r, nfptr);
	jackState.midibuffer   = jack_port_get_buffer(jackState.midiout, nfptr);
	jack_midi_clear_buffer(jackState.midibuffer);

	processOutput(nfptr);
	processInput(nfptr);
	return NULL;
}

static int initJackAudio(void)
{
	jack_set_error_function(_jackError);
	jackState.client = jack_client_open(PROGRAM_TITLE, JackNullOption, NULL);
	if (!jackState.client) return 1;

	jackState.in.l =    jack_port_register(jackState.client, "in_l",     JACK_DEFAULT_AUDIO_TYPE, JackPortIsInput |JackPortIsTerminal, 0);
	jackState.in.r =    jack_port_register(jackState.client, "in_r",     JACK_DEFAULT_AUDIO_TYPE, JackPortIsInput |JackPortIsTerminal, 0);
	jackState.out.l =   jack_port_register(jackState.client, "out_l",    JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput|JackPortIsTerminal, 0);
	jackState.out.r =   jack_port_register(jackState.client, "out_r",    JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput|JackPortIsTerminal, 0);
	jackState.midiout = jack_port_register(jackState.client, "out_midi", JACK_DEFAULT_MIDI_TYPE,  JackPortIsOutput|JackPortIsTerminal, 0);

	jack_set_process_callback(jackState.client, (int(*)(jack_nframes_t, void*))_jackProcess, NULL);

	samplerate = jack_get_sample_rate(jackState.client);
	buffersize = jack_get_buffer_size(jackState.client);

	return 0;
}

static void startJackAudio(void)
{
	jack_activate(jackState.client);
}

static void cleanJackAudio(void)
{
	if (!jackState.client) return;

	jack_deactivate(jackState.client);
	jack_client_close(jackState.client);
}

static int createJackRealtimeThread(pthread_t *thread, void *(*start_routine)(void*), void *arg)
{
	/* try spawning an rtprio thread, and if that fails then fall back to inheriting priority */
	/* TODO: do this with just the pthread library instead of using jack's wrapper */
	int ret;
	if ((ret = jack_client_create_thread(jackState.client, thread,
			jack_client_real_time_priority(jackState.client),
			jack_is_realtime(jackState.client),
			start_routine, arg)))
		ret = pthread_create(thread, NULL, start_routine, arg);

	return ret;
}

const AudioAPI jack_audio_api =
{
	initJackAudio,
	startJackAudio,
	cleanJackAudio,
	writeJackMidiEvent,
	writeJackAudioSample,
	readJackAudioSample,
	createJackRealtimeThread,
};
