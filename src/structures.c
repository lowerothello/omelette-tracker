#define MAX_VALUE_LEN 128

typedef struct
{
	uint8_t note;
	uint8_t inst;
	char    macroc[2]; /* command */
	uint8_t macrov[2]; /* argument */
} row;
typedef struct
{
	uint8_t rowc;
	row     rowv[64][256]; /* 64 channels, each with 256 rows */
} pattern;

typedef struct
{
	uint8_t a;
	uint8_t d;
	uint8_t s;
	uint8_t r;
} adsr;

typedef struct /* TODO: move into instrument_sampler.c */
{
	uint32_t length;
	char     channels;
	uint32_t c5rate;
	uint32_t trim[2];
	uint32_t loop[2];
	adsr     volume;
} sampler_state;

typedef struct
{
	char     mute;            /* saved to disk */
	uint32_t samplepointer;   /* progress through the sample */
	uint32_t sampleoffset;    /* point to base samplepointer off of */
	uint32_t releasepointer;  /* 0 for no release, where releasing started */
	uint8_t  gain;            /* two 4bit uints, one for each channel */
	row      r;
	float    cents;           /* 1 fractional semitone, used for portamento, always between -0.5 and +0.5 */
	uint8_t  portamento;      /* portamento target, 255 for off */
	uint8_t  portamentospeed; /* portamento m */
	uint16_t rtrigsamples;    /* samples per retrigger */
	uint32_t rtrigpointer;    /* sample ptr to ratchet back to */
	uint8_t  effectholdinst;  /* 255 for no hold */
	uint8_t  effectholdindex;

	uint16_t rampindex;       /* progress through the ramp buffer, rampmax if not ramping */
	uint16_t rampmax;         /* length of the ramp buffer */
	float   *rampbuffer;      /* samples to ramp out */
} channel;

typedef struct
{
	uint8_t  type;
	uint8_t  typefollow;         /* follows the type, set once state is guaranteed to be mallocced */
	short   *sampledata;         /* variable size, persists between types */
	uint32_t samplelength;       /* raw samples allocated for sampledata */
	void    *state;              /* instrument working memory */
	char     lock;
	uint8_t  fader[2];
	char     send[16];           /* [0-15][0-15] are used */
	char     processsend[16];    /* takes priority over send, set and used in process */
	void    *plugininstance[16]; /* pointer to an instance of a plugin type */
	char     pluginactive[16];   /* actual active status, follows send */
} instrument;

#define LV2_TYPE_INPUT 0
#define LV2_TYPE_OUTPUT 1
typedef struct
{
	uint32_t     index;
	float        min, max, value;
	char        *name;
	char        *format;
	char         integer;                    /* true for integer, false for float */
	char         toggled;                    /* true if value is boolean */
	char         logarithmic;                /* true if value should be adjusted logarithmically */
	char         samplerate;                 /* true to multiply values by the sample rate? documentation isn't clear */
	int          steps;                      /* the number of steps to divide min-max into */
	unsigned int enumerate;                  /* the number of scalepoints, or 0 for no enumeration */
	char       (*scalelabel)[MAX_VALUE_LEN]; /* string array */
	float       *scalevalue;                 /* float array */
} lv2control;
typedef struct
{
	uint32_t index;
	char     type;
} lv2audio;

typedef struct
{
	uint8_t           type;            /* 0:empty, 2:lv2 */
	uint32_t          indexc;
	const LilvPlugin *plugin;
	const char       *name;
	uint32_t          controlc, audioc;
	uint32_t          inputc, outputc; /* audio input/output count */
	lv2control       *controlv;
	lv2audio         *audiov;
} effect;

#define PLAYING_STOP 0
#define PLAYING_START 1
#define PLAYING_CONT 2
#define PLAYING_PREP_STOP 3
typedef struct
{
	uint8_t     patternc;                /* pattern count */
	uint8_t     patterni[256];           /* pattern backref */
	pattern    *patternv[256];           /* pattern values */
	pattern     patternbuffer;           /* pattern yank buffer */

	uint8_t     instrumentc;             /* instrument count */
	uint8_t     instrumenti[256];        /* instrument backref */
	instrument *instrumentv[256];        /* instrument values */
	instrument  instrumentbuffer;        /* instrument yank buffer */

	effect      effectv[16];             /* effect values */
	float      *effectinl, *effectinr;   /* effect inputs */
	float      *effectoutl, *effectoutr; /* effect outputs */

	uint8_t     channelc;                /* channel count */
	channel     channelv[64];            /* channel values */
	row        *channelbuffer[64];       /* channel yank buffer */
	char        channelbuffermute;

	uint8_t     songi[256];              /* song list backref, links to patterns */
	uint8_t     songa[256];              /* song list attributes */

	uint8_t     songp;                   /* song pos, analogous to window->songfx */
	uint16_t    songr;                   /* song row, analogous to window->trackerfy */

	uint8_t     rowhighlight;
	uint8_t     defpatternlength;        /* only here cos window isn't defined yet */
	uint16_t    bpm;
	uint16_t    songbpm;                 /* to store the song's bpm through bpm change macros */
	uint32_t    spr;                     /* samples per row (samplerate * (60 / bpm) / 4) */
	uint32_t    sprp;                    /* samples per row progress */
	char        playing;
} song;


#define INST_GLOBAL_LOCK_OK 0        /* playback and most memory ops are safe */
#define INST_GLOBAL_LOCK_PREP_FREE 1 /* playback unsafe, preparing to free the state */
#define INST_GLOBAL_LOCK_FREE 2      /* playback has stopped, safe to free the state */
/* inst_global_lock - 16 = the effect whose default mix has changed */

