typedef struct {
	uint8_t type;
	void   *state;
} Effect;
#define EFFECT_CHAIN_LEN 16
typedef struct {
	float  *input [2];
	float  *output[2];
	uint8_t c;
	Effect  v[];
} EffectChain;


#define MIN_EFFECT_WIDTH 38

#define NULL_EFFECT_HEIGHT 2
#define NULL_EFFECT_TEXT "NO EFFECTS"

enum {
	EFFECT_TYPE_NATIVE,
	EFFECT_TYPE_LADSPA,
	EFFECT_TYPE_LV2,
	EFFECT_TYPE_CLAP, /* TODO */
} EFFECT_TYPE;

/* IMPORTANT NOTE: effects should not register any more than 16 controls */
/* TODO: fix this, controls should be dynamically allocated              */

void freeEffect(Effect *e);
EffectChain *newEffectChain(float *input[2], float *output[2]);

/* NOTE: NOT thread safe */
void clearEffectChain(EffectChain *chain);

uint8_t getEffectControlCount(Effect *e);
EffectChain *_addEffect(EffectChain  *chain, unsigned long pluginindex, uint8_t chordindex);
void cb_addEffect(Event*); /* intended to be passed to addEffect().cb */
void          addEffect(EffectChain **chain, unsigned long pluginindex, uint8_t chordindex, void (*cb)(Event *));
EffectChain *_delEffect(EffectChain  *chain, uint8_t chordindex);
void          delEffect(EffectChain **chain, uint8_t chordindex);

/* cursor is (ControlState).cursor compatible */
uint8_t getEffectFromCursor(EffectChain *chain, uint8_t cursor);
uint8_t getCursorFromEffect(EffectChain *chain, uint8_t chordindex);

void copyEffect(EffectChain *destchain, Effect *dest, Effect *src);
void copyEffectChain(EffectChain **dest, EffectChain *src);

void serializeEffect(Effect *e, FILE *fp);
void deserializeEffect(EffectChain *chain, Effect *e, FILE *fp, uint8_t major, uint8_t minor);
void serializeEffectChain(EffectChain *chain, FILE *fp);
void deserializeEffectChain(EffectChain **chain, FILE *fp, uint8_t major, uint8_t minor);

short getEffectHeight(Effect *e);

void drawBoundingBox(short x, short y, short w, short h, short xmin, short xmax, short ymin, short ymax);
int drawEffect(Effect *e, ControlState *cc, bool selected, short x, short w, short y, short ymin, short ymax);

void runEffect(uint32_t samplecount, EffectChain *chain, Effect *e);


/* won't centre properly if multibyte chars are present */
void drawCentreText(short x, short y, short w, const char *text);

void drawAutogenPluginLine(ControlState *cc, short x, short y, short w,
		short ymin, short ymax,
		const char *name, float *value,
		bool toggled, bool integer,
		float min, float max, float def,
		char *prefix, char *postfix,
		uint32_t scalepointlen, uint32_t scalepointcount);

void drawEffects(EffectChain *chain, ControlState *cc, bool boldOutlines, short x, short width, short y);

#include "input.h"
