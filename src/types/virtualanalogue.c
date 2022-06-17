typedef struct
{
	struct
	{
		char    wave;
		uint8_t fm, am;
	} osc1;
	struct {
		char    wave;
		int8_t  oct;
		uint8_t det;
		uint8_t fm, am;
	} osc2;
	struct {
		char    wave;
		int8_t  oct;
	} sub;
	struct {
		filter  state;
		char    type;
		uint8_t cutoff;
		uint8_t resonance;
		uint8_t mod;
		adsr    env;
		char    tracking;
	} filter;
	adsr        amp;
	struct
	{
		uint8_t rate;
		char    wave;
		uint8_t pwm;
	} lfo;
	struct {
		uint8_t osc1;
		uint8_t osc2;
		uint8_t sub;
		uint8_t noise;
	} mix;
	uint8_t multi;
	uint8_t detune;
	uint8_t stereo;
} analogue_state;


void drawAnalogue(instrument *iv, uint8_t index, unsigned short x, unsigned short y, short *cursor, unsigned char fieldpointer)
{
	analogue_state *as = iv->state[iv->type];
	printf("\033[%d;%dH [\033[3mvirtual analogue\033[m] ", y-1, x+28);

	for (char i = 0; i < 12; i++)
		printf("\033[%d;%dH│", y+i, x+25);
	for (char i = 0; i < 6; i++)
		printf("\033[%d;%dH│", y+i, x+53);
	for (char i = 7; i < 12; i++)
		printf("\033[%d;%dH│", y+i, x+45);
	for (char i = 26; i < 69; i++)
		printf("\033[%d;%dH─", y+6, x+i);
	printf("\033[%d;%dH┬\033[%d;%dH┴", y+6, x+45, y+6, x+53);
	printf("\033[%d;%dH\033[1m  OSCILLATOR 1  \033[m",       y+0,  x+7);
	printf("\033[%d;%dHwave    ",               y+1,  x+7); drawWave(as->osc1.wave);
	printf("\033[%d;%dHfm/am   [%02x][%02x]",     y+2,  x+7, as->osc1.fm, as->osc1.am);
	printf("\033[%d;%dH──────────────────┤",    y+3,  x+7);
	printf("\033[%d;%dH\033[1m  OSCILLATOR 2  \033[m",       y+4,  x+7);
	printf("\033[%d;%dHwave    ",               y+5,  x+7); drawWave(as->osc2.wave);
	printf("\033[%d;%dHoct/det [%+1d][%02x]  ├",y+6,  x+7, as->osc2.oct, as->osc2.det);
	printf("\033[%d;%dHfm/am   [%02x][%02x]",     y+7,  x+7, as->osc2.fm, as->osc2.am);
	printf("\033[%d;%dH──────────────────┤",    y+8,  x+7);
	printf("\033[%d;%dH\033[1m SUB OSCILLATOR \033[m",       y+9,  x+7);
	printf("\033[%d;%dHwave    ",               y+10, x+7); drawWave(as->sub.wave);
	printf("\033[%d;%dHoctave      [%+1d]",     y+11, x+7, as->sub.oct);
	printf("\033[%d;%dH\033[1m         FILTER          \033[m",   y+0, x+28);
	printf("\033[%d;%dHtype    ",                    y+1, x+28); drawFilterType(as->filter.type);
	printf(                           "    a[%02x]", as->filter.env.a);
	printf("\033[%d;%dHcutoff    [%02x]    d[%02x]", y+2, x+28, as->filter.cutoff, as->filter.env.d);
	printf("\033[%d;%dHresonance [%02x]    s[%02x]", y+3, x+28, as->filter.resonance, as->filter.env.s);
	printf("\033[%d;%dHlfo mod   [%02x]    r[%02x]", y+4, x+28, as->filter.mod, as->filter.env.r);
	printf("\033[%d;%dHtracking   ",                 y+5, x+28);
	if (as->filter.tracking) printf("[X]");
	else                     printf("[ ]");
	printf("\033[%d;%dH\033[1m  AMPLIFIER  \033[m",   y+0, x+56);
	printf("\033[%d;%dHattack   [%02x]", y+1, x+56, as->amp.a);
	printf("\033[%d;%dHdecay    [%02x]", y+2, x+56, as->amp.d);
	printf("\033[%d;%dHsustain  [%02x]", y+3, x+56, as->amp.s);
	printf("\033[%d;%dHrelease  [%02x]", y+4, x+56, as->amp.r);
	printf("\033[%d;%dH\033[1m      LFO      \033[m",   y+7,  x+28);
	printf("\033[%d;%dHrate       [%02x]", y+8,  x+28, as->lfo.rate);
	printf("\033[%d;%dHwave   ",           y+9,  x+28); drawWave(as->lfo.wave);
	printf("\033[%d;%dHpwm        [%02x]", y+10, x+28, as->lfo.pwm);
	printf("\033[%d;%dH\033[1m        MIXER        \033[m",   y+7,  x+48);
	printf("\033[%d;%dHoscillator 1      [%01x]", y+8,  x+48, as->mix.osc1);
	printf("\033[%d;%dHoscillator 2      [%01x]", y+9,  x+48, as->mix.osc2);
	printf("\033[%d;%dHsub oscillator    [%01x]", y+10, x+48, as->mix.sub);
	printf("\033[%d;%dHnoise             [%01x]", y+11, x+48, as->mix.noise);
	printf("\033[%d;%dHmulti [%x]",  y+13, x+18, as->multi);
	printf("\033[%d;%dHdetune [%02x]", y+13, x+32, as->detune);
	printf("\033[%d;%dHstereo [%x]", y+13, x+48, as->stereo);

	switch (*cursor)
	{
		case 0:  printf("\033[%d;%dH", y+1,  x+21); break;
		case 1:  printf("\033[%d;%dH", y+2,  x+17); break;
		case 2:  printf("\033[%d;%dH", y+2,  x+21); break;
		case 3:  printf("\033[%d;%dH", y+5,  x+21); break;
		case 4:  printf("\033[%d;%dH", y+6,  x+17); break;
		case 5:  printf("\033[%d;%dH", y+6,  x+21); break;
		case 6:  printf("\033[%d;%dH", y+7,  x+17); break;
		case 7:  printf("\033[%d;%dH", y+7,  x+21); break;
		case 8:  printf("\033[%d;%dH", y+10, x+21); break;
		case 9:  printf("\033[%d;%dH", y+11, x+21); break;
		case 10: printf("\033[%d;%dH", y+1,  x+40); break;
		case 11: printf("\033[%d;%dH", y+2,  x+40); break;
		case 12: printf("\033[%d;%dH", y+3,  x+40); break;
		case 13: printf("\033[%d;%dH", y+4,  x+40); break;
		case 14: printf("\033[%d;%dH", y+5,  x+40); break;
		case 15: printf("\033[%d;%dH", y+1,  x+49); break;
		case 16: printf("\033[%d;%dH", y+2,  x+49); break;
		case 17: printf("\033[%d;%dH", y+3,  x+49); break;
		case 18: printf("\033[%d;%dH", y+4,  x+49); break;
		case 19: printf("\033[%d;%dH", y+8,  x+41); break;
		case 20: printf("\033[%d;%dH", y+9,  x+41); break;
		case 21: printf("\033[%d;%dH", y+10, x+41); break;
		case 22: printf("\033[%d;%dH", y+1,  x+67); break;
		case 23: printf("\033[%d;%dH", y+2,  x+67); break;
		case 24: printf("\033[%d;%dH", y+3,  x+67); break;
		case 25: printf("\033[%d;%dH", y+4,  x+67); break;
		case 26: printf("\033[%d;%dH", y+8,  x+67); break;
		case 27: printf("\033[%d;%dH", y+9,  x+67); break;
		case 28: printf("\033[%d;%dH", y+10, x+67); break;
		case 29: printf("\033[%d;%dH", y+11, x+67); break;
		case 30: printf("\033[%d;%dH", y+13, x+25); break;
		case 31: printf("\033[%d;%dH", y+13, x+41); break;
		case 32: printf("\033[%d;%dH", y+13, x+56); break;
	}
}

