enum GRANULAR_MODE {
	GRANULAR_MODE_CYCLIC,
	GRANULAR_MODE_TONAL,
	GRANULAR_MODE_BEAT,
};

enum PITCH_STEP {
	PITCH_STEP_FINE,
	PITCH_STEP_FIFTH,
	PITCH_STEP_OCTAVE,
	PITCH_STEP_HARMONIC,
};

typedef struct {
	enum GRANULAR_MODE mode;

	int16_t timestretch;
	int16_t pitchshift;

	enum PITCH_STEP pitchstep;

	/* sampled for each grain */
	uint16_t grainlength;
	int8_t   grainpointer;
	int8_t   grainpitch;
	int8_t   grainpan;
	uint8_t  graincrossfade; /* square <-> triangle */
	bool     grainreverse;
	uint8_t  grainrepeats;

	struct {
		uint32_t start;
		uint32_t length;
		int8_t   pitch;
		int8_t   pan;
		uint8_t  window;
		bool     reverse;
		uint8_t  repeatprogress; /* how far this grain is into a repeat chain */
	} grain;

} GranularCyclic;

typedef struct {
	enum GRANULAR_MODE mode;

	int16_t timestretch;
	int16_t pitchshift;
	int16_t formantshift;
	int8_t  pitchstereo;

	uint8_t crossfade; /* square <-> triangle */

	struct {
		uint8_t    speed;
		enum SCALE scale;
	} autotune;

	struct {
		uint32_t start;
		uint32_t length;
	} grain;
} GranularTonal;

typedef struct {
	enum GRANULAR_MODE mode;

	int16_t timestretch;
	int16_t pitchshift;

	uint8_t threshold;
	uint8_t decay;

	struct {
		uint32_t start;
		uint32_t length;
		float    gain; /* decay envelope */
	} grain;
} GranularBeat;

union Granular {
	GranularCyclic cyclic;
	GranularTonal tonal;
	GranularBeat beat;
};

typedef struct {
	uint32_t pointer;

	uint32_t trimstart;
	uint32_t trimlength;
	uint32_t looplength;

	bool    pingpong;
	uint8_t loopramp;
	bool    notestretch; /* subject to move */

	union Granular granular;
} SampleOsc;
