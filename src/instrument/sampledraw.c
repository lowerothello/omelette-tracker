void drawInstrumentSampler(Instrument *iv)
{
	short x = INSTRUMENT_INDEX_COLS + ((ws.ws_col - INSTRUMENT_INDEX_COLS - 38)>>1) +1;
	short y = ws.ws_row - 12;

	if (iv->algorithm == INST_ALG_MIDI) drawMidi(&cc, iv);
	else
	{
		if (w->showfilebrowser) drawBrowser(fbstate);
		else
		{
			clearControls(&cc);

			printf("\033[%d;%dH%ds", TRACK_ROW - 1, ws.ws_col - 10, (int)((iv->sample->length * (float)iv->sample->rate/(float)samplerate) / samplerate) + 1);

			drawWaveform(iv, x, y);
			printf("\033[%d;%dHC5 rate: [        ]  track: [      ]", y+0, x);
			printf("\033[%d;%dHquality: [ ][ ][  ]  gain:    [ ][   ]", y+1, x);
			printf("\033[%d;%dHgain env:    [    ]  [    ]:  [  ][  ]", y+2, x);

			addControlInt(&cc, x+10, y+0, &iv->sample->rate, 8, 0x0, 0xffffffff, iv->sample->defrate, 0, 0, instrumentSamplerControlCallback, NULL);
			addControlInt(&cc, x+10, y+1, &iv->interpolate, 0, 0,   1,      0,      0, 0, instrumentSamplerControlCallback, NULL);
			addControlInt(&cc, x+13, y+1, &iv->bitdepth,    1, 0x0, 0xf,    0xf,    0, 0, instrumentSamplerControlCallback, NULL);
			addControlInt(&cc, x+16, y+1, &iv->samplerate,  2, 0x0, 0xff,   0xff,   0, 0, instrumentSamplerControlCallback, NULL);
			addControlInt(&cc, x+14, y+2, &iv->envelope,    4, 0x0, 0xffff, 0x00f0, 0, 0, instrumentSamplerControlCallback, NULL);
			addControlInt(&cc, x+31, y+0, &iv->trackmode, 1, 0,   4,      0,      6, 5, instrumentSamplerControlCallback, NULL);
				addScalePointInt(&cc, "STEREO", SAMPLE_TRACKS_STEREO);
				addScalePointInt(&cc, "  LEFT", SAMPLE_TRACKS_LEFT  );
				addScalePointInt(&cc, " RIGHT", SAMPLE_TRACKS_RIGHT );
				addScalePointInt(&cc, "   MIX", SAMPLE_TRACKS_MIX   );
				addScalePointInt(&cc, "  SWAP", SAMPLE_TRACKS_SWAP  );
			addControlInt(&cc, x+31, y+1, &iv->invert,     0, 0,    1,   0, 0, 0, instrumentSamplerControlCallback, NULL);
			addControlInt(&cc, x+34, y+1, &iv->gain,       3, -128, 127, 0, 0, 0, instrumentSamplerControlCallback, NULL);
			addControlInt(&cc, x+22, y+2, &iv->filtermode, 1, 0,    7,   0, 4, 8, instrumentSamplerControlCallback, NULL);
				addScalePointInt(&cc, "LP12", 0); /* no enums for these cos they make more sense without */
				addScalePointInt(&cc, "HP12", 1);
				addScalePointInt(&cc, "BP12", 2);
				addScalePointInt(&cc, "NT12", 3);
				addScalePointInt(&cc, "LP24", 4);
				addScalePointInt(&cc, "HP24", 5);
				addScalePointInt(&cc, "BP24", 6);
				addScalePointInt(&cc, "NT24", 7);
			addControlInt(&cc, x+31, y+2, &iv->filtercutoff,    2, 0x0, 0xff, 0xff, 0, 0, instrumentSamplerControlCallback, NULL);
			addControlInt(&cc, x+35, y+2, &iv->filterresonance, 2, 0x0, 0xff, 0x0,  0, 0, instrumentSamplerControlCallback, NULL);

			addControlInt(&cc, x+4, y+4, &iv->algorithm, 1, 0, 4, 1, 10, 5, instrumentSamplerControlCallback, NULL);
				addScalePointInt(&cc, "   MINIMAL", INST_ALG_SIMPLE   );
				addScalePointInt(&cc, "    CYCLIC", INST_ALG_CYCLIC   );
				addScalePointInt(&cc, "     TONAL", INST_ALG_TONAL    );
				addScalePointInt(&cc, "      BEAT", INST_ALG_BEAT     );
				addScalePointInt(&cc, " WAVETABLE", INST_ALG_WAVETABLE);

			switch (iv->algorithm)
			{
				case INST_ALG_SIMPLE: printf("\033[%d;%dH + [          ] + ", y+4, x); break;
				case INST_ALG_CYCLIC:    drawCyclic   (&cc, iv); break;
				case INST_ALG_TONAL:     drawTonal    (&cc, iv); break;
				case INST_ALG_BEAT:      drawBeat     (&cc, iv); break;
				case INST_ALG_WAVETABLE: drawWavetable(&cc, iv); break;
			}

			drawControls(&cc);
		}
	}
}
