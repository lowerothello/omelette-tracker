uint32_t getDummyEffectControlCount(void *state)
{
	return 1;
}

short getDummyEffectHeight(void *state)
{
	return DUMMY_EFFECT_HEIGHT;
}

void drawDummyEffect(void *state, short x, short w, short y, short ymin, short ymax)
{
	if (ymin <= y-1 && ymax >= y-1)
		printf("\033[%d;%dH\033[7mNULL\033[27m", y-1, x + 1);
	printf("\033[37;40m");

	if (ymin <= y && ymax >= y)
	{
		printf("\033[1m");
		drawCentreText(x+2, y, w-4, DUMMY_EFFECT_TEXT);
		printf("\033[22m");
	}

	addControlDummy(x + w - 3, y);
}
