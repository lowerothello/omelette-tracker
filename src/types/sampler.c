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
		default: w->fieldpointer = 0; break;
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
			w->fieldpointer = 1; break;
		default: w->fieldpointer = 0; break;
	}
}
void samplerEndFieldPointer(short index)
{ w->fieldpointer = 0; }

void inputSamplerHex(short index, instrument *iv, char value)
{
	switch (index)
	{
		case 1:  updateField(w->fieldpointer, (uint32_t *)&iv->c5rate, value); break;
		case 3:  updateFieldPush(&iv->samplerate, value); break;
		case 6:  updateField(w->fieldpointer, (uint32_t *)&iv->trim[0], value); iv->trim[0] = MIN(iv->trim[1], MIN(iv->trim[0], iv->length)); break;
		case 7:  updateField(w->fieldpointer, (uint32_t *)&iv->trim[1], value); iv->trim[1] = MAX(iv->trim[0], MIN(iv->trim[1], iv->length)); break;
		case 8:  updateField(w->fieldpointer, (uint32_t *)&iv->loop[0], value); iv->loop[0] = MIN(iv->loop[1], MIN(iv->loop[0], iv->length)); break;
		case 9:  updateField(w->fieldpointer, (uint32_t *)&iv->loop[1], value); iv->loop[1] = MAX(iv->loop[0], MIN(iv->loop[1], iv->length)); break;
		case 11: updateFieldPush(&iv->loopramp, value); break;
		case 13: updateFieldPush(&iv->volume.a, value); break;
		case 14: updateFieldPush(&iv->volume.d, value); break;
		case 15: updateFieldPush(&iv->volume.s, value); break;
		case 16: updateFieldPush(&iv->volume.r, value); break;
		case 17: updateFieldPush(&iv->gain, value); break;
		case 19: updateField(w->fieldpointer, (uint32_t *)&iv->cyclelength, value); break;
		case 20: updateFieldPush(&iv->pitchshift, value); break;
	}
	samplerIncFieldPointer(index);
}

