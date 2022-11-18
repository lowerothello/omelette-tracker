typedef struct {
	uint8_t controlc;
	short   height;
	char   *name;
	char   *author;
	bool    instrument;
	void        (*init)(void **instance);
	void        (*free)(void **instance); /* should free the instance */
	void        (*copy)(void **dest, void **src);
	void   (*serialize)(void **instance, FILE *fp);
	void (*deserialize)(void **instance, FILE *fp);
	void        (*draw)(void **instance, ControlState *cc, short x, short w, short y, short ymin, short ymax);
	void         (*run)(uint32_t samplecount, EffectChain *chain, void **instance);
} NATIVE_Descriptor;

typedef struct {
	NATIVE_Descriptor *desc;
	void              *instance;
} NativeState;

typedef struct {
	uint32_t            descc;
	NATIVE_Descriptor **descv;
} NativeDB;
NativeDB native_db;

#include "../effect/native/distortion.c"
#include "../effect/native/equalizer.c"

void initNativeDB(void)
{
	native_db.descc = 2;
	native_db.descv = malloc(2 * sizeof(NATIVE_Descriptor*));
	native_db.descv[0] = distortionDescriptor();
	native_db.descv[1] = equalizerDescriptor();
}

void freeNativeDescriptor(NATIVE_Descriptor *desc)
{
	free(desc->name);
	free(desc->author);
	free(desc);
}

void freeNativeDB(void)
{
	freeNativeDescriptor(native_db.descv[0]);
	freeNativeDescriptor(native_db.descv[1]);
	free(native_db.descv);
}

void initNativeEffect(Effect *e, NATIVE_Descriptor *desc)
{
	e->type = EFFECT_TYPE_NATIVE;
	e->state = calloc(1, sizeof(NativeState));
	((NativeState *)e->state)->desc = desc;
	desc->init(&((NativeState *)e->state)->instance);
}
void freeNativeEffect(Effect *e)
{
	((NativeState *)e->state)->desc->free(&((NativeState *)e->state)->instance);
}

void copyNativeEffect(Effect *dest, Effect *src)
{
	dest->state = calloc(1, sizeof(NativeState));
	((NativeState *)dest->state)->desc = ((NativeState *)src->state)->desc;

	((NativeState *)dest->state)->desc->copy(
		&((NativeState *)dest->state)->instance,
		&((NativeState *)src ->state)->instance);
}

uint8_t getNativeEffectControlCount(Effect *e) { return ((NATIVE_Descriptor *)((NativeState *)e->state)->desc)->controlc; }
uint8_t getNativeEffectHeight      (Effect *e) { return ((NATIVE_Descriptor *)((NativeState *)e->state)->desc)->height;   }

void serializeNativeEffect(Effect *e, FILE *fp)
{
	((NativeState *)e->state)->desc->serialize(&((NativeState *)e->state)->instance, fp);
}
void deserializeNativeEffect(Effect *e, FILE *fp)
{
	((NativeState *)e->state)->desc->deserialize(&((NativeState *)e->state)->instance, fp);
}

void drawNativeEffect(Effect *e, ControlState *cc,
		short x, short w,
		short y, short ymin, short ymax)
{
	((NativeState *)e->state)->desc->draw(&((NativeState *)e->state)->instance,
		cc, x, w, y, ymin, ymax);
}

void runNativeEffect(uint32_t samplecount, EffectChain *chain, Effect *e)
{
	((NativeState *)e->state)->desc->run(samplecount, chain,
		&((NativeState *)e->state)->instance);
}
