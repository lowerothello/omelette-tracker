static void _macroOffset(jack_nframes_t fptr, uint16_t *spr, int m, Track *cv, Row *r)
{
	if (p->s->instrument->i[cv->r.inst] < p->s->instrument->c)
	{
		Instrument *iv = &p->s->instrument->v[p->s->instrument->i[cv->r.inst]];
		if (cv->r.note != NOTE_VOID) /* if playing a note */
		{
			if (r->note == NOTE_VOID) /* if not changing note, explicit ramping needed */
				ramp(cv, 0.0f, p->s->instrument->i[cv->r.inst]);
			cv->pitchedpointer = (m*DIV256) * iv->trimlength * (float)samplerate / (float)iv->sample->rate;
		}
	}
}
static void _macroOffsetJitter(jack_nframes_t fptr, uint16_t *spr, int m, Track *cv, Row *r)
{
	if (p->s->instrument->i[cv->r.inst] < p->s->instrument->c)
	{
		Instrument *iv = &p->s->instrument->v[p->s->instrument->i[cv->r.inst]];
		if (cv->r.note != NOTE_VOID) /* if playing a note */
		{
			if (r->note == NOTE_VOID) /* if not changing note, explicit ramping needed */
				ramp(cv, 0.0f, p->s->instrument->i[cv->r.inst]);
			if (m>>4 == (m&0xf)) /* both nibbles are the same */
				cv->pitchedpointer = (((m&0xf) + (rand()&0xf))*DIV256) * iv->trimlength * (float)samplerate / (float)iv->sample->rate;
			else
			{
				int min = MIN(m>>4, m&0xf);
				int max = MAX(m>>4, m&0xf);
				cv->pitchedpointer = (((min + rand()%(max - min +1))<<4)*DIV256) * iv->trimlength * (float)samplerate / (float)iv->sample->rate;
			}
		}
	}
}



#define MACRO_OFFSET                'O'
#define MACRO_REVERSE_OFFSET        'o'
#define MACRO_OFFSET_JITTER         'U'
#define MACRO_REVERSE_OFFSET_JITTER 'u'

void macroOffsetPostTrig(jack_nframes_t fptr, uint16_t *spr, Track *cv, Row *r)
{
	FOR_ROW_MACROS(i, cv)
		switch (r->macro[i].c)
		{
			case MACRO_OFFSET:                cv->reverse = 0;                    _macroOffset(fptr, spr, r->macro[i].v, cv, r); break;
			case MACRO_REVERSE_OFFSET:        cv->reverse = 1; if (r->macro[i].v) _macroOffset(fptr, spr, r->macro[i].v, cv, r); break;
			case MACRO_OFFSET_JITTER:         cv->reverse = 0;                    _macroOffsetJitter(fptr, spr, r->macro[i].v, cv, r); break;
			case MACRO_REVERSE_OFFSET_JITTER: cv->reverse = 1; if (r->macro[i].v) _macroOffsetJitter(fptr, spr, r->macro[i].v, cv, r); break;
		}
}
