bool ifCommand(Track *cv, Row *r, char m)
{
	for (int i = 0; i <= cv->pattern->commandc; i++)
		if (r->command[i].c == m)
			return 1;
	return 0;
}

/* ifCommand(), but run .triggercallback */
void ifCommandCallback(uint32_t fptr, uint16_t *spr, Track *cv, Row *r, char m, void (*triggercallback)(uint32_t, uint16_t*, int, Track*, Row*))
{
	for (int i = 0; i <= cv->pattern->commandc; i++)
		if (r->command[i].c == m && COMMAND_SET(r->command[i].c))
		{
			triggercallback(fptr, spr, r->command[i].v, cv, r);
			return;
		}
	triggercallback(fptr, spr, -1, cv, r);
}

/* if the row needs to be ramped in based on the commands present */
bool ifCommandRamp(Track *cv, Row *r)
{
	for (int i = 0; i <= cv->pattern->commandc; i++)
		if (COMMAND_RAMP(r->command[i].c)) return 1;

	return 0;
}

void commandCallbackClear(Track *cv)
{
	for (size_t i = 0; i < COMMAND_CALLBACK_MAX; i++)
		if (global_command_callbacks[i].clear)
			global_command_callbacks[i].clear(cv, cv->commandstate[i]);
}
void commandCallbackPreTrig(uint32_t fptr, uint16_t *spr, Track *cv, Row *r)
{
	for (size_t i = 0; i < COMMAND_CALLBACK_MAX; i++)
		if (global_command_callbacks[i].pretrig)
			global_command_callbacks[i].pretrig(fptr, spr, cv, r, cv->commandstate[i]);
}
void commandCallbackPostTrig(uint32_t fptr, uint16_t *spr, Track *cv, Row *r)
{
	for (size_t i = 0; i < COMMAND_CALLBACK_MAX; i++)
		if (global_command_callbacks[i].posttrig)
			global_command_callbacks[i].posttrig(fptr, spr, cv, r, cv->commandstate[i]);
}
void commandCallbackTriggerNote(uint32_t fptr, Track *cv, float oldnote, float note, short inst)
{
	for (size_t i = 0; i < COMMAND_CALLBACK_MAX; i++)
		if (global_command_callbacks[i].triggernote)
			global_command_callbacks[i].triggernote(fptr, cv, oldnote, note, inst, cv->commandstate[i]);
}
float commandCallbackSampleRow(uint32_t fptr, uint16_t count, uint16_t *spr, uint16_t sprp, Track *cv)
{
	float tryret, ret = NOTE_VOID;
	for (size_t i = 0; i < COMMAND_CALLBACK_MAX; i++)
		if (global_command_callbacks[i].samplerow)
		{
			tryret = global_command_callbacks[i].samplerow(fptr, count, spr, sprp, cv, cv->commandstate[i]);
			if (tryret != NOTE_VOID)
				ret = tryret;
		}
	return ret;
}
void commandCallbackPersistent(uint32_t fptr, uint16_t count, uint16_t *spr, uint16_t sprp, Track *cv)
{
	for (size_t i = 0; i < COMMAND_CALLBACK_MAX; i++)
		if (global_command_callbacks[i].persistenttune)
			global_command_callbacks[i].persistenttune(fptr, count, spr, sprp, cv, cv->commandstate[i]);
}
void commandCallbackVolatile(uint32_t fptr, uint16_t count, uint16_t *spr, uint16_t sprp, Track *cv, float *note)
{
	for (size_t i = 0; i < COMMAND_CALLBACK_MAX; i++)
		if (global_command_callbacks[i].volatiletune)
			global_command_callbacks[i].volatiletune(fptr, count, spr, sprp, cv, note, cv->commandstate[i]);
}
void commandCallbackPostSampler(uint32_t fptr, Track *cv, float rp, float *lf, float *rf)
{
	for (size_t i = 0; i < COMMAND_CALLBACK_MAX; i++)
		if (global_command_callbacks[i].postsampler)
			global_command_callbacks[i].postsampler(fptr, cv, rp, lf, rf, cv->commandstate[i]);
}

static int swapCase(int x)
{
	if (islower(x)) return toupper(x);
	else            return tolower(x);
}

