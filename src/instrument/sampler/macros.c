static void _macroOffset(uint32_t fptr, uint16_t *spr, int m, InstSamplerState *iss, Track *cv, Row *r)
{
	if (cv->r.note != NOTE_VOID) /* if playing a note */
	{
		if (r->note == NOTE_VOID) /* if not changing note, explicit ramping needed */
			ramp(fptr, spr, 0, cv, 0.0f, p->s->inst->i[cv->r.inst]);
		cv->pitchedpointer = (m*DIV256) * (*iss->sample)[cv->sampleslot]->trimlength * (float)samplerate / (float)(*iss->sample)[cv->sampleslot]->rate;
	}
}
static void _macroOffsetJitter(uint32_t fptr, uint16_t *spr, int m, InstSamplerState *iss, Track *cv, Row *r)
{
	if (cv->r.note != NOTE_VOID) /* if playing a note */
	{
		if (r->note == NOTE_VOID) /* if not changing note, explicit ramping needed */
			ramp(fptr, spr, 0, cv, 0.0f, p->s->inst->i[cv->r.inst]);
		if (m>>4 == (m&0xf)) /* both nibbles are the same */
			cv->pitchedpointer = (((m&0xf) + (rand()&0xf))*DIV256) * (*iss->sample)[cv->sampleslot]->trimlength * (float)samplerate / (float)(*iss->sample)[cv->sampleslot]->rate;
		else
		{
			int min = MIN(m>>4, m&0xf);
			int max = MAX(m>>4, m&0xf);
		cv->pitchedpointer = (((min + rand()%(max - min +1))<<4)*DIV256) * (*iss->sample)[cv->sampleslot]->trimlength * (float)samplerate / (float)(*iss->sample)[cv->sampleslot]->rate;
		}
	}
}



void macroInstSamplerPostTrig(uint32_t fptr, uint16_t *spr, Track *cv, Row *r, void *state)
{
	macroStateApply(&cv->pitchshift);
	macroStateApply(&cv->pitchwidth);
	macroStateApply(&cv->samplerate);

	if (!instSafe(p->s->inst, cv->r.inst)) return;
	Inst *iv = &p->s->inst->v[p->s->inst->i[cv->r.inst]];
	if (iv->type != INST_TYPE_SAMPLER) return;
	InstSamplerState *iss = iv->state;

	FOR_ROW_MACROS(i, cv)
		switch (r->macro[i].c)
		{
			case MACRO_OFFSET:                cv->reverse = 0;                    _macroOffset(fptr, spr, r->macro[i].v, iss, cv, r); break;
			case MACRO_REVERSE_OFFSET:        cv->reverse = 1; if (r->macro[i].v) _macroOffset(fptr, spr, r->macro[i].v, iss, cv, r); break;

			case MACRO_OFFSET_JITTER:         cv->reverse = 0;                    _macroOffsetJitter(fptr, spr, r->macro[i].v, iss, cv, r); break;
			case MACRO_REVERSE_OFFSET_JITTER: cv->reverse = 1; if (r->macro[i].v) _macroOffsetJitter(fptr, spr, r->macro[i].v, iss, cv, r); break;

			case MACRO_PITCH_SHIFT:        macroStateSet   (&cv->pitchshift, r->macro[i]); break;
			case MACRO_SMOOTH_PITCH_SHIFT: macroStateSmooth(&cv->pitchshift, r->macro[i]); break;

			case MACRO_PITCH_WIDTH:        macroStateSet   (&cv->pitchwidth, r->macro[i]); break;
			case MACRO_SMOOTH_PITCH_WIDTH: macroStateSmooth(&cv->pitchwidth, r->macro[i]); break;

			case MACRO_CYCLE_LENGTH_HI_BYTE:
				if (cv->localcyclelength == -1) cv->localcyclelength = iss->granular.cyclelength;
				cv->localcyclelength = (((uint16_t)cv->localcyclelength<<8)>>8) + (r->macro[i].v<<8);
				break;
			case MACRO_CYCLE_LENGTH_LO_BYTE:
				if (cv->localcyclelength == -1) cv->localcyclelength = iss->granular.cyclelength;
				cv->localcyclelength = (((uint16_t)cv->localcyclelength>>8)<<8)+r->macro[i].v;
				break;

			case MACRO_SAMPLERATE:        macroStateSet   (&cv->samplerate, r->macro[i]); break;
			case MACRO_SMOOTH_SAMPLERATE: macroStateSmooth(&cv->samplerate, r->macro[i]); break;

			case MACRO_ATT_DEC: cv->localenvelope = r->macro[i].v; break;
			case MACRO_SUS_REL: cv->localsustain = r->macro[i].v; break;
		}
}

void macroInstSamplerTriggerNote(uint32_t fptr, Track *cv, uint8_t oldnote, uint8_t note, short inst, void *state)
{
	/* local controls */
	macroStateReset(&cv->pitchshift);
	macroStateReset(&cv->pitchwidth);
	macroStateReset(&cv->samplerate);
	cv->localenvelope = -1;
	cv->localsustain = -1;
	cv->localcyclelength = -1;
}
