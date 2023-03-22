/* caller should free the returned value */
static char *fileExtension(char *path, char *ext)
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

void writeSize(uint32_t size, FILE *fp) { fwrite(&size, sizeof(uint32_t), 1, fp); }

#define WRITE_POINTER_KEY(key, size, count, pointer, fp) fputc(key, fp); writeSize(size*count, fp); fwrite(pointer, size, count, fp);
#define WRITE_POINTER_KEY_STRING(key, string, fp) fputc(key, fp); writeSize(strlen(string)+1, fp); fwrite(string, 1, strlen(string)+1, fp);

// static void writeSongEffect(EffectChain *chain, FILE *fp)
// {
// 	WRITE_POINTER_KEY(FKE_COUNT, sizeof(uint8_t), 1, &chain->c, fp);
// 	for (uint8_t i = 0; i < chain->c; i++)
// 	{
// 		WRITE_POINTER_KEY(FKE_EFFECT, sizeof(uint8_t), 1, &i, fp);
// 		WRITE_POINTER_KEY(FKE_TYPE, sizeof(uint8_t), 1, &chain->v[i].type, fp);
// 		switch (chain->v[i].type)
// 		{
// #ifdef OML_LADSPA
// 			case EFFECT_TYPE_LADSPA:
// 				fputc(FKE_LADSPA_UID, fp); writeSize(sizeof(unsigned long) + strlen(chain->v[i].ladspa.desc->Label) + 1, fp);
// 				fwrite(&chain->v[i].ladspa.desc->UniqueID, sizeof(unsigned long), 1, fp);
// 				fwrite(chain->v[i].ladspa.desc->Label, 1, strlen(chain->v[i].ladspa.desc->Label) + 1, fp);
// 				WRITE_POINTER_KEY(FKE_LADSPA_CONTROLV, sizeof(float), chain->v[i].ladspa.controlc, chain->v[i].ladspa.controlv, fp);
// 				break;
// #endif
// #ifdef OML_LV2
// 			case EFFECT_TYPE_LV2:
// 				WRITE_POINTER_KEY_STRING(FKE_LV2_URI, lilv_node_as_string(lilv_plugin_get_uri(chain->v[i].lv2.plugin)), fp);
// 				WRITE_POINTER_KEY(FKE_LV2_CONTROLV, sizeof(float), chain->v[i].lv2.controlc, chain->v[i].lv2.controlv, fp);
// 				break;
// #endif
// 		}
// 	}
// 	fputc(FKE_EOF, fp); writeSize(0, fp);
// }

