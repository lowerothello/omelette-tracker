#define MAX_VALUE_LEN 128
#define MAX_CHANNELS 32

typedef struct
{
	char    c; /* command */
	uint8_t v; /* argument */
} macro;
#define NOTE_VOID 255
#define NOTE_OFF 254
#define NOTE_UNUSED 253 /* explicitly unused */
#define NOTE_C5 60   /* centre note */
#define NOTE_A10 120 /* first out of range note */
#define INST_VOID 255
typedef struct
{
	uint8_t note;     /* 0-127: MIDI compatible | >127: NOTE_* declares */
	uint8_t inst;     /* INST_* declares */
	macro   macro[8]; /* up to 8 macro columns */
} row;
#define PATTERN_VOID 255
#define ROW_MAX 256
typedef struct Pattern
{
	uint8_t rowc;
	uint8_t rowcc[MAX_CHANNELS];         /* rowc per-channel */
	row     rowv[MAX_CHANNELS][ROW_MAX]; /* MAX_CHANNELS channels, each with ROW_MAX rows */

	struct Pattern *history[128];                /* pattern snapshots */
	uint8_t         historychannel[128];
	short           historyfy[128], historyfx[128];
	uint8_t         historyptr;                  /* highest bit is effectively an overflow bit */
	uint8_t         historybehind;               /* tracks how many less than 128 safe indices there are */
	uint8_t         historyahead;                /* tracks how many times it's safe to redo */
} pattern;

#define C_FLAG_MUTE        0b00000001
#define C_FLAG_REVERSE     0b00000010
#define C_FLAG_RELEASE     0b00000100
#define C_FLAG_RTRIG_REV   0b00001000
#define C_FLAG_TARGET_RAND 0b00010000
typedef struct
{
	uint8_t flags;
	uint8_t macroc; /* macro count */

	uint32_t pointer;        /* clock */
	uint32_t pitchedpointer; /* sample to play */
	uint8_t  gain;           /* unsigned nibble per-channel */
	uint8_t  randgain;       /* gain override for the Ixy macro */
	short    targetgain;     /* smooth gain target */
	row      r;
	uint8_t  samplernote, samplerinst;
	float    finetune;                 /* calculated fine tune, clamped between -2/+2 for midi */

	float    portamentofinetune;       /* portamento fine tune */
	float    targetportamentofinetune; /* cv->portamentofinetune destination */
	float    startportamentofinetune;  /* cv->portamentofinetune start       */
	uint32_t portamentosamples;        /* portamento length   */
	uint32_t portamentosamplepointer;  /* portamento progress */

	float    microtonalfinetune;         /* used by the local microtonal macro */
	uint16_t rtrigsamples;               /* samples per retrigger */
	uint32_t rtrigpointer;               /* clock reference */
	uint32_t rtrigpitchedpointer;        /* pitchedpointer to ratchet back to */
	uint32_t rtrigcurrentpitchedpointer; /* pitchedpointer the current retrig started at */
	int8_t   rtrigblocksize;             /* number of rows block extends to */
	uint16_t cutsamples;                 /* samples into the row to cut, 0 for no cut */
	uint16_t delaysamples;               /* samples into the row to delay, 0 for no delay */
	uint8_t  delaynote;
	uint8_t  delayinst;
	uint8_t  vibrato;                    /* vibrato depth, 0-f */
	uint32_t vibratosamples;             /* samples per full phase walk */
	uint32_t vibratosamplepointer;       /* distance through cv->vibratosamples */

	short localenvelope;
	short localpitchshift;
	int   localcyclelength;

	short   midiccindex;
	uint8_t midicc;
	short   targetmidicc;

	int8_t sendgroup;
	int8_t sendgain;
	int8_t targetsendgain;

	float envgain;

	/* waveshaper */
	char    waveshaper;               /* which waveshaper to use */
	uint8_t waveshaperstrength;       /* mix / input gain */
	short   targetwaveshaperstrength; /* mix / input gain target */

	/* filter */
	SVFilter fl[2], fr[2];
	SVFilter rampfl[2], rampfr[2];
	char     filtermode;
	int8_t   targetfiltermode;
	uint8_t  filtercut;
	short    targetfiltercut;
	int8_t   filterres, targetfilterres;

	/* compressor */
	uint8_t compressor;       /* high nibble:sidechain weight  low nibble:output weight */
	short   targetcompressor;

	/* ramping */
	uint16_t rampindex;        /* progress through the ramp buffer, rampmax if not ramping */
	short   *rampbuffer;       /* samples to ramp out */
	uint8_t  rampgain;         /* raw gain m for the ramp buffer */
	uint8_t  rampinst;         /* real index, needed to determine the output group */

	uint16_t stretchrampindex; /* progress through the stretch ramp buffer, >=cv->stretchrampmax if not ramping */
	uint16_t stretchrampmax;   /* actual stretchrampmax used, to allow for tiny buffer sizes */
} channel;

typedef struct Instrument
{
	short    *sampledata;   /* variable size, persists between types */
	uint32_t  samplelength; /* raw samples allocated for sampledata */

	uint32_t length;
	uint8_t  channels;
	uint32_t c5rate;
	uint8_t  samplerate;  /* percent of c5rate to actually use */
	uint8_t  bitdepth;
	uint16_t cyclelength;
	uint8_t  pitchshift;
	uint32_t trim[2];
	uint32_t loop;
	uint8_t  envelope;
	uint8_t  gain;
	uint8_t  outputgroup;
	uint8_t  channelmode;
	uint8_t  flags;
	uint8_t  loopramp;
	uint8_t  midichannel;

	uint32_t triggerflash;

	struct Instrument *history[128];      /* instrument snapshots */
	short              historyindex[128]; /* cursor positions for instrument snapshots */
	uint8_t            historyptr;        /* highest bit is an overflow bit */
	uint8_t            historybehind;     /* tracks how many less than 128 safe indices there are */
	uint8_t            historyahead;      /* tracks how many times it's safe to redo */
} instrument;

#define SONG_MAX 256
#define PATTERN_MAX 256
#define INSTRUMENT_MAX 255
#define PLAYING_STOP 0
#define PLAYING_START 1
#define PLAYING_CONT 2
#define PLAYING_PREP_STOP 3
typedef struct
{
	/* patterns */
	uint8_t  patternc;              /* pattern count */
	uint8_t  patterni[PATTERN_MAX]; /* pattern backref */
	pattern *patternv[PATTERN_MAX]; /* pattern values */

	/* instruments */
	uint8_t     instrumentc;                 /* instrument count */
	uint8_t     instrumenti[INSTRUMENT_MAX]; /* instrument backref */
	instrument *instrumentv[INSTRUMENT_MAX]; /* instrument values */

	/* channels */
	uint8_t channelc;     /* channel count */
	channel channelv[32]; /* channel values */

	/* effect state */
	struct
	{
		uint8_t mix;
		uint8_t feedback;
	} reverb;
	float compressorcoef;
	float compressorsidechain;

	/* playlist */
	uint8_t songi[SONG_MAX]; /* song list backref, links to patterns */
	uint8_t songf[SONG_MAX]; /* song list flags */

	/* playback pointer */
	uint8_t songp; /* song pos, analogous to window->songfy */
	short   songr; /* song row, analogous to window->trackerfy */

	/* misc. state */
	uint8_t  rowhighlight;
	uint8_t  bpm;
	uint8_t  songbpm;      /* to store the song's bpm through bpm change macros */
	uint16_t spr;          /* samples per row (samplerate * (60 / bpm) / (rowhighlight * 2)) */
	uint16_t sprp;         /* samples per row progress */
	char     playing;
} song;
song *s;


#define INST_GLOBAL_LOCK_OK 0        /* playback and most memory ops are safe */
#define INST_GLOBAL_LOCK_PREP_FREE 1 /* playback unsafe, preparing to free the state */
#define INST_GLOBAL_LOCK_FREE 2      /* playback has stopped, safe to free the state */
#define INST_GLOBAL_LOCK_PREP_HIST 3 /* playback unsafe, preparing to restore history */
#define INST_GLOBAL_LOCK_HIST 4      /* playback has stopped, restoring history */
#define INST_GLOBAL_LOCK_PREP_PUT 5  /* playback unsafe, preparing to overwrite state */
#define INST_GLOBAL_LOCK_PUT 6       /* playback has stopped, overwriting state */
#define INST_GLOBAL_INST_MUTE 7      /* force stop midi for instrument locki */
#define INST_GLOBAL_CHANNEL_MUTE 8   /* like INST_MUTE but locki contains a channel index */

#define INST_REC_LOCK_OK 0          /* playback and most memory ops are safe */
#define INST_REC_LOCK_START 1       /* want to be unsafe                     */
#define INST_REC_LOCK_CUE_START 2   /* want to be unsafe                     */
#define INST_REC_LOCK_CONT 3        /* recording                             */
#define INST_REC_LOCK_CUE_CONT 4    /* recording                             */
#define INST_REC_LOCK_PREP_END 5    /* start stopping recording              */
#define INST_REC_LOCK_END 6         /* stopping recording has finished       */
#define INST_REC_LOCK_PREP_CANCEL 7 /* start cancelling recording            */
#define INST_REC_LOCK_CANCEL 8      /* cancelling recording has finished     */

#define W_FLAG_FOLLOW 0b00000001

#define REQ_OK 0  /* do nothing / done */
#define REQ_BPM 1 /* re-apply the song bpm */
typedef struct
{
	pattern    songbuffer;       /* full pattern paste buffer, TODO: use */
	pattern    patternbuffer;    /* partial pattern paste buffer */
	short      pbfy[2], pbfx[2]; /* partial pattern paste buffer clipping region */
	uint8_t    pbchannel[2];     /* " */
	char       pbpopulated;      /* there's no good way to tell if pb is set */
	instrument instrumentbuffer; /* instrument paste buffer */
	uint8_t    defpatternlength;

	uint8_t songibuffer[PATTERN_MAX]; /* song list paste buffer */
	uint8_t songfbuffer[PATTERN_MAX]; /* song list flags paste buffer */
	uint8_t songbufferlen;            /* how much of song[i,f]buffer has meaningful data */

	char filepath[COMMAND_LENGTH];

	void         (*filebrowserCallback)(char *); /* arg is the selected path */
	command_t      command;
	unsigned char  popup;
	unsigned char  mode, oldmode;
	unsigned short centre;
	uint8_t        pattern;                      /* focused pattern */
	uint8_t        channel;                      /* focused channel */
	row            channelbufferrowv[ROW_MAX];
	uint8_t        channelbufferrowcc;
	uint8_t        channelbuffermacroc;

	short          trackerfy, trackerfx;
	short          visualfy, visualfx;
	uint8_t        visualchannel;
	short          instrumentindex;
	short          instrument;           /* focused instrument */
	unsigned short instrumentcelloffset;
	unsigned short instrumentrowoffset;

	int filebrowserindex;

	unsigned short mousey, mousex;

	short       fyoffset;
	signed char fieldpointer;

	char           dirpath[NAME_MAX+1];
	unsigned int   dirc;
	unsigned short dirmaxwidth;
	unsigned char  dircols;
	DIR           *dir;

	Canvas  *waveformcanvas;
	char   **waveformbuffer;
	size_t   waveformw, waveformh;
	uint32_t waveformwidth;
	uint32_t waveformcursor;
	uint32_t waveformvisual;
	uint32_t waveformdrawpointer;

	short songfy;

	char     chord;         /* key chord buffer, vi-style multi-letter commands */
	uint16_t count;         /* action repeat count, follows similar rules to w->chord */
	char     octave;
	uint8_t  step;
	char     keyboardmacro;
	uint8_t  flags;         /* %1:follow */

	uint8_t songnext;

	row     previewrow;
	uint8_t previewchannelsrc;
	channel previewchannel;
	char    previewtrigger;               /* 0:cut
	                                         1:start inst
	                                         2:still inst */

	uint8_t instrumentlocki; /* realindex */
	uint8_t instrumentlockv; /* value, set to an INST_GLOBAL_LOCK constant */
	char    request;         /* ask the playback function to do something */

	uint8_t  instrumentreci; /* NOT a realindex */
	uint8_t  instrumentrecv; /* value, set to an INST_REC_LOCK constant */
	short   *recbuffer;      /* disallow changing the type or removing while recording */
	uint32_t recptr;

	char newfilename[COMMAND_LENGTH]; /* used by readSong */
} window;
window *w;

