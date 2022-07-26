#define MAX_VALUE_LEN 128
#define MAX_CHANNELS 33 /* TODO: index 0 is unused */

typedef struct
{
	char    c; /* command */
	uint8_t v; /* argument */
} macro;
#define NOTE_VOID 255
#define NOTE_OFF 254
#define NOTE_UNUSED 253 /* explicitly unused */
#define C5 60   /* centre note */
#define A10 120 /* first out of range note */
#define INST_VOID 255
typedef struct
{
	uint8_t note; /* 0-127: MIDI compatible, 255: void, 254: note off */
	uint8_t inst;
	macro   macro[8];
} row;
typedef struct
{
	uint8_t rowc;
	uint8_t rowcc[MAX_CHANNELS];     /* rowc per-channel */
	row     rowv[MAX_CHANNELS][256]; /* MAX_CHANNELS channels, each with 256 rows */
} pattern;

typedef struct
{
	char        mute;                         /* saved to disk */
	uint8_t     macroc;                       /* macro count */

	char        reverse;                      /* decrement pointer instead of incrementing it */
	uint32_t    pointer;                      /* progress through the sound */
	uint32_t    pointeroffset;                /* where to base pointer off of */
	uint32_t    releasepointer;               /* 0 for no release, where releasing started */
	short       gain;                         /* unsigned nibble per-channel, -1 for unset */
	short       targetgain;                   /* smooth gain target */
	row         r;
	float       finetune;                     /* calculated fine tune, should be between -0.5 and +0.5 */
	float       portamentofinetune;           /* used by portamento, clamped between -2/+2 for midi */
	float       microtonalfinetune;           /* used by the local microtonal macro */
	uint8_t     portamento;                   /* portamento target, 255 for off */
	uint8_t     portamentospeed;              /* portamento m */
	uint16_t    rtrigsamples;                 /* samples per retrigger */
	uint32_t    rtrigpointer;                 /* sample ptr to ratchet back to */
	uint8_t     rtrigblocksize;               /* number of rows block extends to */
	uint16_t    cutsamples;                   /* samples into the row to cut, 0 for no cut */
	uint16_t    delaysamples;                 /* samples into the row to delay, 0 for no delay */
	uint8_t     delaynote;
	uint8_t     delayinst;
	uint8_t     vibrato;                      /* vibrato depth, 0-f */
	uint32_t    vibratosamples;               /* samples per full phase walk */
	uint32_t    vibratosamplepointer;         /* distance through cv->vibratosamples */
	uint8_t     gate;                         /* gain m */
	float       gateopen;

	/* waveshaper */
	char        waveshaper;                   /* which waveshaper to use */
	char        waveshaperstrength;           /* mix / input gain */

	/* filter */
	SVFilter    fl, fr;
	char        filtermode;
	uint8_t     filtercut;
	short       targetfiltercut;
	signed char filterres, targetfilterres;

	/* ramping */
	uint16_t    rampindex;                    /* progress through the ramp buffer, rampmax if not ramping */
	short      *rampbuffer;                   /* samples to ramp out */
	uint8_t     rampgain;                     /* raw gain m for the ramp buffer */

	uint16_t    stretchrampindex;             /* progress through the stretch ramp buffer, >=localstretchrampmax if not ramping */
	uint16_t    localstretchrampmax;          /* actual stretchrampmax used, to allow for tiny buffer sizes */
} channel;

typedef struct Instrument
{
	short              *sampledata;                   /* variable size, persists between types */
	uint32_t            samplelength;                 /* raw samples allocated for sampledata */

	uint32_t            length;
	uint8_t             channels;
	uint32_t            c5rate;
	uint8_t             samplerate;                   /* percent of c5rate to actually use */
	uint8_t             bitdepth;
	uint16_t            cyclelength;
	uint8_t             pitchshift;
	uint32_t            trim[2];
	uint32_t            loop[2];
	uint8_t             envelope;
	uint8_t             gain;
	uint8_t             channelmode;
	uint8_t             flags;
	uint8_t             loopramp;
	uint8_t             midichannel;

	struct Instrument  *history[128];                 /* instrument snapshots */
	short               historyindex[128];            /* cursor positions for instrument snapshots */
	uint8_t             historyptr;                   /* highest bit is an overflow bit */
	uint8_t             historybehind;                /* tracks how many less than 128 safe indices there are */
	uint8_t             historyahead;                 /* tracks how many times it's safe to redo */
} instrument;

