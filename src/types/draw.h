/* undefined behaviour if multibyte chars are printed  */
/* only culls the x axis, culling the y axis is simple */
void printCulling(char *s, short x, short y, short minx, short maxx);
void drawRuler(void);
void redraw(void);