#define INST_REC_LOCK_OK 0       /* playback and most memory ops are safe */
#define INST_REC_LOCK_CONT 1     /* recording                             */
#define INST_REC_LOCK_PREP_END 2 /* start stopping recording              */
#define INST_REC_LOCK_END 3      /* stopping recording has finished       */

#define REQ_OK 0  /* do nothing / done */
#define REQ_BPM 1 /* re-apply the song bpm */

typedef struct
{
	char           filepath[COMMAND_LENGTH];

	command_t      command;
	char          *search;

	unsigned char  popup;
	unsigned char  mode;
	unsigned short centre;
	uint8_t        pattern;               /* focused pattern */
	uint8_t        channel;               /* focused channel */
	uint8_t        channeloffset, visiblechannels;

	short          trackerfy, trackerfx;
	unsigned short trackercelloffset;
	short          instrumentindex;
	uint8_t        instrument;            /* focused instrument */
	unsigned short instrumentcelloffset;
	short          instrumentsend;        /* focused send */

	short          effectindex;
	short          pluginindex;
	short          effectoffset;          /* scroll offset for pluginindex */
	unsigned char  effect;                /* focused effect */
	char         (*pluginlist)[INSTRUMENT_TYPE_COLS - 2];

	unsigned short mousey, mousex;

	short          fyoffset;
	signed char    fieldpointer;

	char           dirpath[NAME_MAX + 1];
	unsigned int   dirc;
	unsigned short dirmaxwidth;
	DIR           *dir;

	uint8_t        songfx;
	uint8_t        songoffset, songvisible;
	unsigned short songcelloffset;

	unsigned short instrumentrowoffset;

	char           octave;

	unsigned short akaizertimefactor, akaizercyclelength;

	uint8_t        songnext;

	channel        previewchannel;
	instrument     previewinstrument;     /* used by the file browser */
	char           previewchanneltrigger; /* 0:stopped
	                                         1:start inst
	                                         2:still inst
	                                         3:start sample
	                                         4:still sample
	                                         5:prep volatile */

	char           previewsamplestatus;

	uint8_t        instrumentlocki;       /* realindex */
	uint8_t        instrumentlockv;       /* value, set to an INST_GLOBAL_LOCK constant */

	uint8_t        instrumentreci;        /* realindex */
	uint8_t        instrumentrecv;        /* value, set to an INST_REC_LOCK constant */
	short         *recbuffer;             /* disallow changing the type or removing while recording */
	uint32_t       recptr;

	char           request;               /* ask the playback function to do something */
} window;

typedef struct
{
	struct
	{
		void         (*draw) (instrument *, uint8_t, unsigned short, unsigned short, short *, unsigned char);
		unsigned short indexc;               /* index count used (0 inclusive) */
		void         (*adjustUp)(instrument *, short);
		void         (*adjustDown)(instrument *, short);
		void         (*adjustLeft)(instrument *, short);
		void         (*adjustRight)(instrument *, short);
		void         (*incFieldPointer)(signed char *, short);
		void         (*decFieldPointer)(signed char *, short);
		void         (*endFieldPointer)(signed char *, short);
		void         (*mouseToIndex)(int, int, short *, signed char *);
		void         (*input)(int *);
		void         (*process)(instrument *, channel *, uint32_t, sample_t *, sample_t *);
		uint32_t     (*offset)(instrument *, channel *, int);
		uint8_t      (*getOffset)(instrument *, channel *);
		void         (*changeType)(void **); /* literally a pointer to a pointer */
		void         (*loadSample)(instrument *, SF_INFO);
		void         (*exportSample)(instrument *, SF_INFO *);
		void         (*write)(instrument *, FILE *fp);
		void         (*read)(instrument *, FILE *fp);
	}                  f[INSTRUMENT_TYPE_COUNT];
} typetable;
typetable *t;

typedef struct
{
	LilvWorld   *world;
	LilvPlugins *plugins;
	unsigned int pluginc; /* indexed plugin count */
	LilvNode    *inputport;
	LilvNode    *outputport;
	LilvNode    *audioport;
	LilvNode    *controlport;
	LilvNode    *integer;
	LilvNode    *toggled;
	LilvNode    *samplerate;
	LilvNode    *render;
	LilvNode    *unit;
	LilvNode    *enumeration;
	LilvNode    *logarithmic;
	LilvNode    *rangesteps;
} lilv;
lilv lv2;

/* replace *find with *replace in *s */
/* only replaces the first instance of *find */
void strrep(char *s, char *find, char *replace)
{
	char *buffer = malloc(strlen(s));
	char *pos = strstr(s, find);
	if (pos)
	{
		strcpy(buffer, s);
		size_t len = pos - s + strlen(find);
		memmove(buffer, buffer+len, strlen(buffer) - len + 1);

		s[pos - s] = '\0';
		strcat(s, replace);

		strcat(s, buffer);
	}
	free(buffer);
}

