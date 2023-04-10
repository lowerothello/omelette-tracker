static uint32_t getDummyEffectControlCount(void *state)
{
	return 1;
}

static short getDummyEffectHeight(void *state)
{
	return DUMMY_EFFECT_HEIGHT;
}

static void drawDummyEffect(void *state, short x, short w, short y, short ymin, short ymax)
{
	printf("\033[7m");
	if (ymin <= y-1 && ymax >= y-1)
		printCulling("NULL", x+1, y-1, 1, ws.ws_col);
	printf("\033[27;37;40m");

	if (ymin <= y && ymax >= y)
	{
		printf("\033[1m");
		drawCentreText(x+2, y, w-4, DUMMY_EFFECT_TEXT);
		printf("\033[22m");
	}

	addControlDummy(x + w - 3, y);
}

static struct json_object *serializeDummyEffect(void *state)
{
	return NULL;
}

static void *deserializeDummyEffect(struct json_object *jso, float **input, float **output)
{
	return NULL;
}
