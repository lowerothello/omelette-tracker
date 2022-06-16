void samplerIncFieldPointer(signed char *fieldpointer, short index)
{
	(*fieldpointer)++;
	switch (index)
	{
		case 0: case 2:                 *fieldpointer = 0; break;
		case 1:  if (*fieldpointer > 7) *fieldpointer = 0; break;
		case 3:  if (*fieldpointer > 3) *fieldpointer = 0; break;
		case 4:  if (*fieldpointer > 7) *fieldpointer = 0; break;
		case 5:  if (*fieldpointer > 7) *fieldpointer = 0; break;
		case 6:  if (*fieldpointer > 7) *fieldpointer = 0; break;
		case 7:  if (*fieldpointer > 7) *fieldpointer = 0; break;
		default:                        *fieldpointer = 0; break;
	}
}
void samplerDecFieldPointer(signed char *fieldpointer, short index)
{
	(*fieldpointer)--;
	if (*fieldpointer < 0)
		switch (index)
		{
			case 0: case 2: *fieldpointer = 0; break;
			case 1:         *fieldpointer = 7; break;
			case 3:         *fieldpointer = 3; break;
			case 4:         *fieldpointer = 7; break;
			case 5:         *fieldpointer = 7; break;
			case 6:         *fieldpointer = 7; break;
			case 7:         *fieldpointer = 7; break;
			default:        *fieldpointer = 0; break;
		}
}
void samplerEndFieldPointer(signed char *fieldpointer, short index)
{
	switch (index)
	{
		case 0: case 2: *fieldpointer = 0; break;
		case 1:         *fieldpointer = 7; break;
		case 3:         *fieldpointer = 3; break;
		case 4:         *fieldpointer = 7; break;
		case 5:         *fieldpointer = 7; break;
		case 6:         *fieldpointer = 7; break;
		case 7:         *fieldpointer = 7; break;
		default:        *fieldpointer = 0; break;
	}
}

void inputSamplerHex(signed char *fieldpointer, short index, sampler_state *ss, char value)
{
	switch (index)
	{
		case 1:  updateField(*fieldpointer, 8, (uint32_t *)&ss->c5rate, value); break;
		case 3:  updateField(*fieldpointer, 4, (uint32_t *)&ss->cyclelength, value); break;
		case 4:  updateField(*fieldpointer, 8, (uint32_t *)&ss->trim[0], value); if (ss->trim[0] > ss->length) ss->trim[0] = ss->length; break;
		case 5:  updateField(*fieldpointer, 8, (uint32_t *)&ss->trim[1], value); if (ss->trim[1] > ss->length) ss->trim[1] = ss->length; break;
		case 6:  updateField(*fieldpointer, 8, (uint32_t *)&ss->loop[0], value); if (ss->loop[0] > ss->length) ss->loop[0] = ss->length; break;
		case 7:  updateField(*fieldpointer, 8, (uint32_t *)&ss->loop[1], value); if (ss->loop[1] > ss->length) ss->loop[1] = ss->length; break;
		case 8:  updateFieldPush(&ss->volume.a, value); break;
		case 9:  updateFieldPush(&ss->volume.d, value); break;
		case 10: updateFieldPush(&ss->volume.s, value); break;
		case 11: updateFieldPush(&ss->volume.r, value); break;
	}
	samplerIncFieldPointer(fieldpointer, index);
}