void analogueAdjustUp(instrument *iv, short index)
{
	analogue_state *as = iv->state[iv->type];
	switch (index)
	{
		case 0:  if (as->osc1.wave < WAVE_COUNT) as->osc1.wave++; break;
		case 1:  if (w->fieldpointer) as->osc1.fm+=16; else as->osc1.fm++;  break;
		case 2:  if (w->fieldpointer) as->osc1.am+=16; else as->osc1.am++;  break;
		case 3:  if (as->osc2.wave < WAVE_COUNT) as->osc2.wave++; break;
		case 4:  if (as->osc2.oct < 9) as->osc2.oct++; break;
		case 5:  if (w->fieldpointer) as->osc2.det+=16; else as->osc2.det++; break;
		case 6:  if (w->fieldpointer) as->osc2.fm+=16; else as->osc2.fm++;  break;
		case 7:  if (w->fieldpointer) as->osc2.am+=16; else as->osc2.am++;  break;
		case 8:  if (as->sub.wave < WAVE_COUNT) as->sub.wave++; break;
		case 9:  if (as->sub.oct < -1) as->sub.oct++; break;
		case 10: if (as->filter.type < FILTERTYPE_COUNT) as->filter.type++; break;
		case 11: if (w->fieldpointer) as->filter.cutoff+=16; else as->filter.cutoff++; break;
		case 12: if (w->fieldpointer) as->filter.resonance+=16; else as->filter.resonance++; break;
		case 13: if (w->fieldpointer) as->filter.mod+=16; else as->filter.mod++; break;
		case 15: if (w->fieldpointer) as->filter.env.a+=16; else as->filter.env.a++; break;
		case 16: if (w->fieldpointer) as->filter.env.d+=16; else as->filter.env.d++; break;
		case 17: if (w->fieldpointer) as->filter.env.s+=16; else as->filter.env.s++; break;
		case 18: if (w->fieldpointer) as->filter.env.r+=16; else as->filter.env.r++; break;
		case 19: if (w->fieldpointer) as->lfo.rate+=16; else as->lfo.rate++; break;
		case 20: if (as->lfo.wave < WAVE_COUNT) as->lfo.wave++; break;
		case 21: if (w->fieldpointer) as->lfo.pwm+=16; else as->lfo.pwm++; break;
		case 22: if (w->fieldpointer) as->amp.a+=16; else as->amp.a++; break;
		case 23: if (w->fieldpointer) as->amp.d+=16; else as->amp.d++; break;
		case 24: if (w->fieldpointer) as->amp.s+=16; else as->amp.s++; break;
		case 25: if (w->fieldpointer) as->amp.r+=16; else as->amp.r++; break;
		case 26: if (as->mix.osc1 < 15) as->mix.osc1++; else as->mix.osc1 = 0; break;
		case 27: if (as->mix.osc2 < 15) as->mix.osc2++; else as->mix.osc2 = 0; break;
		case 28: if (as->mix.sub < 15) as->mix.sub++; else as->mix.sub = 0; break;
		case 29: if (as->mix.noise < 15) as->mix.noise++; else as->mix.noise = 0; break;
		case 30: if (as->multi < 15) as->multi++; else as->multi = 0; break;
		case 31: if (w->fieldpointer) as->detune+=16; else as->detune++; break;
		case 32: if (as->stereo < 15) as->stereo++; else as->stereo = 0; break;
	}
}
void analogueAdjustDown(instrument *iv, short index)
{
	analogue_state *as = iv->state[iv->type];
	switch (index)
	{
		case 0:  if (as->osc1.wave > 0) as->osc1.wave--; break;
		case 1:  if (w->fieldpointer) as->osc1.fm-=16; else as->osc1.fm--; break;
		case 2:  if (w->fieldpointer) as->osc1.am-=16; else as->osc1.am--; break;
		case 3:  if (as->osc2.wave > 0) as->osc2.wave--; break;
		case 4:  if (as->osc2.oct > -9) as->osc2.oct--; break;
		case 5:  if (w->fieldpointer) as->osc2.det-=16; else as->osc2.det--; break;
		case 6:  if (w->fieldpointer) as->osc2.fm-=16; else as->osc2.fm--; break;
		case 7:  if (w->fieldpointer) as->osc2.am-=16; else as->osc2.am--; break;
		case 8:  if (as->sub.wave > 0) as->sub.wave--; break;
		case 9:  if (as->sub.oct > -9) as->sub.oct--; break;
		case 10: if (as->filter.type > 0) as->filter.type--; break;
		case 11: if (w->fieldpointer) as->filter.cutoff-=16; else as->filter.cutoff--; break;
		case 12: if (w->fieldpointer) as->filter.resonance-=16; else as->filter.resonance--; break;
		case 13: if (w->fieldpointer) as->filter.mod-=16; else as->filter.mod--; break;
		case 15: if (w->fieldpointer) as->filter.env.a-=16; else as->filter.env.a--; break;
		case 16: if (w->fieldpointer) as->filter.env.d-=16; else as->filter.env.d--; break;
		case 17: if (w->fieldpointer) as->filter.env.s-=16; else as->filter.env.s--; break;
		case 18: if (w->fieldpointer) as->filter.env.r-=16; else as->filter.env.r--; break;
		case 19: if (w->fieldpointer) as->lfo.rate-=16; else as->lfo.rate--; break;
		case 20: if (as->lfo.wave > 0) as->lfo.wave--; break;
		case 21: if (w->fieldpointer) as->lfo.pwm-=16; else as->lfo.pwm--; break;
		case 22: if (w->fieldpointer) as->amp.a-=16; else as->amp.a--; break;
		case 23: if (w->fieldpointer) as->amp.d-=16; else as->amp.d--; break;
		case 24: if (w->fieldpointer) as->amp.s-=16; else as->amp.s--; break;
		case 25: if (w->fieldpointer) as->amp.r-=16; else as->amp.r--; break;
		case 26: if (as->mix.osc1 > 0) as->mix.osc1--; else as->mix.osc1 = 15; break;
		case 27: if (as->mix.osc2 > 0) as->mix.osc2--; else as->mix.osc2 = 15; break;
		case 28: if (as->mix.sub > 0) as->mix.sub--; else as->mix.sub = 15; break;
		case 29: if (as->mix.noise > 0) as->mix.noise--; else as->mix.noise = 15; break;
		case 30: if (as->multi > 0) as->multi--; else as->multi = 15; break;
		case 31: if (w->fieldpointer) as->detune-=16; else as->detune--; break;
		case 32: if (as->stereo > 0) as->stereo--; else as->stereo = 15; break;
	}
}

