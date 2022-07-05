#define S_FLAG_TTEMPO  0b00000001
#define S_FLAG_MONO    0b00000010
#define S_FLAG_8BIT    0b00000100
#define S_FLAG_PHASE   0b00001000
#define S_FLAG_RPLAY   0b00010000
#define S_FLAG_PPLOOP  0b00100000
typedef struct
{
	short   *sampledata;   /* variable size, persists between types */
	uint32_t samplelength; /* raw samples allocated for sampledata */

	uint32_t length;
	uint8_t  channels;
	uint32_t c5rate;
	uint8_t  samplerate;   /* percent of c5rate to actually use */
	uint16_t cyclelength;
	uint8_t  pitchshift;
	uint32_t trim[2];
	uint32_t loop[2];
	adsr     volume;
	uint8_t  gain;
	uint8_t  flags;
	char     multisample;
	uint8_t  loopramp;
} sampler_state;

typedef struct
{
	uint16_t  stretchrampindex;    /* progress through the stretch ramp buffer, >=localstretchrampmax if not ramping */
	uint16_t  localstretchrampmax; /* actual stretchrampmax used, to allow for tiny buffer sizes */
	sample_t *stretchrampbuffer;   /* raw samples to ramp out */
} sampler_channel;


void samplerIncFieldPointer(short index)
{
	switch (index)
	{
		case 1: case 6: case 7: case 8: case 9:
			w->fieldpointer--;
			if (w->fieldpointer < 0) w->fieldpointer = 7;
			break;
		case 19:
			w->fieldpointer--;
			if (w->fieldpointer < 0) w->fieldpointer = 3;
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
		case 1: case 6: case 7: case 8: case 9:
			w->fieldpointer++;
			if (w->fieldpointer > 7) w->fieldpointer = 0;
			break;
		case 19:
			w->fieldpointer++;
			if (w->fieldpointer > 3) w->fieldpointer = 0;
			break;
		case 3:  case 11: case 13: case 14:
		case 15: case 16: case 17: case 20:
			w->fieldpointer = 1;
			break;
		default:
			w->fieldpointer = 0;
			break;
	}
}
void samplerEndFieldPointer(short index)
{
	w->fieldpointer = 0;
}

void inputSamplerHex(short index, sampler_state *ss, char value)
{
	switch (index)
	{
		case 1:  updateField(w->fieldpointer, (uint32_t *)&ss->c5rate, value); break;
		case 3:  updateFieldPush(&ss->samplerate, value); break;
		case 6:  updateField(w->fieldpointer, (uint32_t *)&ss->trim[0], value); ss->trim[0] = MIN(ss->trim[1], MIN(ss->trim[0], ss->length)); break;
		case 7:  updateField(w->fieldpointer, (uint32_t *)&ss->trim[1], value); ss->trim[1] = MAX(ss->trim[0], MIN(ss->trim[1], ss->length)); break;
		case 8:  updateField(w->fieldpointer, (uint32_t *)&ss->loop[0], value); ss->loop[0] = MIN(ss->loop[1], MIN(ss->loop[0], ss->length)); break;
		case 9:  updateField(w->fieldpointer, (uint32_t *)&ss->loop[1], value); ss->loop[1] = MAX(ss->loop[0], MIN(ss->loop[1], ss->length)); break;
		case 11: updateFieldPush(&ss->loopramp, value); break;
		case 13: updateFieldPush(&ss->volume.a, value); break;
		case 14: updateFieldPush(&ss->volume.d, value); break;
		case 15: updateFieldPush(&ss->volume.s, value); break;
		case 16: updateFieldPush(&ss->volume.r, value); break;
		case 17: updateFieldPush(&ss->gain, value); break;
		case 19: updateField(w->fieldpointer, (uint32_t *)&ss->cyclelength, value); break;
		case 20: updateFieldPush(&ss->pitchshift, value); break;
	}
	samplerIncFieldPointer(index);
}

