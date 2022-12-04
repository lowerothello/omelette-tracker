typedef struct {
	enum WAVE_SHAPE shape;
	int8_t          duty;
	int8_t          count; /* up to 8 */

	uint8_t         freq_det;
	uint8_t         phase_det;
	uint8_t         stereo_det;
	uint8_t         duty_det;

	Phase phase[8];
} SuperOsc;