void loadLv2Effect(song *s, window *w, int index, const LilvPlugin *plugin)
{
	effect *le = &s->effectv[index];
	/* TODO: stop using the effect before freeing */
	if (le->type) /* unload any current effect */
	{
		lv2control *lc;

		for (uint32_t i = 0; i < le->controlc; i++)
		{
			lc = &le->controlv[i];
			free(lc->format); lc->format = NULL;
			if (lc->enumerate)
			{
				free(lc->scalelabel); lc->scalelabel = NULL;
				free(lc->scalevalue); lc->scalevalue = NULL;
			}
		}

		free(le->controlv); le->controlv = NULL;
		free(le->audiov); le->audiov = NULL;
		for (uint8_t i = 1; i < s->instrumentc; i++)
			lilv_instance_free(s->instrumentv[i]->plugininstance[index]);
	}

	LilvInstance *li = lilv_plugin_instantiate(plugin, samplerate, NULL);
	if (li == NULL)
	{
		strcpy(w->command.error, "plugin failed to instantiate");
		s->effectv[index].type = 0;
		return;
	}
	if (s->instrumentc > 1) /* any instruments at all */
		s->instrumentv[1]->plugininstance[index] = li;
	for (uint8_t i = 2; i < s->instrumentc; i++)
		s->instrumentv[i]->plugininstance[index] = lilv_plugin_instantiate(plugin, samplerate, NULL);

	s->effectv[index].type = 2;
	le->plugin = plugin;

	le->controlv = calloc(lilv_plugin_get_num_ports_of_class(plugin, lv2.controlport, lv2.inputport, NULL), sizeof(lv2control));
	le->audiov =   calloc(lilv_plugin_get_num_ports_of_class(plugin, lv2.audioport, NULL), sizeof(lv2control));

	uint32_t nports = lilv_plugin_get_num_ports(plugin);
	/* default values for all ports */
	float *min, *max, *def;
	min = calloc(nports, sizeof(float));
	max = calloc(nports, sizeof(float));
	def = calloc(nports, sizeof(float));
	lilv_plugin_get_port_ranges_float(plugin, min, max, def);

	LilvNode *n;

	n = lilv_plugin_get_name(plugin);
	le->name = lilv_node_as_string(n);
	lilv_node_free(n);

	LilvPort *lport;
	lv2control *lc;
	s->effectv[index].indexc = le->controlc = le->audioc = 0;
	le->inputc = le->outputc = 0;
	for (uint32_t i = 0; i < nports; i++)
	{
		lport = (LilvPort *)lilv_plugin_get_port_by_index(plugin, i);
		if (lilv_port_is_a(plugin, lport, lv2.controlport))
		{
			if (lilv_port_is_a(plugin, lport, lv2.inputport))
			{
				lc = &le->controlv[le->controlc];
				lc->index = i;

				lc->min = min[i];
				lc->max = max[i];
				lc->value = def[i];
				for (uint8_t j = 1; j < s->instrumentc; j++)
					lilv_instance_connect_port(s->instrumentv[j]->plugininstance[index], lc->index, &lc->value);

				n = lilv_port_get_name(plugin, lport);
				lc->name = (char *)lilv_node_as_string(n);
				lilv_node_free(n);

				LilvNode *render, *unit;
				LilvNodes *units = lilv_port_get_value(plugin, lport, lv2.unit);
				if (lilv_nodes_size(units) > 0)
				{
					unit = lilv_nodes_get_first(units);
					render = lilv_world_get(lv2.world, unit, lv2.render, NULL);
					if (render)
					{
						lc->format = malloc(strlen((char *)lilv_node_as_string(render)));
						strcpy(lc->format, (char *)lilv_node_as_string(render));
					} else
					{
						lc->format = malloc(strlen("%f"));
						strcpy(lc->format, "%f");
					}
					lilv_node_free(render);
				} else
				{
					lc->format = malloc(strlen("%f"));
					strcpy(lc->format, "%f");
				}
				lilv_nodes_free(units);

				lc->integer = lilv_port_has_property(plugin, lport, lv2.integer);
				if (lc->integer) strrep(lc->format, "%f", "%d");
				lc->toggled = lilv_port_has_property(plugin, lport, lv2.toggled);
				lc->samplerate = lilv_port_has_property(plugin, lport, lv2.samplerate);
				lc->logarithmic = lilv_port_has_property(plugin, lport, lv2.logarithmic);

				n = lilv_port_get(plugin, lport, lv2.rangesteps);
				lc->steps = n ? (int)lilv_node_as_float(n) : 64.0;
				lilv_nodes_free(n);

				if (lilv_port_has_property(plugin, lport, lv2.enumeration))
				{
					LilvScalePoints *points = lilv_port_get_scale_points(plugin, lport);
					lc->enumerate = lilv_scale_points_size(points);
					lc->scalelabel = calloc(lc->enumerate, sizeof(char[MAX_VALUE_LEN]));
					lc->scalevalue = calloc(lc->enumerate, sizeof(float));

					unsigned int c = 0;
					LILV_FOREACH(scale_points, i, points)
					{
						LilvScalePoint *p = (LilvScalePoint *)lilv_scale_points_get(points, i);
						LilvNode *l = (LilvNode *)lilv_scale_point_get_label(p);
						LilvNode *v = (LilvNode *)lilv_scale_point_get_value(p);

						if (l && (lilv_node_is_float(v) || lilv_node_is_int(v))) {
							strcpy(lc->scalelabel[c], lilv_node_as_string(l));
							lc->scalevalue[c] = lilv_node_as_float(v);
						}

						c++;
					}
				}

				s->effectv[index].indexc++;
				le->controlc++;
			}
		} else // audio port
		{
			lv2audio *la = &le->audiov[le->audioc];
			la->index = i;
			if (lilv_port_is_a(plugin, lport, lv2.inputport))
			{
				la->type = LV2_TYPE_INPUT;
				switch (le->inputc)
				{
					case 0: /* left channel */
						for (uint8_t j = 1; j < s->instrumentc; j++)
							lilv_instance_connect_port(s->instrumentv[j]->plugininstance[index], la->index, s->effectinl);
						break;
					case 1: /* right channel */
						for (uint8_t j = 1; j < s->instrumentc; j++)
							lilv_instance_connect_port(s->instrumentv[j]->plugininstance[index], la->index, s->effectinr);
						break;
				}
				le->inputc++;
			} else
			{
				la->type = LV2_TYPE_OUTPUT;
				switch (le->outputc)
				{
					case 0: /* left channel */
						for (uint8_t j = 1; j < s->instrumentc; j++)
							lilv_instance_connect_port(s->instrumentv[j]->plugininstance[index], la->index, s->effectoutl);
						break;
					case 1: /* right channel */
						for (uint8_t j = 1; j < s->instrumentc; j++)
							lilv_instance_connect_port(s->instrumentv[j]->plugininstance[index], la->index, s->effectoutr);
						break;
				}
				le->outputc++;
			}

			le->audioc++;
		}
	}
}

