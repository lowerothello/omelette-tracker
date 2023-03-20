/* callback:
 *   should draw .width characters starting from ->x, ->y.
 *   ->iv can be NULL.
 *   ->index will be >= 0 and < entries.
 */
/* TODO: scrolling */
typedef struct {
	uint8_t width; /* width of each entry */
	uint8_t count;
	uint8_t padding;
	void  (*callback)(short x, short y, Instrument *iv, uint8_t index);
} InstUI;

#define INSTUI_WAVEFORM_MIN 3
#define INSTUI_MULTISAMPLE_WIDTH 4

short getInstUIRows(const InstUI *iui, short cols);
short getInstUICols(const InstUI *iui, short rows);
short getMaxInstUICols(const InstUI *iui, short width);
void drawInstUI(const InstUI *iui, Instrument *iv, short x, short w, short y, short scrolloffset, short rows);

void drawInstrument(void);

#define EMPTY_INST_UI_TEXT "PRESS 'aa' TO ADD A SAMPLE"
void instUIEmptyCallback(short x, short y, Instrument *iv, uint8_t index);
const InstUI emptyInstUI = { 26/*strlen(EMPTY_INST_UI_TEXT)*/, 1, 0, instUIEmptyCallback, };

void instUICyclicCallback(short x, short y, Instrument *iv, uint8_t index);
const InstUI cyclicInstUI = { 19, 13, 2, instUICyclicCallback, };

static void instUIMidiCallback(short x, short y, Instrument *iv, uint8_t index);
const InstUI midiInstUI = { 18, 2, 2, instUIMidiCallback, };

void instUISampleCallback(short x, short y, Instrument *iv, uint8_t index);
const InstUI sampleInstUI = { 17, 6, 3, instUISampleCallback, };

