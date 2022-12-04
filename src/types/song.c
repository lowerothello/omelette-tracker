Song *_addSong(void)
{
	Song *cs = calloc(1, sizeof(Song));
	if (!cs) return NULL;

	cs->masteroutput[0] =       calloc(buffersize, sizeof(float));
	cs->masteroutput[1] =       calloc(buffersize, sizeof(float));
	cs->masterpluginoutput[0] = calloc(buffersize, sizeof(float));
	cs->masterpluginoutput[1] = calloc(buffersize, sizeof(float));
	cs->sendoutput[0] =       calloc(buffersize, sizeof(float));
	cs->sendoutput[1] =       calloc(buffersize, sizeof(float));
	cs->sendpluginoutput[0] = calloc(buffersize, sizeof(float));
	cs->sendpluginoutput[1] = calloc(buffersize, sizeof(float));

	cs->master = newEffectChain(cs->masteroutput, cs->masterpluginoutput);
	cs->send   = newEffectChain(cs->sendoutput,   cs->sendpluginoutput);

	cs->rowhighlight = 4;
	cs->songbpm = DEF_BPM;

	return cs;
}

#define STARTING_TRACKC 4 /* how many tracks to allocate for new files */
Song *addSong(void)
{
	Song *ret = _addSong();

	ret->instrument = calloc(1, sizeof(InstrumentChain));
	memset(ret->instrument->i, INSTRUMENT_VOID, sizeof(uint8_t) * INSTRUMENT_MAX);

	ret->track = calloc(1, sizeof(TrackChain) + STARTING_TRACKC * sizeof(Track));
	regenGlobalRowc(ret);
	ret->track->c = STARTING_TRACKC;
	for (uint8_t i = 0; i < STARTING_TRACKC; i++)
		_addTrack(ret, &ret->track->v[i]);

	return ret;
}

void delSong(Song *cs)
{
	clearEffectChain(cs->master); free(cs->master);
	clearEffectChain(cs->send); free(cs->send);

	free(cs->masteroutput[0]);
	free(cs->masteroutput[1]);
	free(cs->masterpluginoutput[0]);
	free(cs->masterpluginoutput[1]);
	free(cs->sendoutput[0]);
	free(cs->sendoutput[1]);
	free(cs->sendpluginoutput[0]);
	free(cs->sendpluginoutput[1]);

	for (int i = 0; i < cs->track->c; i++)
		_delTrack(cs, &cs->track->v[i]);
	free(cs->track);

	for (int i = 0; i < cs->instrument->c; i++)
		_delInstrument(&cs->instrument->v[i]);
	free(cs->instrument);

	if (cs->bpmcache) free(cs->bpmcache);

	free(cs);
}

void reapplyBpm(void)
{
	Event e;
	e.sem = M_SEM_BPM;
	pushEvent(&e);
}

