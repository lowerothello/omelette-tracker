typedef struct
{
	int8_t  sync;
	uint8_t offset; /* pm mod target */
	int8_t  compression;

	float workphase; /* uncompressed */
	float syncphase;
} Phase;

float runPhase(Phase *p, float delta)
{
	p->workphase += delta;
	p->syncphase += delta * (p->sync+128)*DIV128; /* TODO: should be logarithmic */

	while (p->syncphase > 1.0f)
	{
		p->workphase = 0.0f;
		p->syncphase -= 1.0f;
	}

	while (p->workphase > 1.0f) p->workphase -= 1.0f;

	float ret = p->workphase + p->offset*DIV255;
	if (ret > 1.0f) ret -= 1.0f; /* only need to run once */

	/* TODO: handle phase compression */

	return ret;
}