void _addChannel(channel *cv)
{
	cv->rampmax = samplerate / 1000 * RAMP_MS;
	cv->rampindex = cv->rampmax;
	cv->rampbuffer = malloc(sizeof(float) * cv->rampmax * 2); /* *2 for stereo */
}
int addChannel(song *s, uint8_t index)
{
	_addChannel(&s->channelv[s->channelc]); /* allocate memory */
	if (s->channelc > 0) /* contiguity */
		for (uint8_t i = s->channelc; i >= index; i--)
		{
			for (uint8_t p = 1; p < s->patternc; p++)
				memcpy(s->patternv[p]->rowv[i + 1],
					s->patternv[p]->rowv[i],
					sizeof(row) * 256);
			s->channelv[i + 1].mute = s->channelv[i].mute;
		}

	s->channelv[index].mute = 0;

	for (uint8_t p = 1; p < s->patternc; p++)
	{
		memset(s->patternv[p]->rowv[index], 0, sizeof(row) * 256);
		for (short r = 0; r < 256; r++) s->patternv[p]->rowv[index][r].inst = 255;
	}

	s->channelc++;
	return 0;
}
int delChannel(song *s, uint8_t index)
{
	uint8_t i;
	uint8_t p;
	/* if there's only one channel then clear it */
	if (s->channelc == 1)
	{
		for (p = 1; p < s->patternc; p++)
		{
			memset(s->patternv[p]->rowv,
				0, sizeof(row) * 256);
			for (short r = 0; r < 256; r++)
				s->patternv[p]->rowv[0][r].inst = 255;
		}
		s->channelv[s->channelc].mute = 0;
	} else
	{
		free(s->channelv[index].rampbuffer);
		s->channelv[index].rampbuffer = NULL;

		for (i = index; i < s->channelc; i++)
		{
			for (p = 1; p < s->patternc; p++)
				memcpy(s->patternv[p]->rowv[i],
					s->patternv[p]->rowv[i + 1],
					sizeof(row) * 256);
			memcpy(&s->channelv[i],
				&s->channelv[i + 1],
				sizeof(channel));
		}

		s->channelc--;
	}
	return 0;
}
int yankChannel(song *s, uint8_t index)
{
	for (uint8_t i = 0; i < s->patternc; i++)
		if (s->channelbuffer[i])
		{
			DEBUG=8;
			memcpy(s->channelbuffer[i],
				s->patternv[i]->rowv[index],
				sizeof(row) * 256);
		}
	s->channelbuffermute = s->channelv[index].mute;
	return 0;
}
int putChannel(song *s, uint8_t index)
{
	for (uint8_t i = 0; i < s->patternc; i++)
		if (s->channelbuffer[i])
			memcpy(s->patternv[i]->rowv[index],
				s->channelbuffer[i],
				sizeof(row) * 256);
	s->channelv[index].mute = s->channelbuffermute;
	return 0;
}





/* memory for addPattern */
int _addPattern(song *s, uint8_t realindex)
{
	s->channelbuffer[realindex] = calloc(256, sizeof(row));
	s->patternv[realindex] = calloc(1, sizeof(pattern));
	if (!s->channelbuffer[realindex] || !s->patternv[realindex])
		return 1;
	for (short c = 0; c < 64; c++)
		for (short r = 0; r < 256; r++)
			s->patternv[realindex]->rowv[c][r].inst = 255;
	return 0;
}
/* length 0 for the default */
int addPattern(song *s, window *w, uint8_t index, uint8_t length)
{
	if (index == 255) return 1; /* index invalid */
	if (s->patterni[index] > 0) return 1; /* index occupied */

	if (_addPattern(s, s->patternc))
	{
		strcpy(w->command.error, "failed to add pattern, out of memory");
		return 1;
	}

	if (length > 0) s->patternv[s->patternc]->rowc = length;
	else            s->patternv[s->patternc]->rowc = s->defpatternlength;

	s->patterni[index] = s->patternc;
	s->patternc++;
	return 0;
}
int yankPattern(song *s, uint8_t index)
{
	if (s->patterni[index] == 0) return 1; /* nothing to yank */

	memcpy(&s->patternbuffer,
		s->patternv[s->patterni[index]],
		sizeof(pattern));
	return 0;
}
int putPattern(song *s, window *w, uint8_t index)
{
	if (s->patterni[index] == 0)
		if (addPattern(s, w, index, 0)) return 1; /* allocate memory */

	memcpy(s->patternv[s->patterni[index]],
		&s->patternbuffer,
		sizeof(pattern));

	if (s->patternv[s->patterni[index]]->rowc == 0)
		s->patternv[s->patterni[index]]->rowc = s->defpatternlength;
	return 0;
}
int delPattern(song *s, uint8_t index)
{
	if (s->patternc <= 1) return 1; /* no patterns to remove */
	if (!s->patterni[index]) return 1; /* pattern doesn't exist */
	uint8_t cutIndex = s->patterni[index];

	free(s->patternv[cutIndex]); s->patternv[cutIndex] = NULL;
	free(s->channelbuffer[cutIndex]); s->channelbuffer[cutIndex] = NULL;

	s->patterni[index] = 0;

	/* enforce contiguity */
	unsigned short i;
	for (i = cutIndex; i < s->patternc - 1; i++)
	{
		s->patternv[i] = s->patternv[i + 1];
		s->channelbuffer[i] = s->channelbuffer[i + 1];
	}

	for (i = 0; i < 256; i++) // for every backref index
		if (s->patterni[i] > cutIndex && s->patterni[i] != 0)
			s->patterni[i]--;

	s->patternc--;
	return 0;
}





