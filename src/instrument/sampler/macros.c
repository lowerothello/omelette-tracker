static void _macroOffset(uint32_t fptr, uint16_t *spr, int m, InstSamplerState *iss, Track *cv, Row *r)
{
	InstSamplerPlaybackState *ps = cv->inststate;
	if (cv->r.note != NOTE_VOID) /* if playing a note */
	{
		if (r->note == NOTE_VOID) /* if not changing note, explicit ramping needed */
			ramp(fptr, spr, 0, 0.0f, cv, p->s->inst->i[cv->r.inst]);
		ps->pitchedpointer = (m*DIV256) * (*iss->sample)[ps->sampleslot]->trimlength * (float)samplerate / (float)(*iss->sample)[ps->sampleslot]->rate;
	}
}
static void _macroOffsetJitter(uint32_t fptr, uint16_t *spr, int m, InstSamplerState *iss, Track *cv, Row *r)
{
	InstSamplerPlaybackState *ps = cv->inststate;
	if (cv->r.note != NOTE_VOID) /* if playing a note */
	{
		if (r->note == NOTE_VOID) /* if not changing note, explicit ramping needed */
			ramp(fptr, spr, 0, 0.0f, cv, p->s->inst->i[cv->r.inst]);
		if (m>>4 == (m&0xf)) /* both nibbles are the same */
			ps->pitchedpointer = (((m&0xf) + (rand()&0xf))*DIV256) * (*iss->sample)[ps->sampleslot]->trimlength * (float)samplerate / (float)(*iss->sample)[ps->sampleslot]->rate;
		else
		{
			int min = MIN(m>>4, m&0xf);
			int max = MAX(m>>4, m&0xf);
		ps->pitchedpointer = (((min + rand()%(max - min +1))<<4)*DIV256) * (*iss->sample)[ps->sampleslot]->trimlength * (float)samplerate / (float)(*iss->sample)[ps->sampleslot]->rate;
		}
	}
}



void macroInstSamplerPostTrig(uint32_t fptr, uint16_t *spr, Track *cv, Row *r, void *state)
{
	if (!instSafe(p->s->inst, cv->r.inst)) return;
	Inst *iv = &p->s->inst->v[p->s->inst->i[cv->r.inst]];
	if (iv->type != INST_TYPE_SAMPLER) return;
	InstSamplerState *iss = iv->state;
	InstSamplerPlaybackState *ps = cv->inststate;

	/* TODO: would like to apply these indescriminately, could cause some nasty edge cases between instrument switches */
	macroStateApply(&ps->pitchshift);
	macroStateApply(&ps->rateredux);

	Macro *m;
	FOR_ROW_MACROS(i, cv)
	{
		m = &r->macro[i];
		switch (m->c)
		{
			case MACRO_OFFSET:                cv->reverse = 0;           _macroOffset(fptr, spr, m->v, iss, cv, r); break;
			case MACRO_REVERSE_OFFSET:        cv->reverse = 1; if (m->v) _macroOffset(fptr, spr, m->v, iss, cv, r); break;

			case MACRO_OFFSET_JITTER:         cv->reverse = 0;           _macroOffsetJitter(fptr, spr, m->v, iss, cv, r); break;
			case MACRO_REVERSE_OFFSET_JITTER: cv->reverse = 1; if (m->v) _macroOffsetJitter(fptr, spr, m->v, iss, cv, r); break;

			case MACRO_PITCH_SHIFT:        macroStateSet   (&ps->pitchshift, m); break;
			case MACRO_SMOOTH_PITCH_SHIFT: macroStateSmooth(&ps->pitchshift, m); break;

			case MACRO_CYCLE_LENGTH_HI_BYTE:
				if (ps->localcyclelength == -1) ps->localcyclelength = iss->cyclelength;
				ps->localcyclelength = (((uint16_t)ps->localcyclelength<<8)>>8) + (m->v<<8);
				break;
			case MACRO_CYCLE_LENGTH_LO_BYTE:
				if (ps->localcyclelength == -1) ps->localcyclelength = iss->cyclelength;
				ps->localcyclelength = (((uint16_t)ps->localcyclelength>>8)<<8)+m->v;
				break;

			case MACRO_SAMPLERATE:        macroStateSet   (&ps->rateredux, m); break;
			case MACRO_SMOOTH_SAMPLERATE: macroStateSmooth(&ps->rateredux, m); break;

			case MACRO_ATT_DEC: ps->localenvelope = m->v; break;
			case MACRO_SUS_REL: ps->localsustain  = m->v; break;
		}
	}
}

void macroInstSamplerTriggerNote(uint32_t fptr, Track *cv, float oldnote, float note, short inst, void *state)
{
	if (!instSafe(p->s->inst, cv->r.inst)) return;
	Inst *iv = &p->s->inst->v[p->s->inst->i[cv->r.inst]];
	if (iv->type != INST_TYPE_SAMPLER) return;
	InstSamplerPlaybackState *ps = cv->inststate;

	/* local controls */
	macroStateReset(&ps->pitchshift);
	macroStateReset(&ps->rateredux);
	ps->localenvelope = -1;
	ps->localsustain = -1;
	ps->localcyclelength = -1;
}
