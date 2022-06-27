#define S_FLAG_MONO    0b00001000
#define S_FLAG_PHASE   0b00100000
#define S_FLAG_8BIT    0b00010000
#define S_FLAG_TTEMPO  0b00000001
#define S_FLAG_RPLAY   0b01000000
#define S_FLAG_LRAMP   0b00000100
#define S_FLAG_QCYCLES 0b00000010
typedef struct
{
	short   *sampledata;   /* variable size, persists between types */
	uint32_t samplelength; /* raw samples allocated for sampledata */

	uint32_t length;
	uint8_t  channels;
	uint32_t c5rate;
	uint8_t  samplerate;   /* percent of c5rate to actually use */
	uint16_t cyclelength;
	uint8_t  formantshift;
	uint16_t formantcyclelength;
	uint32_t trim[2];
	uint32_t loop[2];
	adsr     volume;
	uint8_t  gain;
	uint8_t  flags;        /* %1: timestretch tempo
	                        * %2: quantize cycles
	                        * %3: loop ramping
	                        * %4: mono
	                        * %5: 8-bit
	                        * %6: invert phase
	                        * %7: reverse playback
	                        */
} sampler_state;

typedef struct
{
	uint16_t  stretchrampindex;    /* progress through the stretch ramp buffer, >localstretchrampmax if not ramping */
	uint16_t  localstretchrampmax; /* actual stretchrampmax used, to allow for tiny buffer sizes */
	sample_t *stretchrampbuffer;   /* raw samples to ramp out */

	uint16_t  formantrampindex;    /* progress through the stretch ramp buffer, >localstretchrampmax if not ramping */
	uint16_t  localformantrampmax; /* actual stretchrampmax used, to allow for tiny buffer sizes */
	sample_t *formantrampbuffer;   /* raw samples to ramp out */
} sampler_channel;


void samplerIncFieldPointer(short index)
{
	switch (index)
	{
		case 1: case 7: case 8: case 9: case 10:
			w->fieldpointer++;
			if (w->fieldpointer > 7) w->fieldpointer = 0;
			break;
		case 19: case 21:
			w->fieldpointer++;
			if (w->fieldpointer > 3) w->fieldpointer = 0;
			break;
		default:
			w->fieldpointer = 0;
			break;
	}
}
void samplerDecFieldPointer(short index)
{
	switch (index)
	{
		case 1: case 7: case 8: case 9: case 10:
			w->fieldpointer--;
			if (w->fieldpointer < 0) w->fieldpointer = 7;
			break;
		case 19: case 21:
			w->fieldpointer--;
			if (w->fieldpointer < 0) w->fieldpointer = 3;
			break;
		case 3:  case 13: case 14: case 15:
		case 16: case 17: case 20:
			w->fieldpointer = 1;
			break;
		default:
			w->fieldpointer = 0;
			break;
	}
}
void samplerEndFieldPointer(short index)
{
	switch (index)
	{
		case 1: case 7: case 8: case 9: case 10:
			w->fieldpointer = 7;
			break;
		case 19: case 21:
			w->fieldpointer = 3;
			break;
		default:
			w->fieldpointer = 0;
			break;
	}
}

void inputSamplerHex(short index, sampler_state *ss, char value)
{
	switch (index)
	{
		case 1:  updateField(w->fieldpointer, 8, (uint32_t *)&ss->c5rate, value); break;
		case 3:  updateFieldPush(&ss->samplerate, value); break;
		case 7:  updateField(w->fieldpointer, 8, (uint32_t *)&ss->trim[0], value);  ss->trim[0] = MIN(ss->trim[1], MIN(ss->trim[0], ss->length)); break;
		case 8:  updateField(w->fieldpointer, 8, (uint32_t *)&ss->trim[1], value);  ss->trim[1] = MAX(ss->trim[0], MIN(ss->trim[1], ss->length)); break;
		case 9:  updateField(w->fieldpointer, 8, (uint32_t *)&ss->loop[0], value);  ss->loop[0] = MIN(ss->loop[1], MIN(ss->loop[0], ss->length)); break;
		case 10: updateField(w->fieldpointer, 8, (uint32_t *)&ss->loop[1], value);  ss->loop[1] = MAX(ss->loop[0], MIN(ss->loop[1], ss->length)); break;
		case 13: updateFieldPush(&ss->volume.a, value); break;
		case 14: updateFieldPush(&ss->volume.d, value); break;
		case 15: updateFieldPush(&ss->volume.s, value); break;
		case 16: updateFieldPush(&ss->volume.r, value); break;
		case 17: updateFieldPush(&ss->gain, value); break;
		case 19: updateField(w->fieldpointer, 4, (uint32_t *)&ss->cyclelength, value); break;
		case 20: updateFieldPush(&ss->formantshift, value); break;
		case 21: updateField(w->fieldpointer, 4, (uint32_t *)&ss->formantcyclelength, value); break;
	}
	samplerIncFieldPointer(index);
}

