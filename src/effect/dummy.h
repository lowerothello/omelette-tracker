#define DUMMY_EFFECT_HEIGHT 3
#define DUMMY_EFFECT_TEXT "DUMMY EFFECT"

uint32_t getDummyEffectControlCount(void *state);
short getDummyEffectHeight(void *state);
void drawDummyEffect(void *state, short x, short w, short y, short ymin, short ymax);

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
};
