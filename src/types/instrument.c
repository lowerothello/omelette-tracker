void copyInstrument(Instrument *dest, Instrument *src)
{
	dest->samplelength = src->samplelength;
	dest->length = src->length;
	dest->channels = src->channels;
	dest->channelmode = src->channelmode;
	dest->c5rate = src->c5rate;
	dest->samplerate = src->samplerate;
	dest->bitdepth = src->bitdepth;
	dest->trimstart = src->trimstart;
	dest->trimlength = src->trimlength;
	dest->looplength = src->looplength;
	dest->envelope = src->envelope;
	dest->sustain = src->sustain;
	dest->gain = src->gain;
	dest->invert = src->invert;
	dest->pingpong = src->pingpong;
	dest->loopramp = src->loopramp;
	dest->midichannel = src->midichannel;

	dest->cyclelength = src->cyclelength;
	dest->rearrange = src->rearrange;
	dest->reversegrains = src->reversegrains;
	dest->timestretch = src->timestretch;
	dest->notestretch = src->notestretch;
	dest->pitchshift = src->pitchshift;
	dest->pitchstereo = src->pitchstereo;

	if (dest->sampledata)
	{ free(dest->sampledata); dest->sampledata = NULL; }

	if (src->sampledata)
	{ /* only copy sampledata if it exists */
		dest->sampledata = malloc(sizeof(short) * src->samplelength);
		memcpy(dest->sampledata, src->sampledata, sizeof(short) * src->samplelength);
	}

	for (uint8_t i = 0; i < dest->effect.c; i++)
		freeEffect(&dest->effect.v[i]);

	dest->effect.c = src->effect.c;
	for (uint8_t i = 0; i < src->effect.c; i++)
		copyEffect(&dest->effect.v[i], &src->effect.v[i]);
}

/* frees the contents of an instrument */
void _delInstrument(Instrument *iv)
{
	if (iv->sampledata)
	{
		free(iv->sampledata);
		iv->sampledata = NULL;
	}

	for (uint8_t i = 0; i < iv->effect.c; i++)
		freeEffect(&iv->effect.v[i]);
	iv->effect.c = 0;
}

bool instrumentSafe(Song *cs, uint8_t index)
{
	if (index != INSTRUMENT_MAX && cs->instrument->i[index] < cs->instrument->c)
		return 1;
	return 0;
}

/* take a short* and reparent it under instrument iv               */
/* bufferlen is how many stereo pairs of samples are in the buffer */
void reparentSample(Instrument *iv, short *buffer, jack_nframes_t bufferlen)
{
	if (iv->sampledata) { free(iv->sampledata); iv->sampledata = NULL; }

	iv->sampledata = buffer;

	iv->samplelength = bufferlen<<1;
	iv->channels = 2;
	iv->length = bufferlen;
	iv->c5rate = samplerate; /* assume the buffer is at the system sample rate */
	iv->trimstart = 0;
	iv->trimlength = bufferlen - 1;
	iv->looplength = 0;
}

void cb_addInstrument(void *arg)
{
	free(p->semarg); p->semarg = NULL; p->dirty = 1;
}
InstrumentChain *_addInstrument(uint8_t index)
{
	InstrumentChain *newinstrument = calloc(1, sizeof(InstrumentChain) + (s->instrument->c+1) * sizeof(Instrument));
	memcpy(newinstrument, s->instrument, sizeof(InstrumentChain) + s->instrument->c * sizeof(Instrument));

	newinstrument->v[newinstrument->c].samplerate = 0xff;
	newinstrument->v[newinstrument->c].bitdepth = 0xf;
	newinstrument->v[newinstrument->c].sustain = 1;
	newinstrument->v[newinstrument->c].midichannel = -1;

	newinstrument->v[newinstrument->c].cyclelength = 0x3fff;

	newinstrument->i[index] = newinstrument->c;
	newinstrument->c++;

	return newinstrument;
}
int addInstrument(uint8_t index)
{ /* fully atomic */
	if (instrumentSafe(s, index)) return 1; /* index occupied */
	if (p->sem != M_SEM_OK) return 1; /* semaphore is busy */

	p->semarg = _addInstrument(index);
	p->semcallback = cb_addInstrument;
	p->sem = M_SEM_SWAPINST_REQ;
	return 0;
}
typedef struct
{
	short *buffer;
	jack_nframes_t buflen;
	uint8_t index;
} InstrumentAddReparentArg;
void cb_addReparentInstrument(void *arg)
{
	InstrumentAddReparentArg *castarg = arg;
	free(p->semarg); p->semarg = NULL;
	reparentSample(&s->instrument->v[s->instrument->i[castarg->index]], castarg->buffer, castarg->buflen);
	free(arg); p->semcallbackarg = NULL;
	p->dirty = 1;
}
int addReparentInstrument(uint8_t index, short *buffer, jack_nframes_t buflen)
{ /* fully atomic */
	if (instrumentSafe(s, index)) return 1; /* index occupied */
	if (p->sem != M_SEM_OK) return 1; /* semaphore is busy */

	p->semarg = _addInstrument(index);

	p->semcallback = cb_addReparentInstrument;
	InstrumentAddReparentArg *arg = malloc(sizeof(InstrumentAddReparentArg));
	arg->buffer = buffer;
	arg->buflen = buflen;
	arg->index = index;

	p->semcallbackarg = arg;
	p->sem = M_SEM_SWAPINST_REQ;
	return 0;
}

