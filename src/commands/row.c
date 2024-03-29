typedef struct CommandRowState
{
	uint16_t cutsamples;   /* samples into the row to cut, 0 for no cut */
	uint16_t delaysamples; /* samples into the row to delay, 0 for no delay */
	float    delaynote;
} CommandRowState;

#define COMMAND_ROW_CUT   'C'
#define COMMAND_ROW_DELAY 'D'

void commandRowClear(Track *cv, void *state)
{
	CommandRowState *ms = state;
	ms->delaysamples = 0;
	ms->cutsamples = 0;
}

void commandRowPreTrig(uint32_t fptr, uint16_t *spr, Track *cv, Row *r, void *state)
{
	CommandRowState *ms = state;
	ms->cutsamples = 0;
	ms->delaysamples = 0;

	Command *m;
	FOR_ROW_COMMANDS(i, cv)
	{
		m = &r->command[i];
		switch (m->c)
		{
			case COMMAND_ROW_CUT:
				if (!m->v)
				{ /* cut now */
					ramp(fptr, spr, 0, 0.0f, cv, p->s->inst->i[cv->r.inst]); /* TODO: proper rowprogress */
					triggerNote(fptr, cv, cv->r.note, NOTE_OFF, cv->r.inst);
					ms->cutsamples = 0;
					r->note = NOTE_VOID;
				} else /* cut later */
					ms->cutsamples = *spr * m->v*DIV256;
				break;
			case COMMAND_ROW_DELAY:
				if (m->v)
				{
					ms->delaysamples = *spr * m->v*DIV256;
					ms->delaynote = r->note;
					cv->r.inst = r->inst;
					r->note = NOTE_VOID;
				}
				break;
		}
	}
}

/* returns the new note, or NOTE_VOID for no change */
float commandRowSampleRow(uint32_t fptr, uint16_t count, uint16_t *spr, uint16_t sprp, Track *cv, void *state)
{
	CommandRowState *ms = state;
	float ret = NOTE_VOID;

	sprp += count;

	if (ms->delaysamples > ms->cutsamples && sprp > ms->delaysamples)
	{
		ms->cutsamples = 0;
		ms->delaysamples = 0;
		ret = ms->delaynote;
	} else if (ms->cutsamples && sprp > ms->cutsamples)
	{
		ms->cutsamples = 0;
		ret = NOTE_OFF;
	}

	return ret;
}
