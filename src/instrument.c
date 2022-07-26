#include "types/sampler.c"

#define I_MODE_INDICES 0
#define I_MODE_NORMAL 1
#define I_MODE_PREVIEW 2
#define I_MODE_ADJUST 3
#define I_MODE_MOUSEADJUST 4
#define I_MODE_VISUAL 5

#define MAX_INSTRUMENT_INDEX 14

#define INSTRUMENT_BODY_COLS 70
#define INSTRUMENT_BODY_ROWS 20
#define INSTRUMENT_TYPE_ROWS 14


void drawMarker(uint32_t marker, uint32_t offset)
{
	size_t xpos;
	if (marker >= offset && marker <= offset + w->waveformwidth)
	{
		xpos = (float)(marker - offset) / (float)w->waveformwidth * w->waveformw;
		for (size_t i = 0; i < w->waveformh-1; i++) set_pixel_unsafe(w->waveformcanvas, i%2, xpos, i);
	}
}

void drawInstrument(void)
{
	printf("\033[%d;%dH\033[2mPATTERN\033[m \033[1mINSTRUMENT\033[m", CHANNEL_ROW-2, (ws.ws_col-18) / 2);
	switch (w->mode)
	{
		case I_MODE_INDICES: case I_MODE_NORMAL:     printf("\033[0 q"); break;
		case I_MODE_PREVIEW:                         printf("\033[%d;0H\033[1m-- PREVIEW --\033[m\033[3 q", ws.ws_row); w->command.error[0] = '\0'; break;
		case I_MODE_ADJUST: case I_MODE_MOUSEADJUST: printf("\033[%d;0H\033[1m-- ADJUST --\033[m\033[0 q", ws.ws_row); w->command.error[0] = '\0'; break;
		case I_MODE_VISUAL:                          printf("\033[%d;0H\033[1m-- VISUAL --\033[m\033[0 q", ws.ws_row); w->command.error[0] = '\0'; break;
	}

	int i;
	instrument *iv;

	for (i = 0; i < 255; i++)
		if (w->centre - w->instrument + i > CHANNEL_ROW && w->centre - w->instrument + i < ws.ws_row)
		{
			if (w->mode != I_MODE_INDICES && w->instrument == i) printf("\033[7m");

			iv = s->instrumentv[s->instrumenti[i]];
			if (iv) printf("\033[%d;1H %02x \033[2m%02x\033[22m \033[1m[%08x]\033[22m ", w->centre - w->instrument + i, i, s->instrumenti[i], iv->length);
			else    printf("\033[%d;1H %02x \033[2m%02x\033[22m  ........  ",            w->centre - w->instrument + i, i, s->instrumenti[i]);
			printf("\033[m");
		}

	iv = s->instrumentv[s->instrumenti[w->instrument]];
	if (iv)
	{
		if (!iv->samplelength)
			printf("\033[%d;%dH [NO SAMPLE] ", w->centre, INSTRUMENT_INDEX_COLS + (ws.ws_col - INSTRUMENT_INDEX_COLS - 13)/2);
		else
		{
			printf("\033[%d;%dH [WAVEFORM] ", CHANNEL_ROW, INSTRUMENT_INDEX_COLS + (ws.ws_col - INSTRUMENT_INDEX_COLS - 12)/2);
			if (w->waveformbuffer)
			{
				/* draw visual/cursor */
				size_t cursorxpos, visualxpos;
				uint32_t offset;
				if (w->waveformcursor < (w->waveformwidth>>1))
				{
					cursorxpos = (float)w->waveformcursor / (float)w->waveformwidth * w->waveformw;
					offset = 0;
				} else if (w->waveformcursor > iv->length - (w->waveformwidth>>1))
				{
					cursorxpos = (float)(w->waveformcursor - (iv->length - w->waveformwidth)) / (float)w->waveformwidth * w->waveformw;
					offset = (iv->length - w->waveformwidth);
				} else
				{
					cursorxpos = w->waveformw>>1;
					offset = w->waveformcursor - (w->waveformwidth>>1);
				}

				if (w->mode == I_MODE_VISUAL)
				{
					if (w->waveformvisual <= offset)
						visualxpos = 0;
					else if (w->waveformvisual >= offset + w->waveformwidth)
						visualxpos = w->waveformw-1;
					else
						visualxpos = (float)(w->waveformvisual - offset) / (float)w->waveformwidth * w->waveformw;
				} else visualxpos = cursorxpos;

				size_t lowxpos =  MIN(cursorxpos, visualxpos);
				size_t highxpos = MAX(cursorxpos, visualxpos);

				if (w->waveformdrawpointer == 0)
				{
					fill(w->waveformcanvas, 0);
					for (size_t i = lowxpos; i <= highxpos; i++)
						for (size_t j = 0; j < w->waveformh; j++)
							set_pixel_unsafe(w->waveformcanvas, 1, i, j);
				}

				size_t k, x;
				uint32_t l;
				float channelmix = 1.0 / (float)iv->channels;
				double divmaxj = 1.0 / (float)w->waveformwidth;
				float o = (float)w->waveformh * 0.5;
				float sample;
				uint32_t samplesperpixel = w->waveformwidth / w->waveformw;
				if (w->waveformdrawpointer < w->waveformwidth)
				{
					for (uint32_t j = 0; j < WAVEFORM_LAZY_BLOCK_SIZE; j++)
					{
						l = (w->waveformdrawpointer%w->waveformw)*samplesperpixel + w->waveformdrawpointer/w->waveformw;
						k = (float)l * divmaxj * (float)w->waveformwidth;
						x = (float)l * divmaxj * (float)w->waveformw;

						sample = 0.0;
						for (uint8_t i = 0; i < iv->channels; i++) /* mix all channels */
							sample = sample + (iv->sampledata[(offset + k) * iv->channels + i] * channelmix);
						sample = (sample*DIVSHRT) * o + o;

						if (x < lowxpos || x > highxpos) set_pixel_unsafe(w->waveformcanvas, 1, x, sample);
						else                             set_pixel_unsafe(w->waveformcanvas, 0, x, sample);

						w->waveformdrawpointer++;
						if (w->waveformdrawpointer >= w->waveformwidth)
						{
							w->waveformdrawpointer++;
							break;
						}
					} p->dirty = 1; /* continue drawing asap */
				}

				drawMarker(iv->trim[0], offset);
				drawMarker(iv->trim[1], offset);
				drawMarker(iv->loop[0], offset);
				drawMarker(iv->loop[1], offset);
				draw(w->waveformcanvas, w->waveformbuffer);
				for (size_t i = 0; w->waveformbuffer[i] != NULL; i++)
					printf("\033[%ld;%dH%s", CHANNEL_ROW + i +1, INSTRUMENT_INDEX_COLS, w->waveformbuffer[i]);
			}
		}

		unsigned short x = INSTRUMENT_INDEX_COLS + (ws.ws_col - INSTRUMENT_INDEX_COLS - 58)/2;
		unsigned short y = ws.ws_row - INSTRUMENT_CONTROL_ROW;
		printf("\033[%d;%dHC-5 rate:  [%08x]    TODO: gain/pan      MIDI: ", y+0, x, iv->c5rate);
		drawBit(iv->flags&S_FLAG_MIDI);    printf("[%x] ", iv->midichannel);
		printf("\033[%d;%dHdecimate:     [%x][%02x]  ┌─   LOOP   ─┐  ┌─  TIMESTRETCH  ─┐", y+1, x, iv->bitdepth, iv->samplerate);
		printf("\033[%d;%dHchannels:              ping-pong: ", y+2, x);
		drawBit(iv->flags&S_FLAG_PPLOOP);  printf("  cycle size:  [%04x]", iv->cyclelength);
		printf("\033[%d;%dHenv a[d,r]/s: [%02x]", y+3, x, iv->envelope);
		drawBit(!(iv->flags&S_FLAG_SUSTAIN)); printf("  ramping:  [%02x]  time/pitch: ", iv->loopramp);
		drawBit(iv->flags&S_FLAG_TTEMPO);  printf("[%02x]", iv->pitchshift);

		switch (w->mode)
		{
			case I_MODE_ADJUST: case I_MODE_MOUSEADJUST:
				drawChannels(iv->channelmode, y+2, x+12, (w->instrumentindex == 4));
				break;
			default:
				drawChannels(iv->channelmode, y+2, x+12, 0);
				break;
		}

		switch (w->instrumentindex)
		{
			case 0:
				if (!iv->samplelength) printf("\033[%d;%dH", w->centre, INSTRUMENT_INDEX_COLS + (ws.ws_col - INSTRUMENT_INDEX_COLS - 9)/2);
				else                   printf("\033[%d;%dH", CHANNEL_ROW, INSTRUMENT_INDEX_COLS + (ws.ws_col - INSTRUMENT_INDEX_COLS - 12)/2 +2);
				break;
			case 1:  printf("\033[%d;%dH", y+0, x+19 - w->fieldpointer); break;
			case 2:  printf("\033[%d;%dH", y+1, x+15); break;
			case 3:  printf("\033[%d;%dH", y+1, x+19 - w->fieldpointer); break;
			case 4:  printf("\033[%d;%dH", y+2, x+19); break;
			case 5:  printf("\033[%d;%dH", y+3, x+16 - w->fieldpointer); break;
			case 6:  printf("\033[%d;%dH", y+3, x+19); break;
			case 7:  printf("\033[%d;%dH", y+0, x+41 - w->fieldpointer); break;
			case 8:  printf("\033[%d;%dH", y+0, x+52); break;
			case 9:  printf("\033[%d;%dH", y+0, x+55); break;
			case 10: printf("\033[%d;%dH", y+2, x+35); break;
			case 11: printf("\033[%d;%dH", y+3, x+35 - w->fieldpointer); break;
			case 12: printf("\033[%d;%dH", y+2, x+56 - w->fieldpointer); break;
			case 13: printf("\033[%d;%dH", y+3, x+52); break;
			case 14: printf("\033[%d;%dH", y+3, x+56 - w->fieldpointer); break;
		}
	} else printf("\033[%d;%dH [NOT ADDED] \033[11D", w->centre, INSTRUMENT_INDEX_COLS + (ws.ws_col - INSTRUMENT_INDEX_COLS - 13)/2);

	if (w->mode == I_MODE_INDICES)
		printf("\033[%d;%dH", w->centre + w->fyoffset, 9);
}

