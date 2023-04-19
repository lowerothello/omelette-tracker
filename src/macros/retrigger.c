typedef struct MacroRetrigState
{
	uint16_t rtrigsamples;               /* samples per retrigger */
	uint32_t rtrigpointer;               /* clock reference */
	uint32_t rtrigcurrentpointer;        /* pointer the current rtrig started at */
	uint32_t rtrigpitchedpointer;        /* pitchedpointer to ratchet back to */
	uint32_t rtrigcurrentpitchedpointer; /* pitchedpointer the current retrig started at */
	int8_t   rtrigblocksize;             /* number of rows block extends to */

	bool rtrig_rev;
} MacroRetrigState;

static void _macroTickRetrig(uint32_t fptr, uint16_t *spr, int m, Track *cv, Row *r, MacroRetrigState *ms)
{
	if (m)
	{
		if (ms->rtrigblocksize >= 0)
		{ /* starting a new chain */
			ms->rtrigpointer = ms->rtrigcurrentpointer = cv->pointer;
			ms->rtrigpitchedpointer = ms->rtrigcurrentpitchedpointer = cv->pitchedpointer;
		}
		ms->rtrigblocksize = -1;
		ms->rtrigsamples = *spr*DIV256 * m;
	}
}
static void _macroBlockRetrig(uint32_t fptr, uint16_t *spr, int m, Track *cv, Row *r, MacroRetrigState *ms)
{
	ms->rtrigpointer = ms->rtrigcurrentpointer = cv->pointer;
	ms->rtrigpitchedpointer = ms->rtrigcurrentpitchedpointer = cv->pitchedpointer;
	ms->rtrigblocksize = m>>4;
	if (m&15) ms->rtrigsamples = *spr / (m&15);
	else      ms->rtrigsamples = *spr * (ms->rtrigblocksize+1);
}



#define MACRO_TICK_RETRIG          'Q'
#define MACRO_REVERSE_TICK_RETRIG  'q'
#define MACRO_BLOCK_RETRIG         'R'
#define MACRO_REVERSE_BLOCK_RETRIG 'r'

void macroRetrigClear(Track *cv, void *state)
{
	MacroRetrigState *ms = state;
	ms->rtrigsamples = 0;
	ms->rtrig_rev = 0;
}

void macroRetrigPostTrig(uint32_t fptr, uint16_t *spr, Track *cv, Row *r, void *state)
{
	MacroRetrigState *ms = state;

	if (ms->rtrigsamples)
	{
		if (ms->rtrigblocksize > 0 || ms->rtrigblocksize == -1)
			ms->rtrigblocksize--;
		else
		{
			ms->rtrig_rev = 0;
			ms->rtrigsamples = 0;
		}
	}

	FOR_ROW_MACROS(i, cv)
	{
		switch (r->macro[i].c)
		{
			case MACRO_TICK_RETRIG:          ms->rtrig_rev = 0; _macroTickRetrig (fptr, spr, r->macro[i].v, cv, r, state); goto macroRetrigEnd;
			case MACRO_REVERSE_TICK_RETRIG:  ms->rtrig_rev = 1; _macroTickRetrig (fptr, spr, r->macro[i].v, cv, r, state); goto macroRetrigEnd;
			case MACRO_BLOCK_RETRIG:         ms->rtrig_rev = 0; _macroBlockRetrig(fptr, spr, r->macro[i].v, cv, r, state); goto macroRetrigEnd;
			case MACRO_REVERSE_BLOCK_RETRIG: ms->rtrig_rev = 1; _macroBlockRetrig(fptr, spr, r->macro[i].v, cv, r, state); goto macroRetrigEnd;
		}
	}

macroRetrigEnd:
	if (ms->rtrigsamples && ms->rtrigblocksize == -2)
	{ /* clean up if the last row had an altRxx and this row doesn't */
		ms->rtrig_rev = 0;
		ms->rtrigsamples = 0;
	}
}

void macroRetrigTriggerNote(uint32_t fptr, Track *cv, float oldnote, float note, short inst, void *state)
{
	MacroRetrigState *ms = state;

	/* must stop retriggers cos pointers are no longer guaranteed to be valid */
	ms->rtrigblocksize = 0;
	ms->rtrig_rev = 0;
	ms->rtrigsamples = 0;
}

/* TODO: should the persistent pointer be the informant? */
void macroRetrigVolatile(uint32_t fptr, uint16_t count, uint16_t *spr, uint16_t sprp, Track *cv, float *finetune, uint32_t *pointer, uint32_t *pitchedpointer, void *state)
{
	MacroRetrigState *ms = state;
	Inst *iv;
	const InstAPI *api;

	sprp += count;

	if (ms->rtrigsamples)
	{
		if (!((*pointer - ms->rtrigpointer) % ms->rtrigsamples) && *pointer > ms->rtrigpointer)
		{ /* first sample of any retrigger but the first */
			if (instSafe(p->s->inst, cv->r.inst))
			{
				iv = &p->s->inst->v[p->s->inst->i[cv->r.inst]];
				if ((api = instGetAPI(iv->type)))
					api->triggernote(fptr, iv, cv, cv->r.note, cv->r.note, cv->r.inst);
			}
			ms->rtrigcurrentpitchedpointer = *pitchedpointer;
			ms->rtrigcurrentpointer = *pointer;
		}
		if (ms->rtrig_rev)
		{
			if (ms->rtrigpointer > *pointer - ms->rtrigcurrentpointer)
				*pointer = ms->rtrigpointer - (*pointer - ms->rtrigcurrentpointer);
			else *pointer = 0;
			if (ms->rtrigpitchedpointer > *pitchedpointer - ms->rtrigcurrentpitchedpointer)
				*pitchedpointer = ms->rtrigcurrentpitchedpointer - (*pitchedpointer - ms->rtrigcurrentpitchedpointer);
			else *pitchedpointer = 0;
		} else
		{
			*pointer = ms->rtrigpointer + (*pointer - ms->rtrigcurrentpointer);
			*pitchedpointer = ms->rtrigpitchedpointer + (*pitchedpointer - ms->rtrigcurrentpitchedpointer);
		}
	}
}