typedef struct
{
	jack_port_t *l;
	jack_port_t *r;
} portpair;
typedef struct
{
	sample_t *l;
	sample_t *r;
} portbufferpair;
#define OUTPUT_GROUPS 16
typedef struct
{
	portbufferpair in;
	portbufferpair out[OUTPUT_GROUPS];
	void *midiout;
} portbuffers;
portbuffers pb;

#define PLAY_LOCK_OK 0    /* p->s and p->w are safe */
#define PLAY_LOCK_START 1 /* p->s and p->w want to be unsafe */
#define PLAY_LOCK_CONT 2  /* p->s and p->w are unsafe */
typedef struct
{
	song        *s;
	window      *w;
	portpair     in, out[OUTPUT_GROUPS];
	jack_port_t *midiout;
	char         dirty;
	char         lock;  /* PLAY_LOCK */
} playbackinfo;
playbackinfo *p;


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



void clearChannel(channel *cv)
{
	cv->r.note = cv->samplernote = NOTE_VOID;
	cv->r.inst = cv->samplerinst = INST_VOID;
	cv->rtrigsamples = 0;
	if (cv->flags&C_FLAG_RTRIG_REV) cv->flags ^= C_FLAG_RTRIG_REV;
	cv->waveshaperstrength = 0; cv->targetwaveshaperstrength = -1;
	cv->gain = cv->randgain = 0x88; cv->targetgain = -1;
	if (cv->flags&C_FLAG_TARGET_RAND) cv->flags ^= C_FLAG_TARGET_RAND;
	cv->filtermode = 0; cv->targetfiltermode = -1;
	cv->filtercut = 255; cv->targetfiltercut = -1;
	cv->filterres = 0; cv->targetfilterres = -1;
	cv->midiccindex = -1; cv->midicc = 0; cv->targetmidicc = -1;
	cv->sendgroup = 0; cv->sendgain = 0; cv->targetsendgain = -1;
	cv->compressor = 0; cv->targetcompressor = -1;
}

void _addChannel(song *cs, channel *cv)
{
	cv->rampindex = rampmax;
	cv->rampbuffer = malloc(sizeof(short) * rampmax * 2); /* *2 for stereo */
	cv->stretchrampindex = stretchrampmax;
	clearChannel(cv);
}
void clearPatternChannel(song *cs, uint8_t rawpattern, uint8_t channel)
{
	memset(cs->patternv[rawpattern]->rowv[channel], 0, sizeof(row) * ROW_MAX);
	for (short r = 0; r < ROW_MAX; r++)
	{
		cs->patternv[rawpattern]->rowv[channel][r].note = NOTE_VOID;
		cs->patternv[rawpattern]->rowv[channel][r].inst = INST_VOID;
	} cs->patternv[rawpattern]->rowcc[channel] = cs->patternv[rawpattern]->rowc;
}
int addChannel(song *cs, uint8_t index)
{
	if (cs->channelc >= MAX_CHANNELS) return 1;
	if (cs->channelc > 1) /* contiguity */
		for (int i = cs->channelc-1; i >= index; i--)
		{
			for (uint8_t p = 1; p < cs->patternc; p++)
			{
				memcpy(cs->patternv[p]->rowv[i+1],
					cs->patternv[p]->rowv[i],
					sizeof(row) * ROW_MAX);
				cs->patternv[p]->rowcc[i+1] = cs->patternv[p]->rowcc[i];
			}
			memcpy(&cs->channelv[i + 1], &cs->channelv[i], sizeof(channel));
		}

	/* init new channel */
	for (uint8_t p = 1; p < cs->patternc; p++)
		clearPatternChannel(cs, p, index);
	memset(&cs->channelv[index], 0, sizeof(channel));
	_addChannel(cs, &cs->channelv[index]); /* allocate memory */
	cs->channelv[index].macroc = 1;

	cs->channelc++;
DEBUG=cs->channelc;
	return 0;
}
void _delChannel(song *cs, uint8_t index)
{
	if (cs->channelv[index].rampbuffer != NULL)
	{
		free(cs->channelv[index].rampbuffer);
		cs->channelv[index].rampbuffer = NULL;
	}
}
int delChannel(uint8_t index)
{
	uint8_t j;
	/* if there's only one channel then clear it */
	if (s->channelc == 1)
	{
		for (j = 1; j < s->patternc; j++)
		{
			memset(s->patternv[j]->rowv,
				0, sizeof(row) * ROW_MAX);
			for (short r = 0; r < ROW_MAX; r++)
			{
				s->patternv[j]->rowv[0][r].note = NOTE_VOID;
				s->patternv[j]->rowv[0][r].inst = INST_VOID;
			}
			s->patternv[j]->rowcc[0] = s->patternv[j]->rowc;
		}
		s->channelv[s->channelc].flags = C_FLAG_MUTE;
		s->channelv[s->channelc].macroc = 1;
		return 1;
	} else
	{
		_delChannel(s, index);
		for (int i = index; i < s->channelc-1; i++)
		{
			for (j = 1; j < s->patternc; j++)
			{
				memcpy(s->patternv[j]->rowv[i],
					s->patternv[j]->rowv[i+1],
					sizeof(row) * ROW_MAX);
				s->patternv[j]->rowcc[i] = s->patternv[j]->rowcc[i+1];
			}
			memcpy(&s->channelv[i],
				&s->channelv[i + 1],
				sizeof(channel));
		} s->channelc--;
		return 0;
	}
}
void yankChannel(uint8_t pattern, uint8_t channel)
{
	memcpy(&w->channelbufferrowv,
		s->patternv[s->patterni[pattern]]->rowv[channel],
		sizeof(row) * ROW_MAX);
	w->channelbufferrowcc = s->patternv[s->patterni[pattern]]->rowcc[channel];
	w->channelbuffermacroc = s->channelv[channel].macroc;
}
void putChannel(uint8_t pattern, uint8_t channel)
{
	memcpy(s->patternv[s->patterni[pattern]]->rowv[channel],
		&w->channelbufferrowv,
		sizeof(row) * ROW_MAX);
	s->patternv[s->patterni[pattern]]->rowcc[channel] = w->channelbufferrowcc;
	s->channelv[channel].macroc = MAX(s->channelv[channel].macroc, w->channelbuffermacroc);
}
void mixPutChannel(uint8_t pattern, uint8_t channel) /* TODO */
{
	memcpy(s->patternv[s->patterni[pattern]]->rowv[channel],
		&w->channelbufferrowv,
		sizeof(row) * ROW_MAX);

	/* TODO: should render polyrhythms */
	s->patternv[s->patterni[pattern]]->rowcc[channel] =
		MAX(w->channelbufferrowcc, s->patternv[s->patterni[pattern]]->rowcc[channel]);

	s->channelv[channel].macroc = MAX(s->channelv[channel].macroc, w->channelbuffermacroc);
}


/* memory for addPattern */
int _addPattern(song *cs, uint8_t realindex)
{
	cs->patternv[realindex] = calloc(1, sizeof(pattern));
	if (!cs->patternv[realindex]) return 1;

	for (short c = 0; c < MAX_CHANNELS; c++)
		for (short r = 0; r < ROW_MAX; r++)
		{
			cs->patternv[realindex]->rowv[c][r].note = NOTE_VOID;
			cs->patternv[realindex]->rowv[c][r].inst = INST_VOID;
		}
	return 0;
}
void copyPattern(pattern *dest, pattern *src)
{
	dest->rowc = src->rowc;
	memcpy(dest->rowcc, src->rowcc, sizeof(uint8_t) * MAX_CHANNELS);
	memcpy(dest->rowv, src->rowv, sizeof(row) * MAX_CHANNELS * ROW_MAX);
}
void _delPattern(pattern *pv)
{
	for (int i = 0; i < 128; i++)
	{
		if (pv->history[i])
		{
			free(pv->history[i]);
			pv->history[i] = NULL;
		}
	}
}
void pushPatternHistory(pattern *pv)
{
	if (!pv) return;

	if (pv->historyptr == 255) pv->historyptr = 128;
	else                       pv->historyptr++;

	if (pv->historybehind) pv->historybehind--;
	if (pv->historyahead) pv->historyahead = 0;

	if (pv->history[pv->historyptr%128])
	{
		_delPattern(pv->history[pv->historyptr%128]);
		free(pv->history[pv->historyptr%128]);
		pv->history[pv->historyptr%128] = NULL;
	}
	pv->history[pv->historyptr%128] = calloc(1, sizeof(pattern));
	copyPattern(pv->history[pv->historyptr%128], pv);

	pv->historychannel[pv->historyptr%128] = w->channel;
	pv->historyfy[pv->historyptr%128] = w->trackerfy;
	pv->historyfx[pv->historyptr%128] = w->trackerfx;
}
void pushPatternHistoryIfNew(pattern *pv)
{
	if (!pv) return;
	pattern *pvh = pv->history[pv->historyptr%128];
	if (!pvh) return;

	if (pvh->rowc != pv->rowc
			|| memcmp(pvh->rowcc, pv->rowcc, sizeof(uint8_t) * MAX_CHANNELS)
			|| memcmp(pvh->rowv, pv->rowv, sizeof(row) * MAX_CHANNELS * ROW_MAX))
		pushPatternHistory(pv);
}