void drawSampler(instrument *iv, uint8_t index, unsigned short x, unsigned short y, short *cursor, char adjust)
{
	sampler_state *ss = iv->state[iv->type];
	printf("\033[%d;%dH [sampler] ", y-1, x+17);

	for (char i = 1; i < 13; i++)
	printf("\033[%d;%dH│", y+i, x+22);
	printf("\033[%d;%dH",  y+1, x+0);
	if (!ss->samplelength)
		printf("[     \033[1mNO SAMPLE\033[m     ]");
	else switch (ss->channels)
	{
		case 1:  printf("[   \033[1m%08x MONO\033[m   ]", ss->length); break;
		default: printf("[  \033[1m%08x STEREO\033[m  ]", ss->length); break;
	}
	printf("\033[%d;%dHC-5 rate   [%08x]",       y+2,  x+0, ss->c5rate);
	printf("\033[%d;%dHmono/rate     ",          y+3,  x+0); drawBit(ss->flags & S_FLAG_MONO); printf("[%02x]", ss->samplerate);
	printf("\033[%d;%dHinvert/8-bit   ",         y+4,  x+0); drawBit(ss->flags & S_FLAG_PHASE); drawBit(ss->flags & S_FLAG_8BIT);
	printf("\033[%d;%dH──────────────────────┤", y+6,  x+0);
	printf("\033[%d;%dH\033[1mAMPLIFIER\033[m",  y+7,  x+6);
	printf("\033[%d;%dHattack           [%02x]", y+8,  x+0, ss->volume.a);
	printf("\033[%d;%dHdecay            [%02x]", y+9,  x+0, ss->volume.d);
	printf("\033[%d;%dHsustain          [%02x]", y+10, x+0, ss->volume.s);
	printf("\033[%d;%dHrelease          [%02x]", y+11, x+0, ss->volume.r);
	printf("\033[%d;%dHgain             [%02x]", y+12, x+0, ss->gain);
	printf("\033[%d;%dH\033[1mTRIM        LOOP\033[m", y+1,  x+27);
	printf("\033[%d;%dH[%08x]  [%08x]",                y+2,  x+24, ss->trim[0], ss->loop[0]);
	printf("\033[%d;%dH[%08x]  [%08x]",                y+3,  x+24, ss->trim[1], ss->loop[1]);
	printf("\033[%d;%dH%02x->%02x      %02x->%02x",    y+4,  x+26,
			(uint8_t)((float)ss->trim[0] / (float)ss->length * 255),
			(uint8_t)((float)ss->trim[1] / (float)ss->length * 255),
			(uint8_t)((float)ss->loop[0] / (float)ss->length * 255),
			(uint8_t)((float)ss->loop[1] / (float)ss->length * 255));
	printf("\033[%d;%dHreverse playback   ",      y+5,  x+24);
	drawBit(ss->flags & S_FLAG_RPLAY);
	printf("\033[%d;%dHloop crossfade    [%02x]", y+6,  x+24, ss->loopramp);
	printf("\033[%d;%dHping-pong loop     ",      y+7,  x+24);
	drawBit(ss->flags & S_FLAG_PPLOOP);
	printf("\033[%d;%dH├───────────────────────", y+8,  x+22);
	printf("\033[%d;%dH\033[1mTIMESTRETCH\033[m", y+9,  x+30);
	printf("\033[%d;%dHstretch tempo      ",      y+10, x+24); drawBit(ss->flags & S_FLAG_TTEMPO);
	printf("\033[%d;%dHcycle length    [%04x]",   y+11, x+24, ss->cyclelength);
	printf("\033[%d;%dHpitch shift       [%02x]", y+12, x+24, ss->pitchshift);

	if (w->instrumentrecv == INST_REC_LOCK_CONT) printf("\033[%d;%dHREC", y, x+0);

	switch (*cursor)
	{
		case 0:  printf("\033[%d;%dH", y+1,  x+19); break;
		case 1:  printf("\033[%d;%dH", y+2,  x+19 - w->fieldpointer); break;
		case 2:  printf("\033[%d;%dH", y+3,  x+15); break;
		case 3:  printf("\033[%d;%dH", y+3,  x+19 - w->fieldpointer); break;
		case 4:  printf("\033[%d;%dH", y+4,  x+16); break;
		case 5:  printf("\033[%d;%dH", y+4,  x+19); break;
		case 6:  printf("\033[%d;%dH", y+2,  x+32 - w->fieldpointer); break;
		case 7:  printf("\033[%d;%dH", y+3,  x+32 - w->fieldpointer); break;
		case 8:  printf("\033[%d;%dH", y+2,  x+44 - w->fieldpointer); break;
		case 9:  printf("\033[%d;%dH", y+3,  x+44 - w->fieldpointer); break;
		case 10: printf("\033[%d;%dH", y+5,  x+44); break;
		case 11: printf("\033[%d;%dH", y+6,  x+44 - w->fieldpointer); break;
		case 12: printf("\033[%d;%dH", y+7,  x+44); break;
		case 13: printf("\033[%d;%dH", y+8,  x+19 - w->fieldpointer); break;
		case 14: printf("\033[%d;%dH", y+9,  x+19 - w->fieldpointer); break;
		case 15: printf("\033[%d;%dH", y+10, x+19 - w->fieldpointer); break;
		case 16: printf("\033[%d;%dH", y+11, x+19 - w->fieldpointer); break;
		case 17: printf("\033[%d;%dH", y+12, x+19 - w->fieldpointer); break;
		case 18: printf("\033[%d;%dH", y+10, x+44 - w->fieldpointer); break;
		case 19: printf("\033[%d;%dH", y+11, x+44 - w->fieldpointer); break;
		case 20: printf("\033[%d;%dH", y+12, x+44 - w->fieldpointer); break;
	}
}

