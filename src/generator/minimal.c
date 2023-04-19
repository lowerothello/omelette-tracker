void processMinimal(Sample *sample, uint32_t pointer, uint8_t decimate, int8_t bitdepth, float note, short *l, short *r)
{
	float calcrate = (float)sample->rate / (float)samplerate;
	float calcpitch = powf(M_12_ROOT_2, note - NOTE_C5);

	if (sample->channels == 1)
	{
		getSample(pointer*calcrate*calcpitch, 0, decimate, bitdepth, sample, l);
		getSample(pointer*calcrate*calcpitch, 0, decimate, bitdepth, sample, r);
	} else
	{
		getSample(pointer*calcrate*calcpitch * sample->channels + 0, 0, decimate, bitdepth, sample, l);
		getSample(pointer*calcrate*calcpitch * sample->channels + 1, 0, decimate, bitdepth, sample, r);
	}
}