void drawSampler(instrument *iv, uint8_t index, unsigned short x, unsigned short y, short *cursor, char adjust)
{
	sampler_state *ss = iv->state[iv->type];
	unsigned char sampletitleoffset;
	if (!ss->samplelength)
	{
		sampletitleoffset = 3;
		printf("\033[%d;%dH [sampler (no sample)] ", y - 1, x+12);
	}
	else switch (ss->channels)
	{
		case 1:
			sampletitleoffset = 1;
			printf("\033[%d;%dH [sampler (%08x mono)] ", y - 1, x+10, ss->length);
			break;
		default:
			sampletitleoffset = 0;
			printf("\033[%d;%dH [sampler (%08x stereo)] ", y - 1, x+9, ss->length);
			break;
	}

	for (char i = 1; i < 13; i++)
	printf("\033[%d;%dH│", y+i, x+22);
	printf("\033[%d;%dH\033[1mGENERAL\033[m",    y+1,  x+8);
	printf("\033[%d;%dHC-5 rate   [%08x]",       y+2,  x+0, ss->c5rate);
	printf("\033[%d;%dHmono/rate     ",          y+3,  x+0);
	drawBit(ss->flags & S_FLAG_MONO); printf("[%02x]", ss->samplerate);
	printf("\033[%d;%dHphase/8-bit    ",         y+4,  x+0);
	drawBit(ss->flags & S_FLAG_PHASE); drawBit(ss->flags & S_FLAG_8BIT);
	printf("\033[%d;%dHtimestretch tempo ",      y+5,  x+0);
	drawBit(ss->flags & S_FLAG_TTEMPO);
	printf("\033[%d;%dH──────────────────────┤", y+6,  x+0);
	printf("\033[%d;%dH\033[1mAMPLIFIER\033[m",  y+7,  x+6);
	printf("\033[%d;%dHattack           [%02x]", y+8,  x+0, ss->volume.a);
	printf("\033[%d;%dHdecay            [%02x]", y+9,  x+0, ss->volume.d);
	printf("\033[%d;%dHsustain          [%02x]", y+10, x+0, ss->volume.s);
	printf("\033[%d;%dHrelease          [%02x]", y+11, x+0, ss->volume.r);
	printf("\033[%d;%dHgain             [%02x]", y+12, x+0, ss->gain);
	printf("\033[%d;%dH\033[1mTRIM         LOOP\033[m", y+1,  x+27);
	printf("\033[%d;%dH[%08x]   [%08x]",                y+2,  x+24, ss->trim[0], ss->loop[0]);
	printf("\033[%d;%dH[%08x]   [%08x]",                y+3,  x+24, ss->trim[1], ss->loop[1]);
	printf("\033[%d;%dH%02x->%02x       %02x->%02x",    y+4,  x+26,
			(uint8_t)((float)ss->trim[0] / (float)ss->length * 255),
			(uint8_t)((float)ss->trim[1] / (float)ss->length * 255),
			(uint8_t)((float)ss->loop[0] / (float)ss->length * 255),
			(uint8_t)((float)ss->loop[1] / (float)ss->length * 255));
	printf("\033[%d;%dHreverse playback    ",             y+5,  x+24);
	drawBit(ss->flags & S_FLAG_RPLAY);
	printf("\033[%d;%dHloop ramping        ",             y+6,  x+24);
	drawBit(ss->flags & S_FLAG_LRAMP);
	printf("\033[%d;%dH├────────────────────────",   y+7,  x+22);
	printf("\033[%d;%dH\033[1mTIMESTRETCH\033[m",    y+8,  x+30);
	printf("\033[%d;%dHquantize cycles     ",        y+9,  x+24);
	drawBit(ss->flags & S_FLAG_QCYCLES);
	printf("\033[%d;%dHcycle length     [%04x]",     y+10, x+24, ss->cyclelength);
	printf("\033[%d;%dHformant shift      [%02x]",   y+11, x+24, ss->formantshift);
	printf("\033[%d;%dH \" cycle length  [%04x]",    y+12, x+24, ss->formantcyclelength);

	if (w->instrumentrecv == INST_REC_LOCK_CONT)
		printf("\033[%d;%dHREC", y, x+0);

	switch (*cursor)
	{
		case 0:  printf("\033[%d;%dH", y-1,  x+11 + sampletitleoffset); break;
		case 1:  printf("\033[%d;%dH", y+2,  x+12 + w->fieldpointer); break;
		case 2:  printf("\033[%d;%dH", y+3,  x+15); break;
		case 3:  printf("\033[%d;%dH", y+3,  x+19 - w->fieldpointer); break;
		case 4:  printf("\033[%d;%dH", y+4,  x+16); break;
		case 5:  printf("\033[%d;%dH", y+4,  x+20); break;
		case 6:  printf("\033[%d;%dH", y+5,  x+20); break;
		case 7:  printf("\033[%d;%dH", y+2,  x+25 + w->fieldpointer); break;
		case 8:  printf("\033[%d;%dH", y+3,  x+25 + w->fieldpointer); break;
		case 9:  printf("\033[%d;%dH", y+2,  x+38 + w->fieldpointer); break;
		case 10: printf("\033[%d;%dH", y+3,  x+38 + w->fieldpointer); break;
		case 11: printf("\033[%d;%dH", y+5,  x+45); break;
		case 12: printf("\033[%d;%dH", y+6,  x+45); break;
		case 13: printf("\033[%d;%dH", y+8,  x+19 - w->fieldpointer); break;
		case 14: printf("\033[%d;%dH", y+9,  x+19 - w->fieldpointer); break;
		case 15: printf("\033[%d;%dH", y+10, x+19 - w->fieldpointer); break;
		case 16: printf("\033[%d;%dH", y+11, x+19 - w->fieldpointer); break;
		case 17: printf("\033[%d;%dH", y+12, x+19 - w->fieldpointer); break;
		case 18: printf("\033[%d;%dH", y+9,  x+45); break;
		case 19: printf("\033[%d;%dH", y+10, x+42 + w->fieldpointer); break;
		case 20: printf("\033[%d;%dH", y+11, x+45 - w->fieldpointer); break;
		case 21: printf("\033[%d;%dH", y+12, x+42 + w->fieldpointer); break;
	}
}

