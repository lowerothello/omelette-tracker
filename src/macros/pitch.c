#define MACRO_PORTAMENTO   'P'
#define MACRO_PITCH_OFFSET 'p'
#define MACRO_VIBRATO      'V'

void macroPitchPreTrig(uint32_t fptr, uint16_t *spr, Track *cv, Row *r)
{
	bool vibrato = 0;
	FOR_ROW_MACROS(i, cv)
		switch (r->macro[i].c)
		{
			case MACRO_PORTAMENTO:
				if (cv->portamentosamplepointer > cv->portamentosamples)
				{
					cv->portamentosamples = (*spr * r->macro[i].v)/16;
					cv->portamentosamplepointer = 0;
					cv->startportamentofinetune = cv->portamentofinetune;
					cv->targetportamentofinetune = (r->note - (cv->r.note + cv->portamentofinetune));
				}
				r->note = NOTE_VOID;
				break;
			case MACRO_PITCH_OFFSET:
				cv->microtonalfinetune = r->macro[i].v*DIV255;
				break;
			case MACRO_VIBRATO:
				cv->vibrato = r->macro[i].v&0xf;
				if (!cv->vibratosamples) /* reset the phase if starting */
					cv->vibratosamplepointer = 0;
				cv->vibratosamples = *spr / (((r->macro[i].v>>4) + 1) / 16.0); /* use floats for slower lfo speeds */
				cv->vibratosamplepointer = MIN(cv->vibratosamplepointer, cv->vibratosamples - 1);
				vibrato = 1;
				break;
		}

	if (!vibrato)
		cv->vibratosamples = 0;
}

void macroPitchTriggerNote(uint32_t fptr, Track *cv, uint8_t oldnote, uint8_t note, short inst)
{
	cv->portamentosamples = 0; cv->portamentosamplepointer = 1;
	cv->startportamentofinetune = cv->targetportamentofinetune = cv->portamentofinetune = 0.0f;
	cv->microtonalfinetune = 0.0f;
	cv->vibrato = 0;
}

/* should affect cv->pointer, cv->pitchedpointer, and cv->finetune */
/* TODO: should be passed as arguments, cv shouldn't be provided */
void macroPitchPersistent(uint32_t fptr, uint16_t count, uint16_t *spr, uint16_t sprp, Track *cv)
{
	sprp += count;
	uint32_t delta;
	float multiplier;

	if (cv->portamentosamplepointer != -1 && cv->portamentosamplepointer+count > cv->portamentosamples)
	{
		cv->portamentofinetune = cv->targetportamentofinetune;
		cv->portamentosamplepointer = -1;
		cv->r.note += cv->targetportamentofinetune;
	} else
	{
		/* sample portamentofinetune */
		cv->portamentofinetune = cv->startportamentofinetune +
			(cv->targetportamentofinetune - cv->startportamentofinetune) * (float)cv->portamentosamplepointer/(float)cv->portamentosamples;

		multiplier = powf(M_12_ROOT_2, cv->portamentofinetune);
		delta = (int)((cv->portamentosamplepointer+count)*multiplier) - (int)(cv->portamentosamplepointer*multiplier);

		if (cv->reverse)
		{
			if (cv->pitchedpointer > delta) cv->pitchedpointer -= delta;
			else                            cv->pitchedpointer = 0;
		} else cv->pitchedpointer += delta;

		cv->portamentosamplepointer += count;
	}
}

void macroPitchVolatile(uint32_t fptr, uint16_t count, uint16_t *spr, uint16_t sprp, Track *cv, float *finetune, uint32_t *pointer, uint32_t *pitchedpointer)
{
	sprp += count;

	*finetune += cv->microtonalfinetune;
	if (cv->vibratosamples)
	{
		/* sample the vibrato oscillator */
		*finetune += triosc((float)cv->vibratosamplepointer / (float)cv->vibratosamples) * cv->vibrato*DIV16;

		cv->vibratosamplepointer += count;
		/* re-read the macro once phase is about to overflow */
		if (cv->vibratosamplepointer > cv->vibratosamples)
		{
			cv->vibratosamplepointer = 0;
			if (!ifMacro(cv, &cv->r, 'V'))
				cv->vibratosamples = 0;
		}
	}
}
