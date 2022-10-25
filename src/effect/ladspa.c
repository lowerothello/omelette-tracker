#define LADSPA_DEF_MAX 4.0f
#define LADSPA_DEF_MIN 0.0f

#define M_1_OVER_0_9 1.1111111111111112 /* 1/0.9 */

typedef struct
{
	size_t                    descc;
	const LADSPA_Descriptor **descv; /* LADSPA plugin descriptions */
	size_t                    symbolc;
	void                    **symbolv; /* loaded soname symbol tables */
} LadspaDB;

LadspaDB ladspa_db;

void initLadspaDB(void)
{
	const char *dirpath = "/usr/lib/ladspa/";
	/* TODO: should iterate over getenv(LADSPA_PATH) if it is set */
	DIR *dir = opendir(dirpath);
	if (!dir) return; /* TODO: error message */

	struct dirent *ent;

	const LADSPA_Descriptor *(*ldesc)(unsigned long);
	unsigned long i;

	size_t try_symbolc = 0;
	size_t try_descc = 1;
	ladspa_db.symbolc = 0;
	ladspa_db.descc = 0;

	/* get the maximum size symbolv can possibly need to be */
	while ((ent = readdir(dir)))
		if (strlen(ent->d_name) > strlen(".so") && !strcmp(ent->d_name+strlen(ent->d_name) - strlen(".so"), ".so"))
			try_symbolc++;
	/* generously allocate symbolv */
	ladspa_db.symbolv = malloc(try_symbolc * sizeof(void *));

	/* populate symbolv and get descc */
	rewinddir(dir);
	char catpath[PATH_MAX];
	while ((ent = readdir(dir)))
		if (strlen(ent->d_name) > strlen(".so") && !strcmp(ent->d_name+strlen(ent->d_name) - strlen(".so"), ".so"))
		{
			strcpy(catpath, dirpath);
			strcat(catpath, ent->d_name);
			ladspa_db.symbolv[ladspa_db.symbolc] = dlopen(catpath, RTLD_LOCAL | RTLD_LAZY);
			if (ladspa_db.symbolv[ladspa_db.symbolc])
			{
				ldesc = (const LADSPA_Descriptor *(*)(unsigned long))dlsym(ladspa_db.symbolv[ladspa_db.symbolc], "ladspa_descriptor");
				if (ldesc)
				{
					i = 0;
					while (ldesc(i) != NULL) { i++; try_descc++; }
				}
				ladspa_db.symbolc++;
			}
		}
	/* shrink symbolv as much as possible */
	ladspa_db.symbolv = realloc(ladspa_db.symbolv, ladspa_db.symbolc * sizeof(void *));
	/* allocate descv */
	ladspa_db.descv = malloc(try_descc * sizeof(void *));

	/* populate descv */
	for (size_t j = 0; j < ladspa_db.symbolc; j++)
	{
		if ((ldesc = (const LADSPA_Descriptor *(*)(unsigned long))dlsym(ladspa_db.symbolv[j], "ladspa_descriptor")))
		{
			i = 0;
			while ((ladspa_db.descv[ladspa_db.descc] = ldesc(i)))
			{
				i++;
				ladspa_db.descc++;
			}
		}
	}
	/* TODO: this realloc is bad and ugly */
	ladspa_db.descv = realloc(ladspa_db.descv, ladspa_db.descc * sizeof(void *));

	closedir(dir);
}

void freeLadspaDB(void)
{
	for (size_t i = 0; i < ladspa_db.symbolc; i++)
		dlclose(ladspa_db.symbolv[i]);

	free(ladspa_db.symbolv);
	free(ladspa_db.descv);
}

typedef struct
{
	const LADSPA_Descriptor *desc;
	LADSPA_Handle            instance;
	unsigned long            inputc;    /* input audio port count   */
	unsigned long            outputc;   /* output audio port count  */
	unsigned long            controlc;  /* input control port count */
	LADSPA_Data             *controlv;  /* input control ports                 */
	int8_t                  *controla;  /* control automation mapping          */
	uint8_t                 *controlp;  /* control pretty printing pointer     */
	unsigned long            uuid;      /* plugin id, not read from desc cos even if desc is null this needs to be serialized */
} LadspaState;

unsigned long getLadspaEffectControlCount(Effect *e) { return ((LadspaState *)e->state)->controlc<<1; }