void samplerAdjustUp(instrument *iv, short index, char mouse)
{
	sampler_state *ss = iv->state[iv->type];
	switch (index)
	{
		case 1:  ss->c5rate = ss->c5rate * powf(M_12_ROOT_2, 1); break;
		case 3:  if (w->fieldpointer) { if (ss->samplerate < 255 - 16) ss->samplerate+=16; else ss->samplerate = 255; } else if (ss->samplerate < 255) ss->samplerate++; break;
		case 7:  ss->trim[0] += MAX(ss->length / 50.0, 1);  ss->trim[0] = MIN(ss->trim[1], MIN(ss->trim[0], ss->length)); break;
		case 8:  ss->trim[1] += MAX(ss->length / 50.0, 1);  ss->trim[1] = MAX(ss->trim[0], MIN(ss->trim[1], ss->length)); break;
		case 9:  ss->loop[0] += MAX(ss->length / 50.0, 1);  ss->loop[0] = MIN(ss->loop[1], MIN(ss->loop[0], ss->length)); break;
		case 10: ss->loop[1] += MAX(ss->length / 50.0, 1);  ss->loop[1] = MAX(ss->loop[0], MIN(ss->loop[1], ss->length)); break;
		case 13: if (w->fieldpointer) { if (ss->volume.a < 255 - 16) ss->volume.a+=16; else ss->volume.a = 255; } else if (ss->volume.a < 255) ss->volume.a++; break;
		case 14: if (w->fieldpointer) { if (ss->volume.d < 255 - 16) ss->volume.d+=16; else ss->volume.d = 255; } else if (ss->volume.d < 255) ss->volume.d++; break;
		case 15: if (w->fieldpointer) { if (ss->volume.s < 255 - 16) ss->volume.s+=16; else ss->volume.s = 255; } else if (ss->volume.s < 255) ss->volume.s++; break;
		case 16: if (w->fieldpointer) { if (ss->volume.r < 255 - 16) ss->volume.r+=16; else ss->volume.r = 255; } else if (ss->volume.r < 255) ss->volume.r++; break;
		case 17: if (w->fieldpointer) { if (ss->gain < 255 - 16) ss->gain+=16; else ss->gain = 255; } else if (ss->gain < 255) ss->gain++; break;
		case 19: ss->cyclelength += 0x20; break;
		case 20: if (w->fieldpointer) { if (ss->formantshift < 255 - 16) ss->formantshift+=16; else ss->formantshift = 255; } else if (ss->formantshift < 255) ss->formantshift++; break;
		case 21: ss->formantcyclelength += 0x20; break;
	}
}
void samplerAdjustDown(instrument *iv, short index, char mouse)
{
	sampler_state *ss = iv->state[iv->type];
	uint32_t oldpos;
	switch (index)
	{
		case 1:  ss->c5rate = ss->c5rate * powf(M_12_ROOT_2, -1); break;
		case 3:  if (w->fieldpointer) { if (ss->samplerate > 16) ss->samplerate-=16; else ss->samplerate = 0; } else if (ss->samplerate) ss->samplerate--; break;
		case 7:  oldpos = ss->trim[0]; ss->trim[0] -= MAX(ss->length / 50.0, 1); if (ss->trim[0] > oldpos) ss->trim[0] = 0;  ss->trim[0] = MIN(ss->trim[1], MIN(ss->trim[0], ss->length)); break;
		case 8:  oldpos = ss->trim[1]; ss->trim[1] -= MAX(ss->length / 50.0, 1); if (ss->trim[1] > oldpos) ss->trim[1] = 0;  ss->trim[1] = MAX(ss->trim[0], MIN(ss->trim[1], ss->length)); break;
		case 9:  oldpos = ss->loop[0]; ss->loop[0] -= MAX(ss->length / 50.0, 1); if (ss->loop[0] > oldpos) ss->loop[0] = 0;  ss->loop[0] = MIN(ss->loop[1], MIN(ss->loop[0], ss->length)); break;
		case 10: oldpos = ss->loop[1]; ss->loop[1] -= MAX(ss->length / 50.0, 1); if (ss->loop[1] > oldpos) ss->loop[1] = 0;  ss->loop[1] = MAX(ss->loop[0], MIN(ss->loop[1], ss->length)); break;
		case 13: if (w->fieldpointer) { if (ss->volume.a > 16) ss->volume.a-=16; else ss->volume.a = 0; } else if (ss->volume.a) ss->volume.a--; break;
		case 14: if (w->fieldpointer) { if (ss->volume.d > 16) ss->volume.d-=16; else ss->volume.d = 0; } else if (ss->volume.d) ss->volume.d--; break;
		case 15: if (w->fieldpointer) { if (ss->volume.s > 16) ss->volume.s-=16; else ss->volume.s = 0; } else if (ss->volume.s) ss->volume.s--; break;
		case 16: if (w->fieldpointer) { if (ss->volume.r > 16) ss->volume.r-=16; else ss->volume.r = 0; } else if (ss->volume.r) ss->volume.r--; break;
		case 17: if (w->fieldpointer) { if (ss->gain > 16) ss->gain-=16; else ss->gain = 0; } else if (ss->gain) ss->gain--; break;
		case 19: ss->cyclelength -= 0x20; break;
		case 20: if (w->fieldpointer) { if (ss->formantshift > 16) ss->formantshift-=16; else ss->formantshift = 0; } else if (ss->formantshift) ss->formantshift--; break;
		case 21: ss->formantcyclelength -= 0x20; break;
	}
}
void samplerAdjustLeft(instrument *iv, short index, char mouse)
{
	sampler_state *ss = iv->state[iv->type];
	uint32_t oldpos;
	switch (index)
	{
		case 1: ss->c5rate = ss->c5rate * powf(M_12_ROOT_2, -0.2); break;
		case 3: case 14: case 15: case 16: case 17: case 18: case 21: case 22:
			if (!mouse) w->fieldpointer = 1;
			break;
		case 7:  oldpos = ss->trim[0]; ss->trim[0] -= MAX(ss->length / 1000.0, 1); if (ss->trim[0] > oldpos) ss->trim[0] = 0;  ss->trim[0] = MIN(ss->trim[1], MIN(ss->trim[0], ss->length)); break;
		case 8:  oldpos = ss->trim[1]; ss->trim[1] -= MAX(ss->length / 1000.0, 1); if (ss->trim[1] > oldpos) ss->trim[1] = 0;  ss->trim[1] = MAX(ss->trim[0], MIN(ss->trim[1], ss->length)); break;
		case 9:  oldpos = ss->loop[0]; ss->loop[0] -= MAX(ss->length / 1000.0, 1); if (ss->loop[0] > oldpos) ss->loop[0] = 0;  ss->loop[0] = MIN(ss->loop[1], MIN(ss->loop[0], ss->length)); break;
		case 10: oldpos = ss->loop[1]; ss->loop[1] -= MAX(ss->length / 1000.0, 1); if (ss->loop[1] > oldpos) ss->loop[1] = 0;  ss->loop[1] = MAX(ss->loop[0], MIN(ss->loop[1], ss->length)); break;
	}
}
void samplerAdjustRight(instrument *iv, short index, char mouse)
{
	sampler_state *ss = iv->state[iv->type];
	switch (index)
	{
		case 1: ss->c5rate = ss->c5rate * powf(M_12_ROOT_2, 0.2); break;
		case 3: case 14: case 15: case 16: case 17: case 18: case 21: case 22:
			if (!mouse) w->fieldpointer = 0;
			break;
		case 7:  ss->trim[0] += MAX(ss->length / 1000.0, 1);  ss->trim[0] = MIN(ss->trim[1], MIN(ss->trim[0], ss->length)); break;
		case 8:  ss->trim[1] += MAX(ss->length / 1000.0, 1);  ss->trim[1] = MAX(ss->trim[0], MIN(ss->trim[1], ss->length)); break;
		case 9:  ss->loop[0] += MAX(ss->length / 1000.0, 1);  ss->loop[0] = MIN(ss->loop[1], MIN(ss->loop[0], ss->length)); break;
		case 10: ss->loop[1] += MAX(ss->length / 1000.0, 1);  ss->loop[1] = MAX(ss->loop[0], MIN(ss->loop[1], ss->length)); break;
	}
}


