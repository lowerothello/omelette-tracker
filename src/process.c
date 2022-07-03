#define PLAY_LOCK_OK 0    /* p->s and p->w are safe */
#define PLAY_LOCK_START 1 /* p->s and p->w want to be unsafe */
#define PLAY_LOCK_CONT 2  /* p->s and p->w are unsafe */
typedef struct
{
	portbufferpair in;
	portbufferpair out;
	portbufferpair cin[MAX_CHANNELS];
	portbufferpair cout[MAX_CHANNELS];
} portbuffers;

char ifMacro(channel *cv, row r, char m, char (*callback)(int, channel *, row))
{
	char ret = 0;
	for (int i = 0; i < cv->macroc; i++)
		if (r.macro[i].c == m)
			ret = callback(r.macro[i].v, cv, r);
	return ret;
}

void changeBpm(song *s, uint8_t newbpm)
{
	s->bpm = newbpm;
	s->spr = samplerate * (60.0 / newbpm) / (s->rowhighlight * 2);
}

/* freewheel to fill up the ramp buffer */
void ramp(playbackinfo *p, channel *cv, uint8_t realinstrument, uint32_t pointeroffset)
{
	/* clear the rampbuffer so cruft isn't played in edge cases */
	memset(cv->rampbuffer, 0, sizeof(sample_t) * rampmax * 2);
	instrument *iv = p->s->instrumentv[realinstrument];
	if (iv)
	{
		for (uint16_t i = 0; i < rampmax; i++)
		{
			if (!cv->r.note) break;
			t->f[iv->type].process(iv, cv, pointeroffset+i,
					&cv->rampbuffer[i * 2 + 0], &cv->rampbuffer[i * 2 + 1]);
		}
	}
}

void _triggerNote(channel *cv, uint8_t note, uint8_t inst)
{
	if (note == 255) /* note off */
	{
		if (!cv->releasepointer) cv->releasepointer = cv->pointer;
	} else
	{
		cv->r.inst = inst;
		cv->r.note = note;
		cv->portamento = 0;
		cv->pointer = 0;
		cv->portamentofinetune = 0.0;
		cv->pointeroffset = 0;
		cv->releasepointer = 0;
		cv->gain = -1;
		cv->targetgain = -1;
	}
}
/* ramping */
void triggerNote(channel *cv, uint8_t note, uint8_t inst)
{
	if (cv->r.note && note != 255 && p->s->instrumenti[cv->r.inst]) /* old note, ramp it out */
	{
		instrument *iv = p->s->instrumentv[p->s->instrumenti[cv->r.inst]];
		if (!(p->w->instrumentlockv == INST_GLOBAL_LOCK_FREE
					&& p->w->instrumentlocki == p->s->instrumenti[cv->r.inst])
				&& cv->r.note && iv->type < INSTRUMENT_TYPE_COUNT)
			ramp(p, cv, p->s->instrumenti[cv->r.inst], cv->pointer);
		cv->rampindex = 0; /* set this even if it's not populated */
	}
	_triggerNote(cv, note, inst);
}


void calcVibrato(channel *cv, int m)
{
	cv->vibrato = m%16;
	if (!cv->vibratosamples) /* reset the phase if starting */
		cv->vibratosamplepointer = 0;
	cv->vibratosamples = p->s->spr / (((m>>4) + 1) / 16.0); /* use floats for slower lfo speeds */
	cv->vibratosamplepointer = MIN(cv->vibratosamplepointer, cv->vibratosamples - 1);
}

