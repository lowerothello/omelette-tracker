#define E_E_BANDS 4

#define E_E_GRAPH_ROWS 4
#define E_E_GRAPH_CELLS 34

enum {
	E_E_MODE_PEAK,
	E_E_MODE_LOW,
	E_E_MODE_HIGH,
	E_E_MODE_BAND,
} E_E_MODE;

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
void initEqualizer(void **instance)
{
	*instance = calloc(1, sizeof(EqualizerState));
	for (int i = 0; i < E_E_BANDS; i++)
	{
		((EqualizerState *)*instance)->band[i].frequency = 256/E_E_BANDS * i + 128/E_E_BANDS;
		((EqualizerState *)*instance)->band[i].resonance = 0x7f;
	}
}
void freeEqualizer(void **instance) { free(*instance); *instance = NULL; }

void copyEqualizer(void **dest, void **src)
{
	initEqualizer(dest);
	memcpy(*dest, *src, sizeof(DistortionState));
}

void serializeEqualizer(void **instance, FILE *fp)
{
	fwrite(*instance, sizeof(EqualizerState), 1, fp);
}
void deserializeEqualizer(void **instance, FILE *fp)
{
	initEqualizer(instance);
	fread(*instance, sizeof(EqualizerState), 1, fp);
}

#define E_E_GRAPH_Q_WIDTH_MOD 0.078125f
#define E_E_GRAPH_GAIN_MOD ((E_E_GRAPH_ROWS<<2)*DIV256)
// #define E_E_GRAPH_ENABLE
void drawEqualizer(void **instance, ControlState *cc,
		short x, short w,
		short y, short ymin, short ymax)
{
	EqualizerState *s = *instance;

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

	ColumnState cs; resetColumn(&cs, w);
	for (int i = 0; i < E_E_BANDS; i++)
		addColumn(&cs, 7);

	short xx;
	for (int i = 0; i < E_E_BANDS; i++)
	{
		xx = x + getNextColumnOffset(&cs);

		if (ymin <= y+E_E_GRAPH_ROWS+1 && ymax >= y+E_E_GRAPH_ROWS+1)
		{
			printf("\033[%d;%dH%d: [  ]", y+E_E_GRAPH_ROWS+1, xx, i);
			addControlInt(cc, xx+4, y+E_E_GRAPH_ROWS+1, &s->band[i].frequency, 2, 0x0, 0xff, 0x0, 0, 0, NULL, NULL);
		} else addControlDummy(cc);

		if (ymin <= y+E_E_GRAPH_ROWS+2 && ymax >= y+E_E_GRAPH_ROWS+2)
		{
			printf("\033[%d;%dHM[    ]", y+E_E_GRAPH_ROWS+2, xx);
			addControlInt(cc, xx+2, y+E_E_GRAPH_ROWS+2, &s->band[i].mode, 1, 0, 3, 0, 5, 4, NULL, NULL);
				addScalePointInt(cc, "PEAK", E_E_MODE_PEAK);
				addScalePointInt(cc, " LOW", E_E_MODE_LOW );
				addScalePointInt(cc, "HIGH", E_E_MODE_HIGH);
				addScalePointInt(cc, "BAND", E_E_MODE_BAND);
		} else addControlDummy(cc);

		if (ymin <= y+E_E_GRAPH_ROWS+3 && ymax >= y+E_E_GRAPH_ROWS+3)
		{
			printf("\033[%d;%dHG:[   ]", y+E_E_GRAPH_ROWS+3, xx);
			addControlInt(cc, xx+3, y+E_E_GRAPH_ROWS+3, &s->band[i].gain, 3, -128, 127, 0, 0, 0, NULL, NULL);
		} else addControlDummy(cc);

		if (ymin <= y+E_E_GRAPH_ROWS+4 && ymax >= y+E_E_GRAPH_ROWS+4)
		{
			printf("\033[%d;%dHQ: [  ]", y+E_E_GRAPH_ROWS+4, xx);
			addControlInt(cc, xx+4, y+E_E_GRAPH_ROWS+4, &s->band[i].resonance, 2, 0x0, 0xff, 0x0, 0, 0, NULL, NULL);
		} else addControlDummy(cc);
	}
}

#define E_E_GAIN_SCALE 8.0f
void runEqualizer(uint32_t samplecount, EffectChain *chain, void **instance)
{
	EqualizerState *s = *instance;
	float gain;

	for (uint32_t fptr = 0; fptr < samplecount; fptr++)
		for (int i = 0; i < E_E_BANDS; i++)
		{
			runSVFilter(&s->band[i].filter[0], chain->input[0][fptr], s->band[i].frequency*DIV256, s->band[i].resonance*DIV256);
			runSVFilter(&s->band[i].filter[1], chain->input[1][fptr], s->band[i].frequency*DIV256, s->band[i].resonance*DIV256);

			if (s->band[i].gain == -128) gain = -1.0f; /* fully cancel out bands */
			else                         gain = powf(E_E_GAIN_SCALE, s->band[i].gain*DIV128) - 1.0f;
			switch (s->band[i].mode)
			{
				case E_E_MODE_PEAK: chain->input[0][fptr] += s->band[i].filter[0].b * gain; chain->input[1][fptr] += s->band[i].filter[1].b * gain; break;
				case E_E_MODE_LOW:  chain->input[0][fptr] += s->band[i].filter[0].l * gain; chain->input[1][fptr] += s->band[i].filter[1].l * gain; break;
				case E_E_MODE_HIGH: chain->input[0][fptr] += s->band[i].filter[0].h * gain; chain->input[1][fptr] += s->band[i].filter[1].h * gain; break;
				case E_E_MODE_BAND: chain->input[0][fptr]  = s->band[i].filter[0].b;        chain->input[1][fptr]  = s->band[i].filter[1].b;        break;
			}
		}
}

NATIVE_Descriptor *equalizerDescriptor(void)
{
	NATIVE_Descriptor *ret = malloc(sizeof(NATIVE_Descriptor));

	ret->controlc = 4 * E_E_BANDS;
	ret->height   = E_E_GRAPH_ROWS + 7;

	const char name[] = "omuQ";
	const char author[] = "lib";
	ret->name = malloc((strlen(name)+1) * sizeof(char)); strcpy(ret->name, name);
	ret->author = malloc((strlen(author)+1) * sizeof(char)); strcpy(ret->author, author);

	ret->init = initEqualizer;
	ret->free = freeEqualizer;
	ret->copy = copyEqualizer;
	ret->serialize = serializeEqualizer;
	ret->deserialize = deserializeEqualizer;
	ret->draw = drawEqualizer;
	ret->run = runEqualizer;

	return ret;
}