void loadSample(uint8_t index, char *path)
{
	instrument *iv = s->instrumentv[s->instrumenti[index]];
	sampler_state *ss = iv->state[iv->type];
	SF_INFO sfinfo;
	short *sampledata = _loadSample(path, &sfinfo);
	if (!sampledata)
	{
		strcpy(w->command.error, "failed to load sample, out of memory");
		return;
	}

	/* unload any present sample data */
	if (ss->samplelength > 0)
		free(ss->sampledata);
	ss->sampledata = sampledata;
	ss->samplelength = sfinfo.frames * sfinfo.channels;
	ss->channels = sfinfo.channels;
	ss->length = sfinfo.frames;
	ss->c5rate = sfinfo.samplerate;
	ss->trim[0] = 0;
	ss->trim[1] = sfinfo.frames;
	ss->loop[0] = 0;
	ss->loop[1] = 0;
};
int exportSample(uint8_t index, char *path)
{
	instrument *iv = s->instrumentv[s->instrumenti[index]];
	sampler_state *ss = iv->state[iv->type];
	if (!ss->sampledata) return 1; /* no sample data */

	SNDFILE *sndfile;
	SF_INFO sfinfo;
	memset(&sfinfo, 0, sizeof(sfinfo));

	sfinfo.samplerate = ss->c5rate;
	sfinfo.frames = ss->length;
	sfinfo.channels = ss->channels;

	sfinfo.format = SF_FORMAT_WAV | SF_FORMAT_PCM_16;
	sndfile = sf_open(path, SFM_WRITE, &sfinfo);
	if (sndfile == NULL) return 1;

	/* write the sample data to disk */
	sf_writef_short(sndfile, ss->sampledata, ss->length);
	sf_close(sndfile);
	return 0;
};


void samplerApplyTrimming(instrument *iv)
{
	pushInstrumentHistoryIfNew(iv);
	sampler_state *ss = iv->state[iv->type];
	if (ss->samplelength > 0)
	{
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
				ss->sampledata+(sizeof(short) * startOffset),
				sizeof(short) * newlen * ss->channels);

		free(ss->sampledata); ss->sampledata = NULL;
		ss->sampledata = sampledata;
		ss->samplelength = newlen * ss->channels;
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

void samplerLoadCallback(char *path)
{
	loadSample(w->instrument, path);
	pushInstrumentHistory(s->instrumentv[s->instrumenti[w->instrument]]);
	w->popup = 1;
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
					w->filebrowserCallback = &samplerLoadCallback;
					redraw();
					*input = 0; /* don't reprocess */
					break;
				// case 's': /* spleeter */
					if (ss->samplelength > 0)
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
				// case 't': /* apply trimming */
					samplerApplyTrimming(iv);
					redraw();
					break;
				case 'e': /* export */
					setCommand(&w->command, &samplerExportCallback, NULL, NULL, 0, "File name: ", "");
					w->mode = 255;
					redraw();
					break;
			}
			break;
		default:
			switch (*input)
			{
				case 10: case 13: /* return */
					switch (w->instrumentindex)
					{
						case 2:  /* mono              */ ss->flags ^= S_FLAG_MONO; redraw(); *input = 0; break;
						case 4:  /* invert phase      */ ss->flags ^= S_FLAG_PHASE; redraw(); *input = 0; break;
						case 5:  /* 8-bit             */ ss->flags ^= S_FLAG_8BIT; redraw(); *input = 0; break;
						case 6:  /* timestretch tempo */ ss->flags ^= S_FLAG_TTEMPO; redraw(); *input = 0; break;
						case 11: /* reverse playback  */ ss->flags ^= S_FLAG_RPLAY; redraw(); *input = 0; break;
						case 12: /* loop ramping      */ ss->flags ^= S_FLAG_LRAMP; redraw(); *input = 0; break;
						case 18: /* quantize cycles   */ ss->flags ^= S_FLAG_QCYCLES; redraw(); *input = 0; break;
					}
					break;
				case 1: /* ^a */
					switch (w->instrumentindex)
					{
						case 1:  ss->c5rate++; break;
						case 3:  ss->samplerate++; break;
						case 7:  if (ss->trim[0] == ss->length) ss->trim[0] = 0; else ss->trim[0]++;  ss->trim[0] = MIN(ss->trim[1], MIN(ss->trim[0], ss->length)); break;
						case 8:  if (ss->trim[1] == ss->length) ss->trim[1] = 0; else ss->trim[1]++;  ss->trim[1] = MAX(ss->trim[0], MIN(ss->trim[1], ss->length)); break;
						case 9:  if (ss->loop[0] == ss->length) ss->loop[0] = 0; else ss->loop[0]++;  ss->loop[0] = MIN(ss->loop[1], MIN(ss->loop[0], ss->length)); break;
						case 10: if (ss->loop[1] == ss->length) ss->loop[1] = 0; else ss->loop[1]++;  ss->loop[1] = MAX(ss->loop[0], MIN(ss->loop[1], ss->length)); break;
						case 13: ss->volume.a++; break;
						case 14: ss->volume.d++; break;
						case 15: ss->volume.s++; break;
						case 16: ss->volume.r++; break;
						case 17: ss->gain++; break;
						case 19: ss->cyclelength++; break;
						case 20: ss->formantshift++; break;
						case 21: ss->formantcyclelength++; break;
					}
					break;
				case 24: /* ^x */
					switch (w->instrumentindex)
					{
						case 1:  ss->c5rate--; break;
						case 3:  ss->samplerate--; break;
						case 7:  ss->trim[0]--;  ss->trim[0] = MIN(ss->trim[1], MIN(ss->trim[0], ss->length)); break;
						case 8:  ss->trim[1]--;  ss->trim[1] = MAX(ss->trim[0], MIN(ss->trim[1], ss->length)); break;
						case 9:  ss->loop[0]--;  ss->loop[0] = MIN(ss->loop[1], MIN(ss->loop[0], ss->length)); break;
						case 10: ss->loop[1]--;  ss->loop[1] = MAX(ss->loop[0], MIN(ss->loop[1], ss->length)); break;
						case 13: ss->volume.a--; break;
						case 14: ss->volume.d--; break;
						case 15: ss->volume.s--; break;
						case 16: ss->volume.r--; break;
						case 17: ss->gain--; break;
						case 19: ss->cyclelength--; break;
						case 20: ss->formantshift--; break;
						case 21: ss->formantcyclelength--; break;
					}
					break;
				case '0':           inputSamplerHex(w->instrumentindex, ss, 0);   break;
				case '1':           inputSamplerHex(w->instrumentindex, ss, 1);   break;
				case '2':           inputSamplerHex(w->instrumentindex, ss, 2);   break;
				case '3':           inputSamplerHex(w->instrumentindex, ss, 3);   break;
				case '4':           inputSamplerHex(w->instrumentindex, ss, 4);   break;
				case '5':           inputSamplerHex(w->instrumentindex, ss, 5);   break;
				case '6':           inputSamplerHex(w->instrumentindex, ss, 6);   break;
				case '7':           inputSamplerHex(w->instrumentindex, ss, 7);   break;
				case '8':           inputSamplerHex(w->instrumentindex, ss, 8);   break;
				case '9':           inputSamplerHex(w->instrumentindex, ss, 9);   break;
				case 'A': case 'a': inputSamplerHex(w->instrumentindex, ss, 10);  break;
				case 'B': case 'b': inputSamplerHex(w->instrumentindex, ss, 11);  break;
				case 'C': case 'c': inputSamplerHex(w->instrumentindex, ss, 12);  break;
				case 'D': case 'd': inputSamplerHex(w->instrumentindex, ss, 13);  break;
				case 'E': case 'e': inputSamplerHex(w->instrumentindex, ss, 14);  break;
				case 'F': case 'f': inputSamplerHex(w->instrumentindex, ss, 15);  break;
			}
			redraw();
			break;
	}
}

