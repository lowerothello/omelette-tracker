typedef struct InputAPI
{
	bool          (*init)(void); /* returns true for failure */
	void          (*free)(void);
	void  (*autorepeaton)(void);
	void (*autorepeatoff)(void);
	bool poly;          /* polyphonic input is possible */
} InputAPI;
InputAPI input_api = {0};

#include "stdin.c"
#include "raw.c"
#ifdef OML_X11
#include "x11.c"
#endif

void inputInitAPI(void);

void previewNote(uint8_t note, uint8_t inst, bool release);
void previewRow(Row *r, bool release) { previewNote(r->note, r->inst, release); }

void previewFileNote(uint8_t note, bool release);
int getPreviewVoice(uint8_t note, bool release);

#include "input.c"
