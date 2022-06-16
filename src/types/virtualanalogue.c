#define WAVE_COUNT 4
#define FILTERTYPE_COUNT 2

typedef struct
{
	struct
	{
		char    wave;
		uint8_t mod;
	} osc1; struct {
		char    wave;
		int8_t  oct;
		uint8_t det;
		uint8_t mod;
	} osc2; struct {
		char    wave;
		int8_t  octave;
	} sub; struct {
		char    type;
		uint8_t cutoff;
		uint8_t resonance;
		uint8_t mod;
		adsr    env;
	} filter;
	adsr        amp;
	struct {
		uint8_t rate;
		char    wave;
		uint8_t pwm;
	} lfo; struct {
		uint8_t osc1;
		uint8_t osc2;
		uint8_t sub;
		uint8_t noise;
	} mix; struct {
		uint8_t count;
		uint8_t detune;
	} mult;
} analogue_state;


void drawWave(char wave)
{
	switch (wave)
	{
		case 0:  printf("[   tri]"); break;
		case 1:  printf("[   saw]"); break;
		case 2:  printf("[  ramp]"); break;
		case 3:  printf("[square]"); break;
		case 4:  printf("[  sine]"); break;
		default: printf("[ ?????]"); break;
	}
}
void drawFilterType(char type)
{
	switch (type)
	{
		case 0:  printf("[ low]"); break;
		case 1:  printf("[high]"); break;
		case 2:  printf("[band]"); break;
		default: printf("[????]"); break;
	}
}

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
	printf("\033[%d;%dHlfo mod     [%02x]",     y+2,  x+7, as->osc1.mod);
	printf("\033[%d;%dH──────────────────┤",    y+3,  x+7);
	printf("\033[%d;%dH\033[1m  OSCILLATOR 2  \033[m",       y+4,  x+7);
	printf("\033[%d;%dHwave    ",               y+5,  x+7); drawWave(as->osc2.wave);
	printf("\033[%d;%dHoct/det [%+1d][%02x]  ├",y+6,  x+7, as->osc2.oct, as->osc2.det);
	printf("\033[%d;%dHlfo mod     [%02x]",     y+7,  x+7, as->osc2.mod);
	printf("\033[%d;%dH──────────────────┤",    y+8,  x+7);
	printf("\033[%d;%dH\033[1m SUB OSCILLATOR \033[m",       y+9,  x+7);
	printf("\033[%d;%dHwave    ",               y+10, x+7); drawWave(as->sub.wave);
	printf("\033[%d;%dHoctave      [%+1d]",     y+11, x+7, as->sub.octave);
	printf("\033[%d;%dH\033[1m         FILTER          \033[m",   y+0, x+28);
	printf("\033[%d;%dHtype    ",                    y+1, x+28); drawFilterType(as->filter.type);
	printf(                           "    a[%02x]", as->filter.env.a);
	printf("\033[%d;%dHcutoff    [%02x]    d[%02x]", y+2, x+28, as->filter.cutoff, as->filter.env.d);
	printf("\033[%d;%dHresonance [%02x]    s[%02x]", y+3, x+28, as->filter.resonance, as->filter.env.s);
	printf("\033[%d;%dHlfo mod   [%02x]    r[%02x]", y+4, x+28, as->filter.mod, as->filter.env.r);
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
	printf("\033[%d;%dHoscillator 1     [%02x]", y+8,  x+48, as->mix.osc1);
	printf("\033[%d;%dHoscillator 2     [%02x]", y+9,  x+48, as->mix.osc2);
	printf("\033[%d;%dHsub oscillator   [%02x]", y+10, x+48, as->mix.sub);
	printf("\033[%d;%dHnoise            [%02x]", y+11, x+48, as->mix.noise);
	printf("\033[%d;%dHmult [%02x]",         y+13, x+23, as->mult.count);
	printf("\033[%d;%dHdetune range [%02x]", y+13, x+36, as->mult.detune);

	switch (*cursor)
	{
		case 0:  printf("\033[%d;%dH", y+1,  x+21); break;
		case 1:  printf("\033[%d;%dH", y+2,  x+21); break;
		case 2:  printf("\033[%d;%dH", y+5,  x+21); break;
		case 3:  printf("\033[%d;%dH", y+6,  x+17); break;
		case 4:  printf("\033[%d;%dH", y+6,  x+21); break;
		case 5:  printf("\033[%d;%dH", y+7,  x+21); break;
		case 6:  printf("\033[%d;%dH", y+10, x+21); break;
		case 7:  printf("\033[%d;%dH", y+11, x+21); break;
		case 8:  printf("\033[%d;%dH", y+1,  x+40); break;
		case 9:  printf("\033[%d;%dH", y+2,  x+40); break;
		case 10: printf("\033[%d;%dH", y+3,  x+40); break;
		case 11: printf("\033[%d;%dH", y+4,  x+40); break;
		case 12: printf("\033[%d;%dH", y+1,  x+49); break;
		case 13: printf("\033[%d;%dH", y+2,  x+49); break;
		case 14: printf("\033[%d;%dH", y+3,  x+49); break;
		case 15: printf("\033[%d;%dH", y+4,  x+49); break;
		case 16: printf("\033[%d;%dH", y+8,  x+41); break;
		case 17: printf("\033[%d;%dH", y+9,  x+41); break;
		case 18: printf("\033[%d;%dH", y+10, x+41); break;
		case 19: printf("\033[%d;%dH", y+1,  x+67); break;
		case 20: printf("\033[%d;%dH", y+2,  x+67); break;
		case 21: printf("\033[%d;%dH", y+3,  x+67); break;
		case 22: printf("\033[%d;%dH", y+4,  x+67); break;
		case 23: printf("\033[%d;%dH", y+8,  x+67); break;
		case 24: printf("\033[%d;%dH", y+9,  x+67); break;
		case 25: printf("\033[%d;%dH", y+10, x+67); break;
		case 26: printf("\033[%d;%dH", y+11, x+67); break;
		case 27: printf("\033[%d;%dH", y+13, x+30); break;
		case 28: printf("\033[%d;%dH", y+13, x+51); break;
	}
}