void samplerMouseToIndex(int y, int x, int button, short *index)
{
	instrument *iv = s->instrumentv[s->instrumenti[w->instrument]];
	sampler_state *ss = iv->state[iv->type];
	switch (y)
	{
		case 0: case 1:
			*index = 0;
			w->fieldpointer = 0;
			if (button == BUTTON3 || button == BUTTON3_CTRL)
			{
				previewNote(0, 255, w->channel, 1);
				w->popup = 2;
				w->instrumentindex = 0;
				w->fyoffset = 0; /* this can still be set on edge cases */
				w->filebrowserCallback = &samplerLoadCallback;
			} break;
		default:
			if (x < 22)
				switch (y)
				{
					case 2: case 3: *index = 1;
						if (x < 12)          w->fieldpointer = 0;
						else if (x > 12 + 7) w->fieldpointer = 7;
						else w->fieldpointer = x - 12;
						break;
					case 4:
						if (x < 17) { *index = 2; ss->flags ^= S_FLAG_MONO; w->fieldpointer = 0; }
						else        { *index = 3; if (x < 19) w->fieldpointer = 1; else w->fieldpointer = 0; }
						break;
					case 5:
						if (x < 18) { *index = 4; ss->flags ^= S_FLAG_PHASE; }
						else        { *index = 5; ss->flags ^= S_FLAG_8BIT; }
						w->fieldpointer = 0; break;
					case 6: case 7: *index = 6; w->fieldpointer = 0; ss->flags ^= S_FLAG_TTEMPO; break;
					case 8: case 9: *index = 13; if (x < 19) w->fieldpointer = 1; else w->fieldpointer = 0; break;
					case 10: *index = 14; if (x < 19) w->fieldpointer = 1; else w->fieldpointer = 0; break;
					case 11: *index = 15; if (x < 19) w->fieldpointer = 1; else w->fieldpointer = 0; break;
					case 12: *index = 16; if (x < 19) w->fieldpointer = 1; else w->fieldpointer = 0; break;
					default: *index = 17; if (x < 19) w->fieldpointer = 1; else w->fieldpointer = 0; break;
				}
			else
				switch (y)
				{
					case 2: case 3:
						if (x < 16)
						{
							*index = 7;
							if (x < 25)          w->fieldpointer = 0;
							else if (x > 25 + 7) w->fieldpointer = 7;
							else w->fieldpointer = x - 25;
						} else
						{
							*index = 9;
							if (x < 38)          w->fieldpointer = 0;
							else if (x > 38 + 7) w->fieldpointer = 7;
							else w->fieldpointer = x - 38;
						} break;
					case 4: case 5:
						if (x < 36)
						{
							*index = 8;
							if (x < 25)          w->fieldpointer = 0;
							else if (x > 25 + 7) w->fieldpointer = 7;
							else w->fieldpointer = x - 25;
						} else
						{
							*index = 10;
							if (x < 38)          w->fieldpointer = 0;
							else if (x > 38 + 7) w->fieldpointer = 7;
							else w->fieldpointer = x - 38;
						} break;
					case 6: *index = 11; w->fieldpointer = 0; ss->flags ^= S_FLAG_RPLAY; break;
					case 7: case 8: *index = 12; w->fieldpointer = 0; ss->flags ^= S_FLAG_LRAMP; break;
					case 9: case 10: *index = 18; w->fieldpointer = 0; ss->flags ^= S_FLAG_QCYCLES; break;
					case 11: *index = 19;
						if (x < 42)          w->fieldpointer = 0;
						else if (x > 42 + 3) w->fieldpointer = 3;
						else w->fieldpointer = x - 42;
						break;
					case 12: *index = 20; if (x < 45) w->fieldpointer = 1; else w->fieldpointer = 0; break;
					default: *index = 21;
						if (x < 42)          w->fieldpointer = 0;
						else if (x > 42 + 3) w->fieldpointer = 3;
						else w->fieldpointer = x - 42;
						break;
				} break;
	}
}

