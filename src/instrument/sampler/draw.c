static void instUISampleCallback(short x, short y, Inst *iv, uint8_t index)
{
	InstSamplerState *s = iv->state;
	Sample *sample = (*s->sample)[w->sample];
	if (!sample) return;

	switch (index)
	{
		case 0:
			printf("\033[%d;%dHrate:", y, x);
			addControlInt(x+8, y, &sample->rate, 8, 0x0, 0xffffffff, sample->defrate, 0, 0, (void(*)(void*))instControlCallback, NULL);
			break;
		case 1:
			printf("\033[%d;%dHgain:", y, x);
			addControlInt(x+11, y, &sample->invert, 0,    0,   1,    0, 0, 0, (void(*)(void*))instControlCallback, NULL);
			addControlInt(x+14, y, &sample->gain,   2,    0x0, 0xff, 0, 0, 0, (void(*)(void*))instControlCallback, NULL);
			break;
		case 2: printf("\033[%d;%dHstart:", y, x); addControlInt(x+8, y, &sample->trimstart,  8, 0, sample->length-1, 0,                0, 0, (void(*)(void*))instControlCallback, NULL); break;
		case 3: printf("\033[%d;%dHdelta:", y, x); addControlInt(x+8, y, &sample->trimlength, 8, 0, sample->length-1, sample->length-1, 0, 0, (void(*)(void*))instControlCallback, NULL); break;
		case 4: printf("\033[%d;%dHloop:", y, x);  addControlInt(x+8, y, &sample->looplength, 8, 0, sample->length-1, 0,                0, 0, (void(*)(void*))instControlCallback, NULL); break;
		case 5:
			printf("\033[%d;%dHpp/ramp:", y, x);
			addControlInt(x+11, y, &sample->pingpong, 0,   0,    1, 0, 0, 0, (void(*)(void*))instControlCallback, NULL);
			addControlInt(x+14, y, &sample->loopramp, 2, 0x0, 0xff, 0, 0, 0, (void(*)(void*))instControlCallback, NULL);
			break;
	}
}
const InstUI sampleInstUI = { 17, 6, 8, 3, instUISampleCallback, };

