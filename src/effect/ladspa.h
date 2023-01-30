#define LADSPA_DEF_MAX 4.0f
#define LADSPA_DEF_MIN 0.0f

typedef struct {
	uint32_t                  descc;
	const LADSPA_Descriptor **descv; /* LADSPA plugin descriptions */
	uint32_t                  symbolc;
	void                    **symbolv; /* loaded soname symbol tables */
} LadspaDB;
LadspaDB ladspa_db;

void initLadspaDB(void);
void freeLadspaDB(void);

void *getSpecificLadspaDescriptor(const LADSPA_Descriptor **desc, const char *soname, unsigned long index);
void freeSpecificLadspaDescriptor(void *dl);


typedef struct
{
	const LADSPA_Descriptor *desc;
	LADSPA_Handle            instance;
	uint32_t                 inputc;   /* input audio port count   */
	uint32_t                 outputc;  /* output audio port count  */
	uint32_t                 controlc; /* input control port count */
	LADSPA_Data             *controlv; /* input control ports                 */
	LADSPA_Data             *dummyport;
	unsigned long            uuid;     /* (TODO: use the plugin label instead?) plugin id, not read from desc cos even if desc is null this needs to be serialized */
} LadspaState;

uint32_t getLadspaEffectControlCount(LadspaState*);
short getLadspaEffectHeight(LadspaState*);

void initLadspaEffect(LadspaState**, float **input, float **output, const LADSPA_Descriptor*);
void freeLadspaEffect(LadspaState*);
void copyLadspaEffect(LadspaState **dest, LadspaState *src, float **input, float **output);

void serializeLadspaEffect(LadspaState*, FILE*);
void deserializeLadspaEffect(LadspaState**, float **input, float **output, FILE*);

/* the current text colour will apply to the header but not the contents */
void drawLadspaEffect(LadspaState*, ControlState*,
		short x, short w, short y, short ymin, short ymax);

/* only valid to call if input and output are not NULL */
void runLadspaEffect(uint32_t samplecount, LadspaState*, float **input, float **output);

#include "ladspa.c"