/* clamps within range and loop, returns output samples */
uint32_t trimloop(uint32_t pitchedpointer, uint32_t pointer,
		channel *cv, instrument *iv, sampler_state *ss,
		sample_t *l, sample_t *r)
{
	pitchedpointer += ss->trim[0] + cv->pointeroffset;

	if (ss->loop[1])
	{ /* if there is a loop range */
		if (ss->flags & S_FLAG_LRAMP)
		{
			uint32_t looprampmax = MIN(samplerate / 1000 * LOOP_RAMP_MS, (ss->loop[1] - ss->loop[0]) * 0.5);
			uint32_t loopoffset = ss->loop[1] - ss->loop[0] - looprampmax;
			while (pitchedpointer >= ss->loop[1])
				pitchedpointer -= loopoffset;

			if (ss->flags & S_FLAG_RPLAY) /* always point to the left channel */
				pitchedpointer -= (ss->trim[1]*2 - pitchedpointer) % ss->channels;
			else
				pitchedpointer -= pitchedpointer % ss->channels;

			if (pitchedpointer <= ss->length)
			{
				if (pitchedpointer > ss->loop[1] - looprampmax)
				{
					float lerp = (pitchedpointer - ss->loop[1] + looprampmax) / (float)looprampmax;
					uint32_t ramppointer = (pitchedpointer - loopoffset);
					if (ss->flags & S_FLAG_RPLAY) /* always point to the left channel */
						ramppointer -= (ss->trim[1]*2 - ramppointer) % ss->channels;
					else
						ramppointer -= ramppointer % ss->channels;

					if (ss->flags & S_FLAG_8BIT)
					{
						if (ss->flags & S_FLAG_RPLAY)
						{
							*l = (signed char)(ss->sampledata[ss->trim[1]*2 - pitchedpointer * ss->channels + 0]>>8) / (float)SCHAR_MAX * (1.0 - lerp)
								+ (signed char)(ss->sampledata[ss->trim[1]*2 - ramppointer * ss->channels + 0]>>8) / (float)SCHAR_MAX * lerp;

							if (ss->channels > 1)
								*r = (signed char)(ss->sampledata[ss->trim[1]*2 - pitchedpointer * ss->channels + 1]>>8) / (float)SCHAR_MAX * (1.0 - lerp)
									+ (signed char)(ss->sampledata[ss->trim[1]*2 - ramppointer * ss->channels + 1]>>8) / (float)SCHAR_MAX * lerp;
							else *r = *l;
						} else
						{
							*l = (signed char)(ss->sampledata[pitchedpointer * ss->channels + 0]>>8) / (float)SCHAR_MAX * (1.0 - lerp)
								+ (signed char)(ss->sampledata[ramppointer * ss->channels + 0]>>8) / (float)SCHAR_MAX * lerp;

							if (ss->channels > 1)
								*r = (signed char)(ss->sampledata[pitchedpointer * ss->channels + 1]>>8) / (float)SCHAR_MAX * (1.0 - lerp)
									+ (signed char)(ss->sampledata[ramppointer * ss->channels + 1]>>8) / (float)SCHAR_MAX * lerp;
							else *r = *l;
						}
					} else
					{
						if (ss->flags & S_FLAG_RPLAY)
						{
							*l = ss->sampledata[ss->trim[1]*2 - pitchedpointer * ss->channels + 0] / (float)SHRT_MAX * (1.0 - lerp)
								+ ss->sampledata[ss->trim[1]*2 - ramppointer * ss->channels + 0] / (float)SHRT_MAX * lerp;

							if (ss->channels > 1)
								*r = ss->sampledata[ss->trim[1]*2 - pitchedpointer * ss->channels + 1] / (float)SHRT_MAX * (1.0 - lerp)
									+ ss->sampledata[ss->trim[1]*2 - ramppointer * ss->channels + 1] / (float)SHRT_MAX * lerp;
							else *r = *l;
						} else
						{
							*l = ss->sampledata[pitchedpointer * ss->channels + 0] / (float)SHRT_MAX * (1.0 - lerp)
								+ ss->sampledata[ramppointer * ss->channels + 0] / (float)SHRT_MAX * lerp;

							if (ss->channels > 1)
								*r = ss->sampledata[pitchedpointer * ss->channels + 1] / (float)SHRT_MAX * (1.0 - lerp)
									+ ss->sampledata[ramppointer * ss->channels + 1] / (float)SHRT_MAX * lerp;
							else *r = *l;
						}
					}
				} else
				{
					if (ss->flags & S_FLAG_8BIT)
					{
						if (ss->flags & S_FLAG_RPLAY)
						{
							*l = (signed char)(ss->sampledata[ss->trim[1]*2 - pitchedpointer * ss->channels + 0]>>8) / (float)SCHAR_MAX;
							if (ss->channels > 1) *r = (signed char)(ss->sampledata[ss->trim[1]*2 - pitchedpointer * ss->channels + 1]>>8) / (float)SCHAR_MAX;
							else *r = *l;
						} else
						{
							*l = (signed char)(ss->sampledata[pitchedpointer * ss->channels]>>8) / (float)SCHAR_MAX;
							if (ss->channels > 1) *r = (signed char)(ss->sampledata[pitchedpointer * ss->channels + 1]>>8) / (float)SCHAR_MAX;
							else *r = *l;
						}
					} else
					{
						if (ss->flags & S_FLAG_RPLAY)
						{
							*l = ss->sampledata[ss->trim[1]*2 - pitchedpointer * ss->channels + 0] / (float)SHRT_MAX;
							if (ss->channels > 1) *r = ss->sampledata[ss->trim[1]*2 - pitchedpointer * ss->channels + 1] / (float)SHRT_MAX;
							else *r = *l;
						} else
						{
							*l = ss->sampledata[pitchedpointer * ss->channels + 0] / (float)SHRT_MAX;
							if (ss->channels > 1) *r = ss->sampledata[pitchedpointer * ss->channels + 1] / (float)SHRT_MAX;
							else *r = *l;
						}
					}
				}
			} else *l = *r = 0.0;
		} else
			while (pitchedpointer > ss->loop[1])
				pitchedpointer -= ss->loop[1] - ss->loop[0];
	}

	/* trigger the release envelope */
	if (((ss->loop[1] && ss->trim[1] < ss->loop[1]) || !ss->loop[1])
			&& pitchedpointer > ss->trim[1] - (ss->volume.r * ENVELOPE_RELEASE * samplerate)
			&& !cv->releasepointer)
		cv->releasepointer = pointer;

	/* cut if the pointer is ever past trim[1] */
	if (pitchedpointer >= ss->trim[1])
		cv->r.note = 0;

	if (ss->flags & S_FLAG_RPLAY) /* always point to the left channel */
		pitchedpointer -= (ss->trim[1]*2 - pitchedpointer) % ss->channels;
	else
		pitchedpointer -= pitchedpointer % ss->channels;

	if (!(ss->loop[0] || ss->loop[1]))
	{
		if (pitchedpointer <= ss->length)
		{
			if (ss->flags & S_FLAG_8BIT)
			{
				if (ss->flags & S_FLAG_RPLAY)
				{
					*l = (signed char)(ss->sampledata[ss->trim[1]*2 - pitchedpointer * ss->channels + 0]>>8) / (float)SCHAR_MAX;
					if (ss->channels > 1) *r = (signed char)(ss->sampledata[ss->trim[1]*2 - pitchedpointer * ss->channels + 1]>>8) / (float)SCHAR_MAX;
					else *r = *l;
				} else
				{
					*l = (signed char)(ss->sampledata[pitchedpointer * ss->channels + 0]>>8) / (float)SCHAR_MAX;
					if (ss->channels > 1) *r = (signed char)(ss->sampledata[pitchedpointer * ss->channels + 1]>>8) / (float)SCHAR_MAX;
					else *r = *l;
				}
			} else
			{
				if (ss->flags & S_FLAG_RPLAY)
				{
					*l = ss->sampledata[ss->trim[1]*2 - pitchedpointer * ss->channels + 0] / (float)SHRT_MAX;
					if (ss->channels > 1) *r = ss->sampledata[ss->trim[1]*2 - pitchedpointer * ss->channels + 1] / (float)SHRT_MAX;
					else *r = *l;
				} else
				{
					*l = ss->sampledata[pitchedpointer * ss->channels + 0] / (float)SHRT_MAX;
					if (ss->channels > 1) *r = ss->sampledata[pitchedpointer * ss->channels + 1] / (float)SHRT_MAX;
					else *r = *l;
				}
			}
		} else *l = *r = 0.0;
	}

	/* sample gain */
	*l *= ss->gain*DIV64;
	*r *= ss->gain*DIV64;
	return pitchedpointer;
}