void freeLadspaEffect(Effect *e)
{
	LadspaState *s = e->state;

	if (s->desc->deactivate)
		s->desc->deactivate(s->instance);
	s->desc->cleanup(s->instance);
	if (s->controlv) free(s->controlv);
	if (s->controla) free(s->controla);
	if (s->controlp) free(s->controlp);
}

LADSPA_Data prettyPrintingToLadspaData(uint8_t data, LADSPA_PortRangeHint hint)
{
	LADSPA_Data ret = 0.0f;
	if (LADSPA_IS_HINT_TOGGLED(hint.HintDescriptor))
	{
		if (data) return 1.0f;
		else      return 0.0f;
	} else if (LADSPA_IS_HINT_INTEGER(hint.HintDescriptor))
	{
		if (hint.LowerBound < 0.0f) return (int8_t)data;
		else                        return data;
	} else if (LADSPA_IS_HINT_BOUNDED_BELOW(hint.HintDescriptor) && LADSPA_IS_HINT_BOUNDED_ABOVE(hint.HintDescriptor))
	{
		/* offset signed values */
		if (hint.LowerBound < 0.0f) ret = ((short)(int8_t)data + 128)*DIV255;
		else                        ret = data*DIV255;

		// if (LADSPA_IS_HINT_LOGARITHMIC(hint.HintDescriptor)) ret = (-powf(10.0f, -ret) + 1.0f) * M_1_OVER_0_9;
		ret = hint.LowerBound*(1.0f - ret) + hint.UpperBound*ret;
	} else /* assume data shold be between LADSPA_DEF_MIN and LADSPA_DEF_MAX */
	{
		ret = data*DIV255;
		// if (LADSPA_IS_HINT_LOGARITHMIC(hint.HintDescriptor)) ret = (-powf(10.0f, -(ret * 0.9f)) + 1.0f);
		ret = LADSPA_DEF_MIN + ret * LADSPA_DEF_MAX;
	}

	if (LADSPA_IS_HINT_SAMPLE_RATE(hint.HintDescriptor))
		ret *= samplerate;

	return ret;
}
short ladspaDataToPrettyPrinting(LADSPA_Data data, LADSPA_PortRangeHint hint)
{
	LADSPA_Data hold;
	short ret;
	if (LADSPA_IS_HINT_TOGGLED(hint.HintDescriptor)) {
		if (data > 0.0f) return 1;
		else             return 0;
	} else if (LADSPA_IS_HINT_INTEGER(hint.HintDescriptor))
		return data;

	if (LADSPA_IS_HINT_SAMPLE_RATE(hint.HintDescriptor)) data /= samplerate;

	if (LADSPA_IS_HINT_BOUNDED_BELOW(hint.HintDescriptor) && LADSPA_IS_HINT_BOUNDED_ABOVE(hint.HintDescriptor))
	{
		hold = (data - hint.LowerBound) / (hint.UpperBound - hint.LowerBound);
		// if (LADSPA_IS_HINT_LOGARITHMIC(hint.HintDescriptor)) hold = (-powf(10.0f, -hold) + 1.0f) * M_1_OVER_0_9;
		ret = hold * 255.0f;

		/* offset signed values */
		if (hint.LowerBound < 0.0f) ret -= 128;
	} else /* assume data is between LADSPA_DEF_MIN and LADSPA_DEF_MAX */
	{
		hold = (data - LADSPA_DEF_MIN) / (LADSPA_DEF_MAX - LADSPA_DEF_MIN);
		// if (LADSPA_IS_HINT_LOGARITHMIC(hint.HintDescriptor)) hold = (-powf(10.0f, -hold) + 1.0f) * M_1_OVER_0_9;
		ret = hold * 255.0f;
	}

	return ret;
}

