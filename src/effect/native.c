/* memcpying the instance should be safe */
typedef struct {
	uint32_t desc;     /* index in (NativeDB).descv */
	void    *instance; /* malleable data */
	void    *opaque;   /* static data, things like buffer pointers */
} NativeState;

typedef struct {
	uint8_t       controlc;
	short         height;
	char         *name;
	char         *author;
	bool          instrument;
	size_t        instance_size;
	void        (*init)(NativeState *state); /* should init instance and allocate opaque */
	void        (*free)(NativeState *state); /* should free opaque */
	void        (*draw)(NativeState *state, ControlState *cc, short x, short w, short y, short ymin, short ymax);
	void         (*run)(uint32_t samplecount, EffectChain *chain, void **instance);
} NATIVE_Descriptor;

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

void initNativeEffect(NativeDB *db, Effect *e, uint32_t desc)
{
	e->type = EFFECT_TYPE_NATIVE;
	e->state = calloc(1, sizeof(NativeState));
	((NativeState *)e->state)->desc = desc;
	((NativeState *)e->state)->instance = calloc(1, db->descv[desc]->instance_size);
	db->descv[desc]->init((NativeState *)e->state);
}
void freeNativeEffect(NativeDB *db, Effect *e)
{
	if (db->descv[((NativeState *)e->state)->desc]->free)
		db->descv[((NativeState *)e->state)->desc]->free((NativeState *)e->state);
	free(((NativeState *)e->state)->instance);
}

void copyNativeEffect(NativeDB *db, Effect *dest, Effect *src)
{
	NativeState *dests = dest->state;
	NativeState *srcs = src->state;
	initNativeEffect(db, dest, srcs->desc);
	memcpy(dests->instance, srcs->instance, db->descv[srcs->desc]->instance_size);
}

uint8_t getNativeEffectControlCount(NativeDB *db, Effect *e) { return db->descv[((NativeState *)e->state)->desc]->controlc; }
uint8_t getNativeEffectHeight      (NativeDB *db, Effect *e) { return db->descv[((NativeState *)e->state)->desc]->height;   }

void serializeNativeEffect(NativeDB *db, Effect *e, FILE *fp)
{
	fwrite(&((NativeState *)e->state)->desc, sizeof(uint32_t), 1, fp);
	fwrite(((NativeState *)e->state)->instance, db->descv[((NativeState *)e->state)->desc]->instance_size, 1, fp);
}
void deserializeNativeEffect(NativeDB *db, Effect *e, FILE *fp)
{
	fread(&((NativeState *)e->state)->desc, sizeof(uint32_t), 1, fp);
	initNativeEffect(db, e, ((NativeState *)e->state)->desc);
	fread(((NativeState *)e->state)->instance, db->descv[((NativeState *)e->state)->desc]->instance_size, 1, fp);
}

void drawNativeEffect(NativeDB *db, Effect *e, ControlState *cc,
		short x, short w,
		short y, short ymin, short ymax)
{
	db->descv[((NativeState *)e->state)->desc]->draw( (NativeState *)e->state, cc, x, w, y, ymin, ymax);
}

void runNativeEffect(NativeDB *db, uint32_t samplecount, EffectChain *chain, Effect *e)
{
	db->descv[((NativeState *)e->state)->desc]->run(samplecount, chain, &((NativeState *)e->state)->instance);
}
