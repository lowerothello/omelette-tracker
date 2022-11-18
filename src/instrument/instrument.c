#include "chord/add.c"
#include "chord/yank.c"
#include "chord/delete.c"

int sampleExportCallback(char *command, unsigned char *mode)
{
	if (!instrumentSafe(s, w->instrument)) return 1;
	Instrument *iv = &s->instrument->v[s->instrument->i[w->instrument]];

	if (!iv->sample->length) return 1;

	char *buffer = malloc(strlen(command) + 1);
	wordSplit(buffer, command, 0);

	SNDFILE *sndfile;
	SF_INFO sfinfo;
	memset(&sfinfo, 0, sizeof(sfinfo));

	sfinfo.samplerate = iv->sample->rate;
	sfinfo.frames = iv->sample->length;
	sfinfo.channels = iv->sample->channels;

	sfinfo.format = SF_FORMAT_WAV | SF_FORMAT_PCM_16;
	sndfile = sf_open(fileExtension(buffer, ".wav"), SFM_WRITE, &sfinfo);
	if (sndfile == NULL) { free(buffer); return 1; }

	// write the sample data to disk
	sf_writef_short(sndfile, iv->sample->data, iv->sample->length * iv->sample->channels);
	sf_close(sndfile);

	free(buffer);
	return 0;
}
/* TODO: sample could already be loaded into p->semarg, reparent if so */
void sampleLoadCallback(char *path)
{
	if (path) loadSample(w->instrument, path);

	w->page = PAGE_INSTRUMENT_SAMPLE;
	w->mode = I_MODE_NORMAL;
	w->showfilebrowser = 0;
	resetWaveform();
}
