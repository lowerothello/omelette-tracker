typedef struct
{
	uint16_t  stretchrampindex;  /* progress through the stretch ramp buffer, stretchrampmax if not ramping */
	uint16_t  stretchrampmax;    /* length of the stretch ramp buffer */
	sample_t *stretchrampbuffer; /* raw samples to ramp out */
	uint32_t  cycleoffset;
} sampler_channel;


void samplerIncFieldPointer(short index)
{
	w->fieldpointer++;
	switch (index)
	{
		case 1: case 14: case 15: case 16: case 17:
			if (w->fieldpointer > 7) w->fieldpointer = 0;
			break;
		case 8:
			if (w->fieldpointer > 3) w->fieldpointer = 0;
			break;
		default:
			w->fieldpointer = 0;
			break;
	}
}
void samplerDecFieldPointer(short index)
{
	w->fieldpointer--;
	if (w->fieldpointer < 0) switch (index)
		{
			case 1: case 14: case 15: case 16: case 17:
				w->fieldpointer = 7;
				break;
			case 8:
				w->fieldpointer = 3;
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
		case 1: case 14: case 15: case 16: case 17:
			w->fieldpointer = 7;
			break;
		case 8:
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
		case 2:  updateFieldPush(&ss->samplerate, value); break;
		case 8:  updateField(w->fieldpointer, 4, (uint32_t *)&ss->cyclelength, value); break;
		case 10: updateFieldPush(&ss->volume.a, value); break;
		case 11: updateFieldPush(&ss->volume.d, value); break;
		case 12: updateFieldPush(&ss->volume.s, value); break;
		case 13: updateFieldPush(&ss->volume.r, value); break;
		case 14: updateField(w->fieldpointer, 8, (uint32_t *)&ss->trim[0], value); if (ss->trim[0] > ss->length) ss->trim[0] = ss->length; break;
		case 15: updateField(w->fieldpointer, 8, (uint32_t *)&ss->trim[1], value); if (ss->trim[1] > ss->length) ss->trim[1] = ss->length; break;
		case 16: updateField(w->fieldpointer, 8, (uint32_t *)&ss->loop[0], value); if (ss->loop[0] > ss->length) ss->loop[0] = ss->length; break;
		case 17: updateField(w->fieldpointer, 8, (uint32_t *)&ss->loop[1], value); if (ss->loop[1] > ss->length) ss->loop[1] = ss->length; break;
	}
	samplerIncFieldPointer(index);
}

