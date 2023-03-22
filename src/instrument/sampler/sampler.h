typedef enum SampleChannels
{
	SAMPLE_CHANNELS_STEREO,
	SAMPLE_CHANNELS_LEFT,
	SAMPLE_CHANNELS_RIGHT,
	SAMPLE_CHANNELS_MIX,
	SAMPLE_CHANNELS_SWAP,
} SampleChannels;

typedef struct InstSamplerState
{
	SampleChain *sample;
	uint8_t samplemap[NOTE_MAX];

	SampleChannels channelmode;

	/* quality */
	uint8_t  samplerate;  /* percent of c5rate to actually use */
	int8_t   bitdepth;
	bool     interpolate; /* lerp between samples, TODO: implement */

	uint8_t  frame;
	uint16_t envelope;
	int8_t   gain;

	/* granular */
	struct {
		uint16_t cyclelength;
		uint8_t  cyclelengthjitter;
		uint8_t  reversegrains;
		int8_t   rampgrains;
		int16_t  timestretch;
		bool     notestretch;
		int16_t  pitchshift;
		int16_t  formantshift;
		int8_t   pitchstereo;
		uint8_t  pitchjitter;
		uint8_t  formantjitter;
		int8_t   pitchoctaverange;
		int8_t   pitchoctavechance;
		uint8_t  panjitter;
		uint8_t  ptrjitter;
		int16_t  autotune;
		int8_t   autoscale;
		uint8_t  autospeed;
		uint8_t  autostrength;
		uint8_t  beatsensitivity;
		uint8_t  beatdecay;
	} granular;

	/* wavetable */
	struct {
		uint32_t framelength;
		uint8_t  wtpos;
		int8_t   syncoffset;
		int8_t   pulsewidth;
		int8_t   phasedynamics;
		uint16_t envelope;
		uint8_t  lfospeed;
		int8_t   lfoduty;
		bool     lfoshape;
		struct {
			int8_t wtpos;
			int8_t sync;
			int8_t cutoff;
			int8_t phase;
			int8_t pwm;
			int8_t pdyn;
		} env;
		struct {
			uint8_t gain;
			int8_t  wtpos;
			int8_t  sync;
			int8_t  cutoff;
			int8_t  phase;
			int8_t  pwm;
			int8_t  pdyn;
		} lfo;
	} wavetable;
} InstSamplerState;


void samplerInit(Inst *iv);
void samplerFree(Inst *iv);
void samplerCopy(Inst *dest, Inst *src);
void samplerGetIndexInfo(Inst *iv, char *buffer);
void samplerDraw(Inst *iv, short x, short y, short width, short height, short minx, short maxx);
void samplerInput(Inst *iv);
void samplerMouse(Inst *iv, enum Button button, int x, int y);
void samplerTriggerNote(Inst *iv, Track *cv);
void samplerProcess(Inst *iv, Track *cv, float rp, uint32_t pointer, uint32_t pitchedpointer, float finetune, short *l, short *r);
void samplerLookback(Inst *iv, Track *cv, uint16_t *spr);

const InstAPI samplerAPI =
{
	samplerInit,
	samplerFree,
	samplerCopy,
	samplerGetIndexInfo,
	samplerDraw,
	samplerInput,
	samplerMouse,
	samplerTriggerNote,
	samplerProcess,
	samplerLookback,
};

#include "macros.h"