char Vc(int m, channel *cv, row r)
{
	if (m >= 0)
		calcVibrato(cv, m);
	else cv->vibratosamples = 0;
	return 0;
}
/* inst==255 for off */
void playChannel(jack_nframes_t fptr, playbackinfo *p, portbuffers pb, channel *cv, uint8_t inst)
{
	instrument *iv = p->s->instrumentv[p->s->instrumenti[inst]];

	cv->finetune = cv->portamentofinetune;
	if (cv->vibratosamples)
	{
		cv->finetune += oscillator(0, (float)cv->vibratosamplepointer / (float)cv->vibratosamples, 0.5)
			* cv->vibrato*DIV16;

		/* re-read the macro once phase is about to overflow */
		cv->vibratosamplepointer++;
		if (cv->vibratosamplepointer > cv->vibratosamples)
		{
			cv->vibratosamplepointer = 0;
			ifMacro(cv, cv->r, 'V', &Vc); // vibrato
		}
	}

	if (cv->cutsamples && p->s->sprp > cv->cutsamples)
	{
		ramp(p, cv, p->s->instrumenti[inst], cv->pointer);
		cv->rampindex = 0;
		cv->r.note = 0;
		cv->vibrato = 0;
		cv->cutsamples = 0;
	}
	if (cv->delaysamples && p->s->sprp > cv->delaysamples)
	{
		triggerNote(cv, cv->delaynote, cv->delayinst);
		cv->delaysamples = 0;
	}

	float l, r = 0.0;
	/* process the type */
	if (iv && cv->r.note)
	{
		if (!(p->w->instrumentlockv == INST_GLOBAL_LOCK_FREE
					&& p->w->instrumentlocki == p->s->instrumenti[cv->r.inst])
				&& cv->r.note != 255 && iv->type < INSTRUMENT_TYPE_COUNT)
		{
			if (cv->rtrigsamples && inst != 255)
			{
				uint32_t rtrigoffset = (cv->pointer - cv->rtrigpointer) % cv->rtrigsamples;
				if (rtrigoffset == 0 && cv->pointer > cv->rtrigpointer)
				{ /* first sample of any retrigger but the first */
					uint8_t oldnote = cv->r.note; /* in case the ramping freewheel cuts the note */
					ramp(p, cv, p->s->instrumenti[inst], cv->pointer + cv->rtrigsamples);
					cv->r.note = oldnote;
					cv->rampindex = 0;
				}
				t->f[iv->type].process(iv, cv, cv->rtrigpointer + rtrigoffset, &l, &r);
			} else
				t->f[iv->type].process(iv, cv, cv->pointer, &l, &r);
			cv->pointer++;
		} else
		{
			cv->vibrato = 0;
			l = r = 0.0;
		}
	} else
	{
		cv->vibrato = 0;
		l = r = 0.0;
	}

	/* mix in ramp data */
	if (cv->rampindex < rampmax)
	{
		float gain = (float)cv->rampindex / (float)rampmax;
		l = l * gain + cv->rampbuffer[cv->rampindex * 2 + 0] * (1.0 - gain);
		r = r * gain + cv->rampbuffer[cv->rampindex * 2 + 1] * (1.0 - gain);
		cv->rampindex++;
	}

	if (l == 0.0 && r == 0.0) return;

	/* waveshapers */ /* TODO: ramping */
	if (cv->wavefolder)
	{
		float gain = 1.0 + cv->wavefolder*0.3;
		l = wavefolder(l * gain);
		r = wavefolder(r * gain);
	}
	if (cv->wavewrapper)
	{
		float gain = 1.0 + cv->wavewrapper*0.2;
		l = wavewrapper(l * gain);
		r = wavewrapper(r * gain);
	}
	if (cv->signedunsigned)
	{
		float mix = cv->signedunsigned/15.0;
		l = signedunsigned(l) * (MIN(mix, 0.5) * 2) + l * (1.0 - (MAX(mix, 0.5) - 0.5) * 2);
		r = signedunsigned(r) * (MIN(mix, 0.5) * 2) + r * (1.0 - (MAX(mix, 0.5) - 0.5) * 2);
	}
	if (cv->rectifier)
	{
		float mix = cv->rectifier/15.0;
		l = rectify(cv->rectifiertype, l) * (MIN(mix, 0.5) * 2) + l * (1.0 - (MAX(mix, 0.5) - 0.5) * 2);
		r = rectify(cv->rectifiertype, r) * (MIN(mix, 0.5) * 2) + r * (1.0 - (MAX(mix, 0.5) - 0.5) * 2);
	}
	if (cv->hardclip)
	{
		float gain = 1.0 + cv->hardclip*1.50;
		l = hardclip(l * gain);
		r = hardclip(r * gain);
	}
	if (cv->softclip)
	{
		float gain = 1.0 + cv->softclip*0.4;
		l = thirddegreepolynomial(l * gain);
		r = thirddegreepolynomial(r * gain);
	}

	/* gate */
	if (MAX(fabsf(l), fabsf(r)) < (cv->gate%16)*0.025)
		cv->gateopen = MAX(0.5 - ((cv->gate>>4)/15.0) * 0.5, cv->gateopen - MIN_GATE_SPEED_SEC/samplerate - (cv->gate>>4)*DIV128);
	else
		cv->gateopen = MIN(1.0, cv->gateopen + MIN_GATE_SPEED_SEC/samplerate + (cv->gate>>4)*DIV128);
	l *= cv->gateopen;
	r *= cv->gateopen;

	/* gain macro, and all it's inversions */
	if (cv->targetgain != -1)
	{
		float rowprogress = (float)p->s->sprp / (float)p->s->spr;
		if (cv->gain != -1)
		{
			l *= (cv->gain>>4) / 15.0 + ((cv->targetgain>>4) - (cv->gain>>4)) / 15.0 * rowprogress;
			r *= (cv->gain%16) / 15.0 + ((cv->targetgain%16) - (cv->gain%16)) / 15.0 * rowprogress;
		} else if (iv)
		{
			l *= (iv->defgain>>4) / 15.0 + ((cv->targetgain>>4) - (iv->defgain>>4)) / 15.0 * rowprogress;
			r *= (iv->defgain%16) / 15.0 + ((cv->targetgain%16) - (iv->defgain%16)) / 15.0 * rowprogress;
		}
	} else
	{
		if (cv->gain != -1)
		{
			l *= (cv->gain>>4) / 15.0;
			r *= (cv->gain%16) / 15.0;
		} else if (iv)
		{
			l *= (iv->defgain>>4) / 15.0;
			r *= (iv->defgain%16) / 15.0;
		}
	}

	if (!cv->mute)
	{
		pb.out.l[fptr] += l;
		pb.out.r[fptr] += r;
	}
	if (p->w->instrumentrecv == INST_REC_LOCK_CONT
			&& p->w->recordsource == 1
			&& p->w->recordflags & 0b1)
	{
		p->w->recchannelbuffer[fptr * 2 + 0] = l;
		p->w->recchannelbuffer[fptr * 2 + 1] = r;
	}
}

