bool ifMacro(Track *cv, Row *r, char m)
{
	for (int i = 0; i <= cv->variant->macroc; i++)
		if (r->macro[i].c == m)
			return 1;
	return 0;
}

/* ifMacro(), but run .triggercallback */
void ifMacroCallback(uint32_t fptr, uint16_t *spr, Track *cv, Row *r, char m, void (*triggercallback)(uint32_t, uint16_t*, int, Track*, Row*))
{
	for (int i = 0; i <= cv->variant->macroc; i++)
		if (r->macro[i].c == m && MACRO_SET(r->macro[i].c))
		{
			triggercallback(fptr, spr, r->macro[i].v, cv, r);
			return;
		}
	triggercallback(fptr, spr, -1, cv, r);
}

/* if the row needs to be ramped in based on the macros present */
bool ifMacroRamp(Track *cv, Row *r)
{
	for (int i = 0; i <= cv->variant->macroc; i++)
		if (MACRO_RAMP(r->macro[i].c)) return 1;

	return 0;
}

void macroCallbackClear(Track *cv)
{
	for (size_t i = 0; i < MACRO_CALLBACK_MAX; i++)
		if (global_macro_callbacks[i].clear)
			global_macro_callbacks[i].clear(cv, cv->macrostate[i]);
}
void macroCallbackPreTrig(uint32_t fptr, uint16_t *spr, Track *cv, Row *r)
{
	for (size_t i = 0; i < MACRO_CALLBACK_MAX; i++)
		if (global_macro_callbacks[i].pretrig)
			global_macro_callbacks[i].pretrig(fptr, spr, cv, r, cv->macrostate[i]);
}
void macroCallbackPostTrig(uint32_t fptr, uint16_t *spr, Track *cv, Row *r)
{
	for (size_t i = 0; i < MACRO_CALLBACK_MAX; i++)
		if (global_macro_callbacks[i].posttrig)
			global_macro_callbacks[i].posttrig(fptr, spr, cv, r, cv->macrostate[i]);
}
void macroCallbackTriggerNote(uint32_t fptr, Track *cv, uint8_t oldnote, uint8_t note, short inst)
{
	for (size_t i = 0; i < MACRO_CALLBACK_MAX; i++)
		if (global_macro_callbacks[i].triggernote)
			global_macro_callbacks[i].triggernote(fptr, cv, oldnote, note, inst, cv->macrostate[i]);
}
uint8_t macroCallbackSampleRow(uint32_t fptr, uint16_t count, uint16_t *spr, uint16_t sprp, Track *cv)
{
	uint8_t tryret;
	uint8_t ret = NOTE_VOID;
	for (size_t i = 0; i < MACRO_CALLBACK_MAX; i++)
		if (global_macro_callbacks[i].samplerow)
		{
			tryret = global_macro_callbacks[i].samplerow(fptr, count, spr, sprp, cv, cv->macrostate[i]);
			if (tryret != NOTE_VOID)
				ret = tryret;
		}
	return ret;
}
void macroCallbackPersistent(uint32_t fptr, uint16_t count, uint16_t *spr, uint16_t sprp, Track *cv)
{
	for (size_t i = 0; i < MACRO_CALLBACK_MAX; i++)
		if (global_macro_callbacks[i].persistenttune)
			global_macro_callbacks[i].persistenttune(fptr, count, spr, sprp, cv, cv->macrostate[i]);
}
void macroCallbackVolatile(uint32_t fptr, uint16_t count, uint16_t *spr, uint16_t sprp, Track *cv, float *finetune, uint32_t *pointer, uint32_t *pitchedpointer)
{
	for (size_t i = 0; i < MACRO_CALLBACK_MAX; i++)
		if (global_macro_callbacks[i].volatiletune)
			global_macro_callbacks[i].volatiletune(fptr, count, spr, sprp, cv, finetune, pointer, pitchedpointer, cv->macrostate[i]);
}
void macroCallbackPostSampler(uint32_t fptr, Track *cv, float rp, float *lf, float *rf)
{
	for (size_t i = 0; i < MACRO_CALLBACK_MAX; i++)
		if (global_macro_callbacks[i].postsampler)
			global_macro_callbacks[i].postsampler(fptr, cv, rp, lf, rf, cv->macrostate[i]);
}

static int swapCase(int x)
{
	if (islower(x)) return toupper(x);
	else            return tolower(x);
}

bool changeMacro(int input, char *dest)
{
	/* use the current value if input is '\0' */
	if (!input) input = *dest; /* dest is pre-swapped */
	else        input = swapCase(input);

	if (MACRO_SET(input))
		*dest = input;

	return 0;
}

void addMacroBinds(const char *prettyname, unsigned int state, void (*callback)(void*))
{
	addTooltipPrettyPrint(prettyname, state, "macro");
	for (size_t i = 0; i < MACRO_MAX; i++)
		if (MACRO_SET(i))
			addTooltipBind(MACRO_PRETTYNAME(i), state, swapCase(i), 0, callback, (void*)i);
}


