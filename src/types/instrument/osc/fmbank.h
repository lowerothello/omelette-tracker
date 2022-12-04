typedef struct
{
	uint8_t fm[3][3];
	uint8_t pm[3][3];

	struct {
		enum WAVE_SHAPE shape;
		int8_t          duty;
		int16_t         pitch;
		uint8_t         gain;
	} osc[3];
} FmbankOsc;
