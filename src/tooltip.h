enum Button {
	BUTTON1 = 32,
	BUTTON2 = 33,
	BUTTON3 = 34,
	BUTTON_RELEASE = 35,
	BUTTON1_CTRL = BUTTON1 + 16,
	BUTTON2_CTRL = BUTTON2 + 16,
	BUTTON3_CTRL = BUTTON3 + 16,
	BUTTON_RELEASE_CTRL = BUTTON_RELEASE + 16,

	BUTTON1_HOLD = BUTTON1 + 32,
	BUTTON2_HOLD = BUTTON2 + 32,
	BUTTON3_HOLD = BUTTON3 + 32,
	BUTTON1_HOLD_CTRL = BUTTON1_CTRL + 32,
	BUTTON2_HOLD_CTRL = BUTTON2_CTRL + 32,
	BUTTON3_HOLD_CTRL = BUTTON3_CTRL + 32,

	WHEEL_UP = BUTTON1 + 64,
	WHEEL_DOWN = BUTTON2 + 64,
	WHEEL_UP_CTRL = BUTTON1_CTRL + 64,
	WHEEL_DOWN_CTRL = BUTTON2_CTRL + 64,
};

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

void clearTooltip(TooltipState*);
void setTooltipTitle(TooltipState*, char *prettytitle);
void setTooltipMouseCallback(TooltipState*, void (*callback)(enum Button, int, int));
void addTooltipPrettyPrint(TooltipState*, const char *prettyname, unsigned int state, const char *prettykeysym);
void addTooltipBind(TooltipState*, const char *prettyname, unsigned int state, KeySym keysym, uint8_t flags, void (*callback)(void *), void *arg);
void inputTooltip(TooltipState*, unsigned int state, KeySym input, bool release);
void drawTooltip(TooltipState*);

/* the callback's arg is the note offset cast to (void*) */
void addNotePressBinds(TooltipState*, const char *prettyname, unsigned int state, signed char octave, void (*callback)(void*));
void addNoteReleaseBinds(TooltipState*, const char *prettyname, unsigned int state, signed char octave, void (*callback)(void*));

void addHexBinds(TooltipState*, const char *prettyname, unsigned int state, void (*callback)(void*));
void addDecimalBinds(TooltipState*, const char *prettyname, unsigned int state, void (*callback)(void*));
void addPrintableAsciiBinds(TooltipState*, const char *prettyname, unsigned int state, void (*callback)(void*));

void handleStdin(TooltipState*);
