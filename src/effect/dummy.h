#define DUMMY_EFFECT_HEIGHT 3
#define DUMMY_EFFECT_TEXT "DUMMY EFFECT"

static uint32_t getDummyEffectControlCount(void *state);
static short getDummyEffectHeight(void *state);
static void drawDummyEffect(void *state, short x, short w, short y, short ymin, short ymax);
static struct json_object *serializeDummyEffect(void *state);
static void *deserializeDummyEffect(struct json_object *jso, float **input, float **output);

const EffectAPI dummy_effect_api = {
	"Dummy",
	NULL, /* init_db */
	NULL, /* free_db */
	NULL, /* db_count */
	NULL, /* db_line */
	NULL, /* init */
	NULL, /* free */
	NULL, /* copy */
	NULL, /* run */
	getDummyEffectControlCount,
	getDummyEffectHeight,
	drawDummyEffect,
	serializeDummyEffect,
	deserializeDummyEffect,
};
