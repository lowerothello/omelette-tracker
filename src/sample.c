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
	ret->gain = 0xff;

loadSample_end:
	if (sndfile) sf_close(sndfile);
	fcntl(0, F_SETFL, O_NONBLOCK); /* non-blocking */
	return ret;
}

void copySample(Sample **dest, Sample *src)
{
	size_t size = sizeof(Sample) + sizeof(short) * src->length * src->channels;
	(*dest) = malloc(size);
	memcpy((*dest), src, size);
}

struct json_object *serializeSample(Sample *s, size_t *dataoffset)
{
	struct json_object *ret = json_object_new_object();
	json_object_object_add(ret, "length", json_object_new_uint64(s->length));
	json_object_object_add(ret, "channels", json_object_new_int(s->channels));
	json_object_object_add(ret, "rate", json_object_new_uint64(s->rate));
	json_object_object_add(ret, "defrate", json_object_new_uint64(s->defrate));
	json_object_object_add(ret, "gain", json_object_new_int(s->gain));
	json_object_object_add(ret, "invert", json_object_new_boolean(s->invert));

	json_object_object_add(ret, "trimstart", json_object_new_uint64(s->trimstart));
	json_object_object_add(ret, "trimlength", json_object_new_uint64(s->trimlength));
	json_object_object_add(ret, "looplength", json_object_new_uint64(s->looplength));
	json_object_object_add(ret, "pingpong", json_object_new_boolean(s->pingpong));
	json_object_object_add(ret, "loopramp", json_object_new_int(s->loopramp));
	json_object_object_add(ret, "dataoffset", json_object_new_uint64(*dataoffset));
	*dataoffset += sizeof(short) * s->length * s->channels;

	return ret;
}

void serializeSampleData(FILE *fp, Sample *s, size_t *dataoffset)
{
	fwrite(s->data, sizeof(short), s->length * s->channels, fp);
	*dataoffset += sizeof(short) * s->length * s->channels;
}

Sample *deserializeSample(struct json_object *jso, void *data, double ratemultiplier)
{
	Sample *ret = calloc(1, sizeof(Sample) + sizeof(short)
			* json_object_get_uint64(json_object_object_get(jso, "length"))
			* json_object_get_int(json_object_object_get(jso, "channels")));

	ret->length = json_object_get_uint64(json_object_object_get(jso, "length"));
	ret->channels = json_object_get_int(json_object_object_get(jso, "channels"));
	ret->rate = json_object_get_uint64(json_object_object_get(jso, "rate")) * ratemultiplier;
	ret->defrate = json_object_get_uint64(json_object_object_get(jso, "defrate")) * ratemultiplier;
	ret->gain = json_object_get_int(json_object_object_get(jso, "gain"));
	ret->invert = json_object_get_boolean(json_object_object_get(jso, "invert"));
	ret->trimstart = json_object_get_uint64(json_object_object_get(jso, "trimstart"));
	ret->trimlength = json_object_get_uint64(json_object_object_get(jso, "trimlength"));
	ret->looplength = json_object_get_uint64(json_object_object_get(jso, "looplength"));
	ret->pingpong = json_object_get_boolean(json_object_object_get(jso, "pingpong"));
	ret->loopramp = json_object_get_int(json_object_object_get(jso, "loopramp"));
	
	size_t dataoffset = json_object_get_uint64(json_object_object_get(jso, "dataoffset"));
	memcpy(ret->data, (void*)((size_t)data + dataoffset), sizeof(short) * ret->length * ret->channels);

	return ret;
}