void inputAnalogueHex(short index, analogue_state *as, char value)
{
	switch (index)
	{
		case 1:  updateFieldPush(&as->osc1.fm, value); break;
		case 2:  updateFieldPush(&as->osc1.am, value); break;
		case 5:  updateFieldPush(&as->osc2.det, value); break;
		case 6:  updateFieldPush(&as->osc2.fm, value); break;
		case 7:  updateFieldPush(&as->osc2.am, value); break;
		case 11: updateFieldPush(&as->filter.cutoff, value); break;
		case 12: updateFieldPush(&as->filter.resonance, value); break;
		case 14: updateFieldPush(&as->filter.mod, value); break;
		case 15: updateFieldPush(&as->filter.env.a, value); break;
		case 16: updateFieldPush(&as->filter.env.d, value); break;
		case 17: updateFieldPush(&as->filter.env.s, value); break;
		case 18: updateFieldPush(&as->filter.env.r, value); break;
		case 19: updateFieldPush(&as->lfo.rate, value); break;
		case 21: updateFieldPush(&as->lfo.pwm, value); break;
		case 22: updateFieldPush(&as->amp.a, value); break;
		case 23: updateFieldPush(&as->amp.d, value); break;
		case 24: updateFieldPush(&as->amp.s, value); break;
		case 25: updateFieldPush(&as->amp.r, value); break;
		case 26: as->mix.osc1 = value; break;
		case 27: as->mix.osc2 = value; break;
		case 28: as->mix.sub = value; break;
		case 29: as->mix.noise = value; break;
		case 30: as->multi = value; break;
		case 31: updateFieldPush(&as->detune, value); break;
		case 32: as->stereo = value; break;
	}
}
void analogueInput(int *input)
{
	instrument *iv = s->instrumentv[s->instrumenti[w->instrument]];
	analogue_state *as = iv->state[iv->type];
	switch (*input)
	{
		case 10: case 13: /* return */
			if (w->instrumentindex == 14)
				as->filter.tracking = !as->filter.tracking;
			*input = 0; /* don't reprocess */
			break;
		case 1:  /* ^a */ w->fieldpointer = 0; analogueAdjustUp(iv, w->instrumentindex);   break;
		case 24: /* ^x */ w->fieldpointer = 0; analogueAdjustDown(iv, w->instrumentindex); break;
		case '0':           inputAnalogueHex(w->instrumentindex, as, 0);   break;
		case '1':           inputAnalogueHex(w->instrumentindex, as, 1);   break;
		case '2':           inputAnalogueHex(w->instrumentindex, as, 2);   break;
		case '3':           inputAnalogueHex(w->instrumentindex, as, 3);   break;
		case '4':           inputAnalogueHex(w->instrumentindex, as, 4);   break;
		case '5':           inputAnalogueHex(w->instrumentindex, as, 5);   break;
		case '6':           inputAnalogueHex(w->instrumentindex, as, 6);   break;
		case '7':           inputAnalogueHex(w->instrumentindex, as, 7);   break;
		case '8':           inputAnalogueHex(w->instrumentindex, as, 8);   break;
		case '9':           inputAnalogueHex(w->instrumentindex, as, 9);   break;
		case 'A': case 'a': inputAnalogueHex(w->instrumentindex, as, 10);  break;
		case 'B': case 'b': inputAnalogueHex(w->instrumentindex, as, 11);  break;
		case 'C': case 'c': inputAnalogueHex(w->instrumentindex, as, 12);  break;
		case 'D': case 'd': inputAnalogueHex(w->instrumentindex, as, 13);  break;
		case 'E': case 'e': inputAnalogueHex(w->instrumentindex, as, 14);  break;
		case 'F': case 'f': inputAnalogueHex(w->instrumentindex, as, 15);  break;
	}
	redraw();
}

