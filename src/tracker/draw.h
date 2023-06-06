/* will populate buffer, buffer should be at least length 4 */
/* uses a short for note for safe arithmatic */
static void noteToString(short note, char *buffer);

short getTrackWidth(Track*);

/* generate sfx using dynamic width tracks */
short genSfx(short viewportwidth);

/* generate sfx using constant width tracks */
short genConstSfx(short trackw, short viewportwidth);

void drawTracker(void);