void popPatternHistory(uint8_t realindex) /* undo */
{
	pattern *pv = s->patternv[realindex];
	if (!pv) return;

	if (pv->historyptr <= 1 || pv->historybehind >= 127)
	{ strcpy(w->command.error, "already at oldest change"); return; }
	pushPatternHistoryIfNew(pv);

	if (pv->historyptr == 128) pv->historyptr = 255;
	else                       pv->historyptr--;

	copyPattern(pv, pv->history[pv->historyptr%128]);
	w->channel = pv->historychannel[pv->historyptr%128];
	w->trackerfy = pv->historyfy[pv->historyptr%128];
	w->trackerfx = pv->historyfx[pv->historyptr%128];

	pv->historybehind++;
	pv->historyahead++;
}
void unpopPatternHistory(uint8_t realindex) /* redo */
{
	pattern *pv = s->patternv[realindex];
	if (!pv) return;
	if (pv->historyahead == 0)
	{ strcpy(w->command.error, "already at newest change"); return; }
	pushPatternHistoryIfNew(pv);

	if (pv->historyptr == 255)
		pv->historyptr = 128;
	else
		pv->historyptr++;

	copyPattern(pv, pv->history[pv->historyptr%128]);
	w->channel = pv->historychannel[pv->historyptr%128];
	w->trackerfy = pv->historyfy[pv->historyptr%128];
	w->trackerfx = pv->historyfx[pv->historyptr%128];

	pv->historybehind--;
	pv->historyahead--;
}
/* length 0 for the default */
int addPattern(uint8_t index)
{
	if (index == PATTERN_VOID) return 1; /* index invalid */
	if (s->patterni[index] > 0) return 1; /* index occupied */

	if (_addPattern(s, s->patternc))
	{
		strcpy(w->command.error, "failed to add pattern, out of memory");
		return 1;
	}

	s->patternv[s->patternc]->rowc = w->defpatternlength;
	for (short c = 0; c < MAX_CHANNELS; c++)
		s->patternv[s->patternc]->rowcc[c] = s->patternv[s->patternc]->rowc;

	s->patterni[index] = s->patternc;

	pushPatternHistory(s->patternv[s->patternc]);
	s->patternc++;
	return 0;
}
uint8_t emptySongIndex(uint8_t oldindex)
{
	for (int i = 0; i < PATTERN_MAX-1; i++) /* TODO: is the -1 correct? */
		if (!s->patterni[i])
			return i;
	return oldindex; /* no free slots */
}
int duplicatePattern(uint8_t oldindex)
{
	uint8_t index;
	int i;
	if ((index = emptySongIndex(oldindex)) != oldindex)
	{
		_addPattern(s, s->patternc);
		s->patterni[index] = s->patternc;
		s->patternc++;
		memcpy(s->patternv[s->patterni[index]],
				s->patternv[s->patterni[oldindex]],
				sizeof(pattern));
		for (i = 0; i < 128; i++)
		{
			if (s->patternv[s->patterni[oldindex]]->history[i])
			{
				s->patternv[s->patterni[index]]->history[i] = calloc(1, sizeof(pattern));
				copyPattern(s->patternv[s->patterni[oldindex]]->history[i], s->patternv[s->patterni[index]]->history[i]);
			}
		} return index;
	} else return oldindex;
}
int yankPattern(uint8_t index)
{
	if (!s->patterni[index]) return 1; /* nothing to yank */

	memcpy(&w->songbuffer,
			s->patternv[s->patterni[index]],
			sizeof(pattern));
	return 0;
}
int putPattern(uint8_t index)
{
	if (!s->patterni[index])
		if (addPattern(index)) return 1; /* allocate memory */

	memcpy(s->patternv[s->patterni[index]],
			&w->songbuffer,
			sizeof(pattern));

	if (s->patternv[s->patterni[index]]->rowc == 0)
	{
		s->patternv[s->patterni[index]]->rowc = w->defpatternlength;
		for (short c = 0; c < MAX_CHANNELS; c++)
			s->patternv[s->patternc]->rowcc[c] = w->defpatternlength;
	}
	return 0;
}
int delPattern(uint8_t index)
{
	if (s->patterni[index] == PATTERN_VOID) return 1; /* pattern doesn't exist */
	uint8_t cutindex = s->patterni[index];

	_delPattern(s->patternv[cutindex]);
	free(s->patternv[cutindex]); s->patternv[cutindex] = NULL;

	s->patterni[index] = 0;

	/* enforce contiguity */
	unsigned short i;
	for (i = cutindex; i < s->patternc - 1; i++)
		s->patternv[i] = s->patternv[i + 1];

	for (i = 0; i < PATTERN_MAX; i++) // for every backref index
		if (s->patterni[i] > cutindex && s->patterni[i] != 0)
			s->patterni[i]--;

	s->patternc--;
	return 0;
}

void renderPatternChannel(pattern *pv, uint8_t channel, uint16_t count)
{
	int max = pv->rowcc[channel];
	if (count) pv->rowcc[channel] = count - 1;
	else       pv->rowcc[channel] = pv->rowc;
	while (max < pv->rowcc[channel])
		if (max < (ROW_MAX>>1)-1)
		{
			memcpy(&pv->rowv[channel][max + 1], pv->rowv[channel],
					sizeof(row) * (max + 1));
			max = (max + 1) * 2 - 1;
		} else
		{
			memcpy(&pv->rowv[channel][max + 1], pv->rowv[channel],
					sizeof(row) * ((ROW_MAX-1) - max));
			max = (ROW_MAX-1); break;
		}
}

