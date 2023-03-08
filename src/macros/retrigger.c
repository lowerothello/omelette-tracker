static void _macroTickRetrig(jack_nframes_t fptr, uint16_t *spr, int m, Track *cv, Row *r)
{
	if (m > 0)
	{
		if (cv->rtrigblocksize >= 0)
		{ /* starting a new chain */
			cv->rtrigpointer = cv->rtrigcurrentpointer = cv->pointer;
			cv->rtrigpitchedpointer = cv->rtrigcurrentpitchedpointer = cv->pitchedpointer;
		}
		cv->rtrigblocksize = -1;
		cv->rtrigsamples = *spr*DIV256 * m;
	}
}
static void _macroBlockRetrig(jack_nframes_t fptr, uint16_t *spr, int m, Track *cv, Row *r)
{
	cv->rtrigpointer = cv->rtrigcurrentpointer = cv->pointer;
	cv->rtrigpitchedpointer = cv->rtrigcurrentpitchedpointer = cv->pitchedpointer;
	cv->rtrigblocksize = m>>4;
	if (m&0xf) cv->rtrigsamples = *spr / (m&0xf);
	else       cv->rtrigsamples = *spr * (cv->rtrigblocksize+1);
}



#define MACRO_TICK_RETRIG          'Q'
#define MACRO_REVERSE_TICK_RETRIG  'q'
#define MACRO_BLOCK_RETRIG         'R'
#define MACRO_REVERSE_BLOCK_RETRIG 'r'

void macroRetrigPostTrig(jack_nframes_t fptr, uint16_t *spr, Track *cv, Row *r)
{
	if (cv->rtrigsamples)
	{
		if (cv->rtrigblocksize > 0 || cv->rtrigblocksize == -1)
			cv->rtrigblocksize--;
		else
		{
			cv->rtrig_rev = 0;
			cv->rtrigsamples = 0;
		}
	}

	FOR_ROW_MACROS(i, cv)
	{
		switch (r->macro[i].c)
		{
			case MACRO_TICK_RETRIG:          cv->rtrig_rev = 0; _macroTickRetrig (fptr, spr, r->macro[i].v, cv, r); goto macroRetrigEnd;
			case MACRO_REVERSE_TICK_RETRIG:  cv->rtrig_rev = 1; _macroTickRetrig (fptr, spr, r->macro[i].v, cv, r); goto macroRetrigEnd;
			case MACRO_BLOCK_RETRIG:         cv->rtrig_rev = 0; _macroBlockRetrig(fptr, spr, r->macro[i].v, cv, r); goto macroRetrigEnd;
			case MACRO_REVERSE_BLOCK_RETRIG: cv->rtrig_rev = 1; _macroBlockRetrig(fptr, spr, r->macro[i].v, cv, r); goto macroRetrigEnd;
		}
	}

macroRetrigEnd:
	if (cv->rtrigsamples && cv->rtrigblocksize == -2)
	{ /* clean up if the last row had an altRxx and this row doesn't */
		cv->rtrig_rev = 0;
		cv->rtrigsamples = 0;
	}
}

void macroRetrigTriggerNote(jack_nframes_t fptr, Track *cv, uint8_t oldnote, uint8_t note, short inst)
{
	/* must stop retriggers cos pointers are no longer guaranteed to be valid */
	cv->rtrigblocksize = 0;
	cv->rtrig_rev = 0;
	cv->rtrigsamples = 0;
}

/* TODO: should the persistent pointer be the informant? */
void macroRetrigVolatile(jack_nframes_t fptr, uint16_t count, uint16_t *spr, uint16_t sprp, Track *cv, float *finetune, uint32_t *pointer, uint32_t *pitchedpointer)
{
	sprp += count;

	if (cv->rtrigsamples)
	{
		uint32_t rtrigoffset = (*pointer - cv->rtrigpointer) % cv->rtrigsamples;
		if (!rtrigoffset)
		{ /* first sample of any retrigger */
			if (*pointer > cv->rtrigpointer) /* first sample of any retrigger but the first */
			{
				triggerMidi(fptr, cv, cv->r.note, cv->r.note, cv->r.inst);
				cv->rtrigcurrentpitchedpointer = *pitchedpointer;
				cv->rtrigcurrentpointer = *pointer;
			}
		}
		if (cv->rtrig_rev)
		{
			if (cv->rtrigpointer > *pointer - cv->rtrigcurrentpointer)
				*pointer = cv->rtrigpointer - (*pointer - cv->rtrigcurrentpointer);
			else *pointer = 0;
			if (cv->rtrigpitchedpointer > *pitchedpointer - cv->rtrigcurrentpitchedpointer)
				*pitchedpointer = cv->rtrigcurrentpitchedpointer - (*pitchedpointer - cv->rtrigcurrentpitchedpointer);
			else *pitchedpointer = 0;
		} else
		{
			*pointer = cv->rtrigpointer + (*pointer - cv->rtrigcurrentpointer);
			*pitchedpointer = cv->rtrigpitchedpointer + (*pitchedpointer - cv->rtrigcurrentpitchedpointer);
		}
	}
}
