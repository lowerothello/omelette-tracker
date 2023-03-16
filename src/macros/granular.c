#define MACRO_PITCH_SHIFT          'H'
#define MACRO_SMOOTH_PITCH_SHIFT   'h'
#define MACRO_PITCH_WIDTH          'W'
#define MACRO_SMOOTH_PITCH_WIDTH   'w'
#define MACRO_CYCLE_LENGTH_HI_BYTE 'L'
#define MACRO_CYCLE_LENGTH_LO_BYTE 'l'
#define MACRO_SAMPLERATE           'X'
#define MACRO_SMOOTH_SAMPLERATE    'x'
#define MACRO_ATT_DEC              'E'
#define MACRO_SUS_REL              'e'

void macroGranularPostTrig(uint32_t fptr, uint16_t *spr, Track *cv, Row *r, void *state)
{
	macroStateApply(&cv->pitchshift);
	macroStateApply(&cv->pitchwidth);
	macroStateApply(&cv->samplerate);

	FOR_ROW_MACROS(i, cv)
		switch (r->macro[i].c)
		{
			case MACRO_PITCH_SHIFT:        macroStateSet   (&cv->pitchshift, r->macro[i]); break;
			case MACRO_SMOOTH_PITCH_SHIFT: macroStateSmooth(&cv->pitchshift, r->macro[i]); break;

			case MACRO_PITCH_WIDTH:        macroStateSet   (&cv->pitchwidth, r->macro[i]); break;
			case MACRO_SMOOTH_PITCH_WIDTH: macroStateSmooth(&cv->pitchwidth, r->macro[i]); break;

			case MACRO_CYCLE_LENGTH_HI_BYTE:
				if (p->s->instrument->i[cv->r.inst] < p->s->instrument->c)
				{
					Instrument *iv = &p->s->instrument->v[p->s->instrument->i[cv->r.inst]];
					if (cv->localcyclelength == -1) cv->localcyclelength = iv->granular.cyclelength;
					cv->localcyclelength = (((uint16_t)cv->localcyclelength<<8)>>8) + (r->macro[i].v<<8);
				}
				break;
			case MACRO_CYCLE_LENGTH_LO_BYTE:
				if (p->s->instrument->i[cv->r.inst] < p->s->instrument->c)
				{
					Instrument *iv = &p->s->instrument->v[p->s->instrument->i[cv->r.inst]];
					if (cv->localcyclelength == -1) cv->localcyclelength = iv->granular.cyclelength;
					cv->localcyclelength = (((uint16_t)cv->localcyclelength>>8)<<8)+r->macro[i].v;
				}
				break;

			case MACRO_SAMPLERATE:        macroStateSet   (&cv->samplerate, r->macro[i]); break;
			case MACRO_SMOOTH_SAMPLERATE: macroStateSmooth(&cv->samplerate, r->macro[i]); break;

			case MACRO_ATT_DEC: cv->localenvelope = r->macro[i].v; break;
			case MACRO_SUS_REL: cv->localsustain = r->macro[i].v; break;
		}
}

void macroGranularTriggerNote(uint32_t fptr, Track *cv, uint8_t oldnote, uint8_t note, short inst, void *state)
{
	/* local controls */
	macroStateReset(&cv->pitchshift);
	macroStateReset(&cv->pitchwidth);
	macroStateReset(&cv->samplerate);
	cv->localenvelope = -1;
	cv->localsustain = -1;
	cv->localcyclelength = -1;
}