void instrumentIncFieldPointer(short index)
{
	switch (index)
	{
		case 1:  w->fieldpointer--; if (w->fieldpointer < 0) w->fieldpointer = 7; break;
		case 10: w->fieldpointer--; if (w->fieldpointer < 0) w->fieldpointer = 3; break;
		default: w->fieldpointer = 0; break;
	}
}
void instrumentDecFieldPointer(short index)
{
	switch (index)
	{
		case 1:  w->fieldpointer++; if (w->fieldpointer > 7) w->fieldpointer = 0; break;
		case 10: w->fieldpointer++; if (w->fieldpointer > 3) w->fieldpointer = 0; break;
		default: w->fieldpointer = 1; break;
	}
}
void inputInstrumentHex(unsigned short index, instrument *iv, char value)
{
	switch (index)
	{
		case 1:  updateField(w->fieldpointer, (uint32_t *)&iv->c5rate, value); break;
		case 2:  iv->bitdepth = value; break;
		case 3:  updateFieldPush(&iv->samplerate, value); break;
		case 5:  updateFieldPush(&iv->envelope, value); break;
		// case 7:  updateFieldPush(&iv->defgain, value); break;
		case 9:  iv->midichannel = value; break;
		case 11: updateFieldPush(&iv->loopramp, value); break;
		case 12: updateField(w->fieldpointer, (uint32_t *)&iv->cyclelength, value); break;
		case 14: updateFieldPush(&iv->pitchshift, value); break;
	} instrumentIncFieldPointer(index);
}

