/* callback:
 *   should draw .width characters starting from ->x, ->y.
 *   ->iv can be NULL.
 *   ->index will be >= 0 and < entries.
 */
/* TODO: scrolling */
typedef struct {
	uint8_t width; /* width of each entry */
	uint8_t count;
	void  (*callback)(short x, short y, Instrument *iv, uint8_t index);
} InstUI;

#define INSTUI_SAMPLER_WIDTH 19
#define INSTUI_PADDING 2
#define INSTUI_WAVEFORM_MIN 3
#define INSTUI_MULTISAMPLE_WIDTH 4

short getInstUIRows(InstUI *iui, short cols);
short getInstUICols(InstUI *iui, short rows);
void drawInstUI(InstUI *iui, Instrument *iv, short x, short w, short y, short scrolloffset, short rows);

void drawInstrument(void);

#define INSTUI_CYCLIC_CALLBACK_MAX 13
void instUICyclicCallback(short x, short y, Instrument *iv, uint8_t index);
