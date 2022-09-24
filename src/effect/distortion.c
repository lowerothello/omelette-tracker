/*
 * bias:
 *   add dc offset to the input signal
 *
 * biasstereo:
 *   invert bias for the right channel
 *
 * drive:
 *   matched input/output gain, distort
 *   more/less without affecting the volume
 *
 * emphasis:
 *   matched input/output bell filters,
 *   distort certain frequencies more/less
 *
 * sidechaincutoff:
 *   dj-style low/high pass to
 *   only distort the highs/lows
 */
#define E_D_ALG_HARDCLIP 0;
#define E_D_ALG_SOFTCLIP 1;
#define E_D_ALG_HARDFOLD 2;
#define E_D_ALG_SOFTFOLD 3;
#define E_D_ALG_HARDGATE 4;
#define E_D_ALG_WAVEWRAP 5;
typedef struct
{
	int8_t  bias;
	bool    biasstereo;
	int8_t  drive;

	uint8_t emphasisfrequency;
	int8_t  emphasisgain;
	uint8_t emphasisbandwidth;

	int8_t  sidechaincutoff;

	uint8_t algorithm;
} DistortionState;


void initDistortion(Effect *e)
{
	e->state = calloc(1, sizeof(DistortionState));
}

/* returns the height of the widget */
int drawDistortion(Effect *e, ControlState *cc,
		short x, short y, short width, short maxheight)
{
}
