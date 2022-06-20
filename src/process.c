#define PLAY_LOCK_OK 0    /* p->s and p->w are safe */
#define PLAY_LOCK_START 1 /* p->s and p->w want to be unsafe */
#define PLAY_LOCK_CONT 2  /* p->s and p->w are unsafe */
typedef struct
{
	song        *s;
	window      *w;
	jack_port_t *inl;
	jack_port_t *inr;
	jack_port_t *outl;
	jack_port_t *outr;
	char         dirty;
	char         lock;  /* PLAY_LOCK */
} playbackinfo;
playbackinfo *p;

typedef struct
{
	sample_t *inl;
	sample_t *inr;
	sample_t *outl;
	sample_t *outr;
} portbuffers;

int ifMacro(row r, char m)
{
	if (r.macroc[0] == m) return (int)r.macrov[0];
	if (r.macroc[1] == m) return (int)r.macrov[1];
	return -1;
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
	memset(cv->rampbuffer, 0, sizeof(sample_t) * cv->rampmax * 2);
	instrument *iv = p->s->instrumentv[realinstrument];
	cv->rampinstrument = realinstrument;
	cv->rampgain = cv->gain;
	for (uint16_t i = 0; i < cv->rampmax; i++)
	{
		if (!cv->r.note) break;
		t->f[iv->type].process(iv, cv, pointeroffset + i,
				&cv->rampbuffer[i * 2 + 0], &cv->rampbuffer[i * 2 + 1]);
	}
}

void _triggerNote(channel *cv, uint8_t note, uint8_t inst)
{
	if (note == 255) /* note off */
	{
		cv->releasepointer = cv->pointer;
	} else
	{
		cv->r.inst = inst;
		cv->r.note = note;
		cv->portamento = 0;
		cv->pointer = 0;
		cv->cents = 0.0;
		cv->gain = 255;
		cv->pointeroffset = 0;
		cv->releasepointer = 0;
	}
}
/* ramping */
void triggerNote(channel *cv, uint8_t note, uint8_t inst)
{
	if (cv->r.note && note != 255) /* old note, ramp it out */
	{
		instrument *iv = p->s->instrumentv[p->s->instrumenti[cv->r.inst]];
		if (!(p->w->instrumentlockv == INST_GLOBAL_LOCK_FREE
				&& p->w->instrumentlocki == p->s->instrumenti[cv->r.inst])
				&& cv->r.note && iv && iv->type < INSTRUMENT_TYPE_COUNT
				&& t->f[iv->type].process)
			ramp(p, cv, p->s->instrumenti[cv->r.inst], cv->pointer);
		cv->rampindex = 0; /* set this even if it's not populated */
	}
	_triggerNote(cv, note, inst);
}


