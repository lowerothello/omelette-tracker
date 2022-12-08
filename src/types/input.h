enum InputMode {
	INPUT_MODE_NONE,  /* just interpret stdin        */
	INPUT_MODE_RAW,   /* use the raw driver, console */
	INPUT_MODE_X,     /* use an x hack, xterm        */
};

void addCountBinds(TooltipState*, bool draw);
void previewNote(int key, uint8_t inst);
void previewRow(Row*);
void previewFileNote(UI*, int key);

int initRawInput(void);
void freeRawInput(void);

#include "input.c"
