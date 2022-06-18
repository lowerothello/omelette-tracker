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
	char      mute;              /* saved to disk */
	uint32_t  samplepointer;     /* progress through the sample */
	uint32_t  sampleoffset;      /* point to base samplepointer off of */
	uint32_t  releasepointer;    /* 0 for no release, where releasing started */
	uint8_t   gain;              /* two 4bit uints, one for each channel */
	row       r;
	float     cents;             /* 1 fractional semitone, used for portamento, always between -0.5 and +0.5 */
	uint8_t   portamento;        /* portamento target, 255 for off */
	uint8_t   portamentospeed;   /* portamento m */
	uint16_t  rtrigsamples;      /* samples per retrigger */
	uint32_t  rtrigpointer;      /* sample ptr to ratchet back to */
	uint8_t   rtrigblocksize;    /* number of rows block extends to */
	uint8_t   effectholdinst;    /* 255 for no hold */
	uint8_t   effectholdindex;
	uint32_t  cutsamples;        /* samples into the row to cut, 0 for no cut */
	uint32_t  delaysamples;      /* samples into the row to delay, 0 for no delay */
	uint8_t   delaynote;
	uint8_t   delayinst;

	uint16_t  rampindex;         /* progress through the ramp buffer, rampmax if not ramping */
	uint16_t  rampmax;           /* length of the ramp buffer */
	sample_t *rampbuffer;        /* samples to ramp out */
	uint8_t   rampinstrument;    /* old instrument, realindex */
	uint8_t   rampgain;          /* old gain */

	uint16_t  stretchrampindex;  /* progress through the ramp buffer, rampmax if not ramping */
	uint16_t  stretchrampmax;    /* length of the ramp buffer */
	sample_t *stretchrampbuffer; /* samples to ramp out */
	uint32_t  cycleoffset;

	float     ln_1, ln_2;        /* previous samples, used by the filter */
	float     rn_1, rn_2;
	struct
	{
		float osc1phase;         /* stored phase, for freewheel oscillators */
		float osc2phase;
		float subphase;
		float lfophase;
	} analogue[33];             /* one for each potential instance */
} channel;

typedef struct Instrument
{
	uint8_t             type;
	uint8_t             typefollow;                   /* follows the type, set once state is guaranteed to be mallocced */
	short              *sampledata;                   /* variable size, persists between types */
	uint32_t            samplelength;                 /* raw samples allocated for sampledata */
	void               *state[INSTRUMENT_TYPE_COUNT]; /* type working memory */
	uint8_t             fader;
	char                send[16];                     /* set to [0-15] */
	char                processsend[16];              /* used during playback only */
	LilvInstance       *plugininstance[16];           /* pointer to lv2 instances */
	sample_t           *outbufferl;
	sample_t           *outbufferr;
	struct Instrument  *history[128];
	uint8_t             historyptr;                   /* highest bit is an overflow bit */
	uint8_t             historybehind;                /* tracks how many less than 128 safe indices there are */
	uint8_t             historyahead;                 /* tracks how many times it's safe to redo */
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
	unsigned char     type;            /* 0:empty, 2:lv2 */
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
	uint8_t         patternc;                /* pattern count */
	uint8_t         patterni[256];           /* pattern backref */
	pattern        *patternv[256];           /* pattern values */
	pattern         songbuffer;              /* full pattern paste buffer, TODO: use */
	pattern         patternbuffer;           /* partial pattern paste buffer */
	short           pbfy[2], pbfx[2];        /* partial pattern paste buffer clipping region */
	uint8_t         pbchannel[2];            /* " */
	char            pbpopulated;             /* there's no good way to tell if pb is set */

	uint8_t         instrumentc;             /* instrument count */
	uint8_t         instrumenti[256];        /* instrument backref */
	instrument     *instrumentv[256];        /* instrument values */
	instrument      instrumentbuffer;        /* instrument paste buffer */

	effect          effectv[16];             /* effect values */
	sample_t       *effectinl, *effectinr;   /* effect inputs */
	sample_t       *effectoutl, *effectoutr; /* effect outputs */
	sample_t       *effectdryl, *effectdryr; /* effect dry */
	unsigned char   effectbuffertype;        /* effect paste buffer */
	const LilvNode *effectbufferlv2uri;
	float          *effectbufferlv2values;

	uint8_t         channelc;                /* channel count */
	channel         channelv[64];            /* channel values */
	row            *channelbuffer[64];       /* channel paste buffer */
	char            channelbuffermute;

	uint8_t         songi[256];              /* song list backref, links to patterns */
	uint8_t         songa[256];              /* song list attributes */

	uint8_t         songp;                   /* song pos, analogous to window->songfx */
	short           songr;                   /* song row, analogous to window->trackerfy */

	uint8_t         rowhighlight;
	uint8_t         defpatternlength;        /* only here cos window isn't defined yet */
	uint8_t         bpm;
	uint8_t         songbpm;                 /* to store the song's bpm through bpm change macros */
	uint32_t        spr;                     /* samples per row (samplerate * (60 / bpm) / 4) */
	uint32_t        sprp;                    /* samples per row progress */
	char            playing;
} song;
song *s;


#define INST_GLOBAL_LOCK_OK 0        /* playback and most memory ops are safe */
#define INST_GLOBAL_LOCK_PREP_FREE 1 /* playback unsafe, preparing to free the state */
#define INST_GLOBAL_LOCK_FREE 2      /* playback has stopped, safe to free the state */
#define INST_GLOBAL_LOCK_PREP_HIST 3 /* playback unsafe, preparing to restore history */
#define INST_GLOBAL_LOCK_HIST 4      /* playback has stopped, restoring history */
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
	uint8_t        pattern;                     /* focused pattern */
	uint8_t        channel;                     /* focused channel */
	uint8_t        channeloffset, visiblechannels;

	short          trackerfy, trackerfx;
	short          visualfy, visualfx;
	uint8_t        visualchannel;
	unsigned short trackercelloffset;
	short          instrumentindex;
	uint8_t        instrument;                  /* focused instrument */
	unsigned short instrumentcelloffset;
	short          instrumentsend;              /* focused send */

	short          effectindex;
	short          pluginindex;
	short          effectoffset;                /* scroll offset for pluginindex */
	unsigned char  effect;                      /* focused effect */
	char         (*pluginlist)[INSTRUMENT_TYPE_COLS - 2];

	unsigned short mousey, mousex;

	short          fyoffset;
	signed char    fieldpointer;

	char           dirpath[NAME_MAX + 1];
	unsigned int   dirc;
	unsigned short dirmaxwidth;
	unsigned char  dircols;
	DIR           *dir;

	uint8_t        songfx;
	uint8_t        songoffset, songvisible;
	unsigned short songcelloffset;

	unsigned short instrumentrowoffset;

	char           octave;
	char           step;

	unsigned short akaizertimefactor, akaizercyclelength;

	uint8_t        songnext;

	uint8_t        previewnote, previewinst;
	uint8_t        previewchannel;
	instrument     previewinstrument;           /* used by the file browser */
	char           previewtrigger;              /* 0:cut
	                                               1:start inst
	                                               2:still inst
	                                               3:start sample
	                                               4:still sample
	                                               5:prep volatile */

	char           previewsamplestatus;

	uint8_t        instrumentlocki;             /* realindex */
	uint8_t        instrumentlockv;             /* value, set to an INST_GLOBAL_LOCK constant */

	uint8_t        instrumentreci;              /* realindex */
	uint8_t        instrumentrecv;              /* value, set to an INST_REC_LOCK constant */
	short         *recbuffer;                   /* disallow changing the type or removing while recording */
	uint32_t       recptr;

	char           request;                     /* ask the playback function to do something */

	char           newfilename[COMMAND_LENGTH]; /* used by readSong */
} window;
window *w;

