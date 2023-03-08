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

void initLV2DB(void);
void freeLV2DB(void);


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
	uint8_t             type;
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

void freeLV2Effect(LV2State*);
void initLV2Effect(LV2State*, float **input, float **output, const LilvPlugin *plugin);
void copyLV2Effect(LV2State *dest, LV2State *src, float **input, float **output);

uint32_t getLV2EffectControlCount(LV2State*);
short getLV2EffectHeight(LV2State*);

void drawLV2Effect(LV2State*,
		short x, short w,
		short y, short ymin, short ymax);

void runLV2Effect(uint32_t samplecount, LV2State*, float **input, float **output);
