#include "ladspa.h"
#include "lv2.h"

enum EFFECT_TYPE /* cast to a (uint8_t) */
{ /* strict about indices */
	EFFECT_TYPE_DUMMY  = 0,
	EFFECT_TYPE_LADSPA = 1,
	EFFECT_TYPE_LV2    = 2,
};

/* TODO: effect should be a union */
typedef union
{
	uint8_t     type;
	LadspaState ladspa;
	LV2State    lv2;
} Effect;

#define EFFECT_CHAIN_LEN 16
typedef struct EffectChain
{
	float  *input [2];
	float  *output[2];
	uint8_t c;
	Effect  v[];
} EffectChain;


/* IMPORTANT NOTE: effects should not register any more than 16 controls */
/* TODO: fix this, controls should be dynamically allocated              */

void freeEffect(Effect*);
EffectChain *newEffectChain(float *input[2], float *output[2]);

/* NOTE: NOT thread safe */
void clearEffectChain(EffectChain*);
uint8_t getEffectControlCount(Effect*);

EffectChain *_addEffect(EffectChain*, unsigned long pluginindex, uint8_t index);

void cb_addEffect(Event*); /* intended to be passed to addEffect().cb */

/* pluginindex is offset by 1 */
void addEffect(EffectChain**, unsigned long pluginindex, uint8_t index, void (*cb)(Event*));

EffectChain *_delEffect(EffectChain*, uint8_t index);
void delEffect(EffectChain**, uint8_t index);

/* cursor is (ControlState).cursor compatible */
uint8_t getEffectFromCursor(EffectChain*, uint8_t cursor);
uint8_t getCursorFromEffect(EffectChain*, uint8_t index);

void copyEffect(Effect *dest, Effect *src, float **input, float **output);
void copyEffectChain(EffectChain **dest, EffectChain *src);

void runEffect(uint32_t samplecount, EffectChain*, Effect*);
