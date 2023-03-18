/* callback:
 *   should draw w characters starting from x, y.
 *   iv can be null if a null instrument is selected.
 *   index will be >= 0 and < entries.
 */
/* TODO: scrolling */
typedef struct {
	uint8_t count; /* how many entries there are */
	void  (*callback)(short x, short y, Instrument *iv, uint8_t index);
} InstUIBlock;

enum InstUIFlags {
	INSTUI_DRAWWAVEFORM = (1<<0),
};
typedef struct {
	uint8_t     flags;  /* (enum InstUIFlags) values or'd together */
	uint8_t     width;  /* width of each entry (common amongst blocks) */
	uint8_t     blocks; /* how many blocks there are */
	InstUIBlock block[]; /* TODO: remove the concept of blocks */
} InstUI;

#define INSTUI_SAMPLER_WIDTH 19
#define INSTUI_PADDING 2
#define INSTUI_WAVEFORM_MIN 3
#define INSTUI_MULTISAMPLE_WIDTH 4

InstUI *allocInstUI(uint8_t blocks);
size_t getInstUIEntryc(InstUI *iui);
short getInstUIRows(InstUI *iui, short cols);
short getInstUICols(InstUI *iui, short rows);
void drawInstUI(InstUI *iui, Instrument *iv, short x, short w, short y, short scrolloffset, short rows);

void drawInstrument(void);

void initInstUICommonSamplerBlock  (InstUIBlock *block);
void initInstUIRangeSamplerBlock   (InstUIBlock *block);
void initInstUIGranularSamplerBlock(InstUIBlock *block);
void initInstUIPitchSamplerBlock   (InstUIBlock *block);