void instrumentAdjustUp(instrument *iv, short index, char mouse)
{
	uint32_t temp;
	if (!mouse)
		switch (index)
		{
			case 1:  incField(w->fieldpointer, &iv->c5rate, 0xffffffff); break;
			case 2:  if (iv->bitdepth < 15) iv->bitdepth++; break;
			case 3:  if (w->fieldpointer) { if (iv->samplerate < 255 - 16) iv->samplerate+=16; else iv->samplerate = 255; } else if (iv->samplerate < 255) iv->samplerate++; break;
			case 5:  if (w->fieldpointer) { if (iv->envelope < 255 - 16) iv->envelope+=16; else iv->envelope = 255; } else if (iv->envelope < 255) iv->envelope++; break;
			case 9:  if (iv->midichannel < 15) iv->midichannel++; break;
			case 11: if (w->fieldpointer) { if (iv->loopramp < 255 - 16) iv->loopramp+=16; else iv->loopramp = 255; } else if (iv->loopramp < 255) iv->loopramp++; break;
			case 12: temp = iv->cyclelength; incField(w->fieldpointer, &temp, 0xffff); iv->cyclelength = temp; break;
			case 14: if (w->fieldpointer) { if (iv->pitchshift < 255 - 16) iv->pitchshift+=16; else iv->pitchshift = 255; } else if (iv->pitchshift < 255) iv->pitchshift++; break;
		}
	if (index == 4 && iv->channelmode < 4) iv->channelmode++;
}
void instrumentAdjustDown(instrument *iv, short index, char mouse)
{
	uint32_t temp;
	if (!mouse)
		switch (index)
		{
			case 1:  decField(w->fieldpointer, &iv->c5rate); break;
			case 2:  if (iv->bitdepth) iv->bitdepth--; break;
			case 3:  if (w->fieldpointer) { if (iv->samplerate > 16) iv->samplerate-=16; else iv->samplerate = 0; } else if (iv->samplerate) iv->samplerate--; break;
			case 5:  if (w->fieldpointer) { if (iv->envelope > 16) iv->envelope-=16; else iv->envelope = 0; } else if (iv->envelope) iv->envelope--; break;
			case 9:  if (iv->midichannel) iv->midichannel--; break;
			case 11: if (w->fieldpointer) { if (iv->loopramp > 16) iv->loopramp-=16; else iv->loopramp = 0; } else if (iv->loopramp) iv->loopramp--; break;
			case 12: temp = iv->cyclelength; decField(w->fieldpointer, &temp); iv->cyclelength = temp; break;
			case 14: if (w->fieldpointer) { if (iv->pitchshift > 16) iv->pitchshift-=16; else iv->pitchshift = 0; } else if (iv->pitchshift) iv->pitchshift--; break;
		}
	if (index == 4 && iv->channelmode) iv->channelmode--;
}
void instrumentAdjustLeft (instrument *iv, short index, char mouse)
{
	uint32_t temp;
	/*if (index == 7)
	{
		if (iv->defgain%16 > iv->defgain>>4) iv->defgain += 16;
		else if (iv->defgain%16)             iv->defgain -= 1;
	} else */if (mouse)
		switch (index)
		{
			case 1:  decField(w->fieldpointer, &iv->c5rate); break;
			case 2:  if (iv->bitdepth) iv->bitdepth--; break;
			case 3:  if (w->fieldpointer) { if (iv->samplerate > 16) iv->samplerate-=16; else iv->samplerate = 0; } else if (iv->samplerate) iv->samplerate--; break;
			case 5:  if (w->fieldpointer) { if (iv->envelope > 16) iv->envelope-=16; else iv->envelope = 0; } else if (iv->envelope) iv->envelope--; break;
			case 9:  if (iv->midichannel) iv->midichannel--; break;
			case 11: if (w->fieldpointer) { if (iv->loopramp > 16) iv->loopramp-=16; else iv->loopramp = 0; } else if (iv->loopramp) iv->loopramp--; break;
			case 12: temp = iv->cyclelength; decField(w->fieldpointer, &temp); iv->cyclelength = temp; break;
			case 14: if (w->fieldpointer) { if (iv->pitchshift > 16) iv->pitchshift-=16; else iv->pitchshift = 0; } else if (iv->pitchshift) iv->pitchshift--; break;
		}
	else instrumentDecFieldPointer(index);
}
void instrumentAdjustRight(instrument *iv, short index, char mouse)
{
	uint32_t temp;
	/*if (index == 7)
	{
		if (iv->defgain>>4 > iv->defgain%16) iv->defgain += 1;
		else if (iv->defgain>>4)             iv->defgain -= 16;
	} else */if (mouse)
		switch (index)
		{
			case 1:  incField(w->fieldpointer, &iv->c5rate, 0xffffffff); break;
			case 2:  if (iv->bitdepth < 15) iv->bitdepth++; break;
			case 3:  if (w->fieldpointer) { if (iv->samplerate < 255 - 16) iv->samplerate+=16; else iv->samplerate = 255; } else if (iv->samplerate < 255) iv->samplerate++; break;
			case 5:  if (w->fieldpointer) { if (iv->envelope < 255 - 16) iv->envelope+=16; else iv->envelope = 255; } else if (iv->envelope < 255) iv->envelope++; break;
			// case 7:  if (w->fieldpointer) { if (iv->defgain < 255 - 16) iv->defgain+=16; else iv->defgain = 255; } else if (iv->defgain < 255) iv->defgain++; break;
			case 9:  if (iv->midichannel < 15) iv->midichannel++; break;
			case 11: if (w->fieldpointer) { if (iv->loopramp < 255 - 16) iv->loopramp+=16; else iv->loopramp = 255; } else if (iv->loopramp < 255) iv->loopramp++; break;
			case 12: temp = iv->cyclelength; incField(w->fieldpointer, &temp, 0xffff); iv->cyclelength = temp; break;
			case 14: if (w->fieldpointer) { if (iv->pitchshift < 255 - 16) iv->pitchshift+=16; else iv->pitchshift = 255; } else if (iv->pitchshift < 255) iv->pitchshift++; break;
		}
	else instrumentIncFieldPointer(index);
}