void drawSampler(instrument *iv, uint8_t index, unsigned short x, unsigned short y, short *cursor, unsigned char fieldpointer)
{
	sampler_state *ss = iv->state[iv->type];
	unsigned char sampletitleoffset;
	if (!iv->samplelength)
	{
		sampletitleoffset = 26;
		printf("\033[%d;%dH [sampler (no sample)] ", y - 1, x+26);
	}
	else switch (ss->channels)
	{
		case 1:
			sampletitleoffset = 24;
			printf("\033[%d;%dH [sampler (%08x mono)] ", y - 1, x+24, ss->length);
			break;
		default:
			sampletitleoffset = 23;
			printf("\033[%d;%dH [sampler (%08x stereo)] ", y - 1, x+23, ss->length);
			break;
	}

	printf("\033[%d;%dHC-5 rate    [%08x]", y+1, x+3, ss->c5rate);
	printf("\033[%d;%dHfixed tempo        ", y+2, x+3);
	if (ss->attributes & 0b1) printf("[X]");
	else                      printf("[ ]");
	printf("\033[%d;%dHcycle length    [%04x]", y+3, x+3, ss->cyclelength);

	for (char i = 1; i < 5; i++)
		printf("\033[%d;%dH│", y+i, x+28);

	printf("\033[%d;%dHtrim", y+1, x+35);
	printf("\033[%d;%dHloop", y+1, x+48);
	printf("\033[%d;%dH[%08x]   [%08x]", y+2, x+32, ss->trim[0], ss->loop[0]);
	printf("\033[%d;%dH[%08x]   [%08x]", y+3, x+32, ss->trim[1], ss->loop[1]);
	printf("\033[%d;%dH%3d%% -> %d%%", y+4, x+31,
			(char)((float)ss->trim[0] / (float)ss->length * 100),
			(char)((float)ss->trim[1] / (float)ss->length * 100));
	printf("\033[%d;%dH%3d%% -> %d%%", y+4, x+44,
			(char)((float)ss->loop[0] / (float)ss->length * 100),
			(char)((float)ss->loop[1] / (float)ss->length * 100));

	printf("\033[%d;%dH│         a[%02x]", y+1, x+58, ss->volume.a);
	printf("\033[%d;%dH│   env   d[%02x]", y+2, x+58, ss->volume.d);
	printf("\033[%d;%dH│   ===   s[%02x]", y+3, x+58, ss->volume.s);
	printf("\033[%d;%dH│         r[%02x]", y+4, x+58, ss->volume.r);

	if (w->instrumentrecv == INST_REC_LOCK_CONT)
		printf("\033[%d;%dHREC", y, x + 1);

	switch (*cursor)
	{
		case 0:  printf("\033[%d;%dH", y-1,  x+2  + sampletitleoffset); break;
		case 1:  printf("\033[%d;%dH", y+1,  x+16 + fieldpointer); break;
		case 2:  printf("\033[%d;%dH", y+2,  x+23); break;
		case 3:  printf("\033[%d;%dH", y+3,  x+20 + fieldpointer); break;
		case 4:  printf("\033[%d;%dH", y+2,  x+33 + fieldpointer); break;
		case 5:  printf("\033[%d;%dH", y+3,  x+33 + fieldpointer); break;
		case 6:  printf("\033[%d;%dH", y+2,  x+46 + fieldpointer); break;
		case 7:  printf("\033[%d;%dH", y+3,  x+46 + fieldpointer); break;
		case 8:  printf("\033[%d;%dH", y+1,  x+71); break;
		case 9:  printf("\033[%d;%dH", y+2,  x+71); break;
		case 10: printf("\033[%d;%dH", y+3,  x+71); break;
		case 11: printf("\033[%d;%dH", y+4,  x+71); break;
	}
}

void samplerAdjustUp(instrument *iv, short index)
{
	sampler_state *ss = iv->state[iv->type];
	switch (index)
	{
		case 1: ss->c5rate = ss->c5rate * powf(M_12_ROOT_2, 1); break;
		case 3: ss->cyclelength += 0x20; break;
		case 4:
			ss->trim[0] += MAX(ss->length / 50.0, 1);
			if (ss->trim[0] > ss->length) ss->trim[0] = ss->length;
			break;
		case 5:
			ss->trim[1] += MAX(ss->length / 50.0, 1);
			if (ss->trim[1] > ss->length) ss->trim[1] = ss->length;
			break;
		case 6:
			ss->loop[0] += MAX(ss->length / 50.0, 1);
			if (ss->loop[0] > ss->length) ss->loop[0] = ss->length;
			break;
		case 7:
			ss->loop[1] += MAX(ss->length / 50.0, 1);
			if (ss->loop[1] > ss->length) ss->loop[1] = ss->length;
			break;
		case 8:  ss->volume.a++; break;
		case 9:  ss->volume.d++; break;
		case 10: ss->volume.s++; break;
		case 11: ss->volume.r++; break;
	}
}
void samplerAdjustDown(instrument *iv, short index)
{
	sampler_state *ss = iv->state[iv->type];
	uint32_t oldpos;
	switch (index)
	{
		case 1: ss->c5rate = ss->c5rate * powf(M_12_ROOT_2, -1); break;
		case 3: ss->cyclelength -= 0x20; break;
		case 4:
			oldpos = ss->trim[0];
			ss->trim[0] -= MAX(ss->length / 50.0, 1);
			if (ss->trim[0] > oldpos) ss->trim[0] = 0;
			break;
		case 5:
			oldpos = ss->trim[1];
			ss->trim[1] -= MAX(ss->length / 50.0, 1);
			if (ss->trim[1] > oldpos) ss->trim[1] = 0;
			break;
		case 6:
			oldpos = ss->loop[0];
			ss->loop[0] -= MAX(ss->length / 50.0, 1);
			if (ss->loop[0] > oldpos) ss->loop[0] = 0;
			break;
		case 7:
			oldpos = ss->loop[1];
			ss->loop[1] -= MAX(ss->length / 50.0, 1);
			if (ss->loop[1] > oldpos) ss->loop[1] = 0;
			break;
		case 8:  ss->volume.a--; break;
		case 9:  ss->volume.d--; break;
		case 10: ss->volume.s--; break;
		case 11: ss->volume.r--; break;
	}
}
void samplerAdjustLeft(instrument *iv, short index)
{
	sampler_state *ss = iv->state[iv->type];
	uint32_t oldpos;
	switch (index)
	{
		case 1: ss->c5rate = ss->c5rate * powf(M_12_ROOT_2, -0.2); break;
		case 4:
			oldpos = ss->trim[0];
			ss->trim[0] -= MAX(ss->length / 1000.0, 1);
			if (ss->trim[0] > oldpos) ss->trim[0] = 0;
			break;
		case 5:
			oldpos = ss->trim[1];
			ss->trim[1] -= MAX(ss->length / 1000.0, 1);
			if (ss->trim[1] > oldpos) ss->trim[1] = 0;
			break;
		case 6:
			oldpos = ss->loop[0];
			ss->loop[0] -= MAX(ss->length / 1000.0, 1);
			if (ss->loop[0] > oldpos) ss->loop[0] = 0;
			break;
		case 7:
			oldpos = ss->loop[1];
			ss->loop[1] -= MAX(ss->length / 1000.0, 1);
			if (ss->loop[1] > oldpos) ss->loop[1] = 0;
			break;
	}
}
void samplerAdjustRight(instrument *iv, short index)
{
	sampler_state *ss = iv->state[iv->type];
	switch (index)
	{
		case 1: ss->c5rate = ss->c5rate * powf(M_12_ROOT_2, 0.2); break;
		case 4:
			ss->trim[0] += MAX(ss->length / 1000.0, 1);
			if (ss->trim[0] > ss->length) ss->trim[0] = ss->length;
			break;
		case 5:
			ss->trim[1] += MAX(ss->length / 1000.0, 1);
			if (ss->trim[1] > ss->length) ss->trim[1] = ss->length;
			break;
		case 6:
			ss->loop[0] += MAX(ss->length / 1000.0, 1);
			if (ss->loop[0] > ss->length) ss->loop[0] = ss->length;
			break;
		case 7:
			ss->loop[1] += MAX(ss->length / 1000.0, 1);
			if (ss->loop[1] > ss->length) ss->loop[1] = ss->length;
			break;
	}
}


