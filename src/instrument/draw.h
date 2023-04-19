/* callback:
 *   should draw .width characters starting from ->x, ->y.
 *   ->iv can be NULL.
 *   ->index will be >= 0 and < entries.
 */
/* TODO: scrolling */
typedef struct {
	uint8_t width;    /* width of each entry */
	uint8_t count;    /* how many entries */
	uint8_t controls; /* how many controls, over all the entries */
	uint8_t padding;  /* padding between columns */
	void  (*callback)(short x, short y, Inst *iv, uint8_t index);
} InstUI;

#define INSTUI_WAVEFORM_MIN 3

short getInstUIRows(const InstUI *iui, short cols);
short getInstUICols(const InstUI *iui, short rows);
short getMaxInstUICols(const InstUI *iui, short width);
void drawInstUI(const InstUI *iui, void *callbackarg, short x, short w, short y, short scrolloffset, short rows);

void drawInstrument(void);

#define EMPTY_INST_UI_TEXT "PRESS 'aa' TO ADD A SAMPLE"
void instUIEmptyCallback(short x, short y, Inst *iv, uint8_t index);
const InstUI emptyInstUI = { 26/*strlen(EMPTY_INST_UI_TEXT)*/, 1, 1, 0, instUIEmptyCallback, };

