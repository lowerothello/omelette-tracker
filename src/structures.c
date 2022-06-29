#define MAX_VALUE_LEN 128

typedef struct
{
	char    c; /* command */
	uint8_t v; /* argument */
} macro;
typedef struct
{
	uint8_t note;
	uint8_t inst;
	macro   macro[8];
} row;
typedef struct
{
	uint8_t rowc;
	row     rowv[64][256]; /* 64 channels, each with 256 rows */
} pattern;

typedef struct
{
	char      mute;                         /* saved to disk */
	char      macroc;                       /* macro count */

	uint32_t  pointer;                      /* progress through the sound */
	uint32_t  pointeroffset;                /* where to base pointer off of */
	uint32_t  releasepointer;               /* 0 for no release, where releasing started */
	short     gain;                         /* unsigned nibble per-channel, -1 for unset */
	short     targetgain;                   /* smooth gain target */
	row       r;
	float     finetune;                     /* calculated fine tune, should be between -0.5 and +0.5 */
	float     portamentofinetune;           /* used for portamento, should be between -0.5 and +0.5 */
	uint8_t   portamento;                   /* portamento target, 255 for off */
	uint8_t   portamentospeed;              /* portamento m */
	uint16_t  rtrigsamples;                 /* samples per retrigger */
	uint32_t  rtrigpointer;                 /* sample ptr to ratchet back to */
	uint8_t   rtrigblocksize;               /* number of rows block extends to */
	uint16_t  cutsamples;                   /* samples into the row to cut, 0 for no cut */
	uint16_t  delaysamples;                 /* samples into the row to delay, 0 for no delay */
	uint8_t   delaynote;
	uint8_t   delayinst;
	char      vibrato;                      /* vibrato depth, 0-f */
	uint32_t  vibratosamples;               /* samples per full phase walk */
	uint32_t  vibratosamplepointer;         /* distance through cv->vibratosamples */

	/* waveshapers */
	char      softclip;
	char      hardclip;
	char      wavefolder;
	char      wavewrapper;
	char      signedunsigned;
	char      rectifiertype;                /* 0: full-wave, 1: full-wave x2 */
	char      rectifier;                    /* shared rectifier memory */

	/* ramping */
	uint16_t  rampindex;                    /* progress through the ramp buffer, rampmax if not ramping */
	sample_t *rampbuffer;                   /* samples to ramp out */

	void     *state[INSTRUMENT_TYPE_COUNT]; /* type working memory */
} channel;

typedef struct Instrument
{
	uint8_t             type;
	uint8_t             typefollow;                   /* follows the type, set once state is guaranteed to be mallocced */
	void               *state[INSTRUMENT_TYPE_COUNT]; /* type working memory */
	uint8_t             defgain;
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
	channel         channelv[64];            /* channel values */
	row            *channelbuffer[64];       /* channel paste buffer */
	char            channelbuffermute;

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

	char           filepath[COMMAND_LENGTH];

	void         (*filebrowserCallback)(char *); /* arg is the selected path */
	command_t      command;
	char           chord;                        /* key chord buffer, vi-style multi-letter commands eg. dd, di", cap, 4j, etc. */
	unsigned char  popup;
	unsigned char  mode, oldmode;
	unsigned short centre;
	uint8_t        pattern;                      /* focused pattern */
	uint8_t        channel;                      /* focused channel */
	uint8_t        channeloffset, visiblechannels;

	short          trackerfy, trackerfx;
	short          visualfy, visualfx;
	uint8_t        visualchannel;
	short          instrumentindex;
	uint8_t        instrument;                   /* focused instrument */
	unsigned short instrumentcelloffset;
	unsigned short instrumentrowoffset;

	unsigned short mousey, mousex;

	short          fyoffset;
	signed char    fieldpointer;

	char           dirpath[NAME_MAX + 1];
	unsigned int   dirc;
	unsigned short dirmaxwidth;
	unsigned char  dircols;
	DIR           *dir;

	short          songfy, songfx;

	char           octave;
	uint8_t        step;

	uint8_t        songnext;

	uint8_t        previewnote, previewinst;
	uint8_t        previewchannel;
	instrument     previewinstrument;            /* used by the file browser */
	char           previewtrigger;               /* 0:cut
	                                                1:start inst
	                                                2:still inst
	                                                3:start sample
	                                                4:still sample
	                                                5:prep volatile */

	char           previewsamplestatus;

	uint8_t        instrumentlocki;              /* realindex */
	uint8_t        instrumentlockv;              /* value, set to an INST_GLOBAL_LOCK constant */
	char           request;                      /* ask the playback function to do something */

	uint8_t        instrumentreci;               /* realindex */
	uint8_t        instrumentrecv;               /* value, set to an INST_REC_LOCK constant */
	short         *recbuffer;                    /* disallow changing the type or removing while recording */
	sample_t      *recchannelbuffer;             /* buffer for recordflags bit 1 focused channel */
	uint32_t       recptr;
	char           recordsource;                 /* 0:line in, 1:loopback */
	uint8_t        recordflags;                  /* %1: only loop back cursor channel, %2: gate around the next pattern */

	char           newfilename[COMMAND_LENGTH];  /* used by readSong */
} window;
window *w;