int samplerAmplifyCallback(char *command, unsigned char *mode)
{
	char *buffer = malloc(strlen(command) + 1);
	wordSplit(buffer, command, 0);

	instrument *iv = s->instrumentv[s->instrumenti[w->instrument]];
	if (iv->samplelength == 0) return 0; /* no sample data */

	for (uint32_t ptr = 0; ptr < iv->samplelength; ptr++)
	{
		int c = ((int)iv->sampledata[ptr] * (float)strtol(buffer, NULL, 0)) / 100.0;
		if      (c > SHRT_MAX) iv->sampledata[ptr] = SHRT_MAX;
		else if (c < SHRT_MIN) iv->sampledata[ptr] = SHRT_MIN;
		else                   iv->sampledata[ptr] = c;
	}
	free(buffer); buffer = NULL;
	pushInstrumentHistory(iv);
	return 0;
}
int samplerAkaizerCallback3(char *command, unsigned char *mode)
{
	char *buffer = malloc(COMMAND_LENGTH + 1);
	char *altbuffer = malloc(COMMAND_LENGTH + 1);
	wordSplit(altbuffer, command, 0);
	exportSample(w->instrument, "/tmp/omelette.wav");
	snprintf(buffer, COMMAND_LENGTH, "akaizer /tmp/omelette.wav %d %d %d",
		w->akaizertimefactor,
		w->akaizercyclelength,
		(short)strtol(altbuffer, NULL, 0));
	printf("\033[2J"); fflush(stdout);
	system(buffer);
	snprintf(buffer, COMMAND_LENGTH, "/tmp/omelette-%d%%_%d_%d_R.wav",
		w->akaizertimefactor,
		w->akaizercyclelength,
		(short)strtol(altbuffer, NULL, 0));

	FILE *fp = fopen(buffer, "r");
	if (!fp) /* akaizer failed to create output */
	{
		snprintf(w->command.error, COMMAND_LENGTH, "Akaizer failed to create a file");
		return 0;
	}

	instrument *iv = s->instrumentv[s->instrumenti[w->instrument]];
	sampler_state *ss = iv->state[iv->type];
	free(iv->sampledata); iv->sampledata = NULL;
	SF_INFO sfinfo;
	iv->sampledata = _loadSample(buffer, &sfinfo);

	ss->trim[0] = ss->trim[0] * (float)sfinfo.frames / (float)ss->length;
	ss->trim[1] = ss->trim[1] * (float)sfinfo.frames / (float)ss->length;
	ss->loop[0] = ss->loop[0] * (float)sfinfo.frames / (float)ss->length;
	ss->loop[1] = ss->loop[1] * (float)sfinfo.frames / (float)ss->length;

	ss->length = sfinfo.frames;
	free(buffer);    buffer = NULL;
	free(altbuffer); altbuffer = NULL;
	pushInstrumentHistory(iv);
	return 0;
}
int samplerAkaizerCallback2(char *command, unsigned char *mode)
{
	char *buffer = malloc(strlen(command) + 1);
	wordSplit(buffer, command, 0);
	w->akaizercyclelength = strtol(buffer, NULL, 0);
	setCommand(&w->command, &samplerAkaizerCallback3, NULL, 0, "Akaizer transpose [-24 to +24]: ", "0");
	*mode = 255;
	free(buffer); buffer = NULL;
	return 0;
}
int samplerAkaizerCallback1(char *command, unsigned char *mode)
{
	char *buffer = malloc(strlen(command) + 1);
	wordSplit(buffer, command, 0);
	w->akaizertimefactor = strtol(buffer, NULL, 0);
	setCommand(&w->command, &samplerAkaizerCallback2, NULL, 0, "Akaizer cycle length [20 to 2000]: ", "1000");
	*mode = 255;
	free(buffer); buffer = NULL;
	return 0;
}
int samplerLameCallback(char *command, unsigned char *mode)
{
	char *buffer = malloc(strlen(command) + 1);
	wordSplit(buffer, command, 0);
	exportSample(w->instrument, "/tmp/omelette.wav");
	snprintf(buffer, COMMAND_LENGTH, "lame -b%d /tmp/omelette.wav",
		(short)strtol(buffer, NULL, 0));
	system(buffer);

	instrument *iv = s->instrumentv[s->instrumenti[w->instrument]];
	sampler_state *ss = iv->state[iv->type];
	free(iv->sampledata); iv->sampledata = NULL;
	SF_INFO sfinfo;
	iv->sampledata = _loadSample("/tmp/omelette.mp3", &sfinfo);

	ss->length = sfinfo.frames;
	ss->trim[0] = MIN(sfinfo.frames, ss->trim[0]);
	ss->trim[1] = MIN(sfinfo.frames, ss->trim[1]);
	ss->loop[0] = MIN(sfinfo.frames, ss->loop[0]);
	ss->loop[1] = MIN(sfinfo.frames, ss->loop[1]);
	free(buffer); buffer = NULL;
	pushInstrumentHistory(iv);
	return 0;
}
int samplerResampleCallback(char *command, unsigned char *mode)
{
	char *buffer = malloc(strlen(command) + 1);
	wordSplit(buffer, command, 0);
	long newrate = strtol(buffer, NULL, 0);

	instrument *iv = s->instrumentv[s->instrumenti[w->instrument]];
	if (iv->samplelength == 0) return 1; /* no sample data */

	sampler_state *ss = iv->state[iv->type];
	uint32_t newlen = ss->length * ((float)newrate / (float)ss->c5rate);

	/* malloc a new buffer */
	short *sampledata = malloc(sizeof(short) * newlen * ss->channels);
	if (sampledata == NULL) { /* malloc failed */
		strcpy(w->command.error, "failed to resample, out of memory");
		free(buffer); buffer = NULL;
		return 0;
	}

	uint32_t i, pitchedpointer;
	for (i = 0; i < newlen * ss->channels; i++)
	{
		pitchedpointer = (float)i * (float)ss->c5rate / (float)newrate;
		sampledata[i] = iv->sampledata[pitchedpointer];
	}

	free(iv->sampledata); iv->sampledata = NULL;
	iv->sampledata = sampledata;
	iv->samplelength = newlen * ss->channels;
	ss->length = newlen;
	ss->trim[0] = ss->trim[0] * (float)newrate / (float)ss->c5rate;
	ss->trim[1] = ss->trim[1] * (float)newrate / (float)ss->c5rate;
	ss->loop[0] = ss->loop[0] * (float)newrate / (float)ss->c5rate;
	ss->loop[1] = ss->loop[1] * (float)newrate / (float)ss->c5rate;

	ss->c5rate = newrate;
	free(buffer); buffer = NULL;
	pushInstrumentHistory(iv);
	return 0;
}
void samplerApplyTrimming(instrument *iv)
{
	pushInstrumentHistoryIfNew(iv);
	if (iv->samplelength > 0)
	{
		sampler_state *ss = iv->state[iv->type];
		uint32_t newlen
			= MAX(ss->trim[0], ss->trim[1])
			- MIN(ss->trim[0], ss->trim[1]);

		/* malloc a new buffer */
		short *sampledata = malloc(sizeof(short) * newlen * ss->channels);
		if (sampledata == NULL)
		{
			strcpy(w->command.error, "failed to apply trim, out of memory");
			return;
		}

		uint32_t startOffset = MIN(ss->trim[0], ss->trim[1]);
		memcpy(sampledata,
				iv->sampledata+(sizeof(short) * startOffset),
				sizeof(short) * newlen * ss->channels);

		free(iv->sampledata); iv->sampledata = NULL;
		iv->sampledata = sampledata;
		iv->samplelength = newlen * ss->channels;
		ss->length = newlen;
		ss->trim[0] = ss->trim[0] - startOffset;
		ss->trim[1] = ss->trim[1] - startOffset;
	}
}
int samplerExportCallback(char *command, unsigned char *mode)
{
	char *buffer = malloc(strlen(command) + 1);
	wordSplit(buffer, command, 0);
	samplerApplyTrimming(s->instrumentv[s->instrumenti[w->instrument]]);
	exportSample(w->instrument, fileExtension(buffer, ".wav"));

	free(buffer); buffer = NULL;
	return 0;
}

