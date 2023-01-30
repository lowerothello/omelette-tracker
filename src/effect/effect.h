enum EFFECT_TYPE {
	EFFECT_TYPE_DUMMY = 0,
	EFFECT_TYPE_LADSPA,
	EFFECT_TYPE_LV2,
};

typedef struct {
	enum EFFECT_TYPE type;
	void            *state;
} Effect;
#define EFFECT_CHAIN_LEN 16
typedef struct {
	float  *input [2];
	float  *output[2];
	uint8_t c;
	Effect  v[];
} EffectChain;


#define MIN_EFFECT_WIDTH 38

#define NULL_EFFECT_HEIGHT 3
#define NULL_EFFECT_TEXT "NO EFFECTS"
#define DUMMY_EFFECT_TEXT "DUMMY EFFECT"

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

void serializeEffect(Effect*, FILE*);
void deserializeEffect(EffectChain*, Effect*, FILE*, uint8_t major, uint8_t minor);
void serializeEffectChain(EffectChain*, FILE*);
void deserializeEffectChain(EffectChain**, FILE*, uint8_t major, uint8_t minor);

short getEffectHeight(Effect*);

/* draw effects themselves */
// static int  _drawEffect (Effect*,      ControlState*, bool selected,     short x, short width, short y, short ymin, short ymax);
// static void _drawEffects(EffectChain*, ControlState*, bool boldOutlines, short x, short width, short y);

/* draw the full effect page */
void drawEffect(void);

/* draw a single effect chain */
void drawEffectChain(EffectChain*, ControlState*, short x, short width, short y);

void runEffect(uint32_t samplecount, EffectChain*, Effect*);


/* won't centre properly if multibyte chars are present */
void drawCentreText(short x, short y, short w, const char *text);

void drawAutogenPluginLine(ControlState*, short x, short y, short w,
		short ymin, short ymax,
		const char *name, float *value,
		bool toggled, bool integer,
		float min, float max, float def,
		char *prefix, char *postfix,
		uint32_t scalepointlen, uint32_t scalepointcount);
