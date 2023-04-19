typedef struct MacroPitchState
{
	float    portamentofinetune;       /* portamento fine tune            */
	float    targetportamentofinetune; /* .portamentofinetune destination */
	float    startportamentofinetune;  /* .portamentofinetune start       */
	uint32_t portamentosamples;        /* portamento length               */
	uint32_t portamentosamplepointer;  /* portamento progress             */

	float microtonalfinetune;

	uint8_t  vibrato;              /* vibrato depth, 0-f */
	uint32_t vibratosamples;       /* samples per full phase walk */
	uint32_t vibratosamplepointer; /* distance through cv->vibratosamples */
} MacroPitchState;

#define MACRO_PORTAMENTO   'P'
#define MACRO_PITCH_OFFSET 'p'
#define MACRO_VIBRATO      'V'

void macroPitchPreTrig(uint32_t fptr, uint16_t *spr, Track *cv, Row *r, void *state)
{
	MacroPitchState *ms = state;

	bool vibrato = 0;
	FOR_ROW_MACROS(i, cv)
		switch (r->macro[i].c)
		{
			case MACRO_PORTAMENTO:
				if (ms->portamentosamplepointer > ms->portamentosamples)
				{
					ms->portamentosamples = (*spr * r->macro[i].v)/16;
					ms->portamentosamplepointer = 0;
					ms->startportamentofinetune = ms->portamentofinetune;
					ms->targetportamentofinetune = (r->note - (cv->r.note + ms->portamentofinetune));
				}
				r->note = NOTE_VOID;
				break;
			case MACRO_PITCH_OFFSET:
				ms->microtonalfinetune = r->macro[i].v*DIV255;
				break;
			case MACRO_VIBRATO:
				ms->vibrato = r->macro[i].v&0xf;
				if (!ms->vibratosamples) /* reset the phase if starting */
					ms->vibratosamplepointer = 0;
				ms->vibratosamples = *spr / (((r->macro[i].v>>4) + 1) / 16.0); /* use floats for slower lfo speeds */
				ms->vibratosamplepointer = MIN(ms->vibratosamplepointer, ms->vibratosamples - 1);
				vibrato = 1;
				break;
		}

	if (!vibrato)
		ms->vibratosamples = 0;
}

void macroPitchTriggerNote(uint32_t fptr, Track *cv, float oldnote, float note, short inst, void *state)
{
	MacroPitchState *ms = state;

	ms->portamentosamples = 0; ms->portamentosamplepointer = 1;
	ms->startportamentofinetune = ms->targetportamentofinetune = ms->portamentofinetune = 0.0f;
	ms->microtonalfinetune = 0.0f;
	ms->vibrato = 0;
}

/* should affect cv->pointer, cv->pitchedpointer, and cv->finetune */
void macroPitchPersistent(uint32_t fptr, uint16_t count, uint16_t *spr, uint16_t sprp, Track *cv, void *state)
{
	// MacroPitchState *ms = state;
	//
	// sprp += count;
	// int delta; /* pretty sure this needs to be signed */
	// float multiplier;
	//
	// if (ms->portamentosamplepointer != -1 && ms->portamentosamplepointer+count > ms->portamentosamples)
	// {
	// 	ms->portamentofinetune = ms->targetportamentofinetune;
	// 	ms->portamentosamplepointer = -1;
	// 	cv->r.note += ms->targetportamentofinetune;
	// } else
	// {
	// 	/* sample portamentofinetune */
	// 	ms->portamentofinetune = ms->startportamentofinetune +
	// 		(ms->targetportamentofinetune - ms->startportamentofinetune) * (float)ms->portamentosamplepointer/(float)ms->portamentosamples;
	//
	// 	multiplier = powf(M_12_ROOT_2, ms->portamentofinetune);
	// 	delta = (int)((ms->portamentosamplepointer+count)*multiplier) - (int)(ms->portamentosamplepointer*multiplier);
	//
	// 	delta -= count; /* anticipate reapplication */
	//
	// 	// if (cv->reverse)
	// 	// {
	// 	// 	if (cv->pitchedpointer > delta) cv->pitchedpointer -= delta;
	// 	// 	else                            cv->pitchedpointer = 0;
	// 	// } else cv->pitchedpointer += delta;
	//
	// 	ms->portamentosamplepointer += count;
	// }
}

void macroPitchVolatile(uint32_t fptr, uint16_t count, uint16_t *spr, uint16_t sprp, Track *cv, float *note, void *state)
{
	MacroPitchState *ms = state;

	sprp += count;

	*note += ms->microtonalfinetune;
	if (ms->vibratosamples)
	{
		/* sample the vibrato oscillator */
		*note += triosc((float)ms->vibratosamplepointer / (float)ms->vibratosamples) * ms->vibrato*DIV16;

		ms->vibratosamplepointer += count;
		/* re-read the macro once phase is about to overflow */
		if (ms->vibratosamplepointer > ms->vibratosamples)
		{
			ms->vibratosamplepointer = 0;
			if (!ifMacro(cv, &cv->r, 'V'))
				ms->vibratosamples = 0;
		}
	}
}