void samplerAdjustUp(instrument *iv, short index, char mouse)
{
	sampler_state *ss = iv->state[iv->type];
	uint32_t temp;
	switch (index)
	{
		case 1:  incField(w->fieldpointer, &ss->c5rate, 0xffffffff); break;
		case 3:  if (w->fieldpointer) { if (ss->samplerate < 255 - 16) ss->samplerate+=16; else ss->samplerate = 255; } else if (ss->samplerate < 255) ss->samplerate++; break;
		case 6:  incField(w->fieldpointer, &ss->trim[0], 0xffffffff); ss->trim[0] = MIN(ss->trim[1], MIN(ss->trim[0], ss->length)); break;
		case 7:  incField(w->fieldpointer, &ss->trim[1], 0xffffffff); ss->trim[1] = MAX(ss->trim[0], MIN(ss->trim[1], ss->length)); break;
		case 8:  incField(w->fieldpointer, &ss->loop[0], 0xffffffff); ss->loop[0] = MIN(ss->loop[1], MIN(ss->loop[0], ss->length)); break;
		case 9:  incField(w->fieldpointer, &ss->loop[1], 0xffffffff); ss->loop[1] = MAX(ss->loop[0], MIN(ss->loop[1], ss->length)); break;
		case 11: if (w->fieldpointer) { if (ss->loopramp < 255 - 16) ss->loopramp+=16; else ss->loopramp = 255; } else if (ss->loopramp < 255) ss->loopramp++; break;
		case 13: if (w->fieldpointer) { if (ss->volume.a < 255 - 16) ss->volume.a+=16; else ss->volume.a = 255; } else if (ss->volume.a < 255) ss->volume.a++; break;
		case 14: if (w->fieldpointer) { if (ss->volume.d < 255 - 16) ss->volume.d+=16; else ss->volume.d = 255; } else if (ss->volume.d < 255) ss->volume.d++; break;
		case 15: if (w->fieldpointer) { if (ss->volume.s < 255 - 16) ss->volume.s+=16; else ss->volume.s = 255; } else if (ss->volume.s < 255) ss->volume.s++; break;
		case 16: if (w->fieldpointer) { if (ss->volume.r < 255 - 16) ss->volume.r+=16; else ss->volume.r = 255; } else if (ss->volume.r < 255) ss->volume.r++; break;
		case 17: if (w->fieldpointer) { if (ss->gain < 255 - 16) ss->gain+=16; else ss->gain = 255; } else if (ss->gain < 255) ss->gain++; break;
		case 19: temp = ss->cyclelength; incField(w->fieldpointer, &temp, 0xffff); ss->cyclelength = temp; break;
		case 20: if (w->fieldpointer) { if (ss->pitchshift < 255 - 16) ss->pitchshift+=16; else ss->pitchshift = 255; } else if (ss->pitchshift < 255) ss->pitchshift++; break;
	}
}
void samplerAdjustDown(instrument *iv, short index, char mouse)
{
	sampler_state *ss = iv->state[iv->type];
	uint32_t temp;
	switch (index)
	{
		case 1:  decField(w->fieldpointer, &ss->c5rate); break;
		case 3:  if (w->fieldpointer) { if (ss->samplerate > 16) ss->samplerate-=16; else ss->samplerate = 0; } else if (ss->samplerate) ss->samplerate--; break;
		case 6:  decField(w->fieldpointer, &ss->trim[0]); ss->trim[0] = MIN(ss->trim[1], MIN(ss->trim[0], ss->length)); break;
		case 7:  decField(w->fieldpointer, &ss->trim[1]); ss->trim[1] = MAX(ss->trim[0], MIN(ss->trim[1], ss->length)); break;
		case 8:  decField(w->fieldpointer, &ss->loop[0]); ss->loop[0] = MIN(ss->loop[1], MIN(ss->loop[0], ss->length)); break;
		case 9:  decField(w->fieldpointer, &ss->loop[1]); ss->loop[1] = MAX(ss->loop[0], MIN(ss->loop[1], ss->length)); break;
		case 11: if (w->fieldpointer) { if (ss->loopramp > 16) ss->loopramp-=16; else ss->loopramp = 0; } else if (ss->loopramp) ss->loopramp--; break;
		case 13: if (w->fieldpointer) { if (ss->volume.a > 16) ss->volume.a-=16; else ss->volume.a = 0; } else if (ss->volume.a) ss->volume.a--; break;
		case 14: if (w->fieldpointer) { if (ss->volume.d > 16) ss->volume.d-=16; else ss->volume.d = 0; } else if (ss->volume.d) ss->volume.d--; break;
		case 15: if (w->fieldpointer) { if (ss->volume.s > 16) ss->volume.s-=16; else ss->volume.s = 0; } else if (ss->volume.s) ss->volume.s--; break;
		case 16: if (w->fieldpointer) { if (ss->volume.r > 16) ss->volume.r-=16; else ss->volume.r = 0; } else if (ss->volume.r) ss->volume.r--; break;
		case 17: if (w->fieldpointer) { if (ss->gain > 16) ss->gain-=16; else ss->gain = 0; } else if (ss->gain) ss->gain--; break;
		case 19: temp = ss->cyclelength; decField(w->fieldpointer, &temp); ss->cyclelength = temp; break;
		case 20: if (w->fieldpointer) { if (ss->pitchshift > 16) ss->pitchshift-=16; else ss->pitchshift = 0; } else if (ss->pitchshift) ss->pitchshift--; break;
	}
}