void samplerInput(int *input)
{
	instrument *iv = s->instrumentv[s->instrumenti[w->instrument]];
	sampler_state *ss = iv->state[iv->type];
	switch (w->instrumentindex)
	{
		case 0:
			switch (*input)
			{
				case 10: case 13: /* return */
					w->popup = 2;
					w->instrumentindex = 0;
					w->fyoffset = 0; /* this can still be set on edge cases */
					redraw();
					*input = 0; /* don't reprocess */
					break;
				case 'm': /* amplify */
					if (iv->samplelength > 0)
					{
						setCommand(&w->command, &samplerAmplifyCallback, NULL, 0, "Amplify %: ", "100");
						w->mode = 255;
						redraw();
					}
					break;
				case 'a': /* akaizer */
					if (iv->samplelength > 0)
					{
						if (system("type akaizer >/dev/null"))
							strcpy(w->command.error, "\"akaizer\" not found in $PATH");
						else
						{
							setCommand(&w->command, &samplerAkaizerCallback1, NULL, 0, "Akaizer time factor % [25 to 2000]: ", "100");
							w->mode = 255;
							redraw();
						}
					}
					break;
				case 'l': /* lame */
					if (iv->samplelength > 0)
					{
						if (system("type lame >/dev/null"))
							strcpy(w->command.error, "\"lame\" not found in $PATH");
						else
						{
							setCommand(&w->command, &samplerLameCallback, NULL, 0, "MP3 bitrate: ", "128");
							w->mode = 255;
							redraw();
						}
					}
					break;
				case 's': /* spleeter */
					if (iv->samplelength > 0)
					{
						if (system("type spleeter >/dev/null"))
							strcpy(w->command.error, "\"spleeter\" not found in $PATH");
						else
						{
							uint8_t val[4];
							printf("\033[2J"); fflush(stdout);
							exportSample(w->instrument, "/tmp/omelette.wav");
							system("spleeter separate -p spleeter:4stems /tmp/omelette.wav");
							val[0] = newInstrument(w->instrument + 1); addInstrument(val[0]);
							val[1] = newInstrument(w->instrument + 2); addInstrument(val[1]);
							val[2] = newInstrument(w->instrument + 3); addInstrument(val[2]);
							val[3] = newInstrument(w->instrument + 4); addInstrument(val[3]);
							loadSample(val[0], "/tmp/separated_audio/omelette/vocals.wav");
							loadSample(val[1], "/tmp/separated_audio/omelette/bass.wav"  );
							loadSample(val[2], "/tmp/separated_audio/omelette/drums.wav" );
							loadSample(val[3], "/tmp/separated_audio/omelette/other.wav" );

							snprintf(w->command.error, COMMAND_LENGTH,
								"Separated stems into instrument slots %x %x %x %x",
								val[0], val[1], val[2], val[3]);

							redraw();
						}
					}
					break;
				case 't': /* apply trimming */
					samplerApplyTrimming(iv);
					redraw();
					break;
				case 'c': /* signed unsigned conversion */
					if (iv->samplelength > 0)
					{
						uint32_t ptr;
						for (ptr = 0; ptr < iv->samplelength; ptr++)
							/* flip the sign bit */
							iv->sampledata[ptr] ^= 0b1000000000000000;
					}
					break;
				case 'p': /* invert phase */
					if (iv->samplelength > 0)
					{
						uint32_t ptr;
						for (ptr = 0; ptr < iv->samplelength; ptr++)
						{
							if (iv->sampledata[ptr] >= 0) iv->sampledata[ptr] += SHRT_MIN;
							else if (iv->sampledata[ptr] == SHRT_MIN) iv->sampledata[ptr] = SHRT_MAX;
							else { iv->sampledata[ptr] *= -1; iv->sampledata[ptr] -= 1; }
						}
					}
					break;
				case 'd': /* decimate / resample */
					if (iv->samplelength > 0)
					{
						char buffer[COMMAND_LENGTH];
						snprintf(buffer, COMMAND_LENGTH, "%d",
							ss->c5rate);
						setCommand(&w->command, &samplerResampleCallback, NULL, 0, "New C-5 rate: ", buffer);
						w->mode = 255;
						redraw();
					}
					break;
				case 'r': /* arm for recording */
					if (w->instrumentrecv == INST_REC_LOCK_OK)
						w->instrumentreci = s->instrumenti[w->instrument];
					if (w->instrumentreci == s->instrumenti[w->instrument])
					{
						switch (w->instrumentrecv)
						{
							case INST_REC_LOCK_OK:
								w->recbuffer = malloc(sizeof(short) * RECORD_LENGTH * samplerate * 2);
								if (w->recbuffer == NULL)
								{
									strcpy(w->command.error, "failed to start recording, out of memory");
									break;
								}
								w->recptr = 0;
								w->instrumentrecv = INST_REC_LOCK_CONT;
								break;
							case INST_REC_LOCK_CONT:
								w->instrumentrecv = INST_REC_LOCK_PREP_END;
								break;
						}
					}
					redraw();
					break;
				case 'e': /* export */
					setCommand(&w->command, &samplerExportCallback, NULL, 0, "File name: ", "");
					w->mode = 255;
					redraw();
					break;
			}
			break;
		case 2: /* fixed tempo button */
			switch (*input)
			{
				case 10: case 13: /* return */
					ss->attributes ^= 0b1; /* invert the first bit */
					redraw();
					*input = 0; /* don't reprocess */
					break;
			}
			break;
		default:
			switch (*input)
			{
				case 1: /* ^a */
					switch (w->instrumentindex)
					{
						case 1:  ss->c5rate++; break;
						case 3:  ss->cyclelength++; break;
						case 4:  if (ss->trim[0] == ss->length) ss->trim[0] = 0; else ss->trim[0]++; break;
						case 5:  if (ss->trim[1] == ss->length) ss->trim[1] = 0; else ss->trim[1]++; break;
						case 6:  if (ss->loop[0] == ss->length) ss->loop[0] = 0; else ss->loop[0]++; break;
						case 7:  if (ss->loop[1] == ss->length) ss->loop[1] = 0; else ss->loop[1]++; break;
						case 8:  ss->volume.a++; break;
						case 9:  ss->volume.d++; break;
						case 10: ss->volume.s++; break;
						case 11: ss->volume.r++; break;
					}
					break;
				case 24: /* ^x */
					switch (w->instrumentindex)
					{
						case 1:  ss->c5rate--; break;
						case 3:  ss->cyclelength--; break;
						case 4:  ss->trim[0]--; if (ss->trim[0] > ss->length) ss->trim[0] = ss->length; break;
						case 5:  ss->trim[1]--; if (ss->trim[1] > ss->length) ss->trim[1] = ss->length; break;
						case 6:  ss->loop[0]--; if (ss->loop[0] > ss->length) ss->loop[0] = ss->length; break;
						case 7:  ss->loop[1]--; if (ss->loop[1] > ss->length) ss->loop[1] = ss->length; break;
						case 8:  ss->volume.a--; break;
						case 9:  ss->volume.d--; break;
						case 10: ss->volume.s--; break;
						case 11: ss->volume.r--; break;
					}
					break;
				case '0':           inputSamplerHex(&w->fieldpointer, w->instrumentindex, ss, 0);   break;
				case '1':           inputSamplerHex(&w->fieldpointer, w->instrumentindex, ss, 1);   break;
				case '2':           inputSamplerHex(&w->fieldpointer, w->instrumentindex, ss, 2);   break;
				case '3':           inputSamplerHex(&w->fieldpointer, w->instrumentindex, ss, 3);   break;
				case '4':           inputSamplerHex(&w->fieldpointer, w->instrumentindex, ss, 4);   break;
				case '5':           inputSamplerHex(&w->fieldpointer, w->instrumentindex, ss, 5);   break;
				case '6':           inputSamplerHex(&w->fieldpointer, w->instrumentindex, ss, 6);   break;
				case '7':           inputSamplerHex(&w->fieldpointer, w->instrumentindex, ss, 7);   break;
				case '8':           inputSamplerHex(&w->fieldpointer, w->instrumentindex, ss, 8);   break;
				case '9':           inputSamplerHex(&w->fieldpointer, w->instrumentindex, ss, 9);   break;
				case 'A': case 'a': inputSamplerHex(&w->fieldpointer, w->instrumentindex, ss, 10);  break;
				case 'B': case 'b': inputSamplerHex(&w->fieldpointer, w->instrumentindex, ss, 11);  break;
				case 'C': case 'c': inputSamplerHex(&w->fieldpointer, w->instrumentindex, ss, 12);  break;
				case 'D': case 'd': inputSamplerHex(&w->fieldpointer, w->instrumentindex, ss, 13);  break;
				case 'E': case 'e': inputSamplerHex(&w->fieldpointer, w->instrumentindex, ss, 14);  break;
				case 'F': case 'f': inputSamplerHex(&w->fieldpointer, w->instrumentindex, ss, 15);  break;
			}
			redraw();
			break;
	}
}