/* inst==255 for off */
void playChannel(jack_nframes_t fptr, playbackinfo *p, portbuffers pb, channel *cv, uint8_t inst)
{
	instrument *iv= p->s->instrumentv[p->s->instrumenti[inst]];

	if (cv->cutsamples && p->s->sprp > cv->cutsamples)
	{
		ramp(p, cv, p->s->instrumenti[inst], cv->rtrigpointer + cv->rtrigsamples);
		cv->rampindex = 0;
		cv->r.note = 0;
		cv->cutsamples = 0;
	}
	if (cv->delaysamples && p->s->sprp > cv->delaysamples)
	{
		triggerNote(cv, cv->delaynote, cv->delayinst);
		cv->delaysamples = 0;
	}

	float l, r;
	/* process the type */
	if (!(p->w->instrumentlockv == INST_GLOBAL_LOCK_FREE && p->w->instrumentlocki == p->s->instrumenti[cv->r.inst])
			&& !cv->mute && cv->r.note && cv->r.note != 255 && iv && iv->type < INSTRUMENT_TYPE_COUNT && t->f[iv->type].process)
	{
		if (cv->rtrigsamples > 0 && inst < 255)
		{
			uint8_t oldnote = cv->r.note; /* in case the ramping freewheel cuts the note */
			uint32_t rtrigoffset = (cv->pointer - cv->rtrigpointer) % cv->rtrigsamples;
			if (rtrigoffset == 0 && cv->pointer > cv->rtrigpointer)
			{ /* first sample of any retrigger but the first */
				ramp(p, cv, p->s->instrumenti[inst], cv->rtrigpointer + cv->rtrigsamples);
				cv->rampindex = 0;
			}
			t->f[iv->type].process(iv, cv, cv->rtrigpointer + rtrigoffset, &l, &r);
			cv->r.note = oldnote;
		} else
			t->f[iv->type].process(iv, cv, cv->pointer, &l, &r);
		cv->pointer++;
	} else
	{
		l = r = 0.0;
	}

	if (!cv->mute && inst != 255)
	{
		/* mix in ramp data */
		if (cv->rampindex < cv->rampmax)
		{
			float rampgain = (float)cv->rampindex / (float)cv->rampmax;
			l *= rampgain;
			r *= rampgain;
			if (cv->rampinstrument)
			{
				instrument *jv = p->s->instrumentv[cv->rampinstrument];
				jv->outbufferl[fptr] += cv->rampbuffer[cv->rampindex * 2 + 0] * (1.0 - rampgain) * ((cv->rampgain>>4) / 16.0);
				jv->outbufferr[fptr] += cv->rampbuffer[cv->rampindex * 2 + 1] * (1.0 - rampgain) * ((cv->rampgain%16) / 16.0);
			}

			cv->rampindex++;
		}

		/* volume macro */
		l *= (cv->gain>>4) / 16.0;
		r *= (cv->gain%16) / 16.0;

		if (iv)
		{
			iv->outbufferl[fptr] += l;
			iv->outbufferr[fptr] += r;
		}
	}
}

void bendUp(channel *cv, uint32_t spr, uint32_t count)
{
	cv->cents += (12.0 / spr) * (cv->portamentospeed / 255.0) * count;
	while (cv->cents > 0.5)
	{
		cv->cents -= 1.0;
		cv->r.note++;
	}
	if (cv->r.note > cv->portamento || (cv->r.note == cv->portamento && cv->cents >= 0.0))
	{
		cv->cents = 0.0;
		cv->r.note = cv->portamento;
		cv->portamento = 0;
	}
}

void bendDown(channel *cv, uint32_t spr, uint32_t count)
{
	cv->cents -= (12.0 / spr) * (cv->portamentospeed / 255.0);
	while (cv->cents < 0.5)
	{
		cv->cents += 1.0;
		cv->r.note--;
	}
	if (cv->r.note <= cv->portamento && cv->cents <= 0.0)
	{
		cv->cents = 0.0;
		cv->r.note = cv->portamento;
		cv->portamento = 0;
	}
}

void preprocessRow(channel *cv, row r)
{
	int m;
	instrument *iv;

	m = ifMacro(r, 'B');
	if (m >= 32) // bpm
		changeBpm(p->s, m);

	m = ifMacro(r, 'C'); /* cut */
	if (m >= 0 && m>>4 == 0) /* note cut */
	{
		if (!(p->w->instrumentlockv == INST_GLOBAL_LOCK_FREE
				&& p->w->instrumentlocki == p->s->instrumenti[cv->r.inst]))
			ramp(p, cv, p->s->instrumenti[cv->r.inst], cv->pointer);
		cv->rampindex = 0;
		cv->r.note = 0;
	} else
	{
		if (m >= 0 && m%16 != 0) /* cut, don't divide by 0 */
			cv->cutsamples = p->s->spr * (float)(m>>4) / (float)(m%16);

		if (r.note)
		{
			m = ifMacro(r, 'P');
			if (m >= 0) // portamento
			{
				cv->portamento = r.note;
				cv->portamentospeed = m;
			} else
			{
				m = ifMacro(r, 'D'); /* delay */
				if (m >= 0 && m%16 != 0)
				{
					cv->delaysamples = p->s->spr * (float)(m>>4) / (float)(m%16);
					cv->delaynote = r.note;
					cv->delayinst = r.inst;
				} else
					triggerNote(cv, r.note, r.inst);
			}
		}
	}

	m = ifMacro(r, 'M');
	if (m >= 0) // volume
		cv->gain = m;

	m = ifMacro(r, 'R');
	if (m >= 0 && m%16 != 0) // retrigger
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

	/* type macros */
	/* only 1 allowed per row */
	uint8_t num;
	m = -1;

	if (isdigit(r.macroc[0]))
	{ num = r.macroc[0] - 48; m = r.macrov[0]; }
	else if (isdigit(r.macroc[1]))
	{ num = r.macroc[1] - 48; m = r.macrov[1]; }

	if (m >= 0)
	{
		iv = p->s->instrumentv[p->s->instrumenti[cv->r.inst]];
		if (iv && iv->type < INSTRUMENT_TYPE_COUNT && t->f[iv->type].macro)
			t->f[iv->type].macro(iv, cv, r, num, m);
	}



	cv->r.macroc[0] = r.macroc[0];
	cv->r.macrov[0] = r.macrov[0];
	cv->r.macroc[1] = r.macroc[1];
	cv->r.macrov[1] = r.macrov[1];
}

