/* caller should free the returned value */
char *fileExtension(char *path, char *ext);

struct Song_1_0
{
	char magic[3];
	uint8_t major;
	uint8_t minor;

	jack_nframes_t samplerate;
	uint8_t songbpm;
	uint8_t rowhighlight;
	uint16_t songlen; /* TODO: necessary? */
	uint16_t loop[2];

	uint8_t trackc;
};



#include "file.c"
