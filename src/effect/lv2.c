static void initLV2DB(void)
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
	lv2_db.units_unit   = lilv_new_uri(lv2_db.world, LV2_UNITS__unit);
	lv2_db.units_render = lilv_new_uri(lv2_db.world, LV2_UNITS__render);
}

static void freeLV2DB(void)
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
	lilv_node_free(lv2_db.units_unit);
	lilv_node_free(lv2_db.units_render);
	lilv_world_free(lv2_db.world);
}

static uint32_t getLV2DBCount(void)
{
	return lilv_plugins_size(lilv_world_get_all_plugins(lv2_db.world));
}

static EffectBrowserLine getLV2DBLine(uint32_t index)
{
	EffectBrowserLine ret = { 0 };
	const LilvPlugins *lap = lilv_world_get_all_plugins(lv2_db.world);
	const LilvPlugin *lp = NULL;
	LilvNode *node;
	uint32_t i = 0;
	LILV_FOREACH(plugins, iter, lap)
		if (i == index)
		{
			lp = lilv_plugins_get(lap, iter);
			break;
		} else i++;

	if (lp)
	{
		if ((node = lilv_plugin_get_name(lp)))
		{
			ret.name = strdup(lilv_node_as_string(node));
			lilv_node_free(node);
		}

		if ((node = lilv_plugin_get_author_name(lp)))
		{
			ret.maker = strdup(lilv_node_as_string(node));
			lilv_node_free(node);
		}

		ret.data = lp;
	}
	return ret;
}