short tfxToVfx(short trackerfx)
{
	if (trackerfx > 1)
		return 2 + (trackerfx - 2) / 2;
	return trackerfx;
}
short vfxToTfx(short visualfx)
{
	if (visualfx > 1)
		return 2 + (visualfx - 2) * 2;
	return visualfx;
}
void yankPartPattern(short x1, short x2, short y1, short y2, uint8_t c1, uint8_t c2)
{
	w->pbfx[0] = x1; w->pbfy[0] = y1; w->pbchannel[0] = c1;
	w->pbfx[1] = x2; w->pbfy[1] = y2; w->pbchannel[1] = c2;
	w->pbpopulated = 1;

	pattern *destpattern = s->patternv[s->patterni[s->songi[w->songfy]]];
	memcpy(&w->patternbuffer, destpattern, sizeof(pattern));
	for (uint8_t i = c1; i <= c2; i++)
		for (uint8_t j = 1; j < ((unsigned short)destpattern->rowc+1) / ((unsigned short)destpattern->rowcc[i]+1); j++)
			memcpy(&w->patternbuffer.rowv[i][j * ((unsigned short)destpattern->rowcc[i]+1)],
					destpattern->rowv, sizeof(row) * (destpattern->rowcc[i]+1));
}
void putPartPattern(void)
{
	uint8_t i, j, row, channel;
	int k;
	if (!w->pbpopulated) return;
	pattern *destpattern = s->patternv[s->patterni[s->songi[w->songfy]]];
	if (w->pbchannel[0] == w->pbchannel[1]) /* only one channel */
	{
		if (w->pbfx[0] > 1 && w->pbfx[0] == w->pbfx[1]) /* just one macro column */
		{
			unsigned char targetmacro;
			if (w->trackerfx < 2) targetmacro = 0;
			else targetmacro = tfxToVfx(w->trackerfx)-2;
			for (uint8_t j = w->pbfy[0]; j <= w->pbfy[1]; j++)
			{
				if (w->trackerfy + j - w->pbfy[0] > destpattern->rowc) break;
				row = (w->trackerfy + j - w->pbfy[0]) % (destpattern->rowcc[w->channel]+1);
				destpattern->rowv[w->channel][row].macro[targetmacro].c = w->patternbuffer.rowv[w->pbchannel[0]][j].macro[w->pbfx[0]-2].c;
				destpattern->rowv[w->channel][row].macro[targetmacro].v = w->patternbuffer.rowv[w->pbchannel[0]][j].macro[w->pbfx[0]-2].v;
			}
			w->trackerfx = vfxToTfx(targetmacro + 2);
		} else
		{
			for (j = w->pbfy[0]; j <= w->pbfy[1]; j++)
			{
				if (w->trackerfy + j - w->pbfy[0] > destpattern->rowc) break;
				row = (w->trackerfy + j - w->pbfy[0]) % (destpattern->rowcc[w->channel]+1);
				if (w->pbfx[0] <= 0 && w->pbfx[1] >= 0) destpattern->rowv[w->channel][row].note = w->patternbuffer.rowv[w->pbchannel[0]][j].note;
				if (w->pbfx[0] <= 1 && w->pbfx[1] >= 1) destpattern->rowv[w->channel][row].inst = w->patternbuffer.rowv[w->pbchannel[0]][j].inst;
				for (k = 0; k <= s->channelv[w->channel].macroc; k++)
					if (w->pbfx[0] <= k+2 && w->pbfx[1] >= k+2)
					{
						destpattern->rowv[w->channel][row].macro[k].c = w->patternbuffer.rowv[w->pbchannel[0]][j].macro[k].c;
						destpattern->rowv[w->channel][row].macro[k].v = w->patternbuffer.rowv[w->pbchannel[0]][j].macro[k].v;
					}
			} w->trackerfx = vfxToTfx(w->pbfx[0]);
		}
	} else
	{
		for (i = w->pbchannel[0]; i <= w->pbchannel[1]; i++)
		{
			channel = w->channel + i - w->pbchannel[0];
			if (channel < s->channelc)
				for (j = w->pbfy[0]; j <= w->pbfy[1]; j++)
				{
					if (w->trackerfy + j - w->pbfy[0] > destpattern->rowc) break;
					row = (w->trackerfy + j - w->pbfy[0]) % (destpattern->rowcc[channel]+1);
					if (i == w->pbchannel[0]) /* first channel */
					{
						if (w->pbfx[0] <= 0) destpattern->rowv[channel][row].note = w->patternbuffer.rowv[i][j].note;
						if (w->pbfx[0] <= 1) destpattern->rowv[channel][row].inst = w->patternbuffer.rowv[i][j].inst;
						for (k = 0; k <= s->channelv[channel].macroc; k++)
							if (w->pbfx[0] <= k+2)
							{
								destpattern->rowv[channel][row].macro[k].c = w->patternbuffer.rowv[i][j].macro[k].c;
								destpattern->rowv[channel][row].macro[k].v = w->patternbuffer.rowv[i][j].macro[k].v;
							}
					} else if (i == w->pbchannel[1]) /* last channel */
					{
						if (w->pbfx[1] >= 0) destpattern->rowv[channel][row].note = w->patternbuffer.rowv[i][j].note;
						if (w->pbfx[1] >= 1) destpattern->rowv[channel][row].inst = w->patternbuffer.rowv[i][j].inst;
						for (k = 0; k <= s->channelv[channel].macroc; k++)
							if (w->pbfx[1] >= k+2)
							{
								destpattern->rowv[channel][row].macro[k].c = w->patternbuffer.rowv[i][j].macro[k].c;
								destpattern->rowv[channel][row].macro[k].v = w->patternbuffer.rowv[i][j].macro[k].v;
							}
					} else /* middle channel */
						destpattern->rowv[channel][row] = w->patternbuffer.rowv[i][j];
				}
			else break;
		} w->trackerfx = vfxToTfx(w->pbfx[0]);
	}
}
void mixPutPartPattern(void)
{
	uint8_t i, j, row, channel;
	int k;
	if (!w->pbpopulated) return;
	pattern *destpattern = s->patternv[s->patterni[s->songi[w->songfy]]];
	if (w->pbchannel[0] == w->pbchannel[1]) /* only one channel */
	{
		if (w->pbfx[0] > 1 && w->pbfx[0] == w->pbfx[1]) /* just one macro column */
		{
			unsigned char targetmacro;
			if (w->trackerfx < 2) targetmacro = 0;
			else targetmacro = tfxToVfx(w->trackerfx)-2;
			for (uint8_t j = w->pbfy[0]; j <= w->pbfy[1]; j++)
			{
				if (w->trackerfy + j - w->pbfy[0] > destpattern->rowc) break;
				row = (w->trackerfy + j - w->pbfy[0]) % (destpattern->rowcc[w->channel]+1);
				if (w->patternbuffer.rowv[w->pbchannel[0]][j].macro[w->pbfx[0]-2].c)
				{
					destpattern->rowv[w->channel][row].macro[targetmacro].c = w->patternbuffer.rowv[w->pbchannel[0]][j].macro[w->pbfx[0]-2].c;
					destpattern->rowv[w->channel][row].macro[targetmacro].v = w->patternbuffer.rowv[w->pbchannel[0]][j].macro[w->pbfx[0]-2].v;
				}
			}
			w->trackerfx = vfxToTfx(targetmacro + 2);
		} else
		{
			for (j = w->pbfy[0]; j <= w->pbfy[1]; j++)
			{
				if (w->trackerfy + j - w->pbfy[0] > destpattern->rowc) break;
				row = (w->trackerfy + j - w->pbfy[0]) % (destpattern->rowcc[w->channel]+1);
				if (w->pbfx[0] <= 0 && w->pbfx[1] >= 0 && w->patternbuffer.rowv[w->pbchannel[0]][j].note != NOTE_VOID) destpattern->rowv[w->channel][row].note = w->patternbuffer.rowv[w->pbchannel[0]][j].note;
				if (w->pbfx[0] <= 1 && w->pbfx[1] >= 1 && w->patternbuffer.rowv[w->pbchannel[0]][j].inst != INST_VOID) destpattern->rowv[w->channel][row].inst = w->patternbuffer.rowv[w->pbchannel[0]][j].inst;
				for (k = 0; k <= s->channelv[w->channel].macroc; k++)
					if (w->pbfx[0] <= k+2 && w->pbfx[1] >= k+2 && w->patternbuffer.rowv[w->pbchannel[0]][j].macro[k].c)
					{
						destpattern->rowv[w->channel][row].macro[k].c = w->patternbuffer.rowv[w->pbchannel[0]][j].macro[k].c;
						destpattern->rowv[w->channel][row].macro[k].v = w->patternbuffer.rowv[w->pbchannel[0]][j].macro[k].v;
					}
			} w->trackerfx = vfxToTfx(w->pbfx[0]);
		}
	} else
	{
		for (i = w->pbchannel[0]; i <= w->pbchannel[1]; i++)
		{
			channel = w->channel + i - w->pbchannel[0];
			if (channel < s->channelc)
				for (j = w->pbfy[0]; j <= w->pbfy[1]; j++)
				{
					if (w->trackerfy + j - w->pbfy[0] > destpattern->rowc) break;
					row = (w->trackerfy + j - w->pbfy[0]) % (destpattern->rowcc[channel]+1);
					if (i == w->pbchannel[0]) /* first channel */
					{
						if (w->pbfx[0] <= 0 && w->patternbuffer.rowv[i][j].note != NOTE_VOID) destpattern->rowv[channel][row].note = w->patternbuffer.rowv[i][j].note;
						if (w->pbfx[0] <= 1 && w->patternbuffer.rowv[i][j].inst != INST_VOID) destpattern->rowv[channel][row].inst = w->patternbuffer.rowv[i][j].inst;
						for (k = 0; k <= s->channelv[channel].macroc; k++)
							if (w->pbfx[0] <= k+2 && w->patternbuffer.rowv[i][j].macro[k].c)
							{
								destpattern->rowv[channel][row].macro[k].c = w->patternbuffer.rowv[i][j].macro[k].c;
								destpattern->rowv[channel][row].macro[k].v = w->patternbuffer.rowv[i][j].macro[k].v;
							}
					} else if (i == w->pbchannel[1]) /* last channel */
					{
						if (w->pbfx[1] >= 0 && w->patternbuffer.rowv[i][j].note != NOTE_VOID) destpattern->rowv[channel][row].note = w->patternbuffer.rowv[i][j].note;
						if (w->pbfx[1] >= 1 && w->patternbuffer.rowv[i][j].inst != INST_VOID) destpattern->rowv[channel][row].inst = w->patternbuffer.rowv[i][j].inst;
						for (k = 0; k <= s->channelv[channel].macroc; k++)
							if (w->pbfx[1] >= k+2 && w->patternbuffer.rowv[i][j].macro[k].c)
							{
								destpattern->rowv[channel][row].macro[k].c = w->patternbuffer.rowv[i][j].macro[k].c;
								destpattern->rowv[channel][row].macro[k].v = w->patternbuffer.rowv[i][j].macro[k].v;
							}
					} else /* middle channel */
					{
						if (w->patternbuffer.rowv[i][j].note != NOTE_VOID) destpattern->rowv[channel][row].note = w->patternbuffer.rowv[i][j].note;
						if (w->patternbuffer.rowv[i][j].inst != INST_VOID) destpattern->rowv[channel][row].inst = w->patternbuffer.rowv[i][j].inst;
						for (k = 0; k <= s->channelv[channel].macroc; k++)
							if (w->patternbuffer.rowv[i][j].macro[k].c)
							{
								destpattern->rowv[channel][row].macro[k].c = w->patternbuffer.rowv[i][j].macro[k].c;
								destpattern->rowv[channel][row].macro[k].v = w->patternbuffer.rowv[i][j].macro[k].v;
							}
					}
				}
			else break;
		} w->trackerfx = vfxToTfx(w->pbfx[0]);
	}
}
void delPartPattern(short x1, short x2, short y1, short y2, uint8_t c1, uint8_t c2)
{
	uint8_t i, j, row;
	int k;
	pattern *destpattern = s->patternv[s->patterni[s->songi[w->songfy]]];
	if (c1 == c2) /* only one channel */
	{
		for (j = y1; j <= y2; j++)
		{
			row = (y1 + j - w->pbfy[0]) % (destpattern->rowcc[c1]+1);
			if (x1 <= 0 && x2 >= 0) destpattern->rowv[c1][row].note = NOTE_VOID;
			if (x1 <= 1 && x2 >= 1) destpattern->rowv[c1][row].inst = INST_VOID;
			for (k = 0; k <= s->channelv[c1].macroc; k++)
				if (x1 <= k+2 && x2 >= k+2)
				{
					destpattern->rowv[c1][row].macro[k].c = 0;
					destpattern->rowv[c1][row].macro[k].v = 0;
				}
		}
	} else
		for (i = c1; i <= c2; i++)
			if (i < s->channelc)
				for (j = y1; j <= y2; j++)
				{
					row = (y1 + j - w->pbfy[0]) % (destpattern->rowcc[w->channel]+1);
					if (i == c1) /* first channel */
					{
						if (x1 <= 0) destpattern->rowv[i][row].note = NOTE_VOID;
						if (x1 <= 1) destpattern->rowv[i][row].inst = INST_VOID;
						for (k = 0; k <= s->channelv[i].macroc; k++)
							if (x1 <= k+2)
							{
								destpattern->rowv[i][row].macro[k].c = 0;
								destpattern->rowv[i][row].macro[k].v = 0;
							}
					} else if (i == c2) /* last channel */
					{
						if (x2 >= 0) destpattern->rowv[i][row].note = NOTE_VOID;
						if (x2 >= 1) destpattern->rowv[i][row].inst = INST_VOID;
						for (k = 0; k <= s->channelv[i].macroc; k++)
							if (x2 >= k+2)
							{
								destpattern->rowv[i][row].macro[k].c = 0;
								destpattern->rowv[i][row].macro[k].v = 0;
							}
					} else /* middle channel */
					{
						memset(&destpattern->rowv[i][row], 0, sizeof(row));
						destpattern->rowv[i][row].note = NOTE_VOID;
						destpattern->rowv[i][row].inst = INST_VOID;
					}
				}
			else break;
}
/* block inc/dec */
void addPartPattern(signed char value, short x1, short x2, short y1, short y2, uint8_t c1, uint8_t c2)
{
	uint8_t i, j, row;
	int k;
	pattern *destpattern = s->patternv[s->patterni[s->songi[w->songfy]]];
	if (c1 == c2) /* only one channel */
	{
		for (j = y1; j <= y2; j++)
		{
			row = j % (destpattern->rowcc[c1]+1);
			if (x1 <= 0 && x2 >= 0) destpattern->rowv[c1][row].note += value;
			if (x1 <= 1 && x2 >= 1) destpattern->rowv[c1][row].inst += value;
			for (k = 0; k <= s->channelv[c1].macroc; k++)
				if (x1 <= k+2 && x2 >= k+2)
					switch (destpattern->rowv[c1][row].macro[k].c)
					{
						case 'G': destpattern->rowv[c1][row].macro[k].v += value*16;
						default:  destpattern->rowv[c1][row].macro[k].v += value;
					}
		}
	} else
		for (i = c1; i <= c2; i++)
			if (i < s->channelc)
				for (j = y1; j <= y2; j++)
				{
					row = j % (destpattern->rowcc[i]+1);
					if (i == c1) /* first channel */
					{
						if (x1 <= 0) destpattern->rowv[i][row].note += value;
						if (x1 <= 1) destpattern->rowv[i][row].inst += value;
						for (k = 0; k <= s->channelv[i].macroc; k++)
							if (x1 <= k+2)
								switch (destpattern->rowv[i][row].macro[k].c)
								{
									case 'G': destpattern->rowv[i][row].macro[k].v += value*16;
									default:  destpattern->rowv[i][row].macro[k].v += value;
								}
					} else if (i == c2) /* last channel */
					{
						if (x2 >= 0) destpattern->rowv[i][row].note += value;
						if (x2 >= 1) destpattern->rowv[i][row].inst += value;
						for (k = 0; k <= s->channelv[i].macroc; k++)
							if (x2 >= k+2)
								switch (destpattern->rowv[i][row].macro[k].c)
								{
									case 'G': destpattern->rowv[i][row].macro[k].v += value*16;
									default:  destpattern->rowv[i][row].macro[k].v += value;
								}
					} else /* middle channel */
					{
						destpattern->rowv[i][row].note += value;
						destpattern->rowv[i][row].inst += value;
						for (k = 0; k <= s->channelv[i].macroc; k++)
							switch (destpattern->rowv[i][row].macro[k].c)
							{
								case 'G': destpattern->rowv[i][row].macro[k].v += value*16;
								default:  destpattern->rowv[i][row].macro[k].v += value;
							}
					}
				}
			else break;
}
/* block toggle case */
void tildePartPattern(short x1, short x2, short y1, short y2, uint8_t c1, uint8_t c2)
{
	uint8_t i, j, row;
	int k;
	pattern *destpattern = s->patternv[s->patterni[s->songi[w->songfy]]];
	if (c1 == c2) /* only one channel */
	{
		for (j = y1; j <= y2; j++)
		{
			row = j % (destpattern->rowcc[c1]+1);
			for (k = 0; k <= s->channelv[c1].macroc; k++)
				if (x1 <= k+2 && x2 >= k+2)
				{
					if      (isupper(destpattern->rowv[c1][row].macro[k].c))
						changeMacro(destpattern->rowv[c1][row].macro[k].c,
								&destpattern->rowv[c1][row].macro[k].c);
					else if (islower(destpattern->rowv[c1][row].macro[k].c))
						changeMacro(destpattern->rowv[c1][row].macro[k].c,
								&destpattern->rowv[c1][row].macro[k].c);
				}
		}
	} else
		for (i = c1; i <= c2; i++)
			if (i < s->channelc)
				for (j = y1; j <= y2; j++)
				{
					row = j % (destpattern->rowcc[i]+1);
					if (i == c1) /* first channel */
						for (k = 0; k <= s->channelv[i].macroc; k++) {
							if (x1 <= k+2)
							{
								if      (isupper(destpattern->rowv[i][row].macro[k].c))
									changeMacro(destpattern->rowv[i][row].macro[k].c,
											&destpattern->rowv[i][row].macro[k].c);
								else if (islower(destpattern->rowv[i][row].macro[k].c))
									changeMacro(destpattern->rowv[i][row].macro[k].c,
											&destpattern->rowv[i][row].macro[k].c);
							}}
					else if (i == c2) /* last channel */
						for (k = 0; k <= s->channelv[i].macroc; k++) {
							if (x2 >= k+2)
							{
								if      (isupper(destpattern->rowv[i][row].macro[k].c))
									changeMacro(destpattern->rowv[i][row].macro[k].c,
											&destpattern->rowv[i][row].macro[k].c);
								else if (islower(destpattern->rowv[i][row].macro[k].c))
									changeMacro(destpattern->rowv[i][row].macro[k].c,
											&destpattern->rowv[i][row].macro[k].c);
							}}
					else /* middle channel */
						for (k = 0; k <= s->channelv[i].macroc; k++)
						{
							if      (isupper(destpattern->rowv[i][row].macro[k].c))
								changeMacro(destpattern->rowv[i][row].macro[k].c,
										&destpattern->rowv[i][row].macro[k].c);
							else if (islower(destpattern->rowv[i][row].macro[k].c))
								changeMacro(destpattern->rowv[i][row].macro[k].c,
										&destpattern->rowv[i][row].macro[k].c);
						}
				}
			else break;
}
/* block interpolate */
void interpolatePartPattern(short x1, short x2, short y1, short y2, uint8_t c1, uint8_t c2)
{
	uint8_t i, j, row;
	short row1, row2;
	int k;
	pattern *pv = s->patternv[s->patterni[s->songi[w->songfy]]];
	if (c1 == c2) /* only one channel */
	{
		row1 = y1 % (pv->rowcc[c1]+1);
		row2 = y2 % (pv->rowcc[c1]+1);
		for (j = y1; j <= y2; j++)
		{
			row = j % (pv->rowcc[c1]+1);
			if (x1 <= 0 && x2 >= 0)
			{
				if      (pv->rowv[c1][row1].note == NOTE_VOID) pv->rowv[c1][row].note = pv->rowv[c1][row2].note;
				else if (pv->rowv[c1][row2].note == NOTE_VOID) pv->rowv[c1][row].note = pv->rowv[c1][row1].note;
				else pv->rowv[c1][row].note = pv->rowv[c1][row1].note + ((pv->rowv[c1][row2].note - pv->rowv[c1][row1].note) / (float)(y2-y1)) * (j-y1);
			}
			if (x1 <= 1 && x2 >= 1)
			{
				if      (pv->rowv[c1][row1].inst == INST_VOID) pv->rowv[c1][row].inst = pv->rowv[c1][row2].inst;
				else if (pv->rowv[c1][row2].inst == INST_VOID) pv->rowv[c1][row].inst = pv->rowv[c1][row1].inst;
				else pv->rowv[c1][row].inst = pv->rowv[c1][row1].inst + ((pv->rowv[c1][row2].inst - pv->rowv[c1][row1].inst) / (float)(y2-y1)) * (j-y1);
			}
			for (k = 0; k <= s->channelv[c1].macroc; k++)
				if (x1 <= k+2 && x2 >= k+2)
				{
					pv->rowv[c1][row].macro[k].c = (pv->rowv[c1][row1].macro[k].c) ? pv->rowv[c1][row1].macro[k].c : pv->rowv[c1][row2].macro[k].c;
					pv->rowv[c1][row].macro[k].v = pv->rowv[c1][row1].macro[k].v + ((pv->rowv[c1][row2].macro[k].v - pv->rowv[c1][row1].macro[k].v) / (float)(y2-y1)) * (j-y1);
				}
		}
	} else
		for (i = c1; i <= c2; i++)
			if (i < s->channelc) {
				row1 = y1 % (pv->rowcc[i]+1);
				row2 = y2 % (pv->rowcc[i]+1);
				for (j = y1; j <= y2; j++)
				{
					row = j % (pv->rowcc[i]+1);
					if (i == c1) /* first channel */
					{
						if (x1 <= 0) pv->rowv[i][row].note = pv->rowv[i][row1].note + ((pv->rowv[i][row2].note - pv->rowv[i][row1].note) / (float)(y2-y1)) * (j-y1);
						if (x1 <= 1) pv->rowv[i][row].inst = pv->rowv[i][row1].inst + ((pv->rowv[i][row2].inst - pv->rowv[i][row1].inst) / (float)(y2-y1)) * (j-y1);
						for (k = 0; k <= s->channelv[i].macroc; k++)
							if (x1 <= k+2)
							{
								pv->rowv[i][row].macro[k].c = (pv->rowv[i][row1].macro[k].c) ? pv->rowv[i][row1].macro[k].c : pv->rowv[i][row2].macro[k].c;
								pv->rowv[i][row].macro[k].v = pv->rowv[i][row1].macro[k].v + ((pv->rowv[i][row2].macro[k].v - pv->rowv[i][row1].macro[k].v) / (float)(y2-y1)) * (j-y1);
							}
					}
					else if (i == c2) /* last channel */
					{
						if (x2 >= 0) pv->rowv[i][row].note = pv->rowv[i][row1].note + ((pv->rowv[i][row2].note - pv->rowv[i][row1].note) / (float)(y2-y1)) * (j-y1);
						if (x2 >= 1) pv->rowv[i][row].inst = pv->rowv[i][row1].inst + ((pv->rowv[i][row2].inst - pv->rowv[i][row1].inst) / (float)(y2-y1)) * (j-y1);
						for (k = 0; k <= s->channelv[i].macroc; k++)
							if (x2 >= k+2)
							{
								pv->rowv[i][row].macro[k].c = (pv->rowv[i][row1].macro[k].c) ? pv->rowv[i][row1].macro[k].c : pv->rowv[i][row2].macro[k].c;
								pv->rowv[i][row].macro[k].v = pv->rowv[i][row1].macro[k].v + ((pv->rowv[i][row2].macro[k].v - pv->rowv[i][row1].macro[k].v) / (float)(y2-y1)) * (j-y1);
							}
					}
					else /* middle channel */
					{
						pv->rowv[i][row].note = pv->rowv[i][row1].note + ((pv->rowv[i][row2].note - pv->rowv[i][row1].note) / (float)(y2-y1)) * (j-y1);
						pv->rowv[i][row].inst = pv->rowv[i][row1].inst + ((pv->rowv[i][row2].inst - pv->rowv[i][row1].inst) / (float)(y2-y1)) * (j-y1);
						for (k = 0; k <= s->channelv[i].macroc; k++)
						{
							pv->rowv[i][row].macro[k].c = (pv->rowv[i][row1].macro[k].c) ? pv->rowv[i][row1].macro[k].c : pv->rowv[i][row2].macro[k].c;
							pv->rowv[i][row].macro[k].v = pv->rowv[i][row1].macro[k].v + ((pv->rowv[i][row2].macro[k].v - pv->rowv[i][row1].macro[k].v) / (float)(y2-y1)) * (j-y1);
						}
					}
				}
			} else break;
}
/* block randomize */
void randPartPattern(short x1, short x2, short y1, short y2, uint8_t c1, uint8_t c2)
{
	uint8_t i, j, row, randinst;
	int k;
	pattern *destpattern = s->patternv[s->patterni[s->songi[w->songfy]]];
	if (c1 == c2) /* only one channel */
	{
		for (j = y1; j <= y2; j++)
		{
			row = j % (destpattern->rowcc[c1]+1);
			if (x1 <= 0 && x2 >= 0) destpattern->rowv[c1][row].note = MIN(NOTE_A10-1, rand()%36 +MIN(7, w->octave)*12);
			if (x1 <= 1 && x2 >= 1 && destpattern->rowv[c1][row].note != NOTE_VOID)
			{
				destpattern->rowv[c1][row].inst = NOTE_VOID;
				randinst = rand()%(s->instrumentc-1) + 1;
				for (k = 0; k < INSTRUMENT_MAX; k++)
					if (s->instrumenti[k] == randinst)
					{ destpattern->rowv[c1][row].inst = k; break; }
			}
			for (k = 0; k <= s->channelv[c1].macroc; k++)
				if (x1 <= k+2 && x2 >= k+2 && destpattern->rowv[c1][row].macro[k].c)
					destpattern->rowv[c1][row].macro[k].v = rand()%256;
		}
	} else
	{
		for (i = c1; i <= c2; i++)
			if (i < s->channelc)
				for (j = y1; j <= y2; j++)
				{
					row = j % (destpattern->rowcc[i]+1);
					if (i == c1) /* first channel */
					{
						if (x1 <= 0) destpattern->rowv[i][row].note = MIN(NOTE_A10-1, rand()%36 +MIN(7, w->octave)*12);
						if (x1 <= 1 && destpattern->rowv[i][row].note != NOTE_VOID)
						{
							destpattern->rowv[i][row].inst = NOTE_VOID;
							randinst = rand()%(s->instrumentc-1) + 1;
							for (k = 0; k < INSTRUMENT_MAX; k++)
								if (s->instrumenti[k] == randinst)
								{ destpattern->rowv[i][row].inst = k; break; }
						}
						for (k = 0; k <= s->channelv[i].macroc; k++)
							if (x1 <= k+2 && destpattern->rowv[i][row].macro[k].c)
								destpattern->rowv[i][row].macro[k].v = rand()%256;
					} else if (i == c2) /* last channel */
					{
						if (x2 >= 0) destpattern->rowv[i][row].note = MIN(NOTE_A10-1, rand()%36 +MIN(7, w->octave)*12);
						if (x2 >= 1 && destpattern->rowv[i][row].note != NOTE_VOID)
						{
							destpattern->rowv[i][row].inst = NOTE_VOID;
							randinst = rand()%(s->instrumentc-1) + 1;
							for (k = 0; k < INSTRUMENT_MAX; k++)
								if (s->instrumenti[k] == randinst)
								{ destpattern->rowv[i][row].inst = k; break; }
						}
						for (k = 0; k <= s->channelv[i].macroc; k++)
							if (x2 >= k+2 && destpattern->rowv[i][row].macro[k].c)
								destpattern->rowv[i][row].macro[k].v = rand()%256;
					} else /* middle channel */
					{
						destpattern->rowv[i][row].note = MIN(NOTE_A10-1, rand()%36 +MIN(7, w->octave)*12);
						if (destpattern->rowv[i][row].note != NOTE_VOID)
						{
							destpattern->rowv[i][row].inst = NOTE_VOID;
							randinst = rand()%(s->instrumentc-1) + 1;
							for (k = 0; k < INSTRUMENT_MAX; k++)
								if (s->instrumenti[k] == randinst)
								{ destpattern->rowv[i][row].inst = k; break; }
						}
						for (k = 0; k <= s->channelv[i].macroc; k++)
							if (destpattern->rowv[i][row].macro[k].c)
								destpattern->rowv[i][row].macro[k].v = rand()%256;
					}
				}
			else break;
	}
}
void cycleUpPartPattern(short x1, short x2, short y1, short y2, uint8_t c1, uint8_t c2)
{
	if (s->songi[w->songfy] == PATTERN_VOID) return;

	uint8_t i;
	int j, k, l;
	pattern *destpattern = s->patternv[s->patterni[s->songi[w->songfy]]];
	row hold;
	if (c1 == c2) /* only one channel */
	{
		for (l = 0; l < MAX(1, w->count); l++)
		{
			hold = destpattern->rowv[c1][y1];
			if (x1 <= 0 && x2 >= 0)
			{
				for (j = y1; j < y2; j++) destpattern->rowv[c1][j].note = destpattern->rowv[c1][j+1].note;
				destpattern->rowv[c1][y2].note = hold.note;
			}
			if (x1 <= 1 && x2 >= 1)
			{
				for (j = y1; j < y2; j++) destpattern->rowv[c1][j].inst = destpattern->rowv[c1][j+1].inst;
				destpattern->rowv[c1][y2].inst = hold.inst;
			}
			for (k = 0; k <= s->channelv[c1].macroc; k++)
				if (x1 <= k+2 && x2 >= k+2)
				{
					for (j = y1; j < y2; j++)
					{
						destpattern->rowv[c1][j].macro[k].c = destpattern->rowv[c1][j+1].macro[k].c;
						destpattern->rowv[c1][j].macro[k].v = destpattern->rowv[c1][j+1].macro[k].v;
					}
					destpattern->rowv[c1][y2].macro[k].c = hold.macro[k].c;
					destpattern->rowv[c1][y2].macro[k].v = hold.macro[k].v;
				}
		}
	} else
	{
		for (i = c1; i <= c2; i++)
			if (i < s->channelc) {
				if (i == c1) /* first channel */
					for (l = 0; l < MAX(1, w->count); l++)
					{
						hold = destpattern->rowv[c1][y1];
						if (x1 <= 0)
						{
							for (j = y1; j < y2; j++) destpattern->rowv[c1][j].note = destpattern->rowv[c1][j+1].note;
							destpattern->rowv[c1][y2].note = hold.note;
						}
						if (x1 <= 1)
						{
							for (j = y1; j < y2; j++) destpattern->rowv[c1][j].inst = destpattern->rowv[c1][j+1].inst;
							destpattern->rowv[c1][y2].inst = hold.inst;
						}
						for (k = 0; k <= s->channelv[c1].macroc; k++)
							if (x1 <= k+2)
							{
								for (j = y1; j < y2; j++)
								{
									destpattern->rowv[c1][j].macro[k].c = destpattern->rowv[c1][j+1].macro[k].c;
									destpattern->rowv[c1][j].macro[k].v = destpattern->rowv[c1][j+1].macro[k].v;
								}
								destpattern->rowv[c1][y2].macro[k].c = hold.macro[k].c;
								destpattern->rowv[c1][y2].macro[k].v = hold.macro[k].v;
							}
					}
				else if (i == c2) /* last channel */
					for (l = 0; l < MAX(1, w->count); l++)
					{
						hold = destpattern->rowv[c2][y1];
						if (x2 >= 0)
						{
							for (j = y1; j < y2; j++) destpattern->rowv[c2][j].note = destpattern->rowv[c2][j+1].note;
							destpattern->rowv[c2][y2].note = hold.note;
						}
						if (x2 >= 1)
						{
							for (j = y1; j < y2; j++) destpattern->rowv[c2][j].inst = destpattern->rowv[c2][j+1].inst;
							destpattern->rowv[c2][y2].inst = hold.inst;
						}
						for (k = 0; k <= s->channelv[c2].macroc; k++)
							if (x2 >= k+2)
							{
								for (j = y1; j < y2; j++)
								{
									destpattern->rowv[c2][j].macro[k].c = destpattern->rowv[c2][j+1].macro[k].c;
									destpattern->rowv[c2][j].macro[k].v = destpattern->rowv[c2][j+1].macro[k].v;
								}
								destpattern->rowv[c2][y2].macro[k].c = hold.macro[k].c;
								destpattern->rowv[c2][y2].macro[k].v = hold.macro[k].v;
							}
					}
				else /* middle channel */
					for (l = 0; l < MAX(1, w->count); l++)
					{
						hold = destpattern->rowv[i][y1];
						for (j = y1; j < y2; j++) destpattern->rowv[i][j] = destpattern->rowv[i][j+1];
						destpattern->rowv[i][y2] = hold;
					}
			} else break;
	}
}
void cycleDownPartPattern(short x1, short x2, short y1, short y2, uint8_t c1, uint8_t c2)
{
	if (s->songi[w->songfy] == PATTERN_VOID) return;

	uint8_t i;
	int j, k, l;
	pattern *destpattern = s->patternv[s->patterni[s->songi[w->songfy]]];
	row hold;
	if (c1 == c2) /* only one channel */
	{
		for (l = 0; l < MAX(1, w->count); l++)
		{
			hold = destpattern->rowv[c1][y2];
			if (x1 <= 0 && x2 >= 0)
			{
				for (j = y2-1; j >= y1; j--) destpattern->rowv[c1][j+1].note = destpattern->rowv[c1][j].note;
				destpattern->rowv[c1][y1].note = hold.note;
			}
			if (x1 <= 1 && x2 >= 1)
			{
				for (j = y2-1; j >= y1; j--) destpattern->rowv[c1][j+1].inst = destpattern->rowv[c1][j].inst;
				destpattern->rowv[c1][y1].inst = hold.inst;
			}
			for (k = 0; k <= s->channelv[c1].macroc; k++)
				if (x1 <= k+2 && x2 >= k+2)
				{
					for (j = y2-1; j >= y1; j--)
					{
						destpattern->rowv[c1][j+1].macro[k].c = destpattern->rowv[c1][j].macro[k].c;
						destpattern->rowv[c1][j+1].macro[k].v = destpattern->rowv[c1][j].macro[k].v;
					}
					destpattern->rowv[c1][y1].macro[k].c = hold.macro[k].c;
					destpattern->rowv[c1][y1].macro[k].v = hold.macro[k].v;
				}
		}
	} else
	{
		for (i = c1; i <= c2; i++)
			if (i < s->channelc) {
				if (i == c1) /* first channel */
					for (l = 0; l < MAX(1, w->count); l++)
					{
						hold = destpattern->rowv[c1][y2];
						if (x1 <= 0)
						{
							for (j = y2-1; j >= y1; j--) destpattern->rowv[c1][j+1].note = destpattern->rowv[c1][j].note;
							destpattern->rowv[c1][y1].note = hold.note;
						}
						if (x1 <= 1)
						{
							for (j = y2-1; j >= y1; j--) destpattern->rowv[c1][j+1].inst = destpattern->rowv[c1][j].inst;
							destpattern->rowv[c1][y1].inst = hold.inst;
						}
						for (k = 0; k <= s->channelv[c1].macroc; k++)
							if (x1 <= k+2)
							{
								for (j = y2-1; j >= y1; j--)
								{
									destpattern->rowv[c1][j+1].macro[k].c = destpattern->rowv[c1][j].macro[k].c;
									destpattern->rowv[c1][j+1].macro[k].v = destpattern->rowv[c1][j].macro[k].v;
								}
								destpattern->rowv[c1][y1].macro[k].c = hold.macro[k].c;
								destpattern->rowv[c1][y1].macro[k].v = hold.macro[k].v;
							}
					}
				else if (i == c2) /* last channel */
					for (l = 0; l < MAX(1, w->count); l++)
					{
						hold = destpattern->rowv[c2][y2];
						if (x2 >= 0)
						{
							for (j = y2-1; j >= y1; j--) destpattern->rowv[c2][j+1].note = destpattern->rowv[c2][j].note;
							destpattern->rowv[c2][y1].note = hold.note;
						}
						if (x2 >= 1)
						{
							for (j = y2-1; j >= y1; j--) destpattern->rowv[c2][j+1].inst = destpattern->rowv[c2][j].inst;
							destpattern->rowv[c2][y1].inst = hold.inst;
						}
						for (k = 0; k <= s->channelv[c2].macroc; k++)
							if (x2 >= k+2)
							{
								for (j = y2-1; j >= y1; j--)
								{
									destpattern->rowv[c2][j+1].macro[k].c = destpattern->rowv[c2][j].macro[k].c;
									destpattern->rowv[c2][j+1].macro[k].v = destpattern->rowv[c2][j].macro[k].v;
								}
								destpattern->rowv[c2][y1].macro[k].c = hold.macro[k].c;
								destpattern->rowv[c2][y1].macro[k].v = hold.macro[k].v;
							}
					}
				else /* middle channel */
					for (l = 0; l < MAX(1, w->count); l++)
					{
						hold = destpattern->rowv[i][y2];
						for (j = y2-1; j >= y1; j--) destpattern->rowv[i][j+1] = destpattern->rowv[i][j];
						destpattern->rowv[i][y1] = hold;
					}
			} else break;
	}
}


