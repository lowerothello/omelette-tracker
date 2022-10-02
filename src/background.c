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

void resizeBackground(backgroundstate *cb)
{
	cb->width = ws.ws_col * 2;
	cb->height = (ws.ws_row - 2) * 4;

	if (cb->samples) { free(cb->samples); cb->samples = NULL; }
	cb->samples = calloc(cb->width, sizeof(sample_t));

	if (cb->canvas) { free_canvas(cb->canvas); cb->canvas = NULL; }
	if (cb->buffer) { free_buffer(cb->buffer); cb->buffer = NULL; }

	if (cb->width > 0 && cb->height > 0)
		cb->canvas = new_canvas(cb->width, cb->height);

	if (cb->canvas)
		cb->buffer = new_buffer(cb->canvas);

	cb->pointer = 0;
}

void freeBackground(backgroundstate *cb)
{
	if (cb->samples) { free(cb->samples); cb->samples = NULL; }
	if (cb->canvas) { free_canvas(cb->canvas); cb->canvas = NULL; }
	if (cb->buffer) { free_buffer(cb->buffer); cb->buffer = NULL; }
	free(cb);
}

void updateBackground(jack_nframes_t nfptr, sample_t *outl, sample_t *outr)
{
	if (b->samples) /* segfault protection, TODO: shouldn't be needed */
		for (jack_nframes_t i = 0; i < nfptr; i++)
		{
			b->samples[b->pointer] = (outl[i] + outr[i])*0.5f;
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
			/* TODO: use set_pixel_unsafe */
			set_pixel(b->canvas, 1, i,
					b->samples[(i+b->pointer) % b->width] * halfheight + halfheight);

		draw(b->canvas, b->buffer);
		printf("\033[2;0H");
		for (size_t i = 0; b->buffer[i] != NULL; i++)
			if (b->buffer[i]) printf("%s\n", b->buffer[i]);
	}
}