/* must be realtime safe                     */
/* must reasonably accept arbitrary pointers */
void samplerProcess(instrument *iv, channel *cv, uint32_t pointer, float *l, float *r)
{
	uint32_t pitchedpointer;
	sampler_state *ss = iv->state[iv->type];
	sampler_channel *sc = cv->state[iv->type];

	float gain = adsrEnvelope(ss->volume, 0.0, pointer, cv->releasepointer);
	if (pointer > ss->volume.a * ENVELOPE_ATTACK * samplerate
			&& gain == 0.0) /* sound has fully finished */
	{
		cv->r.note = 0;
		*l = 0.0;
		*r = 0.0;
	} else if (ss->length > 0)
	{
		float decimate = 1.0 + (1.0 - ss->samplerate*DIV256) * 20;
		if (ss->flags & S_FLAG_TTEMPO)
		{
			uint32_t pointersnap = pointer % ss->cyclelength;
			if (pointersnap == 0) /* first sample of a cycle */
			{
				/* don't ramp the first cycle */
				if (pointer == 0) sc->stretchrampindex = stretchrampmax;
				else
				{
					sc->stretchrampindex = 0;
					sc->localstretchrampmax = MIN(ss->cyclelength, stretchrampmax);
					uint32_t ramppointer;
					for (uint16_t i = sc->stretchrampindex; i < sc->localstretchrampmax; i++)
					{
						if (ss->samplerate == 255)
						{ /* decimate true bypass */
							ramppointer = pointer - ss->cyclelength + ((ss->cyclelength + 1 + i)
								* ((float)samplerate / (float)ss->c5rate)
								* powf(M_12_ROOT_2, (short)cv->r.note - 61 + cv->finetune));
						} else
						{
							ramppointer = (pointer - ss->cyclelength + ((ss->cyclelength + 1 + i)
								* ((float)samplerate / (float)ss->c5rate)
								* powf(M_12_ROOT_2, (short)cv->r.note - 61 + cv->finetune)))
								/ decimate;
							ramppointer *= decimate;
						}
						ramppointer = trimloop(ramppointer, pointer + i + 1, cv, iv, ss,
							&sc->stretchrampbuffer[i * 2 + 0],
							&sc->stretchrampbuffer[i * 2 + 1]);
						if (ramppointer > ss->length)
						{
							sc->localstretchrampmax = i;
							break;
						}
					}
				}
			}
			if (ss->samplerate == 255)
			{ /* decimate true bypass */
				pitchedpointer = pointer - pointersnap + (pointersnap
					* ((float)samplerate / (float)ss->c5rate)
					* powf(M_12_ROOT_2, (short)cv->r.note - 61 + cv->finetune));
			} else
			{
				pitchedpointer = (pointer - pointersnap + (pointersnap
					* ((float)samplerate / (float)ss->c5rate)
					* powf(M_12_ROOT_2, (short)cv->r.note - 61 + cv->finetune)))
					/ decimate;
				pitchedpointer *= decimate;
			}
		} else
		{
			if (ss->samplerate == 255)
			{ /* decimate true bypass */
				pitchedpointer = (float)pointer
					* ((float)samplerate / (float)ss->c5rate)
					* powf(M_12_ROOT_2, (short)cv->r.note - 61 + cv->finetune);
			} else
			{
				pitchedpointer = ((float)pointer
					* ((float)samplerate / (float)ss->c5rate)
					* powf(M_12_ROOT_2, (short)cv->r.note - 61 + cv->finetune))
					/ decimate;
				pitchedpointer *= decimate;
			}
		}

		/* formant shifting */
		uint32_t pointersnap = pitchedpointer % ss->formantcyclelength;
		if (pointersnap == 0) /* first sample of a cycle */
		{
			/* don't ramp the first cycle */
			if (pointer == 0) sc->formantrampindex = stretchrampmax;
			else
			{
				sc->formantrampindex = 0;
				sc->localformantrampmax = MIN(ss->formantcyclelength, stretchrampmax);
				uint32_t ramppointer;
				for (uint16_t i = sc->formantrampindex; i < sc->localformantrampmax; i++)
				{
					ramppointer = (pitchedpointer - ss->formantcyclelength) + ((float)(ss->formantcyclelength + 1 + i) * ss->formantshift*DIV128);
					ramppointer = trimloop(ramppointer, pointer + i + 1, cv, iv, ss,
						&sc->formantrampbuffer[i * 2 + 0],
						&sc->formantrampbuffer[i * 2 + 1]);
					if (ramppointer > ss->length)
					{
						sc->localformantrampmax = i;
						break;
					}
				}
			}
		}
		pitchedpointer = (pitchedpointer - pointersnap) + ((float)pointersnap * ss->formantshift*DIV128);


		/* trim/loop */
		pitchedpointer = trimloop(pitchedpointer, pointer, cv, iv, ss, l, r);

		if (pitchedpointer > ss->length)
		{
			cv->r.note = 0;
			*l = 0.0;
			*r = 0.0;
		}
	} else
	{
		cv->r.note = 0;
		*l = 0.0;
		*r = 0.0;
	}

	/* mix in ramp data */
	if (sc->stretchrampindex < sc->localstretchrampmax)
	{
		float gain = (float)sc->stretchrampindex / (float)sc->localstretchrampmax;
		*l *= gain;
		*r *= gain;
		*l += sc->stretchrampbuffer[sc->stretchrampindex * 2 + 0] * (1.0 - gain);
		*r += sc->stretchrampbuffer[sc->stretchrampindex * 2 + 1] * (1.0 - gain);
		sc->stretchrampindex++;
	}
	/* mix in formant ramp data */
	if (sc->formantrampindex < sc->localformantrampmax)
	{
		float gain = (float)sc->formantrampindex / (float)sc->localformantrampmax;
		*l *= gain;
		*r *= gain;
		*l += sc->formantrampbuffer[sc->formantrampindex * 2 + 0] * (1.0 - gain);
		*r += sc->formantrampbuffer[sc->formantrampindex * 2 + 1] * (1.0 - gain);
		sc->formantrampindex++;
	}

	if (ss->flags & S_FLAG_MONO)
	{
		*l = (*l + *r) / 2.0;
		*r = *l;
	}
	if (ss->flags & S_FLAG_PHASE)
	{
		*l *= -1;
		*r *= -1;
	}

	*l *= gain;
	*r *= gain;
}

