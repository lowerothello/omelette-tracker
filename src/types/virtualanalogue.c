typedef struct
{
	struct
	{
		uint8_t waveform;
		uint8_t lfo;
		uint8_t mix;
		uint8_t fm;
		uint8_t flags; /* %1:ringmod  %2:sync */
	} osc1;
	struct {
		uint8_t waveform;
		int8_t  oct;
		uint8_t semi;
		uint8_t detune;
		uint8_t lfo;
		uint8_t mix;
		uint8_t flags; /* %1:track */
	} osc2;
	struct {
		uint8_t waveform;
		int8_t  oct;
		uint8_t mix;
	} sub;
	struct {
		uint8_t rate;
		uint8_t waveform;
		uint8_t pwm;
	} lfo;
	struct {
		uint8_t cutoff;
		uint8_t resonance;
		uint8_t lfo;
		adsr    env;
		uint8_t track;
		uint8_t flags; /* %1:high resonance */
	} filter;
	adsr        amp;
	uint8_t noise;
	uint8_t unison;
	uint8_t detune;
	uint8_t stereo;
} analogue_state;

typedef struct
{
	filter fl, fr;
	wnoise wn;
	struct
	{
		float osc1phase; /* stored phase, for freewheel oscillators */
		float osc2phase;
		float subphase;
		float lfophase;
	} unison[33];         /* one for any potential instance */
} analogue_channel;



void drawAnalogue(instrument *iv, uint8_t index, unsigned short x, unsigned short y, short *cursor, char adjust)
{
	analogue_state *as = iv->state[iv->type];
	printf("\033[%d;%dH [virtual analogue] ", y-1, x+21);

	for (char i = 1; i < 7; i++)
		printf("\033[%d;%dH│", y+i, x+48);
	for (char i = 8; i < 13; i++)
		printf("\033[%d;%dH│", y+i, x+40);
	for (char i = 21; i < 62; i++)
		printf("\033[%d;%dH─", y+7, x+i);
	printf("\033[%d;%dH┬\033[%d;%dH┴", y+7, x+40, y+7, x+48);
	printf("\033[%d;%dH\033[1mOSCILLATOR1\033[m   [%02x]",   y+1,  x+0, as->osc1.mix);
	printf("\033[%d;%dHwaveform",                y+2,  x+0);
	printf("\033[%d;%dHfm/ringmod [%02x]",       y+3,  x+0, as->osc1.fm);
	drawBit(as->osc1.flags & 0b1);
	printf("\033[%d;%dHsync/lfo   ", y+4,  x+0);
	drawBit(as->osc1.flags & 0b10);
	printf("\033[%d;%dH[%02x]",      y+4,  x+14, as->osc1.lfo);
	printf("\033[%d;%dH──────────────────",      y+5,  x+0);
	printf("\033[%d;%dH\033[1mOSCILLATOR2\033[m   [%02x]",   y+6,  x+0, as->osc2.mix);
	printf("\033[%d;%dHwaveform",                y+7,  x+0);
	printf("\033[%d;%dHdetune [%+1d][%x][%02x]", y+8,  x+0, as->osc2.oct, as->osc2.semi, as->osc2.detune);
	printf("\033[%d;%dHtrack/lfo  ", y+9,  x+0);
	drawBit(as->osc2.flags & 0b1);
	printf("\033[%d;%dH[%02x]",      y+9,  x+14, as->osc2.lfo);
	printf("\033[%d;%dH──────────────────",      y+10, x+0);
	printf("\033[%d;%dH\033[1mSUB OSC\033[m   [%+1d][%02x]", y+11, x+0, as->sub.oct, as->sub.mix);
	printf("\033[%d;%dHwaveform",                y+12, x+0);
	printf("\033[%d;%dH\033[1mFILTER\033[m",     y+1,  x+31);
	printf("\033[%d;%dHcutoff  [%02x]  attack  [%02x]", y+2, x+21, as->filter.cutoff, as->filter.env.a);
	printf("\033[%d;%dHreso    [%02x]  decay   [%02x]", y+3, x+21, as->filter.resonance, as->filter.env.d);
	printf("\033[%d;%dHlfo     [%02x]  sustain [%02x]", y+4, x+21, as->filter.lfo, as->filter.env.s);
	printf("\033[%d;%dHtrack   [%02x]  release [%02x]", y+5, x+21, as->filter.track, as->filter.env.r);
	printf("\033[%d;%dHhigh resonance (loud)  ",        y+6, x+21);
	drawBit(as->filter.flags & 0b1);
	printf("\033[%d;%dH\033[1mAMPLIFIER\033[m",   y+1, x+51);
	printf("\033[%d;%dHattack  [%02x]", y+2, x+50, as->amp.a);
	printf("\033[%d;%dHdecay   [%02x]", y+3, x+50, as->amp.d);
	printf("\033[%d;%dHsustain [%02x]", y+4, x+50, as->amp.s);
	printf("\033[%d;%dHrelease [%02x]", y+5, x+50, as->amp.r);
	printf("\033[%d;%dH\033[1mLFO\033[m",       y+8,  x+29);
	printf("\033[%d;%dHwaveform",               y+9,  x+21);
	printf("\033[%d;%dHrate          [%02x]",   y+10, x+21, as->lfo.rate);
	printf("\033[%d;%dHpulse width   [%02x]",   y+11, x+21, as->lfo.pwm);
	printf("\033[%d;%dH\033[1mMIXER\033[m",     y+8,  x+49);
	printf("\033[%d;%dHnoise           [%02x]", y+9,  x+42, as->noise);
	printf("\033[%d;%dHosc unison       [%x]",  y+10, x+42, as->unison);
	printf("\033[%d;%dHunison detune   [%02x]", y+11, x+42, as->detune);
	printf("\033[%d;%dHunison stereo    [%x]",  y+12, x+42, as->stereo);

	drawWave(as->lfo.waveform,  y+9,  x+31, *cursor == 25 && adjust);
	drawWave(as->sub.waveform,  y+12, x+10, *cursor == 15 && adjust);
	drawWave(as->osc2.waveform, y+7,  x+10, *cursor == 7 && adjust);
	drawWave(as->osc1.waveform, y+2,  x+10, *cursor == 1 && adjust);

	switch (*cursor)
	{
		case 0:  printf("\033[%d;%dH", y+1,  x+16 - w->fieldpointer); break;
		case 1:  printf("\033[%d;%dH", y+2,  x+16); break;
		case 2:  printf("\033[%d;%dH", y+3,  x+13 - w->fieldpointer); break;
		case 3:  printf("\033[%d;%dH", y+3,  x+16); break;
		case 4:  printf("\033[%d;%dH", y+4,  x+12); break;
		case 5:  printf("\033[%d;%dH", y+4,  x+16 - w->fieldpointer); break;
		case 6:  printf("\033[%d;%dH", y+6,  x+16 - w->fieldpointer); break;
		case 7:  printf("\033[%d;%dH", y+7,  x+16); break;
		case 8:  printf("\033[%d;%dH", y+8,  x+9); break;
		case 9:  printf("\033[%d;%dH", y+8,  x+12); break;
		case 10: printf("\033[%d;%dH", y+8,  x+16 - w->fieldpointer); break;
		case 11: printf("\033[%d;%dH", y+9,  x+12); break;
		case 12: printf("\033[%d;%dH", y+9,  x+16 - w->fieldpointer); break;
		case 13: printf("\033[%d;%dH", y+11, x+12); break;
		case 14: printf("\033[%d;%dH", y+11, x+16 - w->fieldpointer); break;
		case 15: printf("\033[%d;%dH", y+12, x+16); break;
		case 16: printf("\033[%d;%dH", y+2,  x+31 - w->fieldpointer); break;
		case 17: printf("\033[%d;%dH", y+3,  x+31 - w->fieldpointer); break;
		case 18: printf("\033[%d;%dH", y+4,  x+31 - w->fieldpointer); break;
		case 19: printf("\033[%d;%dH", y+5,  x+31 - w->fieldpointer); break;
		case 20: printf("\033[%d;%dH", y+2,  x+45 - w->fieldpointer); break;
		case 21: printf("\033[%d;%dH", y+3,  x+45 - w->fieldpointer); break;
		case 22: printf("\033[%d;%dH", y+4,  x+45 - w->fieldpointer); break;
		case 23: printf("\033[%d;%dH", y+5,  x+45 - w->fieldpointer); break;
		case 24: printf("\033[%d;%dH", y+6,  x+45); break;
		case 25: printf("\033[%d;%dH", y+9,  x+37); break;
		case 26: printf("\033[%d;%dH", y+10, x+37 - w->fieldpointer); break;
		case 27: printf("\033[%d;%dH", y+11, x+37 - w->fieldpointer); break;
		case 28: printf("\033[%d;%dH", y+2,  x+60 - w->fieldpointer); break;
		case 29: printf("\033[%d;%dH", y+3,  x+60 - w->fieldpointer); break;
		case 30: printf("\033[%d;%dH", y+4,  x+60 - w->fieldpointer); break;
		case 31: printf("\033[%d;%dH", y+5,  x+60 - w->fieldpointer); break;
		case 32: printf("\033[%d;%dH", y+9,  x+60 - w->fieldpointer); break;
		case 33: printf("\033[%d;%dH", y+10, x+60); break;
		case 34: printf("\033[%d;%dH", y+11, x+60 - w->fieldpointer); break;
		case 35: printf("\033[%d;%dH", y+12, x+60); break;
	}
}

