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


void writeVariant(FILE *fp, variant *v)
{
	fwrite(&v->rowc, sizeof(uint16_t), 1, fp);
	fwrite(v->rowv, sizeof(row), v->rowc, fp);
}
void readVariant(FILE *fp, variant **v)
{
	uint16_t rowc;
	fread(&rowc, sizeof(uint16_t), 1, fp);
	*v = _copyVariant(NULL, rowc);
	fread((*v)->rowv, sizeof(row), rowc, fp);
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
	FILE *fp = fopen(pathext, "wb");
	int i, j;

	/* egg, for each and every trying time (the most important) */
	fputc('e', fp); fputc('g', fp); fputc('g', fp);

	/* version */
	fputc(MAJOR, fp); fputc(MINOR, fp);

	/* counts */
	fputc(s->songbpm, fp);
	fputc(s->instrumentc, fp);
	fputc(s->channelc, fp);
	fputc(s->rowhighlight, fp);
	fwrite(&s->songlen, sizeof(uint16_t), 1, fp);
	fwrite(s->loop, sizeof(uint16_t), 2, fp);

	/* channels */
	for (i = 0; i < s->channelc; i++)
	{
		fputc(s->channelv[i].data.mute, fp);
		fputc(s->channelv[i].data.macroc, fp);
		for (j = 0; j < VARIANT_MAX; j++)
			fputc(s->channelv[i].data.varianti[j], fp);
		fputc(s->channelv[i].data.variantc, fp);
		for (j = 0; j < s->channelv[i].data.variantc; j++)
			writeVariant(fp, s->channelv[i].data.variantv[j]);

		fwrite(s->channelv[i].data.trig, sizeof(vtrig), s->songlen, fp);
		fwrite(s->channelv[i].data.songv->rowv, sizeof(row), s->songlen, fp);
	}

	/* instrumenti */
	for (i = 0; i < INSTRUMENT_MAX; i++) fputc(s->instrumenti[i], fp);

	/* instrumentv */
	instrument *iv;
	for (i = 0; i < s->instrumentc; i++)
	{
		iv = &s->instrumentv[i];
		fwrite(&iv->samplelength, sizeof(uint32_t), 1, fp);
		fwrite(&iv->length, sizeof(uint32_t), 1, fp);
		fwrite(&iv->channels, sizeof(uint8_t), 1, fp);
		fwrite(&iv->channelmode, sizeof(int8_t), 1, fp);
		fwrite(&iv->c5rate, sizeof(uint32_t), 1, fp);
		fwrite(&iv->samplerate, sizeof(uint8_t), 1, fp);
		fwrite(&iv->bitdepth, sizeof(uint8_t), 1, fp);
		fwrite(&iv->cyclelength, sizeof(uint16_t), 1, fp);
		fwrite(&iv->pitchshift, sizeof(uint8_t), 1, fp);
		fwrite(&iv->timestretch, sizeof(bool), 1, fp);
		fwrite(iv->trim, sizeof(uint32_t), 2, fp);
		fwrite(&iv->loop, sizeof(uint32_t), 1, fp);
		fwrite(&iv->envelope, sizeof(uint8_t), 1, fp);
		fwrite(&iv->sustain, sizeof(bool), 1, fp);
		fwrite(&iv->gain, sizeof(uint8_t), 1, fp);
		fwrite(&iv->invert, sizeof(bool), 1, fp);
		fwrite(&iv->pingpong, sizeof(bool), 1, fp);
		fwrite(&iv->loopramp, sizeof(uint8_t), 1, fp);
		fwrite(&iv->midichannel, sizeof(int8_t), 1, fp);
		if (iv->samplelength)
			fwrite(iv->sampledata, sizeof(short), iv->samplelength, fp);
	}

	fclose(fp);
	fcntl(0, F_SETFL, O_NONBLOCK); /* non-blocking */
	snprintf(w->command.error, COMMAND_LENGTH, "'%s' written", pathext);
	free(pathext);
	return 0;
}
song *readSong(char *path)
{
	fcntl(0, F_SETFL, 0); /* blocking */
	FILE *fp = fopen(path, "r");
	if (!fp) // file doesn't exist, or fopen otherwise failed
	{
		strcpy(w->filepath, path);
		redraw(); return NULL;
	}
	DIR *dp = opendir(path);
	if (dp) // file is a directory
	{
		closedir(dp); fclose(fp);
		fcntl(0, F_SETFL, O_NONBLOCK); /* non-blocking */
		snprintf(w->command.error, COMMAND_LENGTH, "file '%s' is a directory", path);
		redraw(); return NULL;
	}

	int i, j;

	/* ensure egg wasn't broken in transit */
	/* the most important check */
	if (!(fgetc(fp) == 'e' && fgetc(fp) == 'g' && fgetc(fp) == 'g'))
	{
		fclose(fp);
		fcntl(0, F_SETFL, O_NONBLOCK); /* non-blocking */
		snprintf(w->command.error, COMMAND_LENGTH, "file '%s' isn't valid", path);
		redraw(); return NULL;
	}

	/* version */
	/* unsigned char filemajor = fgetc(fp);
	unsigned char fileminor = fgetc(fp); */
	fgetc(fp); /* filemajor */
	fgetc(fp); /* fileminor */
	/* if (filemajor < 1)
	{
		fclose(fp);
		fcntl(0, F_SETFL, O_NONBLOCK);
		strcpy(w->command.error, "failed to read song, file is too old");
		redraw(); return NULL;
	} */

	song *cs = _addSong();
	if (!cs)
	{
		fclose(fp);
		fcntl(0, F_SETFL, O_NONBLOCK); /* non-blocking */
		strcpy(w->command.error, "failed to read song, out of memory");
		redraw(); return NULL;
	}

	/* assume the rest of the file is valid */
	/* TODO: proper error checking lol */
	strcpy(w->filepath, path);

	/* counts */
	cs->songbpm = fgetc(fp);
	w->request = REQ_BPM;

	cs->instrumentc = fgetc(fp);
	cs->channelc = fgetc(fp);
	cs->rowhighlight = fgetc(fp);
	fread(&cs->songlen, sizeof(uint16_t), 1, fp);
	fread(cs->loop, sizeof(uint16_t), 2, fp);

	cs->channelv = calloc(cs->channelc, sizeof(channel));

	/* channels */
	for (i = 0; i < cs->channelc; i++)
	{
		_addChannel(cs, &cs->channelv[i]);
		cs->channelv[i].data.mute = fgetc(fp);
		cs->channelv[i].data.macroc = fgetc(fp);
		for (j = 0; j < VARIANT_MAX; j++)
			cs->channelv[i].data.varianti[j] = fgetc(fp);
		cs->channelv[i].data.variantc = fgetc(fp);
		for (j = 0; j < cs->channelv[i].data.variantc; j++)
			readVariant(fp, &cs->channelv[i].data.variantv[j]);

		fread(cs->channelv[i].data.trig, sizeof(vtrig), cs->songlen, fp);
		fread(cs->channelv[i].data.songv->rowv, sizeof(row), cs->songlen, fp);
	}

	/* instrumenti */
	for (i = 0; i < INSTRUMENT_MAX; i++) cs->instrumenti[i] = fgetc(fp);

	/* instrumentv */
	cs->instrumentv = calloc(cs->instrumentc, sizeof(instrument));
	instrument *iv;
	for (i = 0; i < cs->instrumentc; i++)
	{
		iv = &cs->instrumentv[i];
		fread(&iv->samplelength, sizeof(uint32_t), 1, fp);
		fread(&iv->length, sizeof(uint32_t), 1, fp);
		fread(&iv->channels, sizeof(uint8_t), 1, fp);
		fread(&iv->channelmode, sizeof(int8_t), 1, fp);
		fread(&iv->c5rate, sizeof(uint32_t), 1, fp);
		fread(&iv->samplerate, sizeof(uint8_t), 1, fp);
		fread(&iv->bitdepth, sizeof(uint8_t), 1, fp);
		fread(&iv->cyclelength, sizeof(uint16_t), 1, fp);
		fread(&iv->pitchshift, sizeof(uint8_t), 1, fp);
		fread(&iv->timestretch, sizeof(bool), 1, fp);
		fread(iv->trim, sizeof(uint32_t), 2, fp);
		fread(&iv->loop, sizeof(uint32_t), 1, fp);
		fread(&iv->envelope, sizeof(uint8_t), 1, fp);
		fread(&iv->sustain, sizeof(bool), 1, fp);
		fread(&iv->gain, sizeof(uint8_t), 1, fp);
		fread(&iv->invert, sizeof(bool), 1, fp);
		fread(&iv->pingpong, sizeof(bool), 1, fp);
		fread(&iv->loopramp, sizeof(uint8_t), 1, fp);
		fread(&iv->midichannel, sizeof(int8_t), 1, fp);
		if (iv->samplelength)
		{
			iv->sampledata = malloc(sizeof(short) * iv->samplelength);
			fread(iv->sampledata, sizeof(short), iv->samplelength, fp);
		}
	}

	fclose(fp);
	fcntl(0, F_SETFL, O_NONBLOCK); /* non-blocking */
	redraw(); return cs;
}
