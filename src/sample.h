#define SAMPLE_MAX 16

/* TODO: sample destructive operations:
 *  gain
 *  rate/bit redux
 *  mp3 compression
 *  etc.
 */
typedef struct Sample
{ /* alloc(sizeof(Sample) + sizeof(short) * .length * .channels) */
	uint32_t length;
	uint8_t  channels;
	uint32_t rate;    /* rate to play C5 at */
	uint32_t defrate; /* rate to return to when the rate control is reset */
	bool     invert;

	uint32_t trimstart;
	uint32_t trimlength;
	uint32_t looplength;
	bool     pingpong;
	uint8_t  loopramp;

	short    data[];
} Sample;

typedef Sample *SampleChain[SAMPLE_MAX];

Sample *loadSample(char *path);

/* gets the first free sample slot, or -1 if none are free */
short getEmptySampleIndex(SampleChain *chain);

/* .sample == NULL to detach */
void attachSample(SampleChain **oldchain, Sample *sample, uint8_t index);
void copySampleChain(SampleChain *dest, SampleChain *src);

struct json_object *serializeSample(Sample*, size_t *dataoffset);
void serializeSampleData(FILE *fp, Sample*, size_t *dataoffset);
Sample *deserializeSample(struct json_object*, void *data, double ratemultiplier);

/* iterate over allocated samples in a (SampleChain*) */
#define FOR_SAMPLECHAIN(iter, samplechain) \
	for (uint8_t iter = 0; iter < SAMPLE_MAX; iter++) \
		if ((*samplechain)[iter])