void analogueAdjustUp(instrument *iv, short index)
{
	analogue_state *as = iv->state[iv->type];
	switch (index)
	{
		case 0:  if (as->osc1.wave < WAVE_COUNT) as->osc1.wave++; break;
		case 1:  as->osc1.mod++; break;
		case 2:  if (as->osc2.wave < WAVE_COUNT) as->osc2.wave++; break;
		case 3:  if (as->osc2.oct < 9) as->osc2.oct++; break;
		case 4:  as->osc2.det++; break;
		case 5:  as->osc2.mod++; break;
		case 6:  if (as->sub.wave < WAVE_COUNT) as->sub.wave++; break;
		case 7:  if (as->sub.octave < -1) as->sub.octave++; break;
		case 8:  if (as->filter.type < FILTERTYPE_COUNT) as->filter.type++; break;
		case 9:  as->filter.cutoff++; break;
		case 10: as->filter.resonance++; break;
		case 11: as->filter.mod++; break;
		case 12: as->filter.env.a++; break;
		case 13: as->filter.env.d++; break;
		case 14: as->filter.env.s++; break;
		case 15: as->filter.env.r++; break;
		case 16: as->lfo.rate++; break;
		case 17: if (as->lfo.wave < WAVE_COUNT) as->lfo.wave++; break;
		case 18: as->lfo.pwm++; break;
		case 19: as->amp.a++; break;
		case 20: as->amp.d++; break;
		case 21: as->amp.s++; break;
		case 22: as->amp.r++; break;
		case 23: as->mix.osc1++; break;
		case 24: as->mix.osc2++; break;
		case 25: as->mix.sub++; break;
		case 26: as->mix.noise++; break;
		case 27: as->mult.count++; break;
		case 28: as->mult.detune++; break;
	}
}
void analogueAdjustDown(instrument *iv, short index)
{
	analogue_state *as = iv->state[iv->type];
	switch (index)
	{
		case 0:  if (as->osc1.wave > 0) as->osc1.wave--; break;
		case 1:  as->osc1.mod--; break;
		case 2:  if (as->osc2.wave > 0) as->osc2.wave--; break;
		case 3:  if (as->osc2.oct > -9) as->osc2.oct--; break;
		case 4:  as->osc2.det--; break;
		case 5:  as->osc2.mod--; break;
		case 6:  if (as->sub.wave > 0) as->sub.wave--; break;
		case 7:  if (as->sub.octave > -9) as->sub.octave--; break;
		case 8:  if (as->filter.type > 0) as->filter.type--; break;
		case 9:  as->filter.cutoff--; break;
		case 10: as->filter.resonance--; break;
		case 11: as->filter.mod--; break;
		case 12: as->filter.env.a--; break;
		case 13: as->filter.env.d--; break;
		case 14: as->filter.env.s--; break;
		case 15: as->filter.env.r--; break;
		case 16: as->lfo.rate--; break;
		case 17: if (as->lfo.wave > 0) as->lfo.wave--; break;
		case 18: as->lfo.pwm--; break;
		case 19: as->amp.a--; break;
		case 20: as->amp.d--; break;
		case 21: as->amp.s--; break;
		case 22: as->amp.r--; break;
		case 23: as->mix.osc1--; break;
		case 24: as->mix.osc2--; break;
		case 25: as->mix.sub--; break;
		case 26: as->mix.noise--; break;
		case 27: as->mult.count--; break;
		case 28: as->mult.detune--; break;
	}
}

