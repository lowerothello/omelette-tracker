#define MACRO_ROW_CUT   'C'
#define MACRO_ROW_DELAY 'D'

void macroRowPreTrig(jack_nframes_t fptr, uint16_t *spr, Track *cv, Row *r)
{
	FOR_ROW_MACROS(i, cv)
		switch (r->macro[i].c)
		{
			case MACRO_ROW_CUT:
				if (!r->macro[i].v)
				{ /* cut now */
					ramp(cv, 0.0f, p->s->instrument->i[cv->r.inst]); /* TODO: proper rowprogress */
					triggerNote(fptr, cv, cv->r.note, NOTE_OFF, cv->r.inst);
					cv->cutsamples = 0;
					r->note = NOTE_VOID;
				} else /* cut later */
					cv->cutsamples = *spr * r->macro[i].v*DIV256;
				break;
			case MACRO_ROW_DELAY:
				cv->delaysamples = *spr * r->macro[i].v*DIV256;
				cv->delaynote = r->note;
				cv->r.inst = r->inst;
				r->note = NOTE_VOID;
				break;
		}
}

/* returns the new note, or NOTE_VOID for no change */
uint8_t macroRowSampleRow(jack_nframes_t fptr, uint16_t count, uint16_t *spr, uint16_t sprp, Track *cv)
{
	sprp += count;
	uint8_t ret = NOTE_VOID;

	if (cv->delaysamples > cv->cutsamples && sprp > cv->delaysamples)
	{
		cv->cutsamples = 0;
		cv->delaysamples = 0;
		ret = cv->delaynote;
	} else if (cv->cutsamples && sprp > cv->cutsamples)
	{
		cv->cutsamples = 0;
		ret = NOTE_OFF;
	}

	return ret;
}
