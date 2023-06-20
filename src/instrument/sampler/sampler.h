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
	Sample *sample;

	SampleChannels channelmode;

	/* quality */
	uint8_t  rateredux; /* percent of c5rate to actually use */
	int8_t   bitredux;
	bool     interpolate; /* lerp between samples */

	uint8_t  frame; /* TODO: */
	uint16_t envelope; /* deprecated, TODO: fasttracker2 style envelopes */
	int8_t   gain;

	bool     reverse; /* TODO: */
	int8_t   ramp;

	uint16_t cyclelength;
	uint8_t  cyclelengthjitter;
	uint8_t  transientsensitivity; /* sensitivity to transients triggering grains */

	int16_t  timestretch;
	bool     notestretch;

	int16_t  pitchshift; /* affects the grain length (the speed and the cycle size) */
	int8_t   pitchjitter;

	int16_t  formantshift; /* affects the speed the grain contents is played back */
	int8_t   formantstereo;

	/* granular */
	struct {
		int16_t  autotune;
		int8_t   autoscale;
		uint8_t  autospeed;
		uint8_t  autostrength;
	} granular;
} InstSamplerState;

typedef struct InstSamplerPlaybackState
{
	double pitchedpointer;
	double grainoffset, rawgrainoffset; /* grainoffset is affected by notestretch, rawgrainoffset is not */
	double oldgrainoffset, oldrawgrainoffset;
	uint32_t nextcyclestart;

	float transattackfollower;
	float transattackhighest;
	bool  transattackhold;

	float envgain;
	short localenvelope;
	short localsustain;
	CommandState pitchshift;
	CommandState rateredux;
	int localcyclelength;
	uint16_t grainrampindex; /* progress through the grain ramp buffer */
} InstSamplerPlaybackState;


static void *samplerInit(void);
static void samplerFree(Inst *iv);
static void samplerCopy(Inst *dest, Inst *src);
static void samplerGetIndexInfo(Inst *iv, char *buffer);
static void samplerDraw(Inst *iv, short x, short y, short width, short height, short minx, short maxx);
static void samplerInput(Inst *iv);
static void samplerMouse(Inst *iv, enum Button button, int x, int y);
static void samplerTriggerNote(uint32_t fptr, Inst *iv, Track *cv, float oldnote, float note, short inst);
static void samplerProcess(Inst *iv, Track *cv, float rp, uint32_t pointer, float finetune, short *l, short *r);
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

#include "commands.h"
