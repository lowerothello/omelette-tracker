void copyInstrument(instrument *dest, instrument *src)
{
	dest->samplelength = src->samplelength;
	dest->length = src->length;
	dest->channels = src->channels;
	dest->channelmode = src->channelmode;
	dest->c5rate = src->c5rate;
	dest->samplerate = src->samplerate;
	dest->bitdepth = src->bitdepth;
	dest->cyclelength = src->cyclelength;
	dest->pitchshift = src->pitchshift;
	dest->timestretch = src->timestretch;
	dest->trim[0] = src->trim[0]; dest->trim[1] = src->trim[1];
	dest->loop = src->loop;
	dest->envelope = src->envelope;
	dest->sustain = src->sustain;
	dest->gain = src->gain;
	dest->invert = src->invert;
	dest->pingpong = src->pingpong;
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

void _delInstrument(instrument *iv)
{
	if (iv->sampledata)
	{
		free(iv->sampledata);
		iv->sampledata = NULL;
	}
}

int addInstrument(uint8_t index)
{
	if (s->instrumenti[index] < s->instrumentc) return 1; /* index occupied */

	instrument *newinstrumentv = calloc(s->instrumentc+1, sizeof(instrument));
	if (s->instrumentv)
	{
		memcpy(newinstrumentv, s->instrumentv, s->instrumentc * sizeof(instrument));
		free(s->instrumentv);
	}
	s->instrumentv = newinstrumentv;

	s->instrumentv[s->instrumentc].gain = 0x20;
	s->instrumentv[s->instrumentc].samplerate = 0xff;
	s->instrumentv[s->instrumentc].bitdepth = 0xf;
	s->instrumentv[s->instrumentc].loopramp = 0x00;
	s->instrumentv[s->instrumentc].cyclelength = 0x3fff;
	s->instrumentv[s->instrumentc].pitchshift = 0x80;
	s->instrumentv[s->instrumentc].midichannel = -1;
	s->instrumentv[s->instrumentc].sustain = 1;
	s->instrumenti[index] = s->instrumentc;

	s->instrumentc++;
	return 0;
}
void yankInstrument(uint8_t index)
{
	if (!s->instrumentv) return;
	if (s->instrumenti[index] >= s->instrumentc) return; /* nothing to yank */

	_delInstrument(&w->instrumentbuffer);
	copyInstrument(&w->instrumentbuffer, &s->instrumentv[s->instrumenti[index]]);
}
void putInstrument(uint8_t index)
{
	if (!s->instrumentv) return;
	if (s->instrumenti[index] >= s->instrumentc) addInstrument(index);

	copyInstrument(&s->instrumentv[s->instrumenti[index]], &w->instrumentbuffer);
}
int delInstrument(uint8_t index)
{
	if (!s->instrumentv) return 1;
	if (s->instrumenti[index] >= s->instrumentc) return 1; /* instrument doesn't exist */

	uint8_t cutindex = s->instrumenti[index];
	_delInstrument(&s->instrumentv[cutindex]);
	s->instrumenti[index] = INSTRUMENT_VOID;

	instrument *newinstrumentv = calloc(s->instrumentc+1, sizeof(instrument));

	if (cutindex > 0)
		memcpy(newinstrumentv,
				s->instrumentv,
				sizeof(instrument)*cutindex);

	if (cutindex < s->instrumentc-1)
		memcpy(&newinstrumentv[cutindex-1],
				&s->instrumentv[cutindex],
				sizeof(instrument)*(s->instrumentc-cutindex));
	free(s->instrumentv);
	s->instrumentv = newinstrumentv;

	/* backref contiguity */
	for (uint8_t i = 0; i < 255; i++) // for every backref index
		if (s->instrumenti[i] >= cutindex && s->instrumenti[i] < s->instrumentc)
			s->instrumenti[i]--;

	s->instrumentc--;
	return 0;
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
		if (stat(path, &buf) == -1)
		{
			fcntl(0, F_SETFL, O_NONBLOCK); /* non-blocking */
			return NULL;
		}

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
	/* TODO: check the instrument is safe more aggressively? */
	instrument *iv = &s->instrumentv[s->instrumenti[index]];
	SF_INFO sfinfo;
	short *sampledata = _loadSample(path, &sfinfo);
	if (!sampledata)
	{
		strcpy(w->command.error, "failed to load sample, out of memory");
		return;
	}

	/* unload any present sample data */
	if (iv->sampledata) free(iv->sampledata);
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
