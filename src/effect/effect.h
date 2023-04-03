#define EFFECT_CONTROL_DEF_MAX 4.0f
#define EFFECT_CONTROL_DEF_MIN 0.0f

typedef enum EffectType
{
	EFFECT_TYPE_DUMMY  = 0,
	EFFECT_TYPE_LADSPA = 1,
	EFFECT_TYPE_LV2    = 2,
	EFFECT_TYPE_COUNT
} EffectType;

typedef struct EffectBrowserLine
{
	char *name;  /* free'd */
	char *maker; /* free'd */
	const void *data; /* passed into api->init() */
} EffectBrowserLine;

typedef struct EffectAPI
{
	const char *name;
	void                  (*init_db)(void);
	void                  (*free_db)(void);
	uint32_t             (*db_count)(void); /* get how many plugins */
	EffectBrowserLine     (*db_line)(uint32_t index); /* get a plugin out of the database */
	void*                    (*init)(const void *data, float **i, float **o); /* returns a new state, initialized against .data */
	void                     (*free)(void *state); /* frees .state */
	void                     (*copy)(void *state, void *src, float **i, float **o); /* copies from .src to .state */
	void                      (*run)(void *state, uint32_t bufsize, float **i, float **o); /* process .bufsize samples from .i to .o */
	uint32_t             (*controlc)(void *state); /* get how many controls .state has */
	short                  (*height)(void *state); /* get the height in rows .state wants */
	void                     (*draw)(void *state, short x, short w, short y, short ymin, short ymax);
	struct json_object* (*serialize)(void *state);
	void*             (*deserialize)(struct json_object *jso, float **i, float **o);
} EffectAPI;

#include "dummy.h"

#ifdef OML_LADSPA
#include "ladspa.h"
#endif

#ifdef OML_LV2
#include "lv2.h"
#endif

EffectAPI *effectGetAPI(void);

void initEffectDB(void);
void freeEffectDB(void);

typedef struct Effect
{
	EffectType type;
	void      *state;
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
EffectChain *newEffectChain(void);

/* NOTE: NOT thread safe */
void clearEffectChain(EffectChain*);
void freeEffectChain(EffectChain*);
uint32_t getEffectControlCount(Effect*);

void cb_addEffect(Event*); /* intended to be passed to addEffect().cb */

/* .srcindex == ((uint32_t)-1) to paste */
void addEffect(EffectChain**, EffectType type, uint32_t srcindex, uint8_t destindex, void (*cb)(Event*));

EffectChain *_delEffect(EffectChain*, uint8_t index);
void delEffect(EffectChain**, uint8_t index);

/* cursor is (ControlState).cursor compatible */
uint8_t getEffectFromCursor(EffectChain*, uint8_t cursor);
uint8_t getCursorFromEffect(EffectChain*, uint8_t index);

void copyEffect(Effect *dest, Effect *src, float **input, float **output);
void copyEffectChain(EffectChain **dest, EffectChain *src);

void runEffect(uint32_t samplecount, EffectChain*, Effect*);

struct json_object *serializeEffectChain(EffectChain*);
EffectChain *deserializeEffectChain(struct json_object*);

#include "draw.h"