void analogueMouseToIndex(int y, int x, int button, short *index, signed char *fp)
{
	instrument *iv = s->instrumentv[s->instrumenti[w->instrument]];
	analogue_state *as = iv->state[iv->type];
	if (y > 12)
	{
		if (x > 44)      { *index = 32; if (x < 57) *fp = 1; else *fp = 0; }
		else if (x > 28) { *index = 31; if (x < 41) *fp = 1; else *fp = 0; }
		else             { *index = 30; if (x < 25) *fp = 1; else *fp = 0; }
	} else
	{
		if (x < 25)
			switch (y)
			{
				case 1:  case 2:  *index = 0; break;
				case 3:  case 4:
					if (x < 19) { *index = 1; if (x < 17) *fp = 1; else *fp = 0; }
					else        { *index = 2; if (x < 21) *fp = 1; else *fp = 0; }
					break;
				case 5:  case 6:  *index = 3; break;
				case 7:
					if (x < 19) { *index = 4; if (x < 17) *fp = 1; else *fp = 0; }
					else        { *index = 5; if (x < 21) *fp = 1; else *fp = 0; }
					break;
				case 8:  case 9:
					if (x < 19) { *index = 6; if (x < 17) *fp = 1; else *fp = 0; }
					else        { *index = 7; if (x < 21) *fp = 1; else *fp = 0; }
					break;
				case 10: case 11: *index = 8; break;
				case 12:          *index = 9; break;
			}
		else if (y < 8)
		{
			if (x < 43)
				switch (y)
				{
					case 1: case 2: *index = 10; break;
					case 3:         *index = 11; if (x < 40) *fp = 1; else *fp = 0; break;
					case 4:         *index = 12; if (x < 40) *fp = 1; else *fp = 0; break;
					case 5:         *index = 13; if (x < 40) *fp = 1; else *fp = 0; break;
					case 6: case 7: *index = 14;
						as->filter.tracking = !as->filter.tracking;
						break;
				}
			else if (x < 53)
				switch (y)
				{
					case 1: case 2:         *index = 15; if (x < 49) *fp = 1; else *fp = 0; break;
					case 3:                 *index = 16; if (x < 49) *fp = 1; else *fp = 0; break;
					case 4:                 *index = 17; if (x < 49) *fp = 1; else *fp = 0; break;
					case 5: case 6: case 7: *index = 18; if (x < 49) *fp = 1; else *fp = 0; break;
				}
			else
				switch (y)
				{
					case 1: case 2:         *index = 22; if (x < 67) *fp = 1; else *fp = 0; break;
					case 3:                 *index = 23; if (x < 67) *fp = 1; else *fp = 0; break;
					case 4:                 *index = 24; if (x < 67) *fp = 1; else *fp = 0; break;
					case 5: case 6: case 7: *index = 25; if (x < 67) *fp = 1; else *fp = 0; break;
				}
		} else
		{
			if (x < 45)
				switch (y)
				{
					case 8:  case 9:  *index = 19; if (x < 41) *fp = 1; else *fp = 0; break;
					case 10:          *index = 20; break;
					case 11: case 12: *index = 21; if (x < 41) *fp = 1; else *fp = 0; break;
				}
			else
				switch (y)
				{
					case 8:  case 9:  *index = 26; if (x < 67) *fp = 1; else *fp = 0; break;
					case 10:          *index = 27; if (x < 67) *fp = 1; else *fp = 0; break;
					case 11:          *index = 28; if (x < 67) *fp = 1; else *fp = 0; break;
					case 12: case 13: *index = 29; if (x < 67) *fp = 1; else *fp = 0; break;
				}
		}
	}
}