void samplerMouseToIndex(int y, int x, int button, short *index, signed char *fieldpointer)
{
	instrument    *iv;
	sampler_state *ss;
	switch (y)
	{
		case 0: case 1:
			*index = 0;
			*fieldpointer = 0;
			if (button == BUTTON3)
			{
				w->popup = 2;
				w->instrumentindex = 0;
				w->fyoffset = 0; /* this can still be set on edge cases */
			}
			break;
		default:
			if (x < 28)
			{
				switch (y)
				{
					case 2: *index = 1;
						if (x < 16)          *fieldpointer = 0;
						else if (x > 16 + 7) *fieldpointer = 7;
						else *fieldpointer = x - 16;
						break;
					case 3: *index = 2;
						iv = s->instrumentv[s->instrumenti[w->instrument]];
						ss = iv->state[iv->type];
						ss->attributes ^= 0b1; /* invert the first bit */
						break;
					default: *index = 3;
						if (x < 20)          *fieldpointer = 0;
						else if (x > 20 + 3) *fieldpointer = 3;
						else *fieldpointer = x - 20;
						break;
				}
			} else if (x < 58)
			{
				if (x < 43)
				{
					switch (y)
					{
						case 2: case 3: *index = 4;
							if (x < 33)          *fieldpointer = 0;
							else if (x > 33 + 7) *fieldpointer = 7;
							else *fieldpointer = x - 33;
							break;
						default: *index = 5;
							if (x < 33)          *fieldpointer = 0;
							else if (x > 33 + 7) *fieldpointer = 7;
							else *fieldpointer = x - 33;
							break;
					}
				} else
				{
					switch (y)
					{
						case 2: case 3: *index = 6;
							if (x < 46)          *fieldpointer = 0;
							else if (x > 46 + 7) *fieldpointer = 7;
							else *fieldpointer = x - 46;
							break;
						default: *index = 7;
							if (x < 46)          *fieldpointer = 0;
							else if (x > 46 + 7) *fieldpointer = 7;
							else *fieldpointer = x - 46;
							break;
					}
				}
			} else
			{
				*fieldpointer = 0;
				switch (y)
				{
					case 2:  *index = 8;  break;
					case 3:  *index = 9;  break;
					case 4:  *index = 10; break;
					default: *index = 11; break;
				}
			}
			break;
	}
}