void drawBit(char true)
{
	if (true) printf("[X]");
	else      printf("[ ]");
}
void drawSampler(instrument *iv, uint8_t index, unsigned short x, unsigned short y, short *cursor, char adjust)
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

	for (char i = 1; i < 12; i++)
		printf("\033[%d;%dH│", y+i, x+38);
	printf("\033[%d;%dH\033[1mGENERAL\033[m",      y+1,  x+22);
	printf("\033[%d;%dHC-5 rate     [%08x]",       y+2,  x+14, ss->c5rate);
	printf("\033[%d;%dHsamplerate         [%02x]", y+3,  x+14, ss->samplerate);
	printf("\033[%d;%dHmono/8-bit       ",         y+4,  x+14);
	drawBit(ss->attributes & 0b1000); drawBit(ss->attributes & 0b10000);
	printf("\033[%d;%dHunsigned            ",      y+5,  x+14);
	drawBit(ss->attributes & 0b100000);
	printf("\033[%d;%dHinvert phase        ",      y+6,  x+14);
	drawBit(ss->attributes & 0b1000000);
	printf("\033[%d;%dH────────────────────────┤", y+7,  x+14);
	printf("\033[%d;%dH\033[1mTIMESTRETCH\033[m",         y+8,  x+20);
	printf("\033[%d;%dHpersistent tempo    ",      y+9,  x+14);
	drawBit(ss->attributes & 0b1);
	printf("\033[%d;%dHcycle length     [%04x]",   y+10, x+14, ss->cyclelength);
	printf("\033[%d;%dHquantize cycles     ",      y+11, x+14);
	drawBit(ss->attributes & 0b10);
	printf("\033[%d;%dH\033[1mAMPLIFIER\033[m",       y+1,  x+46);
	printf("\033[%d;%dHattack           [%02x]",      y+2,  x+40, ss->volume.a);
	printf("\033[%d;%dHdecay            [%02x]",      y+3,  x+40, ss->volume.d);
	printf("\033[%d;%dHsustain          [%02x]",      y+4,  x+40, ss->volume.s);
	printf("\033[%d;%dHrelease          [%02x]",      y+5,  x+40, ss->volume.r);
	printf("\033[%d;%dH├──────────────────────",      y+6,  x+38);
	printf("\033[%d;%dH\033[1mTRIM       LOOP\033[m", y+7,  x+43);
	printf("\033[%d;%dH[%08x] [%08x]",  y+8,  x+40, ss->trim[0], ss->loop[0]);
	printf("\033[%d;%dH[%08x] [%08x]",  y+9,  x+40, ss->trim[1], ss->loop[1]);
	printf("\033[%d;%dH%02x->%02x     %02x->%02x",    y+10, x+42,
			(uint8_t)((float)ss->trim[0] / (float)ss->length * 255),
			(uint8_t)((float)ss->trim[1] / (float)ss->length * 255),
			(uint8_t)((float)ss->loop[0] / (float)ss->length * 255),
			(uint8_t)((float)ss->loop[1] / (float)ss->length * 255));
	printf("\033[%d;%dHloop ramping      ",           y+11, x+40);
	drawBit(ss->attributes & 0b100);

	if (w->instrumentrecv == INST_REC_LOCK_CONT)
		printf("\033[%d;%dHREC", y, x + 1);

	switch (*cursor)
	{
		case 0:  printf("\033[%d;%dH", y-1,  x+2 + sampletitleoffset); break;
		case 1:  printf("\033[%d;%dH", y+2,  x+28 + w->fieldpointer); break;
		case 2:  printf("\033[%d;%dH", y+3,  x+35); break;
		case 3:  printf("\033[%d;%dH", y+4,  x+32); break;
		case 4:  printf("\033[%d;%dH", y+4,  x+35); break;
		case 5:  printf("\033[%d;%dH", y+5,  x+35); break;
		case 6:  printf("\033[%d;%dH", y+6,  x+35); break;
		case 7:  printf("\033[%d;%dH", y+9,  x+35); break;
		case 8:  printf("\033[%d;%dH", y+10, x+32 + w->fieldpointer); break;
		case 9:  printf("\033[%d;%dH", y+11, x+35); break;
		case 10: printf("\033[%d;%dH", y+2,  x+59); break;
		case 11: printf("\033[%d;%dH", y+3,  x+59); break;
		case 12: printf("\033[%d;%dH", y+4,  x+59); break;
		case 13: printf("\033[%d;%dH", y+5,  x+59); break;
		case 14: printf("\033[%d;%dH", y+8,  x+41 + w->fieldpointer); break;
		case 15: printf("\033[%d;%dH", y+9,  x+41 + w->fieldpointer); break;
		case 16: printf("\033[%d;%dH", y+8,  x+52 + w->fieldpointer); break;
		case 17: printf("\033[%d;%dH", y+9,  x+52 + w->fieldpointer); break;
		case 18: printf("\033[%d;%dH", y+11, x+59); break;
	}
}