int process(jack_nframes_t nfptr, void *arg)
{
	playbackinfo *p = arg;
	channel *cv;
	instrument *iv;
	portbuffers pb;
	pb.inl =  jack_port_get_buffer(p->inl, nfptr);
	pb.inr =  jack_port_get_buffer(p->inr, nfptr);
	pb.outl = jack_port_get_buffer(p->outl, nfptr); memset(pb.outl, 0, nfptr * sizeof(sample_t));
	pb.outr = jack_port_get_buffer(p->outr, nfptr); memset(pb.outr, 0, nfptr * sizeof(sample_t));

	if (p->lock == PLAY_LOCK_START)
		p->lock = PLAY_LOCK_CONT;

	if (p->lock == PLAY_LOCK_CONT) return 0;

	/* clear instrument buffers */
	for (uint8_t i = 1; i < p->s->instrumentc; i++)
	{
		iv = p->s->instrumentv[i];
		memset(iv->outbufferl, 0, nfptr * sizeof(sample_t));
		memset(iv->outbufferr, 0, nfptr * sizeof(sample_t));
	}


	/* just need to know these have been seen */
	/* will no longer write to the record buffer */
	if (p->w->instrumentrecv == INST_REC_LOCK_PREP_END)
		p->w->instrumentrecv = INST_REC_LOCK_END;
	/* will no longer access the instrument state */
	if (p->w->instrumentlockv == INST_GLOBAL_LOCK_PREP_FREE)
		p->w->instrumentlockv = INST_GLOBAL_LOCK_FREE;
	if (p->w->instrumentlockv == INST_GLOBAL_LOCK_PREP_HIST)
		p->w->instrumentlockv = INST_GLOBAL_LOCK_HIST;


	if (p->w->previewtrigger) switch (p->w->previewtrigger)
	{
		case 1: // start instrument preview
			channel *cv = &p->s->channelv[p->w->previewchannel];
			triggerNote(cv, p->w->previewnote, p->w->previewinst);
			p->w->previewtrigger++;
			break;
		case 3: // start sample preview
			// _triggerNote(&p->w->previewchannelplay, p->w->previewnote, p->w->previewinst);
			p->w->previewtrigger++;
			break;
		case 5: // unload sample
			free(p->w->previewinstrument.sampledata);
			p->w->previewinstrument.sampledata = NULL;
			break;

		case 4: // continue sample preview
			/* for (jack_nframes_t fptr = 0; fptr < nfptr; fptr++)
				playChannel(fptr, p, pb, &p->w->previewchannelplay, 255); */
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
			cv->rtrigpointer = 0;
			cv->rtrigblocksize = 0;
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
							if (cv->cents < 0.0) bendUp(cv, p->s->spr, p->s->spr);
							else                 bendDown(cv, p->s->spr, p->s->spr);
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

		p->s->playing = PLAYING_CONT;
	} else if (p->s->playing == PLAYING_PREP_STOP)
	{
		/* stop channels */
		for (uint8_t i = 0; i < p->s->channelc; i++)
		{
			if (!(p->w->instrumentlockv == INST_GLOBAL_LOCK_FREE
					&& p->w->instrumentlocki == p->s->instrumenti[p->s->channelv[i].r.inst]))
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
		if (p->s->playing == PLAYING_CONT)
		{
			/* record */
			if (p->w->instrumentrecv == INST_REC_LOCK_CONT)
			{
				if (p->w->recptr + 1 > RECORD_LENGTH * samplerate)
				{
					strcpy(p->w->command.error, "record buffer full");
					p->w->instrumentrecv = INST_REC_LOCK_END;
				} else
				{
					int c;
					c = (float)pb.inl[fptr] * (float)SHRT_MAX;
					if      (c > SHRT_MAX) p->w->recbuffer[p->w->recptr * 2 + 0] = SHRT_MAX;
					else if (c < SHRT_MIN) p->w->recbuffer[p->w->recptr * 2 + 0] = SHRT_MIN;
					else                   p->w->recbuffer[p->w->recptr * 2 + 0] = c;
					c = (float)pb.inr[fptr] * (float)SHRT_MAX;
					if      (c > SHRT_MAX) p->w->recbuffer[p->w->recptr * 2 + 1] = SHRT_MAX;
					else if (c < SHRT_MIN) p->w->recbuffer[p->w->recptr * 2 + 1] = SHRT_MIN;
					else                   p->w->recbuffer[p->w->recptr * 2 + 1] = c;
					p->w->recptr++;
				}
			}

			/* preprocess the channel */
			if (p->s->sprp == 0)
				for (uint8_t c = 0; c < p->s->channelc; c++)
					preprocessRow(&p->s->channelv[c],
							p->s->patternv[p->s->patterni[p->s->songi[p->s->songp]]]->rowv[c][p->s->songr]);
		}

		for (uint8_t c = 0; c < p->s->channelc; c++)
		{
			cv = &p->s->channelv[c];
			if (p->s->playing == PLAYING_CONT && cv->portamento)
			{
				if (cv->r.note == cv->portamento)
				{ /* fine bend */
					if (cv->cents < 0.0) bendUp(cv, p->s->spr, 1);
					else                 bendDown(cv, p->s->spr, 1);
				} else if (cv->r.note < cv->portamento)
				{ /* bend up */
					bendUp(cv, p->s->spr, 1);
				} else
				{ /* bend down */
					bendDown(cv, p->s->spr, 1);
				}
			}
			playChannel(fptr, p, pb, cv, cv->r.inst);
		}

		/* next row */
		if (p->s->playing == PLAYING_CONT && p->s->sprp++ > p->s->spr)
		{
			p->s->sprp = 0;
			/* next pattern */
			if (p->s->songr++ >= p->s->patternv[p->s->patterni[p->s->songi[p->s->songp]]]->rowc)
			{
				p->s->songr = 0;
				if (p->w->songfx == p->s->songp)
					p->w->trackerfy = 0;
				p->dirty = 1;
				
				if (p->w->songnext)
				{
					if (p->w->songfx == p->s->songp)
						p->w->songfx = p->w->songnext - 1;
					p->dirty = 1;

					p->s->songp = p->w->songnext - 1;
					p->w->songnext = 0;
				} else if (p->s->songa[p->s->songp]) /* loop */
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

					if (p->w->songfx == p->s->songp)
						p->w->songfx = blockstart;
					p->dirty = 1;

					p->s->songp = blockstart;
				} else
				{
					if (p->w->songfx == p->s->songp)
						p->w->songfx++;
					p->dirty = 1;

					p->s->songp++;
				}
			} else
			{
				if (p->w->songfx == p->s->songp)
					p->w->trackerfy = p->s->songr;
				p->dirty = 1;
			}
		}
	}

	/* final mixdown */
	for (uint8_t i = 1; i < p->s->instrumentc; i++)
	{
		iv = p->s->instrumentv[i];

		/* fader and output */
		for (jack_nframes_t fptr = 0; fptr < nfptr; fptr++)
		{
			pb.outl[fptr] += iv->outbufferl[fptr] * (iv->fader>>4) / 16.0;
			pb.outr[fptr] += iv->outbufferr[fptr] * (iv->fader%16) / 16.0;
		}
	}
	/* master output volume */
	for (jack_nframes_t fptr = 0; fptr < nfptr; fptr++)
	{
		pb.outl[fptr] *= 0.25;
		pb.outr[fptr] *= 0.25;
	}

	return 0;
}