LV2_URID lv2_map_uri(LV2_URID_Map_Handle handle, const char *uri)
{
	struct _UridMapping *s = handle;

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
	struct _UridMapping *s = handle;
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
		else                                                        ret = EFFECT_CONTROL_DEF_MIN;
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
		else                                                        ret = EFFECT_CONTROL_DEF_MAX;
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

static void startLV2Effect(LV2State *s, float **input, float **output)
{
	LilvNode *node;

	/* TODO: required features */
	/* LilvNodes *nodes = lilv_plugin_get_required_features(s->plugin);
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
				if (s->inputc < 2 && input)
					lilv_instance_connect_port(s->instance, i, input[s->inputc]);
				else
					lilv_instance_connect_port(s->instance, i, s->dummyport);

				s->inputc++;
			} else if (lilv_port_is_a(s->plugin, lpo, lv2_db.output_port))
			{
				if (s->outputc < 2 && output)
					lilv_instance_connect_port(s->instance, i, output[s->outputc]);
				else
					lilv_instance_connect_port(s->instance, i, s->dummyport);

				s->outputc++;
			}
		} else if (lilv_port_is_a(s->plugin, lpo, lv2_db.cv_port))
			lilv_instance_connect_port(s->instance, i, s->dummyport);
	}

	lilv_instance_activate(s->instance);
}

static void freeLV2Effect(void *state)
{
	LV2State *s = state;

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
	free(s);
}

static void _initLV2Effect(LV2State *s, const LilvPlugin *plugin, float **input, float **output)
{
	s->plugin = plugin;

	s->urid_map.handle = s->urid_unmap.handle = &s->urid;
	s->urid_map.map = lv2_map_uri;
	s->urid_unmap.unmap = lv2_unmap_uri;
	s->urid_map_feature = (LV2_Feature){LV2_URID__map, &s->urid_map };
	s->urid_unmap_feature = (LV2_Feature){LV2_URID__unmap, &s->urid_unmap };

	s->features = calloc(LV2_SUPPORTED_FEATURE_COUNT + 1, sizeof(LV2_Feature));
	s->features[0] = &s->urid_map_feature;
	s->features[1] = &s->urid_unmap_feature;
	s->features[LV2_SUPPORTED_FEATURE_COUNT] = NULL;

	s->dummyport = malloc(sizeof(float) * samplerate);

	startLV2Effect(s, input, output);
}
// static void *initLV2Effect(void *data, float **input, float **output, const LilvPlugin *plugin)
static void *initLV2Effect(const void *data, float **input, float **output)
{
	LV2State *s = malloc(sizeof(LV2State));
	_initLV2Effect(s, data, input, output);
	return (void*)s;
}

static void copyLV2Effect(void *dest, void *src, float **input, float **output)
{
	LV2State *d = dest;
	LV2State *s = src;
	_initLV2Effect(d, s->plugin, input, output);
	memcpy(d->controlv, s->controlv, s->controlc * sizeof(float));
}

static uint32_t getLV2EffectControlCount(void *s)
{
	return ((LV2State*)s)->controlc;
}
static short getLV2EffectHeight(void *s)
{
	return getLV2EffectControlCount(s) + 3;
}

/* the current text colour will apply to the header but not the contents */
static void drawLV2Effect(void *state,
		short x, short w,
		short y, short ymin, short ymax)
{
	LV2State *s = state;

	if (ymin <= y-1 && ymax >= y-1)
		printf("\033[%d;%dH\033[7mLV2\033[27m", y-1, x + 1);
	printf("\033[37;40m");

	LilvNode *name;
	name = lilv_plugin_get_name(s->plugin);
	if (ymin <= y && ymax >= y)
	{
		printf("\033[1m");
		drawCentreText(x+2, y, w-4, lilv_node_as_string(name));
		printf("\033[22m");
	}
	lilv_node_free(name);

	LilvNode *def, *min, *max, *unit, *render;
	LilvScalePoints *scalepoints;
	char *prefix, *postfix, *subptr;
	const char *srender = NULL;

	uint32_t controlp = 0;
	uint32_t scalepointlen, scalepointcount, splen;
	const LilvPort *lpo;
	for (uint32_t i = 0; i < lilv_plugin_get_num_ports(s->plugin); i++)
	{
		lpo = lilv_plugin_get_port_by_index(s->plugin, i);
		if (lilv_port_is_a(s->plugin, lpo, lv2_db.control_port)
		 && lilv_port_is_a(s->plugin, lpo, lv2_db.input_port))
		{
			prefix = NULL;
			postfix = NULL;
			scalepointlen = scalepointcount = 0;
			lilv_port_get_range(s->plugin, lpo, &def, &min, &max);

			/* prefix/postfix */
			// render = lilv_port_get(s->plugin, lpo, lv2_db.units_render);
			unit = lilv_port_get(s->plugin, lpo, lv2_db.units_unit);
			if (unit)
			{
				render = lilv_world_get(lv2_db.world, unit, lv2_db.units_render, NULL);
				if (render)
				{
					srender = lilv_node_as_string(render);
					if ((subptr = strstr(srender, "%f")))
					{
						if (subptr > srender) /* prefix present */
						{
							prefix = calloc(subptr - srender + 2, sizeof(char));
							strncpy(prefix, srender, subptr - srender);
						}
						if (strlen(srender) - (subptr - srender) > 2) /* postfix present */
						{
							postfix = calloc(strlen(srender) - (subptr - srender) - 1, sizeof(char));
							strcpy(postfix, subptr + 2);
						}
					}
					lilv_node_free(render);
				}
				lilv_node_free(unit);
			}


			name = lilv_port_get_name(s->plugin, lpo);
			scalepoints = lilv_port_get_scale_points(s->plugin, lpo);
			if (scalepoints)
			{
				scalepointcount = lilv_scale_points_size(scalepoints);
				LILV_FOREACH(scale_points, j, scalepoints)
				{
					splen = lilv_node_as_int(
							lilv_scale_point_get_label(
								lilv_scale_points_get(
									scalepoints, j)));
					scalepointlen = MAX(scalepointlen, splen);
				} lilv_scale_points_free(scalepoints);
			} else
				drawAutogenPluginLine(x, y+1 + controlp, w, ymin, ymax,
						lilv_node_as_string(name), &s->controlv[controlp],
						lilv_port_has_property(s->plugin, lpo, lv2_db.toggled),
						lilv_port_has_property(s->plugin, lpo, lv2_db.integer),
						getLV2PortMin(s, lpo, min),
						getLV2PortMax(s, lpo, max),
						getLV2PortDef(s, lpo, def),
						prefix, postfix, scalepointlen, scalepointcount);

			lilv_node_free(name);
			if (def) lilv_node_free(def);
			if (min) lilv_node_free(min);
			if (max) lilv_node_free(max);
			if (prefix) free(prefix);
			if (postfix) free(postfix);

			controlp++;
		}
	}
}

static void runLV2Effect(void *state, uint32_t samplecount, float **input, float **output)
{
	LV2State *s = state;

	lilv_instance_run(s->instance, samplecount);

	if (s->outputc == 1) /* handle mono output correctly */
		memcpy(output[1], output[0], sizeof(float)*samplecount);
}

static struct json_object *serializeLV2Effect(void *state)
{
	LV2State *s = state;
	struct json_object *obj = json_object_new_object();
	json_object_object_add(obj, "uri", json_object_new_string(lilv_node_as_string(lilv_plugin_get_uri(s->plugin))));

	struct json_object *array = json_object_new_array_ext(s->controlc);
	for (uint32_t i = 0; i < s->controlc; i++)
		json_object_array_add(array, json_object_new_double(s->controlv[i]));
	json_object_object_add(obj, "control", array);

	return obj;
}

static void *deserializeLV2Effect(struct json_object *jso, float **input, float **output)
{
	LilvNode *node = lilv_new_uri(lv2_db.world, json_object_get_string(json_object_object_get(jso, "Label")));
	LV2State *ret = initLV2Effect(lilv_plugins_get_by_uri(lilv_world_get_all_plugins(lv2_db.world), node), input, output);
	lilv_node_free(node);

	struct json_object *array = json_object_object_get(jso, "control");
	for (uint32_t i = 0; i < json_object_array_length(array); i++)
		ret->controlv[i] = json_object_get_double(json_object_array_get_idx(array, i));

	return ret;
}