void samplerAdjustUp(instrument *iv, short index)
{
	sampler_state *ss = iv->state[iv->type];
	switch (index)
	{
		case 1: ss->c5rate = ss->c5rate * powf(M_12_ROOT_2, 1); break;
		case 2: if (w->fieldpointer) ss->samplerate+=16; else ss->samplerate++; break;
		case 8: ss->cyclelength += 0x20; break;
		case 10: if (w->fieldpointer) ss->volume.a+=16; else ss->volume.a++; break;
		case 11: if (w->fieldpointer) ss->volume.d+=16; else ss->volume.d++; break;
		case 12: if (w->fieldpointer) ss->volume.s+=16; else ss->volume.s++; break;
		case 13: if (w->fieldpointer) ss->volume.r+=16; else ss->volume.r++; break;
		case 14:
			ss->trim[0] += MAX(ss->length / 50.0, 1);
			if (ss->trim[0] > ss->length) ss->trim[0] = ss->length;
			break;
		case 15:
			ss->trim[1] += MAX(ss->length / 50.0, 1);
			if (ss->trim[1] > ss->length) ss->trim[1] = ss->length;
			break;
		case 16:
			ss->loop[0] += MAX(ss->length / 50.0, 1);
			if (ss->loop[0] > ss->length) ss->loop[0] = ss->length;
			break;
		case 17:
			ss->loop[1] += MAX(ss->length / 50.0, 1);
			if (ss->loop[1] > ss->length) ss->loop[1] = ss->length;
			break;
	}
}
void samplerAdjustDown(instrument *iv, short index)
{
	sampler_state *ss = iv->state[iv->type];
	uint32_t oldpos;
	switch (index)
	{
		case 1: ss->c5rate = ss->c5rate * powf(M_12_ROOT_2, -1); break;
		case 2: if (w->fieldpointer) ss->samplerate-=16; else ss->samplerate--; break;
		case 8: ss->cyclelength -= 0x20; break;
		case 10: if (w->fieldpointer) ss->volume.a-=16; else ss->volume.a--; break;
		case 11: if (w->fieldpointer) ss->volume.d-=16; else ss->volume.d--; break;
		case 12: if (w->fieldpointer) ss->volume.s-=16; else ss->volume.s--; break;
		case 13: if (w->fieldpointer) ss->volume.r-=16; else ss->volume.r--; break;
		case 14:
			oldpos = ss->trim[0];
			ss->trim[0] -= MAX(ss->length / 50.0, 1);
			if (ss->trim[0] > oldpos) ss->trim[0] = 0;
			break;
		case 15:
			oldpos = ss->trim[1];
			ss->trim[1] -= MAX(ss->length / 50.0, 1);
			if (ss->trim[1] > oldpos) ss->trim[1] = 0;
			break;
		case 16:
			oldpos = ss->loop[0];
			ss->loop[0] -= MAX(ss->length / 50.0, 1);
			if (ss->loop[0] > oldpos) ss->loop[0] = 0;
			break;
		case 17:
			oldpos = ss->loop[1];
			ss->loop[1] -= MAX(ss->length / 50.0, 1);
			if (ss->loop[1] > oldpos) ss->loop[1] = 0;
			break;
	}
}
void samplerAdjustLeft(instrument *iv, short index)
{
	sampler_state *ss = iv->state[iv->type];
	uint32_t oldpos;
	switch (index)
	{
		case 1: ss->c5rate = ss->c5rate * powf(M_12_ROOT_2, -0.2); break;
		case 14:
			oldpos = ss->trim[0];
			ss->trim[0] -= MAX(ss->length / 1000.0, 1);
			if (ss->trim[0] > oldpos) ss->trim[0] = 0;
			break;
		case 15:
			oldpos = ss->trim[1];
			ss->trim[1] -= MAX(ss->length / 1000.0, 1);
			if (ss->trim[1] > oldpos) ss->trim[1] = 0;
			break;
		case 16:
			oldpos = ss->loop[0];
			ss->loop[0] -= MAX(ss->length / 1000.0, 1);
			if (ss->loop[0] > oldpos) ss->loop[0] = 0;
			break;
		case 17:
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
		case 14:
			ss->trim[0] += MAX(ss->length / 1000.0, 1);
			if (ss->trim[0] > ss->length) ss->trim[0] = ss->length;
			break;
		case 15:
			ss->trim[1] += MAX(ss->length / 1000.0, 1);
			if (ss->trim[1] > ss->length) ss->trim[1] = ss->length;
			break;
		case 16:
			ss->loop[0] += MAX(ss->length / 1000.0, 1);
			if (ss->loop[0] > ss->length) ss->loop[0] = ss->length;
			break;
		case 17:
			ss->loop[1] += MAX(ss->length / 1000.0, 1);
			if (ss->loop[1] > ss->length) ss->loop[1] = ss->length;
			break;
	}
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
				case 'd': /* decimate / resample */
					if (iv->samplelength > 0)
					{
						char buffer[COMMAND_LENGTH];
						snprintf(buffer, COMMAND_LENGTH, "%d",
							ss->c5rate);
						setCommand(&w->command, &samplerResampleCallback, NULL, NULL, 0, "New C-5 rate: ", buffer);
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
						case 7: /* persistent tempo */
							ss->attributes ^= 0b1;
							redraw(); *input = 0;
							break;
						case 9: /* quantize cycles */
							ss->attributes ^= 0b10;
							redraw(); *input = 0;
							break;
						case 18: /* loop ramping */
							ss->attributes ^= 0b100;
							redraw(); *input = 0;
							break;
						case 3: /* mono */
							ss->attributes ^= 0b1000;
							redraw(); *input = 0;
							break;
						case 4: /* 8-bit */
							ss->attributes ^= 0b10000;
							redraw(); *input = 0;
							break;
						case 5: /* unsigned */
							ss->attributes ^= 0b100000;
							redraw(); *input = 0;
							break;
						case 6: /* invert phase */
							ss->attributes ^= 0b1000000;
							redraw(); *input = 0;
							break;
					}
					break;
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
			if (button == BUTTON3)
			{
				previewNote(0, 255, w->channel, 1);
				w->popup = 2;
				w->instrumentindex = 0;
				w->fyoffset = 0; /* this can still be set on edge cases */
			} break;
		default:
			if (x < 38)
			{
				switch (y)
				{
					case 2: case 3: *index = 1;
						if (x < 28)          w->fieldpointer = 0;
						else if (x > 28 + 7) w->fieldpointer = 7;
						else w->fieldpointer = x - 28;
						break;
					case 4: *index = 2;
						if (x < 35) w->fieldpointer = 1; else w->fieldpointer = 0;
						break;
					case 5:
						if (x < 34) { *index = 3; ss->attributes ^= 0b1000; }
						else        { *index = 4; ss->attributes ^= 0b10000; }
						w->fieldpointer = 0; break;
					case 6: *index = 5; ss->attributes ^= 0b100000; break;
					case 7: case 8: *index = 6; ss->attributes ^= 0b1000000; break;
					case 9: case 10: *index = 7; ss->attributes ^= 0b1; break;
					case 11: *index = 8;
						if (x < 32)          w->fieldpointer = 0;
						else if (x > 32 + 3) w->fieldpointer = 3;
						else w->fieldpointer = x - 32;
						break;
					default: *index = 9; ss->attributes ^= 0b10; break;
				}
			} else
			{
				switch (y)
				{
					case 2: case 3: *index = 10;
						if (x < 59) w->fieldpointer = 1; else w->fieldpointer = 0;
						break;
					case 4: *index = 11;
						if (x < 59) w->fieldpointer = 1; else w->fieldpointer = 0;
						break;
					case 5: *index = 12;
						if (x < 59) w->fieldpointer = 1; else w->fieldpointer = 0;
						break;
					case 6: case 7: *index = 13;
						if (x < 59) w->fieldpointer = 1; else w->fieldpointer = 0;
						break;
					case 8: case 9:
						if (x < 50)
						{
							*index = 14;
							if (x < 41)          w->fieldpointer = 0;
							else if (x > 41 + 7) w->fieldpointer = 7;
							else w->fieldpointer = x - 41;
						} else
						{
							*index = 16;
							if (x < 52)          w->fieldpointer = 0;
							else if (x > 52 + 7) w->fieldpointer = 7;
							else w->fieldpointer = x - 52;
						} break;
					case 10: case 11:
						if (x < 50)
						{
							*index = 15;
							if (x < 41)          w->fieldpointer = 0;
							else if (x > 41 + 7) w->fieldpointer = 7;
							else w->fieldpointer = x - 41;
						} else
						{
							*index = 17;
							if (x < 52)          w->fieldpointer = 0;
							else if (x > 52 + 7) w->fieldpointer = 7;
							else w->fieldpointer = x - 52;
						} break;
					default: *index = 18; ss->attributes ^= 0b100; break;
				}
			} break;
	}
}

