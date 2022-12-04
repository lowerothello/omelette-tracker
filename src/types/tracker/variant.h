typedef struct {
	char    c; /* command  */
	uint8_t v; /* argument */
	bool    alt; /* true to use the altername command */
} Macro;
#define NOTE_VOID 255
#define NOTE_OFF 254
#define NOTE_UNUSED 253 /* explicitly unused */
#define NOTE_C5 60      /* centre note */
#define NOTE_A10 120    /* first out of range note */
#define INST_VOID 255
#define INST_FILEPREVIEW -1 /* signed, be careful with this */
/* TODO: MACRO_VOID */

typedef struct {
	uint8_t note; /* MIDI compatible  | NOTE_* declares */
	uint8_t inst; /* instrument index | INST_* declares */
	Macro   macro[8];
} Row;

#define C_VTRIG_LOOP (1<<0)
typedef struct {
	uint8_t index;
	uint8_t flags;
} Vtrig;

#define VARIANT_VOID 255
#define VARIANT_OFF 254
#define VARIANT_ROWMAX 255
typedef struct {
	uint16_t rowc;
	Row      rowv[];
} Variant;

#define VARIANT_MAX 254
typedef struct {
	Vtrig   *trig;           /* variant triggers        */
	Variant *main;           /* main fallback variant   */
	uint8_t  macroc;         /* macro column count      */
	uint8_t  notec;          /* note column count       */
	uint8_t  c;              /* variant contents length */
	uint8_t  i[VARIANT_MAX]; /* variant index/backref   */
	Variant *v[];            /* variant contents        */
} VariantChain; /* enough pattern data for a full track */


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

void serializeVariant(Variant *v, FILE *fp);
void deserializeVariant(Variant **v, FILE *fp);
