short tfxToVfx(int8_t tfx);
short vfxToTfx(int8_t vfx);
short tfxToVmo(Track *cv, short tfx);
short vfxToVmo(Track *cv, short vfx);
short vfxVmoMin(short x1, short x2);
short vfxVmoMax(short x1, short x2);

/* returns true if (x >= min && x <= max) in visual command order */
bool vfxVmoRangeIncl(short min, short max, short x);

void yankPartPattern(int8_t x1, int8_t x2, short y1, short y2, uint8_t c1, uint8_t c2);
// void yankPartPatternOrder(short y1, short y2, uint8_t c1, uint8_t c2); /* TODO: */
void putPartPattern(bool step); /* TODO: count */
void mixPutPartPattern(bool step); /* TODO: count */
void delPartPattern(int8_t x1, int8_t x2, short y1, short y2, uint8_t c1, uint8_t c2);
void delPartPatternOrder(short y1, short y2, uint8_t c1, uint8_t c2);
void addPartPattern(signed char value, int8_t x1, int8_t x2, short y1, short y2, uint8_t c1, uint8_t c2, bool noteonly);
void addPartPatternOrder(signed char value, short y1, short y2, uint8_t c1, uint8_t c2);
void tildePartPattern(int8_t x1, int8_t x2, short y1, short y2, uint8_t c1, uint8_t c2);
void interpolatePartPattern(int8_t x1, int8_t x2, short y1, short y2, uint8_t c1, uint8_t c2);
void randPartPattern(int8_t x1, int8_t x2, short y1, short y2, uint8_t c1, uint8_t c2);
void cycleUpPartPattern(uint8_t count, int8_t x1, int8_t x2, short y1, short y2, uint8_t c1, uint8_t c2);
void cycleDownPartPattern(uint8_t count, int8_t x1, int8_t x2, short y1, short y2, uint8_t c1, uint8_t c2);
#define BOUNCE_SCALE 2.0f /* multiply samples by this before saving */
void bouncePartPattern(short y1, short y2, uint8_t c1, uint8_t c2);


#include "visual.c"