/* returns -1 if no instrument slots are free */
short emptyInstrument(uint8_t min)
{
	for (int i = min; i < INSTRUMENT_MAX; i++)
		if (!instrumentSafe(s, i)) return i;
	return -1;
}

void yankInstrument(uint8_t index)
{
	if (!instrumentSafe(s, index)) return; /* nothing to copy */
	_delInstrument(&w->instrumentbuffer);
	copyInstrument(&w->instrumentbuffer, &s->instrument->v[s->instrument->i[index]]);
}

void putInstrument(uint8_t index)
{
	if (s->instrument->i[index] >= s->instrument->c) addInstrument(index);
	copyInstrument(&s->instrument->v[s->instrument->i[index]], &w->instrumentbuffer);
}

void cb_delInstrument(void *arg)
{
	_delInstrument(&((InstrumentChain *)p->semarg)->v[(size_t)arg]);
	free(p->semarg); p->semarg = NULL; p->dirty = 1;
}
int delInstrument(uint8_t index)
{ /* fully atomic */
	if (!instrumentSafe(s, index)) return 1; /* instrument doesn't exist */
	if (p->sem != M_SEM_OK) return 1; /* semaphore is busy */

	size_t cutindex = s->instrument->i[index]; /* cast to void* later */
	_delInstrument(&s->instrument->v[cutindex]);

	InstrumentChain *newinstrument = calloc(1, sizeof(InstrumentChain) + s->instrument->c * sizeof(Instrument));
	memcpy(newinstrument, s->instrument, sizeof(InstrumentChain)); /* copy just the header */

	if (cutindex > 0)
		memcpy(newinstrument->v,
				s->instrument->v,
				sizeof(Instrument)*(cutindex+1));

	if (cutindex < s->instrument->c-1)
		memcpy(&newinstrument->v[cutindex],
				&s->instrument->v[cutindex+1],
				sizeof(Instrument)*(s->instrument->c-(cutindex+1)));

	newinstrument->i[index] = INSTRUMENT_VOID;
	/* backref contiguity */
	for (uint8_t i = 0; i < 255; i++)
		if (newinstrument->i[i] >= cutindex && newinstrument->i[i] < newinstrument->c)
			newinstrument->i[i]--;

	newinstrument->c--;

	p->semcallback = cb_delInstrument;
	p->semcallbackarg = (void *)cutindex;
	p->semarg = newinstrument;
	p->sem = M_SEM_SWAPINST_REQ;
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
	if (!instrumentSafe(s, index)) return; /* instrument doesn't exist */
	Instrument *iv = &s->instrument->v[s->instrument->i[index]];
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
	iv->trimstart = 0;
	iv->trimlength = sfinfo.frames-1;
	iv->looplength = 0;
	iv->samplerate = 0xff;
	iv->bitdepth = 0xf;
}

