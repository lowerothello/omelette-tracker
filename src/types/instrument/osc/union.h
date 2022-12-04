typedef struct {
	int8_t  duty;
	int8_t  saw_mix; /* +saw, -ramp */
	uint8_t pulse_mix;    /* duty */
	uint8_t triangle_mix; /* duty */
	uint8_t sine_mix;     /* duty */

	Phase   phase;
} UnionOsc;