void copyInstrument(instrument *dest, instrument *src)
{
	dest->samplelength = src->samplelength;
	dest->length = src->length;
	dest->channels = src->channels;
	dest->c5rate = src->c5rate;
	dest->samplerate = src->samplerate;
	dest->bitdepth = src->bitdepth;
	dest->cyclelength = src->cyclelength;
	dest->pitchshift = src->pitchshift;
	dest->trim[0] = src->trim[0]; dest->trim[1] = src->trim[1];
	dest->loop = src->loop;
	dest->envelope = src->envelope;
	dest->gain = src->gain;
	dest->flags = src->flags;
	dest->loopramp = src->loopramp;
	dest->midichannel = src->loopramp;

	if (dest->sampledata)
	{ free(dest->sampledata); dest->sampledata = NULL; }

	if (src->sampledata)
	{ /* only copy sampledata if it exists */
		dest->sampledata = malloc(sizeof(short) * src->samplelength);
		memcpy(dest->sampledata, src->sampledata, sizeof(short) * src->samplelength);
	}
}

void asyncInstrumentUpdate(song *cs)
{
	uint8_t i;
	if (w->instrumentlockv == INST_GLOBAL_LOCK_FREE
			|| w->instrumentlockv == INST_GLOBAL_LOCK_HIST
			|| w->instrumentlockv == INST_GLOBAL_LOCK_PUT)
		i = w->instrumentlocki;
	else return;

	instrument *iv = cs->instrumentv[i];
	if (iv)
	{
		switch (w->instrumentlockv)
		{
			case INST_GLOBAL_LOCK_HIST:
				copyInstrument(iv, iv->history[iv->historyptr%128]);
				w->instrumentindex = iv->historyindex[iv->historyptr%128];
				break;
			case INST_GLOBAL_LOCK_PUT:
				copyInstrument(iv, &w->instrumentbuffer);
				break;
		}
	}

	w->instrumentlockv = INST_GLOBAL_LOCK_OK; /* mark as free to use */
	redraw();
}

