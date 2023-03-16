typedef struct MacroRowState
{
	uint16_t cutsamples;   /* samples into the row to cut, 0 for no cut */
	uint16_t delaysamples; /* samples into the row to delay, 0 for no delay */
	uint8_t  delaynote;
} MacroRowState;

#define MACRO_ROW_CUT   'C'
#define MACRO_ROW_DELAY 'D'

void macroRowClear(Track *cv, void *state)
{
	MacroRowState *ms = state;
	ms->delaysamples = 0;
	ms->cutsamples = 0;
}

void macroRowPreTrig(uint32_t fptr, uint16_t *spr, Track *cv, Row *r, void *state)
{
	MacroRowState *ms = state;
	ms->cutsamples = 0;
	ms->delaysamples = 0;

	FOR_ROW_MACROS(i, cv)
		switch (r->macro[i].c)
		{
			case MACRO_ROW_CUT:
				if (!r->macro[i].v)
				{ /* cut now */
					ramp(cv, 0.0f, p->s->instrument->i[cv->r.inst]); /* TODO: proper rowprogress */
					triggerNote(fptr, cv, cv->r.note, NOTE_OFF, cv->r.inst);
					ms->cutsamples = 0;
					r->note = NOTE_VOID;
				} else /* cut later */
					ms->cutsamples = *spr * r->macro[i].v*DIV256;
				break;
			case MACRO_ROW_DELAY:
				if (r->macro[i].v)
				{
					ms->delaysamples = *spr * r->macro[i].v*DIV256;
					ms->delaynote = r->note;
					cv->r.inst = r->inst;
					r->note = NOTE_VOID;
				}
				break;
		}
}

/* returns the new note, or NOTE_VOID for no change */
uint8_t macroRowSampleRow(uint32_t fptr, uint16_t count, uint16_t *spr, uint16_t sprp, Track *cv, void *state)
{
	MacroRowState *ms = state;

	sprp += count;
	uint8_t ret = NOTE_VOID;

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
