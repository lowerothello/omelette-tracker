typedef struct CommandPitchState
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
} CommandPitchState;

#define COMMAND_PORTAMENTO   'P'
#define COMMAND_PITCH_OFFSET 'p'
#define COMMAND_VIBRATO      'V'

void commandPitchPreTrig(uint32_t fptr, uint16_t *spr, Track *cv, Row *r, void *state)
{
	CommandPitchState *ms = state;

	bool vibrato = 0;
	Command *m;
	FOR_ROW_COMMANDS(i, cv)
	{
		m = &r->command[i];
		switch (m->c)
		{
			case COMMAND_PORTAMENTO:
				if (ms->portamentosamplepointer > ms->portamentosamples)
				{
					ms->portamentosamples = (*spr * m->v)/16;
					ms->portamentosamplepointer = 0;
					ms->startportamentofinetune = ms->portamentofinetune;
					ms->targetportamentofinetune = (r->note - (cv->r.note + ms->portamentofinetune));
				}
				r->note = NOTE_VOID;
				break;
			case COMMAND_PITCH_OFFSET:
				ms->microtonalfinetune = m->v*DIV255;
				break;
			case COMMAND_VIBRATO:
				ms->vibrato = m->v&0xf;
				if (!ms->vibratosamples) /* reset the phase if starting */
					ms->vibratosamplepointer = 0;
				ms->vibratosamples = *spr / (((m->v>>4) + 1) / 16.0); /* use floats for slower lfo speeds */
				ms->vibratosamplepointer = MIN(ms->vibratosamplepointer, ms->vibratosamples - 1);
				vibrato = 1;
				break;
		}
	}

	if (!vibrato)
		ms->vibratosamples = 0;
}

void commandPitchTriggerNote(uint32_t fptr, Track *cv, float oldnote, float note, short inst, void *state)
{
	CommandPitchState *ms = state;

	ms->portamentosamples = 0; ms->portamentosamplepointer = 1;
	ms->startportamentofinetune = ms->targetportamentofinetune = ms->portamentofinetune = 0.0f;
	ms->microtonalfinetune = 0.0f;
	ms->vibrato = 0;
}

/* should affect cv->pointer, cv->pitchedpointer, and cv->finetune */
void commandPitchPersistent(uint32_t fptr, uint16_t count, uint16_t *spr, uint16_t sprp, Track *cv, void *state)
{
	// CommandPitchState *ms = state;
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

void commandPitchVolatile(uint32_t fptr, uint16_t count, uint16_t *spr, uint16_t sprp, Track *cv, float *note, void *state)
{
	CommandPitchState *ms = state;

	sprp += count;

	*note += ms->microtonalfinetune;
	if (ms->vibratosamples)
	{
		/* sample the vibrato oscillator */
		*note += triosc((float)ms->vibratosamplepointer / (float)ms->vibratosamples) * ms->vibrato*DIV16;

		ms->vibratosamplepointer += count;
		/* re-read the command once phase is about to overflow */
		if (ms->vibratosamplepointer > ms->vibratosamples)
		{
			ms->vibratosamplepointer = 0;
			if (!ifCommand(cv, &cv->r, 'V'))
				ms->vibratosamples = 0;
		}
	}
}
