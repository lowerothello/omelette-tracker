void instUISampleCallback(short x, short y, Inst *iv, uint8_t index)
{
	InstSamplerState *s = iv->state;
	Sample *sample = (*s->sample)[w->sample];
	if (!sample) return;

	switch (index)
	{
		case 0:
			printf("\033[%d;%dHrate:  [        ]", y, x);
			addControlInt(x+8, y, &sample->rate, 8, 0x0, 0xffffffff, sample->defrate, 0, 0, (void(*)(void*))instControlCallback, NULL);
			break;
		case 1:
			printf("\033[%d;%dHgain:    [ ][   ]", y, x);
			addControlInt(x+10, y, &sample->invert, 0,    0,   1, 0, 0, 0, (void(*)(void*))instControlCallback, NULL);
			// addControlInt(x+13, y, &s->gain,       3, -128, 127, 0, 0, 0, (void(*)(void*))instControlCallback, NULL);
			break;
		case 2: printf("\033[%d;%dHstart: [        ]", y, x); addControlInt(x+8, y, &sample->trimstart,  8, 0, sample->length-1  , 0,                0, 0, (void(*)(void*))instControlCallback, NULL); break;
		case 3: printf("\033[%d;%dHdelta: [        ]", y, x); addControlInt(x+8, y, &sample->trimlength, 8, 0, sample->length-1  , sample->length-1, 0, 0, (void(*)(void*))instControlCallback, NULL); break;
		case 4: printf("\033[%d;%dHloop:  [        ]", y, x); addControlInt(x+8, y, &sample->looplength, 8, 0, sample->trimlength, 0,                0, 0, (void(*)(void*))instControlCallback, NULL); break;
		case 5:
			printf("\033[%d;%dHpp/ramp:  [ ][  ]", y, x);
			addControlInt(x+11, y, &sample->pingpong, 0,   0,    1, 0, 0, 0, (void(*)(void*))instControlCallback, NULL);
			addControlInt(x+14, y, &sample->loopramp, 2, 0x0, 0xff, 0, 0, 0, (void(*)(void*))instControlCallback, NULL);
			break;
	}
}