typedef struct
{
	struct
	{
		unsigned short   indexc;               /* index count used (0 inclusive) */
		unsigned short   cellwidth;
		size_t           statesize;
		void           (*draw) (instrument *, uint8_t, unsigned short, unsigned short, short *, char);
		void           (*adjustUp)(instrument *, short, char);
		void           (*adjustDown)(instrument *, short, char);
		void           (*adjustLeft)(instrument *, short, char);
		void           (*adjustRight)(instrument *, short, char);
		void           (*incFieldPointer)(short);
		void           (*decFieldPointer)(short);
		void           (*endFieldPointer)(short);
		void           (*mouseToIndex)(int, int, int, short *);
		void           (*input)(int *);
		void           (*process)(instrument *, channel *, uint32_t, sample_t *, sample_t *);
		void           (*macro)(instrument *, channel *, row, uint8_t, int);
		void           (*addType)(void **);
		void           (*copyType)(void **, void **); /* destination, source */
		void           (*delType)(void **);
		void           (*addChannel)(void **);
		void           (*delChannel)(void **);
		void           (*write)(void **, FILE *fp);
		void           (*read)(void **, unsigned char, unsigned char, FILE *fp);
	} f[INSTRUMENT_TYPE_COUNT];
} typetable;
typetable *t;


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



