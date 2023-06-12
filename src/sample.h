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
	uint8_t  gain;
	bool     invert;

	uint32_t trimstart;
	uint32_t trimlength;
	uint32_t looplength;
	bool     pingpong;
	uint8_t  loopramp;

	short    data[];
} Sample;

Sample *loadSample(char *path);
void copySample(Sample **dest, Sample *src);

struct json_object *serializeSample(Sample*, size_t *dataoffset);
void serializeSampleData(FILE *fp, Sample*, size_t *dataoffset);
Sample *deserializeSample(struct json_object*, void *data, double ratemultiplier);