/* must be realtime safe */
void samplerMacro(instrument *iv, channel *cv, row r, uint8_t macro, int m)
{
	sampler_state *ss = iv->state[iv->type];
	switch (macro)
	{
		case 0:
			if (cv->r.note) /* if playing a note */
			{
				if (!r.note) /* if not changing note: ramping needed */
				{
					/* clear the rampbuffer so cruft isn't played in edge cases */
					memset(cv->rampbuffer, 0, sizeof(sample_t) * rampmax * 2);
					for (uint16_t i = 0; i < rampmax; i++)
					{
						if (!cv->r.note) break;
						t->f[iv->type].process(iv, cv, cv->pointer + i,
								&cv->rampbuffer[i * 2 + 0], &cv->rampbuffer[i * 2 + 1]);
					}
					cv->rampindex = 0;
				}
				cv->pointeroffset = (m*DIV256) * (ss->trim[1] - ss->trim[0]);
				cv->pointer = 0;
			}
			break;
	}
}

/* called when state's type is changed to this file's */
void samplerAddType(void **state)
{
	*state = calloc(1, sizeof(sampler_state));
	sampler_state *ss = *state;
	ss->volume.s = 0xff;
	ss->gain = 0x40;
	ss->samplerate = 0xff;
	ss->flags = S_FLAG_LRAMP; /* loop ramping on by default */
	ss->cyclelength = 0x06ff;
	ss->formantcyclelength = 0x00ff;
	ss->formantshift = 0x80;
}

/* usually this can just be a memcpy call, but the sampler
 * needs to manage sampledata as well */
void samplerCopyType(void **dest, void **src)
{
	sampler_state *sdest = *dest;
	sampler_state *ssrc = *src;
	if (sdest->sampledata) /* free any existing sampledata */
		free(sdest->sampledata);

	memcpy(*dest, *src, sizeof(sampler_state));

	if (ssrc->sampledata) /* only try to copy sampledata if it exists */
	{
		sdest->sampledata = malloc(sizeof(short) * ssrc->samplelength);
		memcpy(sdest->sampledata, ssrc->sampledata, sizeof(short) * ssrc->samplelength);
	}
}

/* should clean up everything allocated in addType */
void samplerDelType(void **state)
{
	sampler_state *sstate = *state;
	if (sstate->sampledata)
		free(sstate->sampledata);

	free(*state);
}

/* should initialize channel state */
void samplerAddChannel(void **state)
{
	*state = calloc(1, sizeof(sampler_channel));
	sampler_channel *sc = *state;
	sc->stretchrampindex = stretchrampmax;
	sc->stretchrampbuffer = malloc(sizeof(sample_t) * stretchrampmax * 2); /* *2 for stereo */
	sc->formantrampbuffer = malloc(sizeof(sample_t) * stretchrampmax * 2); /* *2 for stereo */
}
/* should clean up everything allocated in addChannel */
void samplerDelChannel(void **state)
{
	sampler_channel *sc = *state;
	free(sc->stretchrampbuffer);
	free(sc->formantrampbuffer);
	free(*state);
}


void samplerWrite(void **state, FILE *fp)
{
	sampler_state *ss = *state;

	fwrite(&ss->length, sizeof(uint32_t), 1, fp);
	fwrite(&ss->channels, sizeof(uint8_t), 1, fp);
	fwrite(&ss->c5rate, sizeof(uint32_t), 1, fp);
	fwrite(&ss->samplerate, sizeof(uint8_t), 1, fp);
	fwrite(&ss->cyclelength, sizeof(uint16_t), 1, fp);
	fwrite(&ss->formantshift, sizeof(uint8_t), 1, fp);
	fwrite(&ss->formantcyclelength, sizeof(uint16_t), 1, fp);
	fwrite(ss->trim, sizeof(uint32_t), 2, fp);
	fwrite(ss->loop, sizeof(uint32_t), 2, fp);
	fwrite(&ss->volume, sizeof(adsr), 1, fp);
	fwrite(&ss->gain, sizeof(uint8_t), 1, fp);
	fwrite(&ss->flags, sizeof(uint8_t), 1, fp);
	fwrite(&ss->samplelength, sizeof(uint32_t), 1, fp);

	if (ss->samplelength)
		fwrite(ss->sampledata, sizeof(short), ss->samplelength, fp);
}

/* tied to the index defined in initInstrumentTypes, must have version handling */
void samplerRead(void **state, unsigned char major, unsigned char minor, FILE *fp)
{
	sampler_state *ss = *state;

	fread(&ss->length, sizeof(uint32_t), 1, fp);
	fread(&ss->channels, sizeof(uint8_t), 1, fp);
	fread(&ss->c5rate, sizeof(uint32_t), 1, fp);
	fread(&ss->samplerate, sizeof(uint8_t), 1, fp);
	fread(&ss->cyclelength, sizeof(uint16_t), 1, fp);
	fread(&ss->formantshift, sizeof(uint8_t), 1, fp);
	fread(&ss->formantcyclelength, sizeof(uint16_t), 1, fp);
	fread(ss->trim, sizeof(uint32_t), 2, fp);
	fread(ss->loop, sizeof(uint32_t), 2, fp);
	fread(&ss->volume, sizeof(adsr), 1, fp);
	fread(&ss->gain, sizeof(uint8_t), 1, fp);
	fread(&ss->flags, sizeof(uint8_t), 1, fp);
	fread(&ss->samplelength, sizeof(uint32_t), 1, fp);

	if (ss->samplelength)
	{
		ss->sampledata = malloc(sizeof(short) * ss->samplelength);
		fread(ss->sampledata, sizeof(short), ss->samplelength, fp);
	}
}

void samplerInit(int index)
{
	t->f[index].indexc = 21;
	t->f[index].cellwidth = 47;
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
	t->f[index].macro = &samplerMacro;
	t->f[index].addType = &samplerAddType;
	t->f[index].copyType = &samplerCopyType;
	t->f[index].delType = &samplerDelType;
	t->f[index].addChannel = &samplerAddChannel;
	t->f[index].delChannel = &samplerDelChannel;
	t->f[index].write = &samplerWrite;
	t->f[index].read = &samplerRead;
}
