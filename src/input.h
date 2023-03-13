enum InputMode {
	INPUTMODE_NONE,  /* just interpret stdin        */
	INPUTMODE_RAW,   /* use the raw driver, console */

#ifdef OML_X11
	INPUTMODE_X,     /* use an x hack, xterm        */
#endif
};

void addCountBinds(bool draw);
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