void bendUp(channel *cv, uint32_t spr, uint32_t count)
{
	cv->portamentofinetune += (12.0 / spr) * (cv->portamentospeed / 255.0) * count;
	while (cv->portamentofinetune > 0.5)
	{
		cv->portamentofinetune -= 1.0;
		cv->r.note++;
	}
	if (cv->r.note > cv->portamento || (cv->r.note == cv->portamento && cv->portamentofinetune >= 0.0))
	{
		cv->portamentofinetune = 0.0;
		cv->r.note = cv->portamento;
		cv->portamento = 0;
	}
}

void bendDown(channel *cv, uint32_t spr, uint32_t count)
{
	cv->portamentofinetune -= (12.0 / spr) * (cv->portamentospeed / 255.0);
	while (cv->portamentofinetune < 0.5)
	{
		cv->portamentofinetune += 1.0;
		cv->r.note--;
	}
	if (cv->r.note <= cv->portamento && cv->portamentofinetune <= 0.0)
	{
		cv->portamentofinetune = 0.0;
		cv->r.note = cv->portamento;
		cv->portamento = 0;
	}
}

char Bc(int m, channel *cv, row r)
{ if (m >= 32) changeBpm(p->s, m); return 0; }

char Cc(int m, channel *cv, row r)
{
	if (m != -1 && m>>4 == 0)
	{
		if (!(p->w->instrumentlockv == INST_GLOBAL_LOCK_FREE
				&& p->w->instrumentlocki == p->s->instrumenti[cv->r.inst]))
			ramp(p, cv, p->s->instrumenti[cv->r.inst], cv->pointer);
		cv->rampindex = 0;
		cv->r.note = 0;
		cv->vibrato = 0;
		return 1;
	} else if (m != -1 && m%16 != 0) /* cut, don't divide by 0 */
	{
		cv->cutsamples = p->s->spr * (float)(m>>4) / (float)(m%16);
		return 1;
	}
	return 0;
}
char Pc(int m, channel *cv, row r)
{
	if (m != -1)
	{
		cv->portamento = r.note;
		cv->portamentospeed = m;
		return 1;
	}
	return 0;
}
char Dc(int m, channel *cv, row r)
{
	if (m != -1 && m%16 != 0)
	{
		cv->delaysamples = p->s->spr * (float)(m>>4) / (float)(m%16);
		cv->delaynote = r.note;
		cv->delayinst = r.inst;
		return 1;
	}
	return 0;
}
char dc(int m, channel *cv, row r)
{
	if (m != -1)
	{
		cv->delaysamples = p->s->spr * m*DIV256;
		cv->delaynote = r.note;
		cv->delayinst = r.inst;
		return 1;
	}
	return 0;
}
char Gc(int m, channel *cv, row r)
{
	if (m != -1)
	{
		if (cv->pointer)
		{
			ramp(p, cv, p->s->instrumenti[cv->r.inst], cv->pointer);
			cv->rampindex = 0;
		}
		cv->gain = m;
		return 1;
	}
	return 0;
}
char gc(int m, channel *cv, row r)
{
	if (m != -1)
	{
		if (cv->pointer) /* only slide if a note is already playing */
			cv->targetgain = m;
		else
			cv->gain = m;
	}
	return 0;
}
char Rc(int m, channel *cv, row r)
{
	if (m != -1 && m%16 != 0)
	{
		cv->rtrigpointer = cv->pointer;
		cv->rtrigsamples = (p->s->spr / (m%16)) * ((m>>4) + 1);
		cv->rtrigblocksize = m>>4;
	} else if (cv->rtrigsamples)
	{
		if (cv->rtrigblocksize)
			cv->rtrigblocksize--;
		else
			cv->rtrigsamples = 0;
	}
	return 0;
}
char Wc(int m, channel *cv, row r)
{
	if (m != -1)
	{
		switch (m>>4)
		{
			case 0: cv->hardclip = m%16; break;
			case 1: cv->softclip = m%16; break;
			case 2: cv->rectifiertype = 0; cv->rectifier = m%16; break;
			case 3: cv->rectifiertype = 1; cv->rectifier = m%16; break;
			case 4: cv->wavefolder = m%16; break;
			case 5: cv->wavewrapper = m%16; break;
			case 6: cv->signedunsigned = m%16; break;
		}
	}
	return 0;
}

