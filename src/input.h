enum InputMode {
	INPUTMODE_NONE,  /* just interpret stdin        */
	INPUTMODE_RAW,   /* use the raw driver, console */
	INPUTMODE_X,     /* use an x hack, xterm        */
};

void addCountBinds(TooltipState*, bool draw);
void previewNote(uint8_t note, uint8_t inst, bool release);
void previewRow(Row*, bool release);
void previewFileNote(uint8_t note, bool release);
int getPreviewVoice(uint8_t note, bool release);
void incControlValueRedraw(void);
void decControlValueRedraw(void);
void toggleKeyControlRedraw(void);
void revertKeyControlRedraw(void);

int initRawInput(void);
void freeRawInput(void);

#include "input.c"