void samplerAdjustLeft(instrument *iv, short index, char mouse)
{ if (!mouse) samplerDecFieldPointer(index); }

void samplerAdjustRight(instrument *iv, short index, char mouse)
{ if (!mouse) samplerIncFieldPointer(index); }


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
				// case 't': /* apply trimming */
					samplerApplyTrimming(iv);
					redraw();
					break;
				case 'e': /* export */
					setCommand(&w->command, &samplerExportCallback, NULL, NULL, 0, "File name: ", "");
					w->mode = 255;
					redraw();
					break;
				case 'w': /* waveform */
					w->popup = 4;
					w->instrumentindex = 0;
					w->fyoffset = 0; /* this can still be set on edge cases */
					w->waveformoffset = 0;
					w->waveformwidth = ss->length;
					resizeWaveform();
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
						case 18:  /* timestretch tempo */ ss->flags ^= S_FLAG_TTEMPO; redraw(); *input = 0; break;
						case 10: /* reverse playback  */ ss->flags ^= S_FLAG_RPLAY; redraw(); *input = 0; break;
						case 12: /* ping-pong loop    */ ss->flags ^= S_FLAG_PPLOOP; redraw(); *input = 0; break;
					}
					break;
				case 1: /* ^a */
					switch (w->instrumentindex)
					{
						case 1:  ss->c5rate++; break;
						case 3:  ss->samplerate++; break;
						case 6:  if (ss->trim[0] == ss->length) ss->trim[0] = 0; else ss->trim[0]++;  ss->trim[0] = MIN(ss->trim[1], MIN(ss->trim[0], ss->length)); break;
						case 7:  if (ss->trim[1] == ss->length) ss->trim[1] = 0; else ss->trim[1]++;  ss->trim[1] = MAX(ss->trim[0], MIN(ss->trim[1], ss->length)); break;
						case 8:  if (ss->loop[0] == ss->length) ss->loop[0] = 0; else ss->loop[0]++;  ss->loop[0] = MIN(ss->loop[1], MIN(ss->loop[0], ss->length)); break;
						case 9:  if (ss->loop[1] == ss->length) ss->loop[1] = 0; else ss->loop[1]++;  ss->loop[1] = MAX(ss->loop[0], MIN(ss->loop[1], ss->length)); break;
						case 11: ss->loopramp++; break;
						case 13: ss->volume.a++; break;
						case 14: ss->volume.d++; break;
						case 15: ss->volume.s++; break;
						case 16: ss->volume.r++; break;
						case 17: ss->gain++; break;
						case 19: ss->cyclelength++; break;
						case 20: ss->pitchshift++; break;
					}
					break;
				case 24: /* ^x */
					switch (w->instrumentindex)
					{
						case 1:  ss->c5rate--; break;
						case 3:  ss->samplerate--; break;
						case 6:  ss->trim[0]--;  ss->trim[0] = MIN(ss->trim[1], MIN(ss->trim[0], ss->length)); break;
						case 7:  ss->trim[1]--;  ss->trim[1] = MAX(ss->trim[0], MIN(ss->trim[1], ss->length)); break;
						case 8:  ss->loop[0]--;  ss->loop[0] = MIN(ss->loop[1], MIN(ss->loop[0], ss->length)); break;
						case 9:  ss->loop[1]--;  ss->loop[1] = MAX(ss->loop[0], MIN(ss->loop[1], ss->length)); break;
						case 11: ss->loopramp--; break;
						case 13: ss->volume.a--; break;
						case 14: ss->volume.d--; break;
						case 15: ss->volume.s--; break;
						case 16: ss->volume.r--; break;
						case 17: ss->gain--; break;
						case 19: ss->cyclelength--; break;
						case 20: ss->pitchshift--; break;
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
				// previewNote(0, 255, w->channel);
				w->popup = 2;
				w->instrumentindex = 0;
				w->fyoffset = 0; /* this can still be set on edge cases */
				w->filebrowserCallback = &samplerLoadCallback;
			} break;
		default:
			if (x < 22)
				switch (y)
				{
					case 0: case 1: case 2: *index = 0;
						w->fieldpointer = 0;
						if (button == BUTTON3 || button == BUTTON3_CTRL)
						{
							// previewNote(0, 255, w->channel);
							w->popup = 2;
							w->mode = 0;
							w->instrumentindex = 0;
							w->fyoffset = 0; /* this can still be set on edge cases */
							w->filebrowserCallback = &samplerLoadCallback;
						}
						break;
					case 3: *index = 1;
						if (x < 12)          w->fieldpointer = 7;
						else if (x > 12 + 7) w->fieldpointer = 0;
						else w->fieldpointer = (x - 12)*-1 + 7;
						break;
					case 4:
						if (x < 17) { *index = 2; ss->flags ^= S_FLAG_MONO; w->fieldpointer = 0; }
						else        { *index = 3; if (x < 19) w->fieldpointer = 1; else w->fieldpointer = 0; }
						break;
					case 5: case 6: case 7:
						if (x < 18) { *index = 4; ss->flags ^= S_FLAG_PHASE; }
						else        { *index = 5; ss->flags ^= S_FLAG_8BIT; }
						w->fieldpointer = 0; break;
					case 8: case 9: *index = 13; if (x < 19) w->fieldpointer = 1; else w->fieldpointer = 0; break;
					case 10: *index = 14; if (x < 19) w->fieldpointer = 1; else w->fieldpointer = 0; break;
					case 11: *index = 15; if (x < 19) w->fieldpointer = 1; else w->fieldpointer = 0; break;
					case 12: *index = 16; if (x < 19) w->fieldpointer = 1; else w->fieldpointer = 0; break;
					default: *index = 17; if (x < 19) w->fieldpointer = 1; else w->fieldpointer = 0; break;
				}
			else
				switch (y)
				{
					case 0: case 1: case 2: case 3:
						if (x < 35)
						{
							*index = 6;
							if (x < 25)          w->fieldpointer = 7;
							else if (x > 25 + 7) w->fieldpointer = 0;
							else w->fieldpointer = (x - 25)*-1 + 7;
						} else
						{
							*index = 8;
							if (x < 37)          w->fieldpointer = 7;
							else if (x > 37 + 7) w->fieldpointer = 0;
							else w->fieldpointer = (x - 37)*-1 + 7;
						} break;
					case 4: case 5:
						if (x < 35)
						{
							*index = 7;
							if (x < 25)          w->fieldpointer = 7;
							else if (x > 25 + 7) w->fieldpointer = 0;
							else w->fieldpointer = (x - 25)*-1 + 7;
						} else
						{
							*index = 9;
							if (x < 37)          w->fieldpointer = 7;
							else if (x > 37 + 7) w->fieldpointer = 0;
							else w->fieldpointer = (x - 37)*-1 + 7;
						} break;
					case 6: *index = 10; w->fieldpointer = 0; ss->flags ^= S_FLAG_RPLAY; break;
					case 7: *index = 11; if (x < 44) w->fieldpointer = 1; else w->fieldpointer = 0; break;
					case 8: case 9: *index = 12; w->fieldpointer = 0; ss->flags ^= S_FLAG_PPLOOP; break;
					case 10: case 11: *index = 18; w->fieldpointer = 0; ss->flags ^= S_FLAG_TTEMPO; break;
					case 12: *index = 19;
						if (x < 41)          w->fieldpointer = 3;
						else if (x > 41 + 3) w->fieldpointer = 0;
						else w->fieldpointer = (x - 41)*-1 + 3;
						break;
					default: *index = 20; if (x < 44) w->fieldpointer = 1; else w->fieldpointer = 0; break;
				} break;
	}
}