void analogueAdjustUp(instrument *iv, short index, char mouse)
{
	analogue_state *as = iv->state[iv->type];
	switch (index)
	{
		case 0:  if (w->fieldpointer) { if (as->osc1.mix < 255 - 16) as->osc1.mix+=16; else as->osc1.mix = 255; } else if (as->osc1.mix < 255) as->osc1.mix++; break;
		case 1:  if (as->osc1.waveform < WAVE_COUNT) as->osc1.waveform++; break;
		case 2:  if (w->fieldpointer) { if (as->osc1.fm < 255 - 16) as->osc1.fm+=16; else as->osc1.fm = 255; } else if (as->osc1.fm < 255) as->osc1.fm++; break;
		case 5:  if (w->fieldpointer) { if (as->osc1.lfo < 255 - 16) as->osc1.lfo+=16; else as->osc1.lfo = 255; } else if (as->osc1.lfo < 255) as->osc1.lfo++; break;
		case 6:  if (w->fieldpointer) { if (as->osc2.mix < 255 - 16) as->osc2.mix+=16; else as->osc2.mix = 255; } else if (as->osc2.mix < 255) as->osc2.mix++; break;
		case 7:  if (as->osc2.waveform < WAVE_COUNT) as->osc2.waveform++; break;
		case 8:  if (as->osc2.oct < 9) as->osc2.oct++; break;
		case 9:  if (as->osc2.semi < 15) as->osc2.semi++; break;
		case 10: if (w->fieldpointer) { if (as->osc2.detune < 255 - 16) as->osc2.detune+=16; else as->osc2.detune = 255; } else if (as->osc2.detune < 255) as->osc2.detune++; break;
		case 12: if (w->fieldpointer) { if (as->osc2.lfo < 255 - 16) as->osc2.lfo+=16; else as->osc2.lfo = 255; } else if (as->osc2.lfo < 255) as->osc2.lfo++; break;
		case 13: if (as->sub.oct < 0) as->sub.oct++; break;
		case 14: if (w->fieldpointer) { if (as->sub.mix < 255 - 16) as->sub.mix+=16; else as->sub.mix = 255; } else if (as->sub.mix < 255) as->sub.mix++; break;
		case 15: if (as->sub.waveform < WAVE_COUNT) as->sub.waveform++; break;
		case 16: if (w->fieldpointer) { if (as->filter.cutoff < 255 - 16) as->filter.cutoff+=16; else as->filter.cutoff = 255; } else if (as->filter.cutoff < 255) as->filter.cutoff++; break;
		case 17: if (w->fieldpointer) { if (as->filter.resonance < 255 - 16) as->filter.resonance+=16; else as->filter.resonance = 255; } else if (as->filter.resonance < 255) as->filter.resonance++; break;
		case 18: if (w->fieldpointer) { if (as->filter.lfo < 255 - 16) as->filter.lfo+=16; else as->filter.lfo = 255; } else if (as->filter.lfo < 255) as->filter.lfo++; break;
		case 19: if (w->fieldpointer) { if (as->filter.track < 255 - 16) as->filter.track+=16; else as->filter.track = 255; } else if (as->filter.track < 255) as->filter.track++; break;
		case 20: if (w->fieldpointer) { if (as->filter.env.a < 255 - 16) as->filter.env.a+=16; else as->filter.env.a = 255; } else if (as->filter.env.a < 255) as->filter.env.a++; break;
		case 21: if (w->fieldpointer) { if (as->filter.env.d < 255 - 16) as->filter.env.d+=16; else as->filter.env.d = 255; } else if (as->filter.env.d < 255) as->filter.env.d++; break;
		case 22: if (w->fieldpointer) { if (as->filter.env.s < 255 - 16) as->filter.env.s+=16; else as->filter.env.s = 255; } else if (as->filter.env.s < 255) as->filter.env.s++; break;
		case 23: if (w->fieldpointer) { if (as->filter.env.r < 255 - 16) as->filter.env.r+=16; else as->filter.env.r = 255; } else if (as->filter.env.r < 255) as->filter.env.r++; break;
		case 25: if (as->lfo.waveform < WAVE_COUNT) as->lfo.waveform++; break;
		case 26: if (w->fieldpointer) { if (as->lfo.rate < 255 - 16) as->lfo.rate+=16; else as->lfo.rate = 255; } else if (as->lfo.rate < 255) as->lfo.rate++; break;
		case 27: if (w->fieldpointer) { if (as->lfo.pwm < 255 - 16) as->lfo.pwm+=16; else as->lfo.pwm = 255; } else if (as->lfo.pwm < 255) as->lfo.pwm++; break;
		case 28: if (w->fieldpointer) { if (as->amp.a < 255 - 16) as->amp.a+=16; else as->amp.a = 255; } else if (as->amp.a < 255) as->amp.a++; break;
		case 29: if (w->fieldpointer) { if (as->amp.d < 255 - 16) as->amp.d+=16; else as->amp.d = 255; } else if (as->amp.d < 255) as->amp.d++; break;
		case 30: if (w->fieldpointer) { if (as->amp.s < 255 - 16) as->amp.s+=16; else as->amp.s = 255; } else if (as->amp.s < 255) as->amp.s++; break;
		case 31: if (w->fieldpointer) { if (as->amp.r < 255 - 16) as->amp.r+=16; else as->amp.r = 255; } else if (as->amp.r < 255) as->amp.r++; break;
		case 32: if (w->fieldpointer) { if (as->noise < 255 - 16) as->noise+=16; else as->noise = 255; } else if (as->noise < 255) as->noise++; break;
		case 33: if (as->unison < 15) as->unison++; break;
		case 34: if (w->fieldpointer) { if (as->detune < 255 - 16) as->detune+=16; else as->detune = 255; } else if (as->detune < 255) as->detune++; break;
		case 35: if (as->stereo < 15) as->stereo++; break;
	}
}
void analogueAdjustDown(instrument *iv, short index, char mouse)
{
	analogue_state *as = iv->state[iv->type];
	switch (index)
	{
		case 0:  if (w->fieldpointer) { if (as->osc1.mix > 16) as->osc1.mix-=16; else as->osc1.mix = 0; } else if (as->osc1.mix) as->osc1.mix--; break;
		case 1:  if (as->osc1.waveform > 0) as->osc1.waveform--; break;
		case 2:  if (w->fieldpointer) { if (as->osc1.fm > 16) as->osc1.fm-=16; else as->osc1.fm = 0; } else if (as->osc1.fm) as->osc1.fm--; break;
		case 5:  if (w->fieldpointer) { if (as->osc1.lfo > 16) as->osc1.lfo-=16; else as->osc1.lfo = 0; } else if (as->osc1.lfo) as->osc1.lfo--; break;
		case 6:  if (w->fieldpointer) { if (as->osc2.mix > 16) as->osc2.mix-=16; else as->osc2.mix = 0; } else if (as->osc2.mix) as->osc2.mix--; break;
		case 7:  if (as->osc2.waveform > 0) as->osc2.waveform--; break;
		case 8:  if (as->osc2.oct > -9) as->osc2.oct--; break;
		case 9:  if (as->osc2.semi > 0) as->osc2.semi--; break;
		case 10: if (w->fieldpointer) { if (as->osc2.detune > 16) as->osc2.detune-=16; else as->osc2.detune = 0; } else if (as->osc2.detune) as->osc2.detune--; break;
		case 12: if (w->fieldpointer) { if (as->osc2.lfo > 16) as->osc2.lfo-=16; else as->osc2.lfo = 0; } else if (as->osc2.lfo) as->osc2.lfo--; break;
		case 13: if (as->sub.oct > -9) as->sub.oct--; break;
		case 14: if (w->fieldpointer) { if (as->sub.mix > 16) as->sub.mix-=16; else as->sub.mix = 0; } else if (as->sub.mix) as->sub.mix--; break;
		case 15: if (as->sub.waveform > 0) as->sub.waveform--; break;
		case 16: if (w->fieldpointer) { if (as->filter.cutoff > 16) as->filter.cutoff-=16; else as->filter.cutoff = 0; } else if (as->filter.cutoff) as->filter.cutoff--; break;
		case 17: if (w->fieldpointer) { if (as->filter.resonance > 16) as->filter.resonance-=16; else as->filter.resonance = 0; } else if (as->filter.resonance) as->filter.resonance--; break;
		case 18: if (w->fieldpointer) { if (as->filter.lfo > 16) as->filter.lfo-=16; else as->filter.lfo = 0; } else if (as->filter.lfo) as->filter.lfo--; break;
		case 19: if (w->fieldpointer) { if (as->filter.track > 16) as->filter.track-=16; else as->filter.track = 0; } else if (as->filter.track) as->filter.track--; break;
		case 20: if (w->fieldpointer) { if (as->filter.env.a > 16) as->filter.env.a-=16; else as->filter.env.a = 0; } else if (as->filter.env.a) as->filter.env.a--; break;
		case 21: if (w->fieldpointer) { if (as->filter.env.d > 16) as->filter.env.d-=16; else as->filter.env.d = 0; } else if (as->filter.env.d) as->filter.env.d--; break;
		case 22: if (w->fieldpointer) { if (as->filter.env.s > 16) as->filter.env.s-=16; else as->filter.env.s = 0; } else if (as->filter.env.s) as->filter.env.s--; break;
		case 23: if (w->fieldpointer) { if (as->filter.env.r > 16) as->filter.env.r-=16; else as->filter.env.r = 0; } else if (as->filter.env.r) as->filter.env.r--; break;
		case 25: if (as->lfo.waveform > 0) as->lfo.waveform--; break;
		case 26: if (w->fieldpointer) { if (as->lfo.rate > 16) as->lfo.rate-=16; else as->lfo.rate = 0; } else if (as->lfo.rate) as->lfo.rate--; break;
		case 27: if (w->fieldpointer) { if (as->lfo.pwm > 16) as->lfo.pwm-=16; else as->lfo.pwm = 0; } else if (as->lfo.pwm) as->lfo.pwm--; break;
		case 28: if (w->fieldpointer) { if (as->amp.a > 16) as->amp.a-=16; else as->amp.a = 0; } else if (as->amp.a) as->amp.a--; break;
		case 29: if (w->fieldpointer) { if (as->amp.d > 16) as->amp.d-=16; else as->amp.d = 0; } else if (as->amp.d) as->amp.d--; break;
		case 30: if (w->fieldpointer) { if (as->amp.s > 16) as->amp.s-=16; else as->amp.s = 0; } else if (as->amp.s) as->amp.s--; break;
		case 31: if (w->fieldpointer) { if (as->amp.r > 16) as->amp.r-=16; else as->amp.r = 0; } else if (as->amp.r) as->amp.r--; break;
		case 32: if (w->fieldpointer) { if (as->noise > 16) as->noise-=16; else as->noise = 0; } else if (as->noise) as->noise--; break;
		case 33: if (as->unison > 0) as->unison--; break;
		case 34: if (w->fieldpointer) { if (as->detune > 16) as->detune-=16; else as->detune = 0; } else if (as->detune) as->detune--; break;
		case 35: if (as->stereo > 0) as->stereo--; break;
	}
}
void analogueAdjustLeft(instrument *iv, short index, char mouse)
{
	switch (index)
	{
		case 0:  if (!mouse) w->fieldpointer = 1; break;
		case 2:  if (!mouse) w->fieldpointer = 1; break;
		case 5:  if (!mouse) w->fieldpointer = 1; break;
		case 6:  if (!mouse) w->fieldpointer = 1; break;
		case 10: if (!mouse) w->fieldpointer = 1; break;
		case 12: if (!mouse) w->fieldpointer = 1; break;
		case 14: if (!mouse) w->fieldpointer = 1; break;
		case 16: if (!mouse) w->fieldpointer = 1; break;
		case 17: if (!mouse) w->fieldpointer = 1; break;
		case 18: if (!mouse) w->fieldpointer = 1; break;
		case 19: if (!mouse) w->fieldpointer = 1; break;
		case 20: if (!mouse) w->fieldpointer = 1; break;
		case 21: if (!mouse) w->fieldpointer = 1; break;
		case 22: if (!mouse) w->fieldpointer = 1; break;
		case 23: if (!mouse) w->fieldpointer = 1; break;
		case 26: if (!mouse) w->fieldpointer = 1; break;
		case 27: if (!mouse) w->fieldpointer = 1; break;
		case 28: if (!mouse) w->fieldpointer = 1; break;
		case 29: if (!mouse) w->fieldpointer = 1; break;
		case 30: if (!mouse) w->fieldpointer = 1; break;
		case 31: if (!mouse) w->fieldpointer = 1; break;
		case 32: if (!mouse) w->fieldpointer = 1; break;
		case 34: if (!mouse) w->fieldpointer = 1; break;
	}
}
void analogueAdjustRight(instrument *iv, short index, char mouse)
{
	switch (index)
	{
		case 0:  if (!mouse) w->fieldpointer = 0; break;
		case 2:  if (!mouse) w->fieldpointer = 0; break;
		case 5:  if (!mouse) w->fieldpointer = 0; break;
		case 6:  if (!mouse) w->fieldpointer = 0; break;
		case 10: if (!mouse) w->fieldpointer = 0; break;
		case 12: if (!mouse) w->fieldpointer = 0; break;
		case 14: if (!mouse) w->fieldpointer = 0; break;
		case 16: if (!mouse) w->fieldpointer = 0; break;
		case 17: if (!mouse) w->fieldpointer = 0; break;
		case 18: if (!mouse) w->fieldpointer = 0; break;
		case 19: if (!mouse) w->fieldpointer = 0; break;
		case 20: if (!mouse) w->fieldpointer = 0; break;
		case 21: if (!mouse) w->fieldpointer = 0; break;
		case 22: if (!mouse) w->fieldpointer = 0; break;
		case 23: if (!mouse) w->fieldpointer = 0; break;
		case 26: if (!mouse) w->fieldpointer = 0; break;
		case 27: if (!mouse) w->fieldpointer = 0; break;
		case 28: if (!mouse) w->fieldpointer = 0; break;
		case 29: if (!mouse) w->fieldpointer = 0; break;
		case 30: if (!mouse) w->fieldpointer = 0; break;
		case 31: if (!mouse) w->fieldpointer = 0; break;
		case 32: if (!mouse) w->fieldpointer = 0; break;
		case 34: if (!mouse) w->fieldpointer = 0; break;
	}
}

