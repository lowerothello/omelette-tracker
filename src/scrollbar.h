/* draw a vertical scrollbar using boxdrawing glyphs */
#define VERT_SCROLL_HANDLE_SIZE 2
static void drawVerticalScrollbar(short x, short y, unsigned short h, uint32_t max, uint32_t index)
{
	uint32_t visualmax = (h - VERT_SCROLL_HANDLE_SIZE)<<3; /* 8x the precision */
	uint32_t visualindex = index * (float)visualmax / (float)max;
	uint32_t drawindex = visualindex / 8;
	uint8_t drawcelloffset = visualindex % 8;

	char buffer[4] = {0xe2, 0x96, 0x88 - drawcelloffset, 0x00};

	// printf("\033[%d;%dH%d", y, x + 2, drawindex);
	printf("\033[%d;%dH%s\033[7m", y + drawindex, x, buffer);
	for (int i = 1; i < VERT_SCROLL_HANDLE_SIZE; i++)
		printf("\033[%d;%dH ", y + drawindex + i, x);
	printf("\033[%d;%dH%s\033[27m", y + drawindex + VERT_SCROLL_HANDLE_SIZE, x, buffer);
}