void getSample(uint32_t p, sampler_state *ss, float *l, float *r)
{
	if (ss->flags & S_FLAG_8BIT) {
		if (ss->flags & S_FLAG_RPLAY) {
			/* listchars */       *l += (signed char)(ss->sampledata[ss->trim[1]*2 - p * ss->channels+0]>>8)*DIVCHAR;
			if (ss->channels > 1) *r += (signed char)(ss->sampledata[ss->trim[1]*2 - p * ss->channels+1]>>8)*DIVCHAR;
			else                  *r += (signed char)(ss->sampledata[ss->trim[1]*2 - p * ss->channels+0]>>8)*DIVCHAR;
		} else {
			/* listchars */       *l += (signed char)(ss->sampledata[p * ss->channels+0]>>8)*DIVCHAR;
			if (ss->channels > 1) *r += (signed char)(ss->sampledata[p * ss->channels+1]>>8)*DIVCHAR;
			else                  *r += (signed char)(ss->sampledata[p * ss->channels+0]>>8)*DIVCHAR;
	} } else {
		if (ss->flags & S_FLAG_RPLAY) {
			/* listchars */       *l += ss->sampledata[ss->trim[1]*2 - p * ss->channels+0] * DIVSHRT;
			if (ss->channels > 1) *r += ss->sampledata[ss->trim[1]*2 - p * ss->channels+1] * DIVSHRT;
			else                  *r += ss->sampledata[ss->trim[1]*2 - p * ss->channels+0] * DIVSHRT;
		} else {
			/* listchars */       *l += ss->sampledata[p * ss->channels+0] * DIVSHRT;
			if (ss->channels > 1) *r += ss->sampledata[p * ss->channels+1] * DIVSHRT;
			else                  *r += ss->sampledata[p * ss->channels+0] * DIVSHRT;
} } }

