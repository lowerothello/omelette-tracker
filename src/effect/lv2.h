#include <lilv/lilv.h>
#include <lv2.h>
#include <lv2/urid/urid.h>
#include <lv2/units/units.h>

typedef struct LV2DB
{
	LilvWorld *world;

	/* ports */
	LilvNode *audio_port;
	LilvNode *control_port;
	LilvNode *cv_port;
	LilvNode *input_port;
	LilvNode *output_port;
	LilvNode *toggled;
	LilvNode *integer;
	LilvNode *enumeration;
	LilvNode *sampleRate;
	LilvNode *units_unit;
	LilvNode *units_render;
} LV2DB;
LV2DB lv2_db;

static void initLV2DB(void);
static void freeLV2DB(void);
static uint32_t getLV2DBCount(void);
static EffectBrowserLine getLV2DBLine(uint32_t index);


#define LV2_SUPPORTED_FEATURE_COUNT 2

/* used by both LV2_URID_Map and LV2_URID_Unmap */
struct _UridMapping
{
	LV2_URID size;    /* how many strings are allocated */
	char   **string;  /* string[i + 1] is the index     */
};
LV2_URID lv2_map_uri(LV2_URID_Map_Handle, const char *uri);
const char *lv2_unmap_uri(LV2_URID_Unmap_Handle, LV2_URID urid);


typedef struct LV2State
{
	const LilvPlugin   *plugin;
	LilvInstance       *instance;
	struct _UridMapping urid;
	LV2_URID_Map        urid_map;
	LV2_Feature         urid_map_feature;
	LV2_URID_Unmap      urid_unmap;
	LV2_Feature         urid_unmap_feature;
	const LV2_Feature **features;

	uint32_t          inputc;   /* input audio port count   */
	uint32_t          outputc;  /* output audio port count  */
	uint32_t          controlc; /* input control port count */
	float            *controlv; /* input control ports      */
	float            *dummyport;
} LV2State;

float getLV2PortMin(LV2State*, const LilvPort*, const LilvNode*);
float getLV2PortMax(LV2State*, const LilvPort*, const LilvNode*);
float getLV2PortDef(LV2State*, const LilvPort*, const LilvNode*);

static uint32_t getLV2EffectControlCount(void*);
static short getLV2EffectHeight(void*);
static void *initLV2Effect(const void *data, float **input, float **output);
static void freeLV2Effect(void*);
static void copyLV2Effect(void *dest, void *src, float **input, float **output);
static void drawLV2Effect(void*, short x, short w, short y, short ymin, short ymax);
static void runLV2Effect(void *state, uint32_t samplecount, float **input, float **output);

const EffectAPI lv2_api = {
	"LV2",
	initLV2DB,
	freeLV2DB,
	getLV2DBCount,
	getLV2DBLine,
	initLV2Effect,
	freeLV2Effect,
	copyLV2Effect,
	runLV2Effect,
	getLV2EffectControlCount,
	getLV2EffectHeight,
	drawLV2Effect,
};
