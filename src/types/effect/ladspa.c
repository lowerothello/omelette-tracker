#define LADSPA_DEF_MAX 4.0f
#define LADSPA_DEF_MIN 0.0f

#define M_1_OVER_0_9 1.1111111111111112 /* 1/0.9 */

typedef struct
{
	uint32_t                  descc;
	const LADSPA_Descriptor **descv; /* LADSPA plugin descriptions */
	uint32_t                  symbolc;
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
	/* TODO: this realloc is bad and ugly (and bad) */
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
	uint32_t                 inputc;   /* input audio port count   */
	uint32_t                 outputc;  /* output audio port count  */
	uint32_t                 controlc; /* input control port count */
	LADSPA_Data             *controlv; /* input control ports                 */
	LADSPA_Data             *dummyport;
	unsigned long            uuid;     /* (TODO: use the plugin label instead?) plugin id, not read from desc cos even if desc is null this needs to be serialized */
} LadspaState;

uint32_t getLadspaEffectControlCount(Effect *e) { return ((LadspaState *)e->state)->controlc;     }
short    getLadspaEffectHeight      (Effect *e) { return ((LadspaState *)e->state)->controlc + 3; }

void freeLadspaEffect(Effect *e)
{
	LadspaState *s = e->state;

	if (s->desc->deactivate)
		s->desc->deactivate(s->instance);
	s->desc->cleanup(s->instance);
	if (s->controlv) free(s->controlv);
	if (s->dummyport) free(s->dummyport);
}

LADSPA_Data getLadspaPortMin(LADSPA_PortRangeHint hint)
{
	if (!LADSPA_IS_HINT_BOUNDED_BELOW(hint.HintDescriptor)) return LADSPA_DEF_MIN;

	LADSPA_Data ret = hint.LowerBound;

	if (LADSPA_IS_HINT_SAMPLE_RATE(hint.HintDescriptor)) ret *= samplerate;
	if (LADSPA_IS_HINT_INTEGER    (hint.HintDescriptor)) ret = floorf(ret);

	return ret;
}
LADSPA_Data getLadspaPortMax(LADSPA_PortRangeHint hint)
{
	if (!LADSPA_IS_HINT_BOUNDED_ABOVE(hint.HintDescriptor)) return LADSPA_DEF_MAX;

	LADSPA_Data ret = hint.UpperBound;

	if (LADSPA_IS_HINT_SAMPLE_RATE(hint.HintDescriptor)) ret *= samplerate;
	if (LADSPA_IS_HINT_INTEGER    (hint.HintDescriptor)) ret = floorf(ret);

	return ret;
}
LADSPA_Data getLadspaPortDef(LADSPA_PortRangeHint hint)
{
	if (!LADSPA_IS_HINT_HAS_DEFAULT(hint.HintDescriptor)) return 0.0f;

	LADSPA_Data ret = 0.0f;

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

	if (LADSPA_IS_HINT_SAMPLE_RATE(hint.HintDescriptor)) ret *= samplerate;
	if (LADSPA_IS_HINT_INTEGER    (hint.HintDescriptor)) ret = floorf(ret);

	if (LADSPA_IS_HINT_DEFAULT_100(hint.HintDescriptor)) ret = 100.0f;
	if (LADSPA_IS_HINT_DEFAULT_440(hint.HintDescriptor)) ret = 440.0f;

	return ret;
}