/* apply adjustments to the real values */
void _controlpToV(void *arg)
{
	LadspaState *s = arg;
	unsigned long controlp = 0;
	for (unsigned long i = 0; i < s->desc->PortCount; i++)
		if (LADSPA_IS_PORT_CONTROL(s->desc->PortDescriptors[i]) && LADSPA_IS_PORT_INPUT(s->desc->PortDescriptors[i]))
		{
			s->controlv[controlp] = prettyPrintingToLadspaData(s->controlp[controlp], s->desc->PortRangeHints[i]);
			// s->controlp[controlp] = ladspaDataToPrettyPrinting(s->controlv[controlp], s->desc->PortRangeHints[i]);
			controlp++;
		}
}
/* apply real values to the adjustable memory */
void _controlvToP(LadspaState *s)
{
	unsigned long controlp = 0;
	for (unsigned long i = 0; i < s->desc->PortCount; i++)
		if (LADSPA_IS_PORT_CONTROL(s->desc->PortDescriptors[i]) && LADSPA_IS_PORT_INPUT(s->desc->PortDescriptors[i]))
		{
			s->controlp[controlp] = ladspaDataToPrettyPrinting(s->controlv[controlp], s->desc->PortRangeHints[i]);
			controlp++;
		}
}

LADSPA_Data getLadspaPortMin(LADSPA_PortRangeHint hint)
{
	if (!LADSPA_IS_HINT_BOUNDED_BELOW(hint.HintDescriptor)) return LADSPA_DEF_MIN;

	LADSPA_Data ret = hint.LowerBound;

	if (LADSPA_IS_HINT_SAMPLE_RATE(hint.HintDescriptor)) ret *= samplerate;
	if (LADSPA_IS_HINT_INTEGER(hint.HintDescriptor)) ret = (int)ret;

	return ret;
}
LADSPA_Data getLadspaPortMax(LADSPA_PortRangeHint hint)
{
	if (!LADSPA_IS_HINT_BOUNDED_ABOVE(hint.HintDescriptor)) return LADSPA_DEF_MAX;

	LADSPA_Data ret = hint.UpperBound;

	if (LADSPA_IS_HINT_SAMPLE_RATE(hint.HintDescriptor)) ret *= samplerate;
	if (LADSPA_IS_HINT_INTEGER(hint.HintDescriptor)) ret = (int)ret;

	return ret;
}
LADSPA_Data getLadspaPortDef(LADSPA_PortRangeHint hint)
{
	if (!LADSPA_IS_HINT_HAS_DEFAULT(hint.HintDescriptor)) return 0.0f;

	LADSPA_Data ret;

	if (LADSPA_IS_HINT_DEFAULT_0(hint.HintDescriptor)) return 0.0f;
	if (LADSPA_IS_HINT_DEFAULT_1(hint.HintDescriptor)) return 1.0f;

	if (LADSPA_IS_HINT_DEFAULT_MINIMUM(hint.HintDescriptor)) ret = hint.LowerBound;
	if (LADSPA_IS_HINT_DEFAULT_MAXIMUM(hint.HintDescriptor)) ret = hint.UpperBound;

	if (LADSPA_IS_HINT_DEFAULT_LOW(hint.HintDescriptor)) {
		if (LADSPA_IS_HINT_LOGARITHMIC(hint.HintDescriptor)) ret = exp(log(hint.LowerBound)*0.75f + log(hint.UpperBound)*0.25f);
		else                                                 ret = hint.LowerBound*0.75f + hint.UpperBound*0.25f;
	}
	if (LADSPA_IS_HINT_DEFAULT_MIDDLE(hint.HintDescriptor)) {
		if (LADSPA_IS_HINT_LOGARITHMIC(hint.HintDescriptor)) ret = exp(log(hint.LowerBound)*0.5f + log(hint.UpperBound)*0.5f);
		else                                                 ret = hint.LowerBound*0.5f + hint.UpperBound*0.5f;
	}
	if (LADSPA_IS_HINT_DEFAULT_HIGH(hint.HintDescriptor)) {
		if (LADSPA_IS_HINT_LOGARITHMIC(hint.HintDescriptor)) ret = exp(log(hint.LowerBound)*0.25f + log(hint.UpperBound)*0.75f);
		else                                                 ret = hint.LowerBound*0.25f + hint.UpperBound*0.75f;
	}

	if (LADSPA_IS_HINT_DEFAULT_100(hint.HintDescriptor)) ret = 100.0f;
	if (LADSPA_IS_HINT_DEFAULT_440(hint.HintDescriptor)) ret = 440.0f;

	return ret;
}

