static bool _macroTickRetrig(jack_nframes_t fptr, uint16_t *spr, int m, Track *cv, Row *r)
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
		return 1;
	} return 0;
}
bool macroBlockRetrig(jack_nframes_t fptr, uint16_t *spr, int m, Track *cv, Row *r)
{
	if (m < 0) return 0;

	cv->rtrigpointer = cv->rtrigcurrentpointer = cv->pointer;
	cv->rtrigpitchedpointer = cv->rtrigcurrentpitchedpointer = cv->pitchedpointer;
	cv->rtrigblocksize = m>>4;
	if (m&0xf) cv->rtrigsamples = *spr / (m&0xf);
	else       cv->rtrigsamples = *spr * (cv->rtrigblocksize+1);
	return 1;
}
bool macroReverseBlockRetrig(jack_nframes_t fptr, uint16_t *spr, int m, Track *cv, Row *r)
{
	cv->rtrig_rev = 1;
	return macroBlockRetrig(fptr, spr, m, cv, r);
}
bool macroTickRetrig(jack_nframes_t fptr, uint16_t *spr, int m, Track *cv, Row *r)
{
	cv->rtrig_rev = 0;
	return _macroTickRetrig(fptr, spr, m, cv, r);
}
bool macroReverseTickRetrig(jack_nframes_t fptr, uint16_t *spr, int m, Track *cv, Row *r)
{
	cv->rtrig_rev = 1;
	return _macroTickRetrig(fptr, spr, m, cv, r);
}