void getSampleLoopRamp(uint32_t p, uint32_t q, float lerp, sampler_state *ss, float *l, float *r)
{
	if (ss->flags & S_FLAG_8BIT) {
		if (ss->flags & S_FLAG_RPLAY) {
			/* listchars */       *l += (signed char)(ss->sampledata[ss->trim[1]*2 - p * ss->channels+0]>>8)*DIVCHAR * (1.0 - lerp) + (signed char)(ss->sampledata[ss->trim[1]*2 - q * ss->channels+0]>>8)*DIVCHAR * lerp;
			if (ss->channels > 1) *r += (signed char)(ss->sampledata[ss->trim[1]*2 - p * ss->channels+1]>>8)*DIVCHAR * (1.0 - lerp) + (signed char)(ss->sampledata[ss->trim[1]*2 - q * ss->channels+1]>>8)*DIVCHAR * lerp;
			else                  *r += (signed char)(ss->sampledata[ss->trim[1]*2 - p * ss->channels+0]>>8)*DIVCHAR * (1.0 - lerp) + (signed char)(ss->sampledata[ss->trim[1]*2 - q * ss->channels+0]>>8)*DIVCHAR * lerp;
		} else {
			/* listchars */       *l += (signed char)(ss->sampledata[p * ss->channels+0]>>8)*DIVCHAR * (1.0 - lerp) + (signed char)(ss->sampledata[q * ss->channels+0]>>8)*DIVCHAR * lerp;
			if (ss->channels > 1) *r += (signed char)(ss->sampledata[p * ss->channels+1]>>8)*DIVCHAR * (1.0 - lerp) + (signed char)(ss->sampledata[q * ss->channels+1]>>8)*DIVCHAR * lerp;
			else                  *r += (signed char)(ss->sampledata[p * ss->channels+0]>>8)*DIVCHAR * (1.0 - lerp) + (signed char)(ss->sampledata[q * ss->channels+0]>>8)*DIVCHAR * lerp;
	} } else {
		if (ss->flags & S_FLAG_RPLAY) {
			/* listchars */       *l += ss->sampledata[ss->trim[1]*2 - p * ss->channels+0] * DIVSHRT * (1.0 - lerp) + ss->sampledata[ss->trim[1]*2 - q * ss->channels+0] * DIVSHRT * lerp;
			if (ss->channels > 1) *r += ss->sampledata[ss->trim[1]*2 - p * ss->channels+1] * DIVSHRT * (1.0 - lerp) + ss->sampledata[ss->trim[1]*2 - q * ss->channels+1] * DIVSHRT * lerp;
			else                  *r += ss->sampledata[ss->trim[1]*2 - p * ss->channels+0] * DIVSHRT * (1.0 - lerp) + ss->sampledata[ss->trim[1]*2 - q * ss->channels+0] * DIVSHRT * lerp;
		} else {
			/* listchars */       *l += ss->sampledata[p * ss->channels+0] * DIVSHRT * (1.0 - lerp) + ss->sampledata[q * ss->channels+0] * DIVSHRT * lerp;
			if (ss->channels > 1) *r += ss->sampledata[p * ss->channels+1] * DIVSHRT * (1.0 - lerp) + ss->sampledata[q * ss->channels+1] * DIVSHRT * lerp;
			else                  *r += ss->sampledata[p * ss->channels+0] * DIVSHRT * (1.0 - lerp) + ss->sampledata[q * ss->channels+0] * DIVSHRT * lerp;
} } }