#define PLAYING_STOP 0
#define PLAYING_START 1
#define PLAYING_CONT 2
#define PLAYING_PREP_STOP 3
typedef struct
{
	uint8_t         patternc;                /* pattern count */
	uint8_t         patterni[256];           /* pattern backref */
	pattern        *patternv[256];           /* pattern values */

	uint8_t         instrumentc;             /* instrument count */
	uint8_t         instrumenti[256];        /* instrument backref */
	instrument     *instrumentv[256];        /* instrument values */

	uint8_t         channelc;                /* channel count */
	channel         channelv[32];            /* channel values */

	uint8_t         songi[256];              /* song list backref, links to patterns */
	uint8_t         songf[256];              /* song list flags */

	uint8_t         songp;                   /* song pos, analogous to window->songfy */
	short           songr;                   /* song row, analogous to window->trackerfy */

	uint8_t         rowhighlight;
	uint8_t         bpm;
	uint8_t         songbpm;                 /* to store the song's bpm through bpm change macros */
	uint16_t        spr;                     /* samples per row (samplerate * (60 / bpm) / (rowhighlight * 2)) */
	uint16_t        sprp;                    /* samples per row progress */
	char            playing;
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
#define INST_REC_LOCK_START 1       /* playback and most memory ops are safe */
#define INST_REC_LOCK_CONT 2        /* recording                             */
#define INST_REC_LOCK_PREP_END 3    /* start stopping recording              */
#define INST_REC_LOCK_END 4         /* stopping recording has finished       */
#define INST_REC_LOCK_PREP_CANCEL 5 /* start cancelling recording            */
#define INST_REC_LOCK_CANCEL 6      /* cancelling recording has finished     */

#define REQ_OK 0  /* do nothing / done */
#define REQ_BPM 1 /* re-apply the song bpm */
typedef struct
{
	pattern        songbuffer;                   /* full pattern paste buffer, TODO: use */
	pattern        patternbuffer;                /* partial pattern paste buffer */
	short          pbfy[2], pbfx[2];             /* partial pattern paste buffer clipping region */
	uint8_t        pbchannel[2];                 /* " */
	char           pbpopulated;                  /* there's no good way to tell if pb is set */
	instrument     instrumentbuffer;             /* instrument paste buffer */
	uint8_t        defpatternlength;

	uint8_t        songibuffer[256];             /* song list paste buffer */
	uint8_t        songfbuffer[256];             /* song list flags paste buffer */
	uint8_t        songbufferlen;                /* how much of song[i,f]buffer has meaningful data */

	char           filepath[COMMAND_LENGTH];

	void         (*filebrowserCallback)(char *); /* arg is the selected path */
	command_t      command;
	unsigned char  popup;
	unsigned char  mode, oldmode;
	unsigned short centre;
	uint8_t        pattern;                      /* focused pattern */
	uint8_t        channel;                      /* focused channel */
	uint8_t        channeloffset, visiblechannels;
	row            channelbufferrowv[256];
	uint8_t        channelbufferrowcc;
	uint8_t        channelbuffermacroc;

	short          trackerfy, trackerfx;
	short          visualfy, visualfx;
	uint8_t        visualchannel;
	short          instrumentindex;
	short          instrument;                   /* focused instrument */
	unsigned short instrumentcelloffset;
	unsigned short instrumentrowoffset;

	int            filebrowserindex;

	unsigned short mousey, mousex;

	short          fyoffset;
	signed char    fieldpointer;

	char           dirpath[NAME_MAX + 1];
	unsigned int   dirc;
	unsigned short dirmaxwidth;
	unsigned char  dircols;
	DIR           *dir;

	Canvas        *waveformcanvas;
	char         **waveformbuffer;
	size_t         waveformw, waveformh;
	uint32_t       waveformwidth;
	uint32_t       waveformcursor;
	uint32_t       waveformvisual;
	uint32_t       waveformdrawpointer;

	short          songfy;

	char           chord;                        /* key chord buffer, vi-style multi-letter commands */
	uint16_t       count;                        /* action repeat count, follows similar rules to w->chord */
	char           octave;
	uint8_t        step;
	char           keyboardmacro;
	uint8_t        flags;                        /* %1:follow */

	uint8_t        songnext;

	row            previewrow;
	uint8_t        previewchannelsrc;
	channel        previewchannel;
	char           previewtrigger;               /* 0:cut
	                                                1:start inst
	                                                2:still inst */

	uint8_t        instrumentlocki;              /* realindex */
	uint8_t        instrumentlockv;              /* value, set to an INST_GLOBAL_LOCK constant */
	char           request;                      /* ask the playback function to do something */

	uint8_t        instrumentreci;               /* realindex */
	uint8_t        instrumentrecv;               /* value, set to an INST_REC_LOCK constant */
	short         *recbuffer;                    /* disallow changing the type or removing while recording */
	uint32_t       recptr;
	uint8_t        recordflags;                  /* %2: gate around the next pattern */

	char           newfilename[COMMAND_LENGTH];  /* used by readSong */
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
typedef struct
{
	portbufferpair in;
	portbufferpair out;
	void *midiout;
} portbuffers;

typedef struct
{
	song        *s;
	window      *w;
	portpair     in, out;
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



void _addChannel(song *cs, channel *cv)
{
	cv->rampindex = rampmax;
	cv->rampbuffer = malloc(sizeof(short) * rampmax * 2); /* *2 for stereo */
	cv->stretchrampindex = stretchrampmax;
	cv->filtercut = 1.0f;
	cv->r.note = NOTE_VOID;
}
void clearPatternChannel(song *cs, uint8_t rawpattern, uint8_t channel)
{
	memset(cs->patternv[rawpattern]->rowv[channel], 0, sizeof(row) * 256);
	for (short r = 0; r < 256; r++)
	{
		cs->patternv[rawpattern]->rowv[channel][r].note = NOTE_VOID;
		cs->patternv[rawpattern]->rowv[channel][r].inst = INST_VOID;
	} cs->patternv[rawpattern]->rowcc[channel] = cs->patternv[rawpattern]->rowc;
}
int addChannel(song *cs, uint8_t index)
{
	if (cs->channelc >= MAX_CHANNELS - 1) return 1;
	_addChannel(cs, &cs->channelv[cs->channelc]); /* allocate memory */
	if (cs->channelc > 0) /* contiguity */
		for (uint8_t i = cs->channelc; i >= index; i--)
		{
			for (uint8_t p = 1; p < cs->patternc; p++)
			{
				memcpy(cs->patternv[p]->rowv[i+1],
					cs->patternv[p]->rowv[i],
					sizeof(row) * 256);
				cs->patternv[p]->rowcc[i+1] = cs->patternv[p]->rowcc[i];
			}
			cs->channelv[i + 1].mute = cs->channelv[i].mute;
			cs->channelv[i + 1].macroc = cs->channelv[i].macroc;
		}

	cs->channelv[index].mute = 0;
	cs->channelv[index].macroc = 2;

	for (uint8_t p = 1; p < cs->patternc; p++)
		clearPatternChannel(cs, p, index);

	cs->channelc++;
	return 0;
}
void _delChannel(song *cs, uint8_t index)
{
	free(cs->channelv[index].rampbuffer); cs->channelv[index].rampbuffer = NULL;
}
int delChannel(uint8_t index)
{
	uint8_t i, j;
	/* if there's only one channel then clear it */
	if (s->channelc == 1)
	{
		for (j = 1; j < s->patternc; j++)
		{
			memset(s->patternv[j]->rowv,
				0, sizeof(row) * 256);
			for (short r = 0; r < 256; r++)
			{
				s->patternv[j]->rowv[0][r].note = NOTE_VOID;
				s->patternv[j]->rowv[0][r].inst = INST_VOID;
			}
			s->patternv[j]->rowcc[0] = s->patternv[j]->rowc;
		}
		s->channelv[s->channelc].mute = 0;
		s->channelv[s->channelc].macroc = 2;
		return 1;
	} else
	{
		_delChannel(s, index);

		for (i = index; i < s->channelc; i++)
		{
			for (j = 1; j < s->patternc; j++)
			{
				memcpy(s->patternv[j]->rowv[i],
					s->patternv[j]->rowv[i+1],
					sizeof(row) * 256);
				s->patternv[j]->rowcc[i] = s->patternv[j]->rowcc[i+1];
			}
			memcpy(&s->channelv[i],
				&s->channelv[i + 1],
				sizeof(channel));
		}
		s->channelc--;

		// jack_deactivate(client);
		/* jack_port_unregister(client, p->cin[s->channelc].l);
		jack_port_unregister(client, p->cin[s->channelc].r);
		jack_port_unregister(client, p->cout[s->channelc].l);
		jack_port_unregister(client, p->cout[s->channelc].r); */
		// jack_activate(client);

		if (w->channeloffset)
			w->channeloffset--;
		return 0;
	}
}
void yankChannel(uint8_t pattern, uint8_t channel)
{
	memcpy(&w->channelbufferrowv,
		s->patternv[s->patterni[pattern]]->rowv[channel],
		sizeof(row) * 256);
	w->channelbufferrowcc = s->patternv[s->patterni[pattern]]->rowcc[channel];
	w->channelbuffermacroc = s->channelv[channel].macroc;
}
void putChannel(uint8_t pattern, uint8_t channel)
{
	memcpy(s->patternv[s->patterni[pattern]]->rowv[channel],
		&w->channelbufferrowv,
		sizeof(row) * 256);
	s->patternv[s->patterni[pattern]]->rowcc[channel] = w->channelbufferrowcc;
	s->channelv[channel].macroc = MAX(s->channelv[channel].macroc, w->channelbuffermacroc);
}
void mixPutChannel(uint8_t pattern, uint8_t channel) /* TODO */
{
	memcpy(s->patternv[s->patterni[pattern]]->rowv[channel],
		&w->channelbufferrowv,
		sizeof(row) * 256);

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
		for (short r = 0; r < 256; r++)
		{
			cs->patternv[realindex]->rowv[c][r].note = NOTE_VOID;
			cs->patternv[realindex]->rowv[c][r].inst = INST_VOID;
		}
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
	else            s->patternv[s->patternc]->rowc = w->defpatternlength;
	for (short c = 0; c < MAX_CHANNELS; c++)
		s->patternv[s->patternc]->rowcc[c] = s->patternv[s->patternc]->rowc;

	s->patterni[index] = s->patternc;
	s->patternc++;
	return 0;
}
uint8_t emptySongIndex(uint8_t oldindex)
{
	for (int i = 0; i < 255; i++)
		if (!s->patterni[i])
			return i;
	return oldindex; /* no free slots */
}
int duplicatePattern(uint8_t oldindex)
{
	uint8_t index = emptySongIndex(oldindex);
	if (index != oldindex)
	{
		addPattern(index, 0);
		memcpy(s->patternv[s->patterni[index]],
				s->patternv[s->patterni[oldindex]],
				sizeof(pattern));
		return index;
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
		if (addPattern(index, 0)) return 1; /* allocate memory */

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
	if (s->patternc <= 1) return 1; /* no patterns to remove */
	if (!s->patterni[index]) return 1; /* pattern doesn't exist */
	uint8_t cutIndex = s->patterni[index];

	free(s->patternv[cutIndex]); s->patternv[cutIndex] = NULL;

	s->patterni[index] = 0;

	/* enforce contiguity */
	unsigned short i;
	for (i = cutIndex; i < s->patternc - 1; i++)
		s->patternv[i] = s->patternv[i + 1];

	for (i = 0; i < 256; i++) // for every backref index
		if (s->patterni[i] > cutIndex && s->patterni[i] != 0)
			s->patterni[i]--;

	s->patternc--;
	return 0;
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
	w->pbfx[0] = x1;
	w->pbfx[1] = x2;
	w->pbfy[0] = y1;
	w->pbfy[1] = y2;
	w->pbchannel[0] = c1;
	w->pbchannel[1] = c2;
	w->pbpopulated = 1;

	pattern *destpattern = s->patternv[s->patterni[s->songi[w->songfy]]];
	// uint8_t row = (w->trackerfy + j - w->pbfy[0]) % (destpattern->rowcc[channel]+1);
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
		if ((w->pbfx[0] == 2 && w->pbfx[1] == 2) || (w->pbfx[0] == 3 && w->pbfx[1] == 3)) /* just one macro column */
		{
			unsigned char targetmacro;
			if (w->trackerfx < 2) targetmacro = 0;
			else targetmacro = tfxToVfx(w->trackerfx) - 2;
			for (uint8_t j = w->pbfy[0]; j <= w->pbfy[1]; j++)
			{
				uint8_t row = (w->trackerfy + j - w->pbfy[0]) % (destpattern->rowcc[w->channel]+1);
				destpattern->rowv[w->channel][row].macro[targetmacro].c = w->patternbuffer.rowv[w->pbchannel[0]][j].macro[w->pbfx[0] - 2].c;
				destpattern->rowv[w->channel][row].macro[targetmacro].v = w->patternbuffer.rowv[w->pbchannel[0]][j].macro[w->pbfx[0] - 2].v;
			}
			w->trackerfx = vfxToTfx(targetmacro + 2);
		} else
		{
			for (j = w->pbfy[0]; j <= w->pbfy[1]; j++)
			{
				row = (w->trackerfy + j - w->pbfy[0]) % (destpattern->rowcc[w->channel]+1);
				if (w->pbfx[0] <= 0 && w->pbfx[1] >= 0) destpattern->rowv[w->channel][row].note = w->patternbuffer.rowv[w->pbchannel[0]][j].note;
				if (w->pbfx[0] <= 1 && w->pbfx[1] >= 1) destpattern->rowv[w->channel][row].inst = w->patternbuffer.rowv[w->pbchannel[0]][j].inst;
				for (k = 0; k < s->channelv[w->channel].macroc; k++)
					if (w->pbfx[0] <= k+2 && w->pbfx[1] >= k+2)
					{
						destpattern->rowv[w->channel][row].macro[k].c = w->patternbuffer.rowv[w->pbchannel[0]][j].macro[k].c;
						destpattern->rowv[w->channel][row].macro[k].v = w->patternbuffer.rowv[w->pbchannel[0]][j].macro[k].v;
					}
			}
			w->trackerfx = vfxToTfx(w->pbfx[0]);
		}
	} else
	{
		for (i = w->pbchannel[0]; i <= w->pbchannel[1]; i++)
		{
			channel = w->channel + i - w->pbchannel[0];
			if (channel < s->channelc)
				for (j = w->pbfy[0]; j <= w->pbfy[1]; j++)
				{
					row = (w->trackerfy + j - w->pbfy[0]) % (destpattern->rowcc[channel]+1);
					if (i == w->pbchannel[0]) /* first channel */
					{
						if (w->pbfx[0] <= 0) destpattern->rowv[channel][row].note = w->patternbuffer.rowv[i][j].note;
						if (w->pbfx[0] <= 1) destpattern->rowv[channel][row].inst = w->patternbuffer.rowv[i][j].inst;
						for (k = 0; k < s->channelv[channel].macroc; k++)
							if (w->pbfx[0] <= k+2)
							{
								destpattern->rowv[channel][row].macro[k].c = w->patternbuffer.rowv[i][j].macro[k].c;
								destpattern->rowv[channel][row].macro[k].v = w->patternbuffer.rowv[i][j].macro[k].v;
							}
					} else if (i == w->pbchannel[1]) /* last channel */
					{
						if (w->pbfx[1] >= 0) destpattern->rowv[channel][row].note = w->patternbuffer.rowv[i][j].note;
						if (w->pbfx[1] >= 1) destpattern->rowv[channel][row].inst = w->patternbuffer.rowv[i][j].inst;
						for (k = 0; k < s->channelv[channel].macroc; k++)
							if (w->pbfx[1] >= k+2)
							{
								destpattern->rowv[channel][row].macro[k].c = w->patternbuffer.rowv[i][j].macro[k].c;
								destpattern->rowv[channel][row].macro[k].v = w->patternbuffer.rowv[i][j].macro[k].v;
							}
					} else /* middle channel */
						destpattern->rowv[channel][row] = w->patternbuffer.rowv[i][j];
				}
			else break;
		}
		w->trackerfx = vfxToTfx(w->pbfx[0]);
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
			row = (y1 + j - w->pbfy[0]) % (destpattern->rowcc[w->channel]+1);
			if (x1 <= 0 && x2 >= 0) destpattern->rowv[c1][row].note = NOTE_VOID;
			if (x1 <= 1 && x2 >= 1) destpattern->rowv[c1][row].inst = INST_VOID;
			for (k = 0; k < s->channelv[c1].macroc; k++)
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
						for (k = 0; k < s->channelv[i].macroc; k++)
							if (x1 <= k+2)
							{
								destpattern->rowv[i][row].macro[k].c = 0;
								destpattern->rowv[i][row].macro[k].v = 0;
							}
					} else if (i == c2) /* last channel */
					{
						if (x2 >= 0) destpattern->rowv[i][row].note = NOTE_VOID;
						if (x2 >= 1) destpattern->rowv[i][row].inst = INST_VOID;
						for (k = 0; k < s->channelv[i].macroc; k++)
							if (x1 >= k+2)
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
			row = j % (destpattern->rowcc[w->channel]+1);
			if (x1 <= 0 && x2 >= 0) destpattern->rowv[c1][row].note += value;
			if (x1 <= 1 && x2 >= 1) destpattern->rowv[c1][row].inst += value;
			for (k = 0; k < s->channelv[c1].macroc; k++)
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
						for (k = 0; k < s->channelv[i].macroc; k++)
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
						for (k = 0; k < s->channelv[i].macroc; k++)
							if (x1 >= k+2)
								switch (destpattern->rowv[i][row].macro[k].c)
								{
									case 'G': destpattern->rowv[i][row].macro[k].v += value*16;
									default:  destpattern->rowv[i][row].macro[k].v += value;
								}
					} else /* middle channel */
					{
						destpattern->rowv[i][row].note += value;
						destpattern->rowv[i][row].inst += value;
						for (k = 0; k < s->channelv[i].macroc; k++)
							switch (destpattern->rowv[i][row].macro[k].c)
							{
								case 'G': destpattern->rowv[i][row].macro[k].v += value*16;
								default:  destpattern->rowv[i][row].macro[k].v += value;
							}
					}
				}
			else break;
}
/* block randomize */
void randPartPattern(signed char value, short x1, short x2, short y1, short y2, uint8_t c1, uint8_t c2)
{
	uint8_t i, j, row, randinst;
	int k;
	pattern *destpattern = s->patternv[s->patterni[s->songi[w->songfy]]];
	if (c1 == c2) /* only one channel */
	{
		for (j = y1; j <= y2; j++)
		{
			row = j % (destpattern->rowcc[w->channel]+1);
			if (x1 <= 0 && x2 >= 0) destpattern->rowv[c1][row].note = MIN(A10-1, rand()%36 +MIN(7, w->octave)*12);
			if (x1 <= 1 && x2 >= 1 && destpattern->rowv[c1][row].note != NOTE_VOID)
			{
				destpattern->rowv[c1][row].inst = NOTE_VOID;
				randinst = rand()%(s->instrumentc-1) + 1;
				for (k = 0; k < 255; k++)
					if (s->instrumenti[k] == randinst)
					{ destpattern->rowv[c1][row].inst = k; break; }
			}
			for (k = 0; k < s->channelv[c1].macroc; k++)
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
						if (x1 <= 0) destpattern->rowv[i][row].note = MIN(A10-1, rand()%36 +MIN(7, w->octave)*12);
						if (x1 <= 1 && destpattern->rowv[i][row].note)
						{
							destpattern->rowv[i][row].inst = NOTE_VOID;
							randinst = rand()%(s->instrumentc-1) + 1;
							for (k = 0; k < 255; k++)
								if (s->instrumenti[k] == randinst)
								{ destpattern->rowv[i][row].inst = k; break; }
						}
						for (k = 0; k < s->channelv[i].macroc; k++)
							if (x1 <= k+2 && destpattern->rowv[i][row].macro[k].c)
								destpattern->rowv[i][row].macro[k].v = rand()%256;
					} else if (i == c2) /* last channel */
					{
						if (x2 >= 0) destpattern->rowv[i][row].note = MIN(A10-1, rand()%36 +MIN(7, w->octave)*12);
						if (x2 >= 1 && destpattern->rowv[i][row].note)
						{
							destpattern->rowv[i][row].inst = NOTE_VOID;
							randinst = rand()%(s->instrumentc-1) + 1;
							for (k = 0; k < 255; k++)
								if (s->instrumenti[k] == randinst)
								{ destpattern->rowv[i][row].inst = k; break; }
						}
						for (k = 0; k < s->channelv[i].macroc; k++)
							if (x1 >= k+2 && destpattern->rowv[i][row].macro[k].c)
								destpattern->rowv[i][row].macro[k].v = rand()%256;
					} else /* middle channel */
					{
						destpattern->rowv[i][row].note = MIN(A10-1, rand()%36 +MIN(7, w->octave)*12);
						if (destpattern->rowv[i][row].note)
						{
							destpattern->rowv[i][row].inst = NOTE_VOID;
							randinst = rand()%(s->instrumentc-1) + 1;
							for (k = 0; k < 255; k++)
								if (s->instrumenti[k] == randinst)
								{ destpattern->rowv[i][row].inst = k; break; }
						}
						for (k = 0; k < s->channelv[i].macroc; k++)
							if (destpattern->rowv[i][row].macro[k].c)
								destpattern->rowv[i][row].macro[k].v = rand()%256;
					}
				}
			else break;
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
	dest->loop[0] = src->loop[0]; dest->loop[1] = src->loop[1];
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
		{ free(iv->history[i]->sampledata); iv->history[i]->sampledata = NULL; }
		free(iv->history[i]); iv->history[i] = NULL;
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
		_delInstrument(iv->history[iv->historyptr%128]);
	free(iv->history[iv->historyptr%128]);
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
			|| ivh->loop[0] != iv->loop[0] || ivh->loop[1] != iv->loop[1]
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
	{ strcpy(w->command.error, "already at newest change"); return; }
	pushInstrumentHistoryIfNew(iv);

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

	s->instrumentv[s->instrumentc] = calloc(1, sizeof(instrument));
	if (!s->instrumentv[s->instrumentc])
	{
		strcpy(w->command.error, "failed to add instrument, out of memory");
		return 1;
	}
	s->instrumentv[s->instrumentc]->gain = 0xff;
	s->instrumentv[s->instrumentc]->samplerate = 0xff;
	s->instrumentv[s->instrumentc]->bitdepth = 0xf;
	s->instrumentv[s->instrumentc]->loopramp = 0xff;
	s->instrumentv[s->instrumentc]->cyclelength = 0x06ff;
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


void toggleRecording(uint8_t inst)
{
	if (w->instrumentrecv == INST_REC_LOCK_OK)
		w->instrumentreci = s->instrumenti[inst];
	if (w->instrumentreci == s->instrumenti[inst])
	{
		switch (w->instrumentrecv)
		{
			case INST_REC_LOCK_OK:
				w->recbuffer = malloc(sizeof(short) * RECORD_LENGTH * samplerate * 2);
				if (!w->recbuffer)
				{
					strcpy(w->command.error, "failed to start recording, out of memory");
					if (w->recbuffer) { free(w->recbuffer); w->recbuffer = NULL; }
					break;
				}
				w->recptr = 0;
				w->instrumentrecv = INST_REC_LOCK_START;
				break;
			default:
				w->instrumentrecv = INST_REC_LOCK_PREP_END;
				break;
		}
	}
	redraw();
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
	iv->trim[1] = sfinfo.frames - 1;
	iv->loop[0] = 0;
	iv->loop[1] = 0;
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

	memset(cs->songi, 255, sizeof(uint8_t) * 256);

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
	{
		/* jack_port_unregister(client, p->cin[i].l);
		jack_port_unregister(client, p->cin[i].r);
		jack_port_unregister(client, p->cout[i].l);
		jack_port_unregister(client, p->cout[i].r); */
		_delChannel(cs, i);
	}

	for (int i = 1; i < cs->patternc; i++)
		free(cs->patternv[i]);

	for (int i = 1; i < cs->instrumentc; i++)
		_delInstrument(cs->instrumentv[i]);

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
	fputc('e', fp);
	fputc('g', fp);
	fputc('g', fp);

	/* version */
	fputc(MAJOR, fp);
	fputc(MINOR, fp);

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
	for (i = 0; i < s->channelc; i++)
		fputc(s->channelv[i].macroc, fp);

	/* songi */
	for (i = 0; i < 256; i++)
		fputc(s->songi[i], fp);
	/* songf */
	for (i = 0; i < 256; i++)
		fputc(s->songf[i], fp);
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
		{
			fputc(s->patternv[i]->rowcc[j], fp);
			char macroc = s->channelv[j].macroc;
			for (k = 0; k < s->patternv[i]->rowcc[j] + 1; k++)
			{
				fputc(s->patternv[i]->rowv[j][k].note, fp);
				fputc(s->patternv[i]->rowv[j][k].inst, fp);
				for (l = 0; l < macroc; l++)
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
		fwrite(iv->loop, sizeof(uint32_t), 2, fp);
		fwrite(&iv->envelope, sizeof(uint8_t), 1, fp);
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
		redraw();
		return NULL;
	}
	DIR *dp = opendir(path);
	if (dp) // file is a directory
	{
		closedir(dp);
		fclose(fp);
		fcntl(0, F_SETFL, O_NONBLOCK); /* non-blocking */
		snprintf(w->command.error, COMMAND_LENGTH, "file '%s' is a directory", path);
		redraw();
		return NULL;
	}

	int i, j, k, l;

	/* ensure egg wasn't broken in transit */
	/* the most important check */
	if (!(fgetc(fp) == 'e' && fgetc(fp) == 'g' && fgetc(fp) == 'g'))
	{
		fclose(fp);
		fcntl(0, F_SETFL, O_NONBLOCK); /* non-blocking */
		snprintf(w->command.error, COMMAND_LENGTH, "file '%s' isn't valid", path);
		redraw();
		return NULL;
	}

	/* version */
	unsigned char filemajor = fgetc(fp);
	unsigned char fileminor = fgetc(fp);
	if (filemajor == 0 && fileminor < 86)
	{
		fclose(fp);
		fcntl(0, F_SETFL, O_NONBLOCK);
		strcpy(w->command.error, "failed to read song, file uses removed features");
		redraw();
		return NULL;
	}

	song *cs = _addSong();
	if (!cs)
	{
		fclose(fp);
		fcntl(0, F_SETFL, O_NONBLOCK); /* non-blocking */
		strcpy(w->command.error, "failed to read song, out of memory");
		redraw();
		return NULL;
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
	for (int i = 0; i < cs->channelc; i++)
		_addChannel(cs, &cs->channelv[i]);
	cs->rowhighlight = fgetc(fp);

	/* mutes */
	char byte;
	for (i = 0; i < 4; i++)
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
	}
	for (i = 0; i < cs->channelc; i++)
		cs->channelv[i].macroc = fgetc(fp);

	/* songi */
	for (i = 0; i < 256; i++)
		cs->songi[i] = fgetc(fp);
	/* songf */
	for (i = 0; i < 256; i++)
		cs->songf[i] = fgetc(fp);
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
		{
			if (!(filemajor == 0 && fileminor < 85))
				cs->patternv[i]->rowcc[j] = fgetc(fp);
			char macroc = cs->channelv[j].macroc;
			for (k = 0; k < cs->patternv[i]->rowcc[j] + 1; k++)
			{
				cs->patternv[i]->rowv[j][k].note = fgetc(fp);
				if (filemajor == 0 && fileminor < 88) cs->patternv[i]->rowv[j][k].note--;
				cs->patternv[i]->rowv[j][k].inst = fgetc(fp);
				for (l = 0; l < macroc; l++)
				{
					cs->patternv[i]->rowv[j][k].macro[l].c = fgetc(fp);
					cs->patternv[i]->rowv[j][k].macro[l].v = fgetc(fp);
				}
			}
		}
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
		fread(iv->loop, sizeof(uint32_t), 2, fp);
		if (filemajor == 0 && fileminor < 89) fseek(fp, sizeof(uint8_t)*4, SEEK_CUR);
		else fread(&iv->envelope, sizeof(uint8_t), 1, fp);
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
		}

		pushInstrumentHistory(cs->instrumentv[i]);
	}


	fclose(fp);
	fcntl(0, F_SETFL, O_NONBLOCK); /* non-blocking */
	redraw();
	return cs;
}