/* clamps within range and loop, returns output samples */
uint32_t trimloop(uint32_t pitchedpointer, uint32_t pointer,
		channel *cv, instrument *iv, sampler_state *ss,
		sample_t *l, sample_t *r)
{
	if (ss->trim[0] < ss->trim[1])
	{ /* forwards */
		pitchedpointer += ss->trim[0];

		if (ss->loop[0] || ss->loop[1])
		{ /* if there is a loop range */
			if (ss->loop[0] < ss->loop[1])
			{ /* forwards loop */
				if (ss->attributes & 0b100)
				{
					uint32_t rampmax = MIN(samplerate / 1000 * LOOP_RAMP_MS, (ss->loop[1] - ss->loop[0]) / 2);
					uint32_t loopoffset = ss->loop[1] - ss->loop[0] - rampmax;
					while (pitchedpointer >= ss->loop[1])
						pitchedpointer -= loopoffset;

					pitchedpointer -= pitchedpointer % ss->channels; /* always point to the left channel */
					if (pitchedpointer <= ss->length)
					{
						if (pitchedpointer > ss->loop[1] - rampmax)
						{
							float lerp = (pitchedpointer - ss->loop[1] + rampmax) / (float)rampmax;
							uint32_t ramppointer = (pitchedpointer - loopoffset);
							ramppointer -= ramppointer % ss->channels; /* align with channels */

							if (ss->attributes & 0b10000) /* 8-bit */
							{
								*l = (signed char)(iv->sampledata[pitchedpointer * ss->channels]>>8) / (float)SCHAR_MAX * (1.0 - lerp)
									+ (signed char)(iv->sampledata[ramppointer * ss->channels]>>8) / (float)SCHAR_MAX * lerp;

								if (ss->channels > 1)
									*r = (signed char)(iv->sampledata[pitchedpointer * ss->channels + 1]>>8) / (float)SCHAR_MAX * (1.0 - lerp)
										+ (signed char)(iv->sampledata[ramppointer * ss->channels + 1]>>8) / (float)SCHAR_MAX * lerp;
								else *r = *l;
							} else
							{
								*l = iv->sampledata[pitchedpointer * ss->channels] / (float)SHRT_MAX * (1.0 - lerp)
									+ iv->sampledata[ramppointer * ss->channels] / (float)SHRT_MAX * lerp;

								if (ss->channels > 1)
									*r = iv->sampledata[pitchedpointer * ss->channels + 1] / (float)SHRT_MAX * (1.0 - lerp)
										+ iv->sampledata[ramppointer * ss->channels + 1] / (float)SHRT_MAX * lerp;
								else *r = *l;
							}
						} else
						{
							if (ss->attributes & 0b10000) /* 8-bit */
							{
								*l = (signed char)(iv->sampledata[pitchedpointer * ss->channels]>>8) / (float)SCHAR_MAX;
								if (ss->channels > 1) *r = (signed char)(iv->sampledata[pitchedpointer * ss->channels + 1]>>8) / (float)SCHAR_MAX;
								else *r = *l;
							} else
							{
								*l = iv->sampledata[pitchedpointer * ss->channels] / (float)SHRT_MAX;
								if (ss->channels > 1) *r = iv->sampledata[pitchedpointer * ss->channels + 1] / (float)SHRT_MAX;
								else *r = *l;
							}
						}
					} else *l = *r = 0.0;
				}
				else
					while (pitchedpointer >= ss->loop[1])
						pitchedpointer -= ss->loop[1] - ss->loop[0];
			}
			/* TODO: bidi loop */
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
			{
				if (ss->attributes & 0b100)
				{
					uint32_t rampmax = MIN(samplerate / 1000 * LOOP_RAMP_MS, (ss->loop[0] - ss->loop[1]) / 2);
					uint32_t loopoffset = ss->loop[0] - ss->loop[1] - rampmax;
					while (pitchedpointer >= ss->loop[0])
						pitchedpointer += loopoffset;

					pitchedpointer -= pitchedpointer % ss->channels; /* always point to the left channel */
					if (pitchedpointer <= ss->length)
					{
						if (pitchedpointer > ss->loop[0] - rampmax)
						{
							float lerp = (pitchedpointer - ss->loop[0] + rampmax) / (float)rampmax;
							uint32_t ramppointer = (pitchedpointer + loopoffset);
							ramppointer -= ramppointer % ss->channels; /* align with channels */

							if (ss->attributes & 0b10000) /* 8-bit */
							{
								*l = (signed char)(iv->sampledata[pitchedpointer * ss->channels]>>8) / (float)SCHAR_MAX * (1.0 - lerp)
								+ (signed char)(iv->sampledata[ramppointer * ss->channels]>>8) / (float)SCHAR_MAX * lerp;

								if (ss->channels > 1)
									*r = (signed char)(iv->sampledata[pitchedpointer * ss->channels + 1]>>8) / (float)SCHAR_MAX * (1.0 - lerp)
										+ (signed char)(iv->sampledata[ramppointer * ss->channels + 1]>>8) / (float)SCHAR_MAX * lerp;
								else *r = *l;
							} else
							{
								*l = iv->sampledata[pitchedpointer * ss->channels] / (float)SHRT_MAX * (1.0 - lerp)
									+ iv->sampledata[ramppointer * ss->channels] / (float)SHRT_MAX * lerp;

								if (ss->channels > 1)
									*r = iv->sampledata[pitchedpointer * ss->channels + 1] / (float)SHRT_MAX * (1.0 - lerp)
										+ iv->sampledata[ramppointer * ss->channels + 1] / (float)SHRT_MAX * lerp;
								else *r = *l;
							}
						} else
						{
							if (ss->attributes & 0b10000) /* 8-bit */
							{
								*l = (signed char)(iv->sampledata[pitchedpointer * ss->channels]>>8) / (float)SCHAR_MAX;
								if (ss->channels > 1) *r = (signed char)(iv->sampledata[pitchedpointer * ss->channels + 1]>>8) / (float)SCHAR_MAX;
								else *r = *l;
							} else
							{
								*l = iv->sampledata[pitchedpointer * ss->channels] / (float)SHRT_MAX;
								if (ss->channels > 1) *r = iv->sampledata[pitchedpointer * ss->channels + 1] / (float)SHRT_MAX;
								else *r = *l;
							}
						}
					} else *l = *r = 0.0;
				}
				else
					while (pitchedpointer <= ss->loop[1])
						pitchedpointer += ss->loop[0] - ss->loop[1];
			}
			/* TODO: bidi loop */
		}

		/* trigger the release envelope */
		if (((ss->loop[1] && ss->trim[1] > ss->loop[1]) || !ss->loop[1])
				&& pitchedpointer < ss->trim[1] + (ss->volume.r * ENVELOPE_RELEASE * samplerate))
			cv->releasepointer = pointer;

		/* cut if the pointer is ever past trim[1] */
		if (pitchedpointer <= ss->trim[1])
			cv->r.note = 0;
	}

	pitchedpointer -= pitchedpointer % ss->channels; /* always point to the left channel */
	if (!(ss->attributes & 0b100) || !(ss->loop[0] || ss->loop[1]))
	{
		if (pitchedpointer <= ss->length)
		{
			if (ss->attributes & 0b10000) /* 8-bit */
			{
				*l = (signed char)(iv->sampledata[pitchedpointer * ss->channels]>>8) / (float)SCHAR_MAX;
				if (ss->channels > 1) *r = (signed char)(iv->sampledata[pitchedpointer * ss->channels + 1]>>8) / (float)SCHAR_MAX;
				else *r = *l;
			} else
			{
				*l = iv->sampledata[pitchedpointer * ss->channels] / (float)SHRT_MAX;
				if (ss->channels > 1) *r = iv->sampledata[pitchedpointer * ss->channels + 1] / (float)SHRT_MAX;
				else *r = *l;
			}
		} else *l = *r = 0.0;
	}
	return pitchedpointer;
}

