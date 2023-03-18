Sample *loadSample(char *path)
{
	fcntl(0, F_SETFL, 0); /* blocking */
	SF_INFO sfinfo = { 0 };

	Sample *ret = NULL;

	SNDFILE *sndfile = sf_open(path, SFM_READ, &sfinfo);
	if (!sndfile)
	{ /* raw file */
		struct stat buf;
		if (stat(path, &buf) == -1) goto loadSample_end;

		ret = calloc(1, sizeof(Sample) + buf.st_size - buf.st_size % sizeof(short));
		if (!ret) goto loadSample_end;

		/* read the whole file into memory */
		FILE *fp = fopen(path, "r");
		fread(&ret->data, sizeof(short), buf.st_size / sizeof(short), fp);
		fclose(fp);

		ret->channels = 1;
		ret->length = buf.st_size / sizeof(short);
		ret->rate = ret->defrate = 12000;
	} else /* audio file */
	{
		ret = calloc(1, sizeof(Sample) + sizeof(short)*sfinfo.frames*sfinfo.channels);

		if (!ret) goto loadSample_end;

		/* read the whole file into memory */
		sf_readf_short(sndfile, ret->data, sfinfo.frames);
		ret->length = sfinfo.frames;
		ret->channels = sfinfo.channels;
		ret->rate = ret->defrate = sfinfo.samplerate;
	}

	ret->trimstart = 0;
	ret->trimlength = ret->length-1;
	ret->looplength = 0;

loadSample_end:
	if (sndfile) sf_close(sndfile);
	fcntl(0, F_SETFL, O_NONBLOCK); /* non-blocking */
	return ret;
}

short getEmptySampleIndex(SampleChain *chain)
{
	for (uint8_t i = 0; i < SAMPLE_MAX; i++)
		if (!(*chain)[i])
			return i;
	return -1;
}

static void cb_attachSample(Event *e)
{
	if (e->callbackarg) free(e->callbackarg);
	if (e->src) free(e->src);
}

/* .sample == NULL to detach */
void attachSample(SampleChain **oldchain, Sample *sample, uint8_t index)
{
	SampleChain *newchain = malloc(sizeof(SampleChain));
	memcpy(newchain, *oldchain, sizeof(SampleChain));
	(*newchain)[index] = sample;

	Event e;
	e.sem = M_SEM_SWAP_REQ;
	e.dest = (void**)oldchain;
	e.src = newchain;
	e.callback = cb_attachSample;
	e.callbackarg = (**oldchain)[index];
	pushEvent(&e);
}

void copySampleChain(SampleChain *dest, SampleChain *src)
{
	size_t size;
	FOR_SAMPLECHAIN(i, src)
	{
		size = sizeof(Sample) + sizeof(short) * (*src)[i]->length * (*src)[i]->channels;
		(*dest)[i] = malloc(size);
		memcpy((*dest)[i], (*src)[i], size);
	}
}