int changeInstrumentType(song *s, window *w, typetable *t, uint8_t forceindex)
{
	uint8_t i;
	if (!forceindex)
		if (w->instrumentlockv == INST_GLOBAL_LOCK_FREE)
			i = w->instrumentlocki;
		else return 0;
	else i = forceindex;

	if (!forceindex)
		w->instrumentlockv = INST_GLOBAL_LOCK_OK; // mark as free to use


	instrument *iv = s->instrumentv[i];
	iv->typefollow = iv->type;
	if (iv->state) { free(iv->state); iv->state = NULL; } // free old state
	if (iv && iv->type < INSTRUMENT_TYPE_COUNT && t->f[iv->type].changeType)
	{
		t->f[iv->type].changeType(&iv->state);

		if (!iv->state)
		{
			strcpy(w->command.error, "failed to change instrument type, out of memory");
			return 1;
		}
	}

	return 0;
}

int _addInstrument(song *s, uint8_t realindex)
{
	s->instrumentv[realindex] = calloc(1, sizeof(instrument));
	if (s->instrumentv[realindex] == NULL) return 1;
	return 0;
}
void _instantiateInstrumentEffect(song *s, uint8_t realindex)
{
	for (int i = 0; i < 16; i++)
	{
		if (s->effectv[i].type > 0)
		{
			effect *le = &s->effectv[i];

			s->instrumentv[realindex]->plugininstance[i] =
				lilv_plugin_instantiate(le->plugin, samplerate, NULL);

			lv2control *lc;
			for (int j = 0; j < le->controlc; j++)
			{
				lc = &le->controlv[j];
				lilv_instance_connect_port(s->instrumentv[realindex]->plugininstance[i], lc->index, &lc->value);
			}

			int ins = 0, outs = 0;
			lv2audio *la;
			for (int j = 0; j < le->audioc; j++)
			{
				la = &le->audiov[j];
				if (la->type == LV2_TYPE_OUTPUT)
				{
					switch (ins)
					{
						case 0: /* left channel */
							lilv_instance_connect_port(s->instrumentv[realindex]->plugininstance[i], j, s->effectinl);
							break;
						case 1: /* right channel */
							lilv_instance_connect_port(s->instrumentv[realindex]->plugininstance[i], j, s->effectinr);
							break;
					}
					ins++;
				} else
				{
					switch (outs)
					{
						case 0: /* left channel */
							lilv_instance_connect_port(s->instrumentv[realindex]->plugininstance[i], j, s->effectoutl);
							break;
						case 1: /* right channel */
							lilv_instance_connect_port(s->instrumentv[realindex]->plugininstance[i], j, s->effectoutr);
							break;
					}
					outs++;
				}
			}
		}
	}
}
int addInstrument(song *s, window *w, typetable *t, uint8_t index)
{
	if (s->instrumenti[index] > 0) return 1; /* index occupied */

	if (_addInstrument(s, s->instrumentc))
	{
		strcpy(w->command.error, "failed to add instrument, out of memory");
		return 1;
	}
	s->instrumentv[s->instrumentc]->fader[0] = 0xFF;
	s->instrumentv[s->instrumentc]->fader[1] = 0xFF;
	changeInstrumentType(s, w, t, s->instrumentc);
	sampler_state *ss = s->instrumentv[s->instrumentc]->state;
	ss->volume.s = 255;
	s->instrumenti[index] = s->instrumentc;

	_instantiateInstrumentEffect(s, s->instrumentc);

	s->instrumentc++;
	return 0;
}
/* return a fresh slot */
uint8_t newInstrument(song *s, uint8_t minindex)
{
	for (uint8_t i = minindex; i < 256; i++) // is 256 right? idfk
	{
		if (s->instrumenti[i] == 0)
		{
			return i;
		}
	}
}
int yankInstrument(song *s, window *w, uint8_t index)
{
	if (s->instrumenti[index] == 0) return 1; /* nothing to yank */

	if (s->instrumentbuffer.samplelength > 0)
		free(s->instrumentbuffer.sampledata);
	if (s->instrumentbuffer.state)
	{ free(s->instrumentbuffer.state); s->instrumentbuffer.state = NULL; }

	memcpy(&s->instrumentbuffer,
		s->instrumentv[s->instrumenti[index]],
		sizeof(instrument));

	if (s->instrumentv[s->instrumenti[index]]->samplelength > 0)
	{
		s->instrumentbuffer.sampledata =
			malloc(s->instrumentv[s->instrumenti[index]]->samplelength);

		if (s->instrumentbuffer.sampledata == NULL)
		{
			strcpy(w->command.error, "failed to yank instrument, out of memory");
			return 1;
		}

		memcpy(s->instrumentbuffer.sampledata,
			s->instrumentv[s->instrumenti[index]]->sampledata,
			s->instrumentv[s->instrumenti[index]]->samplelength);
	}

	if (s->instrumentv[s->instrumenti[index]]->state != NULL)
	{
		switch (s->instrumentbuffer.type)
		{
			case 0: s->instrumentbuffer.state = malloc(sizeof(sampler_state)); break;
		}

		if (!s->instrumentbuffer.state)
		{
			free(s->instrumentbuffer.sampledata);
			strcpy(w->command.error, "failed to yank instrument, out of memory");
			return 1;
		}

		switch (s->instrumentbuffer.type)
		{
			case 0:
				memcpy(s->instrumentbuffer.state,
					s->instrumentv[s->instrumenti[index]]->state,
					sizeof(sampler_state));
				break;
		}
	}
	return 0;
}
int putInstrument(song *s, window *w, typetable *t, uint8_t index)
{
	if (s->instrumenti[index] == 0)
	{
		if (addInstrument(s, w, t, index)) /* allocate memory */
		{
			strcpy(w->command.error, "failed to put instrument, out of memory");
			return 1;
		}
	}
	if (w->instrumentreci == s->instrumenti[index] && w->instrumentrecv > INST_REC_LOCK_OK) return 1;

	if (s->instrumentv[s->instrumenti[index]]->samplelength > 0)
	{
		free(s->instrumentv[s->instrumenti[index]]->sampledata);
		s->instrumentv[s->instrumenti[index]]->sampledata = NULL;
	}

	memcpy(s->instrumentv[s->instrumenti[index]],
		&s->instrumentbuffer,
		sizeof(instrument));

	if (s->instrumentbuffer.samplelength > 0)
	{
		s->instrumentv[s->instrumenti[index]]->sampledata =
			malloc(s->instrumentbuffer.samplelength);

		if (s->instrumentv[s->instrumenti[index]]->sampledata == NULL)
		{
			strcpy(w->command.error, "failed to put instrument, out of memory");
			return 1;
		}

		memcpy(s->instrumentv[s->instrumenti[index]]->sampledata,
			s->instrumentbuffer.sampledata,
			s->instrumentbuffer.samplelength);
	}

	_instantiateInstrumentEffect(s, index);

	return 0;
}
int delInstrument(song *s, uint8_t index)
{
	if (s->instrumenti[index] < 1) return 1; /* instrument doesn't exist */

	uint8_t cutIndex = s->instrumenti[index];
	s->instrumenti[index] = 0;

	/* free the sample data */
	if (s->instrumentv[cutIndex]->state)
		free(s->instrumentv[cutIndex]->state);
	if (s->instrumentv[cutIndex]->samplelength > 0)
		free(s->instrumentv[cutIndex]->sampledata);

	for (int i = 0; i < 16; i++)
	{
		if (s->effectv[i].type > 0) switch (s->effectv[i].type)
		{
			case 2:
				lilv_instance_free(s->instrumentv[cutIndex]->plugininstance[i]);
				break;
		}
	}

	free(s->instrumentv[cutIndex]);
	s->instrumentv[cutIndex] = NULL;

	/* enforce contiguity */
	uint8_t i;
	for (i = cutIndex; i < s->instrumentc - 1; i++)
		s->instrumentv[i] = s->instrumentv[i + 1];

	for (i = 0; i < s->instrumentc; i++) // for every backref index
		if (s->instrumenti[i] > cutIndex)
			s->instrumenti[i]--;

	s->instrumentc--;
	return 0;
}





