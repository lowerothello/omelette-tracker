/* the returned value should be free'd with dlclose() */
void *getSpecificLadspaDescriptor(const LADSPA_Descriptor **desc, const char *soname, unsigned long index)
{
	void *dl;
	const LADSPA_Descriptor *(*ldesc)(unsigned long);

	if ((dl = dlopen(soname, RTLD_LOCAL | RTLD_LAZY)))
		if ((ldesc = (const LADSPA_Descriptor *(*)(unsigned long))dlsym(dl, "ladspa_descriptor")))
			*desc = ldesc(index);
	return dl;
}

static void initLadspaDB(void)
{
	const char *dirpath = "/usr/lib/ladspa/"; /* TODO: should iterate over getenv(LADSPA_PATH) if it is set */
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
					while (ldesc(i)) { i++; try_descc++; }
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
	ladspa_db.descv = realloc(ladspa_db.descv, ladspa_db.descc * sizeof(void *)); /* TODO: this realloc is bad and ugly (and bad) */

	closedir(dir);
}

static void freeLadspaDB(void)
{
	for (size_t i = 0; i < ladspa_db.symbolc; i++)
		dlclose(ladspa_db.symbolv[i]);

	free(ladspa_db.symbolv);
	free(ladspa_db.descv);
}

static uint32_t getLadspaDBCount(void)
{
	return ladspa_db.descc;
}

static EffectBrowserLine getLadspaDBLine(uint32_t index)
{
	EffectBrowserLine ret;
	ret.name =  strdup(ladspa_db.descv[index]->Name);
	ret.maker = strdup(ladspa_db.descv[index]->Maker);
	ret.data =  ladspa_db.descv[index];
	return ret;
}

static uint32_t getLadspaEffectControlCount(void *state)
{
	return ((LadspaState*)state)->controlc;
}

static short getLadspaEffectHeight(void *state)
{
	return getLadspaEffectControlCount(state) + 3;
}

static void freeLadspaEffect(void *state)
{
	LadspaState *s = state;

	if (s->desc->deactivate)
		s->desc->deactivate(s->instance);
	s->desc->cleanup(s->instance);
	if (s->controlv) free(s->controlv);
	if (s->dummyport) free(s->dummyport);
	free(s);
}

static LADSPA_Data getLadspaPortMin(LADSPA_PortRangeHint hint)
{
	if (!LADSPA_IS_HINT_BOUNDED_BELOW(hint.HintDescriptor)) return EFFECT_CONTROL_DEF_MIN;

	LADSPA_Data ret = hint.LowerBound;

	if (LADSPA_IS_HINT_SAMPLE_RATE(hint.HintDescriptor)) ret *= samplerate;
	if (LADSPA_IS_HINT_INTEGER    (hint.HintDescriptor)) ret  = floorf(ret);

	return ret;
}
static LADSPA_Data getLadspaPortMax(LADSPA_PortRangeHint hint)
{
	if (!LADSPA_IS_HINT_BOUNDED_ABOVE(hint.HintDescriptor)) return EFFECT_CONTROL_DEF_MAX;

	LADSPA_Data ret = hint.UpperBound;

	if (LADSPA_IS_HINT_SAMPLE_RATE(hint.HintDescriptor)) ret *= samplerate;
	if (LADSPA_IS_HINT_INTEGER    (hint.HintDescriptor)) ret  = floorf(ret);

	return ret;
}
static LADSPA_Data getLadspaPortDef(LADSPA_PortRangeHint hint)
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

