/* free the returned value */
char *fileExtension(char *path, char *ext)
{
	char *ret;
	if (strlen(path) < strlen(ext) || strcmp(path+(strlen(path) - strlen(ext)), ext))
	{
		ret = malloc(strlen(path) + strlen(ext) + 1);
		strcpy(ret, path);
		strcat(ret, ext);
	} else
	{
		ret = malloc(strlen(path) + 1);
		strcpy(ret, path);
	} return ret;
}

int writeSong(char *path)
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

	FILE *debugfp = fopen(".save_debug", "w");

	int i;

	/* egg, for each and every trying time (the most important) */
	fputc('e', fp); fputc('g', fp); fputc('g', fp);

	/* version */
	fputc(MAJOR, fp); fputc(MINOR, fp);

	/* counts */
	fwrite(&samplerate, sizeof(jack_nframes_t), 1, fp);
	fputc(s->songbpm, fp);
	fputc(s->instrument->c, fp);
	fputc(s->channelc, fp);
	fputc(s->rowhighlight, fp);
	fwrite(&s->songlen, sizeof(uint16_t), 1, fp);
	fwrite(s->loop, sizeof(uint16_t), 2, fp);

	/* channels */
	fprintf(debugfp, "%02x channel(s) expected\n", s->channelc);
	for (i = 0; i < s->channelc; i++)
	{
		fprintf(debugfp, "CHANNEL %02x - 0x%zx\n", i, ftell(fp));
		serializeChannel(s, &s->channelv[i], fp);
	}

	/* instrument->i */
	for (i = 0; i < INSTRUMENT_MAX; i++) fputc(s->instrument->i[i], fp);

	/* instrument->v */
	fprintf(debugfp, "\n%02x instrument(s) expected\n", s->instrument->c);
	for (i = 0; i < s->instrument->c; i++)
	{
		fprintf(debugfp, "INSTRUMENT %02x - 0x%zx\n", i, ftell(fp));
		serializeInstrument(&s->instrument->v[i], fp);
	}

	fclose(fp);
	fclose(debugfp);
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
		strcpy(w->filepath, path);
		p->dirty = 1; return NULL;
	}
	DIR *dp = opendir(path);
	if (dp) // file is a directory
	{
		closedir(dp); fclose(fp);
		fcntl(0, F_SETFL, O_NONBLOCK); /* non-blocking */
		snprintf(w->command.error, COMMAND_LENGTH, "file '%s' is a directory", path);
		p->dirty = 1; return NULL;
	}

	int i;

	/* ensure egg wasn't broken in transit */
	/* the most important check */
	if (!(fgetc(fp) == 'e' && fgetc(fp) == 'g' && fgetc(fp) == 'g'))
	{
		fclose(fp);
		fcntl(0, F_SETFL, O_NONBLOCK); /* non-blocking */
		snprintf(w->command.error, COMMAND_LENGTH, "file '%s' isn't valid", path);
		p->dirty = 1; return NULL;
	}

	/* version */
	uint8_t filemajor = fgetc(fp);
	uint8_t fileminor = fgetc(fp);
	/* if (filemajor < 1)
	{
		fclose(fp);
		fcntl(0, F_SETFL, O_NONBLOCK);
		strcpy(w->command.error, "failed to read song, file is too old");
		p->dirty = 1; return NULL;
	} */

	Song *cs = _addSong();
	if (!cs)
	{
		fclose(fp);
		fcntl(0, F_SETFL, O_NONBLOCK); /* non-blocking */
		strcpy(w->command.error, "failed to read song, out of memory");
		p->dirty = 1; return NULL;
	}

	/* assume the rest of the file is valid */
	/* TODO: proper error checking lol */
	strcpy(w->filepath, path);

	double ratemultiplier = 1.0;
	if (!(filemajor == 0 && fileminor < 98))
	{
		jack_nframes_t filesamplerate;
		fread(&filesamplerate, sizeof(jack_nframes_t), 1, fp);
		ratemultiplier = (double)samplerate / (double)filesamplerate;
	}

	/* counts */
	cs->songbpm = fgetc(fp);
	w->request = REQ_BPM;

	uint8_t tempinstrumentc = fgetc(fp);
	cs->channelc = fgetc(fp);
	cs->rowhighlight = fgetc(fp);
	fread(&cs->songlen, sizeof(uint16_t), 1, fp);
	fread(cs->loop, sizeof(uint16_t), 2, fp);

	cs->channelv = calloc(cs->channelc, sizeof(Channel));
	/* channels */
	for (i = 0; i < cs->channelc; i++)
		deserializeChannel(cs, &cs->channelv[i], fp, filemajor, fileminor);

	/* instruments */
	cs->instrument = calloc(1, sizeof(InstrumentChain) + tempinstrumentc * sizeof(Instrument));
	cs->instrument->c = tempinstrumentc;
	for (i = 0; i < INSTRUMENT_MAX; i++) cs->instrument->i[i] = fgetc(fp);
	for (i = 0; i < cs->instrument->c; i++)
		deserializeInstrument(&cs->instrument->v[i], fp, ratemultiplier, filemajor, fileminor);

	fclose(fp);
	fcntl(0, F_SETFL, O_NONBLOCK); /* non-blocking */
	p->dirty = 1; return cs;
}