void instUICyclicCallback(short x, short y, Inst *iv, uint8_t index)
{
	InstSamplerState *s = iv->state;
	switch (index)
	{
		case 0:
			printf("\033[%d;%dHquality: [ ][ ][  ]", y, x);
			addControlInt(x+10, y, &s->interpolate, 0,   0,    1,    0, 0, 0, (void(*)(void*))instControlCallback, NULL);
			addControlInt(x+13, y, &s->bitdepth,    1, 0x0,  0xf,  0xf, 0, 0, (void(*)(void*))instControlCallback, NULL);
			addControlInt(x+16, y, &s->samplerate,  2, 0x0, 0xff, 0xff, 0, 0, (void(*)(void*))instControlCallback, NULL);
			break;
		case 1:
			printf("\033[%d;%dHgain env:    [    ]", y, x);
			addControlInt(x+14, y, &s->envelope, 4, 0x0, 0xffff, 0x00f0, 0, 0, (void(*)(void*))instControlCallback, NULL);
			break;
		case 2:
			printf("\033[%d;%dHchannel:   [      ]", y, x);
			addControlInt(x+12, y, &s->channelmode, 1, 0, 4, 0, 6, 5, (void(*)(void*))instControlCallback, NULL);
				addScalePointInt("STEREO", SAMPLE_CHANNELS_STEREO);
				addScalePointInt("  LEFT", SAMPLE_CHANNELS_LEFT  );
				addScalePointInt(" RIGHT", SAMPLE_CHANNELS_RIGHT );
				addScalePointInt("   MIX", SAMPLE_CHANNELS_MIX   );
				addScalePointInt("  SWAP", SAMPLE_CHANNELS_SWAP  );
			break;
		case 3:
			printf("\033[%d;%dHgain:         [   ]", y, x);
			// if (sample) addControlInt(x+12, y, &sample->invert, 0,    0,   1, 0, 0, 0, (void(*)(void*))instControlCallback, NULL);
			addControlInt(x+15, y, &s->gain,       3, -128, 127, 0, 0, 0, (void(*)(void*))instControlCallback, NULL);
			break;
		case 4:
			printf("\033[%d;%dHunison:   [ ][ ][ ]", y, x);
			break;
		case 5:
			printf("\033[%d;%dHrev/ramp:   [  ][ ]", y, x);
			addControlInt(x+13, y, &s->granular.reversegrains, 2, 0x0, 0xff, 0, 0, 0, (void(*)(void*))instControlCallback, NULL);
			addControlInt(x+17, y, &s->granular.rampgrains,    1, 0x0, 0xf,  8, 0, 0, (void(*)(void*))instControlCallback, NULL);
			break;
		case 6:
			printf("\033[%d;%dHcycle:   [    ][  ]", y, x);
			addControlInt(x+10, y, &s->granular.cyclelength,       4, 0x0, 0xffff, 0x3fff, 0, 0, (void(*)(void*))instControlCallback, NULL);
			addControlInt(x+16, y, &s->granular.cyclelengthjitter, 2, 0x0, 0xff,   0x0,    0, 0, (void(*)(void*))instControlCallback, NULL);
			break;
		case 7:
			printf("\033[%d;%dHtime     [ ][     ]", y, x);
			addControlInt(x+10, y, &s->granular.notestretch, 0, 0,      1,      0, 0, 0, (void(*)(void*))instControlCallback, NULL);
			addControlInt(x+13, y, &s->granular.timestretch, 5, 0x8bff, 0x7bff, 0, 0, 0, (void(*)(void*))instControlCallback, NULL);
			break;
		case 8:
			printf("\033[%d;%dHpan jitter:    [  ]", y, x);
			addControlInt(x+16, y, &s->granular.panjitter, 2, 0x00, 0xff, 0, 0, 0, (void(*)(void*))instControlCallback, NULL);
			break;
		case 9:
			printf("\033[%d;%dHptr jitter:    [  ]", y, x);
			addControlInt(x+16, y, &s->granular.ptrjitter, 2, 0x00, 0xff, 0, 0, 0, (void(*)(void*))instControlCallback, NULL);
			break;
		case 10:
			printf("\033[%d;%dHframe:         [  ]", y, x); addControlInt(x+10, y, &s->frame, 2, 0, 255, 0, 0, 0, (void(*)(void*))instControlCallback, NULL);
			break;
		case 11:
			printf("\033[%d;%dHpitch: [   ][     ]", y, x);
			addControlInt(x+8 , y, &s->granular.pitchstereo, 3, -128,   127,    0, 0, 0, (void(*)(void*))instControlCallback, NULL);
			addControlInt(x+13, y, &s->granular.pitchshift,  5, 0x8bff, 0x7bff, 0, 0, 0, (void(*)(void*))instControlCallback, NULL);
			break;
		case 12:
			printf("\033[%d;%dHjitter:  [  ][ ][ ]", y, x);
			addControlInt(x+10, y, &s->granular.pitchjitter,       2, 0x00, 0xff, 0, 0, 0, (void(*)(void*))instControlCallback, NULL);
			addControlInt(x+14, y, &s->granular.pitchoctaverange,  1, 0x0,  0xf,  0, 0, 0, (void(*)(void*))instControlCallback, NULL);
			addControlInt(x+17, y, &s->granular.pitchoctavechance, 1, 0x0,  0xf,  0, 0, 0, (void(*)(void*))instControlCallback, NULL);
			break;
	}
}

void instUIMidiCallback(short x, short y, Inst *iv, uint8_t index)
{
	switch (index)
	{
		case 0:
			printf("\033[%d;%dHMIDI channel:  [ ]", y, x);
			addControlInt(x+16, y, &((InstMidiState*)iv->state)->channel, 1, -1, 15, -1, 0, 0, (void(*)(void*))instControlCallback, NULL);
			break;
		case 1:
			printf("\033[%d;%dHMIDI program: [  ]", y, x);
			break;
	}
}

void instUIEmptyCallback(short x, short y, Inst *iv, uint8_t index)
{
	printf("\033[%d;%d%s", y, x, EMPTY_INST_UI_TEXT);
}