typedef struct
{
	struct
	{
		unsigned short   indexc;               /* index count used (0 inclusive) */
		size_t           statesize;
		void           (*draw) (instrument *, uint8_t, unsigned short, unsigned short, short *, unsigned char);
		void           (*adjustUp)(instrument *, short);
		void           (*adjustDown)(instrument *, short);
		void           (*adjustLeft)(instrument *, short);
		void           (*adjustRight)(instrument *, short);
		void           (*incFieldPointer)(signed char *, short);
		void           (*decFieldPointer)(signed char *, short);
		void           (*endFieldPointer)(signed char *, short);
		void           (*mouseToIndex)(int, int, int, short *, signed char *);
		void           (*input)(int *);
		void           (*process)(instrument *, channel *, uint32_t, sample_t *, sample_t *);
		uint32_t       (*offset)(instrument *, channel *, int);
		uint8_t        (*getOffset)(instrument *, channel *);
		void           (*initType)(void **);
		void           (*write)(void **, FILE *fp);
		void           (*read)(void **, FILE *fp);
	} f[INSTRUMENT_TYPE_COUNT];
} typetable;
typetable *t;

typedef struct
{
	uint32_t length;
	char     channels;
	uint32_t c5rate;
	uint16_t cyclelength;
	uint32_t trim[2];
	uint32_t loop[2];
	adsr     volume;
	uint8_t  attributes; /* %1: fixed tempo */
} sampler_state;




typedef struct
{
	LilvWorld    *world;
	LilvPlugins  *plugins;
	unsigned int  pluginc; /* indexed plugin count */
	LilvNode     *inputport;
	LilvNode     *outputport;
	LilvNode     *audioport;
	LilvNode     *controlport;
	LilvNode     *integer;
	LilvNode     *toggled;
	LilvNode     *samplerate;
	LilvNode     *render;
	LilvNode     *unit;
	LilvNode     *enumeration;
	LilvNode     *logarithmic;
	LilvNode     *rangesteps;
} lilv;
lilv lv2;

/* replace *find with *replace in *s */
/* only replaces the first instance of *find */
void strrep(char *string, char *find, char *replace)
{
	char *buffer = malloc(strlen(string) + 1);
	char *pos = strstr(string, find);
	if (pos)
	{
		strcpy(buffer, string);
		size_t len = pos - string + strlen(find);
		memmove(buffer, buffer+len, strlen(buffer) - len + 1);

		string[pos - string] = '\0';
		strcat(string, replace);

		strcat(string, buffer);
	}
	free(buffer);
}

