typedef struct Macro
{
	char    c; /* command  */
	uint8_t v; /* argument */
} Macro;

#define NOTE_SMOOTH_OFFSET 120 /* offset from any note value to get it's smooth variant */
#define NOTE_C_OFFSET 3 /* offset to work based off of C */
enum NOTE_MASK {
	NOTE_VOID       = 0x00,
	NOTE_MIN        = 0x01, // 1
	NOTE_C5         = 0x40, // 64
	NOTE_MAX        = 0x78, // 120
	NOTE_UNUSED     = 0xfd,
	NOTE_CUT        = 0xfe,
	NOTE_OFF        = 0xff,
};
#define INST_VOID 255
/* TODO: MACRO_VOID */

typedef struct Row
{
	uint8_t note; /* MIDI compatible  | NOTE_* declares */
	uint8_t inst; /* instrument index | INST_* declares */
	Macro   macro[8];
} Row;

#define C_VTRIG_LOOP (1<<0) /* variant should loop indefinitely */
typedef struct Vtrig
{
	uint8_t index;
	uint8_t flags;
} Vtrig;

#define VARIANT_VOID 255
#define VARIANT_OFF 254
#define VARIANT_ROWMAX 255
typedef struct Variant
{
	uint16_t rowc;
	Row      rowv[];
} Variant;

#define VARIANT_MAX 254
typedef struct VariantChain
{
	uint16_t songlen;        /* main variant length        */
	Vtrig   *trig;           /* variant triggers           */
	Variant *main;           /* main fallback variant      */
	uint8_t  macroc;         /* macro column count         */
	uint8_t  c;              /* variant contents length    */
	uint8_t  i[VARIANT_MAX]; /* variant index/backref      */
	Variant *v[];            /* variant contents           */
} VariantChain; /* enough pattern data for a full track    */


Variant *dupVariant(Variant *oldvariant, uint16_t newlen);
VariantChain *dupVariantChain(VariantChain *vc);
uint8_t dupEmptyVariantIndex(VariantChain *vc, uint8_t fallbackindex);


/* set the length of the main variant */
/* invalidates past getRow() calls    */
void resizeVariantChain(VariantChain *vc, uint16_t newlen);

/* returns true on failure */
int addVariant(VariantChain **vc, uint8_t index, uint8_t length);
int delVariant(VariantChain **vc, uint8_t index);

void freeVariantChain(VariantChain **vc);

/* returns true if the variant is popuplated */
bool variantPopulated(VariantChain *vc, uint8_t index);

/* remove variant if it's empty           */
/* returns true if the variant was pruned */
bool pruneVariant(VariantChain **vc, uint8_t index);

/* get a row out of a variant, returned value should not be freed */
Row *getVariantRow(Variant *v, uint16_t row) { return &v->rowv[row%(v->rowc+1)]; }

/* the effective rowc, how many rows are actually used */
uint16_t getSignificantRowc(VariantChain *vc);

/* returns the last (if any) variant trigger */
/* returns -1 if no vtrig is within range    */
int getVariantChainPrevVtrig(VariantChain *vc, uint16_t index);

uint8_t getEmptyVariantIndex(VariantChain *vc, uint8_t fallbackindex);

void inputVariantChainTrig(VariantChain **vc, uint16_t index, char    value);
void setVariantChainTrig  (VariantChain **vc, uint16_t index, uint8_t value);

/* returns the index within the variant   */
/* writes the variant pointer to **output */
/* **output == NULL is allowed            */
/* returns -1 if not in a variant         */
int getVariantChainVariant      (Variant **output, VariantChain *vc, uint16_t index);
/* like above, but ignore variant looping */
int getVariantChainVariantNoLoop(Variant **output, VariantChain *vc, uint16_t index);
