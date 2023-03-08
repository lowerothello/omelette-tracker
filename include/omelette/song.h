typedef struct _Song
{
	/* instruments */
	InstrumentChain *instrument;

	/* tracks */
	TrackChain *track;
	short      *bpmcache;    /* bpm change caching so multithreading isn't hell */
	uint16_t    bpmcachelen; /* how far into bpmcache it's safe to index */

	/* song pointers */
	uint16_t playfy;  /* analogous to window->trackerfy */
	uint16_t songlen; /* how long the global variant is */
	uint16_t loop[3]; /* loop range pointers, [2] is the staging loop end */

	/* effect chains */
	EffectChain *master;
	float       *masteroutput[2];
	float       *masterpluginoutput[2]; /* some external plugins need to read and write from separate buffers */
	EffectChain *send;
	float       *sendoutput[2];
	float       *sendpluginoutput[2]; /* some external plugins need to read and write from separate buffers */

	/* misc. state */
	uint8_t  rowhighlight;
	uint8_t  songbpm;
	uint16_t spr;     /* samples per row (samplerate * (60 / bpm) / (rowhighlight * 2)) */
	uint16_t sprp;    /* samples per row progress */
	bool     playing; /* true if the sequencer is running */
} Song;
Song *s;

Song *_addSong(void); /* TODO: should be internal */
Song *allocSong(void);
void freeSong(Song *);
void reapplyBpm(void);