static void startLadspaEffect(LadspaState *s, float **input, float **output)
{
	s->instance = s->desc->instantiate(s->desc, samplerate);

	/* iterate first to find the size of the control block */
	s->controlc = 0;
	for (uint32_t i = 0; i < s->desc->PortCount; i++)
		if (LADSPA_IS_PORT_CONTROL(s->desc->PortDescriptors[i]) && LADSPA_IS_PORT_INPUT(s->desc->PortDescriptors[i]))
			s->controlc++;

	/* allocate the control block */
	bool setDefaults = 0; /* only set the defaults if memory isn't yet allocated */
	if (!s->controlv)
	{
		s->controlv = calloc(s->controlc, sizeof(LADSPA_Data));
		setDefaults = 1;
	}

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
					if (input)
						s->desc->connect_port(s->instance, i, input[s->inputc]);

					s->inputc++;
				}
			} else /* implies output */
			{
				if (s->outputc < 2)
				{
					if (output)
						s->desc->connect_port(s->instance, i, output[s->outputc]);

					s->outputc++;
				}
			}
		}
	}

	if (s->desc->activate)
		s->desc->activate(s->instance);
}

static void _initLadspaEffect(LadspaState *s, const LADSPA_Descriptor *desc, float **input, float **output)
{
	s->desc = desc;
	s->dummyport = malloc(sizeof(LADSPA_Data) * samplerate);
	startLadspaEffect(s, input, output);
}
static void *initLadspaEffect(const void *data, float **input, float **output)
{
	LadspaState *s = calloc(1, sizeof(LadspaState));
	_initLadspaEffect(s, data, input, output);
	return (void*)s;
}
static void *copyLadspaEffect(void *src, float **input, float **output)
{
	LadspaState *ret = calloc(1, sizeof(LadspaState));
	LadspaState *s = src;
	_initLadspaEffect(ret, s->desc, input, output);
	memcpy(ret->controlv, s->controlv, s->controlc * sizeof(LADSPA_Data));
	return ret;
}

/* the current text colour will apply to the header but not the contents */
static void drawLadspaEffect(void *state, short x, short w, short y, short ymin, short ymax)
{
	LadspaState *s = state;

	printf("\033[7m");
	if (ymin <= y-1 && ymax >= y-1)
		printCulling("LADSPA", x+1, y-1, 1, ws.ws_col);
	printf("\033[27;37;40m");

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
			drawAutogenPluginLine(x, y+1 + controlp, w, ymin, ymax,
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

/* only valid to call if input and output are not NULL */
static void runLadspaEffect(void *state, uint32_t samplecount, float **input, float **output)
{
	LadspaState *s = state;

	s->desc->run(s->instance, samplecount);

	if (s->outputc == 1) /* handle mono output correctly */
		memcpy(output[1], output[0], sizeof(float)*samplecount);
}

static struct json_object *serializeLadspaEffect(void *state)
{
	LadspaState *s = state;
	struct json_object *obj = json_object_new_object();
	json_object_object_add(obj, "UniqueID", json_object_new_int(s->desc->UniqueID));
	json_object_object_add(obj, "Label", json_object_new_string(s->desc->Label));

	struct json_object *array = json_object_new_array_ext(s->controlc);
	for (uint32_t i = 0; i < s->controlc; i++)
		json_object_array_add(array, json_object_new_double(s->controlv[i]));
	json_object_object_add(obj, "control", array);

	return obj;
}

static void *deserializeLadspaEffect(struct json_object *jso, float **input, float **output)
{
	unsigned long UniqueID = json_object_get_int(json_object_object_get(jso, "UniqueID"));
	const char *Label = json_object_get_string(json_object_object_get(jso, "Label"));

	for (unsigned long i = 0; i < ladspa_db.descc; i++)
		if (ladspa_db.descv[i]->UniqueID == UniqueID && !strcmp(ladspa_db.descv[i]->Label, Label))
		{
			LadspaState *ret = initLadspaEffect(ladspa_db.descv[i], input, output);

			struct json_object *array = json_object_object_get(jso, "control");
			for (uint32_t i = 0; i < json_object_array_length(array); i++)
				ret->controlv[i] = json_object_get_double(json_object_array_get_idx(array, i));

			return ret;
		}

	/* TODO: handle the plugin not being found */
	return NULL;
}
