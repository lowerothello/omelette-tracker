/* caller should free the returned value */
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

void writeSongNew(Song *cs, FILE *fp)
{
}

static void readSongEffect(EffectChain **chain, FILE *fp, uint16_t version)
{
	uint8_t effect;

	uint8_t count;

	int key;
	uint32_t size;
	while (1)
	{
		key = fgetc(fp);
		fread(&size, sizeof(uint32_t), 1, fp);
		switch (key)
		{
			case -1: /* fall through */
			case FKE_EOF: return;

			case FKE_COUNT:
				fread(&count, size, 1, fp);
				*chain = calloc(1, sizeof(EffectChain) + count*sizeof(Effect));
				(*chain)->c = count;
				continue;

			case FKE_EFFECT:   fread(&effect,                   size, 1, fp);
			case FKE_TYPE:     fread(&(*chain)->v[effect].type, size, 1, fp);

			default: fseek(fp, size, SEEK_CUR); continue;
		}
	}
}

static void readSongTrack(Song *ret, FILE *fp, uint16_t version)
{
	uint8_t track, variant;
	uint8_t vtrigsize;
	uint8_t rowsize;

	uint8_t count;

	int key;
	uint32_t size;
	while (1)
	{
		key = fgetc(fp);
		fread(&size, sizeof(uint32_t), 1, fp);
		switch (key)
		{
			case -1: /* fall through */
			case FKT_EOF: return;

			case FKT_COUNT:
				fread(&count, size, 1, fp);
				ret->track = calloc(1, sizeof(TrackChain) + count*sizeof(Track));
				ret->track->c = count;
				continue;

			case FKT_TRACK:
				fread(&track, size, 1, fp);
				ret->track->v[track].data.variant->songlen = ret->songlen;
				resizeVariantChain(ret->track->v[track].data.variant, ret->songlen);
				continue;

			case FKT_VTRIG:         fread(&ret->track->v[track].data.variant->trig,             vtrigsize, size/vtrigsize, fp); continue;
			case FKT_MAIN:          fread(&ret->track->v[track].data.variant->main->rowv,       rowsize, size/rowsize, fp); continue;
			case FKT_VARIANTV_ROWV: fread(&ret->track->v[track].data.variant->v[variant]->rowv, rowsize, size/rowsize, fp); continue;
			case FKT_VARIANTV_ROWC: fread(&ret->track->v[track].data.variant->v[variant]->rowc, size, 1, fp); continue;
			case FKT_VARIANT:       fread(&variant,                                             size, 1, fp); continue;
			case FKT_ROWSIZE:       fread(&rowsize,                                             size, 1, fp); continue;
			case FKT_VTRIGSIZE:     fread(&vtrigsize,                                           size, 1, fp); continue;
			case FKT_MUTE:          fread(&ret->track->v[track].data.mute,                      size, 1, fp); continue;
			case FKT_MACROC:        fread(&ret->track->v[track].data.variant->macroc,           size, 1, fp); continue;
			case FKT_VARIANTC:      fread(&ret->track->v[track].data.variant->c,                size, 1, fp); continue;
			case FKT_VARIANTI:      fread( ret->track->v[track].data.variant->i,                1, size, fp); continue;

			case FKT_GOTO_EFFECT: readSongEffect(&ret->track->v[track].data.effect, fp, version); continue;

			default: fseek(fp, size, SEEK_CUR); continue;
		}
	}
}

Song *readSongNew(FILE *fp)
{
	Song *ret = _addSong();
	uint16_t version;
	float ratemultiplier = 1.0f;

	int key;
	uint32_t size;
	while (1)
	{
		key = fgetc(fp);
		fread(&size, sizeof(uint32_t), 1, fp);
		switch (key)
		{
			case -1: /* fall through */
			case FKH_EOF: return ret;

			case FKH_SAMPLERATE:
				{
					jack_nframes_t filesamplerate;
					fread(&filesamplerate, size, 1, fp);
					ratemultiplier = (float)samplerate / (float)filesamplerate;
				} continue;

			case FKH_VERSION:      fread(&version          , size, 1, fp); continue;
			case FKH_BPM:          fread(&ret->songbpm     , size, 1, fp); continue;
			case FKH_ROWHIGHLIGHT: fread(&ret->rowhighlight, size, 1, fp); continue;
			case FKH_LOOP:         fread(ret->loop         , size, 1, fp); continue;

			case FKH_GOTO_TRACK: readSongTrack(ret, fp, version); continue;

			default: fseek(fp, size, SEEK_CUR); continue;
		}
	}
}