char tc(int m, channel *cv, row r)
{ cv->gate = m; return 0; }

void preprocessRow(channel *cv, row r)
{
	char ret;

	ifMacro(cv, r, 'B', &Bc); // bpm

	ret = ifMacro(cv, r, 'C', &Cc); // cut
	if (!ret && r.note)
	{
		ret = ifMacro(cv, r, 'P', &Pc);
		if (!ret)
		{
			ret = ifMacro(cv, r, 'D', &Dc);
			if (!ret)
			{
				ret = ifMacro(cv, r, 'd', &dc);
				if (!ret) triggerNote(cv, r.note, r.inst);
			}
		}
	}

	/* gain */
	if (cv->targetgain != -1)
	{
		cv->gain = cv->targetgain;
		cv->targetgain = -1;
	}
	ret = ifMacro(cv, r, 'G', &Gc);
	if (!ret)
		ifMacro(cv, r, 'g', &gc);

	ifMacro(cv, r, 'R', &Rc); /* retrigger */
	ifMacro(cv, r, 'V', &Vc); /* vibrato   */

	/* type macros */
	instrument *iv = p->s->instrumentv[p->s->instrumenti[cv->r.inst]];
	for (int i = 0; i < cv->macroc; i++)
	{
		if (isdigit(r.macro[i].c))
		{
			int m = r.macro[i].v;
			if (iv && iv->type < INSTRUMENT_TYPE_COUNT && t->f[iv->type].macro)
				t->f[iv->type].macro(iv, cv, r, r.macro[i].c - 48, m);
		}
	}

	ifMacro(cv, r, 'W', &Wc); /* waveshaper */
	ifMacro(cv, r, 't', &tc); /* gate       */

	for (int i = 0; i < cv->macroc; i++)
		cv->r.macro[i] = r.macro[i];
}

