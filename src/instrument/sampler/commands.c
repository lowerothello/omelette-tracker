static void _commandOffset(uint32_t fptr, uint16_t *spr, int m, InstSamplerState *iss, Track *cv, Row *r)
{
	InstSamplerPlaybackState *ps = cv->inststate;
	if (cv->r.note != NOTE_VOID) /* if playing a note */
	{
		if (r->note == NOTE_VOID) /* if not changing note, explicit ramping needed */
			ramp(fptr, spr, 0, 0.0f, cv, p->s->inst->i[cv->r.inst]);
		ps->pitchedpointer = (m*DIV256) * iss->sample->trimlength * (float)samplerate / (float)iss->sample->rate;
	}
}
static void _commandOffsetJitter(uint32_t fptr, uint16_t *spr, int m, InstSamplerState *iss, Track *cv, Row *r)
{
	InstSamplerPlaybackState *ps = cv->inststate;
	if (cv->r.note != NOTE_VOID) /* if playing a note */
	{
		if (r->note == NOTE_VOID) /* if not changing note, explicit ramping needed */
			ramp(fptr, spr, 0, 0.0f, cv, p->s->inst->i[cv->r.inst]);
		if (m>>4 == (m&0xf)) /* both nibbles are the same */
			ps->pitchedpointer = (((m&0xf) + (rand()&0xf))*DIV256) * iss->sample->trimlength * (float)samplerate / (float)iss->sample->rate;
		else
		{
			int min = MIN(m>>4, m&0xf);
			int max = MAX(m>>4, m&0xf);
		ps->pitchedpointer = (((min + rand()%(max - min +1))<<4)*DIV256) * iss->sample->trimlength * (float)samplerate / (float)iss->sample->rate;
		}
	}
}



void commandInstSamplerPostTrig(uint32_t fptr, uint16_t *spr, Track *cv, Row *r, void *state)
{
	if (!instSafe(p->s->inst, cv->r.inst)) return;
	Inst *iv = &p->s->inst->v[p->s->inst->i[cv->r.inst]];
	if (iv->type != INST_TYPE_SAMPLER) return;
	InstSamplerState *iss = iv->state;
	InstSamplerPlaybackState *ps = cv->inststate;

	/* TODO: would like to apply these indescriminately, could cause some nasty edge cases between instrument switches */
	commandStateApply(&ps->pitchshift);
	commandStateApply(&ps->rateredux);

	Command *m;
	FOR_ROW_COMMANDS(i, cv)
	{
		m = &r->command[i];
		switch (m->c)
		{
			case COMMAND_OFFSET:                cv->reverse = 0;           _commandOffset(fptr, spr, m->v, iss, cv, r); break;
			case COMMAND_REVERSE_OFFSET:        cv->reverse = 1; if (m->v) _commandOffset(fptr, spr, m->v, iss, cv, r); break;

			case COMMAND_OFFSET_JITTER:         cv->reverse = 0;           _commandOffsetJitter(fptr, spr, m->v, iss, cv, r); break;
			case COMMAND_REVERSE_OFFSET_JITTER: cv->reverse = 1; if (m->v) _commandOffsetJitter(fptr, spr, m->v, iss, cv, r); break;

			case COMMAND_PITCH_SHIFT:        commandStateSet   (&ps->pitchshift, m); break;
			case COMMAND_SMOOTH_PITCH_SHIFT: commandStateSmooth(&ps->pitchshift, m); break;

			case COMMAND_CYCLE_LENGTH_HI_BYTE:
				if (ps->localcyclelength == -1) ps->localcyclelength = iss->cyclelength;
				ps->localcyclelength = (((uint16_t)ps->localcyclelength<<8)>>8) + (m->v<<8);
				break;
			case COMMAND_CYCLE_LENGTH_LO_BYTE:
				if (ps->localcyclelength == -1) ps->localcyclelength = iss->cyclelength;
				ps->localcyclelength = (((uint16_t)ps->localcyclelength>>8)<<8)+m->v;
				break;

			case COMMAND_SAMPLERATE:        commandStateSet   (&ps->rateredux, m); break;
			case COMMAND_SMOOTH_SAMPLERATE: commandStateSmooth(&ps->rateredux, m); break;

			case COMMAND_ATT_DEC: ps->localenvelope = m->v; break;
			case COMMAND_SUS_REL: ps->localsustain  = m->v; break;
		}
	}
}

void commandInstSamplerTriggerNote(uint32_t fptr, Track *cv, float oldnote, float note, short inst, void *state)
{
	if (!instSafe(p->s->inst, cv->r.inst)) return;
	Inst *iv = &p->s->inst->v[p->s->inst->i[cv->r.inst]];
	if (iv->type != INST_TYPE_SAMPLER) return;
	InstSamplerPlaybackState *ps = cv->inststate;

	/* local controls */
	commandStateReset(&ps->pitchshift);
	commandStateReset(&ps->rateredux);
	ps->localenvelope = -1;
	ps->localsustain = -1;
	ps->localcyclelength = -1;
}