uint32_t trimloop(uint32_t pitchedpointer, uint32_t pointer, channel *cv, sampler_state *ss)
{
	if (ss->trim[0] < ss->trim[1])
	{ /* forwards */
		pitchedpointer += ss->trim[0];

		if (ss->loop[0] || ss->loop[1])
		{ /* if there is a loop range */
			if (ss->loop[0] < ss->loop[1])
				while (pitchedpointer >= ss->loop[1])
					pitchedpointer -= ss->loop[1] - ss->loop[0];
			/* TODO: pingpong loop */
		}

		/* trigger the release envelope */
		if (((ss->loop[1] && ss->trim[1] < ss->loop[1]) || !ss->loop[1])
				&& pitchedpointer > ss->trim[1] - (ss->volume.r * ENVELOPE_RELEASE * samplerate)
				&& !cv->releasepointer)
			cv->releasepointer = pointer;

		/* cut if the pointer is ever past trim[1] */
		if (pitchedpointer >= ss->trim[1])
			cv->r.note = 0;
	} else
	{ /* backwards */
		pitchedpointer -= ss->trim[1];

		if (ss->loop[0] || ss->loop[1])
		{ /* if there is a loop range */
			if (ss->loop[0] > ss->loop[1])
				while (pitchedpointer <= ss->loop[1])
					pitchedpointer += ss->loop[0] - ss->loop[1];
			/* TODO: pingpong loop */
		}

		/* trigger the release envelope */
		if (((ss->loop[1] && ss->trim[1] > ss->loop[1]) || !ss->loop[1])
				&& pitchedpointer < ss->trim[1] + (ss->volume.r * ENVELOPE_RELEASE * samplerate))
			cv->releasepointer = pointer;

		/* cut if the pointer is ever past trim[1] */
		if (pitchedpointer <= ss->trim[1])
			cv->r.note = 0;
	}
	return pitchedpointer;
}
/* must be realtime safe          */
/* must accept arbitrary pointers */
void samplerProcess(instrument *iv, channel *cv, uint32_t pointer, float *l, float *r)
{
	if (cv->r.note == 254) /* skip note cut */
	{
		cv->r.note = 0;
		*l = 0.0;
		*r = 0.0;
		return;
	}

	float gain = 1.0;
	uint32_t pitchedpointer;

	sampler_state *ss = iv->state[iv->type];

	if (ss->length > 0)
	{
		/* 61 is C-5 */
		if (ss->attributes & 0b1) /* tempo match */
		{
			if (pointer % MAX(ss->cyclelength, cv->stretchrampmax) == 0) /* first sample of a cycle */
			{
				/* don't ramp the first cycle */
				if (pointer == 0) cv->stretchrampindex = cv->stretchrampmax;
				else
				{
					uint32_t ramppointer;
					cv->stretchrampindex = 0;
					for (uint16_t i = 0; i < cv->stretchrampmax; i++)
					{
						ramppointer = cv->cycleoffset + (float)(MAX(ss->cyclelength, cv->stretchrampmax) + i + 1)
								/ (float)samplerate * ss->c5rate
								* powf(M_12_ROOT_2, (short)cv->r.note - 61 + cv->cents);
						ramppointer = trimloop(ramppointer, pointer + i + 1, cv, ss);
						if (ramppointer > ss->length) break;
						cv->stretchrampbuffer[i * 2 + 0] =
							iv->sampledata[ramppointer * ss->channels]
							/ (float)SHRT_MAX * gain;
						if (ss->channels > 1)
							cv->stretchrampbuffer[i * 2 + 1] =
								iv->sampledata[ramppointer * ss->channels + 1]
								/ (float)SHRT_MAX * gain;
						else
							cv->stretchrampbuffer[i * 2 + 1] =
								iv->sampledata[ramppointer * ss->channels]
								/ (float)SHRT_MAX * gain;
					}
				}
				cv->cycleoffset = (float)(pointer + cv->sampleoffset)
					/ (float)samplerate * ss->c5rate;
			}
			pitchedpointer = cv->cycleoffset + (float)(pointer % MAX(ss->cyclelength, cv->stretchrampmax))
				/ (float)samplerate * ss->c5rate
				* powf(M_12_ROOT_2, (short)cv->r.note - 61 + cv->cents);
		} else
			pitchedpointer = (float)(pointer + cv->sampleoffset)
				/ (float)samplerate * ss->c5rate
				* powf(M_12_ROOT_2, (short)cv->r.note - 61 + cv->cents);

		/* trim/loop */
		pitchedpointer = trimloop(pitchedpointer, pointer, cv, ss);

		float *gainp = &gain;
		adsrEnvelope(ss->volume, gainp, pointer, cv->releasepointer);
		if (gainp == NULL)
		{
			cv->r.note = 0;
			*l = 0.0;
			*r = 0.0;
		} else
		{
			adsrEnvelope(ss->volume, gainp, pointer, cv->releasepointer); /* exponential */

			if (pitchedpointer <= ss->length)
			{
				if (pitchedpointer % ss->channels)
					pitchedpointer -= pitchedpointer % ss->channels;

				if (!cv->mute && gain > 0.0)
				{
					*l = iv->sampledata[pitchedpointer * ss->channels] / (float)SHRT_MAX * gain;
					if (ss->channels > 1)
						*r = iv->sampledata[pitchedpointer * ss->channels + 1] / (float)SHRT_MAX * gain;
					else
						*r = iv->sampledata[pitchedpointer * ss->channels] / (float)SHRT_MAX * gain;
				}
			} else
			{
				*l = 0.0;
				*r = 0.0;
			}
		}
	} else
	{
		cv->r.note = 0;
		*l = 0.0;
		*r = 0.0;
	}

	/* mix in ramp data */
	if (cv->stretchrampindex < cv->stretchrampmax)
	{
		float rampgain = (float)cv->stretchrampindex / (float)cv->stretchrampmax;
		*l *= rampgain; /* fade in new data */
		*r *= rampgain;
		*l += cv->stretchrampbuffer[cv->stretchrampindex * 2 + 0] * (1.0 - rampgain); /* fade out old data */
		*r += cv->stretchrampbuffer[cv->stretchrampindex * 2 + 1] * (1.0 - rampgain);

		cv->stretchrampindex++;
	}
}