void inputAnalogueHex(short index, analogue_state *as, char value)
{
	switch (index)
	{
		case 1:  updateFieldPush(&as->osc1.mod, value); break;
		case 4:  updateFieldPush(&as->osc2.det, value); break;
		case 5:  updateFieldPush(&as->osc2.mod, value); break;
		case 9:  updateFieldPush(&as->filter.cutoff, value); break;
		case 10: updateFieldPush(&as->filter.resonance, value); break;
		case 11: updateFieldPush(&as->filter.mod, value); break;
		case 12: updateFieldPush(&as->filter.env.a, value); break;
		case 13: updateFieldPush(&as->filter.env.d, value); break;
		case 14: updateFieldPush(&as->filter.env.s, value); break;
		case 15: updateFieldPush(&as->filter.env.r, value); break;
		case 16: updateFieldPush(&as->lfo.rate, value); break;
		case 18: updateFieldPush(&as->lfo.pwm, value); break;
		case 19: updateFieldPush(&as->amp.a, value); break;
		case 20: updateFieldPush(&as->amp.d, value); break;
		case 21: updateFieldPush(&as->amp.s, value); break;
		case 22: updateFieldPush(&as->amp.r, value); break;
		case 23: updateFieldPush(&as->mix.osc1, value); break;
		case 24: updateFieldPush(&as->mix.osc2, value); break;
		case 25: updateFieldPush(&as->mix.sub, value); break;
		case 26: updateFieldPush(&as->mix.noise, value); break;
		case 27: updateFieldPush(&as->mult.count, value); break;
		case 28: updateFieldPush(&as->mult.detune, value); break;
	}
}
void analogueInput(int *input)
{
	instrument *iv = s->instrumentv[s->instrumenti[w->instrument]];
	analogue_state *as = iv->state[iv->type];
	switch (*input)
	{
		case 1:  /* ^a */ analogueAdjustUp(iv, w->instrumentindex);   break;
		case 24: /* ^x */ analogueAdjustDown(iv, w->instrumentindex); break;
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

void analogueMouseToIndex(int y, int x, int button, short *index, signed char *fieldpointer)
{
	if (y > 12)
	{
		if (x < 34) *index = 27;
		else        *index = 28;
	} else
	{
		if (x < 25)
		{
			switch (y)
			{
				case 1:  case 2:  *index = 0; break;
				case 3:  case 4:  *index = 1; break;
				case 5:  case 6:  *index = 2; break;
				case 7:
					if (x < 19)   *index = 3;
					else          *index = 4;
					break;
				case 8:  case 9:  *index = 5; break;
				case 10: case 11: *index = 6; break;
				case 12:          *index = 7; break;
			}
		} else if (y < 8)
		{
			if (x < 43)
			{
				switch (y)
				{
					case 1: case 2:         *index = 8;  break;
					case 3:                 *index = 9;  break;
					case 4:                 *index = 10; break;
					case 5: case 6: case 7: *index = 11; break;
				}
			} else if (x < 53)
			{
				switch (y)
				{
					case 1: case 2:         *index = 12; break;
					case 3:                 *index = 13; break;
					case 4:                 *index = 14; break;
					case 5: case 6: case 7: *index = 15; break;
				}
			} else
			{
				switch (y)
				{
					case 1: case 2:         *index = 19; break;
					case 3:                 *index = 20; break;
					case 4:                 *index = 21; break;
					case 5: case 6: case 7: *index = 22; break;
				}
			}
		} else
		{
			if (x < 45)
			{
				switch (y)
				{
					case 8:  case 9:  *index = 16; break;
					case 10:          *index = 17; break;
					case 11: case 12: *index = 18; break;
				}
			} else
				switch (y)
				{
					case 8:  case 9:  *index = 23; break;
					case 10:          *index = 24; break;
					case 11:          *index = 25; break;
					case 12: case 13: *index = 26; break;
				}
			{
			}
		}
	}
}

void analogueProcess(instrument *iv, channel *cv, uint32_t pointer, float *l, float *r)
{
	*l = 0.0;
	*r = 0.0;
}

void analogueChangeType(void **state)
{
	*state = calloc(1, sizeof(analogue_state));
	analogue_state *as = *state;
	as->osc2.det = 127;
	as->sub.octave = -1;
	as->filter.cutoff = 255;
	as->filter.env.s = 255;
	as->amp.s = 255;
	as->lfo.rate = 127;
	as->mix.osc1 = 127;
	as->mix.osc2 = 127;
	as->mix.noise = 127;
	as->mix.sub = 127;
}

void analogueWrite(instrument *iv, uint8_t index, FILE *fp)
{
	fwrite(&iv->state[index], sizeof(analogue_state), 1, fp);
}
void analogueRead(instrument *iv, uint8_t index, FILE *fp)
{
	fread(&iv->state[index], sizeof(analogue_state), 1, fp);
}

void analogueInit(int index)
{
	t->f[index].indexc = 28;
	t->f[index].statesize = sizeof(analogue_state);
	t->f[index].draw = &drawAnalogue;
	t->f[index].adjustUp = &analogueAdjustUp;
	t->f[index].adjustDown = &analogueAdjustDown;
	t->f[index].mouseToIndex = &analogueMouseToIndex;
	t->f[index].input = &analogueInput;
	t->f[index].process = &analogueProcess;
	t->f[index].changeType = &analogueChangeType;
	t->f[index].write = &analogueWrite;
	t->f[index].read = &analogueRead;
}