short *_loadSample(char *path, SF_INFO *sfinfo)
{
	memset(sfinfo, 0, sizeof(SF_INFO));

	SNDFILE *sndfile = sf_open(path, SFM_READ, sfinfo);

	short *ptr;

	if (sndfile == NULL)
	{ /* raw file */
		struct stat buf;
		stat(path, &buf);

		ptr = malloc(buf.st_size - buf.st_size % sizeof(short));
		if (ptr == NULL) // malloc failed
			return NULL;

		/* read the whole file into memory */
		FILE *fp = fopen(path, "r");
		fread(ptr, sizeof(short), buf.st_size / sizeof(short), fp);
		fclose(fp);

		/* spoof data */
		sfinfo->channels = 1;
		sfinfo->frames = buf.st_size / sizeof(short);
		sfinfo->samplerate = 12000;
	} else /* audio file */
	{
		if (sfinfo->channels > 2) /* fail on high channel files */
		{
			sf_close(sndfile);
			return NULL;
		}

		ptr = malloc(sizeof(short) * sfinfo->frames * sfinfo->channels);
		if (ptr == NULL) // malloc failed
		{
			sf_close(sndfile);
			return NULL;
		}

		/* read the whole file into memory */
		sf_readf_short(sndfile, ptr, sfinfo->frames);

		sf_close(sndfile);
	}
	return ptr;
}
int loadSample(song *s, window *w, typetable *t, uint8_t index, char *path)
{
	if (s->instrumenti[index] == 0)
	{
		if (addInstrument(s, w, t, index)) /* allocate instrument memory */
		{
			strcpy(w->command.error, "failed to add instrument, out of memory");
			return 1;
		}
	}
	instrument *iv = s->instrumentv[s->instrumenti[index]];

	if (iv->type < INSTRUMENT_TYPE_COUNT && t->f[iv->type].loadSample != NULL)
	{
		SF_INFO sfinfo;
		short *sampledata = _loadSample(path, &sfinfo);
		if (!sampledata)
		{
			strcpy(w->command.error, "failed to load sample, out of memory");
			return 1;
		}

		/* unload any present sample data */
		if (iv->samplelength > 0)
			free(iv->sampledata);
		iv->sampledata = sampledata;
		iv->samplelength = sfinfo.frames * sfinfo.channels;

		t->f[iv->type].loadSample(iv, sfinfo);
		return 0;
	}
	return 1;
};
int exportSample(song *s, typetable *t, uint8_t index, char *path)
{
	if (s->instrumenti[index] < 0) return 1; /* instrument doesn't exist */
	if (s->instrumentv[s->instrumenti[index]]->samplelength < 1) return 1; /* no sample data */

	instrument *iv = s->instrumentv[s->instrumenti[index]];
	if (iv->type < INSTRUMENT_TYPE_COUNT && t->f[iv->type].exportSample != NULL)
	{
		SNDFILE *sndfile;
		SF_INFO sfinfo;
		memset(&sfinfo, 0, sizeof(sfinfo));

		t->f[iv->type].exportSample(iv, &sfinfo);

		sfinfo.format = SF_FORMAT_WAV | SF_FORMAT_PCM_16;
		sndfile = sf_open(path, SFM_WRITE, &sfinfo);
		if (sndfile == NULL) return 1;

		/* write the sample data to disk */
		sf_writef_short(sndfile, iv->sampledata, iv->samplelength);
		sf_close(sndfile);
		return 0;
	}
	return 1;
};