void sampleApplyTrimming(instrument *iv)
{
	pushInstrumentHistoryIfNew(iv);
	if (iv->samplelength > 0)
	{
		uint32_t newlen
			= MAX(iv->trim[0], iv->trim[1])
			- MIN(iv->trim[0], iv->trim[1]) + 1;

		// malloc a new buffer
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
int sampleExportCallback(char *command, unsigned char *mode)
{
	char *buffer = malloc(strlen(command) + 1);
	wordSplit(buffer, command, 0);
	sampleApplyTrimming(s->instrumentv[s->instrumenti[w->instrument]]);

	instrument *iv = s->instrumentv[s->instrumenti[w->instrument]];
	if (!iv->sampledata) { free(buffer); return 1; }

	SNDFILE *sndfile;
	SF_INFO sfinfo;
	memset(&sfinfo, 0, sizeof(sfinfo));

	sfinfo.samplerate = iv->c5rate;
	sfinfo.frames = iv->length;
	sfinfo.channels = iv->channels;

	sfinfo.format = SF_FORMAT_WAV | SF_FORMAT_PCM_16;
	sndfile = sf_open(fileExtension(buffer, ".wav"), SFM_WRITE, &sfinfo);
	if (sndfile == NULL) { free(buffer); return 1; }

	// write the sample data to disk
	sf_writef_short(sndfile, iv->sampledata, iv->length);
	sf_close(sndfile);

	free(buffer);
	return 0;
}
void resetWaveform(void)
{
	instrument *iv = s->instrumentv[s->instrumenti[w->instrument]];
	if (iv)
	{
		w->waveformwidth = iv->length;
		w->waveformcursor = 0;
		w->waveformdrawpointer = 0; redraw();
	}
}
void sampleLoadCallback(char *path)
{
	loadSample(w->instrument, path);
	pushInstrumentHistory(s->instrumentv[s->instrumenti[w->instrument]]);
	w->popup = 1;
	w->mode = w->oldmode;
	resetWaveform();
}

void instrumentUpArrow(int count, instrument *iv)
{
	switch (w->mode)
	{
		case I_MODE_INDICES:
			if (w->instrumentrecv == INST_REC_LOCK_OK)
			{
				w->instrument -= count;
				if (w->instrument < 0) w->instrument = 0;
				resetWaveform();
			} break;
		case I_MODE_NORMAL: case I_MODE_PREVIEW:
			if (s->instrumentv[s->instrumenti[w->instrument]])
			{
				w->instrumentindex -= count;
				if (w->instrumentindex < 0) w->instrumentindex = 0;
			} w->fieldpointer = 0; break;
		case I_MODE_ADJUST: case I_MODE_MOUSEADJUST:
			instrumentAdjustUp(iv, w->instrumentindex, 0);
			break;
	}
}
void instrumentDownArrow(int count, instrument *iv)
{
	switch (w->mode)
	{
		case I_MODE_INDICES:
			if (w->instrumentrecv == INST_REC_LOCK_OK)
			{
				w->instrument += count;
				if (w->instrument > 254) w->instrument = 254;
				resetWaveform();
			} break;
		case I_MODE_VISUAL: w->mode = I_MODE_NORMAL; w->waveformdrawpointer = 0; redraw();
		case I_MODE_NORMAL: case I_MODE_PREVIEW:
			if (s->instrumentv[s->instrumenti[w->instrument]])
			{
				w->instrumentindex += count;
				if (w->instrumentindex > MAX_INSTRUMENT_INDEX) w->instrumentindex = MAX_INSTRUMENT_INDEX;
			} w->fieldpointer = 0; break;
		case I_MODE_ADJUST: case I_MODE_MOUSEADJUST:
			instrumentAdjustDown(iv, w->instrumentindex, 0);
			break;
	}
}
void instrumentLeftArrow(instrument *iv)
{
	uint32_t delta;
	switch (w->mode)
	{
		case I_MODE_NORMAL: case I_MODE_PREVIEW: case I_MODE_VISUAL:
		case I_MODE_ADJUST: case I_MODE_MOUSEADJUST:
			if (!w->instrumentindex)
			{
				if (iv)
				{
					delta = w->waveformwidth / WAVEFORM_COARSE_SLICES;
					if (delta > w->waveformcursor) w->waveformcursor = 0;
					else                           w->waveformcursor -= delta;
					w->waveformdrawpointer = 0; redraw();
				}
			} else instrumentAdjustLeft(iv, w->instrumentindex, 0);
			break;
	}
}
void instrumentCtrlLeftArrow(instrument *iv)
{
	uint32_t delta;
	if (!w->instrumentindex && iv)
		switch (w->mode)
		{
			case I_MODE_NORMAL: case I_MODE_PREVIEW: case I_MODE_VISUAL:
			case I_MODE_ADJUST: case I_MODE_MOUSEADJUST:
				delta = w->waveformwidth / WAVEFORM_FINE_SLICES;
				if (delta > w->waveformcursor) w->waveformcursor = 0;
				else                           w->waveformcursor -= delta;
				w->waveformdrawpointer = 0; redraw(); break;
		}
}
void instrumentCtrlRightArrow(instrument *iv)
{
	uint32_t delta;
	if (!w->instrumentindex && iv)
		switch (w->mode)
		{
			case I_MODE_NORMAL: case I_MODE_PREVIEW: case I_MODE_VISUAL:
			case I_MODE_ADJUST: case I_MODE_MOUSEADJUST:
				delta = w->waveformwidth / WAVEFORM_FINE_SLICES;
				if (delta > iv->length-1 - w->waveformcursor) w->waveformcursor = iv->length-1;
				else                                          w->waveformcursor += delta;
				w->waveformdrawpointer = 0; redraw(); break;
		}
}
void instrumentRightArrow(instrument *iv)
{
	uint32_t delta;
	switch (w->mode)
	{
		case I_MODE_NORMAL: case I_MODE_PREVIEW: case I_MODE_VISUAL:
		case I_MODE_ADJUST: case I_MODE_MOUSEADJUST:
			if (!w->instrumentindex)
			{
				if (iv)
				{
					delta = w->waveformwidth / WAVEFORM_COARSE_SLICES;
					if (delta > iv->length-1 - w->waveformcursor) w->waveformcursor = iv->length-1;
					else                                          w->waveformcursor += delta;
					w->waveformdrawpointer = 0; redraw();
				}
			} else instrumentAdjustRight(iv, w->instrumentindex, 0);
			break;
	}
}
void instrumentHome(instrument *iv)
{
	switch (w->mode)
	{
		case I_MODE_INDICES:
			if (w->instrumentrecv == INST_REC_LOCK_OK)
			{ w->instrument = 0; resetWaveform(); }
			break;
		case I_MODE_VISUAL:
			if (w->instrumentindex)
			{
				w->mode = I_MODE_NORMAL;
				w->instrumentindex = 1;
				w->fieldpointer = 0;
				w->waveformdrawpointer = 0; redraw();
			} else if (iv) { w->waveformcursor = 0; w->waveformdrawpointer = 0; redraw(); }
			break;
		case I_MODE_NORMAL: case I_MODE_PREVIEW:
			if (w->instrumentindex)
			{
				w->instrumentindex = 1;
				w->fieldpointer = 0;
			} else if (iv) { w->waveformcursor = 0; w->waveformdrawpointer = 0; redraw(); }
			break;
	}
}
void instrumentEnd(instrument *iv)
{
	switch (w->mode)
	{
		case I_MODE_INDICES:
			if (w->instrumentrecv == INST_REC_LOCK_OK)
			{ w->instrument = 254; resetWaveform(); }
			break;
		case I_MODE_VISUAL:
			if (w->instrumentindex)
			{
				w->mode = I_MODE_NORMAL;
				w->instrumentindex = MAX_INSTRUMENT_INDEX;
				w->fieldpointer = 0;
				w->waveformdrawpointer = 0; redraw();
			} else if (iv) { w->waveformcursor = iv->length-1; w->waveformdrawpointer = 0; redraw(); }
			break;
		case I_MODE_NORMAL: case I_MODE_PREVIEW:
			if (w->instrumentindex)
			{
				w->instrumentindex = MAX_INSTRUMENT_INDEX;
				w->fieldpointer = 0;
			} else if (iv) { w->waveformcursor = iv->length-1; w->waveformdrawpointer = 0; redraw(); }
			break;
	}
}

void instrumentInput(int input)
{
	instrument *iv = s->instrumentv[s->instrumenti[w->instrument]];
	int button, x, y;
	unsigned short yo, xo;
	if (!s->instrumenti[w->instrument])
		w->instrumentindex = 0;
	switch (input)
	{
		case '\033':
			switch (getchar())
			{
				case 'O':
					pushInstrumentHistoryIfNew(s->instrumentv[s->instrumenti[w->instrument]]);
					previewNote(NOTE_OFF, INST_VOID, w->channel);
					switch (getchar())
					{
						case 'P': /* xterm f1 */ showTracker(); break;
						case 'Q': /* xterm f2 */ showInstrument(); break;
					} redraw(); break;
				case '[':
					switch (getchar())
					{
						case '[':
							pushInstrumentHistoryIfNew(s->instrumentv[s->instrumenti[w->instrument]]);
							previewNote(NOTE_OFF, INST_VOID, w->channel);
							switch (getchar())
							{
								case 'A': /* linux f1 */ showTracker(); break;
								case 'B': /* linux f2 */ showInstrument(); break;
								case 'E': /* linux f5 */ startPlayback(); break;
							} redraw(); break;
						case 'A': /* up arrow    */ instrumentUpArrow(1, iv); redraw(); break;
						case 'B': /* down arrow  */ instrumentDownArrow(1, iv); redraw(); break;
						case 'D': /* left arrow  */ instrumentLeftArrow(iv); redraw(); break;
						case 'C': /* right arrow */ instrumentRightArrow(iv); redraw(); break;
						case '1': /* mod+arrow / f5 - f8 */
							switch (getchar())
							{
								case '5': /* xterm f5, play  */ startPlayback(); getchar(); break;
								case '7': /* f6, stop        */ stopPlayback(); getchar(); break;
								case ';': /* mod+arrow */
									switch (getchar())
									{
										case '5': /* ctrl+arrow */
											switch (getchar())
											{
												case 'D': /* left  */ instrumentCtrlLeftArrow(iv); redraw(); break;
												case 'C': /* right */ instrumentCtrlRightArrow(iv); redraw(); break;
											} break;
									} break;
								case '~': /* linux home */ instrumentHome(iv); redraw(); break;
							} break;
						case 'H': /* xterm home */ instrumentHome(iv); redraw(); break;
						case '4': /* end        */ if (getchar() == '~') { instrumentEnd(iv); redraw(); } break;
						case '5':
							switch (getchar())
							{
								case '~': /* page up        */ instrumentUpArrow(ws.ws_row>>1, iv); redraw(); break;
								case ';': /* shift+scrollup */ getchar(); break;
							} break;
						case '6':
							switch (getchar())
							{
								case '~': /* page down      */ instrumentDownArrow(ws.ws_row>>1, iv); redraw(); break;
								case ';': /* shift+scrolldn */ getchar(); break;
							} break;
						case 'M': /* mouse */
							button = getchar();
							x = getchar() - 32;
							y = getchar() - 32;
							yo = ws.ws_row - INSTRUMENT_CONTROL_ROW;
							switch (button)
							{
								case WHEEL_UP: case WHEEL_UP_CTRL:
									if (x < INSTRUMENT_INDEX_COLS)
									{
										w->mode = I_MODE_INDICES;
										if (w->instrument > WHEEL_SPEED) w->instrument -= WHEEL_SPEED;
										else                             w->instrument = 0;
										resetWaveform();
									} else
									{
										if (w->mode == I_MODE_INDICES) w->mode = I_MODE_NORMAL;
										if (y < yo && iv)
										{
											w->instrumentindex = 0;
											w->waveformwidth /= 2;
											w->waveformdrawpointer = 0; redraw();
										}
									} break;
								case WHEEL_DOWN: case WHEEL_DOWN_CTRL:
									if (x < INSTRUMENT_INDEX_COLS)
									{
										w->mode = I_MODE_INDICES;
										if (w->instrument < 254 - WHEEL_SPEED) w->instrument += WHEEL_SPEED;
										else                                   w->instrument = 254;
										resetWaveform();
									} else
									{
										if (w->mode == I_MODE_INDICES) w->mode = I_MODE_NORMAL;
										if (y < yo && iv)
										{
											w->instrumentindex = 0;
											w->waveformwidth = MIN(iv->length, w->waveformwidth*2);
											w->waveformdrawpointer = 0; redraw();
										}
									} break;
								case BUTTON_RELEASE: case BUTTON_RELEASE_CTRL:
									switch (w->mode)
									{
										case I_MODE_INDICES:
											if ((short)w->instrument + w->fyoffset < 0)        w->instrument = 0;
											else if ((short)w->instrument + w->fyoffset > 254) w->instrument = 254;
											else                                               w->instrument += w->fyoffset;
											resetWaveform();
											w->fyoffset = 0;
											break;
										case I_MODE_MOUSEADJUST:
											w->mode = I_MODE_NORMAL;
											switch (w->instrumentindex)
											{
												case 1: case 12: break;
												default: w->fieldpointer = 0; break;
											} break;
									} break;
								case BUTTON1_HOLD: case BUTTON1_HOLD_CTRL:
									if (w->mode == I_MODE_MOUSEADJUST)
									{
										if      (x > w->mousex) instrumentAdjustRight(iv, w->instrumentindex, 1);
										else if (x < w->mousex) instrumentAdjustLeft(iv, w->instrumentindex, 1);
										if      (y > w->mousey) instrumentAdjustDown(iv, w->instrumentindex, 1);
										else if (y < w->mousey) instrumentAdjustUp(iv, w->instrumentindex, 1);
										w->mousey = y; w->mousex = x;
									} break;
								case BUTTON1:      case BUTTON3:
								case BUTTON1_CTRL: case BUTTON3_CTRL:
									if (y <= CHANNEL_ROW-2)
									{
										if (x < (ws.ws_col-17) / 2 + 7) showTracker();
										else                            showInstrument();
										break;
									} pushInstrumentHistoryIfNew(s->instrumentv[s->instrumenti[w->instrument]]);

									if (x < INSTRUMENT_INDEX_COLS)
									{
										if (w->mode == I_MODE_VISUAL) w->mode = I_MODE_NORMAL;
										if (button == BUTTON1 || button == BUTTON1_CTRL)
										{
											w->mode = I_MODE_INDICES;
											w->fyoffset = y - w->centre;
										} else
										{
											if ((short)w->instrument + (y - w->centre) < 0)        w->instrument = 0;
											else if ((short)w->instrument + (y - w->centre) > 254) w->instrument = 254;
											else                                                   w->instrument += y - w->centre;

											if (!s->instrumenti[w->instrument]) addInstrument(w->instrument);
											w->popup = 2;
											w->fyoffset = 0;
											w->oldmode = I_MODE_INDICES;
											w->mode = 0;
											w->filebrowserCallback = &sampleLoadCallback;
										} previewNote(NOTE_OFF, INST_VOID, w->channel);
									} else if (iv)
									{
										if (w->mode == I_MODE_INDICES) w->mode = I_MODE_NORMAL;
										if (y < yo)
										{
											w->instrumentindex = 0;
											if (button == BUTTON3 || button == BUTTON3_CTRL)
											{
												if (w->mode != I_MODE_VISUAL)
												{
													w->mode = I_MODE_VISUAL;
													w->waveformvisual = w->waveformcursor;
												}
											} else if (w->mode == I_MODE_VISUAL) w->mode = I_MODE_NORMAL;

											uint32_t offset;
											if (w->waveformcursor < (w->waveformwidth>>1))                   offset = 0;
											else if (w->waveformcursor > iv->length - (w->waveformwidth>>1)) offset = (iv->length - w->waveformwidth)<<1;
											else                                                             offset = (w->waveformcursor<<1) - w->waveformwidth;

											w->waveformcursor = MIN(iv->length-1, (uint32_t)((offset>>2) + (float)(x - INSTRUMENT_INDEX_COLS) / (float)w->waveformw * w->waveformwidth)<<1);
										} else
										{
											if (w->mode == I_MODE_VISUAL) w->mode = I_MODE_NORMAL;
											xo = INSTRUMENT_INDEX_COLS + (ws.ws_col - INSTRUMENT_INDEX_COLS - 58)/2 - 1;
											switch (y - yo)
											{
												case 0:
													if (x - xo < 23)
													{
														w->instrumentindex = 1;
														if (x - xo < 13)      w->fieldpointer = 7;
														else if (x - xo > 20) w->fieldpointer = 0;
														else                  w->fieldpointer = (x - xo - 13)*-1 + 7;
													} else if (x - xo < 45)
													{
														w->instrumentindex = 7;
														if (x - xo < 42) w->fieldpointer = 1;
														else             w->fieldpointer = 0;
													} else if (x - xo < 55)
													{
														w->instrumentindex = 8;
														iv->flags ^= S_FLAG_MIDI;
														if (!(iv->flags&S_FLAG_MIDI) && w->instrumentlockv == INST_GLOBAL_LOCK_OK)
														{
															w->instrumentlocki = s->instrumenti[w->instrument];
															w->instrumentlockv = INST_GLOBAL_INST_MUTE;
														}
													} else w->instrumentindex = 9;
													break;
												case 1:
													if (x - xo < 18) w->instrumentindex = 2;
													else
													{
														w->instrumentindex = 3;
														if (x - xo < 20) w->fieldpointer = 1;
														else             w->fieldpointer = 0;
													} break;
												case 2:
													if (x - xo < 23)        w->instrumentindex = 4;
													else if (x - xo < 39) { w->instrumentindex = 10; iv->flags ^= S_FLAG_PPLOOP; }
													else
													{
														w->instrumentindex = 12;
														if (x - xo < 54)      w->fieldpointer = 3;
														else if (x - xo > 57) w->fieldpointer = 0;
														else                  w->fieldpointer = (x - xo - 54)*-1 + 3;
													} break;
												case 3:
													if (x - xo < 19)
													{
														w->instrumentindex = 5;
														if (x - xo < 17) w->fieldpointer = 1;
														else             w->fieldpointer = 0;
													} else if (x - xo < 23) { w->instrumentindex = 6; iv->flags ^= S_FLAG_SUSTAIN; }
													else if (x - xo < 39)
													{
														w->instrumentindex = 11;
														if (x - xo < 36) w->fieldpointer = 1;
														else             w->fieldpointer = 0;
													} else if (x - xo < 55) { w->instrumentindex = 13; iv->flags ^= S_FLAG_TTEMPO; }
													else
													{
														w->instrumentindex = 14;
														if (x - xo < 57) w->fieldpointer = 1;
														else             w->fieldpointer = 0;
													} break;
											}
											/* enter mouseadjust mode */
											w->oldmode = w->mode;
											w->mode = I_MODE_MOUSEADJUST;
											w->mousey = y; w->mousex = x;
										}
									} if (iv) { w->waveformdrawpointer = 0; redraw(); } break;
							} redraw(); break;
					} break;
				default:
					previewNote(NOTE_OFF, INST_VOID, w->channel);
					switch (w->mode)
					{
						case I_MODE_VISUAL:                      w->mode = I_MODE_NORMAL; w->fieldpointer = 0; w->waveformdrawpointer = 0; redraw(); break;
						case I_MODE_PREVIEW: case I_MODE_ADJUST: w->mode = w->oldmode; w->fieldpointer = 0; break;
						case I_MODE_MOUSEADJUST:                 w->mode = w->oldmode; break;
					} redraw(); break;
			} break;
		default:
			switch (w->mode)
			{
				case I_MODE_INDICES:
					switch (input)
					{ /* set count first */
						case '0': w->count *= 10; w->count += 0; redraw(); return;
						case '1': w->count *= 10; w->count += 1; redraw(); return;
						case '2': w->count *= 10; w->count += 2; redraw(); return;
						case '3': w->count *= 10; w->count += 3; redraw(); return;
						case '4': w->count *= 10; w->count += 4; redraw(); return;
						case '5': w->count *= 10; w->count += 5; redraw(); return;
						case '6': w->count *= 10; w->count += 6; redraw(); return;
						case '7': w->count *= 10; w->count += 7; redraw(); return;
						case '8': w->count *= 10; w->count += 8; redraw(); return;
						case '9': w->count *= 10; w->count += 9; redraw(); return;
						default:
							if (w->chord)
							{
								w->count = MIN(256, w->count);
								switch (w->chord)
								{
									case 'r':
										switch (input)
										{
											case 's': /* start/stop */
												if (!s->instrumenti[w->instrument]) addInstrument(w->instrument);
												toggleRecording(w->instrument);
												break;
											case 'c': /* cancel     */
												if (w->instrumentrecv != INST_REC_LOCK_OK)
													w->instrumentrecv = INST_REC_LOCK_PREP_CANCEL;
												break;
										} break;
									case 'k': /* keyboard macro */ /* ignores w->count */
										w->keyboardmacro = '\0';
										if (input != 'k') changeMacro(input, &w->keyboardmacro);
										break;
								} w->count = 0; redraw();
							} else
							{
								switch (input)
								{
									case '\t': /* normal         */ w->mode = I_MODE_NORMAL; w->fieldpointer = 0; redraw(); break;
									case 'i':  /* preview        */ w->instrumentindex = 0; w->oldmode = w->mode; w->mode = I_MODE_PREVIEW; redraw(); break;
									case 'r':  /* record         */ w->chord = 'r'; redraw(); return;
									case 'k':  /* keyboard macro */ w->chord = 'k'; redraw(); return;
									case 'a':  /* add            */ if (!s->instrumenti[w->instrument]) { addInstrument(w->instrument); redraw(); } break;
									case 'd':  /* delete         */
										if (w->instrumentrecv == INST_REC_LOCK_OK && s->instrumenti[w->instrument])
										{
											yankInstrument(w->instrument);
											delInstrument(w->instrument);
										} redraw(); break;
									case 'y':  /* yank    */ yankInstrument(w->instrument); redraw(); break;
									case 'p':  /* put     */
										if (w->instrumentrecv == INST_REC_LOCK_OK)
										{
											if (!s->instrumenti[w->instrument]) addInstrument(w->instrument);
											w->instrumentlocki = s->instrumenti[w->instrument];
											w->instrumentlockv = INST_GLOBAL_LOCK_PREP_PUT;
										} redraw(); break;
									case '\n': case '\r':
										if (!s->instrumenti[w->instrument]) addInstrument(w->instrument);
										w->popup = 2;
										w->fyoffset = 0;
										w->oldmode = w->mode;
										w->mode = 0;
										w->filebrowserCallback = &sampleLoadCallback;
										redraw(); break;
									case 'e': /* export */
										setCommand(&w->command, &sampleExportCallback, NULL, NULL, 0, "File name: ", "");
										w->mode = 255;
										redraw(); break;
								}
							} break;
					} break;
				case I_MODE_VISUAL:
					switch (input)
					{ /* set count first */
						case '0': w->count *= 10; w->count += 0; redraw(); return;
						case '1': w->count *= 10; w->count += 1; redraw(); return;
						case '2': w->count *= 10; w->count += 2; redraw(); return;
						case '3': w->count *= 10; w->count += 3; redraw(); return;
						case '4': w->count *= 10; w->count += 4; redraw(); return;
						case '5': w->count *= 10; w->count += 5; redraw(); return;
						case '6': w->count *= 10; w->count += 6; redraw(); return;
						case '7': w->count *= 10; w->count += 7; redraw(); return;
						case '8': w->count *= 10; w->count += 8; redraw(); return;
						case '9': w->count *= 10; w->count += 9; redraw(); return;
						default:
							if (w->chord)
							{
								w->count = MIN(256, w->count);
								switch (w->chord)
								{
									case 'z':
										switch (input)
										{
											case 'z': /* reset zoom */ if (iv) { w->waveformwidth = iv->length; w->waveformdrawpointer = 0; } break;
											case 'i': /* zoom in    */ if (iv) { w->waveformwidth /= 2; w->waveformdrawpointer = 0; } break;
											case 'o': /* zoom out   */ if (iv) { w->waveformwidth = MIN(iv->length, w->waveformwidth*2); w->waveformdrawpointer = 0; } break;
										} w->waveformdrawpointer = 0; redraw(); break;
								} w->count = 0; redraw();
							} else
								switch (input)
								{
									case 'v':  /* normal  */ w->mode = I_MODE_NORMAL; w->waveformdrawpointer = 0; redraw(); break;
									case '\t': /* indices */ w->mode = I_MODE_INDICES; w->waveformdrawpointer = 0; redraw(); break;
									case 'z':  /* zoom    */ if (!w->instrumentindex) w->chord = 'z'; redraw(); return;
									case '+': case '=': /* zoom in  */ if (iv) { w->waveformwidth /= 2; w->waveformdrawpointer = 0; redraw(); } break;
									case '-':           /* zoom out */ if (iv) { w->waveformwidth = MIN(iv->length, w->waveformwidth*2); w->waveformdrawpointer = 0; redraw(); } break;
									case 't':  /* trim    */
										if (iv)
										{
											iv->trim[0] = MIN(w->waveformcursor, w->waveformvisual);
											iv->trim[1] = MAX(w->waveformcursor, w->waveformvisual);
											w->mode = I_MODE_NORMAL; w->waveformdrawpointer = 0; redraw();
										} break;
									case 'l':  /* loop    */
										if (iv)
										{
											iv->loop[0] = MIN(w->waveformcursor, w->waveformvisual);
											iv->loop[1] = MAX(w->waveformcursor, w->waveformvisual);
											w->mode = I_MODE_NORMAL; w->waveformdrawpointer = 0; redraw();
										} break;
								}
							break;
					} break;
				case I_MODE_NORMAL:
					if (!w->instrumentindex)
						switch (input)
						{ /* check counts first */
							case '0': w->count *= 10; w->count += 0; redraw(); return;
							case '1': w->count *= 10; w->count += 1; redraw(); return;
							case '2': w->count *= 10; w->count += 2; redraw(); return;
							case '3': w->count *= 10; w->count += 3; redraw(); return;
							case '4': w->count *= 10; w->count += 4; redraw(); return;
							case '5': w->count *= 10; w->count += 5; redraw(); return;
							case '6': w->count *= 10; w->count += 6; redraw(); return;
							case '7': w->count *= 10; w->count += 7; redraw(); return;
							case '8': w->count *= 10; w->count += 8; redraw(); return;
							case '9': w->count *= 10; w->count += 9; redraw(); return;
						}

					if (w->chord)
					{
						w->count = MIN(256, w->count);
						switch (w->chord)
						{
							case 'r': /* record */
								switch (input)
								{
									case 's': /* start/stop */ toggleRecording(w->instrument); break;
									case 'c': /* cancel     */
										if (w->instrumentrecv != INST_REC_LOCK_OK)
											w->instrumentrecv = INST_REC_LOCK_PREP_CANCEL;
										break;
								} break;
							case 'z': /* zoom */
								switch (input)
								{
									case 'z': /* reset zoom */ if (iv) { w->waveformwidth = iv->length; w->waveformdrawpointer = 0; redraw(); } break;
									case 'i': /* zoom in    */ if (iv) { w->waveformwidth /= 2; w->waveformdrawpointer = 0; redraw(); } break;
									case 'o': /* zoom out   */ if (iv) { w->waveformwidth = MIN(iv->length, w->waveformwidth*2); w->waveformdrawpointer = 0; redraw(); } break;
								} break;
							case 'm': /* marker */
								switch (input)
								{
									case 't': /* trim start to cursor */ if (iv) { iv->trim[0] = w->waveformcursor; pushInstrumentHistory(iv); w->waveformdrawpointer = 0; redraw(); } break;
									case 'T': /* trim end to cursor   */ if (iv) { iv->trim[1] = w->waveformcursor; pushInstrumentHistory(iv); w->waveformdrawpointer = 0; redraw(); } break;
									case 'l': /* loop start to cursor */ if (iv) { iv->loop[0] = w->waveformcursor; pushInstrumentHistory(iv); w->waveformdrawpointer = 0; redraw(); } break;
									case 'L': /* loop end to cursor   */ if (iv) { iv->loop[1] = w->waveformcursor; pushInstrumentHistory(iv); w->waveformdrawpointer = 0; redraw(); } break;
								} break;
							case 'j': /* jump */
								switch (input)
								{
									case 't': /* cursor to trim start */ if (iv) { w->waveformcursor = iv->trim[0]; w->waveformdrawpointer = 0; redraw(); } break;
									case 'T': /* cursor to trim end   */ if (iv) { w->waveformcursor = iv->trim[1]; w->waveformdrawpointer = 0; redraw(); } break;
									case 'l': /* cursor to loop start */ if (iv) { w->waveformcursor = iv->loop[0]; w->waveformdrawpointer = 0; redraw(); } break;
									case 'L': /* cursor to loop end   */ if (iv) { w->waveformcursor = iv->loop[1]; w->waveformdrawpointer = 0; redraw(); } break;
								} break;
							case 'k': /* keyboard macro */ /* ignores w->count */
								w->keyboardmacro = '\0';
								if (input != 'k') // kk resets
									changeMacro(input, &w->keyboardmacro);
								break;
							case 'd': /* delete */
								if (input == 'd')
								{
									if (iv->samplelength) free(iv->sampledata);
									iv->sampledata = NULL;
									iv->samplelength = 0;
									iv->channels = 0;
									iv->length = 0;
									iv->c5rate = 0;
									iv->trim[0] = 0;
									iv->trim[1] = 0;
									iv->loop[0] = 0;
									iv->loop[1] = 0;
								}
								break;
						} w->count = 0; redraw();
					} else
					{
						if (w->instrumentindex)
							switch (input)
							{
								case '\n': case '\r':
									switch (w->instrumentindex)
									{
										case 6:  iv->flags ^= S_FLAG_SUSTAIN; break;
										case 8:
											iv->flags ^= S_FLAG_MIDI;
											if (!(iv->flags&S_FLAG_MIDI) && w->instrumentlockv == INST_GLOBAL_LOCK_OK)
											{
												w->instrumentlocki = s->instrumenti[w->instrument];
												w->instrumentlockv = INST_GLOBAL_INST_MUTE;
											} break;
										case 10: iv->flags ^= S_FLAG_PPLOOP;  break;
										case 13: iv->flags ^= S_FLAG_TTEMPO;  break;
										default: w->oldmode = w->mode; w->mode = I_MODE_ADJUST; break;
									}
									redraw(); break;
								case '\t': /* indices          */ w->mode = I_MODE_INDICES; redraw(); break;
								case 'i':  /* preview          */ w->oldmode = w->mode; w->mode = I_MODE_PREVIEW; redraw(); break;
								case 'r':  /* record           */ w->chord = 'r'; redraw(); return;
								case 'k':  /* keyboard macro   */ w->chord = 'k'; redraw(); return;
								case 'u':  /* undo             */ popInstrumentHistory(s->instrumenti[w->instrument]); redraw(); break;
								case 18:   /* ^R redo          */ unpopInstrumentHistory(s->instrumenti[w->instrument]); redraw(); break;
								case 127: case 8: /* backspace */ if (iv) instrumentDecFieldPointer(w->instrumentindex); redraw(); break;
								case ' ':         /* space     */ if (iv) instrumentIncFieldPointer(w->instrumentindex); redraw(); break;
								case 1:  /* ^a */
									if (iv)
										switch (w->instrumentindex)
										{
											case 1:  iv->c5rate++; break;
											case 2:  iv->bitdepth++; break;
											case 3:  iv->samplerate++; break;
											case 5:  iv->envelope++; break;
											// case 7:  iv->defgain+=17; break;
											case 9:  iv->midichannel++; break;
											case 11: iv->loopramp++; break;
											case 12: iv->cyclelength++; break;
											case 14: iv->pitchshift++; break;
										}
									redraw(); break;
								case 24: /* ^x */
									if (iv)
										switch (w->instrumentindex)
										{
											case 1:  iv->c5rate--; break;
											case 2:  iv->bitdepth--; break;
											case 3:  iv->samplerate--; break;
											case 5:  iv->envelope--; break;
											// case 7:  iv->defgain-=17; break;
											case 9:  iv->midichannel--; break;
											case 11: iv->loopramp--; break;
											case 12: iv->cyclelength--; break;
											case 14: iv->pitchshift--; break;
										}
									redraw(); break;
								case '0':           inputInstrumentHex(w->instrumentindex, iv, 0);  redraw(); break;
								case '1':           inputInstrumentHex(w->instrumentindex, iv, 1);  redraw(); break;
								case '2':           inputInstrumentHex(w->instrumentindex, iv, 2);  redraw(); break;
								case '3':           inputInstrumentHex(w->instrumentindex, iv, 3);  redraw(); break;
								case '4':           inputInstrumentHex(w->instrumentindex, iv, 4);  redraw(); break;
								case '5':           inputInstrumentHex(w->instrumentindex, iv, 5);  redraw(); break;
								case '6':           inputInstrumentHex(w->instrumentindex, iv, 6);  redraw(); break;
								case '7':           inputInstrumentHex(w->instrumentindex, iv, 7);  redraw(); break;
								case '8':           inputInstrumentHex(w->instrumentindex, iv, 8);  redraw(); break;
								case '9':           inputInstrumentHex(w->instrumentindex, iv, 9);  redraw(); break;
								case 'A': case 'a': inputInstrumentHex(w->instrumentindex, iv, 10); redraw(); break;
								case 'B': case 'b': inputInstrumentHex(w->instrumentindex, iv, 11); redraw(); break;
								case 'C': case 'c': inputInstrumentHex(w->instrumentindex, iv, 12); redraw(); break;
								case 'D': case 'd': inputInstrumentHex(w->instrumentindex, iv, 13); redraw(); break;
								case 'E': case 'e': inputInstrumentHex(w->instrumentindex, iv, 14); redraw(); break;
								case 'F': case 'f': inputInstrumentHex(w->instrumentindex, iv, 15); redraw(); break;
							}
						else
							switch (input)
							{
								case '\n': case '\r':
									if (!iv) addInstrument(w->instrument);
									w->popup = 2;
									w->fyoffset = 0;
									w->oldmode = w->mode;
									w->mode = 0;
									w->filebrowserCallback = &sampleLoadCallback;
									redraw(); break;
								case 'v':  /* visual           */ w->mode = I_MODE_VISUAL; w->waveformvisual = w->waveformcursor; w->waveformdrawpointer = 0; redraw(); break;
								case 'z':  /* zoom             */ w->chord = 'z'; redraw(); return;
								case '+': case '=': /* zoom in */ if (iv) { w->waveformwidth /= 2; w->waveformdrawpointer = 0; redraw(); } break;
								case '-':  /* zoom out         */ if (iv) { w->waveformwidth = MIN(iv->length, w->waveformwidth*2); w->waveformdrawpointer = 0; redraw(); } break;
								case 'm':  /* marker           */ w->chord = 'm'; redraw(); return;
								case 'j':  /* jump             */ w->chord = 'j'; redraw(); return;
								case 'd':  /* delete           */ w->chord = 'd'; redraw(); return;
								case '\t': /* indices          */ w->mode = I_MODE_INDICES; redraw(); break;
								case 'i':  /* preview          */ w->oldmode = w->mode; w->mode = I_MODE_PREVIEW; redraw(); break;
								case 'r':  /* record           */ w->chord = 'r'; redraw(); return;
								case 'k':  /* keyboard macro   */ w->chord = 'k'; redraw(); return;
								case 'u':  /* undo             */ popInstrumentHistory(s->instrumenti[w->instrument]); redraw(); break;
								case 18:   /* ^R redo          */ unpopInstrumentHistory(s->instrumenti[w->instrument]); redraw(); break;
							}
					} break;
				case I_MODE_ADJUST: case I_MODE_MOUSEADJUST:
					switch (input)
					{
						case '\n': case '\r': w->mode = I_MODE_NORMAL; redraw(); break;
					} break;
				case I_MODE_PREVIEW:
					switch (input)
					{
						case '\n': case '\r':
							if (!w->instrumentindex)
							{
								if (!s->instrumenti[w->instrument]) addInstrument(w->instrument);
								w->popup = 2;
								w->fyoffset = 0;
								w->oldmode = w->mode;
								w->mode = 0;
								w->filebrowserCallback = &sampleLoadCallback;
								redraw();
							} break;
						case '0': w->octave = 0; redraw(); break;
						case '1': w->octave = 1; redraw(); break;
						case '2': w->octave = 2; redraw(); break;
						case '3': w->octave = 3; redraw(); break;
						case '4': w->octave = 4; redraw(); break;
						case '5': w->octave = 5; redraw(); break;
						case '6': w->octave = 6; redraw(); break;
						case '7': w->octave = 7; redraw(); break;
						case '8': w->octave = 8; redraw(); break;
						case '9': w->octave = 9; redraw(); break;
						default: previewNote(charToNote(input), w->instrument, w->channel); break;
					} break;
			} break;
	}
	if (w->count) { w->count = 0; redraw(); }
	if (w->chord) { w->chord = '\0'; redraw(); }
}
