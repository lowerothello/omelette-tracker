static void samplerInstUICommonCallback(short x, short y, Instrument *iv, uint8_t index)
{
	switch (index)
	{
		case 0:
			printf("\033[%d;%dHC5 rate: [        ]", y, x);
			addControlInt(&cc, x+10, y, &iv->sample->rate, 8, 0x0, 0xffffffff, iv->sample->defrate, 0, 0, instrumentSamplerControlCallback, NULL);
			break;
		case 1:
			printf("\033[%d;%dHquality: [ ][ ][  ]", y, x);
			addControlInt(&cc, x+10, y, &iv->interpolate, 0, 0,   1,      0,    0, 0, instrumentSamplerControlCallback, NULL);
			addControlInt(&cc, x+13, y, &iv->bitdepth,    1, 0x0, 0xf,    0xf,  0, 0, instrumentSamplerControlCallback, NULL);
			addControlInt(&cc, x+16, y, &iv->samplerate,  2, 0x0, 0xff,   0xff, 0, 0, instrumentSamplerControlCallback, NULL);
			break;
		case 2:
			printf("\033[%d;%dHgain env:    [    ]", y, x);
			addControlInt(&cc, x+14, y, &iv->envelope, 4, 0x0, 0xffff, 0x00f0, 0, 0, instrumentSamplerControlCallback, NULL);
			break;
		case 3:
			printf("\033[%d;%dHchannel:   [      ]", y, x);
			addControlInt(&cc, x+12, y, &iv->channelmode, 1, 0, 4, 0, 6, 5, instrumentSamplerControlCallback, NULL);
				addScalePointInt(&cc, "STEREO", SAMPLE_CHANNELS_STEREO);
				addScalePointInt(&cc, "  LEFT", SAMPLE_CHANNELS_LEFT  );
				addScalePointInt(&cc, " RIGHT", SAMPLE_CHANNELS_RIGHT );
				addScalePointInt(&cc, "   MIX", SAMPLE_CHANNELS_MIX   );
				addScalePointInt(&cc, "  SWAP", SAMPLE_CHANNELS_SWAP  );
			break;
		case 4:
			printf("\033[%d;%dHgain:      [ ][   ]", y, x);
			addControlInt(&cc, x+12, y, &iv->invert, 0, 0,    1,   0, 0, 0, instrumentSamplerControlCallback, NULL);
			addControlInt(&cc, x+15, y, &iv->gain,   3, -128, 127, 0, 0, 0, instrumentSamplerControlCallback, NULL);
			break;
		case 5:
			printf("\033[%d;%dH[    ]:    [  ][  ]", y, x);
			addControlInt(&cc, x+1, y, &iv->filtermode, 1, 0, 7, 0, 4, 8, instrumentSamplerControlCallback, NULL);
				addScalePointInt(&cc, "LP12", FILTER_MODE_LP12);
				addScalePointInt(&cc, "HP12", FILTER_MODE_HP12);
				addScalePointInt(&cc, "BP12", FILTER_MODE_BP12);
				addScalePointInt(&cc, "NT12", FILTER_MODE_NT12);
				addScalePointInt(&cc, "LP24", FILTER_MODE_LP24);
				addScalePointInt(&cc, "HP24", FILTER_MODE_HP24);
				addScalePointInt(&cc, "BP24", FILTER_MODE_BP24);
				addScalePointInt(&cc, "NT24", FILTER_MODE_NT24);
			addControlInt(&cc, x+10, y, &iv->filtercutoff,    2, 0x0, 0xff, 0xff, 0, 0, instrumentSamplerControlCallback, NULL);
			addControlInt(&cc, x+14, y, &iv->filterresonance, 2, 0x0, 0xff, 0x0,  0, 0, instrumentSamplerControlCallback, NULL);
			break;
		case 6:
			printf("\033[%d;%dH + [           ] + ", y, x);
			addControlInt(&cc, x+4, y, &iv->algorithm, 1, 0, 4, 1, 10, 5, instrumentSamplerControlCallback, NULL);
				addScalePointInt(&cc, "   MINIMAL", INST_ALG_SIMPLE   );
				addScalePointInt(&cc, "    CYCLIC", INST_ALG_CYCLIC   );
				addScalePointInt(&cc, "     TONAL", INST_ALG_TONAL    );
				addScalePointInt(&cc, "      BEAT", INST_ALG_BEAT     );
			break;
	}
	// printf("\033[%d;%dHC5 rate: [        ]  channel: [      ]", y+0, xx);
	// printf("\033[%d;%dHquality: [ ][ ][  ]  gain:    [ ][   ]", y+1, xx);
	// printf("\033[%d;%dHgain env:    [    ]  [    ]:  [  ][  ]", y+2, xx);
}
void initInstUICommonSamplerBlock(InstUIBlock *block)
{
	block->count = 7;
	block->callback = samplerInstUICommonCallback;
}

