typedef struct
{
	uint8_t lfospeed;
	int8_t  lfospeedstereo;
	bool    lfophasestereo;

	uint8_t delaymin;
	uint8_t delayrange;

	uint8_t taps;

	int8_t  sidechaincutoff;
} ChorusState;