static short drawInstIndex(short bx, short minx, short maxx)
{
	Inst *iv;
	const InstAPI *api;
	char buffer[9];
	short x = 0;
	for (int i = 0; i < INSTRUMENT_MAX; i++)
		if (w->centre - w->instrument + i > TRACK_ROW && w->centre - w->instrument + i < ws.ws_row)
		{
			x = bx;

			if (instSafe(s->inst, i))
				if (s->inst->v[s->inst->i[i]].triggerflash)
					printf("\033[3%dm", i%6+1);

			if (w->instrument + w->fyoffset == i) /* TODO: fyoffset can get stuck set sometimes */
				printf("\033[1;7m");

			if (x <= ws.ws_col)
			{
				snprintf(buffer, 4, "%02x ", i);
				printCulling(buffer, x, w->centre - w->instrument + i, minx, maxx);
			} x += 3;

			if (x <= ws.ws_col)
			{
				if (instSafe(s->inst, i))
				{
					iv = &s->inst->v[s->inst->i[i]];
					if ((api = instGetAPI(iv->type)))
						api->getindexinfo(iv, buffer);
				} else snprintf(buffer, 9, "........");
				printCulling("        ", x, w->centre - w->instrument + i, minx, maxx); /* flush with attribute, TODO: is there an escape code to do this in a better way? */
				printCulling(buffer, x + 8 - strlen(buffer), w->centre - w->instrument + i, minx, maxx);
			} x += 9;

			printf("\033[40;37;22;27m");
		}
	return x - bx;
}

short getInstUIRows(const InstUI *iui, short cols)
{
	size_t entryc = iui->count;
	short ret = entryc / cols;

	/* round up instead of down */
	if (entryc%cols)
		ret++;

	return ret;
}
short getInstUICols(const InstUI *iui, short rows)
{
	size_t entryc = iui->count;
	short ret = entryc / rows;

	/* round up instead of down */
	if (entryc%rows)
		ret++;

	return ret;
}
short getMaxInstUICols(const InstUI *iui, short width)
{
	return (width + (iui->padding<<1)) / (iui->width + iui->padding);
}

void drawInstUI(const InstUI *iui, void *callbackarg, short x, short w, short y, short scrolloffset, short rows)
{
	short cols = MIN(getMaxInstUICols(iui, w), getInstUICols(iui, rows));
	x += (w - (cols*(iui->width + iui->padding)) + iui->padding)>>1;
	short cx, cy;
	for (uint8_t i = 0; i < iui->count; i++)
	{
		cx = x + (i/rows)*(iui->width + iui->padding);
		cy = y + i%rows;
		if (cy < ws.ws_row - 1 && cy > scrolloffset)
			iui->callback(cx, cy - scrolloffset, callbackarg, i);
	}
}

void drawInstrument(void)
{
	switch (w->mode)
	{
		case MODE_INSERT:
			if (cc.mouseadjust || cc.keyadjust) printf("\033[%d;0H\033[1m-- INSERT ADJUST --\033[m\033[4 q", ws.ws_row);
			else                                printf("\033[%d;0H\033[1m-- INSERT --\033[m\033[6 q",        ws.ws_row);
			w->command.error[0] = '\0';
			break;
		default:
			if (cc.mouseadjust || cc.keyadjust) { printf("\033[%d;0H\033[1m-- ADJUST --\033[m\033[4 q", ws.ws_row); w->command.error[0] = '\0'; }
			break;
	}

	short minx = 1;
	short maxx = ws.ws_col;
	short x = drawInstIndex(1, minx, maxx) + 2;

	if (instSafe(s->inst, w->instrument))
	{
		clearControls();

		Inst *iv = &s->inst->v[s->inst->i[w->instrument]];
		const InstAPI *api;
		if ((api = instGetAPI(iv->type)))
			api->draw(iv, x, TRACK_ROW+1, ws.ws_col - x, ws.ws_row - 1 - (TRACK_ROW+1), minx, maxx);

		drawControls();
	} else
		drawBrowser(fbstate);
}