void startLadspaEffect(EffectChain *chain, Effect *e)
{
	LadspaState *s = e->state;

	s->uuid = s->desc->UniqueID; /* TODO: maybe set this somewhere else */
	s->instance = s->desc->instantiate(s->desc, samplerate);

	/* iterate first to find the size of the control block */
	s->controlc = 0;
	for (uint32_t i = 0; i < s->desc->PortCount; i++)
		if (LADSPA_IS_PORT_CONTROL(s->desc->PortDescriptors[i]) && LADSPA_IS_PORT_INPUT(s->desc->PortDescriptors[i]))
			s->controlc++;
	/* allocate the control block */
	bool setDefaults = 0; /* only set the defaults if memory isn't yet allocated */
	if (!s->controlv) { s->controlv = calloc(s->controlc, sizeof(LADSPA_Data)); setDefaults = 1; }

	/* iterate again to connect ports */
	uint32_t controlp = 0;
	s->inputc = 0;
	s->outputc = 0;
	for (uint32_t i = 0; i < s->desc->PortCount; i++)
	{
		if (LADSPA_IS_PORT_CONTROL(s->desc->PortDescriptors[i]))
		{
			if (LADSPA_IS_PORT_INPUT(s->desc->PortDescriptors[i]))
			{
				s->desc->connect_port(s->instance, i, &s->controlv[controlp]);
				if (setDefaults) s->controlv[controlp] = getLadspaPortDef(s->desc->PortRangeHints[i]);
				controlp++;
			} else /* TODO: output controls are currently ignored */
			{
				s->desc->connect_port(s->instance, i, s->dummyport);
			}
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
			} else /* implies output */
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

	if (s->desc->activate)
		s->desc->activate(s->instance);
}

void initLadspaEffect(EffectChain *chain, Effect *e, const LADSPA_Descriptor *desc)
{
	e->type = EFFECT_TYPE_LADSPA;
	e->state = calloc(1, sizeof(LadspaState));
	((LadspaState *)e->state)->desc = desc;
	((LadspaState *)e->state)->dummyport = malloc(sizeof(LADSPA_Data) * samplerate);

	startLadspaEffect(chain, e);
}

void copyLadspaEffect(EffectChain *destchain, Effect *dest, Effect *src)
{
	LadspaState *s_src = src->state;
	initLadspaEffect(destchain, dest, s_src->desc);
	LadspaState *s_dest = dest->state;

	memcpy(s_dest->controlv, s_src->controlv, s_src->controlc * sizeof(LADSPA_Data));
}

void serializeLadspaEffect(Effect *e, FILE *fp)
{
	LadspaState *s = e->state;

	fwrite(&s->uuid, sizeof(uint32_t), 1, fp);
	fwrite(&s->controlc, sizeof(uint32_t), 1, fp);
	fwrite(s->controlv, sizeof(LADSPA_Data), s->controlc, fp);
}
void deserializeLadspaEffect(EffectChain *chain, Effect *e, FILE *fp)
{
	e->state = calloc(1, sizeof(LadspaState));
	LadspaState *s = e->state;

	fread(&s->uuid, sizeof(uint32_t), 1, fp);
	fread(&s->controlc, sizeof(uint32_t), 1, fp);
	s->controlv = calloc(s->controlc, sizeof(LADSPA_Data));
	fread(s->controlv, sizeof(LADSPA_Data), s->controlc, fp);

	/* TODO: look up desc based on uuid */
	startLadspaEffect(chain, e);
}

void drawLadspaEffect(Effect *e, ControlState *cc,
		short x, short w,
		short y, short ymin, short ymax)
{
	LadspaState *s = e->state;

	if (ymin <= y && ymax >= y)
	{
		printf("\033[1m");
		drawCentreText(x+2, y, w-4, s->desc->Name);
		printf("\033[22m");
	}

	uint32_t controlp = 0;
	for (uint32_t i = 0; i < s->desc->PortCount; i++)
		if (LADSPA_IS_PORT_CONTROL(s->desc->PortDescriptors[i]) && LADSPA_IS_PORT_INPUT(s->desc->PortDescriptors[i]))
		{
			drawAutogenPluginLine(cc, x, y+1 + controlp, w, ymin, ymax,
					s->desc->PortNames[i], &s->controlv[controlp],
					LADSPA_IS_HINT_TOGGLED(s->desc->PortRangeHints[i].HintDescriptor),
					LADSPA_IS_HINT_INTEGER(s->desc->PortRangeHints[i].HintDescriptor),
					getLadspaPortMin(s->desc->PortRangeHints[i]),
					getLadspaPortMax(s->desc->PortRangeHints[i]),
					getLadspaPortDef(s->desc->PortRangeHints[i]),
					NULL, NULL, 0, 0);

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
