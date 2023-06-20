#define COMMAND_GAIN        'G'
#define COMMAND_SMOOTH_GAIN 'g'

void commandGainClear(Track *cv, void *state)
{
	CommandState *s = state;
	s->base = s->rand = 0x88;
	s->target = -1;
	s->target_rand = 0;
}

void commandGainPreTrig(uint32_t fptr, uint16_t *spr, Track *cv, Row *r, void *state)
{
	commandStateApply(state);

	Command *m;
	FOR_ROW_COMMANDS(i, cv)
	{
		m = &r->command[i];
		switch (m->c)
		{
			case COMMAND_GAIN: commandStateSet(state, m); break;
			case COMMAND_SMOOTH_GAIN: commandStateSmooth(state, m); break;
		}
	}
}

void commandGainPostSampler(uint32_t fptr, Track *cv, float rp, float *lf, float *rf, void *state)
{
	float lm = 0.0f, rm = 0.0f;
	commandStateGetStereo(state, rp, &lm, &rm);
	*lf *= lm;
	*rf *= rm;
}