/* clamps within range and loop, returns output samples */
void trimloop(uint32_t pitchedpointer, uint32_t pointer,
		channel *cv, instrument *iv, sampler_state *ss, sampler_channel *sc,
		sample_t *l, sample_t *r)
{
	uint32_t p = pitchedpointer + ss->trim[0] + cv->pointeroffset;

	if (ss->loop[1] && ss->loop[0] == ss->loop[1] && p > ss->loop[1])
	{ /* avoid looping a dc offset and nothing else */
		p = ss->loop[1] + p%(uint32_t)(samplerate/1000 * MIN_LOOP_MS);
		getSample(p, ss, l, r);
	} else if (ss->loop[1])
	{ /* if there is a loop range */
		if (ss->flags & S_FLAG_PPLOOP)
		{ /* ping-pong loop */
			uint32_t looplength = ss->loop[1] - ss->loop[0];
			if (p > ss->loop[1])
			{
				uint32_t i = (p - ss->loop[1])/looplength;
				if (i % 2 == 0) /* backwards */ p = ss->loop[1] - (p - ss->loop[1])%looplength;
				else            /* forwards  */ p = ss->loop[0] + (p - ss->loop[1])%looplength;
			}

			/* always point to the left channel */
			if (ss->flags & S_FLAG_RPLAY) p -= (ss->trim[1]*2 - p) % ss->channels;
			else                          p -= p % ss->channels;
			getSample(p, ss, l, r);
		} else
		{ /* crossfaded forwards loop */
			uint32_t looprampmax = MIN(samplerate/1000 * LOOP_RAMP_MS, (ss->loop[1] - ss->loop[0]) * 0.5) * (ss->loopramp*DIV255);
			uint32_t looplength = ss->loop[1] - ss->loop[0] - looprampmax;
			if (p > ss->loop[1]) p = ss->loop[0] + looprampmax + (p - ss->loop[1])%looplength;

			/* always point to the left channel */
			if (ss->flags & S_FLAG_RPLAY) p -= (ss->trim[1]*2 - p) % ss->channels;
			else                          p -= p % ss->channels;

			if (p > ss->loop[1] - looprampmax)
			{
				float lerp = (p - ss->loop[1] + looprampmax) / (float)looprampmax;
				uint32_t ramppointer = (p - looplength);
				/* always point to the left channel */
				if (ss->flags & S_FLAG_RPLAY) ramppointer -= (ss->trim[1]*2 - ramppointer) % ss->channels;
				else                          ramppointer -= ramppointer % ss->channels;
				getSampleLoopRamp(p, ramppointer, lerp, ss, l, r);
			} else getSample(p, ss, l, r);
		}
	}

	/* trigger the release envelope */
	if (((ss->loop[1] && ss->trim[1] < ss->loop[1]) || !ss->loop[1])
			&& p > ss->trim[1] - (ss->volume.r * ENVELOPE_RELEASE * samplerate)
			&& !cv->releasepointer)
		cv->releasepointer = pointer;

	/* cut if the pointer is ever past trim[1] */
	if (p > ss->trim[1]) cv->r.note = 0;

	if (!(ss->loop[0] || ss->loop[1]))
	{
		/* always point to the left channel */
		if (ss->flags & S_FLAG_RPLAY) p -= (ss->trim[1]*2 - p) % ss->channels;
		else                          p -= p % ss->channels;

		if (p <= ss->length) getSample(p, ss, l, r);
	}

	/* sample gain */
	*l *= ss->gain*DIV64;
	*r *= ss->gain*DIV64;
}

uint32_t calcDecimate(sampler_state *ss, float decimate, uint32_t pointer)
{
	if (ss->samplerate == 255)
		return pointer;
	else
		return (uint32_t)(pointer / decimate) * decimate;
}

