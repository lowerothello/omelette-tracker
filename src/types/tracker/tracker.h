#define TRACK_TRIG_PAD 3 /* space between the vtrigcol and the notecol */
#define TRACK_LINENO_COLS (LINENO_COLS - 3)
#define INST_PEEK_COLS 0 /* TODO: flesh out instrument index peeking or scrap it */

#include "variant.h"
#include "track.h"

uint8_t changeNoteOctave(uint8_t octave, uint8_t note);
void trackerUpArrow   (size_t count);
void trackerDownArrow (size_t count);
void trackerLeftArrow (size_t count);
void trackerRightArrow(size_t count);
void trackLeft (size_t count);
void trackRight(size_t count);
void trackerHome(void);
void trackerEnd (void);
void cycleUp  (size_t count); /* TODO: count */
void cycleDown(size_t count); /* TODO: count */
void shiftUp  (size_t count);
void shiftDown(size_t count);
void trackerPgUp(size_t count);
void trackerPgDn(size_t count);

/* generate sfx using dynamic width tracks */
short genSfx(short minx);
/* generate sfx using constant width tracks */
short genConstSfx(short trackw);

void drawTracker(void);
void initTrackerInput(TooltipState *tt);
