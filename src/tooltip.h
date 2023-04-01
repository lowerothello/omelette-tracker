#define MOD_WIDTH 9 /* max width for the key name */

enum TT {
	TT_DEAD    = (1<<0), /* skip regenerating the tooltip state */
	TT_DRAW    = (1<<1), /* include in the info popup           */
	TT_RELEASE = (1<<2), /* trigger on key release              */
};

typedef struct {
	char        *prettyname;
	char        *prettykeysym;
	unsigned int state;  /* (XEvent*)->state compatible   */
	KeySym       keysym; /* X11 keysym (ascii compatible) */
	void       (*callback)(void*);
	void        *arg;    /* arg passed to callback */
	uint8_t      flags;
} TooltipEntry;

/* TODO: w->count should be here */
typedef struct {
	short  maxprettynamelen;
	char  *prettytitle;
	void (*mousecallback)(enum Button, int, int);

	uint8_t       entryc;
	TooltipEntry *entryv;
} TooltipState;
TooltipState tt;

/* must be defined elsewhere */
void resetInput(void);

void clearTooltip(void);
void setTooltipTitle(char *prettytitle);
void setTooltipMouseCallback(void (*callback)(enum Button, int, int));
void addTooltipPrettyPrint(const char *prettyname, unsigned int state, const char *prettykeysym);
void addTooltipBind(const char *prettyname, unsigned int state, KeySym keysym, uint8_t flags, void (*callback)(void *), void *arg);

/* procs the first relevant bind */
void inputTooltip(unsigned int state, KeySym input, bool release);

void drawTooltip(void);

/* the callback's arg is the note offset cast to (void*) */
void addNotePressBinds(const char *prettyname, unsigned int state, signed char octave, void (*callback)(void*));
void addNoteReleaseBinds(const char *prettyname, unsigned int state, signed char octave, void (*callback)(void*));

void addHexBinds(const char *prettyname, unsigned int state, void (*callback)(void*));
void addDecimalBinds(const char *prettyname, unsigned int state, void (*callback)(void*));
void addPrintableAsciiBinds(const char *prettyname, unsigned int state, void (*callback)(void*));

void handleStdin(void);
