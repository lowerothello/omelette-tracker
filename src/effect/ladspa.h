#include <ladspa.h>

#define LADSPA_DEF_MAX 4.0f
#define LADSPA_DEF_MIN 0.0f

typedef struct LadspaDB
{
	uint32_t descc;   const LADSPA_Descriptor **descv;   /* LADSPA plugin descriptions  */
	uint32_t symbolc; void                    **symbolv; /* loaded soname symbol tables */
} LadspaDB;
LadspaDB ladspa_db;

static void initLadspaDB(void);
static void freeLadspaDB(void);
static uint32_t getLadspaDBCount(void);
static EffectBrowserLine getLadspaDBLine(uint32_t index);

void *getSpecificLadspaDescriptor(const LADSPA_Descriptor **desc, const char *soname, unsigned long index);
void freeSpecificLadspaDescriptor(void *dl);


typedef struct LadspaState
{
	const LADSPA_Descriptor *desc;
	LADSPA_Handle            instance;
	uint32_t                 inputc;   /* input audio port count   */
	uint32_t                 outputc;  /* output audio port count  */
	uint32_t                 controlc; /* input control port count */
	LADSPA_Data             *controlv; /* input control ports      */
	LADSPA_Data             *dummyport;
} LadspaState;

static uint32_t getLadspaEffectControlCount(void*);
static short getLadspaEffectHeight(void*);
static void *initLadspaEffect(const void *data, float **input, float **output);
static void freeLadspaEffect(void*);
static void copyLadspaEffect(void *dest, void *src, float **input, float **output);
static void drawLadspaEffect(void*, short x, short w, short y, short ymin, short ymax); /* the current text colour will apply to the header but not the contents */
static void runLadspaEffect(void*, uint32_t samplecount, float **input, float **output); /* only valid to call if input and output are not NULL */

const EffectAPI ladspa_api = {
	"Ladspa",
	initLadspaDB,
	freeLadspaDB,
	getLadspaDBCount,
	getLadspaDBLine,
	initLadspaEffect,
	freeLadspaEffect,
	copyLadspaEffect,
	runLadspaEffect,
	getLadspaEffectControlCount,
	getLadspaEffectHeight,
	drawLadspaEffect,
};