static void instUICyclicCallback(short x, short y, Inst *iv, uint8_t index)
{
	InstSamplerState *s = iv->state;
	switch (index)
	{
		case 0:
			printf("\033[%d;%dHquality:", y, x);
			addControlInt(x+10, y, &s->interpolate, 0,   0,    1,    0, 0, 0, (void(*)(void*))instControlCallback, NULL);
			addControlInt(x+13, y, &s->bitredux,    1, 0x0,  0xf,  0xf, 0, 0, (void(*)(void*))instControlCallback, NULL);
			addControlInt(x+16, y, &s->rateredux,   2, 0x0, 0xff, 0xff, 0, 0, (void(*)(void*))instControlCallback, NULL);
			break;
		case 1:
			printf("\033[%d;%dHgain env:", y, x);
			addControlInt(x+14, y, &s->envelope, 4, 0x0, 0xffff, 0x00f0, 0, 0, (void(*)(void*))instControlCallback, NULL);
			break;
		case 2:
			printf("\033[%d;%dHchannel:", y, x);
			addControlInt(x+12, y, &s->channelmode, 1, 0, 4, 0, 6, 5, (void(*)(void*))instControlCallback, NULL);
				addScalePointInt("STEREO", SAMPLE_CHANNELS_STEREO);
				addScalePointInt("  LEFT", SAMPLE_CHANNELS_LEFT  );
				addScalePointInt(" RIGHT", SAMPLE_CHANNELS_RIGHT );
				addScalePointInt("   MIX", SAMPLE_CHANNELS_MIX   );
				addScalePointInt("  SWAP", SAMPLE_CHANNELS_SWAP  );
			break;
		case 3:
			printf("\033[%d;%dHgain:", y, x);
			addControlInt(x+15, y, &s->gain, 3, -128, 127, 0, 0, 0, (void(*)(void*))instControlCallback, NULL);
			break;
		case 4:
			printf("\033[%d;%dHrev/ramp:", y, x);
			addControlInt(x+14, y, &s->reverse, 0, 0,   0,   0, 0, 0, (void(*)(void*))instControlCallback, NULL);
			addControlInt(x+17, y, &s->ramp,    1, 0x0, 0xf, 8, 0, 0, (void(*)(void*))instControlCallback, NULL);
			break;
		case 5:
			printf("\033[%d;%dHcycle:", y, x);
			addControlInt(x+10, y, &s->cyclelength,       4, 0x0, 0xffff, 0x3fff, 0, 0, (void(*)(void*))instControlCallback, NULL);
			addControlInt(x+16, y, &s->cyclelengthjitter, 2, 0x0, 0xff,   0x0,    0, 0, (void(*)(void*))instControlCallback, NULL);
			break;
		case 6:
			printf("\033[%d;%dHtransient:", y, x);
			addControlInt(x+16, y, &s->transientsensitivity, 2, 0x0, 0xff,   0x0,    0, 0, (void(*)(void*))instControlCallback, NULL);
			break;
		case 7:
			printf("\033[%d;%dHtime:", y, x);
			addControlInt(x+10, y, &s->notestretch, 0, 0,      1,      0, 0, 0, (void(*)(void*))instControlCallback, NULL);
			addControlInt(x+13, y, &s->timestretch, 5, 0x8bff, 0x7bff, 0, 0, 0, (void(*)(void*))instControlCallback, NULL);
			break;
		case 8:
			printf("\033[%d;%dHframe:", y, x);
			addControlInt(x+16, y, &s->frame, 2, 0, 255, 0, 0, 0, (void(*)(void*))instControlCallback, NULL);
			break;
		case 9:
			printf("\033[%d;%dHpitch:", y, x);
			addControlInt(x+9 , y, &s->pitchjitter, 2, 0x0,    0xff,   0, 0, 0, (void(*)(void*))instControlCallback, NULL);
			addControlInt(x+13, y, &s->pitchshift,  5, 0x8bff, 0x7bff, 0, 0, 0, (void(*)(void*))instControlCallback, NULL);
			break;
		case 10:
			printf("\033[%d;%dHfmnt:", y, x);
			addControlInt(x+8 , y, &s->formantstereo, 3, -128,   127,    0, 0, 0, (void(*)(void*))instControlCallback, NULL);
			addControlInt(x+13, y, &s->formantshift,  5, 0x8bff, 0x7bff, 0, 0, 0, (void(*)(void*))instControlCallback, NULL);
			break;
	}
}
const InstUI cyclicInstUI = { 19, 11, 18, 2, instUICyclicCallback, };

static void samplerDraw(Inst *iv, short x, short y, short width, short height, short minx, short maxx)
{
	InstSamplerState *s = iv->state;

	short rows, cols;
	if (!(cols = getMaxInstUICols(&cyclicInstUI, width))) return;
	if (!(rows = getInstUIRows(&cyclicInstUI, cols))) return;

	short wh = MAX(height - rows, INSTUI_WAVEFORM_MIN);

	drawBoundingBox(x, y, width, wh - 1, minx, maxx, 1, ws.ws_row);

	short sample_rows = getInstUIRows(&sampleInstUI, getMaxInstUICols(&sampleInstUI, width - 5));

	short whh = wh - sample_rows;

	/* multisample indices */
	char buffer[5];
	short siy;
	for (uint8_t i = 0; i < SAMPLE_MAX; i++)
	{
		siy = y + (whh>>1) - w->sample + i;
		if (siy > y && siy < y + (whh - 1))
		{ /* vertical bounds limiting */
			if (i == w->sample) printf("\033[1;7m");
			if ((*s->sample)[i]) snprintf(buffer, 5, " %01x ", i);
			else                 snprintf(buffer, 5, " . ");
			printCulling(buffer, x + ((width - 3)>>1) + (i - w->sample)*5, y+1, x+1, x + width-1);
			if (i == w->sample) printf("\033[22;27m");
		}
	}

	if ((*s->sample)[w->sample])
	{
		drawWaveform((*s->sample)[w->sample], x+1, y+2, width-1, whh-3);
		drawInstUI(&sampleInstUI, iv, x+1, width-2, y+whh-1, 0, sample_rows);
	} else
	{
		resizeBrowser(fbstate, x + 3, y + 2, width - 3, wh - 3);
		drawBrowser(fbstate);
	}

	y += wh;

	drawInstUI(&cyclicInstUI, iv, x, width, y, 0, rows);
}
