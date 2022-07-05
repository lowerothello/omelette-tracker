typedef struct
{
	void *placeholder;
} backgroundstate;
backgroundstate *b;

void initBackground(void)
{ b = calloc(1, sizeof(backgroundstate)); }

void freeBackground(void)
{ free(b); }

void updateBackground(jack_nframes_t nfptr, portbufferpair out)
{
}

void drawBackground(void)
{
	/* puts("\033[7m");
	for (unsigned short y = 1; y <= ws.ws_row; y++)
		for (unsigned short x = 1; x <= ws.ws_col; x++)
			printf("\033[%d;%dH ", y, x);
	puts("\033[27m"); */
}
