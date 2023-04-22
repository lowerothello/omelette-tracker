enum ControlNibbles {
	CONTROL_NIBBLES_BOOL   = 0, /* shows either "X" or " ", click/return to toggle (ignores other input) */
	CONTROL_NIBBLES_PRETTY = 1, /* shows -1 to 15, reads pretty names */
	CONTROL_NIBBLES_UINT8  = 2, /* shows 0 to 0xff */
	CONTROL_NIBBLES_INT8   = 3, /* shows 0 to 0x3f/0x40 and the sign bit */
	CONTROL_NIBBLES_UINT16 = 4, /* shows 0 to 0xffff */
	CONTROL_NIBBLES_INT16  = 5, /* shows 0 to 0x7fff/0x8000 and the sign bit, min and max are nibble-wise, min should be absolute */
	CONTROL_NIBBLES_UINT32 = 8, /* shows 0 to 0xffffffff */
	CONTROL_NIBBLES_UNSIGNED_FLOAT,
	CONTROL_NIBBLES_SIGNED_FLOAT,
	CONTROL_NIBBLES_UNSIGNED_INT,
	CONTROL_NIBBLES_SIGNED_INT,
	CONTROL_NIBBLES_TOGGLED,
};

typedef union {
	float    f;
	uint32_t i;
} ControlRange;

typedef struct {
	ControlRange value;
	char        *label;
} ScalePoint;

typedef struct
{
	short               x, y; /* position on the screen */
	void               *value;
	ControlRange        min, max, def;
	enum ControlNibbles nibbles;
	uint32_t            scalepointlen;
	ScalePoint         *scalepoint;
	uint32_t            scalepointptr;
	uint32_t            scalepointcount;

	void       (*callback)(void *arg); /* called when self->value is changed */
	void        *callbackarg;
} Control;

typedef struct
{
	uint8_t cursor;
	signed char fieldpointer;

	size_t controlc; /* how many controls are assigned */
	Control *control;

	bool mouseadjust;
	int8_t mouseclickwalk;
	bool keyadjust;
	bool resetadjust;
	short prevmousex;
} ControlState;
ControlState cc;


void clearControls(void);
void addControlInt(short x, short y, void *value, int8_t nibbles,
		uint32_t min, uint32_t max, uint32_t def,
		uint32_t scalepointlen, uint32_t scalepointcount,
		void (*callback)(void *), void *callbackarg);
void addControlFloat(short x, short y, void *value, int8_t nibbles,
		float min, float max, float def,
		uint32_t scalepointlen, uint32_t scalepointcount,
		void (*callback)(void *), void *callbackarg);
void addControlString(short x, short y, void *value, short width, uint32_t maxlen,
		void (*callback)(void *), void *callbackarg);
void addControlDummy(short x, short y);

/* applies retroactively to the previously registered control */
void addScalePointInt  (char *label, uint32_t value);
void addScalePointFloat(char *label, uint32_t value);

/* number of digits before the radix, up to 6 are checked for (safe float range) */
/* don't think there's a more efficient way to do this than an if chain? there might well be */
int getPreRadixDigits(float);

/* dump state to the screen */
/* leaves the cursor over the selected control */
void drawControls(void);

void incControlValue(void);
void decControlValue(void);
void incControlCursor(uint8_t count);
void decControlCursor(uint8_t count);
void setControlCursor(uint8_t newcursor);
void incControlFieldpointer(void);
void decControlFieldpointer(void);

void hexControlValue(char value);
void toggleKeyControl(void);
void revertKeyControl(void);
void mouseControls(int button, int x, int y);

#include "control.c"
