static void _macroOffset(uint32_t fptr, uint16_t *spr, int m, InstSamplerState *iss, Track *cv, Row *r)
{
	InstSamplerPlaybackState *ps = cv->inststate;
	if (cv->r.note != NOTE_VOID) /* if playing a note */
	{
		if (r->note == NOTE_VOID) /* if not changing note, explicit ramping needed */
			ramp(fptr, spr, 0, 0.0f, cv, p->s->inst->i[cv->r.inst]);
		cv->pitchedpointer = (m*DIV256) * (*iss->sample)[ps->sampleslot]->trimlength * (float)samplerate / (float)(*iss->sample)[ps->sampleslot]->rate;
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
			cv->pitchedpointer = (((m&0xf) + (rand()&0xf))*DIV256) * (*iss->sample)[ps->sampleslot]->trimlength * (float)samplerate / (float)(*iss->sample)[ps->sampleslot]->rate;
		else
		{
			int min = MIN(m>>4, m&0xf);
			int max = MAX(m>>4, m&0xf);
		cv->pitchedpointer = (((min + rand()%(max - min +1))<<4)*DIV256) * (*iss->sample)[ps->sampleslot]->trimlength * (float)samplerate / (float)(*iss->sample)[ps->sampleslot]->rate;
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
	macroStateApply(&ps->pitchwidth);
	macroStateApply(&ps->rateredux);

	FOR_ROW_MACROS(i, cv)
		switch (r->macro[i].c)
		{
			case MACRO_OFFSET:                cv->reverse = 0;                    _macroOffset(fptr, spr, r->macro[i].v, iss, cv, r); break;
			case MACRO_REVERSE_OFFSET:        cv->reverse = 1; if (r->macro[i].v) _macroOffset(fptr, spr, r->macro[i].v, iss, cv, r); break;

			case MACRO_OFFSET_JITTER:         cv->reverse = 0;                    _macroOffsetJitter(fptr, spr, r->macro[i].v, iss, cv, r); break;
			case MACRO_REVERSE_OFFSET_JITTER: cv->reverse = 1; if (r->macro[i].v) _macroOffsetJitter(fptr, spr, r->macro[i].v, iss, cv, r); break;

			case MACRO_PITCH_SHIFT:        macroStateSet   (&ps->pitchshift, r->macro[i]); break;
			case MACRO_SMOOTH_PITCH_SHIFT: macroStateSmooth(&ps->pitchshift, r->macro[i]); break;

			case MACRO_PITCH_WIDTH:        macroStateSet   (&ps->pitchwidth, r->macro[i]); break;
			case MACRO_SMOOTH_PITCH_WIDTH: macroStateSmooth(&ps->pitchwidth, r->macro[i]); break;

			case MACRO_CYCLE_LENGTH_HI_BYTE:
				if (ps->localcyclelength == -1) ps->localcyclelength = iss->granular.cyclelength;
				ps->localcyclelength = (((uint16_t)ps->localcyclelength<<8)>>8) + (r->macro[i].v<<8);
				break;
			case MACRO_CYCLE_LENGTH_LO_BYTE:
				if (ps->localcyclelength == -1) ps->localcyclelength = iss->granular.cyclelength;
				ps->localcyclelength = (((uint16_t)ps->localcyclelength>>8)<<8)+r->macro[i].v;
				break;

			case MACRO_SAMPLERATE:        macroStateSet   (&ps->rateredux, r->macro[i]); break;
			case MACRO_SMOOTH_SAMPLERATE: macroStateSmooth(&ps->rateredux, r->macro[i]); break;

			case MACRO_ATT_DEC: ps->localenvelope = r->macro[i].v; break;
			case MACRO_SUS_REL: ps->localsustain = r->macro[i].v; break;
		}
}

void macroInstSamplerTriggerNote(uint32_t fptr, Track *cv, uint8_t oldnote, uint8_t note, short inst, void *state)
{
	if (!instSafe(p->s->inst, cv->r.inst)) return;
	Inst *iv = &p->s->inst->v[p->s->inst->i[cv->r.inst]];
	if (iv->type != INST_TYPE_SAMPLER) return;
	InstSamplerPlaybackState *ps = cv->inststate;

	/* local controls */
	macroStateReset(&ps->pitchshift);
	macroStateReset(&ps->pitchwidth);
	macroStateReset(&ps->rateredux);
	ps->localenvelope = -1;
	ps->localsustain = -1;
	ps->localcyclelength = -1;
}