void drawSampler(instrument *iv, uint8_t index, unsigned short x, unsigned short y, short *cursor, char adjust)
{
	printf("\033[%d;%dH [sampler] ", y-1, x+17);

	for (char i = 1; i < 13; i++)
	printf("\033[%d;%dH│", y+i, x+22);
	printf("\033[%d;%dH",  y+1, x+0);
	if (!iv->samplelength)
		printf("[     \033[1mNO SAMPLE\033[m     ]");
	else switch (iv->channels)
	{
		case 1:  printf("[   \033[1m%08x MONO\033[m   ]", iv->length); break;
		default: printf("[  \033[1m%08x STEREO\033[m  ]", iv->length); break;
	}
	printf("\033[%d;%dHC-5 rate   [%08x]",       y+2,  x+0, iv->c5rate);
	printf("\033[%d;%dHmono/rate     ",          y+3,  x+0); drawBit(iv->flags & S_FLAG_MONO); printf("[%02x]", iv->samplerate);
	printf("\033[%d;%dHinvert/8-bit   ",         y+4,  x+0); drawBit(iv->flags & S_FLAG_PHASE); drawBit(iv->flags & S_FLAG_8BIT);
	printf("\033[%d;%dH──────────────────────┤", y+6,  x+0);
	printf("\033[%d;%dH\033[1mAMPLIFIER\033[m",  y+7,  x+6);
	printf("\033[%d;%dHattack           [%02x]", y+8,  x+0, iv->volume.a);
	printf("\033[%d;%dHdecay            [%02x]", y+9,  x+0, iv->volume.d);
	printf("\033[%d;%dHsustain          [%02x]", y+10, x+0, iv->volume.s);
	printf("\033[%d;%dHrelease          [%02x]", y+11, x+0, iv->volume.r);
	printf("\033[%d;%dHgain             [%02x]", y+12, x+0, iv->gain);
	printf("\033[%d;%dH\033[1mTRIM        LOOP\033[m", y+1,  x+27);
	printf("\033[%d;%dH[%08x]  [%08x]",                y+2,  x+24, iv->trim[0], iv->loop[0]);
	printf("\033[%d;%dH[%08x]  [%08x]",                y+3,  x+24, iv->trim[1], iv->loop[1]);
	printf("\033[%d;%dH%02x->%02x      %02x->%02x",    y+4,  x+26,
			(uint8_t)((float)iv->trim[0] / (float)iv->length * 255),
			(uint8_t)((float)iv->trim[1] / (float)iv->length * 255),
			(uint8_t)((float)iv->loop[0] / (float)iv->length * 255),
			(uint8_t)((float)iv->loop[1] / (float)iv->length * 255));
	printf("\033[%d;%dHreverse playback   ",      y+5,  x+24);
	drawBit(iv->flags & S_FLAG_RPLAY);
	printf("\033[%d;%dHloop crossfade    [%02x]", y+6,  x+24, iv->loopramp);
	printf("\033[%d;%dHping-pong loop     ",      y+7,  x+24);
	drawBit(iv->flags & S_FLAG_PPLOOP);
	printf("\033[%d;%dH├───────────────────────", y+8,  x+22);
	printf("\033[%d;%dH\033[1mTIMESTRETCH\033[m", y+9,  x+30);
	printf("\033[%d;%dHstretch tempo      ",      y+10, x+24); drawBit(iv->flags & S_FLAG_TTEMPO);
	printf("\033[%d;%dHcycle length    [%04x]",   y+11, x+24, iv->cyclelength);
	printf("\033[%d;%dHpitch shift       [%02x]", y+12, x+24, iv->pitchshift);

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
	uint32_t temp;
	switch (index)
	{
		case 1:  incField(w->fieldpointer, &iv->c5rate, 0xffffffff); break;
		case 3:  if (w->fieldpointer) { if (iv->samplerate < 255 - 16) iv->samplerate+=16; else iv->samplerate = 255; } else if (iv->samplerate < 255) iv->samplerate++; break;
		case 6:  incField(w->fieldpointer, &iv->trim[0], 0xffffffff); iv->trim[0] = MIN(iv->trim[1], MIN(iv->trim[0], iv->length)); break;
		case 7:  incField(w->fieldpointer, &iv->trim[1], 0xffffffff); iv->trim[1] = MAX(iv->trim[0], MIN(iv->trim[1], iv->length)); break;
		case 8:  incField(w->fieldpointer, &iv->loop[0], 0xffffffff); iv->loop[0] = MIN(iv->loop[1], MIN(iv->loop[0], iv->length)); break;
		case 9:  incField(w->fieldpointer, &iv->loop[1], 0xffffffff); iv->loop[1] = MAX(iv->loop[0], MIN(iv->loop[1], iv->length)); break;
		case 11: if (w->fieldpointer) { if (iv->loopramp < 255 - 16) iv->loopramp+=16; else iv->loopramp = 255; } else if (iv->loopramp < 255) iv->loopramp++; break;
		case 13: if (w->fieldpointer) { if (iv->volume.a < 255 - 16) iv->volume.a+=16; else iv->volume.a = 255; } else if (iv->volume.a < 255) iv->volume.a++; break;
		case 14: if (w->fieldpointer) { if (iv->volume.d < 255 - 16) iv->volume.d+=16; else iv->volume.d = 255; } else if (iv->volume.d < 255) iv->volume.d++; break;
		case 15: if (w->fieldpointer) { if (iv->volume.s < 255 - 16) iv->volume.s+=16; else iv->volume.s = 255; } else if (iv->volume.s < 255) iv->volume.s++; break;
		case 16: if (w->fieldpointer) { if (iv->volume.r < 255 - 16) iv->volume.r+=16; else iv->volume.r = 255; } else if (iv->volume.r < 255) iv->volume.r++; break;
		case 17: if (w->fieldpointer) { if (iv->gain < 255 - 16) iv->gain+=16; else iv->gain = 255; } else if (iv->gain < 255) iv->gain++; break;
		case 19: temp = iv->cyclelength; incField(w->fieldpointer, &temp, 0xffff); iv->cyclelength = temp; break;
		case 20: if (w->fieldpointer) { if (iv->pitchshift < 255 - 16) iv->pitchshift+=16; else iv->pitchshift = 255; } else if (iv->pitchshift < 255) iv->pitchshift++; break;
	}
}
void samplerAdjustDown(instrument *iv, short index, char mouse)
{
	uint32_t temp;
	switch (index)
	{
		case 1:  decField(w->fieldpointer, &iv->c5rate); break;
		case 3:  if (w->fieldpointer) { if (iv->samplerate > 16) iv->samplerate-=16; else iv->samplerate = 0; } else if (iv->samplerate) iv->samplerate--; break;
		case 6:  decField(w->fieldpointer, &iv->trim[0]); iv->trim[0] = MIN(iv->trim[1], MIN(iv->trim[0], iv->length)); break;
		case 7:  decField(w->fieldpointer, &iv->trim[1]); iv->trim[1] = MAX(iv->trim[0], MIN(iv->trim[1], iv->length)); break;
		case 8:  decField(w->fieldpointer, &iv->loop[0]); iv->loop[0] = MIN(iv->loop[1], MIN(iv->loop[0], iv->length)); break;
		case 9:  decField(w->fieldpointer, &iv->loop[1]); iv->loop[1] = MAX(iv->loop[0], MIN(iv->loop[1], iv->length)); break;
		case 11: if (w->fieldpointer) { if (iv->loopramp > 16) iv->loopramp-=16; else iv->loopramp = 0; } else if (iv->loopramp) iv->loopramp--; break;
		case 13: if (w->fieldpointer) { if (iv->volume.a > 16) iv->volume.a-=16; else iv->volume.a = 0; } else if (iv->volume.a) iv->volume.a--; break;
		case 14: if (w->fieldpointer) { if (iv->volume.d > 16) iv->volume.d-=16; else iv->volume.d = 0; } else if (iv->volume.d) iv->volume.d--; break;
		case 15: if (w->fieldpointer) { if (iv->volume.s > 16) iv->volume.s-=16; else iv->volume.s = 0; } else if (iv->volume.s) iv->volume.s--; break;
		case 16: if (w->fieldpointer) { if (iv->volume.r > 16) iv->volume.r-=16; else iv->volume.r = 0; } else if (iv->volume.r) iv->volume.r--; break;
		case 17: if (w->fieldpointer) { if (iv->gain > 16) iv->gain-=16; else iv->gain = 0; } else if (iv->gain) iv->gain--; break;
		case 19: temp = iv->cyclelength; decField(w->fieldpointer, &temp); iv->cyclelength = temp; break;
		case 20: if (w->fieldpointer) { if (iv->pitchshift > 16) iv->pitchshift-=16; else iv->pitchshift = 0; } else if (iv->pitchshift) iv->pitchshift--; break;
	}
}

