#define CHANNEL_LINENO_COLS (LINENO_COLS - 3)
#define INST_PEEK_COLS 0

/* generate sfx using dynamic width tracks */
short genSfx(short minx);
/* generate sfx using constant width tracks */
short genConstSfx(short trackw);

/* draw the whole tracker to the screen */
void drawTracker(void);


#include "draw.c"