void _delInstrument(instrument *iv)
{
	if (iv->sampledata) { free(iv->sampledata); iv->sampledata = NULL; }
	for (int i = 0; i < 128; i++)
	{
		if (!iv->history[i]) continue;
		if (iv->history[i]->sampledata)
		{
			free(iv->history[i]->sampledata);
			iv->history[i]->sampledata = NULL;
		}
		free(iv->history[i]);
		iv->history[i] = NULL;
	}
}
void pushInstrumentHistory(instrument *iv)
{
	if (!iv) return;

	if (iv->historyptr == 255) iv->historyptr = 128;
	else                       iv->historyptr++;

	if (iv->historybehind) iv->historybehind--;
	if (iv->historyahead)  iv->historyahead = 0;

	if (iv->history[iv->historyptr%128])
	{
		_delInstrument(iv->history[iv->historyptr%128]);
		free(iv->history[iv->historyptr%128]);
	}
	iv->history[iv->historyptr%128] = calloc(1, sizeof(instrument));
	copyInstrument(iv->history[iv->historyptr%128], iv);

	iv->historyindex[iv->historyptr%128] = w->instrumentindex;
}
void pushInstrumentHistoryIfNew(instrument *iv)
{
	if (!iv) return;

	instrument *ivh = iv->history[iv->historyptr%128];
	if (!ivh) return;

	/* TODO: maybe check sampledata for changes too, or have a way to force a push if sampledata has been changed */
	if (ivh->samplelength != iv->samplelength
			|| ivh->length != iv->length
			|| ivh->channels != iv->channels
			|| ivh->c5rate != iv->c5rate
			|| ivh->samplerate != iv->samplerate
			|| ivh->bitdepth != iv->bitdepth
			|| ivh->cyclelength != iv->cyclelength
			|| ivh->pitchshift != iv->pitchshift
			|| ivh->trim[0] != iv->trim[0] || ivh->trim[1] != iv->trim[1]
			|| ivh->loop != iv->loop
			|| ivh->envelope != iv->envelope
			|| ivh->gain != iv->gain
			|| ivh->flags != iv->flags
			|| ivh->loopramp != iv->loopramp
			|| ivh->midichannel != iv->midichannel)
		pushInstrumentHistory(iv);
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
	pushInstrumentHistoryIfNew(iv);

	if (iv->historyptr == 128) iv->historyptr = 255;
	else                       iv->historyptr--;

	_popInstrumentHistory(realindex);

	iv->historybehind++;
	iv->historyahead++;
}
void unpopInstrumentHistory(uint8_t realindex) /* redo */
{
	instrument *iv = s->instrumentv[realindex];
	if (!iv) return;
	if (iv->historyahead == 0)
	{
		strcpy(w->command.error, "already at newest change");
		return;
	}
	pushInstrumentHistoryIfNew(iv);

	if (iv->historyptr == 255) iv->historyptr = 128;
	else                       iv->historyptr++;

	_popInstrumentHistory(realindex);

	iv->historybehind--;
	iv->historyahead--;
}
int addInstrument(uint8_t index)
{
	if (s->instrumenti[index] > 0) return 1; /* index occupied */

	s->instrumentv[s->instrumentc] = calloc(1, sizeof(instrument));
	if (!s->instrumentv[s->instrumentc])
	{
		strcpy(w->command.error, "failed to add instrument, out of memory");
		return 1;
	}
	s->instrumentv[s->instrumentc]->gain = 0x20;
	s->instrumentv[s->instrumentc]->samplerate = 0xff;
	s->instrumentv[s->instrumentc]->bitdepth = 0xf;
	s->instrumentv[s->instrumentc]->loopramp = 0xff;
	s->instrumentv[s->instrumentc]->cyclelength = 0x37ff;
	s->instrumentv[s->instrumentc]->pitchshift = 0x80;
	s->instrumenti[index] = s->instrumentc;

	pushInstrumentHistory(s->instrumentv[s->instrumentc]);
	s->instrumentc++;
	return 0;
}
int yankInstrument(uint8_t index)
{
	if (!s->instrumenti[index]) return 1; /* nothing to yank */
	_delInstrument(&w->instrumentbuffer);
	copyInstrument(&w->instrumentbuffer, s->instrumentv[s->instrumenti[index]]);
	return 0;
}
int delInstrument(uint8_t index)
{
	if (s->instrumenti[index] < 1) return 1; /* instrument doesn't exist */

	uint8_t cutindex = s->instrumenti[index];
	s->instrumenti[index] = 0;

	_delInstrument(s->instrumentv[cutindex]);
	free(s->instrumentv[cutindex]); s->instrumentv[cutindex] = NULL;

	/* enforce contiguity */
	for (uint8_t i = cutindex; i < s->instrumentc-1; i++)
		s->instrumentv[i] = s->instrumentv[i+1];

	for (uint8_t i = 0; i < 255; i++) // for every backref index
		if (s->instrumenti[i] >= cutindex)
			s->instrumenti[i]--;

	s->instrumentc--;
	return 0;
}