void samplerAdjustLeft(instrument *iv, short index, char mouse)
{ if (!mouse) samplerDecFieldPointer(index); }

void samplerAdjustRight(instrument *iv, short index, char mouse)
{ if (!mouse) samplerIncFieldPointer(index); }

void loadSample(uint8_t index, char *path)
{
	instrument *iv = s->instrumentv[s->instrumenti[index]];
	SF_INFO sfinfo;
	short *sampledata = _loadSample(path, &sfinfo);
	if (!sampledata)
	{
		strcpy(w->command.error, "failed to load sample, out of memory");
		return;
	}

	/* unload any present sample data */
	if (iv->samplelength > 0)
		free(iv->sampledata);
	iv->sampledata = sampledata;
	iv->samplelength = sfinfo.frames * sfinfo.channels;
	iv->channels = sfinfo.channels;
	iv->length = sfinfo.frames;
	iv->c5rate = sfinfo.samplerate;
	iv->trim[0] = 0;
	iv->trim[1] = sfinfo.frames;
	iv->loop[0] = 0;
	iv->loop[1] = 0;
};
int exportSample(uint8_t index, char *path)
{
	instrument *iv = s->instrumentv[s->instrumenti[index]];
	if (!iv->sampledata) return 1; /* no sample data */

	SNDFILE *sndfile;
	SF_INFO sfinfo;
	memset(&sfinfo, 0, sizeof(sfinfo));

	sfinfo.samplerate = iv->c5rate;
	sfinfo.frames = iv->length;
	sfinfo.channels = iv->channels;

	sfinfo.format = SF_FORMAT_WAV | SF_FORMAT_PCM_16;
	sndfile = sf_open(path, SFM_WRITE, &sfinfo);
	if (sndfile == NULL) return 1;

	/* write the sample data to disk */
	sf_writef_short(sndfile, iv->sampledata, iv->length);
	sf_close(sndfile);
	return 0;
};


