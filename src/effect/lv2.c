typedef struct {
	LilvWorld *world;

	/* ports */
	LilvNode  *audio_port;
	LilvNode  *control_port;
	LilvNode  *cv_port;
	LilvNode  *input_port;
	LilvNode  *output_port;
	LilvNode  *toggled;
	LilvNode  *integer;
	LilvNode  *enumeration;
	LilvNode  *sampleRate;
} LV2DB;
LV2DB lv2_db;

void initLV2DB(void)
{
	lv2_db.world = lilv_world_new();
	lilv_world_load_all(lv2_db.world);

	/* ports */
	lv2_db.audio_port   = lilv_new_uri(lv2_db.world, LILV_URI_AUDIO_PORT);
	lv2_db.control_port = lilv_new_uri(lv2_db.world, LILV_URI_CONTROL_PORT);
	lv2_db.cv_port      = lilv_new_uri(lv2_db.world, LILV_URI_CV_PORT);
	lv2_db.input_port   = lilv_new_uri(lv2_db.world, LILV_URI_INPUT_PORT);
	lv2_db.output_port  = lilv_new_uri(lv2_db.world, LILV_URI_OUTPUT_PORT);
	lv2_db.toggled      = lilv_new_uri(lv2_db.world, LV2_CORE__toggled);
	lv2_db.integer      = lilv_new_uri(lv2_db.world, LV2_CORE__integer);
	lv2_db.enumeration  = lilv_new_uri(lv2_db.world, LV2_CORE__enumeration);
	lv2_db.sampleRate   = lilv_new_uri(lv2_db.world, LV2_CORE__sampleRate);
}

void freeLV2DB(void)
{
	lilv_node_free(lv2_db.audio_port);
	lilv_node_free(lv2_db.control_port);
	lilv_node_free(lv2_db.cv_port);
	lilv_node_free(lv2_db.input_port);
	lilv_node_free(lv2_db.output_port);
	lilv_node_free(lv2_db.toggled);
	lilv_node_free(lv2_db.integer);
	lilv_node_free(lv2_db.enumeration);
	lilv_node_free(lv2_db.sampleRate);
	lilv_world_free(lv2_db.world);
}

typedef struct { /* used by both LV2_URID_Map and LV2_URID_Unmap */
	LV2_URID size;    /* how many strings are allocated */
	char   **string;  /* string[i + 1] is the index */
} UridMapping;

typedef struct {
	const LilvPlugin   *plugin;
	LilvInstance       *instance;
	UridMapping         urid;
	LV2_URID_Map        urid_map;
	LV2_Feature         urid_map_feature;
	LV2_URID_Unmap      urid_unmap;
	LV2_Feature         urid_unmap_feature;
	const LV2_Feature **features;

	uint32_t          inputc;   /* input audio port count   */
	uint32_t          outputc;  /* output audio port count  */
	uint32_t          controlc; /* input control port count */
	float            *controlv; /* input control ports      */
	float            *dummyport;
	const LilvNode   *uri; /* not read from desc cos even if desc is null this needs to be serialized */
} LV2State;

/* typedef uint32_t LV2_URID */
LV2_URID lv2_map_uri(LV2_URID_Map_Handle handle, const char *uri)
{
	UridMapping *s = handle;

	/* check if the string is already allocated */
	if (s->string)
		for (LV2_URID i = 0; i < s->size; i++)
			if (!strcmp(s->string[i], uri)) return i+1;

	/* allocate the string */
	s->string = realloc(s->string, (s->size+1) * sizeof(char*));
	s->string[s->size] = strdup(uri);
	return s->size++;
}
const char *lv2_unmap_uri(LV2_URID_Unmap_Handle handle, LV2_URID urid)
{
	UridMapping *s = handle;
	if (urid-1 < s->size)
		return s->string[urid-1];
	return 0;
}


