#include <pulse/pulseaudio.h>

struct {
	pa_mainloop *ml;
	pa_mainloop_api *mlapi;
	pa_context *ctx;
	pa_stream *out_stream;
	pa_stream *in_stream;
	pa_sample_spec ss;
	void *out_data;
	const void *in_data;
	uint32_t samplerate;
	uint32_t buffersize;
	pthread_t thread;
} pulseState;

void pa_state_cb(pa_context *c, void *userdata)
{
	int *pa_ready = userdata;
	switch (pa_context_get_state(c))
	{
		case PA_CONTEXT_FAILED: /* fall through */
		case PA_CONTEXT_TERMINATED:
			*pa_ready = 2; /* fail */
			break;
		case PA_CONTEXT_READY:
			*pa_ready = 1; /* success */
			break;
		default: break;
	}
}

/* pulseaudio doesn't handle midi */
void writeMidiEvent(uint32_t bufptr, unsigned char *data, size_t datalength) { }

void writeAudioSample(uint32_t bufptr, float left, float right)
{
	size_t framesize = pa_frame_size(&pulseState.ss);
	size_t samplesize = pa_sample_size(&pulseState.ss);
	size_t offset = bufptr * framesize;
	memcpy(pulseState.out_data+offset,            &left,  samplesize);
	memcpy(pulseState.out_data+offset+samplesize, &right, samplesize);
}
void readAudioSample(uint32_t bufptr, float *left, float *right)
{
	size_t framesize = pa_frame_size(&pulseState.ss);
	size_t samplesize = pa_sample_size(&pulseState.ss);
	size_t offset = bufptr * framesize;
	memcpy(left,  pulseState.in_data+offset,            samplesize);
	memcpy(right, pulseState.in_data+offset+samplesize, samplesize);
}

static void _pulseProcessOutput(pa_stream *p, size_t nbytes)
{
	pa_stream_begin_write(p, &pulseState.out_data, &nbytes);
	processOutput(nbytes/pa_frame_size(&pulseState.ss));
	pa_stream_write(p, pulseState.out_data, nbytes, NULL, 0, PA_SEEK_RELATIVE);
}
static void _pulseProcessInput(pa_stream *p, size_t nbytes)
{
	pa_stream_peek(p, &pulseState.in_data, &nbytes);
	if (nbytes) processInput(nbytes/pa_frame_size(&pulseState.ss));
	pa_stream_drop(p);
}

static void _defTimings_Sink_cb(pa_context *c, const pa_sink_info *i, int eol, void *userdata)
{
	if (eol) return;
	samplerate = i->sample_spec.rate;
	buffersize = ((double)i->latency / (double)PA_USEC_PER_SEC) * samplerate;
}
static void _defTimings_Server_cb(pa_context *c, const pa_server_info *i, void *userdata)
{
	pa_operation_unref(pa_context_get_sink_info_by_name(c, i->default_sink_name, _defTimings_Sink_cb, userdata));
}

int initAudio(void)
{
	pulseState.ml = pa_mainloop_new();
	pulseState.mlapi = pa_mainloop_get_api(pulseState.ml);
	pulseState.ctx = pa_context_new(pulseState.mlapi, PROGRAM_TITLE);
	pa_context_connect(pulseState.ctx, NULL, 0, NULL);

	/* wait for the server to respond */
	int ready = 0;
	pa_context_set_state_callback(pulseState.ctx, pa_state_cb, &ready);

	while (!ready)
		pa_mainloop_iterate(pulseState.ml, 1, NULL);

	if (ready == 2)
	{
		puts("failed to init the pulseaudio client");
		return 1;
	}

	pa_operation_unref(pa_context_get_server_info(pulseState.ctx, _defTimings_Server_cb, NULL));

	while (!samplerate)
		pa_mainloop_iterate(pulseState.ml, 1, NULL);

	pulseState.ss.format = PA_SAMPLE_FLOAT32NE;
	pulseState.ss.rate = samplerate;
	pulseState.ss.channels = 2;

	pulseState.out_stream = pa_stream_new(pulseState.ctx, "Output", &pulseState.ss, NULL);
	if (!pulseState.out_stream) { puts("failed to create the pulseaudio output stream"); return 1; }
	pulseState.in_stream = pa_stream_new(pulseState.ctx, "Input", &pulseState.ss, NULL);
	if (!pulseState.in_stream) { puts("failed to create the pulseaudio input stream"); return 1; }

	pa_stream_set_write_callback(pulseState.out_stream, (void(*)(pa_stream*, size_t, void*))_pulseProcessOutput, NULL);
	pa_stream_set_read_callback(pulseState.in_stream, (void(*)(pa_stream*, size_t, void*))_pulseProcessInput, NULL);
	/* TODO: might want to round buffersize up to the nearest power of 2 */

	// buffersize >>= 2;

	buffersize = samplerate >> 8;
	pa_buffer_attr bufattr;
	bufattr.fragsize =  buffersize * pa_frame_size(&pulseState.ss);
	bufattr.maxlength = buffersize * pa_frame_size(&pulseState.ss);
	bufattr.tlength =   buffersize * pa_frame_size(&pulseState.ss);
	bufattr.minreq =                 pa_frame_size(&pulseState.ss);
	bufattr.prebuf = -1;
	pa_stream_connect_playback(pulseState.out_stream, NULL, &bufattr, 0, NULL, NULL);
	pa_stream_connect_record  (pulseState.in_stream,  NULL, &bufattr, 0);

	return 0;
}

void startAudio(void)
{
	pthread_create(&pulseState.thread, NULL, (void*(*)(void*))pa_mainloop_run, pulseState.ml); /* this cast brings me great joy */
}

void cleanAudio(void)
{
	pthread_cancel(pulseState.thread);
	pthread_join(pulseState.thread, NULL);
	pa_mainloop_quit(pulseState.ml, 0);
	pa_context_disconnect(pulseState.ctx);
	pa_context_unref(pulseState.ctx);
	pa_mainloop_free(pulseState.ml);
}

int createRealtimeThread(pthread_t *thread, void *(*start_routine)(void*), void *arg)
{
	return pthread_create(thread, NULL, start_routine, arg);
}
