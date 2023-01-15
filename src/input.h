enum InputMode {
	INPUMODE_NONE,  /* just interpret stdin        */
	INPUMODE_RAW,   /* use the raw driver, console */
	INPUMODE_X,     /* use an x hack, xterm        */
};

void addCountBinds(TooltipState*, bool draw);
void previewNote(int key, uint8_t inst);
void previewRow(Row*);
void previewFileNote(UI*, int key);
void incControlValueRedraw(ControlState*);
void decControlValueRedraw(ControlState*);
void toggleKeyControlRedraw(ControlState*);
void revertKeyControlRedraw(ControlState*);

int initRawInput(void);
void freeRawInput(void);

#include "input.c"