float getLV2PortMin(LV2State *s, const LilvPort *lpo, const LilvNode *node)
{
	float ret;
	if (!node)
	{
		if (lilv_port_has_property(s->plugin, lpo, lv2_db.toggled)) ret = 0.0f;
		else                                                        ret = LADSPA_DEF_MIN;
	} else ret = lilv_node_as_float(node);

	if (lilv_port_has_property(s->plugin, lpo, lv2_db.sampleRate)) ret *= samplerate;
	if (lilv_port_has_property(s->plugin, lpo, lv2_db.integer   )) ret = floorf(ret);

	return ret;
}
float getLV2PortMax(LV2State *s, const LilvPort *lpo, const LilvNode *node)
{
	float ret;
	if (!node)
	{
		if (lilv_port_has_property(s->plugin, lpo, lv2_db.toggled)) ret = 1.0f;
		else                                                        ret = LADSPA_DEF_MAX;
	} else ret = lilv_node_as_float(node);

	if (lilv_port_has_property(s->plugin, lpo, lv2_db.sampleRate)) ret *= samplerate;
	if (lilv_port_has_property(s->plugin, lpo, lv2_db.integer   )) ret = floorf(ret);

	return ret;
}
float getLV2PortDef(LV2State *s, const LilvPort *lpo, const LilvNode *node)
{
	float ret;
	if (!node) ret = 0.0f;
	else       ret = lilv_node_as_float(node);

	if (lilv_port_has_property(s->plugin, lpo, lv2_db.sampleRate)) ret *= samplerate;
	if (lilv_port_has_property(s->plugin, lpo, lv2_db.integer   )) ret = floorf(ret);

	return ret;
}

void startLV2Effect(EffectChain *chain, Effect *e)
{
	LV2State *s = e->state;
	LilvNode *node;

	/* TODO: required features */
	/* LilvNodes *nodes = lilv_plugin_get_required_features(s->plugin);
DEBUG=lilv_nodes_size(nodes); p->redraw=1;
	lilv_nodes_free(nodes); */

	s->instance = lilv_plugin_instantiate(s->plugin, (double)samplerate, s->features);

	s->controlc = lilv_plugin_get_num_ports_of_class(s->plugin, lv2_db.control_port, lv2_db.input_port, NULL);
	/* allocate the control block */
	bool setDefaults = 0; /* only set the defaults if memory isn't yet allocated */
	if (!s->controlv) { s->controlv = calloc(s->controlc, sizeof(float)); setDefaults = 1; }

	/* iterate to connect ports */
	uint32_t controlp = 0;
	s->inputc = 0;
	s->outputc = 0;
	const LilvPort *lpo;
	for (uint32_t i = 0; i < lilv_plugin_get_num_ports(s->plugin); i++)
	{
		lpo = lilv_plugin_get_port_by_index(s->plugin, i);
		if (lilv_port_is_a(s->plugin, lpo, lv2_db.control_port))
		{
			if (lilv_port_is_a(s->plugin, lpo, lv2_db.input_port))
			{
				lilv_instance_connect_port(s->instance, i, &s->controlv[controlp]);
				if (setDefaults)
				{
					lilv_port_get_range(s->plugin, lpo, &node, NULL, NULL); /* only care about the default value */
					if (node)
					{
						s->controlv[controlp] = lilv_node_as_float(node);
						lilv_node_free(node);
					}
				} controlp++;
			} else /* connect non-input control ports to the dummy port TODO: show output control ports */
				lilv_instance_connect_port(s->instance, i, s->dummyport);
		} else if (lilv_port_is_a(s->plugin, lpo, lv2_db.audio_port))
		{
			if (lilv_port_is_a(s->plugin, lpo, lv2_db.input_port))
			{
				if (s->inputc < 2 && chain->input)
					lilv_instance_connect_port(s->instance, i, chain->input[s->inputc]);
				else
					lilv_instance_connect_port(s->instance, i, s->dummyport);

				s->inputc++;
			} else if (lilv_port_is_a(s->plugin, lpo, lv2_db.output_port))
			{
				if (s->outputc < 2 && chain->output)
					lilv_instance_connect_port(s->instance, i, chain->output[s->outputc]);
				else
					lilv_instance_connect_port(s->instance, i, s->dummyport);

				s->outputc++;
			}
		} else if (lilv_port_is_a(s->plugin, lpo, lv2_db.cv_port))
			lilv_instance_connect_port(s->instance, i, s->dummyport);
	}

	lilv_instance_activate(s->instance);
}

void freeLV2Effect(Effect *e)
{
	LV2State *s = e->state;

	lilv_instance_deactivate(s->instance);
	lilv_instance_free(s->instance);
	free(s->features);
	if (s->urid.string)
	{
		for (LV2_URID i = 0; i < s->urid.size; i++)
			free(s->urid.string[i]);
		free(s->urid.string);
	}
	if (s->controlv) free(s->controlv);
	if (s->dummyport) free(s->dummyport);
}

