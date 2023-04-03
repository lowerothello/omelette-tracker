typedef enum SampleChannels
{
	SAMPLE_CHANNELS_STEREO,
	SAMPLE_CHANNELS_LEFT,
	SAMPLE_CHANNELS_RIGHT,
	SAMPLE_CHANNELS_MIX,
	SAMPLE_CHANNELS_SWAP,
	SAMPLE_CHANNELS_MAX
} SampleChannels;
const char *SampleChannelsString[SAMPLE_CHANNELS_MAX] =
{
	"stereo",
	"left",
	"right",
	"mix",
	"swap"
};

typedef struct InstSamplerState
{
	SampleChain *sample;
	uint8_t samplemap[NOTE_MAX];

	SampleChannels channelmode;

	/* quality */
	uint8_t  rateredux;  /* percent of c5rate to actually use */
	int8_t   bitredux;
	bool     interpolate; /* lerp between samples, TODO: */

	uint8_t  frame;
	uint16_t envelope; /* deprecated, TODO: fasttracker2 style envelopes */
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

typedef struct InstSamplerPlaybackState
{
	float envgain;
	float modenvgain; /* wavetable modulation envelope */
	short localenvelope;
	short localsustain;
	MacroState pitchshift;
	MacroState pitchwidth;
	MacroState rateredux;
	int localcyclelength;
	uint16_t grainrampindex; /* progress through the grain ramp buffer */
	uint8_t sampleslot;
} InstSamplerPlaybackState;


static void *samplerInit(void);
static void samplerFree(Inst *iv);
static void samplerCopy(Inst *dest, Inst *src);
static void samplerGetIndexInfo(Inst *iv, char *buffer);
static void samplerDraw(Inst *iv, short x, short y, short width, short height, short minx, short maxx);
static void samplerInput(Inst *iv);
static void samplerMouse(Inst *iv, enum Button button, int x, int y);
static void samplerTriggerNote(uint32_t fptr, Inst *iv, Track *cv, uint8_t oldnote, uint8_t note, short inst);
static void samplerProcess(Inst *iv, Track *cv, float rp, uint32_t pointer, uint32_t pitchedpointer, float finetune, short *l, short *r);
static void samplerLookback(Inst *iv, Track *cv, uint16_t *spr);
static struct json_object *samplerSerialize(void *state, size_t *dataoffset);
void samplerSerializeData(FILE *fp, void *state, size_t *dataoffset);
static void *samplerDeserialize(struct json_object *jso, void *data, double ratemultiplier);

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
	samplerSerialize,
	samplerSerializeData,
	samplerDeserialize,
	sizeof(InstSamplerPlaybackState),
};

#include "macros.h"