song *_addSong(song *s, window *w)
{
	s = calloc(1, sizeof(song));
	if (s == NULL)
		return NULL;

	s->patternc = 1;
	s->instrumentc = 1;

	s->rowhighlight = 4;
	s->defpatternlength = 63;
	s->songbpm = 125;
	w->request = REQ_BPM;

	memset(s->songi, 255, sizeof(uint8_t) * 256);

	s->effectinl = calloc(sizeof(float), buffersize);
	s->effectinr = calloc(sizeof(float), buffersize);
	s->effectoutl = calloc(sizeof(float), buffersize);
	s->effectoutr = calloc(sizeof(float), buffersize);
	if (!s->effectinl || !s->effectinr || !s->effectoutl || !s->effectoutr)
	{
		free(s);
		return NULL;
	}

	return s;
}
song *addSong(song *s, window *w)
{
	s = _addSong(s, w);
	if (s == NULL) return NULL;
	addChannel(s, s->channelc);
	addChannel(s, s->channelc);
	addChannel(s, s->channelc);
	addChannel(s, s->channelc);
	return s;
}

void delSong(song *s)
{
	free(s->effectinl);
	free(s->effectinr);
	free(s->effectoutl);
	free(s->effectoutr);

	for (int i = 0; i < s->channelc; i++)
		free(s->channelv[i].rampbuffer);

	for (int i = 1; i < s->patternc; i++)
		free(s->patternv[i]);

	for (int i = 1; i < s->instrumentc; i++)
	{
		if (s->instrumentv[i]->samplelength > 0)
			free(s->instrumentv[i]->sampledata);
		if (s->instrumentv[i]->state)
			free(s->instrumentv[i]->state);

		for (int j = 0; j < 16; j++)
		{
			if (s->effectv[j].type > 0) switch (s->effectv[j].type)
			{
				case 2:
					lilv_instance_free(s->instrumentv[i]->plugininstance[j]);
					break;
			}
		}

		free(s->instrumentv[i]);
	}

	for (int i = 0; i < 16; i++)
		if (s->effectv[i].type)
		{
			effect *le = &s->effectv[i];
			lv2control *lc;

			if (le->controlc > 0) for (uint32_t j = 0; j < le->controlc; j++)
			{
				lc = &le->controlv[j];
				free(lc->format);
				if (lc->enumerate)
				{
					free(lc->scalelabel);
					free(lc->scalevalue);
				}
			}

			free(le->controlv);
			free(le->audiov);
		}

	free(s);
}

int writeSong(song *s, window *w, char *path)
{
	if (!strcmp(path, ""))
	{
		if (!strlen(w->filepath))
		{
			strcpy(w->command.error, "No file name");
			return 1;
		}
		strcpy(path, w->filepath);
	} else strcpy(w->filepath, path);

	FILE *fp = fopen(path, "wb");
	int i, j, k;

	/* egg (the most important) */
	fputc('e', fp);
	fputc('g', fp);
	fputc('g', fp);
	fseek(fp, 0x4, SEEK_SET);

	/* counts */
	fwrite(&s->songbpm, sizeof(uint16_t), 1, fp);
	fputc(s->patternc, fp);
	fputc(s->instrumentc, fp);
	fputc(s->channelc, fp);
	fputc(s->rowhighlight, fp);

	fseek(fp, 0x10, SEEK_SET);
	/* mutes */
	char byte;
	for (i = 0; i < 8; i++)
	{
		byte = 0b00000000;
		if (s->channelv[i * 8 + 0].mute) byte |= 0b10000000;
		if (s->channelv[i * 8 + 1].mute) byte |= 0b01000000;
		if (s->channelv[i * 8 + 2].mute) byte |= 0b00100000;
		if (s->channelv[i * 8 + 3].mute) byte |= 0b00010000;
		if (s->channelv[i * 8 + 4].mute) byte |= 0b00001000;
		if (s->channelv[i * 8 + 5].mute) byte |= 0b00000100;
		if (s->channelv[i * 8 + 6].mute) byte |= 0b00000010;
		if (s->channelv[i * 8 + 7].mute) byte |= 0b00000001;
		fputc(byte, fp);
	}

	fseek(fp, 0x20, SEEK_SET);
	/* songi */
	for (i = 0; i < 256; i++)
		fputc(s->songi[i], fp);
	/* songa */
	for (i = 0; i < 256; i++)
		fputc(s->songa[i], fp);
	/* patterni */
	for (i = 0; i < 256; i++)
		fputc(s->patterni[i], fp);
	/* instrumenti */
	for (i = 0; i < 256; i++)
		fputc(s->instrumenti[i], fp);

	/* patternv */
	for (i = 1; i < s->patternc; i++)
	{
		fputc(s->patternv[i]->rowc, fp);
		for (j = 0; j < s->channelc; j++)
			for (k = 0; k < s->patternv[i]->rowc + 1; k++)
			{
				fputc(s->patternv[i]->rowv[j][k].note, fp);
				fputc(s->patternv[i]->rowv[j][k].inst, fp);
				fputc(s->patternv[i]->rowv[j][k].macroc[0], fp);
				fputc(s->patternv[i]->rowv[j][k].macroc[1], fp);
				fputc(s->patternv[i]->rowv[j][k].macrov[0], fp);
				fputc(s->patternv[i]->rowv[j][k].macrov[1], fp);
			}
	}

	/* instrumentv */
	for (i = 1; i < s->instrumentc; i++)
	{
		fputc(s->instrumentv[i]->type, fp);

		if (s->instrumentv[i]->type < INSTRUMENT_TYPE_COUNT
				&& t->f[s->instrumentv[i]->type].write != NULL)
			t->f[s->instrumentv[i]->type].write(s->instrumentv[i], fp);

		fputc(s->instrumentv[i]->fader[0], fp);
		fputc(s->instrumentv[i]->fader[1], fp);
		fwrite(&s->instrumentv[i]->send, sizeof(char), 16, fp);
		fwrite(&s->instrumentv[i]->samplelength, sizeof(uint32_t), 1, fp);
		if (s->instrumentv[i]->samplelength > 0)
			fwrite(s->instrumentv[i]->sampledata, sizeof(short), s->instrumentv[i]->samplelength, fp);
	}
	fclose(fp);

	snprintf(w->command.error, COMMAND_LENGTH, "'%s' written", path);
	return 0;
}

