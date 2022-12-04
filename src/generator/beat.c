void drawBeat(ControlState *cc, Instrument *iv, short x)
{
	short y = ws.ws_row - 12;

	drawWaveform(iv);
	printf("\033[%d;%dH + [          ] +   offset: [        ]", y+4, x);
	printf("\033[%d;%dHsensitivity:  [  ]  length: [        ]", y+5, x);
	printf("\033[%d;%dHdecay:        [  ]  loop:   [        ]", y+6, x);
	printf("\033[%d;%dH                    pp/ramp:   [ ][  ]", y+7, x);
	printf("\033[%d;%dHtime:   [ ][     ]                    ", y+8, x);
	printf("\033[%d;%dHpitch [   ][     ]                    ", y+9, x);

	/* granular */
	addControlInt(cc, x+15, y+5, &iv->granular.beatsensitivity, 2, 0x0, 0xff, 0x80, 0, 0, instrumentSamplerControlCallback, NULL);
	addControlInt(cc, x+15, y+6, &iv->granular.beatdecay,       2, 0x0, 0xff, 0xff, 0, 0, instrumentSamplerControlCallback, NULL);

	addControlInt(cc, x+9,  y+8, &iv->granular.notestretch, 0, 0,      1,      0, 0, 0, instrumentSamplerControlCallback, NULL);
	addControlInt(cc, x+12, y+8, &iv->granular.timestretch, 5, 0x8bff, 0x7bff, 0, 0, 0, instrumentSamplerControlCallback, NULL);
	addControlInt(cc, x+7,  y+9, &iv->granular.pitchstereo, 3, -128,   127,    0, 0, 0, instrumentSamplerControlCallback, NULL);
	addControlInt(cc, x+12, y+9, &iv->granular.pitchshift,  5, 0x8bff, 0x7bff, 0, 0, 0, instrumentSamplerControlCallback, NULL);

	/* range */
	iv->trimlength = MIN(iv->trimlength, iv->sample->length-1);
	iv->looplength = MIN(iv->looplength, iv->sample->length-1);
	addControlInt(cc, x+29, y+4, &iv->trimstart,  8, 0x0, iv->sample->length-1,                    0, 0, 0, instrumentSamplerControlCallback, NULL);
	addControlInt(cc, x+29, y+5, &iv->trimlength, 8, 0x0, iv->sample->length-1, iv->sample->length-1, 0, 0, instrumentSamplerControlCallback, NULL);
	addControlInt(cc, x+29, y+6, &iv->looplength, 8, 0x0, iv->trimlength,                          0, 0, 0, instrumentSamplerControlCallback, NULL);
	addControlInt(cc, x+32, y+7, &iv->pingpong,   0, 0,   1,    0, 0, 0, instrumentSamplerControlCallback, NULL);
	addControlInt(cc, x+35, y+7, &iv->loopramp,   2, 0x0, 0xff, 0, 0, 0, instrumentSamplerControlCallback, NULL);
}