void analogueEndFieldPointer(short _)
{
	w->fieldpointer = 0;
}

void analogueIncFieldPointer(short index)
{
	switch (index)
	{
		case 0:  w->fieldpointer = 0; break;
		case 2:  w->fieldpointer = 0; break;
		case 5:  w->fieldpointer = 0; break;
		case 6:  w->fieldpointer = 0; break;
		case 10: w->fieldpointer = 0; break;
		case 12: w->fieldpointer = 0; break;
		case 14: w->fieldpointer = 0; break;
		case 16: w->fieldpointer = 0; break;
		case 17: w->fieldpointer = 0; break;
		case 18: w->fieldpointer = 0; break;
		case 19: w->fieldpointer = 0; break;
		case 20: w->fieldpointer = 0; break;
		case 21: w->fieldpointer = 0; break;
		case 22: w->fieldpointer = 0; break;
		case 23: w->fieldpointer = 0; break;
		case 26: w->fieldpointer = 0; break;
		case 27: w->fieldpointer = 0; break;
		case 28: w->fieldpointer = 0; break;
		case 29: w->fieldpointer = 0; break;
		case 30: w->fieldpointer = 0; break;
		case 31: w->fieldpointer = 0; break;
		case 32: w->fieldpointer = 0; break;
		case 34: w->fieldpointer = 0; break;
	}
}
void analogueDecFieldPointer(short index)
{
	switch (index)
	{
		case 0:  w->fieldpointer = 1; break;
		case 2:  w->fieldpointer = 1; break;
		case 5:  w->fieldpointer = 1; break;
		case 6:  w->fieldpointer = 1; break;
		case 10: w->fieldpointer = 1; break;
		case 12: w->fieldpointer = 1; break;
		case 14: w->fieldpointer = 1; break;
		case 16: w->fieldpointer = 1; break;
		case 17: w->fieldpointer = 1; break;
		case 18: w->fieldpointer = 1; break;
		case 19: w->fieldpointer = 1; break;
		case 20: w->fieldpointer = 1; break;
		case 21: w->fieldpointer = 1; break;
		case 22: w->fieldpointer = 1; break;
		case 23: w->fieldpointer = 1; break;
		case 26: w->fieldpointer = 1; break;
		case 27: w->fieldpointer = 1; break;
		case 28: w->fieldpointer = 1; break;
		case 29: w->fieldpointer = 1; break;
		case 30: w->fieldpointer = 1; break;
		case 31: w->fieldpointer = 1; break;
		case 32: w->fieldpointer = 1; break;
		case 34: w->fieldpointer = 1; break;
	}
}