float analogueInstance(analogue_state *as, channel *cv, uint8_t index, float detune)
{
	float pps = 1.0 / ((float)samplerate
		/ (C5_FREQ * powf(M_12_ROOT_2, (short)cv->r.note - 61 + cv->cents + detune)));
	
	cv->analogue[index].lfophase += 1.0 / ((float)samplerate * LFO_MAX
		+ (float)samplerate * (LFO_MIN - LFO_MAX)
		* (1.0 - as->lfo.rate / 256.0) + detune);

	float output = 0.0;
	float lfo = (oscillator(as->lfo.wave, cv->analogue[index].lfophase, 0.5) + 1.0) / 2;
	float pw = 0.5 * (1.0 + lfo * as->lfo.pwm / 256.0);

	{ /* osc1 */
		cv->analogue[index].osc1phase += pps;
		if (cv->analogue[index].osc1phase > 1.0) cv->analogue[index].osc1phase -= 1.0;

		output += oscillator(as->osc1.wave, cv->analogue[index].osc1phase
				+ (lfo * FM_DEPTH * as->osc1.fm / 256.0), pw)
			* as->mix.osc1 / 16.0 * (1.0 + lfo * as->osc1.am / 256.0);
	}
	{ /* osc2 */
		cv->analogue[index].osc2phase += pps * powf(2, as->osc2.oct)
			* powf(2, as->osc2.det / 256.0 - 0.5);
		if (cv->analogue[index].osc2phase > 1.0) cv->analogue[index].osc2phase -= 1.0;

		output += oscillator(as->osc2.wave, cv->analogue[index].osc2phase
				+ (lfo * FM_DEPTH * as->osc2.fm / 256.0), pw)
			* as->mix.osc2 / 16.0 * (1.0 + lfo * as->osc2.am / 256.0);
	}
	{ /* sub */
		cv->analogue[index].subphase += pps * powf(2, as->sub.oct);
		if (cv->analogue[index].subphase > 1.0) cv->analogue[index].subphase -= 1.0;

		output += oscillator(as->sub.wave, cv->analogue[index].subphase, pw)
			* as->mix.sub / 16.0;
	}
	return output;
}