int process(jack_nframes_t nfptr, void *arg)
{
	playbackinfo *p = arg;
	channel *cv;
	portbuffers pb;
	pb.in.l =  jack_port_get_buffer(p->in.l, nfptr);
	pb.in.r =  jack_port_get_buffer(p->in.r, nfptr);
	pb.out.l = jack_port_get_buffer(p->out.l, nfptr); memset(pb.out.l, 0, nfptr * sizeof(sample_t));
	pb.out.r = jack_port_get_buffer(p->out.r, nfptr); memset(pb.out.r, 0, nfptr * sizeof(sample_t));

	if (p->lock == PLAY_LOCK_START)
		p->lock = PLAY_LOCK_CONT;

	if (p->lock == PLAY_LOCK_CONT) return 0;


	/* just need to know these have been seen */
	/* will no longer write to the record buffer */
	if (p->w->instrumentrecv == INST_REC_LOCK_PREP_END)
		p->w->instrumentrecv = INST_REC_LOCK_END;
	if (p->w->instrumentrecv == INST_REC_LOCK_PREP_CANCEL)
		p->w->instrumentrecv = INST_REC_LOCK_CANCEL;
	/* start recording if gate pattern is off */
	if (p->w->instrumentrecv == INST_REC_LOCK_START && !(p->w->recordflags & 0b10))
	{
		p->w->instrumentrecv = INST_REC_LOCK_CONT;
		p->dirty = 1;
	}
	/* will no longer access the instrument state */
	if (p->w->instrumentlockv == INST_GLOBAL_LOCK_PREP_FREE
			|| p->w->instrumentlockv == INST_GLOBAL_LOCK_PREP_HIST
			|| p->w->instrumentlockv == INST_GLOBAL_LOCK_PREP_PUT)
		p->w->instrumentlockv++;


	if (p->w->previewtrigger) switch (p->w->previewtrigger)
	{
		case 1: // start instrument preview
			channel *cv = &p->s->channelv[p->w->previewchannel];
			cv->gain = -1;
			// triggerNote(cv, p->w->previewnote, p->w->previewinst);
			row r;
			r.note = p->w->previewnote;
			r.inst = p->w->previewinst;
			r.macro[0].c = p->w->previewmacro.c;
			r.macro[0].v = p->w->previewmacro.v;
			preprocessRow(cv, r);
			p->w->previewtrigger++;
			break;
		case 3: // start sample preview
			p->w->previewtrigger++;
			break;
		case 4: // continue sample preview
		case 5: // unload sample
			break;
	}


	/* start/stop playback */
	if (p->s->playing == PLAYING_START)
	{
		changeBpm(p->s, p->s->songbpm);
		p->s->sprp = 0;

		/* clear the channels */
		for (uint8_t c = 0; c < p->s->channelc; c++)
		{
			cv = &p->s->channelv[c];
			cv->r.note = 0;
			cv->softclip = 0;
			cv->hardclip = 0;
			cv->wavefolder = 0;
			cv->wavewrapper = 0;
			cv->signedunsigned = 0;
			cv->rectifier = 0;
			cv->gate = 0;
			cv->gateopen = 1.0;
		}

		/* non-volatile state */
		/* start at the beginning of the block */
		uint8_t blockstart = 0;
		for (uint8_t i = p->s->songp; i >= 0; i--)
			if (p->s->songi[i] == 255)
			{
				blockstart = i + 1;
				break;
			}

		/* for each pattern in the block */
		pattern *pt;
		channel *cv;
		for (uint8_t b = blockstart; b < p->s->songp; b++)
		{
			pt = p->s->patternv[p->s->patterni[p->s->songi[b]]];
			/* for each row */
			for (uint8_t r = 0; r < pt->rowc; r++)
			{
				for (uint8_t c = 0; c < p->s->channelc; c++)
				{
					cv = &p->s->channelv[c];
					preprocessRow(cv, pt->rowv[c][r]);

					if (cv->cutsamples && cv->delaysamples)
					{
						if (cv->cutsamples > cv->delaysamples)
						{
							cv->r.note = 0;
							cv->cutsamples = 0;
						} else
						{
							_triggerNote(cv, cv->delaynote, cv->delayinst);
							cv->pointer = p->s->spr - cv->pointer;
							cv->delaysamples = 0;
						}
					}
					else if (cv->cutsamples)
					{
						cv->r.note = 0;
						cv->cutsamples = 0;
					}
					else if (cv->delaysamples)
					{
						_triggerNote(cv, cv->delaynote, cv->delayinst);
						cv->pointer = p->s->spr - cv->pointer;
						cv->delaysamples = 0;
					} else if (cv->r.note) cv->pointer += p->s->spr;

					if (cv->r.note && cv->portamento)
					{
						if (cv->r.note == cv->portamento)
						{ /* fine bend */
							if (cv->portamentofinetune < 0.0)
								bendUp(cv, p->s->spr, p->s->spr);
							else
								bendDown(cv, p->s->spr, p->s->spr);
						} else if (cv->r.note < cv->portamento)
						{ /* bend up */
							bendUp(cv, p->s->spr, p->s->spr);
						} else
						{ /* bend down */
							bendDown(cv, p->s->spr, p->s->spr);
						}
					}
				}
			}
		}
		/* start recording if gate pattern is on */
		if (p->w->instrumentrecv == INST_REC_LOCK_START && p->w->recordflags & 0b10)
			p->w->instrumentrecv = INST_REC_LOCK_CONT;

		p->s->playing = PLAYING_CONT;
	} else if (p->s->playing == PLAYING_PREP_STOP)
	{
		/* stop channels */
		for (uint8_t i = 0; i < p->s->channelc; i++)
		{
			if (!(p->w->instrumentlockv == INST_GLOBAL_LOCK_FREE
					&& p->w->instrumentlocki == p->s->instrumenti[p->s->channelv[i].r.inst])
					&& p->s->channelv[i].r.inst)
				ramp(p, &p->s->channelv[i],
						p->s->instrumenti[p->s->channelv[i].r.inst],
						p->s->channelv[i].pointer);
			p->s->channelv[i].r.note = 0;
		}

		p->dirty = 1;
		p->s->playing = PLAYING_STOP;
	}

	if (p->w->request == REQ_BPM)
	{
		changeBpm(p->s, p->s->songbpm);
		p->w->request = REQ_OK;
	}


	/* loop over samples */
	for (jack_nframes_t fptr = 0; fptr < nfptr; fptr++)
	{
		/* preprocess the channel */
		if (p->s->playing == PLAYING_CONT && p->s->sprp == 0)
			for (uint8_t c = 0; c < p->s->channelc; c++)
				preprocessRow(&p->s->channelv[c],
						p->s->patternv[p->s->patterni[p->s->songi[p->s->songp]]]->rowv[c][p->s->songr]);

		for (uint8_t c = 0; c < p->s->channelc; c++)
		{
			cv = &p->s->channelv[c];
			if (p->s->playing == PLAYING_CONT && cv->portamento)
			{
				if (cv->r.note == cv->portamento)
				{ /* fine bend */
					if (cv->portamentofinetune < 0.0)
						bendUp(cv, p->s->spr, 1);
					else
						bendDown(cv, p->s->spr, 1);
				} else if (cv->r.note < cv->portamento)
				{ /* coarse bend up */
					bendUp(cv, p->s->spr, 1);
				} else
				{ /* coarse bend down */
					bendDown(cv, p->s->spr, 1);
				}
			}
			playChannel(fptr, p, pb, cv, cv->r.inst);
		}

		/* next row */
		if (p->s->sprp++ > p->s->spr)
		{
			p->s->sprp = 0;
			if (p->s->playing == PLAYING_CONT)
			{
				/* next pattern */
				if (p->s->songr++ >= p->s->patternv[p->s->patterni[p->s->songi[p->s->songp]]]->rowc)
				{
					p->s->songr = 0;
					if (p->w->songfy == p->s->songp && w->popup == 0 && w->mode == 0)
						p->w->trackerfy = 0;
					p->dirty = 1;
					
					/* stop recording if gate pattern is on */
					if (p->w->instrumentrecv == INST_REC_LOCK_CONT && p->w->recordflags & 0b10)
						p->w->instrumentrecv = INST_REC_LOCK_END;
					/* start recording if gate pattern is on */
					if (p->w->instrumentrecv == INST_REC_LOCK_START && p->w->recordflags & 0b10)
						p->w->instrumentrecv = INST_REC_LOCK_CONT;

					if (p->w->songnext)
					{
						if (p->w->songfy == p->s->songp && w->popup == 0 && w->mode == 0)
							p->w->songfy = p->w->songnext - 1;
						p->dirty = 1;

						p->s->songp = p->w->songnext - 1;
						p->w->songnext = 0;
					} else if (p->s->songf[p->s->songp]) /* loop */
					{
						/* do nothing, don't inc the song pointer */
					} else if (p->s->songi[p->s->songp + 1] == 255)
					{ /* no next pattern, go to the beginning of the block */
						uint8_t blockstart = 0;
						for (uint8_t i = p->s->songp; i >= 0; i--)
							if (p->s->songi[i] == 255)
							{
								blockstart = i + 1;
								break;
							}

						if (p->w->songfy == p->s->songp && w->popup == 0 && w->mode == 0)
							p->w->songfy = blockstart;
						p->dirty = 1;

						p->s->songp = blockstart;
					} else
					{
						if (p->w->songfy == p->s->songp && w->popup == 0 && w->mode == 0)
							p->w->songfy++;
						p->dirty = 1;

						p->s->songp++;
					}
				} else
				{
					if (p->w->songfy == p->s->songp && w->popup == 0 && w->mode == 0)
						p->w->trackerfy = p->s->songr;
					p->dirty = 1;
				}
			}
		}
	}

	/* record */
	if (p->s->playing == PLAYING_CONT && p->w->instrumentrecv == INST_REC_LOCK_CONT)
	{
		if (p->w->recptr + nfptr > RECORD_LENGTH * samplerate)
		{
			strcpy(p->w->command.error, "record buffer full");
			p->w->instrumentrecv = INST_REC_LOCK_END;
		} else
		{
			for (jack_nframes_t fptr = 0; fptr < nfptr; fptr++)
			{
				switch (p->w->recordsource)
				{
					case 0:
						int c;
						c = (float)pb.in.l[fptr] * (float)SHRT_MAX;
						if      (c > SHRT_MAX) p->w->recbuffer[p->w->recptr * 2 + 0] = SHRT_MAX;
						else if (c < SHRT_MIN) p->w->recbuffer[p->w->recptr * 2 + 0] = SHRT_MIN;
						else                   p->w->recbuffer[p->w->recptr * 2 + 0] = c;
						c = (float)pb.in.r[fptr] * (float)SHRT_MAX;
						if      (c > SHRT_MAX) p->w->recbuffer[p->w->recptr * 2 + 1] = SHRT_MAX;
						else if (c < SHRT_MIN) p->w->recbuffer[p->w->recptr * 2 + 1] = SHRT_MIN;
						else                   p->w->recbuffer[p->w->recptr * 2 + 1] = c;
						break;
					case 1:
						if (p->w->recordflags & 0b1)
						{
							int c;
							c = (float)p->w->recchannelbuffer[fptr * 2 + 0] * (float)SHRT_MAX;
							if      (c > SHRT_MAX) p->w->recbuffer[p->w->recptr * 2 + 0] = SHRT_MAX;
							else if (c < SHRT_MIN) p->w->recbuffer[p->w->recptr * 2 + 0] = SHRT_MIN;
							else                   p->w->recbuffer[p->w->recptr * 2 + 0] = c;
							c = (float)p->w->recchannelbuffer[fptr * 2 + 1] * (float)SHRT_MAX;
							if      (c > SHRT_MAX) p->w->recbuffer[p->w->recptr * 2 + 1] = SHRT_MAX;
							else if (c < SHRT_MIN) p->w->recbuffer[p->w->recptr * 2 + 1] = SHRT_MIN;
							else                   p->w->recbuffer[p->w->recptr * 2 + 1] = c;
						} else
						{
							int c;
							c = (float)pb.out.l[fptr] * (float)SHRT_MAX;
							if      (c > SHRT_MAX) p->w->recbuffer[p->w->recptr * 2 + 0] = SHRT_MAX;
							else if (c < SHRT_MIN) p->w->recbuffer[p->w->recptr * 2 + 0] = SHRT_MIN;
							else                   p->w->recbuffer[p->w->recptr * 2 + 0] = c;
							c = (float)pb.out.r[fptr] * (float)SHRT_MAX;
							if      (c > SHRT_MAX) p->w->recbuffer[p->w->recptr * 2 + 1] = SHRT_MAX;
							else if (c < SHRT_MIN) p->w->recbuffer[p->w->recptr * 2 + 1] = SHRT_MIN;
							else                   p->w->recbuffer[p->w->recptr * 2 + 1] = c;
						}
						break;
				}
				p->w->recptr++;
			}
		}
	}

	return 0;
}