void toggleRecording(uint8_t inst, char cue)
{
	if (w->instrumentrecv == INST_REC_LOCK_OK) w->instrumentreci = inst;
	if (w->instrumentreci == inst)
	{
		switch (w->instrumentrecv)
		{
			case INST_REC_LOCK_OK:
				w->recbuffer = malloc(sizeof(short) * RECORD_LENGTH * samplerate * 2);
				if (!w->recbuffer)
				{
					strcpy(w->command.error, "failed to start recording, out of memory");
					break;
				}
				w->recptr = 0;
				if (cue) w->instrumentrecv = INST_REC_LOCK_CUE_START;
				else     w->instrumentrecv = INST_REC_LOCK_START;
				break;
			default: w->instrumentrecv = INST_REC_LOCK_PREP_END; break;
		}
	} redraw();
}
void recordBinds(short instrument, int input)
{
	switch (input)
	{
		case 'R': case 'r': /* start/stop */
			if (w->instrumentrecv != INST_REC_LOCK_OK && w->instrumentreci != instrument)
			{ /* stop whichever instrument is already recording */
				toggleRecording(w->instrumentreci, 0);
			} else
			{
				if (!s->instrumenti[instrument]) addInstrument(instrument);
				toggleRecording(instrument, 0);
			} break;
		case 'Q': case 'q': /* start/stop cue */
			if (w->instrumentrecv != INST_REC_LOCK_OK && w->instrumentreci != instrument)
			{ /* stop whichever instrument is already recording */
				toggleRecording(w->instrumentreci, 1);
			} else
			{
				if (!s->instrumenti[instrument]) addInstrument(instrument);
				toggleRecording(instrument, 1);
			} break;
		case 'C': case 'c': /* cancel */
			if (w->instrumentrecv != INST_REC_LOCK_OK)
				w->instrumentrecv = INST_REC_LOCK_PREP_CANCEL;
			break;
	}
}


short *_loadSample(char *path, SF_INFO *sfinfo)
{
	fcntl(0, F_SETFL, 0); /* blocking */
	memset(sfinfo, 0, sizeof(SF_INFO));

	SNDFILE *sndfile = sf_open(path, SFM_READ, sfinfo);
	short *ptr;

	if (!sndfile)
	{ /* raw file */
		struct stat buf;
		stat(path, &buf);

		ptr = malloc(buf.st_size - buf.st_size % sizeof(short));
		if (!ptr) // malloc failed
		{
			fcntl(0, F_SETFL, O_NONBLOCK); /* non-blocking */
			return NULL;
		} else
		{
			/* read the whole file into memory */
			FILE *fp = fopen(path, "r");
			fread(ptr, sizeof(short), buf.st_size / sizeof(short), fp);
			fclose(fp);

			/* spoof data */
			sfinfo->channels = 1;
			sfinfo->frames = buf.st_size / sizeof(short);
			sfinfo->samplerate = 12000;
		}
	} else /* audio file */
	{
		if (sfinfo->channels > 2) /* fail on high channel files */
		{
			sf_close(sndfile);
			fcntl(0, F_SETFL, O_NONBLOCK); /* non-blocking */
			return NULL;
		}

		ptr = malloc(sizeof(short) * sfinfo->frames * sfinfo->channels);
		if (!ptr) // malloc failed
		{
			sf_close(sndfile);
			fcntl(0, F_SETFL, O_NONBLOCK); /* non-blocking */
			return NULL;
		} else
		{
			/* read the whole file into memory */
			sf_readf_short(sndfile, ptr, sfinfo->frames);
			sf_close(sndfile);
		}
	}
	fcntl(0, F_SETFL, O_NONBLOCK); /* non-blocking */
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
	if (iv->samplelength) free(iv->sampledata);
	iv->sampledata = sampledata;
	iv->samplelength = sfinfo.frames * sfinfo.channels;
	iv->channels = sfinfo.channels;
	iv->length = sfinfo.frames;
	iv->c5rate = sfinfo.samplerate;
	iv->trim[0] = 0;
	iv->trim[1] = sfinfo.frames-1;
	iv->loop = sfinfo.frames-1;
	iv->samplerate = 0xff;
	iv->bitdepth = 0xf;
}

song *_addSong(void)
{
	song *cs = calloc(1, sizeof(song));
	if (!cs) return NULL;

	cs->patternc = 1;
	cs->instrumentc = 1;

	cs->rowhighlight = 4;
	cs->songbpm = DEF_BPM;
	w->request = REQ_BPM;

	memset(cs->songi, PATTERN_VOID, sizeof(uint8_t) * SONG_MAX);

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
	for (int i = 0; i < cs->channelc; i++)
		_delChannel(cs, i);

	for (int i = 1; i < cs->patternc; i++)
	{
		_delPattern(cs->patternv[i]);
		free(cs->patternv[i]);
	}

	for (int i = 1; i < cs->instrumentc; i++)
	{
		_delInstrument(cs->instrumentv[i]);
		free(cs->instrumentv[i]);
	}

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
	} return ret;
}