void inputAnalogueHex(short index, analogue_state *as, char value)
{
	switch (index)
	{
		case 0:  updateFieldPush(&as->osc1.mix, value); break;
		case 2:  updateFieldPush(&as->osc1.fm, value); break;
		case 5:  updateFieldPush(&as->osc1.lfo, value); break;
		case 6:  updateFieldPush(&as->osc2.mix, value); break;
		case 9:  as->osc2.semi = value; break;
		case 10: updateFieldPush(&as->osc2.detune, value); break;
		case 12: updateFieldPush(&as->osc2.lfo, value); break;
		case 14: updateFieldPush(&as->sub.mix, value); break;
		case 17: updateFieldPush(&as->filter.cutoff, value); break;
		case 18: updateFieldPush(&as->filter.resonance, value); break;
		case 19: updateFieldPush(&as->filter.lfo, value); break;
		case 20: updateFieldPush(&as->filter.track, value); break;
		case 21: updateFieldPush(&as->filter.env.a, value); break;
		case 22: updateFieldPush(&as->filter.env.d, value); break;
		case 23: updateFieldPush(&as->filter.env.s, value); break;
		case 25: updateFieldPush(&as->filter.env.r, value); break;
		case 27: updateFieldPush(&as->lfo.rate, value); break;
		case 28: updateFieldPush(&as->lfo.pwm, value); break;
		case 29: updateFieldPush(&as->amp.a, value); break;
		case 30: updateFieldPush(&as->amp.d, value); break;
		case 31: updateFieldPush(&as->amp.s, value); break;
		case 32: updateFieldPush(&as->amp.r, value); break;
		case 33: updateFieldPush(&as->noise, value); break;
		case 34: as->unison = value; break;
		case 35: updateFieldPush(&as->detune, value); break;
		case 36: as->stereo = value; break;
	}
}
void analogueInput(int *input)
{
	instrument *iv = s->instrumentv[s->instrumenti[w->instrument]];
	analogue_state *as = iv->state[iv->type];
	switch (*input)
	{
		case 10: case 13: /* return */
			switch (w->instrumentindex)
			{
				case 3:  as->osc1.flags ^= 0b1;   *input = 0; break;
				case 4:  as->osc1.flags ^= 0b10;  *input = 0; break;
				case 11: as->osc2.flags ^= 0b1;   *input = 0; break;
				case 24: as->filter.flags ^= 0b1; *input = 0; break;
			}
			break;
		case 1:  /* ^a */ w->fieldpointer = 0; analogueAdjustUp(iv, w->instrumentindex, 0);   break;
		case 24: /* ^x */ w->fieldpointer = 0; analogueAdjustDown(iv, w->instrumentindex, 0); break;
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

void analogueMouseToIndex(int y, int x, int button, short *index)
{
	instrument *iv = s->instrumentv[s->instrumenti[w->instrument]];
	analogue_state *as = iv->state[iv->type];
	if (x < 20)
		switch (y)
		{
			case 1: case 2:   *index = 0; if (x < 16) w->fieldpointer = 1; else w->fieldpointer = 0; break;
			case 3:           *index = 1; break;
			case 4:
				if (x < 15) { *index = 2; if (x < 13) w->fieldpointer = 1; else w->fieldpointer = 0; }
				else        { *index = 3; as->osc1.flags ^= 0b1; }
				break;
			case 5:
				if (x < 14) { *index = 4; as->osc1.flags ^= 0b10; }
				else        { *index = 5; if (x < 16) w->fieldpointer = 1; else w->fieldpointer = 0; }
				break;
			case 6: case 7:   *index = 6; if (x < 16) w->fieldpointer = 1; else w->fieldpointer = 0; break;
			case 8:           *index = 7; break;
			case 9:
				if (x < 11)        *index = 8;
				else if (x < 14)   *index = 9;
				else             { *index = 10; if (x < 16) w->fieldpointer = 1; else w->fieldpointer = 0; }
				break;
			case 10:
				if (x < 14) { *index = 11; as->osc2.flags ^= 0b1; }
				else        { *index = 12; if (x < 16) w->fieldpointer = 1; else w->fieldpointer = 0; }
				break;
			case 11: case 12:
				if (x < 14)   *index = 13;
				else        { *index = 14; if (x < 16) w->fieldpointer = 1; else w->fieldpointer = 0; }
				break;
			default:          *index = 15; break;
		}
	else if (y < 8)
	{
		if (x < 49)
		{
			if (y >= 7) { *index = 24; as->filter.flags ^= 0b1; }
			else if (x < 34)
				switch (y)
				{
					case 1: case 2: case 3:  *index = 16; if (x < 31) w->fieldpointer = 1; else w->fieldpointer = 0; break;
					case 4:                  *index = 17; if (x < 31) w->fieldpointer = 1; else w->fieldpointer = 0; break;
					case 5:                  *index = 18; if (x < 31) w->fieldpointer = 1; else w->fieldpointer = 0; break;
					default:                 *index = 19; if (x < 31) w->fieldpointer = 1; else w->fieldpointer = 0; break;
				}
			else
				switch (y)
				{
					case 1: case 2: case 3:  *index = 20; if (x < 45) w->fieldpointer = 1; else w->fieldpointer = 0; break;
					case 4:                  *index = 21; if (x < 45) w->fieldpointer = 1; else w->fieldpointer = 0; break;
					case 5:                  *index = 22; if (x < 45) w->fieldpointer = 1; else w->fieldpointer = 0; break;
					default:                 *index = 23; if (x < 45) w->fieldpointer = 1; else w->fieldpointer = 0; break;
				}
		}
		else
			switch (y)
			{
				case 1: case 2: case 3:  *index = 28; if (x < 60) w->fieldpointer = 1; else w->fieldpointer = 0; break;
				case 4:                  *index = 29; if (x < 60) w->fieldpointer = 1; else w->fieldpointer = 0; break;
				case 5:                  *index = 30; if (x < 60) w->fieldpointer = 1; else w->fieldpointer = 0; break;
				default:                 *index = 31; if (x < 60) w->fieldpointer = 1; else w->fieldpointer = 0; break;
			}
	} else
	{
		if (x < 48)
			switch (y)
			{
				case 8: case 9: case 10:  *index = 25; break;
				case 11:                  *index = 26; if (x < 37) w->fieldpointer = 1; else w->fieldpointer = 0; break;
				default:                  *index = 27; if (x < 37) w->fieldpointer = 1; else w->fieldpointer = 0; break;
			}
		else
			switch (y)
			{
				case 8: case 9: case 10:  *index = 32; if (x < 60) w->fieldpointer = 1; else w->fieldpointer = 0; break;
				case 11:                  *index = 33; break;
				case 12:                  *index = 34; if (x < 60) w->fieldpointer = 1; else w->fieldpointer = 0; break;
				default:                  *index = 35; break;
			}
	}
}

float analogueInstance(analogue_state *as, analogue_channel *ac, channel *cv, uint8_t index, float detune)
{
	float pps = 1.0 / ((float)samplerate
		/ (C5_FREQ * powf(M_12_ROOT_2, (short)cv->r.note - C5 + detune)));

	ac->unison[index].lfophase += 1.0 / ((float)samplerate * LFO_MAX
		+ (float)samplerate * (LFO_MIN - LFO_MAX)
		* (1.0 - as->lfo.rate*DIV256) + detune);
	while (ac->unison[index].lfophase > 1.0) ac->unison[index].lfophase -= 1.0;

	float output = 0.0;
	float lfo = (oscillator(as->lfo.waveform, ac->unison[index].lfophase, 0.5) + 1.0) / 2;
	float pw = 0.5 * (1.0 + lfo * as->lfo.pwm*DIV256);

	/* osc2 */
	float osc2;
	if (as->osc1.flags & 0b10 && ac->unison[index].osc1phase >= 1.0) /* sync (1 sample behind, but that's fine) */
		ac->unison[index].osc2phase = 0.0;
	else
	{
		if (as->osc2.flags & 0b1) /* pitch tracking */
			ac->unison[index].osc2phase += pps
				* powf(M_12_ROOT_2, as->osc2.semi + (as->osc2.oct-1 + as->osc2.detune*DIV256 + 0.5) * 12)
				+ (lfo * FM_DEPTH * as->osc2.lfo*DIV256);
		else
			ac->unison[index].osc2phase += 1.0 / ((float)samplerate / C5_FREQ)
				* powf(M_12_ROOT_2, as->osc2.semi + (as->osc2.oct-1 + as->osc2.detune*DIV256 + 0.5) * 12)
				+ (lfo * FM_DEPTH * as->osc2.lfo*DIV256);
		ac->unison[index].osc2phase = fmodf(ac->unison[index].osc2phase, 1.0);
	}

	osc2 = oscillator(as->osc2.waveform, ac->unison[index].osc2phase, pw);
	output += osc2 * as->osc2.mix;

	float osc1lfo = lfo * FM_DEPTH * as->osc1.lfo*DIV256;

	/* osc1 */
	ac->unison[index].osc1phase = fmodf(ac->unison[index].osc1phase, 1.0);
	ac->unison[index].osc1phase += pps + osc1lfo + (osc2 * as->osc1.fm*DIV256);

	if (as->osc1.flags & 0b1) /* osc2 ringmod */
		output += oscillator(as->osc1.waveform, ac->unison[index].osc1phase, pw)
			* as->osc1.mix * fabsf(osc2) * 2;
	else
		output += oscillator(as->osc1.waveform, ac->unison[index].osc1phase, pw)
			* as->osc1.mix;

	/* sub */
	ac->unison[index].subphase += (pps + osc1lfo) * powf(2, as->sub.oct);
	ac->unison[index].subphase = fmodf(ac->unison[index].subphase, 1.0);

	output += oscillator(as->sub.waveform, ac->unison[index].subphase, pw)
		* as->sub.mix;

	return output*DIV256;
}

void analogueProcess(instrument *iv, channel *cv, uint32_t pointer, float *l, float *r)
{
	*l = 0.0;
	*r = 0.0;

	analogue_state *as = iv->state[iv->type];
	analogue_channel *ac = cv->state[iv->type];

	if (pointer == 0)
	{
		/* reset the filter state on trigger */
		/* otherwise the filter can get confused */
		ac->fl.a1 = ac->fl.a2 = 0.0;
		ac->fl.b1 = ac->fl.b2 = 0.0;
		ac->fl.n1 = ac->fl.n2 = 0.0;

		ac->fr.a1 = ac->fr.a2 = 0.0;
		ac->fr.b1 = ac->fr.b2 = 0.0;
		ac->fr.n1 = ac->fr.n2 = 0.0;
	}

	float again = adsrEnvelope(as->amp,        1.0, pointer, cv->releasepointer);
	float fgain = adsrEnvelope(as->filter.env, 0.0, pointer, cv->releasepointer);
	if (pointer > MAX(as->amp.a, as->filter.env.a) * ENVELOPE_ATTACK * samplerate
			&& again == 0.0 && fgain == 0.0) /* sound has fully finished */
	{
		cv->r.note = 0;
	} else
	{
		/* centre */
		*l = *r = analogueInstance(as, ac, cv, 2, cv->finetune) * again;

		float c, gain, detune, width;
		for (char i = 1; i <= as->unison; i++)
		{
			gain = i / as->unison;
			width = (1.0 - gain) * as->stereo / 16.0;
			detune = gain * as->detune*DIV256 + cv->finetune;
			if (i % 2)
			{
				c = analogueInstance(as, ac, cv, i * 2 + 0,  detune);
				*l += c * (gain + width);
				*r += c * MAX(gain - width, 0.0);
				c = analogueInstance(as, ac, cv, i * 2 + 1, -detune);
				*r += c * (gain + width);
				*l += c * MAX(gain - width, 0.0);
			} else
			{
				c = analogueInstance(as, ac, cv, i * 2 + 0, -detune);
				*l += c * (gain + width);
				*r += c * MAX(gain - width, 0.0);
				c = analogueInstance(as, ac, cv, i * 2 + 1,  detune);
				*r += c * (gain + width);
				*l += c * MAX(gain - width, 0.0);
			}
		}

		/* noise */
		float noise = getWnoise(&ac->wn) * as->noise / 512.0 * again; /* 256 * 2 */
		*l += noise;
		*r += noise;

		/* filter */
		float cutoff = fgain * as->filter.cutoff*DIV256
				* powf(M_12_ROOT_2, ((short)cv->r.note - C5 + cv->finetune) * as->filter.track*DIV256);
		if (as->filter.flags & 0b1) /* high resonance */
			calcFilter(&ac->fl, cutoff * -1, 1.0 - as->filter.resonance*DIV256);
		else
			calcFilter(&ac->fl, cutoff, as->filter.resonance*DIV256);
		*l = runFilter(&ac->fl, *l);
		*r = runFilter(&ac->fl, *r);
		ac->fl.n2 = ac->fl.n1; ac->fl.n1 = MIN(MAX(*l, -6.0), 6.0);
		ac->fr.n2 = ac->fr.n1; ac->fr.n1 = MIN(MAX(*r, -6.0), 6.0);


		*l *= again;
		*l *= again;

		// saturation
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

		/* asymmetry */
		if (*l < -1.5) *l = -1.5 - (*l + 1.5);
		if (*r < -1.5) *r = -1.5 - (*r + 1.5);
	}
}

void analogueAddType(void **state)
{
	*state = calloc(1, sizeof(analogue_state));
	analogue_state *as = *state;
	as->osc1.mix = 0x80;
	as->osc2.mix = 0x00;
	as->osc2.detune = 0x80;
	as->osc2.flags = 0b00000001; /* pitch tracking on */
	as->sub.oct = -1;
	as->sub.waveform = 4; /* sine */
	as->filter.cutoff = 255;
	as->filter.env.s = 255;
	as->amp.s = 255;
	as->lfo.rate = 0x80;
	as->sub.mix = 0;
	as->noise = 0;
}
void analogueCopyType(void **dest, void **src)
{ memcpy(*dest, *src, sizeof(analogue_state)); }
void analogueDelType(void **state)
{ free(*state); }

void analogueAddChannel(void **state)
{
	*state = calloc(1, sizeof(analogue_channel));
	analogue_channel *ac = *state;
	initWnoise(&ac->wn);
}
void analogueDelChannel(void **state)
{ free(*state); *state = NULL; }


void analogueWrite(void **state, FILE *fp)
{
	analogue_state *as = *state;

	fwrite(&as->osc1.waveform, sizeof(uint8_t), 1, fp);
	fwrite(&as->osc1.lfo, sizeof(uint8_t), 1, fp);
	fwrite(&as->osc1.mix, sizeof(uint8_t), 1, fp);
	fwrite(&as->osc1.fm, sizeof(uint8_t), 1, fp);
	fwrite(&as->osc1.flags, sizeof(uint8_t), 1, fp);
	fwrite(&as->osc2.waveform, sizeof(uint8_t), 1, fp);
	fwrite(&as->osc2.oct, sizeof(int8_t), 1, fp);
	fwrite(&as->osc2.semi, sizeof(uint8_t), 1, fp);
	fwrite(&as->osc2.detune, sizeof(uint8_t), 1, fp);
	fwrite(&as->osc2.lfo, sizeof(uint8_t), 1, fp);
	fwrite(&as->osc2.mix, sizeof(uint8_t), 1, fp);
	fwrite(&as->osc2.flags, sizeof(uint8_t), 1, fp);
	fwrite(&as->sub.waveform, sizeof(uint8_t), 1, fp);
	fwrite(&as->sub.oct, sizeof(int8_t), 1, fp);
	fwrite(&as->sub.mix, sizeof(uint8_t), 1, fp);
	fwrite(&as->lfo.rate, sizeof(uint8_t), 1, fp);
	fwrite(&as->lfo.waveform, sizeof(uint8_t), 1, fp);
	fwrite(&as->lfo.pwm, sizeof(uint8_t), 1, fp);
	fwrite(&as->filter.cutoff, sizeof(uint8_t), 1, fp);
	fwrite(&as->filter.resonance, sizeof(uint8_t), 1, fp);
	fwrite(&as->filter.lfo, sizeof(uint8_t), 1, fp);
	fwrite(&as->filter.env, sizeof(adsr), 1, fp);
	fwrite(&as->filter.track, sizeof(uint8_t), 1, fp);
	fwrite(&as->filter.flags, sizeof(uint8_t), 1, fp);
	fwrite(&as->amp, sizeof(adsr), 1, fp);
	fwrite(&as->noise, sizeof(uint8_t), 1, fp);
	fwrite(&as->unison, sizeof(uint8_t), 1, fp);
	fwrite(&as->detune, sizeof(uint8_t), 1, fp);
	fwrite(&as->stereo, sizeof(uint8_t), 1, fp);
}

void analogueRead(void **state, unsigned char major, unsigned char minor, FILE *fp)
{
	analogue_state *as = *state;

	fread(&as->osc1.waveform, sizeof(uint8_t), 1, fp);
	fread(&as->osc1.lfo, sizeof(uint8_t), 1, fp);
	fread(&as->osc1.mix, sizeof(uint8_t), 1, fp);
	fread(&as->osc1.fm, sizeof(uint8_t), 1, fp);
	fread(&as->osc1.flags, sizeof(uint8_t), 1, fp);
	fread(&as->osc2.waveform, sizeof(uint8_t), 1, fp);
	fread(&as->osc2.oct, sizeof(int8_t), 1, fp);
	fread(&as->osc2.semi, sizeof(uint8_t), 1, fp);
	fread(&as->osc2.detune, sizeof(uint8_t), 1, fp);
	fread(&as->osc2.lfo, sizeof(uint8_t), 1, fp);
	fread(&as->osc2.mix, sizeof(uint8_t), 1, fp);
	fread(&as->osc2.flags, sizeof(uint8_t), 1, fp);
	fread(&as->sub.waveform, sizeof(uint8_t), 1, fp);
	fread(&as->sub.oct, sizeof(int8_t), 1, fp);
	fread(&as->sub.mix, sizeof(uint8_t), 1, fp);
	fread(&as->lfo.rate, sizeof(uint8_t), 1, fp);
	fread(&as->lfo.waveform, sizeof(uint8_t), 1, fp);
	fread(&as->lfo.pwm, sizeof(uint8_t), 1, fp);
	fread(&as->filter.cutoff, sizeof(uint8_t), 1, fp);
	fread(&as->filter.resonance, sizeof(uint8_t), 1, fp);
	fread(&as->filter.lfo, sizeof(uint8_t), 1, fp);
	fread(&as->filter.env, sizeof(adsr), 1, fp);
	fread(&as->filter.track, sizeof(uint8_t), 1, fp);
	fread(&as->filter.flags, sizeof(uint8_t), 1, fp);
	fread(&as->amp, sizeof(adsr), 1, fp);
	fread(&as->noise, sizeof(uint8_t), 1, fp);
	fread(&as->unison, sizeof(uint8_t), 1, fp);
	fread(&as->detune, sizeof(uint8_t), 1, fp);
	fread(&as->stereo, sizeof(uint8_t), 1, fp);
}

void analogueInit(int index)
{
	t->f[index].indexc = 35;
	t->f[index].cellwidth = 62;
	t->f[index].statesize = sizeof(analogue_state);
	t->f[index].draw = &drawAnalogue;
	t->f[index].adjustUp = &analogueAdjustUp;
	t->f[index].adjustDown = &analogueAdjustDown;
	t->f[index].adjustLeft = &analogueAdjustLeft;
	t->f[index].adjustRight = &analogueAdjustRight;
	t->f[index].incFieldPointer = &analogueIncFieldPointer;
	t->f[index].decFieldPointer = &analogueDecFieldPointer;
	t->f[index].endFieldPointer = &analogueEndFieldPointer;
	t->f[index].mouseToIndex = &analogueMouseToIndex;
	t->f[index].input = &analogueInput;
	t->f[index].process = &analogueProcess;
	t->f[index].addType = &analogueAddType;
	t->f[index].copyType = &analogueCopyType;
	t->f[index].delType = &analogueDelType;
	t->f[index].addChannel = &analogueAddChannel;
	t->f[index].delChannel = &analogueDelChannel;
	t->f[index].write = &analogueWrite;
	t->f[index].read = &analogueRead;
}