void samplerApplyTrimming(instrument *iv)
{
	pushInstrumentHistoryIfNew(iv);
	if (iv->samplelength > 0)
	{
		uint32_t newlen
			= MAX(iv->trim[0], iv->trim[1])
			- MIN(iv->trim[0], iv->trim[1]);

		/* malloc a new buffer */
		short *sampledata = malloc(sizeof(short) * newlen * iv->channels);
		if (sampledata == NULL)
		{
			strcpy(w->command.error, "failed to apply trim, out of memory");
			return;
		}

		uint32_t startOffset = MIN(iv->trim[0], iv->trim[1]);
		memcpy(sampledata,
				iv->sampledata+(sizeof(short) * startOffset),
				sizeof(short) * newlen * iv->channels);

		free(iv->sampledata); iv->sampledata = NULL;
		iv->sampledata = sampledata;
		iv->samplelength = newlen * iv->channels;
		iv->length = newlen;
		iv->trim[0] = iv->trim[0] - startOffset;
		iv->trim[1] = iv->trim[1] - startOffset;
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
	switch (w->instrumentindex)
	{
		case 0:
			switch (*input)
			{
				case 10: case 13: /* return */
					w->popup = 2;
					w->fyoffset = 0; /* this can still be set on edge cases */
					w->filebrowserCallback = &samplerLoadCallback;
					*input = 0; /* don't reprocess */
					redraw(); break;
				// case 't': /* apply trimming */
					samplerApplyTrimming(iv);
					redraw(); break;
				case 'e': /* export */
					setCommand(&w->command, &samplerExportCallback, NULL, NULL, 0, "File name: ", "");
					w->mode = 255;
					redraw(); break;
				case 'w': /* waveform */
					w->popup = 4;
					w->fyoffset = 0; /* this can still be set on edge cases */
					w->waveformoffset = 0;
					w->waveformwidth = iv->length;
					resizeWaveform();
					redraw(); break;
			} break;
		default:
			switch (*input)
			{
				case 10: case 13: /* return */
					switch (w->instrumentindex)
					{
						case 2:  /* mono              */ iv->flags ^= S_FLAG_MONO; redraw(); *input = 0; break;
						case 4:  /* invert phase      */ iv->flags ^= S_FLAG_PHASE; redraw(); *input = 0; break;
						case 5:  /* 8-bit             */ iv->flags ^= S_FLAG_8BIT; redraw(); *input = 0; break;
						case 18: /* timestretch tempo */ iv->flags ^= S_FLAG_TTEMPO; redraw(); *input = 0; break;
						case 10: /* reverse playback  */ iv->flags ^= S_FLAG_RPLAY; redraw(); *input = 0; break;
						case 12: /* ping-pong loop    */ iv->flags ^= S_FLAG_PPLOOP; redraw(); *input = 0; break;
					} break;
				case 1: /* ^a */
					switch (w->instrumentindex)
					{
						case 1:  iv->c5rate++; break;
						case 3:  iv->samplerate++; break;
						case 6:  if (iv->trim[0] == iv->length) iv->trim[0] = 0; else iv->trim[0]++; iv->trim[0] = MIN(iv->trim[1], MIN(iv->trim[0], iv->length)); break;
						case 7:  if (iv->trim[1] == iv->length) iv->trim[1] = 0; else iv->trim[1]++; iv->trim[1] = MAX(iv->trim[0], MIN(iv->trim[1], iv->length)); break;
						case 8:  if (iv->loop[0] == iv->length) iv->loop[0] = 0; else iv->loop[0]++; iv->loop[0] = MIN(iv->loop[1], MIN(iv->loop[0], iv->length)); break;
						case 9:  if (iv->loop[1] == iv->length) iv->loop[1] = 0; else iv->loop[1]++; iv->loop[1] = MAX(iv->loop[0], MIN(iv->loop[1], iv->length)); break;
						case 11: iv->loopramp++; break;
						case 13: iv->volume.a++; break;
						case 14: iv->volume.d++; break;
						case 15: iv->volume.s++; break;
						case 16: iv->volume.r++; break;
						case 17: iv->gain++; break;
						case 19: iv->cyclelength++; break;
						case 20: iv->pitchshift++; break;
					} break;
				case 24: /* ^x */
					switch (w->instrumentindex)
					{
						case 1:  iv->c5rate--; break;
						case 3:  iv->samplerate--; break;
						case 6:  iv->trim[0]--; iv->trim[0] = MIN(iv->trim[1], MIN(iv->trim[0], iv->length)); break;
						case 7:  iv->trim[1]--; iv->trim[1] = MAX(iv->trim[0], MIN(iv->trim[1], iv->length)); break;
						case 8:  iv->loop[0]--; iv->loop[0] = MIN(iv->loop[1], MIN(iv->loop[0], iv->length)); break;
						case 9:  iv->loop[1]--; iv->loop[1] = MAX(iv->loop[0], MIN(iv->loop[1], iv->length)); break;
						case 11: iv->loopramp--; break;
						case 13: iv->volume.a--; break;
						case 14: iv->volume.d--; break;
						case 15: iv->volume.s--; break;
						case 16: iv->volume.r--; break;
						case 17: iv->gain--; break;
						case 19: iv->cyclelength--; break;
						case 20: iv->pitchshift--; break;
					} break;
				case '0':           inputSamplerHex(w->instrumentindex, iv, 0);  break;
				case '1':           inputSamplerHex(w->instrumentindex, iv, 1);  break;
				case '2':           inputSamplerHex(w->instrumentindex, iv, 2);  break;
				case '3':           inputSamplerHex(w->instrumentindex, iv, 3);  break;
				case '4':           inputSamplerHex(w->instrumentindex, iv, 4);  break;
				case '5':           inputSamplerHex(w->instrumentindex, iv, 5);  break;
				case '6':           inputSamplerHex(w->instrumentindex, iv, 6);  break;
				case '7':           inputSamplerHex(w->instrumentindex, iv, 7);  break;
				case '8':           inputSamplerHex(w->instrumentindex, iv, 8);  break;
				case '9':           inputSamplerHex(w->instrumentindex, iv, 9);  break;
				case 'A': case 'a': inputSamplerHex(w->instrumentindex, iv, 10); break;
				case 'B': case 'b': inputSamplerHex(w->instrumentindex, iv, 11); break;
				case 'C': case 'c': inputSamplerHex(w->instrumentindex, iv, 12); break;
				case 'D': case 'd': inputSamplerHex(w->instrumentindex, iv, 13); break;
				case 'E': case 'e': inputSamplerHex(w->instrumentindex, iv, 14); break;
				case 'F': case 'f': inputSamplerHex(w->instrumentindex, iv, 15); break;
			} redraw(); break;
	}
}

void samplerMouseToIndex(int y, int x, int button, short *index)
{
	instrument *iv = s->instrumentv[s->instrumenti[w->instrument]];
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
						} break;
					case 3: *index = 1;
						if (x < 12)          w->fieldpointer = 7;
						else if (x > 12 + 7) w->fieldpointer = 0;
						else w->fieldpointer = (x - 12)*-1 + 7;
						break;
					case 4:
						if (x < 17) { *index = 2; iv->flags ^= S_FLAG_MONO; w->fieldpointer = 0; }
						else        { *index = 3; if (x < 19) w->fieldpointer = 1; else w->fieldpointer = 0; }
						break;
					case 5: case 6: case 7:
						if (x < 18) { *index = 4; iv->flags ^= S_FLAG_PHASE; }
						else        { *index = 5; iv->flags ^= S_FLAG_8BIT; }
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
					case 6: *index = 10; w->fieldpointer = 0; iv->flags ^= S_FLAG_RPLAY; break;
					case 7: *index = 11; if (x < 44) w->fieldpointer = 1; else w->fieldpointer = 0; break;
					case 8: case 9: *index = 12; w->fieldpointer = 0; iv->flags ^= S_FLAG_PPLOOP; break;
					case 10: case 11: *index = 18; w->fieldpointer = 0; iv->flags ^= S_FLAG_TTEMPO; break;
					case 12: *index = 19;
						if (x < 41)          w->fieldpointer = 3;
						else if (x > 41 + 3) w->fieldpointer = 0;
						else w->fieldpointer = (x - 41)*-1 + 3;
						break;
					default: *index = 20; if (x < 44) w->fieldpointer = 1; else w->fieldpointer = 0; break;
				} break;
	}
}

