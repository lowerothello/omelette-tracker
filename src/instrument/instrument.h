#define INSTRUMENT_VOID 255
#define INSTRUMENT_MAX 255

typedef enum InstType
{
	INST_TYPE_NULL    = 0,
	INST_TYPE_SAMPLER = 1,
	INST_TYPE_MIDI    = 2,
	INST_TYPE_COUNT
} InstType;

typedef struct Inst
{
	InstType type;
	void *state;

	uint32_t triggerflash;
} Inst;

typedef struct InstChain
{
	uint8_t c;                 /* instrument count   */
	uint8_t i[INSTRUMENT_MAX]; /* instrument backref */
	Inst    v[];               /* instrument values  */
} InstChain;

typedef struct InstAPI
{
	void         (*init)(Inst*);
	void         (*free)(Inst*);
	void         (*copy)(Inst *dest, Inst *src); /* dest has already been free'd */
	void (*getindexinfo)(Inst*, char *buffer); /* buffer is at least 9 bytes long */
	void         (*draw)(Inst*, short x, short y, short width, short height, short minx, short maxx);
	void        (*input)(Inst*);
	void        (*mouse)(Inst*, enum Button button, int x, int y);
	void  (*triggernote)(Inst*, Track*);
	void      (*process)(Inst*, Track*, float rp, uint32_t pointer, uint32_t pitchedpointer, float finetune, short *l, short *r);
	void     (*lookback)(Inst*, Track*, uint16_t *spr);
	/* playback state (can't use the heap) */
	/* inst-specific macros */
} InstAPI;

const InstAPI *instGetAPI(InstType type);

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

#include "input.h"
#include "waveform.h"
#include "draw.h"

#include "sampler/sampler.h"