/* must be realtime safe                     */
/* must reasonably accept arbitrary pointers */
void samplerProcess(instrument *iv, channel *cv, uint32_t pointer, float *l, float *r)
{
	uint32_t ramppos, pointersnap;
	uint16_t cyclelength;
	sampler_state *ss = iv->state[iv->type];
	sampler_channel *sc = cv->state[iv->type];

	float decimate = 1.0 + (1.0 - ss->samplerate*DIV255) * 20;
	float gain = adsrEnvelope(ss->volume, 0.0, pointer, cv->releasepointer);
	if (pointer > (ss->volume.a+ENVELOPE_ATTACK_MIN) * ENVELOPE_ATTACK * samplerate
			&& gain == 0.0) /* sound has fully finished */
		cv->r.note = 0;
	else if (ss->length > 0)
	{
		cyclelength = MAX(ss->cyclelength, 1);
		pointersnap = pointer % cyclelength;

		if (cv->reverse) ramppos = cyclelength;
		else             ramppos = 0;
		if (pointersnap == ramppos)
		{ // first sample of a cycle
			sc->localstretchrampmax = MIN(cyclelength, stretchrampmax);
			if (pointer == 0) sc->stretchrampindex = sc->localstretchrampmax;
			else sc->stretchrampindex = 0;
		}

		if (ss->flags & S_FLAG_TTEMPO)
		{ /* time stretch */
			/* trim/loop */
			trimloop(calcDecimate(ss, decimate,
					pointer - pointersnap + (pointersnap
						* powf(M_12_ROOT_2, (short)cv->r.note - C5 + cv->finetune))
						* ((float)ss->c5rate / (float)samplerate)),
					pointer, cv, iv, ss, sc, l, r);

			if (sc->stretchrampindex < sc->localstretchrampmax)
			{
				float rl = 0.0, rr = 0.0;
				trimloop(calcDecimate(ss, decimate,
						pointer - pointersnap - cyclelength + ((cyclelength + sc->stretchrampindex)
							* powf(M_12_ROOT_2, (short)cv->r.note - C5 + cv->finetune)
							* ((float)ss->c5rate / (float)samplerate))),
						pointer, cv, iv, ss, sc, &rl, &rr);

				float gain = (float)sc->stretchrampindex / (float)sc->localstretchrampmax;
				*l = *l * gain + rl * (1.0 - gain);
				*r = *r * gain + rr * (1.0 - gain);
				sc->stretchrampindex++;
			}
		} else
		{ /* pitch shift */
			/* trim/loop */
			trimloop(calcDecimate(ss, decimate,
					(float)(pointer - pointersnap + (pointersnap * (float)(ss->pitchshift*DIV128)))
						* powf(M_12_ROOT_2, (short)cv->r.note - C5 + cv->finetune)
						* ((float)ss->c5rate / (float)samplerate)),
					pointer, cv, iv, ss, sc, l, r);

			if (sc->stretchrampindex < sc->localstretchrampmax)
			{
				float rl = 0.0, rr = 0.0;
				trimloop(calcDecimate(ss, decimate,
						(float)(pointer - pointersnap - cyclelength + ((cyclelength + sc->stretchrampindex) * (float)(ss->pitchshift*DIV128)))
							* powf(M_12_ROOT_2, (short)cv->r.note - C5 + cv->finetune)
							* ((float)ss->c5rate / (float)samplerate)),
						pointer, cv, iv, ss, sc, &rl, &rr);

				float gain = (float)sc->stretchrampindex / (float)sc->localstretchrampmax;
				*l = *l * gain + rl * (1.0 - gain);
				*r = *r * gain + rr * (1.0 - gain);
				sc->stretchrampindex++;
			}
		}
	} else cv->r.note = 0;

	if (ss->flags & S_FLAG_MONO)
	{ *l = (*l + *r) / 2.0; *r = *l; }
	if (ss->flags & S_FLAG_PHASE)
	{ *l *= -1; *r *= -1; }

	*l *= gain; *r *= gain;
}

/* must be realtime safe         */
/* can respond to Oxx (offset)   */
/* numbers are reserved for here */
void samplerMacro(instrument *iv, channel *cv, row r, uint8_t macro, int m)
{
	sampler_state *ss = iv->state[iv->type];
	switch (macro)
	{
		case 'O':
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
				cv->pointeroffset = (m*DIV255) * (ss->trim[1] - ss->trim[0]);
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
	ss->flags = 0;
	ss->loopramp = 0xff;
	ss->cyclelength = 0x06ff;
	ss->pitchshift = 0x80;
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
}
/* should clean up everything allocated in addChannel */
void samplerDelChannel(void **state)
{
	sampler_channel *sc = *state;
	free(sc->stretchrampbuffer);
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
	fwrite(&ss->pitchshift, sizeof(uint8_t), 1, fp);
	fwrite(ss->trim, sizeof(uint32_t), 2, fp);
	fwrite(ss->loop, sizeof(uint32_t), 2, fp);
	fwrite(&ss->volume, sizeof(adsr), 1, fp);
	fwrite(&ss->gain, sizeof(uint8_t), 1, fp);
	fwrite(&ss->flags, sizeof(uint8_t), 1, fp);
	fwrite(&ss->samplelength, sizeof(uint32_t), 1, fp);
	fwrite(&ss->loopramp, sizeof(uint8_t), 1, fp);

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
	fread(&ss->pitchshift, sizeof(uint8_t), 1, fp);
	if (major == 0 && minor < 83) fseek(fp, sizeof(uint16_t), SEEK_CUR);
	fread(ss->trim, sizeof(uint32_t), 2, fp);
	fread(ss->loop, sizeof(uint32_t), 2, fp);
	fread(&ss->volume, sizeof(adsr), 1, fp);
	fread(&ss->gain, sizeof(uint8_t), 1, fp);
	fread(&ss->flags, sizeof(uint8_t), 1, fp);
	fread(&ss->samplelength, sizeof(uint32_t), 1, fp);
	fread(&ss->loopramp, sizeof(uint8_t), 1, fp);

	if (ss->samplelength)
	{
		ss->sampledata = malloc(sizeof(short) * ss->samplelength);
		fread(ss->sampledata, sizeof(short), ss->samplelength, fp);
	}
}

void samplerInit(int index)
{
	t->f[index].indexc = 20;
	t->f[index].cellwidth = 45;
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