void _startLadspaEffect(EffectChain *chain, Effect *e)
{
	LadspaState *s = e->state;

	s->uuid = s->desc->UniqueID; /* TODO: maybe set this somewhere else */
	s->instance = s->desc->instantiate(s->desc, samplerate);

	/* iterate first to find the size of the control block */
	s->controlc = 0;
	for (unsigned long i = 0; i < s->desc->PortCount; i++)
		if (LADSPA_IS_PORT_CONTROL(s->desc->PortDescriptors[i]) && LADSPA_IS_PORT_INPUT(s->desc->PortDescriptors[i]))
			s->controlc++;
	/* allocate the control block */
	bool setDefaults = 0; /* only set the defaults if memory isn't yet allocated */
	if (!s->controlv) { s->controlv = calloc(s->controlc, sizeof(LADSPA_Data)); setDefaults = 1; }
	if (!s->controla) { s->controla = calloc(s->controlc, sizeof(int8_t)); memset(s->controla, -1, s->controlc * sizeof(int8_t)); }
	if (!s->controlp)   s->controlp = calloc(s->controlc, sizeof(uint8_t));

	/* iterate again to connect ports */
	unsigned long controlp = 0;
	s->inputc = 0;
	s->outputc = 0;
	for (unsigned long i = 0; i < s->desc->PortCount; i++)
	{
		if (LADSPA_IS_PORT_CONTROL(s->desc->PortDescriptors[i]) && LADSPA_IS_PORT_INPUT(s->desc->PortDescriptors[i]))
		{
			s->desc->connect_port(s->instance, i, &s->controlv[controlp]);
			if (setDefaults) s->controlv[controlp] = getLadspaPortDef(s->desc->PortRangeHints[i]);
			controlp++;
		} else if (LADSPA_IS_PORT_AUDIO(s->desc->PortDescriptors[i]))
		{
			if (LADSPA_IS_PORT_INPUT(s->desc->PortDescriptors[i]))
			{
				if (s->inputc < 2)
				{
					if (chain->input)
						s->desc->connect_port(s->instance, i, chain->input[s->inputc]);

					s->inputc++;
				}
			} else if (LADSPA_IS_PORT_OUTPUT(s->desc->PortDescriptors[i]))
			{
				if (s->outputc < 2)
				{
					if (chain->output)
						s->desc->connect_port(s->instance, i, chain->output[s->outputc]);

					s->outputc++;
				}
			}
		}
	}

	_controlvToP(s);

	if (s->desc->activate)
		s->desc->activate(s->instance);
}

void initLadspaEffect(EffectChain *chain, Effect *e, const LADSPA_Descriptor *desc)
{
	e->state = calloc(1, sizeof(LadspaState));
	((LadspaState *)e->state)->desc = desc;
	_startLadspaEffect(chain, e);
}

void copyLadspaEffect(EffectChain *destchain, Effect *dest, Effect *src)
{
	LadspaState *s_src = src->state;
	initLadspaEffect(destchain, dest, s_src->desc);
	LadspaState *s_dest = dest->state;

	memcpy(s_dest->controlv, s_src->controlv, s_src->controlc * sizeof(LADSPA_Data));
	_controlvToP(s_dest);
}

void serializeLadspaEffect(Effect *e, FILE *fp)
{
	LadspaState *s = e->state;

	fwrite(&s->uuid, sizeof(unsigned long), 1, fp);
	fwrite(&s->controlc, sizeof(unsigned long), 1, fp);
	fwrite(s->controlv, sizeof(LADSPA_Data), s->controlc, fp);
	fwrite(s->controla, sizeof(int8_t), s->controlc, fp);
}
void deserializeLadspaEffect(EffectChain *chain, Effect *e, FILE *fp)
{
	LadspaState *s = e->state;

	fread(&s->uuid, sizeof(unsigned long), 1, fp);
	fread(&s->controlc, sizeof(unsigned long), 1, fp);
	s->controlv = calloc(s->controlc, sizeof(LADSPA_Data));
	fread(s->controlv, sizeof(LADSPA_Data), s->controlc, fp);
	fread(s->controla, sizeof(int8_t), s->controlc, fp);

	/* TODO: loop up desc */
	_startLadspaEffect(chain, e);
}

short getLadspaEffectHeight(Effect *e, short w) { return ((LadspaState *)e->state)->controlc + 3; }

