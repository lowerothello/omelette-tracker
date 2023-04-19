#define INSTRUMENT_VOID 255
#define INSTRUMENT_MAX 255

typedef enum InstType
{
	INST_TYPE_NULL    = 0,
	INST_TYPE_SAMPLER = 1,
	INST_TYPE_MIDI    = 2,
	INST_TYPE_COUNT
} InstType;
const char *InstTypeString[INST_TYPE_COUNT] =
{
	"null",
	"sampler",
	"midi"
};

typedef struct Inst
{
	InstType type;
	void *state;

	/* runtime */
	uint32_t triggerflash;
} Inst;

typedef struct InstChain
{
	uint8_t c;                 /* instrument count   */
	uint8_t i[INSTRUMENT_MAX]; /* instrument backref */ /* json: index */
	Inst    v[];               /* instrument values  */ /* json: data  */
} InstChain;

typedef struct InstAPI
{
	void*                    (*init)(void);
	void                     (*free)(Inst*);
	void                     (*copy)(Inst *dest, Inst *src); /* dest has already been free'd */
	void             (*getindexinfo)(Inst*, char *buffer); /* buffer is at least 9 bytes long */
	void                     (*draw)(Inst*, short x, short y, short width, short height, short minx, short maxx);
	void                    (*input)(Inst*);
	void                    (*mouse)(Inst*, enum Button button, int x, int y);
	void              (*triggernote)(uint32_t fptr, Inst*, Track*, float oldnote, float note, short inst);
	void                  (*process)(Inst*, Track*, float rp, uint32_t pointer, float note, short *l, short *r);
	void                 (*lookback)(Inst*, Track*, uint16_t *spr);
	struct json_object* (*serialize)(void*, size_t *dataoffset);
	void            (*serializedata)(FILE *fp, void*, size_t *dataoffset);
	void*             (*deserialize)(struct json_object *jso, void *data, double ratemultiplier);
	size_t statesize;
} InstAPI;



const InstAPI *instGetAPI(InstType type);
size_t instGetPlaybackStateSize(void);

/* checks to see if an inst is allocated and safe to use */
bool instSafe(InstChain*, short index);

int copyInst(uint8_t index, Inst *src);

/* frees the contents of an inst */
void popInstIndexPointer(InstChain *chain, uint8_t index);
int delInst(uint8_t index);

/* take a Sample* and reparent it under Inst .iv */
void reparentSample(Inst*, Sample *sample);

void toggleRecording(uint8_t inst, char cue);

/* __ layer of abstraction for initializing instbuffer */
InstChain *_addInst(uint8_t index, InstType algorithm);
int addInst(uint8_t index, InstType algorithm, void (*cb)(Event*), void *cbarg);

int addReparentInst(uint8_t index, InstType algorithm, Sample *buffer);

/* returns -1 if no inst slots are free */
short emptyInst(uint8_t min);

void yankInst(uint8_t index);
void putInst(size_t index);

void instControlCallback(void);

struct json_object *serializeInst(Inst *inst, size_t *dataoffset);
void serializeInstData(FILE *fp, Inst *inst, size_t *dataoffset);
Inst deserializeInst(struct json_object *jso, void *data, double ratemultiplier);
struct json_object *serializeInstChain(InstChain *chain);
void serializeInstChainData(FILE *fp, InstChain *chain);
InstChain *deserializeInstChain(struct json_object *jso, void *data, double ratemultiplier);

#include "input.h"
#include "waveform.h"
#include "draw.h"

#include "sampler/sampler.h"
#include "midi/midi.h"