int writeSong(Song *cs, char *path)
{
	char *pathext = fileExtension(path, MODULE_EXTENSION);
	if (!strcmp(pathext, MODULE_EXTENSION))
	{
		free(pathext);
		if (!strlen(w->filepath))
		{
			strcpy(w->command.error, "no file name");
			return 1;
		}
		pathext = malloc(sizeof(w->filepath) + 1);
		strcpy(pathext, w->filepath);
	} else strcpy(w->filepath, pathext);

	fcntl(0, F_SETFL, 0); /* blocking */

	FILE *fp = fopen(pathext, "w");

#ifdef DEBUG_LOGS /* TODO: too many #ifdefs here */
	FILE *debugfp = fopen(".oml_savedump", "a");
	fprintf(debugfp, "===== SAVE DUMP =====");
#endif

	int i;

	/* egg, for each and every trying time (the most important) */
	fputc('e', fp); fputc('g', fp); fputc('g', fp);

	/* version */
	fputc(MAJOR, fp); fputc(MINOR, fp);

	/* counts */
	fwrite(&samplerate, sizeof(jack_nframes_t), 1, fp);
	fputc(cs->songbpm, fp);
	fputc(cs->instrument->c, fp);
	fputc(cs->track->c, fp);
	fputc(cs->rowhighlight, fp);
	fwrite(&cs->songlen, sizeof(uint16_t), 1, fp);
	fwrite(cs->loop, sizeof(uint16_t), 2, fp);

	/* tracks */
#ifdef DEBUG_LOGS
	fprintf(debugfp, "%02x track(s) expected\n", cs->track->c);
#endif
	for (i = 0; i < cs->track->c; i++)
	{
#ifdef DEBUG_LOGS
		fprintf(debugfp, "TRACK %02x - 0x%zx\n", i, ftell(fp));
#endif
		serializeTrack(cs, &cs->track->v[i], fp);
	}

	/* instrument->i */
	for (i = 0; i < INSTRUMENT_MAX; i++) fputc(cs->instrument->i[i], fp);

	/* instrument->v */
#ifdef DEBUG_LOGS
	fprintf(debugfp, "\n%02x instrument(s) expected\n", cs->instrument->c);
#endif
	for (i = 0; i < cs->instrument->c; i++)
	{
#ifdef DEBUG_LOGS
		fprintf(debugfp, "INSTRUMENT %02x - 0x%zx\n", i, ftell(fp));
#endif
		serializeInstrument(&cs->instrument->v[i], fp);
	}

	fclose(fp);
#ifdef DEBUG_LOGS
	fprintf(debugfp, "\n\n");
	fclose(debugfp);
#endif
	fcntl(0, F_SETFL, O_NONBLOCK); /* non-blocking */
	snprintf(w->command.error, COMMAND_LENGTH, "'%s' written", pathext);
	free(pathext);
	return 0;
}
Song *readSong(char *path)
{
	fcntl(0, F_SETFL, 0); /* blocking */
	FILE *fp = fopen(path, "r");
	if (!fp) // file doesn't exist, or fopen otherwise failed
	{
		p->redraw = 1; return NULL;
	}
	DIR *dp = opendir(path);
	if (dp) // file is a directory
	{
		closedir(dp); fclose(fp);
		fcntl(0, F_SETFL, O_NONBLOCK); /* non-blocking */
		snprintf(w->command.error, COMMAND_LENGTH, "file '%s' is a directory", path);
		p->redraw = 1; return NULL;
	}

	int i;

	/* ensure egg wasn't broken in transit */
	/* the most important check */
	if (!(fgetc(fp) == 'e' && fgetc(fp) == 'g' && fgetc(fp) == 'g'))
	{
		fclose(fp);
		fcntl(0, F_SETFL, O_NONBLOCK); /* non-blocking */
		snprintf(w->command.error, COMMAND_LENGTH, "file '%s' isn't valid", path);
		p->redraw = 1; return NULL;
	}

	/* version */
	uint8_t filemajor = fgetc(fp);
	uint8_t fileminor = fgetc(fp);

	if (filemajor < 1)
	{
		fclose(fp);
		fcntl(0, F_SETFL, O_NONBLOCK); /* non-blocking */
		strcpy(w->command.error, "failed to read song, file too old");
		p->redraw = 1; return NULL;
	}

	Song *cs = _addSong();
	if (!cs)
	{
		fclose(fp);
		fcntl(0, F_SETFL, O_NONBLOCK); /* non-blocking */
		strcpy(w->command.error, "failed to read song, out of memory");
		p->redraw = 1; return NULL;
	}

	/* assume the rest of the file is valid */
	/* TODO: proper error checking lol */
	strcpy(w->filepath, path);

	double ratemultiplier = 1.0;
	jack_nframes_t filesamplerate;
	fread(&filesamplerate, sizeof(jack_nframes_t), 1, fp);
	ratemultiplier = (double)samplerate / (double)filesamplerate;

	/* counts */
	cs->songbpm = fgetc(fp);

	uint8_t tempinstrumentc = fgetc(fp);
	uint8_t temptrackc = fgetc(fp);
	cs->rowhighlight = fgetc(fp);
	fread(&cs->songlen, sizeof(uint16_t), 1, fp);
	fread(cs->loop, sizeof(uint16_t), 2, fp);

	cs->track = calloc(1, sizeof(TrackChain) + temptrackc * sizeof(Track));
	cs->track->c = temptrackc;
	/* tracks */
	for (i = 0; i < cs->track->c; i++)
		deserializeTrack(cs, &cs->track->v[i], fp, filemajor, fileminor);

	/* instruments */
	cs->instrument = calloc(1, sizeof(InstrumentChain) + tempinstrumentc * sizeof(Instrument));
	cs->instrument->c = tempinstrumentc;
	for (i = 0; i < INSTRUMENT_MAX; i++) cs->instrument->i[i] = fgetc(fp);
	for (i = 0; i < cs->instrument->c; i++)
		deserializeInstrument(&cs->instrument->v[i], fp, ratemultiplier, filemajor, fileminor);

	fclose(fp);
	fcntl(0, F_SETFL, O_NONBLOCK); /* non-blocking */
	p->redraw = 1; return cs;
}