int writeSongNew(Song *cs, char *path)
{
	char *pathext = fileExtension(path, MODULE_EXTENSION);
	if (!strcmp(pathext, MODULE_EXTENSION))
	{
		free(pathext);
		if (!strlen(w->filepath))
		{
			strcpy(w->command.error, "no file name");
			p->redraw = 1; return 1;
		}
		pathext = malloc(sizeof(w->filepath) + 1);
		strcpy(pathext, w->filepath);
	} else strcpy(w->filepath, pathext);

	fcntl(0, F_SETFL, 0); /* blocking */

	FILE *fp = fopen(pathext, "w");

	WRITE_POINTER_KEY(FKH_VERSION,      sizeof(uint16_t), 1, &version,          fp);
	WRITE_POINTER_KEY(FKH_SAMPLERATE,   sizeof(uint32_t), 1, &samplerate,       fp);
	WRITE_POINTER_KEY(FKH_BPM,          sizeof(uint8_t),  1, &cs->songbpm,      fp);
	WRITE_POINTER_KEY(FKH_ROWHIGHLIGHT, sizeof(uint8_t),  1, &cs->rowhighlight, fp);
	WRITE_POINTER_KEY(FKH_SONGLEN,      sizeof(uint16_t), 1, &cs->songlen,      fp);
	WRITE_POINTER_KEY(FKH_LOOP,         sizeof(uint16_t), 2, cs->loop,          fp);

	fputc(FKH_GOTO_TRACK, fp); writeSize(0, fp);

	WRITE_POINTER_KEY(FKT_COUNT, sizeof(uint8_t), 1, &cs->track->c, fp);
	fputc(FKT_ROWSIZE, fp); writeSize(1, fp); fputc(sizeof(Row), fp);
	fputc(FKT_VTRIGSIZE, fp); writeSize(1, fp); fputc(sizeof(Vtrig), fp);
	for (uint8_t i = 0; i < cs->track->c; i++)
	{
		WRITE_POINTER_KEY(FKT_TRACK,    sizeof(uint8_t), 1,           &i,                                  fp);
		WRITE_POINTER_KEY(FKT_MUTE,     sizeof(bool),    1,           &cs->track->v[i]->mute,               fp);
		WRITE_POINTER_KEY(FKT_VTRIG,    sizeof(Vtrig),   cs->songlen, cs->track->v[i]->variant->trig,       fp);
		WRITE_POINTER_KEY(FKT_MAIN,     sizeof(Row),     cs->songlen, cs->track->v[i]->variant->main->rowv, fp);
		WRITE_POINTER_KEY(FKT_MACROC,   sizeof(uint8_t), 1,           &cs->track->v[i]->variant->macroc,    fp);
		WRITE_POINTER_KEY(FKT_VARIANTC, sizeof(uint8_t), 1,           &cs->track->v[i]->variant->c,         fp);
		WRITE_POINTER_KEY(FKT_VARIANTI, sizeof(uint8_t), VARIANT_MAX, cs->track->v[i]->variant->i,          fp);
		for (uint8_t j = 0; j < cs->track->v[i]->variant->c; j++)
		{
			WRITE_POINTER_KEY(FKT_VARIANT,       sizeof(uint8_t),  1,                                   &j,                                   fp);
			WRITE_POINTER_KEY(FKT_VARIANTV_ROWC, sizeof(uint16_t), 1,                                   &cs->track->v[i]->variant->v[j]->rowc, fp);
			WRITE_POINTER_KEY(FKT_VARIANTV_ROWV, sizeof(Row),      cs->track->v[i]->variant->v[j]->rowc, cs->track->v[i]->variant->v[j]->rowv,  fp);
		}

		// fputc(FKT_GOTO_EFFECT, fp); writeSize(0, fp);
		// writeSongEffect(cs->track->v[i]->effect, fp);
	}

	fputc(FKT_EOF, fp); writeSize(0, fp);

	fputc(FKH_GOTO_INST, fp); writeSize(0, fp);
	WRITE_POINTER_KEY(FKI_COUNT, sizeof(uint8_t), 1,              &cs->inst->c, fp);
	WRITE_POINTER_KEY(FKI_INDEX, sizeof(uint8_t), INSTRUMENT_MAX, cs->inst->i,  fp);
	for (uint8_t i = 0; i < cs->inst->c; i++)
	{
		WRITE_POINTER_KEY(FKI_INST, sizeof(uint8_t), 1, &i, fp);
		/* TODO: finish instruments */
	}
	fputc(FKI_EOF, fp); writeSize(0, fp);

	fclose(fp);
	fcntl(0, F_SETFL, O_NONBLOCK); /* non-blocking */
	snprintf(w->command.error, COMMAND_LENGTH, "'%s' written", pathext);
	free(pathext);
	p->redraw = 1; return 0;
}