bool changeCommand(int input, char *dest)
{
	/* use the current value if input is '\0' */
	if (!input) input = *dest; /* dest is pre-swapped */
	else        input = swapCase(input);

	if (COMMAND_SET(input))
		*dest = input;

	return 0;
}

void addCommandBinds(const char *prettyname, unsigned int state, void (*callback)(void*))
{
	addTooltipPrettyPrint(prettyname, state, "command");
	for (size_t i = 0; i < COMMAND_MAX; i++)
		if (COMMAND_SET(i))
			addTooltipBind(COMMAND_PRETTYNAME(i), state, swapCase(i), 0, callback, (void*)i);
}


void commandStateReset(CommandState *s)
{
	s->base = -1;
	s->rand = -1;
	s->target = -1;
}
void commandStateSet(CommandState *s, Command *m)
{
	uint8_t hold;
	short oversample;
	if (m->t) /* special tokens are present */
	{
		switch (m->t>>4)
		{
			case 1: /* ~ */
				s->lfo_stereo = 0;
				s->lfospeed = m->v&15;
				break;
			case 2: /* + */
				hold = m->v&15;
				if (!hold) hold = 1;
				hold *= 0x10;

				oversample = (short)s->rand + (short)hold;
				oversample = MIN(oversample, 0xff);
				oversample = MAX(oversample, 0x00);
				s->rand = oversample;
				break;
			case 3: /* - */
				hold = m->v&15;
				if (!hold) hold = 1;
				hold *= 0x10;

				oversample = (short)s->rand - (short)hold;
				oversample = MIN(oversample, 0xff);
				oversample = MAX(oversample, 0x00);
				s->rand = oversample;
				break;
			case 4: /* % */
				hold = m->v&15;
				if (!hold) hold = 1;
				hold++;

				if (COMMAND_STEREO(m->c))
				{
					hold<<=4;
					oversample = rand()%hold;
					s->rand = MAX(0, s->base - oversample);
				} else
				{
					oversample = rand()%hold;
					s->rand =
						 (MAX(0, (s->base>>4) - oversample)<<4)
						+ MAX(0, (s->base&15) - oversample);
				} break;
		}

		switch (m->t&15)
		{
			case 1: /* ~ */
				s->lfo_stereo = 1;
				s->lfospeed = m->v>>4;
				break;
			case 2: /* + */
				hold = m->v>>4;
				if (!hold) hold = 1;

				oversample = (short)s->rand + (short)hold;
				oversample = MIN(oversample, 0xff);
				oversample = MAX(oversample, 0x00);
				s->rand = oversample;
				break;
			case 3: /* - */
				hold = m->v>>4;
				if (!hold) hold = 1;

				oversample = (short)s->rand - (short)hold;
				oversample = MIN(oversample, 0xff);
				oversample = MAX(oversample, 0x00);
				s->rand = oversample;
				break;
			case 4: /* % */
				hold = m->v>>4;
				if (!hold) hold = 1;
				hold++;

				if (COMMAND_STEREO(m->c))
				{
					hold<<=4;
					oversample = rand()%hold;
					s->rand = MAX(0, s->base - oversample);
				} else
				{
					s->rand =
						 (MAX(0, (s->base>>4) - (short)(rand()%hold))<<4)
						+ MAX(0, (s->base&15) - (short)(rand()%hold));
				} break;
		}
	} else
	{
		s->base = m->v;
		s->rand = m->v;
	}
}
void commandStateSmooth(CommandState *s, Command *m)
{
	uint8_t hold;
	short oversample;
	if (m->t) /* special tokens are present */
	{
		switch (m->t>>4)
		{
			case 1: /* ~ */
				s->lfo_stereo  = 0;
				s->target_rand = 1;
				s->target = s->rand; /* mark as smooth */
				s->lfospeed = m->v&15;
				break;
			case 2: /* + */
				s->target_rand = 1;
				hold = m->v&15;
				if (!hold) hold = 1;
				hold *= 0x10;

				oversample = (short)s->rand + (short)hold;
				oversample = MIN(oversample, 0xff);
				oversample = MAX(oversample, 0x00);
				s->target = oversample;
				break;
			case 3: /* - */
				s->target_rand = 1;
				hold = m->v&15;
				if (!hold) hold = 1;
				hold *= 0x10;

				oversample = (short)s->rand - (short)hold;
				oversample = MIN(oversample, 0xff);
				oversample = MAX(oversample, 0x00);
				s->target = oversample;
				break;
			case 4: /* % */
				s->target_rand = 1;
				hold = m->v&15;
				if (!hold) hold = 1;
				hold++;

				if (COMMAND_STEREO(m->c))
				{
					hold<<=4;
					oversample = rand()%hold;
					s->target = MAX(0, s->base - oversample);
				} else
				{
					oversample = rand()%hold;
					s->target =
						 (MAX(0, (s->base>>4) - oversample)<<4)
						+ MAX(0, (s->base&15) - oversample);
				} break;
		}

		switch (m->t&15)
		{
			case 1: /* ~ */
				s->lfo_stereo  = 1;
				s->target_rand = 1;
				s->target = s->rand; /* mark as smooth */
				s->lfospeed = m->v>>4;
				break;
			case 2: /* + */
				s->target_rand = 1;
				hold = m->v>>4;
				if (!hold) hold = 1;

				oversample = (short)s->rand + (short)hold;
				oversample = MIN(oversample, 0xff);
				oversample = MAX(oversample, 0x00);
				s->target = oversample;
				break;
			case 3: /* - */
				s->target_rand = 1;
				hold = m->v>>4;
				if (!hold) hold = 1;

				oversample = (short)s->rand - (short)hold;
				oversample = MIN(oversample, 0xff);
				oversample = MAX(oversample, 0x00);
				s->target = oversample;
				break;
			case 4: /* % */
				s->target_rand = 1;
				hold = m->v>>4;
				if (!hold) hold = 1;
				hold++;

				if (COMMAND_STEREO(m->c))
				{
					hold<<=4;
					oversample = rand()%hold;
					s->target = MAX(0, s->base - oversample);
				} else
				{
					s->target =
						 (MAX(0, (s->base>>4) - (short)(rand()%hold))<<4)
						+ MAX(0, (s->base&15) - (short)(rand()%hold));
				} break;
		}
	} else
		s->target = m->v;
}
void commandStateApply(CommandState *s)
{
	s->lfospeed = 0;
	if (s->target != -1)
	{
		if (s->target_rand) /* only apply the new gain to rand, not to base */
		{
			s->rand = s->target;
			s->target_rand = 0;
		} else
		{
			s->base = s->target;
			s->rand = s->target;
		}
		s->target = -1;
	}
}
float commandStateGetMono(CommandState *s, float rp)
{
	if (s->rand == -1)
	{
		DEBUG=-2; p->redraw = 1;
		return NAN;
	}

	if (s->target != -1)
	{
		float ret = s->rand*DIV256 + (s->target - s->rand)*DIV256 * rp;
		if (s->lfospeed)
		{
			float lfo = fmodf(rp, 1.0f / s->lfospeed) * 2.0f;
			if (lfo > 1.0f) lfo = 2.0f - lfo;
			lfo = 1.0f - lfo;
			return ret * lfo;
		} else return ret;
	} else
	{
		if (s->lfospeed && fmodf(rp, 1.0f / s->lfospeed) > 0.5f)
			return 0.0f;
		return s->rand*DIV256;
	}
}
void commandStateGetStereo(CommandState *s, float rp, float *l, float *r)
{
	if (s->rand == -1)
		return;

	if (s->target != -1)
	{
		*l = (s->rand>>4)*DIV16 + ((s->target>>4) - (s->rand>>4))*DIV16 * rp;
		*r = (s->rand&15)*DIV16 + ((s->target&15) - (s->rand&15))*DIV16 * rp;

		if (s->lfospeed)
		{
			float lfo = fmodf(rp, 1.0f / s->lfospeed) * 2.0f;
			if (lfo > 1.0f) lfo = 2.0f - lfo;
			*l *= 1.0f - lfo;

			if (s->lfo_stereo) *r *= lfo;
			else               *r *= 1.0f - lfo;
		}
	} else
	{
		*l = (s->rand>>4)*DIV16;
		*r = (s->rand&15)*DIV16;
		if (s->lfospeed)
		{
			if (s->lfo_stereo)
			{
				if (fmodf(rp, 1.0f / s->lfospeed) > 0.5f) *l = 0.0f;
				else                                      *r = 0.0f;
			} else
			{
				if (fmodf(rp, 1.0f / s->lfospeed) > 0.5f)
					*l = *r = 0.0f;
			}
		}
	}
}