void drawLadspaEffect(Effect *e, ControlState *cc,
		short x, short w,
		short y, short ymin, short ymax)
{
	LadspaState *s = e->state;

	if (ymin <= y && ymax >= y) printf("\033[%d;%dH%.*s", y, MAX(x + 1, x + ((w - (short)strlen(s->desc->Name))>>1)), w - 2, s->desc->Name);

	unsigned long controlp = 0;
	for (unsigned long i = 0; i < s->desc->PortCount; i++)
		if (LADSPA_IS_PORT_CONTROL(s->desc->PortDescriptors[i]) && LADSPA_IS_PORT_INPUT(s->desc->PortDescriptors[i]))
		{
			if (ymin <= y+1 + controlp && ymax >= y+1 + controlp)
			{
				printf("\033[%ld;%dH%.*s", y+1 + controlp, x + 1, w - 20, s->desc->PortNames[i]);
				printf("\033[%ld;%dH%f", y+1 + controlp, x + w - 18, s->controlv[controlp]);

				if (LADSPA_IS_HINT_TOGGLED(s->desc->PortRangeHints[i].HintDescriptor))
				{
					printf("\033[%ld;%dH[ ]", y+1 + controlp, x + w - 7);
					addControl(cc, x + w - 6, y+1 + controlp, &s->controlp[controlp], 0, 0, 1,
							ladspaDataToPrettyPrinting(getLadspaPortDef(s->desc->PortRangeHints[i]), s->desc->PortRangeHints[i]),
							0, _controlpToV, s);
				} else if (LADSPA_IS_HINT_BOUNDED_BELOW(s->desc->PortRangeHints[i].HintDescriptor)
						&& LADSPA_IS_HINT_BOUNDED_ABOVE(s->desc->PortRangeHints[i].HintDescriptor)
						&& s->desc->PortRangeHints[i].LowerBound < 0.0f)
				{ /* signed */
					printf("\033[%ld;%dH[   ]", y+1 + controlp, x + w - 9);
					addControl(cc, x + w - 8, y+1 + controlp, &s->controlp[controlp], 3,
							ladspaDataToPrettyPrinting(s->desc->PortRangeHints[i].LowerBound, s->desc->PortRangeHints[i]),
							ladspaDataToPrettyPrinting(s->desc->PortRangeHints[i].UpperBound, s->desc->PortRangeHints[i]),
							ladspaDataToPrettyPrinting(getLadspaPortDef(s->desc->PortRangeHints[i]), s->desc->PortRangeHints[i]),
							0, _controlpToV, s);
				} else
				{ /* unsigned */
					printf("\033[%ld;%dH[  ]", y+1 + controlp, x + w - 8);
					addControl(cc, x + w - 7, y+1 + controlp, &s->controlp[controlp], 2,
							ladspaDataToPrettyPrinting(getLadspaPortMin(s->desc->PortRangeHints[i]), s->desc->PortRangeHints[i]),
							ladspaDataToPrettyPrinting(getLadspaPortMax(s->desc->PortRangeHints[i]), s->desc->PortRangeHints[i]),
							ladspaDataToPrettyPrinting(getLadspaPortDef(s->desc->PortRangeHints[i]), s->desc->PortRangeHints[i]),
							0, _controlpToV, s);
				}

				printf("\033[%ld;%dH[ ]", y+1 + controlp, x + w - 4);
				addControl(cc, x + w - 3, y+1 + controlp, &s->controla[controlp], 1, -1, 15, -1, 0, NULL, NULL);
			} else
			{
				addControl(cc, 0, 0, NULL, 0, 0, 0, 0, 0, NULL, NULL);
				addControl(cc, 0, 0, NULL, 0, 0, 0, 0, 0, NULL, NULL);
			}

			controlp++;
		}
}

/* only valid to call if e->state->input and e->state->output are not NULL */
void runLadspaEffect(uint32_t samplecount, EffectChain *chain, Effect *e)
{
	LadspaState *s = e->state;

	s->desc->run(s->instance, samplecount);

	if (s->outputc == 1) /* handle mono output correctly */
	{
		memcpy(chain->input[0], chain->output[0], sizeof(float) * samplecount);
		memcpy(chain->input[1], chain->output[0], sizeof(float) * samplecount);
	} else if (s->outputc >= 2)
	{
		memcpy(chain->input[0], chain->output[0], sizeof(float) * samplecount);
		memcpy(chain->input[1], chain->output[0], sizeof(float) * samplecount);
	}
}