static void samplerInstUIRangeCallback(short x, short y, Instrument *iv, uint8_t index)
{
	switch (index)
	{
		case 0: printf("\033[%d;%dHoffset:  [        ]", y, x); addControlInt(&cc, x+10, y, &iv->trimstart,  8, 0, iv->sample->length-1, 0,                    0, 0, instrumentSamplerControlCallback, NULL); break;
		case 1: printf("\033[%d;%dHlength:  [        ]", y, x); addControlInt(&cc, x+10, y, &iv->trimlength, 8, 0, iv->sample->length-1, iv->sample->length-1, 0, 0, instrumentSamplerControlCallback, NULL); break;
		case 2: printf("\033[%d;%dHloop:    [        ]", y, x); addControlInt(&cc, x+10, y, &iv->looplength, 8, 0, iv->trimlength,       0,                    0, 0, instrumentSamplerControlCallback, NULL); break;
		case 3: printf("\033[%d;%dHframe:         [  ]", y, x); addControlInt(&cc, x+10, y, &iv->frame,      2, 0,                  255, 0,                    0, 0, instrumentSamplerControlCallback, NULL); break;
		case 4:
			printf("\033[%d;%dHpp/ramp:    [ ][  ]", y, x);
			addControlInt(&cc, x+13, y, &iv->pingpong, 0, 0,   1,    0, 0, 0, instrumentSamplerControlCallback, NULL);
			addControlInt(&cc, x+16, y, &iv->loopramp, 2, 0x0, 0xff, 0, 0, 0, instrumentSamplerControlCallback, NULL);
			break;
	}
}
void initInstUIRangeSamplerBlock(InstUIBlock *block)
{
	block->count = 5;
	block->callback = samplerInstUIRangeCallback;
}

static void samplerInstUIGranularCallback(short x, short y, Instrument *iv, uint8_t index)
{
	switch (index)
	{
		case 0: printf("\033[%d;%dHunison:   [ ][ ][ ]", y, x); break;
		case 1:
			printf("\033[%d;%dHrev/ramp:   [  ][ ]", y, x);
			addControlInt(&cc, x+12, y, &iv->granular.reversegrains,     2, 0x0,    0xff,   0,      0, 0, instrumentSamplerControlCallback, NULL);
			addControlInt(&cc, x+16, y, &iv->granular.rampgrains,        1, 0x0,    0xf,    8,      0, 0, instrumentSamplerControlCallback, NULL);
			break;
		case 2:
			printf("\033[%d;%dHcycle:   [    ][  ]", y, x);
			addControlInt(&cc, x+9,  y, &iv->granular.cyclelength,       4, 0x0,    0xffff, 0x3fff, 0, 0, instrumentSamplerControlCallback, NULL);
			addControlInt(&cc, x+15, y, &iv->granular.cyclelengthjitter, 2, 0x0,    0xff,   0x0,    0, 0, instrumentSamplerControlCallback, NULL);
			break;
		case 3:
			printf("\033[%d;%dHtime     [ ][     ]", y, x);
			addControlInt(&cc, x+9,  y, &iv->granular.notestretch,       0, 0,      1,      0,      0, 0, instrumentSamplerControlCallback, NULL);
			addControlInt(&cc, x+12, y, &iv->granular.timestretch,       5, 0x8bff, 0x7bff, 0,      0, 0, instrumentSamplerControlCallback, NULL);
			break;
		case 4:
			printf("\033[%d;%dHpan jitter:    [  ]", y, x);
			addControlInt(&cc, x+15, y, &iv->granular.panjitter, 2, 0x00, 0xff, 0, 0, 0, instrumentSamplerControlCallback, NULL);
			break;
		case 5:
			printf("\033[%d;%dHptr jitter:    [  ]", y, x);
			addControlInt(&cc, x+15, y, &iv->granular.ptrjitter, 2, 0x00, 0xff, 0, 0, 0, instrumentSamplerControlCallback, NULL);
			break;
	}
}
void initInstUIGranularSamplerBlock(InstUIBlock *block)
{
	block->count = 6;
	block->callback = samplerInstUIGranularCallback;
}

static void samplerInstUIPitchCallback(short x, short y, Instrument *iv, uint8_t index)
{
	switch (index)
	{
		case 0:
			printf("\033[%d;%dHpitch: [   ][     ]", y, x);
			addControlInt(&cc, x+8 , y, &iv->granular.pitchstereo,       3, -128,   127,    0, 0, 0, instrumentSamplerControlCallback, NULL);
			addControlInt(&cc, x+13, y, &iv->granular.pitchshift,        5, 0x8bff, 0x7bff, 0, 0, 0, instrumentSamplerControlCallback, NULL);
			break;
		case 1:
			printf("\033[%d;%dHjitter:  [  ][ ][ ]", y, x);
			addControlInt(&cc, x+10, y, &iv->granular.pitchjitter,       2, 0x00,   0xff,   0, 0, 0, instrumentSamplerControlCallback, NULL);
			addControlInt(&cc, x+14, y, &iv->granular.pitchoctaverange,  1, 0x0,    0xf,    0, 0, 0, instrumentSamplerControlCallback, NULL);
			addControlInt(&cc, x+17, y, &iv->granular.pitchoctavechance, 1, 0x0,    0xf,    0, 0, 0, instrumentSamplerControlCallback, NULL);
			break;
	}
}
void initInstUIPitchSamplerBlock(InstUIBlock *block)
{
	block->count = 2;
	block->callback = samplerInstUIPitchCallback;
}

