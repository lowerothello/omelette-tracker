typedef struct Song
{
	InstChain  *inst;
	TrackChain *track;

	/* effect chains */
	EffectChain *master;
	EffectChain *send;

	/* song pointers */
	uint16_t songlen; /* how long the global variant is */
	uint16_t loop[3]; /* loop range pointers, 3rd value is the staging loop end */

	/* misc. state */
	uint8_t plen;
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