/* must be realtime safe            */
/* should accept arbitrary pointers */
void samplerProcess(instrument *iv, channel *cv, uint32_t pointer, float *l, float *r)
{
	uint32_t pitchedpointer;
	sampler_state *ss = iv->state[iv->type];
	sampler_channel *sc = cv->state[iv->type];

	float gain = adsrEnvelope(ss->volume, 0.0, pointer, cv->releasepointer);

	if (ss->length > 0)
	{
		float decimate = 1.0 + (1.0 - ss->samplerate / 256.0) * 20;
		if (ss->attributes & 0b1) /* persistent tempo */
		{
			if (pointer % MAX(ss->cyclelength, sc->stretchrampmax) == 0) /* first sample of a cycle */
			{
				/* don't ramp the first cycle */
				if (pointer == 0) sc->stretchrampindex = sc->stretchrampmax;
				else
				{
					uint32_t ramppointer;
					sc->stretchrampindex = 0;
					for (uint16_t i = 0; i < sc->stretchrampmax; i++)
					{
						ramppointer = (sc->cycleoffset + (float)(MAX(ss->cyclelength, sc->stretchrampmax) + i + 1)
								/ (float)samplerate * ss->c5rate
								* powf(M_12_ROOT_2, (short)cv->r.note - 61 + cv->cents))
								/ decimate;
						ramppointer *= decimate;
						ramppointer = trimloop(ramppointer, pointer + i + 1, cv, iv, ss,
								&sc->stretchrampbuffer[i * 2 + 0],
								&sc->stretchrampbuffer[i * 2 + 1]);
						if (ramppointer > ss->length) break;
					}
				}
				sc->cycleoffset = (float)(pointer + cv->pointeroffset)
					/ (float)samplerate * ss->c5rate;
			}
			pitchedpointer = (sc->cycleoffset + (float)(pointer % MAX(ss->cyclelength, sc->stretchrampmax))
				/ (float)samplerate * ss->c5rate
				* powf(M_12_ROOT_2, (short)cv->r.note - 61 + cv->cents))
				/ decimate;
			pitchedpointer *= decimate;
		} else
		{
			/* 61 is C-5 */
			pitchedpointer = ((float)(pointer + cv->pointeroffset)
				/ (float)samplerate * ss->c5rate
				* powf(M_12_ROOT_2, (short)cv->r.note - 61 + cv->cents))
				/ decimate;
			pitchedpointer *= decimate;
		}

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
	if (sc->stretchrampindex < sc->stretchrampmax)
	{
		float rampgain = (float)sc->stretchrampindex / (float)sc->stretchrampmax;
		*l *= rampgain;
		*r *= rampgain;
		*l += sc->stretchrampbuffer[sc->stretchrampindex * 2 + 0] * (1.0 - rampgain);
		*r += sc->stretchrampbuffer[sc->stretchrampindex * 2 + 1] * (1.0 - rampgain);
		sc->stretchrampindex++;
	}

	if (ss->attributes & 0b1000) /* mono */
	{
		*l = (*l + *r) / 2.0;
		*r = *l;
	}
	if (ss->attributes & 0b1000000) /* invert phase */
	{
		*l *= -1;
		*r *= -1;
	}
	if (ss->attributes & 0b100000) // signed unsigned conversion
	{
		if (*l != 0.0)
		{
			if (*l > 0.0) *l -= 1.0;
			else          *l += 1.0;
		}
		if (*r != 0.0)
		{
			if (*r > 0.0) *r -= 1.0;
			else          *r += 1.0;
		}
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
					memset(cv->rampbuffer, 0, sizeof(sample_t) * cv->rampmax * 2);
					for (uint16_t i = 0; i < cv->rampmax; i++)
					{
						if (!cv->r.note) break;
						t->f[iv->type].process(iv, cv, cv->pointer + i,
								&cv->rampbuffer[i * 2 + 0], &cv->rampbuffer[i * 2 + 1]);
					}
					cv->rampindex = 0;
				}
				cv->pointeroffset = ss->trim[0] + m / 256.0 * (ss->trim[1] - ss->trim[0]);
				cv->pointer = 0;
			}
			break;
	}
}

