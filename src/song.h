typedef struct Song
{
	InstChain  *inst;
	TrackChain *track;

	/* effect chains */
	EffectChain *master;

	/* misc. state */
	uint8_t plen; /* rows in each pattern */
	uint8_t slen; /* patterns in the song */
	uint8_t rowhighlight;
	uint8_t songbpm;
} Song;
Song *s;

Song *addSong(void);
void initSong(Song*);
void freeSong(Song*);
void reapplyBpm(void);

void serializeSong(FILE*, Song*);
Song *deserializeSong(FILE*);