#define LV2_SUPPORTED_FEATURE_COUNT 2
void initLV2Effect(EffectChain *chain, Effect *e, const LilvPlugin *lp)
{
	e->type = EFFECT_TYPE_LV2;
	LV2State *s = e->state = calloc(1, sizeof(LV2State));
	s->plugin = lp;
	s->uri = lilv_plugin_get_uri(lp);

	s->urid_map.handle = s->urid_unmap.handle = &s->urid;
	s->urid_map.map = lv2_map_uri;
	s->urid_unmap.unmap = lv2_unmap_uri;
	s->urid_map_feature =   (LV2_Feature){ LV2_URID__map,   &s->urid_map   };
	s->urid_unmap_feature = (LV2_Feature){ LV2_URID__unmap, &s->urid_unmap };

	s->features = calloc(LV2_SUPPORTED_FEATURE_COUNT + 1, sizeof(LV2_Feature));
	s->features[0] = &s->urid_map_feature;
	s->features[1] = &s->urid_unmap_feature;
	s->features[LV2_SUPPORTED_FEATURE_COUNT] = NULL;

	s->dummyport = malloc(sizeof(float) * samplerate);

	startLV2Effect(chain, e);
}

void copyLV2Effect(EffectChain *destchain, Effect *dest, Effect *src)
{
	LV2State *s_src = src->state;
	initLV2Effect(destchain, dest, s_src->plugin);
	LV2State *s_dest = dest->state;

	memcpy(s_dest->controlv, s_src->controlv, s_src->controlc * sizeof(float));
}

uint32_t getLV2EffectControlCount(Effect *e) { return ((LV2State *)e->state)->controlc;     }
short    getLV2EffectHeight      (Effect *e) { return ((LV2State *)e->state)->controlc + 3; }

void serializeLV2Effect(Effect *e, FILE *fp)
{
	LV2State *s = e->state;

	// ((LV2State *)e->state)->uri = lilv_plugin_get_uri(lp);
	const char *suri = lilv_node_as_string(s->uri);
	size_t surilen = strlen(suri);
	fwrite(&surilen, sizeof(size_t), 1, fp);
	fwrite(suri, sizeof(char), surilen+1, fp);

	fwrite(&s->controlc, sizeof(uint32_t), 1, fp);
	fwrite(s->controlv, sizeof(float), s->controlc, fp);
}
void deserializeLV2Effect(EffectChain *chain, Effect *e, FILE *fp)
{
	e->state = calloc(1, sizeof(LV2State));
	LV2State *s = e->state;

	size_t surilen;
	fread(&surilen, sizeof(size_t), 1, fp);
	char *suri = malloc(surilen+1);
	fread(suri, sizeof(char), surilen+1, fp);
	s->uri = lilv_new_string(lv2_db.world, suri);
	free(suri);

	s->plugin = lilv_plugins_get_by_uri(lilv_world_get_all_plugins(lv2_db.world), s->uri);

	fread(&s->controlc, sizeof(uint32_t), 1, fp);
	s->controlv = calloc(s->controlc, sizeof(float));
	fread(s->controlv, sizeof(float), s->controlc, fp);

	/* TODO: look up desc based on uuid */
	startLV2Effect(chain, e);
}

void drawLV2Effect(Effect *e, ControlState *cc,
		short x, short w,
		short y, short ymin, short ymax)
{
	LV2State *s = e->state;

	LilvNode *node, *def, *min, *max;

	node = lilv_plugin_get_name(s->plugin);
	if (ymin <= y && ymax >= y)
	{
		printf("\033[1m");
		drawCentreText(x+2, y, w-4, lilv_node_as_string(node));
		printf("\033[22m");
	}
	lilv_node_free(node);

	uint32_t controlp = 0;
	const LilvPort *lpo;
	for (uint32_t i = 0; i < lilv_plugin_get_num_ports(s->plugin); i++)
	{
		lpo = lilv_plugin_get_port_by_index(s->plugin, i);
		if (lilv_port_is_a(s->plugin, lpo, lv2_db.control_port)
		 && lilv_port_is_a(s->plugin, lpo, lv2_db.input_port))
		{
			node = lilv_port_get_name(s->plugin, lpo);
			lilv_port_get_range(s->plugin, lpo, &def, &min, &max);
			drawAutogenPluginLine(cc, x, y+1 + controlp, w, ymin, ymax,
					lilv_node_as_string(node), &s->controlv[controlp],
					lilv_port_has_property(s->plugin, lpo, lv2_db.toggled),
					lilv_port_has_property(s->plugin, lpo, lv2_db.integer),
					getLV2PortMin(s, lpo, min),
					getLV2PortMax(s, lpo, max),
					getLV2PortDef(s, lpo, def));
			lilv_node_free(node);
			if (def) lilv_node_free(def);
			if (min) lilv_node_free(min);
			if (max) lilv_node_free(max);

			controlp++;
		}
	}
}

void runLV2Effect(uint32_t samplecount, EffectChain *chain, Effect *e)
{
	LV2State *s = e->state;

	lilv_instance_run(s->instance, samplecount);

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