void _addChannel(channel *cv)
{
	cv->rampindex = rampmax;
	cv->rampbuffer = malloc(sizeof(sample_t) * rampmax * 2); /* *2 for stereo */
}
int addChannel(song *cs, uint8_t index)
{
	if (cs->channelc >= 63) return 1;
	_addChannel(&cs->channelv[cs->channelc]); /* allocate memory */
	if (cs->channelc > 0) /* contiguity */
		for (uint8_t i = cs->channelc; i >= index; i--)
		{
			for (uint8_t p = 1; p < cs->patternc; p++)
				memcpy(cs->patternv[p]->rowv[i + 1],
					cs->patternv[p]->rowv[i],
					sizeof(row) * 256);
			cs->channelv[i + 1].mute = cs->channelv[i].mute;
			cs->channelv[i + 1].macroc = cs->channelv[i].macroc;
		}

	cs->channelv[index].mute = 0;
	cs->channelv[index].macroc = 2;

	for (uint8_t p = 1; p < cs->patternc; p++)
	{
		memset(cs->patternv[p]->rowv[index], 0, sizeof(row) * 256);
		for (short r = 0; r < 256; r++)
			cs->patternv[p]->rowv[index][r].inst = 255;
	}

	cs->channelc++;
	return 0;
}
void _delChannel(song *cs, uint8_t index)
{
	free(cs->channelv[index].rampbuffer);
	cs->channelv[index].rampbuffer = NULL;

	for (int i = 0; i < INSTRUMENT_TYPE_COUNT; i++)
		if (cs->channelv[index].state[i])
		{
			t->f[i].delChannel(&cs->channelv[index].state[i]);
			cs->channelv[index].state[i] = NULL;
		}
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
		s->channelv[s->channelc].macroc = 2;
	} else
	{
		_delChannel(s, index);

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

		if (w->channeloffset)
			w->channeloffset--;
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
	else            s->patternv[s->patternc]->rowc = w->defpatternlength;

	s->patterni[index] = s->patternc;
	s->patternc++;
	return 0;
}
int duplicatePattern(uint8_t oldindex)
{
	for (uint8_t i = 0; i < 255; i++)
		if (!s->patterni[i])
		{
			addPattern(i, 0);
			memcpy(s->patternv[s->patterni[i]],
					s->patternv[s->patterni[oldindex]],
					sizeof(pattern));
			return i;
		}
	return oldindex; /* duplication failed, no free slots */
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
		s->patternv[s->patterni[index]]->rowc = w->defpatternlength;
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
	memcpy(&w->patternbuffer, s->patternv[s->patterni[s->songi[w->songfy]]], sizeof(pattern));
	w->pbfx[0] = x1;
	w->pbfx[1] = x2;
	w->pbfy[0] = y1;
	w->pbfy[1] = y2;
	w->pbchannel[0] = c1;
	w->pbchannel[1] = c2;
	w->pbpopulated = 1;
}
void putPartPattern(void)
{
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
				uint8_t row = w->trackerfy + j - w->pbfy[0];
				if (row <= destpattern->rowc)
				{
					destpattern->rowv[w->channel][row].macro[targetmacro].c = w->patternbuffer.rowv[w->pbchannel[0]][j].macro[w->pbfx[0] - 2].c;
					destpattern->rowv[w->channel][row].macro[targetmacro].v = w->patternbuffer.rowv[w->pbchannel[0]][j].macro[w->pbfx[0] - 2].v;
				} else break;
			}
			w->trackerfx = vfxToTfx(targetmacro + 2);
		} else
		{
			for (uint8_t j = w->pbfy[0]; j <= w->pbfy[1]; j++)
			{
				uint8_t row = w->trackerfy + j - w->pbfy[0];
				if (row <= destpattern->rowc)
				{
					if (w->pbfx[0] <= 0 && w->pbfx[1] >= 0) destpattern->rowv[w->channel][row].note = w->patternbuffer.rowv[w->pbchannel[0]][j].note;
					if (w->pbfx[0] <= 1 && w->pbfx[1] >= 1) destpattern->rowv[w->channel][row].inst = w->patternbuffer.rowv[w->pbchannel[0]][j].inst;
					if (w->pbfx[0] <= 2 && w->pbfx[1] >= 2)
					{
						destpattern->rowv[w->channel][row].macro[0].c = w->patternbuffer.rowv[w->pbchannel[0]][j].macro[0].c;
						destpattern->rowv[w->channel][row].macro[0].v = w->patternbuffer.rowv[w->pbchannel[0]][j].macro[0].v;
					}
					if (w->pbfx[0] <= 3 && w->pbfx[1] >= 3)
					{
						destpattern->rowv[w->channel][row].macro[1].c = w->patternbuffer.rowv[w->pbchannel[0]][j].macro[1].c;
						destpattern->rowv[w->channel][row].macro[1].v = w->patternbuffer.rowv[w->pbchannel[0]][j].macro[1].v;
					}
				} else break;
			}
			w->trackerfx = vfxToTfx(w->pbfx[0]);
		}
	} else
	{
		for (uint8_t i = w->pbchannel[0]; i <= w->pbchannel[1]; i++)
		{
			uint8_t channel = w->channel + i - w->pbchannel[0];
			if (channel < s->channelc)
			{
				for (uint8_t j = w->pbfy[0]; j <= w->pbfy[1]; j++)
				{
					uint8_t row = w->trackerfy + j - w->pbfy[0];
					if (row <= destpattern->rowc)
					{
						if (i == w->pbchannel[0]) /* first channel */
						{
							if (w->pbfx[0] <= 0) destpattern->rowv[channel][row].note = w->patternbuffer.rowv[i][j].note;
							if (w->pbfx[0] <= 1) destpattern->rowv[channel][row].inst = w->patternbuffer.rowv[i][j].inst;
							if (w->pbfx[0] <= 2)
							{
								destpattern->rowv[channel][row].macro[0].c = w->patternbuffer.rowv[i][j].macro[0].c;
								destpattern->rowv[channel][row].macro[0].v = w->patternbuffer.rowv[i][j].macro[0].v;
							}
							if (w->pbfx[0] <= 3)
							{
								destpattern->rowv[channel][row].macro[1].c = w->patternbuffer.rowv[i][j].macro[1].c;
								destpattern->rowv[channel][row].macro[1].v = w->patternbuffer.rowv[i][j].macro[1].v;
							}
						} else if (i == w->pbchannel[1]) /* last channel */
						{
							if (w->pbfx[1] >= 0) destpattern->rowv[channel][row].note = w->patternbuffer.rowv[i][j].note;
							if (w->pbfx[1] >= 1) destpattern->rowv[channel][row].inst = w->patternbuffer.rowv[i][j].inst;
							if (w->pbfx[1] >= 2)
							{
								destpattern->rowv[channel][row].macro[0].c = w->patternbuffer.rowv[i][j].macro[0].c;
								destpattern->rowv[channel][row].macro[0].v = w->patternbuffer.rowv[i][j].macro[0].v;
							}
							if (w->pbfx[1] >= 3)
							{
								destpattern->rowv[channel][row].macro[1].c = w->patternbuffer.rowv[i][j].macro[1].c;
								destpattern->rowv[channel][row].macro[1].v = w->patternbuffer.rowv[i][j].macro[1].v;
							}
						} else /* middle channel */
							destpattern->rowv[channel][row] = w->patternbuffer.rowv[i][j];
					} else break;
				}
			} else break;
		}
		w->trackerfx = vfxToTfx(w->pbfx[0]);
	}
}
void delPartPattern(short x1, short x2, short y1, short y2, uint8_t c1, uint8_t c2)
{
	pattern *destpattern = s->patternv[s->patterni[s->songi[w->songfy]]];
	if (c1 == c2) /* only one channel */
	{
		for (uint8_t j = y1; j <= y2; j++)
		{
			if (j <= destpattern->rowc)
			{
				if (x1 <= 0 && x2 >= 0) destpattern->rowv[c1][j].note = 0;
				if (x1 <= 1 && x2 >= 1) destpattern->rowv[c1][j].inst = 255;
				if (x1 <= 2 && x2 >= 2)
				{
					destpattern->rowv[c1][j].macro[0].c = 0;
					destpattern->rowv[c1][j].macro[0].v = 0;
				}
				if (x1 <= 3 && x2 >= 3)
				{
					destpattern->rowv[c1][j].macro[1].c = 0;
					destpattern->rowv[c1][j].macro[1].v = 0;
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
					if (j <= destpattern->rowc)
					{
						if (i == c1) /* first channel */
						{
							if (x1 <= 0) destpattern->rowv[i][j].note = 0;
							if (x1 <= 1) destpattern->rowv[i][j].inst = 255;
							if (x1 <= 2)
							{
								destpattern->rowv[i][j].macro[0].c = 0;
								destpattern->rowv[i][j].macro[0].v = 0;
							}
							if (x1 <= 3)
							{
								destpattern->rowv[i][j].macro[1].c = 0;
								destpattern->rowv[i][j].macro[1].v = 0;
							}
						} else if (i == c2) /* last channel */
						{
							if (x2 >= 0) destpattern->rowv[i][j].note = 0;
							if (x2 >= 1) destpattern->rowv[i][j].inst = 255;
							if (x2 >= 2)
							{
								destpattern->rowv[i][j].macro[0].c = 0;
								destpattern->rowv[i][j].macro[0].v = 0;
							}
							if (x2 >= 3)
							{
								destpattern->rowv[i][j].macro[1].c = 0;
								destpattern->rowv[i][j].macro[1].v = 0;
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
	pattern *destpattern = s->patternv[s->patterni[s->songi[w->songfy]]];
	if (c1 == c2) /* only one channel */
	{
		for (uint8_t j = y1; j <= y2; j++)
		{
			if (j <= destpattern->rowc)
			{
				if (x1 <= 0 && x2 >= 0) destpattern->rowv[c1][j].note += value;
				if (x1 <= 1 && x2 >= 1) destpattern->rowv[c1][j].inst += value;
				if (x1 <= 2 && x2 >= 2) destpattern->rowv[c1][j].macro[0].v += value;
				if (x1 <= 3 && x2 >= 3) destpattern->rowv[c1][j].macro[1].v += value;
			} else break;
		}
	} else
		for (uint8_t i = c1; i <= c2; i++)
		{
			if (i < s->channelc)
			{
				for (uint8_t j = y1; j <= y2; j++)
				{
					if (j <= destpattern->rowc)
					{
						if (i == c1) /* first channel */
						{
							if (x1 <= 0) destpattern->rowv[i][j].note += value;
							if (x1 <= 1) destpattern->rowv[i][j].inst += value;
							if (x1 <= 2) destpattern->rowv[i][j].macro[0].v += value;
							if (x1 <= 3) destpattern->rowv[i][j].macro[1].v += value;
						} else if (i == c2) /* last channel */
						{
							if (x2 >= 0) destpattern->rowv[i][j].note += value;
							if (x2 >= 1) destpattern->rowv[i][j].inst += value;
							if (x2 >= 2) destpattern->rowv[i][j].macro[0].v += value;
							if (x2 >= 3) destpattern->rowv[i][j].macro[1].v += value;
						} else /* middle channel */
						{
							destpattern->rowv[i][j].inst += value;
							destpattern->rowv[i][j].note += value;
							destpattern->rowv[i][j].inst += value;
							destpattern->rowv[i][j].macro[0].v += value;
							destpattern->rowv[i][j].macro[1].v += value;
						}
					} else break;
				}
			} else break;
		}
}



void _allocChannelMemory(song *cs)
{
	char used[INSTRUMENT_TYPE_COUNT];
	memset(&used, 0, INSTRUMENT_TYPE_COUNT);

	for (int i = 1; i < cs->instrumentc; i++)
	{
		if (cs->instrumentv[i]->type < INSTRUMENT_TYPE_COUNT
				&& !used[cs->instrumentv[i]->type])
			used[cs->instrumentv[i]->type] = 1;
	}

	for (int i = 0; i < cs->channelc; i++)
		for (int j = 0; j < INSTRUMENT_TYPE_COUNT; j++)
		{
			if (used[j] && !cs->channelv[i].state[j])
				t->f[j].addChannel(&cs->channelv[i].state[j]);

			if (!used[j] && cs->channelv[i].state[j])
			{
				t->f[j].delChannel(&cs->channelv[i].state[j]);
				cs->channelv[i].state[j] = NULL;
			}
		}
}

/* don't force while forceindex is playing back! */
int changeInstrumentType(song *cs, uint8_t forceindex)
{
	uint8_t i;
	if (!forceindex)
		if (w->instrumentlockv == INST_GLOBAL_LOCK_FREE
				|| w->instrumentlockv == INST_GLOBAL_LOCK_HIST
				|| w->instrumentlockv == INST_GLOBAL_LOCK_PUT)
			i = w->instrumentlocki;
		else return 0;
	else i = forceindex;

	instrument *iv = cs->instrumentv[i];
	instrument *src;
	if (iv)
	{
		if (forceindex)
		{
			if (!iv->state[iv->type] && iv->type < INSTRUMENT_TYPE_COUNT)
			{
				t->f[iv->type].addType(&iv->state[iv->type]);
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
					t->f[iv->type].addType(&iv->state[iv->type]);
					if (!iv->state[iv->type])
					{
						strcpy(w->command.error, "failed to allocate instrument type, out of memory");
						return 1;
					}
				}
				break;
			case INST_GLOBAL_LOCK_HIST:
				src = iv->history[iv->historyptr%128];

				iv->type = src->type;
				iv->defgain = src->defgain;

				for (int j = 0; j < INSTRUMENT_TYPE_COUNT; j++)
					if (src->state[j])
					{
						if (!iv->state[j]) t->f[j].addType(&iv->state[j]);
						t->f[j].copyType(&iv->state[j], &src->state[j]);
					}
				w->instrumentindex = iv->historyindex[iv->historyptr%128];
				break;
			case INST_GLOBAL_LOCK_PUT:
				src = &w->instrumentbuffer;

				iv->type = src->type;
				iv->defgain = src->defgain;

				for (int j = 0; j < INSTRUMENT_TYPE_COUNT; j++)
				{
					if (iv->state[j])
					{
						t->f[j].delType(&iv->state[j]);
						iv->state[j] = NULL;
					}

					if (src->state[j])
					{
						t->f[j].addType(&iv->state[j]);
						t->f[j].copyType(&iv->state[j], &src->state[j]);
					}
				}
				break;
		}
		iv->typefollow = iv->type;
	}


	_allocChannelMemory(cs);


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
	return 0;
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
			{
				t->f[i].delType(&iv->history[iv->historyptr%128]->state[i]);
				iv->history[iv->historyptr%128]->state[i] = NULL;
			}

	instrument *ivh = iv->history[iv->historyptr%128];
	ivh->type =  iv->type;
	ivh->defgain = iv->defgain;
	for (int i = 0; i < INSTRUMENT_TYPE_COUNT; i++)
		if (iv->state[i])
		{
			t->f[i].addType(&ivh->state[i]);
			t->f[i].copyType(&ivh->state[i], &iv->state[i]);
		}
	iv->historyindex[iv->historyptr%128] = w->instrumentindex;
}
void pushInstrumentHistoryIfNew(instrument *iv)
{
	if (!iv) return;

	instrument *ivh = iv->history[iv->historyptr%128];
	if (ivh && iv->type < INSTRUMENT_TYPE_COUNT)
	{
		if (!iv->state[iv->type] || ivh->defgain != iv->defgain)
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
DEBUG=iv->historyptr;
	if (iv->historyptr <= 1 || iv->historybehind >= 127)
	{ strcpy(w->command.error, "already at oldest change"); return; }
	pushInstrumentHistoryIfNew(iv);

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
DEBUG=iv->historyptr;
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

	if (_addInstrument(s, s->instrumentc))
	{
		strcpy(w->command.error, "failed to add instrument, out of memory");
		return 1;
	}
	s->instrumentv[s->instrumentc]->defgain = 0x88;
	changeInstrumentType(s, s->instrumentc);
	t->f[0].addType(&s->instrumentv[s->instrumentc]->state[0]);
	s->instrumenti[index] = s->instrumentc;

	pushInstrumentHistory(s->instrumentv[s->instrumentc]);

	s->instrumentc++;

	_allocChannelMemory(s);
	return 0;
}
/* return a fresh slot */
uint8_t newInstrument(uint8_t minindex)
{
	for (uint8_t i = minindex; i < 256; i++) // is 256 right? idfk
		if (s->instrumenti[i] == 0)
			return i;
}
int yankInstrument(uint8_t index)
{
	if (s->instrumenti[index] == 0) return 1; /* nothing to yank */

	for (int i = 0; i < INSTRUMENT_TYPE_COUNT; i++)
		if (w->instrumentbuffer.state[i])
		{
			t->f[i].delType(&w->instrumentbuffer.state[i]);
			w->instrumentbuffer.state[i] = NULL;
		}

	instrument *src  = s->instrumentv[s->instrumenti[index]];
	instrument *dest = &w->instrumentbuffer;

	dest->type =  src->type;
	dest->defgain = src->defgain;
	for (int i = 0; i < INSTRUMENT_TYPE_COUNT; i++)
		if (src->state[i])
		{
			t->f[i].addType(&dest->state[i]);
			t->f[i].copyType(&dest->state[i], &src->state[i]);
		}

	return 0;
}
int putInstrument(uint8_t index)
{
	return 0;
}
void _delInstrument(song *cs, uint8_t realindex)
{
	for (int i = 0; i < INSTRUMENT_TYPE_COUNT; i++)
		if (cs->instrumentv[realindex]->state[i])
		{
			t->f[i].delType(&cs->instrumentv[realindex]->state[i]);
			cs->instrumentv[realindex]->state[i] = NULL;
		}

	for (int i = 0; i < 128; i++)
	{
		if (!cs->instrumentv[realindex]->history[i]) continue;
		for (int j = 0; j < INSTRUMENT_TYPE_COUNT; j++)
			if (cs->instrumentv[realindex]->history[i]->state[j])
			{
				t->f[j].delType(&cs->instrumentv[realindex]->history[i]->state[j]);
				cs->instrumentv[realindex]->history[i]->state[j] = NULL;
			}
		free(cs->instrumentv[realindex]->history[i]);
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
				w->recchannelbuffer = malloc(sizeof(sample_t) * buffersize * 2);
				if (!w->recbuffer || !w->recchannelbuffer)
				{
					strcpy(w->command.error, "failed to start recording, out of memory");
					if (w->recbuffer) { free(w->recbuffer); w->recbuffer = NULL; }
					if (w->recchannelbuffer) { free(w->recchannelbuffer); w->recchannelbuffer = NULL; }
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
		_delChannel(cs, i);

	for (int i = 1; i < cs->patternc; i++)
	{
		free(cs->channelbuffer[i]);
		free(cs->patternv[i]);
	}

	for (int i = 1; i < cs->instrumentc; i++)
		_delInstrument(cs, i);

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
			char macroc = s->channelv[j].macroc;
			for (k = 0; k < s->patternv[i]->rowc + 1; k++)
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
		fputc(iv->defgain, fp);
		fputc(iv->type, fp);

		for (uint8_t j = 0; j < INSTRUMENT_TYPE_COUNT; j++)
			if (iv->state[j])
			{
				fputc(j + 1, fp); // type index
				t->f[j].write(&iv->state[j], fp);
			}
		fputc(0, fp); // no more types
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

	int i, j, k, l;

	/* ensure egg wasn't broken in transit */
	/* the most important check */
	if (!(fgetc(fp) == 'e' && fgetc(fp) == 'g' && fgetc(fp) == 'g'))
	{
		snprintf(w->command.error, COMMAND_LENGTH, "file '%s' isn't valid", path);
		return NULL;
	}

	song *cs = _addSong();
	if (!cs)
	{
		strcpy(w->command.error, "failed to read song, out of memory");
		return NULL;
	}

	/* assume the rest of the file is valid */
	/* TODO: proper error checking lol */
	strcpy(w->filepath, path);

	/* version */
	unsigned char filemajor = fgetc(fp);
	unsigned char fileminor = fgetc(fp);

	/* counts */
	cs->songbpm = fgetc(fp);
	w->request = REQ_BPM;

	cs->patternc = fgetc(fp);
	cs->instrumentc = fgetc(fp);
	cs->channelc = fgetc(fp);
	for (int i = 0; i < cs->channelc; i++)
		_addChannel(&cs->channelv[i]);
	cs->rowhighlight = fgetc(fp);

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
			char macroc = cs->channelv[j].macroc;
			for (k = 0; k < cs->patternv[i]->rowc + 1; k++)
			{
				cs->patternv[i]->rowv[j][k].note = fgetc(fp);
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
		_addInstrument(cs, i);

		iv = cs->instrumentv[i];
		iv->defgain = fgetc(fp);
		iv->type = fgetc(fp);
		iv->typefollow = iv->type; /* confirm the type is safe to use */

		uint8_t j;
		while ((j = fgetc(fp))) /* read until there's no more */
			if (t->f[j-1].read)
			{
				t->f[j-1].addType(&iv->state[j-1]);
				t->f[j-1].read(&iv->state[j-1], filemajor, fileminor, fp);
			}
		if (!(iv->state[0]))
			t->f[0].addType(&iv->state[0]);

		pushInstrumentHistory(cs->instrumentv[i]);
	}


	fclose(fp);
	_allocChannelMemory(cs);
	return cs;
}