void serializeInstrument(Instrument *iv, FILE *fp)
{
	fwrite(&iv->samplelength, sizeof(uint32_t), 1, fp);
	fwrite(&iv->length, sizeof(uint32_t), 1, fp);
	fwrite(&iv->channels, sizeof(uint8_t), 1, fp);
	fwrite(&iv->channelmode, sizeof(int8_t), 1, fp);
	fwrite(&iv->c5rate, sizeof(uint32_t), 1, fp);
	fwrite(&iv->samplerate, sizeof(uint8_t), 1, fp);
	fwrite(&iv->bitdepth, sizeof(uint8_t), 1, fp);
	fwrite(&iv->trimstart, sizeof(uint32_t), 1, fp);
	fwrite(&iv->trimlength, sizeof(uint32_t), 1, fp);
	fwrite(&iv->looplength, sizeof(uint32_t), 1, fp);
	fwrite(&iv->envelope, sizeof(uint8_t), 1, fp);
	fwrite(&iv->sustain, sizeof(bool), 1, fp);
	fwrite(&iv->gain, sizeof(uint8_t), 1, fp);
	fwrite(&iv->invert, sizeof(bool), 1, fp);
	fwrite(&iv->pingpong, sizeof(bool), 1, fp);
	fwrite(&iv->loopramp, sizeof(uint8_t), 1, fp);
	fwrite(&iv->midichannel, sizeof(int8_t), 1, fp);

	fwrite(&iv->cyclelength, sizeof(uint16_t), 1, fp);
	fwrite(&iv->rearrange, sizeof(uint8_t), 1, fp);
	fwrite(&iv->reversegrains, sizeof(bool), 1, fp);
	fwrite(&iv->timestretch, sizeof(int16_t), 1, fp);
	fwrite(&iv->notestretch, sizeof(bool), 1, fp);
	fwrite(&iv->pitchshift, sizeof(int16_t), 1, fp);
	fwrite(&iv->pitchstereo, sizeof(int8_t), 1, fp);

	if (iv->samplelength)
		fwrite(iv->sampledata, sizeof(short), iv->samplelength, fp);

	fwrite(&iv->effect.c, sizeof(uint8_t), 1, fp);
	for (int i = 0; i < iv->effect.c; i++)
		serializeEffect(&iv->effect.v[i], fp);
}
void deserializeInstrument(Instrument *iv, FILE *fp, double ratemultiplier, uint8_t major, uint8_t minor)
{
	fread(&iv->samplelength, sizeof(uint32_t), 1, fp);
	fread(&iv->length, sizeof(uint32_t), 1, fp);
	fread(&iv->channels, sizeof(uint8_t), 1, fp);
	fread(&iv->channelmode, sizeof(int8_t), 1, fp);
	fread(&iv->c5rate, sizeof(uint32_t), 1, fp); iv->c5rate *= ratemultiplier;
	fread(&iv->samplerate, sizeof(uint8_t), 1, fp);
	fread(&iv->bitdepth, sizeof(uint8_t), 1, fp);
	fread(&iv->trimstart, sizeof(uint32_t), 1, fp);
	fread(&iv->trimlength, sizeof(uint32_t), 1, fp);
	fread(&iv->looplength, sizeof(uint32_t), 1, fp);
	fread(&iv->envelope, sizeof(uint8_t), 1, fp);
	fread(&iv->sustain, sizeof(bool), 1, fp);
	fread(&iv->gain, sizeof(uint8_t), 1, fp);
	fread(&iv->invert, sizeof(bool), 1, fp);
	fread(&iv->pingpong, sizeof(bool), 1, fp);
	fread(&iv->loopramp, sizeof(uint8_t), 1, fp);
	fread(&iv->midichannel, sizeof(int8_t), 1, fp);

	fread(&iv->cyclelength, sizeof(uint16_t), 1, fp);
	fread(&iv->rearrange, sizeof(uint8_t), 1, fp);
	fread(&iv->reversegrains, sizeof(bool), 1, fp);
	fread(&iv->timestretch, sizeof(int16_t), 1, fp);
	fread(&iv->notestretch, sizeof(bool), 1, fp);
	fread(&iv->pitchshift, sizeof(int16_t), 1, fp);
	fread(&iv->pitchstereo, sizeof(int8_t), 1, fp);

	if (iv->samplelength)
	{
		iv->sampledata = malloc(sizeof(short) * iv->samplelength);
		fread(iv->sampledata, sizeof(short), iv->samplelength, fp);
	}

	if (!(major == 1 && minor < 97))
	{
		fread(&iv->effect.c, sizeof(uint8_t), 1, fp);
		for (int i = 0; i < iv->effect.c; i++)
			deserializeEffect(&iv->effect.v[i], fp, major, minor);
	}
}