void getSample(uint32_t p, instrument *iv, float *l, float *r)
{
	if (iv->flags & S_FLAG_8BIT)
	{
		if (iv->flags & S_FLAG_RPLAY)
		{
			/* listchars */       *l += (signed char)(iv->sampledata[iv->trim[1]*2 - p * iv->channels+0]>>8)*DIVCHAR;
			if (iv->channels > 1) *r += (signed char)(iv->sampledata[iv->trim[1]*2 - p * iv->channels+1]>>8)*DIVCHAR;
			else                  *r += (signed char)(iv->sampledata[iv->trim[1]*2 - p * iv->channels+0]>>8)*DIVCHAR;
		} else
		{
			/* listchars */       *l += (signed char)(iv->sampledata[p * iv->channels+0]>>8)*DIVCHAR;
			if (iv->channels > 1) *r += (signed char)(iv->sampledata[p * iv->channels+1]>>8)*DIVCHAR;
			else                  *r += (signed char)(iv->sampledata[p * iv->channels+0]>>8)*DIVCHAR;
		}
	} else
	{
		if (iv->flags & S_FLAG_RPLAY)
		{
			/* listchars */       *l += iv->sampledata[iv->trim[1]*2 - p * iv->channels+0] * DIVSHRT;
			if (iv->channels > 1) *r += iv->sampledata[iv->trim[1]*2 - p * iv->channels+1] * DIVSHRT;
			else                  *r += iv->sampledata[iv->trim[1]*2 - p * iv->channels+0] * DIVSHRT;
		} else
		{
			/* listchars */       *l += iv->sampledata[p * iv->channels+0] * DIVSHRT;
			if (iv->channels > 1) *r += iv->sampledata[p * iv->channels+1] * DIVSHRT;
			else                  *r += iv->sampledata[p * iv->channels+0] * DIVSHRT;
		}
	}
}