// static void readSongEffect(EffectChain **chain, FILE *fp, uint16_t version)
// {
// 	uint8_t effect;
//
// 	uint8_t count;
//
// 	int key;
// 	uint32_t size;
// 	while (1)
// 	{
// 		key = fgetc(fp);
// 		fread(&size, sizeof(uint32_t), 1, fp);
// 		switch (key)
// 		{
// 			case -1: /* fall through */
// 			case FKE_EOF: return;
//
// 			case FKE_COUNT:
// 				fread(&count, size, 1, fp);
// 				*chain = realloc(*chain, sizeof(EffectChain) + count*sizeof(Effect));
// 				(*chain)->c = count;
// 				continue;
//
// #ifdef OML_LADSPA
// 			case FKE_LADSPA_UID:
// 				{
// 					unsigned long uniqueid;
// 					char *label = malloc(size - sizeof(unsigned long));
// 					fread(&uniqueid, 1, sizeof(unsigned long), fp);
// 					fread(label, 1, size - sizeof(unsigned long), fp);
//
// 					for (unsigned long i = 0; i < ladspa_db.descc; i++)
// 						if (ladspa_db.descv[i]->UniqueID == uniqueid && !strcmp(ladspa_db.descv[i]->Label, label))
// 						{
// 							initLadspaEffect(&(*chain)->v[effect].ladspa, (*chain)->input, (*chain)->output, ladspa_db.descv[i]);
// 							goto FKE_LADSPA_UID_done;
// 						}
// 					/* TODO: handle the plugin not being found */
// FKE_LADSPA_UID_done:
// 					free(label);
// 				} continue;
//
// 			case FKE_LADSPA_CONTROLV:
// 				/* allocated by FKE_LADSPA_UID */
// 				fread(&(*chain)->v[effect].ladspa.controlv, size, 1, fp);
// 				continue;
// #endif
//
// #ifdef OML_LV2
// 			case FKE_LV2_URI:
// 				{
// 					char *uri = malloc(size);
// 					fread(uri, size, 1, fp);
// 					LilvNode *node = lilv_new_uri(lv2_db.world, uri);
// 					initLV2Effect(&(*chain)->v[effect].lv2, (*chain)->input, (*chain)->output,
// 							lilv_plugins_get_by_uri(lilv_world_get_all_plugins(lv2_db.world), node));
// 					lilv_node_free(node);
// 					free(uri);
// 				} continue;
//
// 			case FKE_LV2_CONTROLV:
// 				/* allocated by FKE_LV2_URI */
// 				fread(&(*chain)->v[effect].lv2.controlv, size, 1, fp);
// 				continue;
// #endif
//
// 			case FKE_EFFECT: fread(&effect,                   size, 1, fp); continue;
// 			case FKE_TYPE:   fread(&(*chain)->v[effect].type, size, 1, fp); continue;
//
// 			default: fseek(fp, size, SEEK_CUR); continue;
// 		}
// 	}
// }

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
				addTrackRuntime(ret->track->v[track]);
				addTrackData(ret->track->v[track], ret->songlen);
				resizeVariantChain(ret->track->v[track]->variant, ret->songlen);
				continue;

			case FKT_VTRIG:         fread(ret->track->v[track]->variant->trig,              vtrigsize, size/vtrigsize, fp); continue;
			case FKT_MAIN:
				fread(&ret->track->v[track]->variant->main->rowv, rowsize, size/rowsize, fp);
				continue;
			case FKT_VARIANTV_ROWV: fread(&ret->track->v[track]->variant->v[variant]->rowv, rowsize, size/rowsize, fp); continue;
			case FKT_VARIANTV_ROWC: fread(&ret->track->v[track]->variant->v[variant]->rowc, size, 1, fp); continue;
			case FKT_VARIANT:       fread(&variant,                                         size, 1, fp); continue;
			case FKT_ROWSIZE:       fread(&rowsize,                                         size, 1, fp); continue;
			case FKT_VTRIGSIZE:     fread(&vtrigsize,                                       size, 1, fp); continue;
			case FKT_MUTE:          fread(&ret->track->v[track]->mute,                      size, 1, fp); continue;
			case FKT_MACROC:        fread(&ret->track->v[track]->variant->macroc,           size, 1, fp); continue;
			case FKT_VARIANTC:      fread(&ret->track->v[track]->variant->c,                size, 1, fp); continue;
			case FKT_VARIANTI:      fread( ret->track->v[track]->variant->i,                1, size, fp); continue;

			// case FKT_GOTO_EFFECT: readSongEffect(&ret->track->v[track]->effect, fp, version); continue;

			default: fseek(fp, size, SEEK_CUR); continue;
		}
	}
}

/* TODO: unfinished */
static void readSongInst(Song *ret, FILE *fp, uint16_t version, float ratemultiplier)
{
	uint8_t inst;

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
			case FKI_EOF: return;

			case FKI_COUNT:
				fread(&count, size, 1, fp);
				ret->inst = calloc(1, sizeof(InstChain) + count*sizeof(Inst));
				ret->inst->c = count;
				continue;

			case FKI_INST:  fread(&inst,        size, 1, fp); continue;
			case FKI_INDEX: fread(ret->inst->i, 1, size, fp); continue;

			default: fseek(fp, size, SEEK_CUR); continue;
		}
	}
}

Song *readSongNew(char *path)
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

	Song *ret = addSong();
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
					uint32_t filesamplerate;
					fread(&filesamplerate, size, 1, fp);
					ratemultiplier = (float)samplerate / (float)filesamplerate;
				} continue;

			case FKH_VERSION:      fread(&version          , size, 1, fp); continue;
			case FKH_BPM:          fread(&ret->songbpm     , size, 1, fp); continue;
			case FKH_ROWHIGHLIGHT: fread(&ret->rowhighlight, size, 1, fp); continue;
			case FKH_SONGLEN:      fread(&ret->songlen     , size, 1, fp); continue;
			case FKH_LOOP:         fread(ret->loop         , size, 1, fp); continue;

			case FKH_GOTO_TRACK: readSongTrack(ret, fp, version); continue;
			case FKH_GOTO_INST:  readSongInst(ret, fp, version, ratemultiplier); continue;

			default: fseek(fp, size, SEEK_CUR); continue;
		}
	}

	fclose(fp);
	fcntl(0, F_SETFL, O_NONBLOCK); /* non-blocking */
	p->redraw = 1; return ret;
}
