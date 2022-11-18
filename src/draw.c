/* undefined behaviour if multibyte chars are printed  */
/* only culls the x axis, culling the y axis is simple */
void printCulling(char *s, short x, short y,
		short minx, short maxx)
{
	if (x < minx) { if (x > minx - strlen(s)) printf("\033[%d;%dH%s", y, minx, s+MIN(minx - x, strlen(s))); }
	else if (x < maxx)                        printf("\033[%d;%dH%.*s", y, x, maxx - x, s);
}