/* must be realtime safe */
uint32_t samplerOffset(instrument *iv, channel *cv, int m)
{
	sampler_state *ss = iv->state[iv->type];
	return ss->trim[0] + m / 255.0 * (ss->trim[1] - ss->trim[0]);
}


uint8_t samplerGetOffset(instrument *iv, channel *cv)
{
	if (!cv->r.note) return 0;
	sampler_state *ss = iv->state[iv->type];
	return (cv->samplepointer - ss->trim[0]) / (float)(ss->trim[1] - ss->trim[0]) * 255.0;
}

/* called when state's type is changed to this file's */
void samplerChangeType(void **state)
{
	*state = calloc(1, sizeof(sampler_state));
	sampler_state *ss = *state;
	ss->volume.s = 255;
	ss->cyclelength = 0x6ff; /* feels like a good default? hard to be sure */
}

void samplerWrite(instrument *iv, uint8_t index, FILE *fp)
{
	sampler_state *ss = iv->state[index];
	fwrite(&ss->length, sizeof(uint32_t), 1, fp);
	fputc(ss->channels, fp);
	fwrite(&ss->attributes, sizeof(uint8_t), 1, fp);
	fwrite(&ss->c5rate, sizeof(uint32_t), 1, fp);
	fwrite(&ss->cyclelength, sizeof(uint16_t), 1, fp);
	fwrite(ss->trim, sizeof(uint32_t), 2, fp);
	fwrite(ss->loop, sizeof(uint32_t), 2, fp);
	fputc(ss->volume.a, fp);
	fputc(ss->volume.d, fp);
	fputc(ss->volume.s, fp);
	fputc(ss->volume.r, fp);
}