void analogueProcess(instrument *iv, channel *cv, uint32_t pointer, float *l, float *r)
{
	*l = 0.0;
	*r = 0.0;

	analogue_state *as = iv->state[iv->type];

	// float again = 1.0 / (as->multi * 2 + 1);
	float again = 1.0;
	adsrEnvelope(as->amp, &again, pointer, cv->releasepointer);
	adsrEnvelope(as->amp, &again, pointer, cv->releasepointer);

	/* centre */
	*l = *r = analogueInstance(as, cv, 2, 0) * again;

	float c, gain, detune, width;
	for (char i = 1; i <= as->multi; i++)
	{
		gain = i / as->multi;
		width = (1.0 - gain) * as->stereo / 16.0;
		detune = gain * as->detune / 256.0;
		if (i % 2)
		{
			c = analogueInstance(as, cv, i * 2 + 0,  detune) * again;
			*l += c * (gain + width);
			*r += c * MAX(gain - width, 0.0);
			c = analogueInstance(as, cv, i * 2 + 1, -detune) * again;
			*r += c * (gain + width);
			*l += c * MAX(gain - width, 0.0);
		} else
		{
			c = analogueInstance(as, cv, i * 2 + 0, -detune) * again;
			*l += c * (gain + width);
			*r += c * MAX(gain - width, 0.0);
			c = analogueInstance(as, cv, i * 2 + 1,  detune) * again;
			*r += c * (gain + width);
			*l += c * MAX(gain - width, 0.0);
		}
	}

	{ /* noise */
		g_x1 ^= g_x2;
		float noise = g_x2 * (as->mix.noise / 32.0) * g_fScale * again; /* 16 * 2 */
		g_x2 += g_x1;
		*l += noise;
		*r += noise;
	}
	{ /* filter */
		float fgain = 1.0;
		adsrEnvelope(as->filter.env, &fgain, pointer, cv->releasepointer);
		adsrEnvelope(as->filter.env, &fgain, pointer, cv->releasepointer);
		float lfo = oscillator(as->lfo.wave, cv->analogue[0].lfophase, 0.5);
		switch (as->filter.type)
		{
			case 0:
				if (as->filter.tracking)
					calcLp(&as->filter.state,
							fgain * MAX(1, as->filter.cutoff) / 256.0 * (1.0 + lfo * as->filter.mod / 256.0)
							* powf(M_12_ROOT_2, (short)cv->r.note - 61 + cv->cents),
							as->filter.resonance / 256.0);
				else
					calcLp(&as->filter.state,
							fgain * MAX(1, as->filter.cutoff) / 256.0 * (1.0 + lfo * as->filter.mod / 256.0),
							as->filter.resonance / 256.0);
				break;
			case 1:
				if (as->filter.tracking)
					calcHp(&as->filter.state,
							fgain * MAX(1, as->filter.cutoff) / 256.0 * (1.0 + lfo * as->filter.mod / 256.0)
							* powf(M_12_ROOT_2, (short)cv->r.note - 61 + cv->cents),
							as->filter.resonance / 256.0);
				else
					calcHp(&as->filter.state,
							fgain * MAX(1, as->filter.cutoff) / 256.0 * (1.0 + lfo * as->filter.mod / 256.0),
							as->filter.resonance / 256.0);
				break;
		}
		*l = runFilter(&as->filter.state, *l, cv->ln_1, cv->ln_2);
		*r = runFilter(&as->filter.state, *r, cv->rn_1, cv->rn_2);
		/* push filter history */
		cv->ln_2 = cv->ln_1; cv->ln_1 = *l;
		cv->rn_2 = cv->rn_1; cv->rn_1 = *r;
	}
	{ /* saturation */
		if (*l > 0.0)
		{ if (*l > 1.0)  *l =  1.0 + tanhf(tanhf(*l - 1.0));
		} else
		{ if (*l < -1.0) *l = -1.0 + tanhf(tanhf(*l + 1.0));
		}
		if (*r > 0.0)
		{ if (*r > 1.0)  *r =  1.0 + tanhf(tanhf(*r - 1.0));
		} else
		{ if (*r < -1.0) *r = -1.0 + tanhf(tanhf(*r + 1.0));
		}
	}
}