/* called when state's type is changed to this file's (**state will later be freed) */
void samplerInitType(void **state)
{
	*state = calloc(1, sizeof(sampler_state));
	sampler_state *ss = *state;
	ss->volume.s = 255;
	ss->samplerate = 255;
	ss->attributes = 0b00000100; /* loop ramping on by default */
	ss->cyclelength = 0x6ff; /* feels like a good default? hard to be sure */
}

/* should initialize channel state */
void samplerAddChannel(void **state)
{
	*state = calloc(1, sizeof(sampler_channel));
	sampler_channel *sc = *state;
	sc->stretchrampmax = samplerate / 1000 * TIMESTRETCH_RAMP_MS;
	sc->stretchrampindex = sc->stretchrampmax;
	sc->stretchrampbuffer = malloc(sizeof(sample_t) * sc->stretchrampmax * 2); /* *2 for stereo */
}

/* should clean up everything allocated in initChannel */
void samplerDelChannel(void **state)
{
	sampler_channel *sc = *state;
	free(sc->stretchrampbuffer);
	free(*state); *state = NULL;
}


void samplerWrite(void **state, FILE *fp)
{ fwrite(*state, sizeof(sampler_state), 1, fp); }

void samplerRead(void **state, FILE *fp)
{ fread(*state, sizeof(sampler_state), 1, fp); }

void samplerInit(int index)
{
	t->f[index].indexc = 18;
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
	t->f[index].initType = &samplerInitType;
	t->f[index].addChannel = &samplerAddChannel;
	t->f[index].delChannel = &samplerDelChannel;
	t->f[index].write = &samplerWrite;
	t->f[index].read = &samplerRead;
}