void samplerRead(instrument *iv, uint8_t index, FILE *fp)
{
	sampler_state *ss = iv->state[index];
	fread(&ss->length, sizeof(uint32_t), 1, fp);
	ss->channels = fgetc(fp);
	fread(&ss->attributes, sizeof(uint8_t), 1, fp);
	fread(&ss->c5rate, sizeof(uint32_t), 1, fp);
	fread(&ss->cyclelength, sizeof(uint16_t), 1, fp);
	fread(ss->trim, sizeof(uint32_t), 2, fp);
	fread(ss->loop, sizeof(uint32_t), 2, fp);
	ss->volume.a = fgetc(fp);
	ss->volume.d = fgetc(fp);
	ss->volume.s = fgetc(fp);
	ss->volume.r = fgetc(fp);
}

void samplerInit(int index)
{
	t->f[index].indexc = 11;
	t->f[index].statesize = sizeof(sampler_state);
	t->f[index].draw = &drawSampler;
	t->f[index].adjustUp = &samplerAdjustUp;
	t->f[index].adjustDown = &samplerAdjustDown;
	t->f[index].adjustLeft = &samplerAdjustLeft;
	t->f[index].adjustRight = &samplerAdjustRight;
	t->f[index].incFieldPointer = &samplerIncFieldPointer;
	t->f[index].decFieldPointer = &samplerDecFieldPointer;
	t->f[index].endFieldPointer = &samplerEndFieldPointer;
	t->f[index].mouseToIndex = &samplerMouseToIndex;
	t->f[index].input = &samplerInput;
	t->f[index].process = &samplerProcess;
	t->f[index].offset = &samplerOffset;
	t->f[index].getOffset = &samplerGetOffset;
	t->f[index].changeType = &samplerChangeType;
	t->f[index].write = &samplerWrite;
	t->f[index].read = &samplerRead;
}