void getSampleLoopRamp(uint32_t p, uint32_t q, float lerp, instrument *iv, float *l, float *r)
{
	if (iv->flags & S_FLAG_8BIT)
	{
		if (iv->flags & S_FLAG_RPLAY)
		{
			/* listchars */       *l += (signed char)(iv->sampledata[iv->trim[1]*2 - p * iv->channels+0]>>8)*DIVCHAR * (1.0 - lerp) + (signed char)(iv->sampledata[iv->trim[1]*2 - q * iv->channels+0]>>8)*DIVCHAR * lerp;
			if (iv->channels > 1) *r += (signed char)(iv->sampledata[iv->trim[1]*2 - p * iv->channels+1]>>8)*DIVCHAR * (1.0 - lerp) + (signed char)(iv->sampledata[iv->trim[1]*2 - q * iv->channels+1]>>8)*DIVCHAR * lerp;
			else                  *r += (signed char)(iv->sampledata[iv->trim[1]*2 - p * iv->channels+0]>>8)*DIVCHAR * (1.0 - lerp) + (signed char)(iv->sampledata[iv->trim[1]*2 - q * iv->channels+0]>>8)*DIVCHAR * lerp;
		} else
		{
			/* listchars */       *l += (signed char)(iv->sampledata[p * iv->channels+0]>>8)*DIVCHAR * (1.0 - lerp) + (signed char)(iv->sampledata[q * iv->channels+0]>>8)*DIVCHAR * lerp;
			if (iv->channels > 1) *r += (signed char)(iv->sampledata[p * iv->channels+1]>>8)*DIVCHAR * (1.0 - lerp) + (signed char)(iv->sampledata[q * iv->channels+1]>>8)*DIVCHAR * lerp;
			else                  *r += (signed char)(iv->sampledata[p * iv->channels+0]>>8)*DIVCHAR * (1.0 - lerp) + (signed char)(iv->sampledata[q * iv->channels+0]>>8)*DIVCHAR * lerp;
		}
	} else
	{
		if (iv->flags & S_FLAG_RPLAY)
		{
			/* listchars */       *l += iv->sampledata[iv->trim[1]*2 - p * iv->channels+0] * DIVSHRT * (1.0 - lerp) + iv->sampledata[iv->trim[1]*2 - q * iv->channels+0] * DIVSHRT * lerp;
			if (iv->channels > 1) *r += iv->sampledata[iv->trim[1]*2 - p * iv->channels+1] * DIVSHRT * (1.0 - lerp) + iv->sampledata[iv->trim[1]*2 - q * iv->channels+1] * DIVSHRT * lerp;
			else                  *r += iv->sampledata[iv->trim[1]*2 - p * iv->channels+0] * DIVSHRT * (1.0 - lerp) + iv->sampledata[iv->trim[1]*2 - q * iv->channels+0] * DIVSHRT * lerp;
		} else
		{
			/* listchars */       *l += iv->sampledata[p * iv->channels+0] * DIVSHRT * (1.0 - lerp) + iv->sampledata[q * iv->channels+0] * DIVSHRT * lerp;
			if (iv->channels > 1) *r += iv->sampledata[p * iv->channels+1] * DIVSHRT * (1.0 - lerp) + iv->sampledata[q * iv->channels+1] * DIVSHRT * lerp;
			else                  *r += iv->sampledata[p * iv->channels+0] * DIVSHRT * (1.0 - lerp) + iv->sampledata[q * iv->channels+0] * DIVSHRT * lerp;
		}
	}
}