void _unloadLv2Effect(int index)
{
	effect *le = &s->effectv[index];
	if (le->type)
	{
		le->type = 0;
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
}
void loadLv2Effect(int index, const LilvPlugin *plugin)
{
	/* TODO: stop using the effect before freeing */
	_unloadLv2Effect(index);

	effect *le = &s->effectv[index];

	LilvInstance *li = lilv_plugin_instantiate(plugin, samplerate, NULL);
	if (!li)
	{
		strcpy(w->command.error, "plugin failed to instantiate");
		le->type = 0;
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
						lc->format = malloc(strlen(lilv_node_as_string(render)) + 1);
						strcpy(lc->format, lilv_node_as_string(render));
					} else
					{
						lc->format = malloc(strlen("%f") + 1);
						strcpy(lc->format, "%f");
					}
					lilv_node_free(render);
				} else
				{
					lc->format = malloc(strlen("%f") + 1);
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
	free(min);
	free(max);
	free(def);
}
void yankEffect(int index)
{
	effect *le = &s->effectv[index];
	switch (le->type)
	{
		case 0: /* nothing */
			s->effectbuffertype = 0;
			break;
		case 2: /* lv2 */
			s->effectbuffertype = 2;
			if (s->effectbufferlv2values)
				free(s->effectbufferlv2values);

			s->effectbufferlv2uri = lilv_plugin_get_uri(le->plugin);
			s->effectbufferlv2values = malloc(sizeof(float) * le->indexc);
			for (int i = 0; i < le->indexc; i++)
				s->effectbufferlv2values[i] = le->controlv[i].value;
			break;
	}
}
void putEffect(int index)
{
	switch (s->effectbuffertype)
	{
		case 0: /* nothing */
			_unloadLv2Effect(index);
			break;
		case 2: /* lv2 */
			loadLv2Effect(index, lilv_plugins_get_by_uri(lv2.plugins, s->effectbufferlv2uri));
			effect *le = &s->effectv[index];
			for (int i = 0; i < le->indexc; i++)
				le->controlv[i].value = s->effectbufferlv2values[i];
			break;
	}
}

void _addChannel(channel *cv)
{
	cv->rampmax = samplerate / 1000 * RAMP_MS;
	cv->rampindex = cv->rampmax;
	cv->rampbuffer = malloc(sizeof(sample_t) * cv->rampmax * 2); /* *2 for stereo */
	cv->stretchrampmax = samplerate / 1000 * TIMESTRETCH_RAMP_MS;
	cv->stretchrampindex = cv->stretchrampmax;
	cv->stretchrampbuffer = malloc(sizeof(sample_t) * cv->stretchrampmax * 2); /* *2 for stereo */
	for (int i = 0; i < 33; i++)
	{
		cv->analogue[i].osc1phase = 0.0;
		cv->analogue[i].osc2phase = 0.0;
		cv->analogue[i].subphase = 0.0;
		cv->analogue[i].lfophase = 0.0;
	}
}
int addChannel(song *cs, uint8_t index)
{
	_addChannel(&cs->channelv[cs->channelc]); /* allocate memory */
	if (cs->channelc > 0) /* contiguity */
		for (uint8_t i = cs->channelc; i >= index; i--)
		{
			for (uint8_t p = 1; p < cs->patternc; p++)
				memcpy(cs->patternv[p]->rowv[i + 1],
					cs->patternv[p]->rowv[i],
					sizeof(row) * 256);
			cs->channelv[i + 1].mute = cs->channelv[i].mute;
		}

	cs->channelv[index].mute = 0;

	for (uint8_t p = 1; p < cs->patternc; p++)
	{
		memset(cs->patternv[p]->rowv[index], 0, sizeof(row) * 256);
		for (short r = 0; r < 256; r++) cs->patternv[p]->rowv[index][r].inst = 255;
	}

	cs->channelc++;
	return 0;
}
int delChannel(uint8_t index)
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
		free(s->channelv[index].stretchrampbuffer);
		s->channelv[index].stretchrampbuffer = NULL;

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
int yankChannel(uint8_t index)
{
	for (uint8_t i = 0; i < s->patternc; i++)
		if (s->channelbuffer[i])
		{
			memcpy(s->channelbuffer[i],
				s->patternv[i]->rowv[index],
				sizeof(row) * 256);
		}
	s->channelbuffermute = s->channelv[index].mute;
	return 0;
}
int putChannel( uint8_t index)
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
int _addPattern(song *cs, uint8_t realindex)
{
	cs->channelbuffer[realindex] = calloc(256, sizeof(row));
	cs->patternv[realindex] = calloc(1, sizeof(pattern));
	if (!cs->channelbuffer[realindex] || !cs->patternv[realindex])
		return 1;
	for (short c = 0; c < 64; c++)
		for (short r = 0; r < 256; r++)
			cs->patternv[realindex]->rowv[c][r].inst = 255;
	return 0;
}
/* length 0 for the default */
int addPattern(uint8_t index, uint8_t length)
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
int yankPattern(uint8_t index)
{
	if (s->patterni[index] == 0) return 1; /* nothing to yank */

	memcpy(&s->songbuffer,
		s->patternv[s->patterni[index]],
		sizeof(pattern));
	return 0;
}
int putPattern(uint8_t index)
{
	if (s->patterni[index] == 0)
		if (addPattern(index, 0)) return 1; /* allocate memory */

	memcpy(s->patternv[s->patterni[index]],
		&s->songbuffer,
		sizeof(pattern));

	if (s->patternv[s->patterni[index]]->rowc == 0)
		s->patternv[s->patterni[index]]->rowc = s->defpatternlength;
	return 0;
}
int delPattern(uint8_t index)
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

short tfxToVfx(short trackerfx)
{
	switch (trackerfx)
	{
		case 2: case 3: return 2;
		case 4: case 5: return 3;
	}
	return trackerfx;
}
short vfxToTfx(short visualfx)
{
	switch (visualfx)
	{
		case 2: return 2;
		case 3: return 4;
	}
	return visualfx;
}
void yankPartPattern(short x1, short x2, short y1, short y2, uint8_t c1, uint8_t c2)
{
	memcpy(&s->patternbuffer, s->patternv[s->patterni[s->songi[w->songfx]]], sizeof(pattern));
	s->pbfx[0] = x1;
	s->pbfx[1] = x2;
	s->pbfy[0] = y1;
	s->pbfy[1] = y2;
	s->pbchannel[0] = c1;
	s->pbchannel[1] = c2;
	s->pbpopulated = 1;
}
void putPartPattern(void)
{
	if (!s->pbpopulated) return;
	pattern *destpattern = s->patternv[s->patterni[s->songi[w->songfx]]];
	if (s->pbchannel[0] == s->pbchannel[1]) /* only one channel */
	{
		if ((s->pbfx[0] == 2 && s->pbfx[1] == 2) || (s->pbfx[0] == 3 && s->pbfx[1] == 3)) /* just one macro column */
		{
			unsigned char targetmacro;
			if (w->trackerfx < 2) targetmacro = 0;
			else targetmacro = tfxToVfx(w->trackerfx) - 2;

			for (uint8_t j = s->pbfy[0]; j <= s->pbfy[1]; j++)
			{
				uint8_t row = w->trackerfy + j - s->pbfy[0];
				if (row < destpattern->rowc)
				{
					destpattern->rowv[w->channel][row].macroc[targetmacro] = s->patternbuffer.rowv[s->pbchannel[0]][j].macroc[s->pbfx[0] - 2];
					destpattern->rowv[w->channel][row].macrov[targetmacro] = s->patternbuffer.rowv[s->pbchannel[0]][j].macrov[s->pbfx[0] - 2];
				} else break;
			}
			w->trackerfx = vfxToTfx(targetmacro + 2);
		} else
		{
			for (uint8_t j = s->pbfy[0]; j <= s->pbfy[1]; j++)
			{
				uint8_t row = w->trackerfy + j - s->pbfy[0];
				if (row < destpattern->rowc)
				{
					if (s->pbfx[0] <= 0 && s->pbfx[1] >= 0) destpattern->rowv[w->channel][row].note = s->patternbuffer.rowv[s->pbchannel[0]][j].note;
					if (s->pbfx[0] <= 1 && s->pbfx[1] >= 1) destpattern->rowv[w->channel][row].inst = s->patternbuffer.rowv[s->pbchannel[0]][j].inst;
					if (s->pbfx[0] <= 2 && s->pbfx[1] >= 2)
					{
						destpattern->rowv[w->channel][row].macroc[0] = s->patternbuffer.rowv[s->pbchannel[0]][j].macroc[0];
						destpattern->rowv[w->channel][row].macrov[0] = s->patternbuffer.rowv[s->pbchannel[0]][j].macrov[0];
					}
					if (s->pbfx[0] <= 3 && s->pbfx[1] >= 3)
					{
						destpattern->rowv[w->channel][row].macroc[1] = s->patternbuffer.rowv[s->pbchannel[0]][j].macroc[1];
						destpattern->rowv[w->channel][row].macrov[1] = s->patternbuffer.rowv[s->pbchannel[0]][j].macrov[1];
					}
				} else break;
			}
			w->trackerfx = vfxToTfx(s->pbfx[0]);
		}
	} else
	{
		for (uint8_t i = s->pbchannel[0]; i <= s->pbchannel[1]; i++)
		{
			uint8_t channel = w->channel + i - s->pbchannel[0];
			if (channel < s->channelc)
			{
				for (uint8_t j = s->pbfy[0]; j <= s->pbfy[1]; j++)
				{
					uint8_t row = w->trackerfy + j - s->pbfy[0];
					if (row < destpattern->rowc)
					{
						if (i == s->pbchannel[0]) /* first channel */
						{
							if (s->pbfx[0] <= 0) destpattern->rowv[channel][row].note = s->patternbuffer.rowv[i][j].note;
							if (s->pbfx[0] <= 1) destpattern->rowv[channel][row].inst = s->patternbuffer.rowv[i][j].inst;
							if (s->pbfx[0] <= 2)
							{
								destpattern->rowv[channel][row].macroc[0] = s->patternbuffer.rowv[i][j].macroc[0];
								destpattern->rowv[channel][row].macrov[0] = s->patternbuffer.rowv[i][j].macrov[0];
							}
							if (s->pbfx[0] <= 3)
							{
								destpattern->rowv[channel][row].macroc[1] = s->patternbuffer.rowv[i][j].macroc[1];
								destpattern->rowv[channel][row].macrov[1] = s->patternbuffer.rowv[i][j].macrov[1];
							}
						} else if (i == s->pbchannel[1]) /* last channel */
						{
							if (s->pbfx[1] >= 0) destpattern->rowv[channel][row].note = s->patternbuffer.rowv[i][j].note;
							if (s->pbfx[1] >= 1) destpattern->rowv[channel][row].inst = s->patternbuffer.rowv[i][j].inst;
							if (s->pbfx[1] >= 2)
							{
								destpattern->rowv[channel][row].macroc[0] = s->patternbuffer.rowv[i][j].macroc[0];
								destpattern->rowv[channel][row].macrov[0] = s->patternbuffer.rowv[i][j].macrov[0];
							}
							if (s->pbfx[1] >= 3)
							{
								destpattern->rowv[channel][row].macroc[1] = s->patternbuffer.rowv[i][j].macroc[1];
								destpattern->rowv[channel][row].macrov[1] = s->patternbuffer.rowv[i][j].macrov[1];
							}
						} else /* middle channel */
							destpattern->rowv[channel][row] = s->patternbuffer.rowv[i][j];
					} else break;
				}
			} else break;
		}
		w->trackerfx = vfxToTfx(s->pbfx[0]);
	}
}
void delPartPattern(short x1, short x2, short y1, short y2, uint8_t c1, uint8_t c2)
{
	pattern *destpattern = s->patternv[s->patterni[s->songi[w->songfx]]];
	if (c1 == c2) /* only one channel */
	{
		for (uint8_t j = y1; j <= y2; j++)
		{
			if (j < destpattern->rowc)
			{
				if (x1 <= 0 && x2 >= 0) destpattern->rowv[c1][j].note = 0;
				if (x1 <= 1 && x2 >= 1) destpattern->rowv[c1][j].inst = 255;
				if (x1 <= 2 && x2 >= 2)
				{
					destpattern->rowv[c1][j].macroc[0] = 0;
					destpattern->rowv[c1][j].macrov[0] = 0;
				}
				if (x1 <= 3 && x2 >= 3)
				{
					destpattern->rowv[c1][j].macroc[1] = 0;
					destpattern->rowv[c1][j].macrov[1] = 0;
				}
			} else break;
		}
	} else
		for (uint8_t i = c1; i <= c2; i++)
		{
			if (i < s->channelc)
			{
				for (uint8_t j = y1; j <= y2; j++)
				{
					if (j < destpattern->rowc)
					{
						if (i == c1) /* first channel */
						{
							if (x1 <= 0) destpattern->rowv[i][j].note = 0;
							if (x1 <= 1) destpattern->rowv[i][j].inst = 255;
							if (x1 <= 2)
							{
								destpattern->rowv[i][j].macroc[0] = 0;
								destpattern->rowv[i][j].macrov[0] = 0;
							}
							if (x1 <= 3)
							{
								destpattern->rowv[i][j].macroc[1] = 0;
								destpattern->rowv[i][j].macrov[1] = 0;
							}
						} else if (i == c2) /* last channel */
						{
							if (x2 >= 0) destpattern->rowv[i][j].note = 0;
							if (x2 >= 1) destpattern->rowv[i][j].inst = 255;
							if (x2 >= 2)
							{
								destpattern->rowv[i][j].macroc[0] = 0;
								destpattern->rowv[i][j].macrov[0] = 0;
							}
							if (x2 >= 3)
							{
								destpattern->rowv[i][j].macroc[1] = 0;
								destpattern->rowv[i][j].macrov[1] = 0;
							}
						} else /* middle channel */
						{
							memset(&destpattern->rowv[i][j], 0, sizeof(row));
							destpattern->rowv[i][j].inst = 255;
						}
					} else break;
				}
			} else break;
		}
}
void addPartPattern(signed char value, short x1, short x2, short y1, short y2, uint8_t c1, uint8_t c2)
{
	pattern *destpattern = s->patternv[s->patterni[s->songi[w->songfx]]];
	if (c1 == c2) /* only one channel */
	{
		for (uint8_t j = y1; j <= y2; j++)
		{
			if (j < destpattern->rowc)
			{
				if (x1 <= 0 && x2 >= 0) destpattern->rowv[c1][j].note += value;
				if (x1 <= 1 && x2 >= 1) destpattern->rowv[c1][j].inst += value;
				if (x1 <= 2 && x2 >= 2) destpattern->rowv[c1][j].macrov[0] += value;
				if (x1 <= 3 && x2 >= 3) destpattern->rowv[c1][j].macrov[1] += value;
			} else break;
		}
	} else
		for (uint8_t i = c1; i <= c2; i++)
		{
			if (i < s->channelc)
			{
				for (uint8_t j = y1; j <= y2; j++)
				{
					if (j < destpattern->rowc)
					{
						if (i == c1) /* first channel */
						{
							if (x1 <= 0) destpattern->rowv[i][j].note += value;
							if (x1 <= 1) destpattern->rowv[i][j].inst += value;
							if (x1 <= 2) destpattern->rowv[i][j].macrov[0] += value;
							if (x1 <= 3) destpattern->rowv[i][j].macrov[1] += value;
						} else if (i == c2) /* last channel */
						{
							if (x2 >= 0) destpattern->rowv[i][j].note += value;
							if (x2 >= 1) destpattern->rowv[i][j].inst += value;
							if (x2 >= 2) destpattern->rowv[i][j].macrov[0] += value;
							if (x2 >= 3) destpattern->rowv[i][j].macrov[1] += value;
						} else /* middle channel */
						{
							destpattern->rowv[i][j].inst += value;
							destpattern->rowv[i][j].note += value;
							destpattern->rowv[i][j].inst += value;
							destpattern->rowv[i][j].macrov[0] += value;
							destpattern->rowv[i][j].macrov[1] += value;
						}
					} else break;
				}
			} else break;
		}
}



/* forceindex is a realindex */
int changeInstrumentType(song *cs, uint8_t forceindex)
{
	uint8_t i;
	if (!forceindex)
		if (w->instrumentlockv == INST_GLOBAL_LOCK_FREE
				|| w->instrumentlockv == INST_GLOBAL_LOCK_HIST)
			i = w->instrumentlocki;
		else return 0;
	else i = forceindex;

	instrument *iv = cs->instrumentv[i];
	if (iv)
	{
		if (forceindex)
		{
			if (!iv->state[iv->type] && iv->type < INSTRUMENT_TYPE_COUNT)
			{
				t->f[iv->type].initType(&iv->state[iv->type]);
				if (!iv->state[iv->type])
				{
					strcpy(w->command.error, "failed to allocate instrument type, out of memory");
					return 1;
				}
			}
		} else switch (w->instrumentlockv)
		{
			case INST_GLOBAL_LOCK_FREE:
				if (!iv->state[iv->type] && iv->type < INSTRUMENT_TYPE_COUNT)
				{
					t->f[iv->type].initType(&iv->state[iv->type]);
					if (!iv->state[iv->type])
					{
						strcpy(w->command.error, "failed to allocate instrument type, out of memory");
						return 1;
					}
				}
				break;
			case INST_GLOBAL_LOCK_HIST:
				instrument *dest = iv;
				instrument *src = iv->history[iv->historyptr%128];

				dest->type = src->type;
				dest->fader = src->fader;
				memcpy(dest->send, src->send, sizeof(char) * 16);

				for (int j = 0; j < INSTRUMENT_TYPE_COUNT; j++)
					if (src->state[j])
					{
						if (!dest->state[j]) t->f[j].initType(&dest->state[j]);
						memcpy(dest->state[j], src->state[j], t->f[j].statesize);
					}

				dest->samplelength = src->samplelength;
				if (dest->sampledata)
				{
					free(dest->sampledata);
					dest->sampledata = NULL;
				}
				if (src->samplelength)
				{
					dest->sampledata = malloc(sizeof(short) * dest->samplelength);
					memcpy(dest->sampledata, src->sampledata, sizeof(short) * dest->samplelength);
				}
				break;
		}
		iv->typefollow = iv->type;
	}


	if (!forceindex)
	{
		w->instrumentlockv = INST_GLOBAL_LOCK_OK; // mark as free to use
		redraw();
	}

	return 0;
}

int _addInstrument(song *cs, uint8_t realindex)
{
	cs->instrumentv[realindex] = calloc(1, sizeof(instrument));
	if (!cs->instrumentv[realindex]) return 1;
	cs->instrumentv[realindex]->outbufferl = calloc(sizeof(sample_t), buffersize);
	cs->instrumentv[realindex]->outbufferr = calloc(sizeof(sample_t), buffersize);
	return 0;
}
void _instantiateInstrumentEffect(uint8_t realindex)
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
void pushInstrumentHistory(instrument *iv)
{
	if (!iv) return;
	if (iv->historyptr == 255)
		iv->historyptr = 128;
	else
		iv->historyptr++;

	if (iv->historybehind)
		iv->historybehind--;
	if (iv->historyahead)
		iv->historyahead = 0;

	if (!iv->history[iv->historyptr%128])
		iv->history[iv->historyptr%128] = calloc(1, sizeof(instrument));
	else
		for (int i = 0; i < INSTRUMENT_TYPE_COUNT; i++)
			if (iv->history[iv->historyptr%128]->state[i])
				free(iv->history[iv->historyptr%128]->state[i]);

	instrument *ivh = iv->history[iv->historyptr%128];
	ivh->type = iv->type;
	for (int i = 0; i < INSTRUMENT_TYPE_COUNT; i++)
		if (iv->state[i])
		{
			t->f[i].initType(&ivh->state[i]);
			memcpy(ivh->state[i], iv->state[i], t->f[i].statesize);
		}
	ivh->fader = iv->fader;
	memcpy(ivh->send, iv->send, sizeof(char) * 16);

	ivh->samplelength = iv->samplelength;
	if (iv->sampledata)
	{
		ivh->sampledata = malloc(sizeof(short) * iv->samplelength);
		memcpy(ivh->sampledata, iv->sampledata, sizeof(short) * iv->samplelength);
	}
}
void pushInstrumentHistoryIfNew(instrument *iv)
{
	if (!iv) return;

	instrument *ivh = iv->history[iv->historyptr%128];
	if (ivh && iv->type < INSTRUMENT_TYPE_COUNT)
	{
		if (!iv->state[iv->type]
				|| ivh->fader != iv->fader
				|| memcmp(ivh->send, iv->send, sizeof(char) * 16)
				|| ivh->samplelength != iv->samplelength)
			pushInstrumentHistory(iv);
		else
			for (int i = 0; i < INSTRUMENT_TYPE_COUNT; i++)
				if (ivh->state[i] || iv->state[i])
					if (!ivh->state[i] || !iv->state[i] || memcmp(ivh->state[i], iv->state[i], t->f[i].statesize))
					{ pushInstrumentHistory(iv); break; }
	}
}

void _popInstrumentHistory(uint8_t realindex)
{
	w->instrumentlocki = realindex;
	w->instrumentlockv = INST_GLOBAL_LOCK_PREP_HIST;
}
void popInstrumentHistory(uint8_t realindex) /* undo */
{
	instrument *iv = s->instrumentv[realindex];
	if (!iv) return;
	if (iv->historyptr <= 1 || iv->historybehind >= 127)
	{ strcpy(w->command.error, "already at oldest change"); return; }

	if (iv->historyptr == 128)
		iv->historyptr = 255;
	else
		iv->historyptr--;

	_popInstrumentHistory(realindex);

	iv->historybehind++;
	iv->historyahead++;
}
void unpopInstrumentHistory(uint8_t realindex) /* redo */
{
	instrument *iv = s->instrumentv[realindex];
	if (!iv) return;
	if (iv->historyahead == 0)
	{ strcpy(w->command.error, "already at newest change"); return; }

	if (iv->historyptr == 255)
		iv->historyptr = 128;
	else
		iv->historyptr++;

	_popInstrumentHistory(realindex);

	iv->historybehind--;
	iv->historyahead--;
}
int addInstrument(uint8_t index)
{
	if (s->instrumenti[index] > 0) return 1; /* index occupied */

	if (_addInstrument(s, s->instrumentc))
	{
		strcpy(w->command.error, "failed to add instrument, out of memory");
		return 1;
	}
	s->instrumentv[s->instrumentc]->fader = 0xFF;
	changeInstrumentType(s, s->instrumentc);
	t->f[s->instrumentv[s->instrumentc]->type].initType(&s->instrumentv[s->instrumentc]->state[s->instrumentv[s->instrumentc]->type]);
	s->instrumenti[index] = s->instrumentc;

	_instantiateInstrumentEffect(s->instrumentc);
	pushInstrumentHistory(s->instrumentv[s->instrumentc]);

	s->instrumentc++;
	return 0;
}
/* return a fresh slot */
uint8_t newInstrument(uint8_t minindex)
{
	for (uint8_t i = minindex; i < 256; i++) // is 256 right? idfk
	{
		if (s->instrumenti[i] == 0)
		{
			return i;
		}
	}
}
int yankInstrument(uint8_t index)
{
	if (s->instrumenti[index] == 0) return 1; /* nothing to yank */

	if (s->instrumentbuffer.samplelength > 0)
	{
		free(s->instrumentbuffer.sampledata);
		s->instrumentbuffer.sampledata = NULL;
	}

	for (int i = 0; i < INSTRUMENT_TYPE_COUNT; i++)
		if (s->instrumentbuffer.state[i])
		{
			free(s->instrumentbuffer.state[i]);
			s->instrumentbuffer.state[i] = NULL;
		}

	s->instrumentbuffer.type = s->instrumentv[s->instrumenti[index]]->type;
	s->instrumentbuffer.samplelength = s->instrumentv[s->instrumenti[index]]->samplelength;
	s->instrumentbuffer.fader = s->instrumentv[s->instrumenti[index]]->fader;
	memcpy(s->instrumentbuffer.send,  s->instrumentv[s->instrumenti[index]]->send, 1 * 16);

	if (s->instrumentbuffer.samplelength > 0)
	{
		s->instrumentbuffer.sampledata =
			malloc(sizeof(short) * s->instrumentv[s->instrumenti[index]]->samplelength);

		if (s->instrumentbuffer.sampledata == NULL)
		{
			strcpy(w->command.error, "failed to yank instrument, out of memory");
			return 1;
		}

		memcpy(s->instrumentbuffer.sampledata,
			s->instrumentv[s->instrumenti[index]]->sampledata,
			sizeof(short) * s->instrumentv[s->instrumenti[index]]->samplelength);
	}

	t->f[s->instrumentbuffer.type].initType(&s->instrumentbuffer.state[s->instrumentbuffer.type]);
	memcpy(s->instrumentbuffer.state[s->instrumentbuffer.type],
			s->instrumentv[s->instrumenti[index]]->state[s->instrumentbuffer.type],
			t->f[s->instrumentbuffer.type].statesize);
	return 0;
}
int putInstrument(uint8_t index)
{
	if (s->instrumenti[index] == 0)
	{
		if (addInstrument(index)) /* allocate memory */
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

	s->instrumentv[s->instrumenti[index]]->type = s->instrumentbuffer.type;
	s->instrumentv[s->instrumenti[index]]->samplelength = s->instrumentbuffer.samplelength;
	s->instrumentv[s->instrumenti[index]]->fader = s->instrumentbuffer.fader;
	memcpy(s->instrumentv[s->instrumenti[index]]->send,  s->instrumentbuffer.send, 1 * 16);
	changeInstrumentType(s, s->instrumenti[index]); /* is this safe to force? */

	memcpy(s->instrumentv[s->instrumenti[index]]->state[s->instrumentbuffer.type],
			s->instrumentbuffer.state[s->instrumentbuffer.type],
			t->f[s->instrumentbuffer.type].statesize);

	if (s->instrumentbuffer.samplelength > 0)
	{
		s->instrumentv[s->instrumenti[index]]->sampledata =
			malloc(sizeof(short) * s->instrumentbuffer.samplelength);

		if (s->instrumentv[s->instrumenti[index]]->sampledata == NULL)
		{
			strcpy(w->command.error, "failed to put instrument, out of memory");
			return 1;
		}

		memcpy(s->instrumentv[s->instrumenti[index]]->sampledata,
			s->instrumentbuffer.sampledata,
			sizeof(short) * s->instrumentbuffer.samplelength);
	}

	return 0;
}
void _delInstrument(song *cs, uint8_t realindex)
{
	/* free the sample data */
	for (int i = 0; i < INSTRUMENT_TYPE_COUNT; i++)
		if (cs->instrumentv[realindex]->state[i])
			free(cs->instrumentv[realindex]->state[i]);
	if (cs->instrumentv[realindex]->samplelength > 0)
		free(cs->instrumentv[realindex]->sampledata);
	free(cs->instrumentv[realindex]->outbufferl);
	free(cs->instrumentv[realindex]->outbufferr);

	for (int i = 0; i < 128; i++)
	{
		if (!cs->instrumentv[realindex]->history[i]) continue;
		for (int j = 0; j < INSTRUMENT_TYPE_COUNT; j++)
			if (cs->instrumentv[realindex]->history[i]->state[j])
				free(cs->instrumentv[realindex]->history[i]->state[j]);
		if (cs->instrumentv[realindex]->history[i]->sampledata)
			free(cs->instrumentv[realindex]->history[i]->sampledata);
		free(cs->instrumentv[realindex]->history[i]);
	}

	for (int i = 0; i < 16; i++)
		switch (cs->effectv[i].type)
		{
			case 2:
				lilv_instance_free(cs->instrumentv[realindex]->plugininstance[i]);
				break;
		}

	free(cs->instrumentv[realindex]);
	cs->instrumentv[realindex] = NULL;
}
int delInstrument(uint8_t index)
{
	if (s->instrumenti[index] < 1) return 1; /* instrument doesn't exist */

	uint8_t cutindex = s->instrumenti[index];
	s->instrumenti[index] = 0;

	_delInstrument(s, cutindex);

	/* enforce contiguity */
	for (uint8_t i = cutindex; i < s->instrumentc - 1; i++)
		s->instrumentv[i] = s->instrumentv[i + 1];

	for (uint8_t i = 0; i < s->instrumentc; i++) // for every backref index
		if (s->instrumenti[i] > cutindex)
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
void loadSample(uint8_t index, char *path)
{
	instrument *iv = s->instrumentv[s->instrumenti[index]];
	SF_INFO sfinfo;
	short *sampledata = _loadSample(path, &sfinfo);
	if (!sampledata)
	{
		strcpy(w->command.error, "failed to load sample, out of memory");
		return;
	}

	/* unload any present sample data */
	if (iv->samplelength > 0)
		free(iv->sampledata);
	iv->sampledata = sampledata;
	iv->samplelength = sfinfo.frames * sfinfo.channels;

	sampler_state *ss = iv->state[0];
	ss->channels = sfinfo.channels;
	ss->length = sfinfo.frames;
	ss->c5rate = sfinfo.samplerate;
	ss->trim[0] = 0;
	ss->trim[1] = sfinfo.frames;
	ss->loop[0] = 0;
	ss->loop[1] = 0;
};
int exportSample(uint8_t index, char *path)
{
	if (s->instrumenti[index] < 0) return 1; /* instrument doesn't exist */
	if (s->instrumentv[s->instrumenti[index]]->samplelength < 1) return 1; /* no sample data */

	instrument *iv = s->instrumentv[s->instrumenti[index]];
	SNDFILE *sndfile;
	SF_INFO sfinfo;
	memset(&sfinfo, 0, sizeof(sfinfo));

	sampler_state *ss = iv->state[0];
	sfinfo.samplerate = ss->c5rate;
	sfinfo.frames = ss->length;
	sfinfo.channels = ss->channels;

	sfinfo.format = SF_FORMAT_WAV | SF_FORMAT_PCM_16;
	sndfile = sf_open(path, SFM_WRITE, &sfinfo);
	if (sndfile == NULL) return 1;

	/* write the sample data to disk */
	sf_writef_short(sndfile, iv->sampledata, ss->length);
	sf_close(sndfile);
	return 0;
};





song *_addSong(void)
{
	song *cs = calloc(1, sizeof(song));
	if (!cs) return NULL;

	cs->patternc = 1;
	cs->instrumentc = 1;

	cs->rowhighlight = 4;
	cs->defpatternlength = 63;
	cs->songbpm = 125;
	w->request = REQ_BPM;

	memset(cs->songi, 255, sizeof(uint8_t) * 256);

	cs->effectinl = calloc(sizeof(sample_t), buffersize);
	cs->effectinr = calloc(sizeof(sample_t), buffersize);
	cs->effectoutl = calloc(sizeof(sample_t), buffersize);
	cs->effectoutr = calloc(sizeof(sample_t), buffersize);
	cs->effectdryl = calloc(sizeof(sample_t), buffersize);
	cs->effectdryr = calloc(sizeof(sample_t), buffersize);
	if (       !cs->effectinl  || !cs->effectinr
			|| !cs->effectoutl || !cs->effectoutr
			|| !cs->effectdryl || !cs->effectdryr)
	{
		free(cs);
		return NULL;
	}
	return cs;
}
song *addSong(void)
{
	song *cs = _addSong();
	if (!cs) return NULL;
	addChannel(cs, cs->channelc);
	addChannel(cs, cs->channelc);
	addChannel(cs, cs->channelc);
	addChannel(cs, cs->channelc);
	return cs;
}

void delSong(song *cs)
{
	free(cs->effectinl);
	free(cs->effectinr);
	free(cs->effectoutl);
	free(cs->effectoutr);
	free(cs->effectdryl);
	free(cs->effectdryr);

	for (int i = 0; i < cs->channelc; i++)
	{
		free(cs->channelv[i].rampbuffer);
		free(cs->channelv[i].stretchrampbuffer);
	}

	for (int i = 1; i < cs->patternc; i++)
	{
		free(cs->channelbuffer[i]);
		free(cs->patternv[i]);
	}

	for (int i = 0; i < 16; i++)
		_unloadLv2Effect(i);
	if (cs->effectbufferlv2values)
		free(cs->effectbufferlv2values);

	for (int i = 1; i < cs->instrumentc; i++)
		_delInstrument(cs, i);

	for (int i = 0; i < INSTRUMENT_TYPE_COUNT; i++)
		if (s->instrumentbuffer.state[i])
			free(s->instrumentbuffer.state[i]);
	if (cs->instrumentbuffer.samplelength > 0)
		free(cs->instrumentbuffer.sampledata);

	free(cs);
}

/* free the return value */
char *fileExtension(char *path, char *ext)
{
	char *ret;
	if (strlen(path) < strlen(ext) || strcmp(path+(strlen(path) - strlen(ext)), ext))
	{
		ret = malloc(strlen(path) + strlen(ext) + 1);
		strcpy(ret, path);
		strcat(ret, ext);
	} else
	{
		ret = malloc(strlen(path) + 1);
		strcpy(ret, path);
	}
	return ret;
}
int writeSong(char *path)
{
	char *pathext = fileExtension(path, ".omlm");
	if (!strcmp(pathext, ".omlm"))
	{
		if (!strlen(w->filepath))
		{
			strcpy(w->command.error, "no file name");
			return 1;
		}
		strcpy(pathext, w->filepath);
	} else strcpy(w->filepath, pathext);

	FILE *fp = fopen(pathext, "wb");
	int i, j, k;

	/* egg, for each and every trying time (the most important) */
	fputc('e', fp);
	fputc('g', fp);
	fputc('g', fp);
	fseek(fp, 0x4, SEEK_SET);

	/* counts */
	fwrite(&s->songbpm, sizeof(uint8_t), 1, fp);
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
	instrument *iv;
	for (i = 1; i < s->instrumentc; i++)
	{
		iv = s->instrumentv[i];
		fputc(iv->type, fp);

		for (uint8_t j = 0; j < INSTRUMENT_TYPE_COUNT; j++)
			if (iv->state[j])
			{
				fputc(j + 1, fp); // type index
				t->f[j].write(&iv->state[j], fp);
			}
		fputc(0, fp); // no more types

		fputc(iv->fader, fp);
		fwrite(&iv->send, 1, 16, fp);
		fwrite(&iv->samplelength, sizeof(uint32_t), 1, fp);
		if (iv->samplelength > 0)
			fwrite(iv->sampledata, sizeof(short), iv->samplelength, fp);
	}

	/* effectv */
	for (i = 0; i < 16; i++)
	{
		fputc(s->effectv[i].type, fp);
		if (s->effectv[i].type)
		{
			/* plugin uri */
			const LilvNode *uri = lilv_plugin_get_uri(s->effectv[i].plugin);
			const char *uristring = lilv_node_as_string(uri);
			fputc(strlen(uristring), fp); /* assume the uri is never longer than 255 chars */
			fwrite(uristring, 1, strlen(uristring), fp);

			/* control port values */
			for (int j = 0; j < s->effectv[i].indexc; j++)
				fwrite(&s->effectv[i].controlv[j].value, sizeof(float), 1, fp);
		}
	}


	fclose(fp);
	snprintf(w->command.error, COMMAND_LENGTH, "'%s' written", pathext);
	free(pathext);
	return 0;
}

song *readSong(char *path)
{
	FILE *fp = fopen(path, "r");
	if (!fp) // file doesn't exist, or fopen otherwise failed
	{
		snprintf(w->command.error, COMMAND_LENGTH, "file '%s' doesn't exist", path);
		return NULL;
	}
	DIR *dp = opendir(path);
	if (dp) // file is a directory
	{
		closedir(dp);
		snprintf(w->command.error, COMMAND_LENGTH, "file '%s' is a directory", path);
		return NULL;
	}

	int i, j, k;

	/* ensure egg wasn't broken in transit */
	/* the most important check */
	if (!(fgetc(fp) == 'e' && fgetc(fp) == 'g' && fgetc(fp) == 'g'))
	{
		snprintf(w->command.error, COMMAND_LENGTH, "file '%s' isn't valid", path);
		return NULL;
	}
	fseek(fp, 0x4, SEEK_SET);

	song *cs = _addSong();
	if (!cs)
	{
		strcpy(w->command.error, "failed to read song, out of memory");
		return NULL;
	}

	/* assume the rest of the file is valid */
	/* TODO: proper error checking lol */
	strcpy(w->filepath, path);

	/* counts */
	fread(&cs->songbpm, sizeof(uint8_t), 1, fp);
	w->request = REQ_BPM;

	cs->patternc = fgetc(fp);
	cs->instrumentc = fgetc(fp);
	cs->channelc = fgetc(fp);
	for (int i = 0; i < cs->channelc; i++)
		_addChannel(&cs->channelv[i]);
	cs->rowhighlight = fgetc(fp);

	fseek(fp, 0x10, SEEK_SET);
	/* mutes */
	char byte;
	for (i = 0; i < 8; i++)
	{
		byte = fgetc(fp);
		if (byte & 0b10000000) cs->channelv[i * 8 + 0].mute = 1;
		if (byte & 0b01000000) cs->channelv[i * 8 + 1].mute = 1;
		if (byte & 0b00100000) cs->channelv[i * 8 + 2].mute = 1;
		if (byte & 0b00010000) cs->channelv[i * 8 + 3].mute = 1;
		if (byte & 0b00001000) cs->channelv[i * 8 + 4].mute = 1;
		if (byte & 0b00000100) cs->channelv[i * 8 + 5].mute = 1;
		if (byte & 0b00000010) cs->channelv[i * 8 + 6].mute = 1;
		if (byte & 0b00000001) cs->channelv[i * 8 + 7].mute = 1;
		fputc(byte, fp);
	}

	fseek(fp, 0x20, SEEK_SET);
	/* songi */
	for (i = 0; i < 256; i++)
		cs->songi[i] = fgetc(fp);
	/* songa */
	for (i = 0; i < 256; i++)
		cs->songa[i] = fgetc(fp);
	/* patterni */
	for (i = 0; i < 256; i++)
		cs->patterni[i] = fgetc(fp);
	/* instrumenti */
	for (i = 0; i < 256; i++)
		cs->instrumenti[i] = fgetc(fp);

	/* patternv */
	for (i = 1; i < cs->patternc; i++)
	{
		_addPattern(cs, i);
		cs->patternv[i]->rowc = fgetc(fp);
		for (j = 0; j < cs->channelc; j++)
			for (k = 0; k < cs->patternv[i]->rowc + 1; k++)
			{
				cs->patternv[i]->rowv[j][k].note = fgetc(fp);
				cs->patternv[i]->rowv[j][k].inst = fgetc(fp);
				cs->patternv[i]->rowv[j][k].macroc[0] = fgetc(fp);
				cs->patternv[i]->rowv[j][k].macroc[1] = fgetc(fp);
				cs->patternv[i]->rowv[j][k].macrov[0] = fgetc(fp);
				cs->patternv[i]->rowv[j][k].macrov[1] = fgetc(fp);
			}
	}

	/* instrumentv */
	instrument *iv;
	for (i = 1; i < cs->instrumentc; i++)
	{
		_addInstrument(cs, i);

		iv = cs->instrumentv[i];
		iv->type = fgetc(fp);
		iv->typefollow = iv->type; /* confirm the type is safe to use */

		uint8_t j;
		while ((j = fgetc(fp))) /* read until there's no more */
			if (t->f[j-1].read)
			{
				t->f[j-1].initType(&iv->state[j-1]);
				t->f[j-1].read(&iv->state[j-1], fp);
			}

		cs->instrumentv[i]->fader = fgetc(fp);
		fread(&cs->instrumentv[i]->send, 1, 16, fp);
		fread(&cs->instrumentv[i]->samplelength, sizeof(uint32_t), 1, fp);
		if (cs->instrumentv[i]->samplelength > 0)
		{
			short *sampledata = malloc(sizeof(short) * cs->instrumentv[i]->samplelength);
			if (!sampledata) // malloc failed
			{
				strcpy(w->command.error, "some sample data failed to load, out of memory");
				fseek(fp, sizeof(short) * cs->instrumentv[i]->samplelength, SEEK_CUR);
				continue;
			}

			fread(sampledata, sizeof(short), cs->instrumentv[i]->samplelength, fp); /* <<< this sometimes segfaults */
			cs->instrumentv[i]->sampledata = sampledata;
		}
		pushInstrumentHistory(cs->instrumentv[i]);
	}

	/* effectv */
	for (i = 0; i < 16; i++)
	{
		cs->effectv[i].type = fgetc(fp);
		if (cs->effectv[i].type)
		{
			/* plugin uri */
			unsigned char urilen = fgetc(fp);
			char uristring[urilen];

			fread(uristring, 1, urilen, fp);
			uristring[urilen] = '\0';

			LilvNode *uri = lilv_new_uri(lv2.world, uristring);
			loadLv2Effect(i, lilv_plugins_get_by_uri(lv2.plugins, uri));
			lilv_node_free(uri);

			/* control port values */
			for (int j = 0; j < cs->effectv[i].indexc; j++)
				fread(&cs->effectv[i].controlv[j].value, sizeof(float), 1, fp);
		}
	}


	fclose(fp);
	return cs;
}
