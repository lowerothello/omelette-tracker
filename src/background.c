typedef struct
{
	sample_t *samples;
	size_t    pointer;
	size_t    width, height;
	Canvas   *canvas;
	char    **buffer;
} backgroundstate;
backgroundstate *b;

void initBackground(void)
{
	b = calloc(1, sizeof(backgroundstate));
}

void resizeBackground(void)
{
	b->width = ws.ws_col * 2;
	b->height = (ws.ws_row - 2) * 4;

	if (b->samples) { free(b->samples); b->samples = NULL; }
	b->samples = calloc(b->width, sizeof(sample_t));

	if (b->canvas) { free_canvas(b->canvas); b->canvas = NULL; }
	if (b->buffer) { free_buffer(b->buffer); b->buffer = NULL; }
	b->canvas = new_canvas(b->width, b->height);
	b->buffer = new_buffer(b->canvas);

	b->pointer = 0;
}

void freeBackground(void)
{
	if (b->samples) { free(b->samples); b->samples = NULL; }
	if (b->canvas) { free_canvas(b->canvas); b->canvas = NULL; }
	if (b->buffer) { free_buffer(b->buffer); b->buffer = NULL; }
	free(b);
}

void updateBackground(jack_nframes_t nfptr, portbufferpair out)
{
	if (b->samples) /* protect against freak segfault */
		for (jack_nframes_t i = 0; i < nfptr; i++)
		{
			b->samples[b->pointer] = out.l[i];
			b->pointer++;
			b->pointer = b->pointer%b->width;
		}
}

void drawBackground(void)
{
	if (b->buffer)
	{
		fill(b->canvas, 0);
		size_t halfheight = b->height / 2;
		for (size_t i = 0; i < b->width; i++)
		{
			set_pixel(b->canvas, 1, i,
					b->samples[(i+b->pointer) % b->width] * halfheight + halfheight);
		}

		draw(b->canvas, b->buffer);
		printf("\033[2;0H");
		for (size_t i = 0; b->buffer[i] != NULL; i++)
			if (b->buffer[i]) printf("%s\n", b->buffer[i]);
	}

	/* puts("\033[7m");
	for (unsigned short y = 1; y <= ws.ws_row; y++)
		for (unsigned short x = 1; x <= ws.ws_col; x++)
			printf("\033[%d;%dH ", y, x);
	puts("\033[27m"); */
}
