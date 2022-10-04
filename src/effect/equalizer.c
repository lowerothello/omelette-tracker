#define E_E_BANDS 4

#define E_E_GRAPH_ROWS 4
#define E_E_GRAPH_CELLS 34
const uint8_t equalizerControlCount = 4 * E_E_BANDS;

#define E_E_MODE_PEAK 0
#define E_E_MODE_LOW  1
#define E_E_MODE_HIGH 2
#define E_E_MODE_BAND 3
typedef struct
{
	struct
	{
		uint8_t  frequency;
		int8_t   mode;
		int8_t   gain;
		uint8_t  resonance;
		SVFilter filter[2];
	} band[E_E_BANDS];
} EqualizerState;

	/* for the time being this is hardcoded to 34cols wide */
	// ((EqualizerState *)e->state)->canvas = new_canvas(34<<1, 4<<2);
void initEqualizer(Effect *e)
{
	e->state = calloc(1, sizeof(EqualizerState));
	for (int i = 0; i < E_E_BANDS; i++)
	{
		((EqualizerState *)e->state)->band[i].frequency = 256/E_E_BANDS * i + 128/E_E_BANDS;
		((EqualizerState *)e->state)->band[i].resonance = 0x7f;
	}
}
void copyEqualizer(Effect *dest, Effect *src) { memcpy(dest->state, src->state, sizeof(DistortionState)); }

void serializeEqualizer  (Effect *e, FILE *fp) { fwrite(e->state, sizeof(EqualizerState), 1, fp); }
void deserializeEqualizer(Effect *e, FILE *fp) { fread (e->state, sizeof(EqualizerState), 1, fp); }

short getEqualizerHeight(Effect *e, short w) { return E_E_GRAPH_ROWS + 7; }