song *readSong(song *s, window *w, typetable *t, char *path)
{
	FILE *fp = fopen(path, "rb");
	if (fp == NULL) // file doesn't exist, or fopen otherwise failed
	{
		snprintf(w->command.error, COMMAND_LENGTH, "File '%s' doesn't exist", path);
		return NULL;
	}

	int i, j, k;

	/* ensure egg wasn't broken in transit */
	/* the most important check */
	if (!(fgetc(fp) == 'e' && fgetc(fp) == 'g' && fgetc(fp) == 'g'))
	{
		snprintf(w->command.error, COMMAND_LENGTH, "File '%s' isn't valid", path);
		return NULL;
	}
	fseek(fp, 0x4, SEEK_SET);

	if (s != NULL) delSong(s);

	s = _addSong(s, w);
	if (s == NULL)
	{
		strcpy(w->command.error, "failed to read song, out of memory");
		return NULL;
	}

	/* assume the rest of the file is valid */
	/* TODO: proper error checking lol */
	strcpy(w->filepath, path);

	/* counts */
	fread(&s->songbpm, sizeof(uint16_t), 1, fp);
	w->request = REQ_BPM;

	s->patternc = fgetc(fp);
	s->instrumentc = fgetc(fp);
	s->channelc = fgetc(fp);
	for (int i = 0; i < s->channelc; i++)
		_addChannel(&s->channelv[i]);
	s->rowhighlight = fgetc(fp);

	fseek(fp, 0x10, SEEK_SET);
	/* mutes */
	char byte;
	for (i = 0; i < 8; i++)
	{
		byte = fgetc(fp);
		if (byte & 0b10000000) s->channelv[i * 8 + 0].mute = 1;
		if (byte & 0b01000000) s->channelv[i * 8 + 1].mute = 1;
		if (byte & 0b00100000) s->channelv[i * 8 + 2].mute = 1;
		if (byte & 0b00010000) s->channelv[i * 8 + 3].mute = 1;
		if (byte & 0b00001000) s->channelv[i * 8 + 4].mute = 1;
		if (byte & 0b00000100) s->channelv[i * 8 + 5].mute = 1;
		if (byte & 0b00000010) s->channelv[i * 8 + 6].mute = 1;
		if (byte & 0b00000001) s->channelv[i * 8 + 7].mute = 1;
		fputc(byte, fp);
	}

	fseek(fp, 0x20, SEEK_SET);
	/* songi */
	for (i = 0; i < 256; i++)
		s->songi[i] = fgetc(fp);
	/* songa */
	for (i = 0; i < 256; i++)
		s->songa[i] = fgetc(fp);
	/* patterni */
	for (i = 0; i < 256; i++)
		s->patterni[i] = fgetc(fp);
	/* instrumenti */
	for (i = 0; i < 256; i++)
		s->instrumenti[i] = fgetc(fp);

	/* patternv */
	for (i = 1; i < s->patternc; i++)
	{
		_addPattern(s, i);
		s->patternv[i]->rowc = fgetc(fp);
		for (j = 0; j < s->channelc; j++)
			for (k = 0; k < s->patternv[i]->rowc + 1; k++)
			{
				s->patternv[i]->rowv[j][k].note = fgetc(fp);
				s->patternv[i]->rowv[j][k].inst = fgetc(fp);
				s->patternv[i]->rowv[j][k].macroc[0] = fgetc(fp);
				s->patternv[i]->rowv[j][k].macroc[1] = fgetc(fp);
				s->patternv[i]->rowv[j][k].macrov[0] = fgetc(fp);
				s->patternv[i]->rowv[j][k].macrov[1] = fgetc(fp);
			}
	}

	/* instrumentv */
	for (i = 1; i < s->instrumentc; i++)
	{
		_addInstrument(s, i);

		s->instrumentv[i]->type = fgetc(fp);
		changeInstrumentType(s, w, t, i);

		if (s->instrumentv[i]->type < INSTRUMENT_TYPE_COUNT
				&& t->f[s->instrumentv[i]->type].read != NULL)
			t->f[s->instrumentv[i]->type].read(s->instrumentv[i], fp);

		s->instrumentv[i]->fader[0] = fgetc(fp);
		s->instrumentv[i]->fader[1] = fgetc(fp);
		fread(&s->instrumentv[i]->send, sizeof(char), 16, fp);
		fread(&s->instrumentv[i]->samplelength, sizeof(uint32_t), 1, fp);
		if (s->instrumentv[i]->samplelength > 0)
		{
			short *sampledata = malloc(sizeof(short) * s->instrumentv[i]->samplelength);
			if (sampledata == NULL) // malloc failed
			{
				strcpy(w->command.error, "some sample data failed to load, out of memory");
				continue;
			}
			fread(sampledata, sizeof(short), s->instrumentv[i]->samplelength, fp);
			s->instrumentv[i]->sampledata = sampledata;
		}
	}
	fclose(fp);
	return s;
}