/* clamps within range and loop, returns output samples */
void trimloop(uint32_t pitchedpointer, uint32_t pointer,
		channel *cv, instrument *iv, sample_t *l, sample_t *r)
{
	uint32_t p = pitchedpointer + iv->trim[0] + cv->pointeroffset;

	if (iv->loop[1])
	{ /* if there is a loop range */
		if (iv->loop[0] == iv->loop[1] && p >= iv->loop[1])
		{
			*l = *r = 0.0f;
			cv->r.note = 0;
			return;
		}

		if (iv->flags & S_FLAG_PPLOOP)
		{ /* ping-pong loop */
			uint32_t looplength = iv->loop[1] - iv->loop[0];
			if (p >= iv->loop[1])
			{
				uint32_t i = (p - iv->loop[1])/looplength;
				if (i % 2 == 0) /* backwards */ p = iv->loop[1] - (p - iv->loop[1])%looplength;
				else            /* forwards  */ p = iv->loop[0] + (p - iv->loop[1])%looplength;
			}

			/* always point to the left channel */
			if (iv->flags & S_FLAG_RPLAY) p -= (iv->trim[1]*2 - p) % iv->channels;
			else                          p -= p % iv->channels;
			getSample(p, iv, l, r);
		} else
		{ /* crossfaded forwards loop */
			uint32_t looprampmax = MIN(samplerate/1000 * LOOP_RAMP_MS, (iv->loop[1] - iv->loop[0]) * 0.5) * (iv->loopramp*DIV255);
			uint32_t looplength = iv->loop[1] - iv->loop[0] - looprampmax;
			if (p > iv->loop[1]) p = iv->loop[0] + looprampmax + (p - iv->loop[1])%looplength;

			/* always point to the left channel */
			if (iv->flags & S_FLAG_RPLAY) p -= (iv->trim[1]*2 - p) % iv->channels;
			else                          p -= p % iv->channels;

			if (p > iv->loop[1] - looprampmax)
			{
				float lerp = (p - iv->loop[1] + looprampmax) / (float)looprampmax;
				uint32_t ramppointer = (p - looplength);
				/* always point to the left channel */
				if (iv->flags & S_FLAG_RPLAY) ramppointer -= (iv->trim[1]*2 - ramppointer) % iv->channels;
				else                          ramppointer -= ramppointer % iv->channels;
				getSampleLoopRamp(p, ramppointer, lerp, iv, l, r);
			} else getSample(p, iv, l, r);
		}
	}

	/* trigger the release envelope */
	if (((iv->loop[1] && iv->trim[1] < iv->loop[1]) || !iv->loop[1])
			&& p > iv->trim[1] - (iv->volume.r * ENVELOPE_RELEASE * samplerate)
			&& !cv->releasepointer)
		cv->releasepointer = pointer;

	if (!(iv->loop[0] || iv->loop[1]))
	{
		/* always point to the left channel */
		if (iv->flags & S_FLAG_RPLAY) p -= (iv->trim[1]*2 - p) % iv->channels;
		else                          p -= p % iv->channels;

		if (p < iv->length) getSample(p, iv, l, r);
	}

	/* sample gain */
	*l *= iv->gain*DIV64; *r *= iv->gain*DIV64;
}

