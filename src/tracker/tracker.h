#define TRACK_LINENO_COLS 2

#include "pattern.h"
#include "track.h"

uint8_t changeNoteOctave(uint8_t octave, uint8_t note);
void trackerUpArrow   (size_t count);
void trackerDownArrow (size_t count);
void trackerLeftArrow (size_t count);
void trackerRightArrow(size_t count);
void trackSet(uint8_t track);
void trackLeft (void);
void trackRight(void);
void trackerHome(void);
void trackerEnd (void);
void cycleUp  (void);
void cycleDown(void);
void shiftUp  (void);
void shiftDown(void);
void trackerPgUp(void);
void trackerPgDn(void);

void initTrackerInput(void);
#include "draw.h"
