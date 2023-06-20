typedef struct Command
{
	char    c; /* command        */
	uint8_t v; /* argument       */
	uint8_t t; /* special tokens */
} Command;

#define NOTE_SMOOTH_OFFSET 120 /* offset from any note value to get it's legato variant */
#define NOTE_C_OFFSET 3 /* offset to work based off of C */
enum Note
{
	NOTE_VOID   = 0x00,
	NOTE_MIN    = 0x01, // 1
	NOTE_C5     = 0x40, // 64
	NOTE_MAX    = 0x78, // 120
	NOTE_UNUSED = 0xfd,
	NOTE_CUT    = 0xfe,
	NOTE_OFF    = 0xff,
};
#define INST_VOID 255
/* TODO: COMMAND_VOID */

typedef struct Row
{
	float   note; /* MIDI compatible  | NOTE_* declares */
	uint8_t inst; /* instrument index | INST_* declares */
	Command   command[8];
} Row;

typedef struct Pattern
{
	uint16_t length;
	Row      row[];
} Pattern;

#define PATTERN_VOID 255
typedef struct PatternChain
{
	uint8_t  commandc; /* TODO: should be addressable from the settings page */
	uint8_t  order[PATTERN_VOID]; /* pattern playback order        */
	uint8_t  c;                   /* pattern data length           */
	uint8_t  i[PATTERN_VOID];     /* pattern data arrangement      */
	Pattern *v[];                 /* pattern data                  */
} PatternChain;

PatternChain *newPatternChain(void);
void freePatternChain(PatternChain*);
// PatternChain *dupPatternChain(PatternChain*);
// PatternChain *deepDupPatternChain(PatternChain*);

uint8_t getPatternLength(void); /* TODO: this function is kinda stupid lol, should probably get rid of it */
uint8_t getPatternChainIndex(uint16_t index);
uint8_t getPatternIndex(uint16_t index);
Pattern *getPatternChainPattern(PatternChain*, uint16_t index);

bool addPattern(PatternChain**, uint8_t index);
bool delPattern(PatternChain**, uint8_t index);

Row *getPatternRow(Pattern*, uint8_t index);

/* remove pattern if it's empty, returns true if the pattern was pruned */
bool prunePattern(PatternChain**, uint8_t index);

/* returns the first free pattern slot, or .fallbackindex if there are no free slots */
uint8_t getFreePatternIndex(PatternChain*, uint8_t fallbackindex);

/* push a hex digit into the playback order */
void pushPatternOrder(PatternChain**, uint8_t orderindex, char value);
/* set the playback order directly */
Pattern *_setPatternOrder(PatternChain**, uint8_t index, short value);
void setPatternOrder(PatternChain**, uint8_t index, short value);

/* setPatternOrder, but with immediate access to newchain    */
/* be careful using, has a tendancy to smash the event queue */
PatternChain *setPatternOrderPeek(PatternChain**, uint8_t index, short value);

struct json_object *serializeCommand(Command*);
Command deserializeCommand(struct json_object*);
struct json_object *serializeRow(Row*);
Row deserializeRow(struct json_object*);
struct json_object *serializePattern(Pattern*);
Pattern *deserializePattern(struct json_object*);
struct json_object *serializePatternChain(PatternChain*);
PatternChain *deserializePatternChain(struct json_object*);

void regenSongLength(void);