int writeSong(char *path)
{
	char *pathext = fileExtension(path, ".omlm");
	if (!strcmp(pathext, ".omlm"))
	{
		free(pathext);
		if (!strlen(w->filepath))
		{
			strcpy(w->command.error, "no file name");
			return 1;
		}
		pathext = malloc(sizeof(w->filepath) + 1);
		strcpy(pathext, w->filepath);
	} else strcpy(w->filepath, pathext);

	fcntl(0, F_SETFL, 0); /* blocking */
	FILE *fp = fopen(pathext, "wb");
	int i, j, k, l;

	/* egg, for each and every trying time (the most important) */
	fputc('e', fp); fputc('g', fp); fputc('g', fp);

	/* version */
	fputc(MAJOR, fp); fputc(MINOR, fp);

	/* counts */
	fputc(s->songbpm, fp);
	fputc(s->patternc, fp);
	fputc(s->instrumentc, fp);
	fputc(s->channelc, fp);
	fputc(s->rowhighlight, fp);

	/* mutes */
	uint8_t byte;
	for (i = 0; i < 4; i++)
	{
		byte = 0b00000000;
		if (s->channelv[i * 8 + 0].flags&C_FLAG_MUTE) byte |= 0b10000000;
		if (s->channelv[i * 8 + 1].flags&C_FLAG_MUTE) byte |= 0b01000000;
		if (s->channelv[i * 8 + 2].flags&C_FLAG_MUTE) byte |= 0b00100000;
		if (s->channelv[i * 8 + 3].flags&C_FLAG_MUTE) byte |= 0b00010000;
		if (s->channelv[i * 8 + 4].flags&C_FLAG_MUTE) byte |= 0b00001000;
		if (s->channelv[i * 8 + 5].flags&C_FLAG_MUTE) byte |= 0b00000100;
		if (s->channelv[i * 8 + 6].flags&C_FLAG_MUTE) byte |= 0b00000010;
		if (s->channelv[i * 8 + 7].flags&C_FLAG_MUTE) byte |= 0b00000001;
		fputc(byte, fp);
	}
	for (i = 0; i < s->channelc; i++)
		fputc(s->channelv[i].macroc, fp);

	/* songi       */ for (i = 0; i < SONG_MAX; i++) fputc(s->songi[i], fp);
	/* songf       */ for (i = 0; i < SONG_MAX; i++) fputc(s->songf[i], fp);
	/* patterni    */ for (i = 0; i < PATTERN_MAX; i++) fputc(s->patterni[i], fp);
	/* instrumenti */ for (i = 0; i < INSTRUMENT_MAX; i++) fputc(s->instrumenti[i], fp);

	/* patternv */
	for (i = 1; i < s->patternc; i++)
	{
		fputc(s->patternv[i]->rowc, fp);
		for (j = 0; j < s->channelc; j++)
		{
			fputc(s->patternv[i]->rowcc[j], fp);
			char macroc = s->channelv[j].macroc;
			for (k = 0; k < s->patternv[i]->rowcc[j] + 1; k++)
			{
				fputc(s->patternv[i]->rowv[j][k].note, fp);
				fputc(s->patternv[i]->rowv[j][k].inst, fp);
				for (l = 0; l <= macroc; l++)
				{
					fputc(s->patternv[i]->rowv[j][k].macro[l].c, fp);
					fputc(s->patternv[i]->rowv[j][k].macro[l].v, fp);
				}
			}
		}
	}

	/* instrumentv */
	instrument *iv;
	for (i = 1; i < s->instrumentc; i++)
	{
		iv = s->instrumentv[i];
		fwrite(&iv->length, sizeof(uint32_t), 1, fp);
		fwrite(&iv->channels, sizeof(uint8_t), 1, fp);
		fwrite(&iv->c5rate, sizeof(uint32_t), 1, fp);
		fwrite(&iv->samplerate, sizeof(uint8_t), 1, fp);
		fwrite(&iv->bitdepth, sizeof(uint8_t), 1, fp);
		fwrite(&iv->cyclelength, sizeof(uint16_t), 1, fp);
		fwrite(&iv->pitchshift, sizeof(uint8_t), 1, fp);
		fwrite(iv->trim, sizeof(uint32_t), 2, fp);
		fwrite(&iv->loop, sizeof(uint32_t), 1, fp);
		fwrite(&iv->envelope, sizeof(uint8_t), 1, fp);
		fwrite(&iv->outputgroup, sizeof(uint8_t), 1, fp);
		fwrite(&iv->gain, sizeof(uint8_t), 1, fp);
		fwrite(&iv->channelmode, sizeof(uint8_t), 1, fp);
		fwrite(&iv->flags, sizeof(uint8_t), 1, fp);
		fwrite(&iv->samplelength, sizeof(uint32_t), 1, fp);
		fwrite(&iv->loopramp, sizeof(uint8_t), 1, fp);
		fwrite(&iv->midichannel, sizeof(uint8_t), 1, fp);
		if (iv->samplelength)
			fwrite(iv->sampledata, sizeof(short), iv->samplelength, fp);
	}

	fclose(fp);
	fcntl(0, F_SETFL, O_NONBLOCK); /* non-blocking */
	snprintf(w->command.error, COMMAND_LENGTH, "'%s' written", pathext);
	free(pathext);
	return 0;
}
song *readSong(char *path)
{
	fcntl(0, F_SETFL, 0); /* blocking */
	FILE *fp = fopen(path, "r");
	if (!fp) // file doesn't exist, or fopen otherwise failed
	{
		strcpy(w->filepath, path);
		redraw(); return NULL;
	}
	DIR *dp = opendir(path);
	if (dp) // file is a directory
	{
		closedir(dp); fclose(fp);
		fcntl(0, F_SETFL, O_NONBLOCK); /* non-blocking */
		snprintf(w->command.error, COMMAND_LENGTH, "file '%s' is a directory", path);
		redraw(); return NULL;
	}

	int i, j, k, l;

	/* ensure egg wasn't broken in transit */
	/* the most important check */
	if (!(fgetc(fp) == 'e' && fgetc(fp) == 'g' && fgetc(fp) == 'g'))
	{
		fclose(fp);
		fcntl(0, F_SETFL, O_NONBLOCK); /* non-blocking */
		snprintf(w->command.error, COMMAND_LENGTH, "file '%s' isn't valid", path);
		redraw(); return NULL;
	}

	/* version */
	unsigned char filemajor = fgetc(fp);
	unsigned char fileminor = fgetc(fp);
	if (filemajor == 0 && fileminor < 86)
	{
		fclose(fp);
		fcntl(0, F_SETFL, O_NONBLOCK);
		strcpy(w->command.error, "failed to read song, file is FAR too old");
		redraw(); return NULL;
	}

	song *cs = _addSong();
	if (!cs)
	{
		fclose(fp);
		fcntl(0, F_SETFL, O_NONBLOCK); /* non-blocking */
		strcpy(w->command.error, "failed to read song, out of memory");
		redraw(); return NULL;
	}

	/* assume the rest of the file is valid */
	/* TODO: proper error checking lol */
	strcpy(w->filepath, path);

	/* counts */
	cs->songbpm = fgetc(fp);
	w->request = REQ_BPM;

	cs->patternc = fgetc(fp);
	cs->instrumentc = fgetc(fp);
	cs->channelc = fgetc(fp);
	for (i = 0; i < cs->channelc; i++) _addChannel(cs, &cs->channelv[i]);
	cs->rowhighlight = fgetc(fp);

	/* mutes */
	char byte;
	for (i = 0; i < 4; i++)
	{
		byte = fgetc(fp);
		if (byte & 0b10000000) cs->channelv[i * 8 + 0].flags = C_FLAG_MUTE;
		if (byte & 0b01000000) cs->channelv[i * 8 + 1].flags = C_FLAG_MUTE;
		if (byte & 0b00100000) cs->channelv[i * 8 + 2].flags = C_FLAG_MUTE;
		if (byte & 0b00010000) cs->channelv[i * 8 + 3].flags = C_FLAG_MUTE;
		if (byte & 0b00001000) cs->channelv[i * 8 + 4].flags = C_FLAG_MUTE;
		if (byte & 0b00000100) cs->channelv[i * 8 + 5].flags = C_FLAG_MUTE;
		if (byte & 0b00000010) cs->channelv[i * 8 + 6].flags = C_FLAG_MUTE;
		if (byte & 0b00000001) cs->channelv[i * 8 + 7].flags = C_FLAG_MUTE;
	}
	for (i = 0; i < cs->channelc; i++)
	{
		cs->channelv[i].macroc = fgetc(fp);
		if (filemajor == 0 && fileminor < 94) cs->channelv[i].macroc--;
	}

	/* songi    */ for (i = 0; i < SONG_MAX; i++) cs->songi[i] = fgetc(fp);
	/* songf    */ for (i = 0; i < SONG_MAX; i++) cs->songf[i] = fgetc(fp);
	/* patterni */ for (i = 0; i < PATTERN_MAX; i++) cs->patterni[i] = fgetc(fp);
	/* instrumenti */
	for (i = 0; i < INSTRUMENT_MAX; i++) cs->instrumenti[i] = fgetc(fp);
	if (filemajor == 0 && fileminor < 93) fgetc(fp);

	/* patternv */
	for (i = 1; i < cs->patternc; i++)
	{
		_addPattern(cs, i);
		cs->patternv[i]->rowc = fgetc(fp);
		for (j = 0; j < cs->channelc; j++)
		{
			if (!(filemajor == 0 && fileminor < 85)) cs->patternv[i]->rowcc[j] = fgetc(fp);
			char macroc = cs->channelv[j].macroc;
			for (k = 0; k < cs->patternv[i]->rowcc[j] + 1; k++)
			{
				cs->patternv[i]->rowv[j][k].note = fgetc(fp);
				if (filemajor == 0 && fileminor < 88) cs->patternv[i]->rowv[j][k].note--;
				cs->patternv[i]->rowv[j][k].inst = fgetc(fp);
				for (l = 0; l <= macroc; l++)
				{
					cs->patternv[i]->rowv[j][k].macro[l].c = fgetc(fp);
					cs->patternv[i]->rowv[j][k].macro[l].v = fgetc(fp);
				}
			}
		} pushPatternHistory(cs->patternv[i]);
	}

	/* instrumentv */
	instrument *iv;
	for (i = 1; i < cs->instrumentc; i++)
	{
		cs->instrumentv[i] = calloc(1, sizeof(instrument));

		iv = cs->instrumentv[i];
		if (filemajor == 0 && fileminor < 89) fseek(fp, 1, SEEK_CUR);
		fread(&iv->length, sizeof(uint32_t), 1, fp);
		fread(&iv->channels, sizeof(uint8_t), 1, fp);
		fread(&iv->c5rate, sizeof(uint32_t), 1, fp);
		fread(&iv->samplerate, sizeof(uint8_t), 1, fp);
		if (!(filemajor == 0 && fileminor < 87)) fread(&iv->bitdepth, sizeof(uint8_t), 1, fp);
		fread(&iv->cyclelength, sizeof(uint16_t), 1, fp);
		fread(&iv->pitchshift, sizeof(uint8_t), 1, fp);
		if (filemajor == 0 && fileminor < 83) fseek(fp, sizeof(uint16_t), SEEK_CUR);
		fread(iv->trim, sizeof(uint32_t), 2, fp);
		fread(&iv->loop, sizeof(uint32_t), 1, fp);
		if (filemajor == 0 && fileminor < 92) fseek(fp, sizeof(uint32_t), SEEK_CUR);
		if (filemajor == 0 && fileminor < 89) fseek(fp, sizeof(uint8_t), SEEK_CUR);
		else fread(&iv->envelope, sizeof(uint8_t), 1, fp);
		if (filemajor == 0 && fileminor < 91) fseek(fp, sizeof(uint8_t), SEEK_CUR);
		else fread(&iv->outputgroup, sizeof(uint8_t), 1, fp);
		fread(&iv->gain, sizeof(uint8_t), 1, fp);
		if (!(filemajor == 0 && fileminor < 89)) fread(&iv->channelmode, sizeof(uint8_t), 1, fp);
		fread(&iv->flags, sizeof(uint8_t), 1, fp);
		fread(&iv->samplelength, sizeof(uint32_t), 1, fp);
		fread(&iv->loopramp, sizeof(uint8_t), 1, fp);
		if (filemajor == 0 && fileminor < 87)
		{ /* old 8-bit toggle */
			if (iv->flags & 0b100) iv->bitdepth = 0x8;
			else                   iv->bitdepth = 0xf;
			iv->flags |= 0b100; iv->flags ^= 0b100; /* default to sustain on */
		} else fread(&iv->midichannel, sizeof(uint8_t), 1, fp);
		if (iv->samplelength)
		{
			iv->sampledata = malloc(sizeof(short) * iv->samplelength);
			fread(iv->sampledata, sizeof(short), iv->samplelength, fp);
		} pushInstrumentHistory(cs->instrumentv[i]);
	}

	fclose(fp);
	fcntl(0, F_SETFL, O_NONBLOCK); /* non-blocking */
	redraw(); return cs;
}