#define E_E_GRAPH_Q_WIDTH_MOD 0.078125f
#define E_E_GRAPH_GAIN_MOD ((E_E_GRAPH_ROWS<<2)*DIV256)
// #define E_E_GRAPH_ENABLE
void drawEqualizer(Effect *e, ControlState *cc,
		short x, short w,
		short y, short ymin, short ymax)
{
	EqualizerState *s = (EqualizerState *)e->state;

	const char *text = "# EQUALIZER #";
	if (ymin <= y+0 && ymax >= y+0) printf("\033[%d;%dH\033[1m%s\033[22m", y+0, x + ((w-(short)strlen(text))>>1), text);

#ifdef E_E_GRAPH_ENABLE
	Canvas *canvas = new_canvas(E_E_GRAPH_CELLS<<1, E_E_GRAPH_ROWS<<2);
	char  **buffer = new_buffer(canvas);
	int centrex, width, j;
	float offset;
	for (int i = 0; i < E_E_BANDS; i++)
	{
		centrex = s->band[i].frequency*DIV256 * (E_E_GRAPH_CELLS<<1);
		width = (255 - s->band[i].resonance)*E_E_GRAPH_Q_WIDTH_MOD;

		for (j = 0; j < E_E_GRAPH_CELLS<<1; j++)
			set_pixel(canvas, 1, j, E_E_GRAPH_ROWS<<1);
		switch (s->band[i].mode)
		{
			case E_E_MODE_PEAK:
				for (j = 0; j <= width; j++)
				{
					offset = (thirddegreepolynomial((float)j / (float)width * 2.0f - 1.0f) + 1.0f) / 2.0f;
					set_pixel(canvas, 1, centrex - width + j, MIN((E_E_GRAPH_ROWS<<2)-1, (E_E_GRAPH_ROWS<<1) - s->band[i].gain*E_E_GRAPH_GAIN_MOD*offset));
					set_pixel(canvas, 1, centrex + width - j, MIN((E_E_GRAPH_ROWS<<2)-1, (E_E_GRAPH_ROWS<<1) - s->band[i].gain*E_E_GRAPH_GAIN_MOD*offset));
				}
				set_pixel(canvas, 1, centrex, MIN((E_E_GRAPH_ROWS<<2)-1, (E_E_GRAPH_ROWS<<1) - s->band[i].gain*E_E_GRAPH_GAIN_MOD));
				break;
			case E_E_MODE_LOW:
				offset = fabsf((s->band[i].gain>>1)*E_E_GRAPH_GAIN_MOD);
				for (j = 0; j < centrex - offset - width; j++)
					set_pixel(canvas, 1, j, (E_E_GRAPH_ROWS<<1) - (s->band[i].gain>>1)*E_E_GRAPH_GAIN_MOD);
				for (j = 0; j <= width>>1; j++)
				{
					offset = (thirddegreepolynomial((float)j / (float)(width>>1) * 2.0f - 1.0f) + 1.0f) / 2.0f;
					set_pixel(canvas, 1, centrex - offset - 2 - width + j, MIN((E_E_GRAPH_ROWS<<2)-1, (E_E_GRAPH_ROWS<<1) - (s->band[i].gain>>1)*E_E_GRAPH_GAIN_MOD - (s->band[i].resonance>>2)*E_E_GRAPH_GAIN_MOD*offset));
					set_pixel(canvas, 1, centrex - offset - 2 - j,         MIN((E_E_GRAPH_ROWS<<2)-1, (E_E_GRAPH_ROWS<<1) - (s->band[i].gain>>1)*E_E_GRAPH_GAIN_MOD - (s->band[i].resonance>>2)*E_E_GRAPH_GAIN_MOD*offset));
				}

				if (s->band[i].gain > 0)
				{
					for (j = 0; j < offset; j++)
						if (centrex - j >= 0)
							set_pixel(canvas, 1, centrex - j, (E_E_GRAPH_ROWS<<1) - j);
				} else
				{
					for (j = 0; j < offset; j++)
						if (centrex - j >= 0)
							set_pixel(canvas, 1, centrex - j, (E_E_GRAPH_ROWS<<1) + j);
				}
				break;
			case E_E_MODE_HIGH:
				break;
			case E_E_MODE_BAND:
				break;
		}
	}

	draw(canvas, buffer);

	for (size_t i = 0; buffer[i] != NULL; i++)
		printf("\033[%ld;%dH%s", y + 1 + i, x + ((w - E_E_GRAPH_CELLS)>>1), buffer[i]);

	free_canvas(canvas);
	free_buffer(buffer);
#endif

	short xx;
	ColumnState cs; resetColumn(&cs, w);
	for (int i = 0; i < E_E_BANDS; i++)
		addColumn(&cs, 7);
	for (int i = 0; i < E_E_BANDS; i++)
	{
		xx = x + getNextColumnOffset(&cs);

		if (ymin <= y+E_E_GRAPH_ROWS+1 && ymax >= y+E_E_GRAPH_ROWS+1)
		{
			printf("\033[%d;%dH%d: [  ]", y+E_E_GRAPH_ROWS+1, xx, i);
			addControl(cc, xx+4, y+E_E_GRAPH_ROWS+1, &s->band[i].frequency, 2, 0x0, 0xff, 0);
		} else addControl(cc, 0, 0, NULL, 0, 0, 0, 0);

		if (ymin <= y+E_E_GRAPH_ROWS+2 && ymax >= y+E_E_GRAPH_ROWS+2)
		{
			printf("\033[%d;%dHM[    ]", y+E_E_GRAPH_ROWS+2, xx);
			addControl(cc, xx+2, y+E_E_GRAPH_ROWS+2, &s->band[i].mode, 1, 0, 3, 5);
				setControlPrettyName(cc, "PEAK");
				setControlPrettyName(cc, " LOW");
				setControlPrettyName(cc, "HIGH");
				setControlPrettyName(cc, "BAND");
		} else addControl(cc, 0, 0, NULL, 0, 0, 0, 0);

		if (ymin <= y+E_E_GRAPH_ROWS+3 && ymax >= y+E_E_GRAPH_ROWS+3)
		{
			printf("\033[%d;%dHG:[   ]", y+E_E_GRAPH_ROWS+3, xx);
			addControl(cc, xx+3, y+E_E_GRAPH_ROWS+3, &s->band[i].gain, 3, 0, 0, 0);
		} else addControl(cc, 0, 0, NULL, 0, 0, 0, 0);

		if (ymin <= y+E_E_GRAPH_ROWS+4 && ymax >= y+E_E_GRAPH_ROWS+4)
		{
			printf("\033[%d;%dHQ: [  ]", y+E_E_GRAPH_ROWS+4, xx);
			addControl(cc, xx+4, y+E_E_GRAPH_ROWS+4, &s->band[i].resonance, 2, 0x0, 0xff, 0);
		} else addControl(cc, 0, 0, NULL, 0, 0, 0, 0);
	}
}

#define E_E_GAIN_SCALE 8.0f
void stepEqualizer(Effect *e, float *l, float *r)
{
	EqualizerState *s = (EqualizerState *)e->state;

	float gain;
	for (int i = 0; i < E_E_BANDS; i++)
	{
		runSVFilter(&s->band[i].filter[0], *l, s->band[i].frequency*DIV256, s->band[i].resonance*DIV256);
		runSVFilter(&s->band[i].filter[1], *r, s->band[i].frequency*DIV256, s->band[i].resonance*DIV256);

		if (s->band[i].gain == -128) gain = -1.0f; /* fully cancel out bands */
		else                         gain = powf(E_E_GAIN_SCALE, s->band[i].gain*DIV128) - 1.0f;
		switch (s->band[i].mode)
		{
			case E_E_MODE_PEAK: *l += s->band[i].filter[0].b * gain; *r += s->band[i].filter[1].b * gain; break;
			case E_E_MODE_LOW:  *l += s->band[i].filter[0].l * gain; *r += s->band[i].filter[1].l * gain; break;
			case E_E_MODE_HIGH: *l += s->band[i].filter[0].h * gain; *r += s->band[i].filter[1].h * gain; break;
			case E_E_MODE_BAND: *l  = s->band[i].filter[0].b;        *r  = s->band[i].filter[1].b;        break;
		}
	}
}