uint32_t calcDecimate(instrument *iv, uint8_t decimate, uint32_t pointer)
{
	float d = 1.0f + (1.0f - decimate*DIV255) * 20;

	if (iv->samplerate == 255) return pointer;
	else                       return (uint32_t)(pointer / d) * d;
}

/* must be realtime safe                     */
/* must reasonably accept arbitrary pointers */
void samplerProcess(instrument *iv, channel *cv, uint32_t pointer, float *l, float *r)
{
	uint32_t ramppos, pointersnap;
	uint16_t cyclelength;

	float gain = adsrEnvelope(iv->volume, 0.0, pointer, cv->releasepointer);
	if (pointer > (iv->volume.a+ENVELOPE_ATTACK_MIN) * ENVELOPE_ATTACK * samplerate
			&& gain == 0.0f) /* sound has fully finished */
		cv->r.note = 0;
	else if (iv->length > 0)
	{
		cyclelength = MAX(iv->cyclelength, 1);
		pointersnap = pointer % cyclelength;

		if (cv->reverse) ramppos = cyclelength;
		else             ramppos = 0;
		if (pointersnap == ramppos)
		{ // first sample of a cycle
			cv->localstretchrampmax = MIN(cyclelength, stretchrampmax);
			if (pointer == 0) cv->stretchrampindex = cv->localstretchrampmax;
			else cv->stretchrampindex = 0;
		}

		if (iv->flags & S_FLAG_TTEMPO)
		{ /* time stretch */
			/* trim/loop */
			trimloop(calcDecimate(iv, iv->samplerate,
					pointer - pointersnap + (pointersnap
						* powf(M_12_ROOT_2, (short)cv->r.note - C5 + cv->finetune))
						* ((float)iv->c5rate / (float)samplerate)),
					pointer, cv, iv, l, r);

			if (cv->stretchrampindex < cv->localstretchrampmax)
			{
				float rl = 0.0f; float rr = 0.0f;
				trimloop(calcDecimate(iv, iv->samplerate,
						pointer - pointersnap - cyclelength + ((cyclelength + cv->stretchrampindex)
							* powf(M_12_ROOT_2, (short)cv->r.note - C5 + cv->finetune)
							* ((float)iv->c5rate / (float)samplerate))),
						pointer, cv, iv, &rl, &rr);

				float gain = (float)cv->stretchrampindex / (float)cv->localstretchrampmax;
				*l = *l * gain + rl * (1.0f - gain);
				*r = *r * gain + rr * (1.0f - gain);
				cv->stretchrampindex++;
			}
		} else
		{ /* pitch shift */
			/* trim/loop */
			trimloop(calcDecimate(iv, iv->samplerate,
					(float)(pointer - pointersnap + (pointersnap * (float)(iv->pitchshift*DIV128)))
						* powf(M_12_ROOT_2, (short)cv->r.note - C5 + cv->finetune)
						* ((float)iv->c5rate / (float)samplerate)),
					pointer, cv, iv, l, r);

			if (cv->stretchrampindex < cv->localstretchrampmax)
			{
				float rl = 0.0f; float rr = 0.0f;
				trimloop(calcDecimate(iv, iv->samplerate,
						(float)(pointer - pointersnap - cyclelength + ((cyclelength + cv->stretchrampindex) * (float)(iv->pitchshift*DIV128)))
							* powf(M_12_ROOT_2, (short)cv->r.note - C5 + cv->finetune)
							* ((float)iv->c5rate / (float)samplerate)),
						pointer, cv, iv, &rl, &rr);

				float gain = (float)cv->stretchrampindex / (float)cv->localstretchrampmax;
				*l = *l * gain + rl * (1.0f - gain);
				*r = *r * gain + rr * (1.0f - gain);
				cv->stretchrampindex++;
			}
		}
	} else cv->r.note = 0;

	if (iv->flags & S_FLAG_MONO)  { *l = (*l + *r) / 2.0f; *r = *l; }
	if (iv->flags & S_FLAG_PHASE) { *l *= -1; *r *= -1; }

	*l *= gain; *r *= gain;
}

const unsigned short samplercellwidth = 45;
const unsigned short samplerindexc = 20;