void analogueInitType(void **state)
{
	*state = calloc(1, sizeof(analogue_state));
	analogue_state *as = *state;
	as->osc2.det = 128;
	as->sub.oct = -1;
	as->filter.cutoff = 255;
	as->filter.env.s = 255;
	as->amp.s = 255;
	as->lfo.rate = 127;
	as->mix.osc1 = 8;
	as->mix.osc2 = 8;
	as->mix.sub = 0;
	as->mix.noise = 0;
}

void analogueWrite(instrument *iv, uint8_t index, FILE *fp)
{ fwrite(&iv->state[index], sizeof(analogue_state), 1, fp); }
void analogueRead(instrument *iv, uint8_t index, FILE *fp)
{ fread(&iv->state[index], sizeof(analogue_state), 1, fp); }

void analogueInit(int index)
{
	t->f[index].indexc = 32;
	t->f[index].statesize = sizeof(analogue_state);
	t->f[index].draw = &drawAnalogue;
	t->f[index].adjustUp = &analogueAdjustUp;
	t->f[index].adjustDown = &analogueAdjustDown;
	t->f[index].mouseToIndex = &analogueMouseToIndex;
	t->f[index].input = &analogueInput;
	t->f[index].process = &analogueProcess;
	t->f[index].initType = &analogueInitType;
	t->f[index].write = &analogueWrite;
	t->f[index].read = &analogueRead;
}
