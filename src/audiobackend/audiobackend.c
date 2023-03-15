#define AUDIO_GET_API_TRY(backend) if (backend##_audio_api.init && !backend##_audio_api.init()) { audio_api = backend##_audio_api; return 0; }
int audioInitAPI(void)
{
#ifdef DEBUG_DISABLE_AUDIO_OUTPUT
	AUDIO_GET_API_TRY(dummy);
	return 1;
#else


#ifdef OML_JACK
	AUDIO_GET_API_TRY(jack);
#endif

	AUDIO_GET_API_TRY(dummy);
	return 1;

#endif /* DEBUG_DISABLE_AUDIO_OUTPUT */
}
