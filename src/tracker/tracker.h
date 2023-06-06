#define TRACK_LINENO_COLS 2

#include "variant.h"
#include "pattern.h"
#include "track.h"

uint8_t changeNoteOctave(uint8_t octave, uint8_t note);
void trackerUpArrow   (size_t count);
void trackerDownArrow (size_t count);
void trackerLeftArrow (size_t count);
void trackerRightArrow(size_t count);
void trackerHome(void);
void trackerEnd (void);
void cycleUp  (void);
void cycleDown(void);
void shiftUp  (void);
void shiftDown(void);
void trackerPgUp(void);
void trackerPgDn(void);

/* generate sfx using dynamic width tracks */
short genSfx(short minx);
/* generate sfx using constant width tracks */
short genConstSfx(short trackw);

void drawTracker(void);
void initTrackerInput(void);