void macroStateReset(MacroState *s)
{
	s->base = -1;
	s->rand = -1;
	s->target = -1;
}
void macroStateSet(MacroState *s, Macro macro)
{
	uint8_t hold;
	short oversample;
	if (macro.t) /* special tokens are present */
	{
		switch (macro.t>>4)
		{
			case 1: /* ~ */
				s->lfo_stereo = 0;
				s->lfospeed = macro.v&15;
				break;
			case 2: /* + */
				hold = macro.v&15;
				if (!hold) hold = 1;
				hold *= 0x10;

				oversample = (short)s->rand + (short)hold;
				oversample = MIN(oversample, 0xff);
				oversample = MAX(oversample, 0x00);
				s->rand = oversample;
				break;
			case 3: /* - */
				hold = macro.v&15;
				if (!hold) hold = 1;
				hold *= 0x10;

				oversample = (short)s->rand - (short)hold;
				oversample = MIN(oversample, 0xff);
				oversample = MAX(oversample, 0x00);
				s->rand = oversample;
				break;
			case 4: /* % */
				hold = macro.v&15;
				if (!hold) hold = 1;
				hold++;

				if (MACRO_STEREO(macro.c))
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

		switch (macro.t&15)
		{
			case 1: /* ~ */
				s->lfo_stereo = 1;
				s->lfospeed = macro.v>>4;
				break;
			case 2: /* + */
				hold = macro.v>>4;
				if (!hold) hold = 1;

				oversample = (short)s->rand + (short)hold;
				oversample = MIN(oversample, 0xff);
				oversample = MAX(oversample, 0x00);
				s->rand = oversample;
				break;
			case 3: /* - */
				hold = macro.v>>4;
				if (!hold) hold = 1;

				oversample = (short)s->rand - (short)hold;
				oversample = MIN(oversample, 0xff);
				oversample = MAX(oversample, 0x00);
				s->rand = oversample;
				break;
			case 4: /* % */
				hold = macro.v>>4;
				if (!hold) hold = 1;
				hold++;

				if (MACRO_STEREO(macro.c))
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
		s->base = macro.v;
		s->rand = macro.v;
	}
}
void macroStateSmooth(MacroState *s, Macro macro)
{
	uint8_t hold;
	short oversample;
	if (macro.t) /* special tokens are present */
	{
		switch (macro.t>>4)
		{
			case 1: /* ~ */
				s->lfo_stereo  = 0;
				s->target_rand = 1;
				s->target = s->rand; /* mark as smooth */
				s->lfospeed = macro.v&15;
				break;
			case 2: /* + */
				s->target_rand = 1;
				hold = macro.v&15;
				if (!hold) hold = 1;
				hold *= 0x10;

				oversample = (short)s->rand + (short)hold;
				oversample = MIN(oversample, 0xff);
				oversample = MAX(oversample, 0x00);
				s->target = oversample;
				break;
			case 3: /* - */
				s->target_rand = 1;
				hold = macro.v&15;
				if (!hold) hold = 1;
				hold *= 0x10;

				oversample = (short)s->rand - (short)hold;
				oversample = MIN(oversample, 0xff);
				oversample = MAX(oversample, 0x00);
				s->target = oversample;
				break;
			case 4: /* % */
				s->target_rand = 1;
				hold = macro.v&15;
				if (!hold) hold = 1;
				hold++;

				if (MACRO_STEREO(macro.c))
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

		switch (macro.t&15)
		{
			case 1: /* ~ */
				s->lfo_stereo  = 1;
				s->target_rand = 1;
				s->target = s->rand; /* mark as smooth */
				s->lfospeed = macro.v>>4;
				break;
			case 2: /* + */
				s->target_rand = 1;
				hold = macro.v>>4;
				if (!hold) hold = 1;

				oversample = (short)s->rand + (short)hold;
				oversample = MIN(oversample, 0xff);
				oversample = MAX(oversample, 0x00);
				s->target = oversample;
				break;
			case 3: /* - */
				s->target_rand = 1;
				hold = macro.v>>4;
				if (!hold) hold = 1;

				oversample = (short)s->rand - (short)hold;
				oversample = MIN(oversample, 0xff);
				oversample = MAX(oversample, 0x00);
				s->target = oversample;
				break;
			case 4: /* % */
				s->target_rand = 1;
				hold = macro.v>>4;
				if (!hold) hold = 1;
				hold++;

				if (MACRO_STEREO(macro.c))
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
		s->target = macro.v;
}
void macroStateApply(MacroState *s)
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
float macroStateGetMono(MacroState *s, float rp)
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
void macroStateGetStereo(MacroState *s, float rp, float *l, float *r)
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
