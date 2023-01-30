void processMinimal(Sample *sample, uint32_t pointer, uint8_t decimate, int8_t bitdepth, short note, short *l, short *r)
{
	float calcrate = (float)sample->rate / (float)samplerate;
	float calcpitch = powf(M_12_ROOT_2, note - NOTE_C5);

	if (sample->channels == 1)
	{
		getSample(pointer*calcrate*calcpitch, decimate, bitdepth, sample, l);
		getSample(pointer*calcrate*calcpitch, decimate, bitdepth, sample, r);
	} else
	{
		getSample(pointer*calcrate*calcpitch * sample->channels + 0, decimate, bitdepth, sample, l);
		getSample(pointer*calcrate*calcpitch * sample->channels + 1, decimate, bitdepth, sample, r);
	}
}

static void samplerInstUISimpleCallback(short x, short y, Instrument *iv, uint8_t index)
{
	switch (index)
	{
		case 0:
			printf("\033[%d;%dH + [           ] + ", y, x);
			addControlInt(&cc, x+10, y, &iv->sample->rate, 8, 0x0, 0xffffffff, iv->sample->defrate, 0, 0, (void(*)(void*))instrumentControlCallback, NULL);
			break;
	}
}
void initInstUISimpleSamplerBlock(InstUIBlock *block)
{
	block->count = 1;
	block->callback = samplerInstUISimpleCallback;
}
