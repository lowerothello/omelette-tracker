typedef struct Pattern
{
	uint16_t length;
	Row      row[];
} Pattern;

#define PATTERN_VOID 255
#define PATTERN_ORDER_LENGTH 256
typedef struct PatternChain
{
	uint8_t  macroc; /* TODO: should be addressable from the settings page */
	uint8_t  order[PATTERN_ORDER_LENGTH]; /* pattern playback order        */
	uint8_t  c;                           /* pattern data length           */
	uint8_t  i[PATTERN_VOID];             /* pattern data arrangement      */
	Pattern *v[];                         /* pattern data                  */
} PatternChain;

PatternChain *newPatternChain(void);
void freePatternChain(PatternChain*);
PatternChain *dupPatternChain(PatternChain*);
PatternChain *deepDupPatternChain(PatternChain*);

uint8_t getPatternLength(void); /* TODO: this function is kinda stupid lol, should probably get rid of it */
uint8_t getPatternChainIndex(uint16_t index);
uint8_t getPatternIndex(uint16_t index);
Pattern *getPatternChainPattern(PatternChain*, uint16_t index);

Pattern *dupPattern(Pattern*, uint16_t newlen);
bool addPattern(PatternChain**, uint8_t index);
bool delPattern(PatternChain**, uint8_t index);

Row *getPatternRow(Pattern*, uint8_t index);

/* remove pattern if it's empty, returns true if the pattern was pruned */
bool prunePattern(PatternChain**, uint8_t index);

/* returns the first free pattern slot, or .fallbackindex if there are no free slots */
uint8_t getFreePatternIndex(PatternChain*, uint8_t fallbackindex);
/* duplicates a pattern into a new slot if there's space for it */
uint8_t dupFreePatternIndex(PatternChain*, uint8_t fallbackindex);

/* push a hex digit into the playback order */
void pushPatternOrder(PatternChain**, uint8_t orderindex, char value);
/* set the playback order directly */
void setPatternOrder(PatternChain**, uint8_t orderindex, uint8_t index);

struct json_object *serializePattern(Pattern*);
Pattern *deserializePattern(struct json_object*);
struct json_object *serializePatternChain(PatternChain*);
PatternChain *deserializePatternChain(struct json_object*);
