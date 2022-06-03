typedef struct
{
	song *s;
	window *w;
	jack_port_t *inl;
	jack_port_t *inr;
	jack_port_t *outl;
	jack_port_t *outr;
	char dirty;
} playbackinfo;

typedef struct
{
	float *inl;
	float *inr;
	float *outl;
	float *outr;
} portbuffers;

int ifMacro(row r, char m)
{
	if (r.macroc[0] == m) return (int)r.macrov[0];
	if (r.macroc[1] == m) return (int)r.macrov[1];
	return -1;
}

void changeBpm(song *s, uint16_t newbpm)
{
	s->bpm = newbpm;
	s->spr = samplerate * (60.0 / newbpm) / 4;
}

void playChannel(jack_nframes_t fptr, playbackinfo *p, portbuffers pb, channel *cv, instrument *iv)
{
	/* process the type */
	if (cv->r.note && iv && iv->type < INSTRUMENT_TYPE_COUNT && t->f[iv->type].process != NULL)
	{
		if (cv->rtrigsamples > 0)
			t->f[iv->type].process(iv, cv, cv->rtrigpointer + (cv->samplepointer - cv->rtrigpointer) % cv->rtrigsamples,
					p->s->effectoutl, p->s->effectoutr);
		else
			t->f[iv->type].process(iv, cv, cv->samplepointer,
					p->s->effectoutl, p->s->effectoutr);
		cv->samplepointer++;
	} else
	{
		*p->s->effectoutl = 0.0;
		*p->s->effectoutr = 0.0;
	}

	/* mix in ramp data */
	if (cv->rampindex < cv->rampmax)
	{
		float rampgain = 1.0 - (float)cv->rampindex / (float)cv->rampmax;
		*p->s->effectoutl += cv->rampbuffer[cv->rampindex * 2 + 0] * rampgain;
		*p->s->effectoutr += cv->rampbuffer[cv->rampindex * 2 + 1] * rampgain;

		cv->rampindex++;
	}

	/* process the effect sends */
	if (iv)
	{
		for (int j = 0; j < 16; j++)
			if (iv->processsend[j] && p->s->effectv[j].type > 0)
			{
				*p->s->effectinl = *p->s->effectoutl * (iv->processsend[j] - 1) / 15.0;
				*p->s->effectinr = *p->s->effectoutr * (iv->processsend[j] - 1) / 15.0;
				lilv_instance_run(iv->plugininstance[j], 1); // TODO: call this bufferwise
			}
		*p->s->effectoutl *= iv->fader[0] / 256.0;
		*p->s->effectoutr *= iv->fader[1] / 256.0;
	}

	pb.outl[fptr] += *p->s->effectoutl * ((cv->gain>>4) / 16.0);
	pb.outr[fptr] += *p->s->effectoutr * ((cv->gain%16) / 16.0);
}

void bendUp(channel *cv)
{
	cv->cents += PORTAMENTO_SEMITONES * cv->portamentospeed;
	while (cv->cents > 0.5)
	{
		cv->cents -= 1.0;
		cv->r.note++;
	}
	if (cv->r.note >= cv->portamento && cv->cents >= 0.0)
	{
		cv->cents = 0.0;
		cv->r.note = cv->portamento;
		cv->portamento = 0;
	}
}

void bendDown(channel *cv)
{
	cv->cents -= PORTAMENTO_SEMITONES * cv->portamentospeed;
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


int process(jack_nframes_t nfptr, void *arg)
{
	playbackinfo *p = arg;
	channel *cv;
	instrument *iv;
	row r;
	int m;
	portbuffers pb;
	pb.inl = jack_port_get_buffer(p->inl, nfptr);
	pb.inr = jack_port_get_buffer(p->inr, nfptr);
	pb.outl = jack_port_get_buffer(p->outl, nfptr); memset(pb.outl, 0, nfptr * sizeof(sample_t));
	pb.outr = jack_port_get_buffer(p->outr, nfptr); memset(pb.outr, 0, nfptr * sizeof(sample_t));


	/* just need to know this has been seen */
	/* will no longer write to the record buffer */
	if (p->w->instrumentrecv == INST_REC_LOCK_PREP_END)
		p->w->instrumentrecv = INST_REC_LOCK_END;


	if (p->w->previewchanneltrigger) switch (p->w->previewchanneltrigger)
	{
		case 1: // start instrument preview
			if (!p->s->instrumentv[p->s->instrumenti[p->w->previewchannel.r.inst]])
			{
				p->w->previewchanneltrigger = 0;
				break;
			}
		case 3: // start sample preview
			p->w->previewchannel.portamento = 0;
			p->w->previewchannel.samplepointer = 0;
			p->w->previewchannel.cents = 0.0;
			p->w->previewchannel.gain = 255;
			p->w->previewchannel.sampleoffset = 0;
			p->w->previewchannel.releasepointer = 0;
			p->w->previewchanneltrigger++;
			break;
		case 5: // unload sample
			free(p->w->previewinstrument.sampledata);
			break;

		case 4: // continue sample preview
			for (jack_nframes_t fptr = 0; fptr < nfptr; fptr++)
				playChannel(fptr, p, pb, &p->w->previewchannel,
						&p->w->previewinstrument);
			break;
		case 2: // continue instrument preview
			for (jack_nframes_t fptr = 0; fptr < nfptr; fptr++)
				playChannel(fptr, p, pb, &p->w->previewchannel,
						p->s->instrumentv[p->s->instrumenti[p->w->previewchannel.r.inst]]);
			break;
	}


	/* start/stop playback */
	if (p->s->playing == PLAYING_START)
	{
		changeBpm(p->s, p->s->songbpm);
		p->s->sprp = 0;

		/* activate plugin instances */
		for (int i = 1; i < p->s->instrumentc; i++)
		{
			iv = p->s->instrumentv[i];
			for (int j = 0; j < 16; j++)
			{
				iv->processsend[j] = iv->send[j];
				if (iv->send[j] && p->s->effectv[j].type > 0)
					lilv_instance_activate(iv->plugininstance[j]);
			}
		}

		/* clear the channels */
		for (uint8_t c = 0; c < p->s->channelc; c++)
		{
			cv = &p->s->channelv[c];
			cv->r.note = 0;
		}

		p->s->playing = PLAYING_CONT;
	} else if (p->s->playing == PLAYING_PREP_STOP)
	{
		/* deactivate plugin instances */
		for (int i = 1; i < p->s->instrumentc; i++)
		{
			iv = p->s->instrumentv[i];
			for (int j = 0; j < 16; j++)
			{
				if (iv->processsend[j] && p->s->effectv[j].type > 0)
					lilv_instance_deactivate(iv->plugininstance[j]);
			}
		}

		p->s->playing = PLAYING_STOP;
		p->dirty = 1;
	}

	if (p->w->request == REQ_BPM)
	{
		changeBpm(p->s, p->s->songbpm);
		p->w->request = REQ_OK;
	}


	if (p->s->playing == PLAYING_CONT)
	{
		/* loop over samples */
		for (jack_nframes_t fptr = 0; fptr < nfptr; fptr++)
		{
			/* record */
			if (p->w->instrumentrecv == INST_REC_LOCK_CONT)
			{
				if (p->w->recptr + 2 > RECORD_LENGTH)
				{
					strcpy(p->w->command.error, "record buffer full");
					p->w->instrumentrecv = INST_REC_LOCK_END;
				} else
				{
					p->w->recbuffer[p->w->recptr + 0] = (float)pb.inl[fptr] * (float)SHRT_MAX;
					p->w->recbuffer[p->w->recptr + 1] = (float)pb.inr[fptr] * (float)SHRT_MAX;
					p->w->recptr += 2;
				}
			}

			/* preprocess the channel */
			if (p->s->sprp == 0)
			{
				for (uint8_t c = 0; c < p->s->channelc; c++)
				{
					r = p->s->patternv[p->s->patterni[p->s->songi[p->s->songp]]]->rowv[c][p->s->songr];
					cv = &p->s->channelv[c];

					if (r.note) switch (r.note)
					{
						case 255:
							cv->releasepointer = cv->samplepointer;
							break;
						case 254:
							iv = p->s->instrumentv[p->s->instrumenti[cv->r.inst]];
							if (cv->r.note && iv && iv->type < INSTRUMENT_TYPE_COUNT && t->f[iv->type].process != NULL)
							{
								/* freewheel to fill up the ramp buffer, potential speed issues? */
								for (uint16_t i = 0; i < cv->rampmax; i++)
								{
									if (!cv->r.note) break;
									t->f[iv->type].process(iv, cv, cv->samplepointer,
											&cv->rampbuffer[i * 2 + 0], &cv->rampbuffer[i * 2 + 1]);
									cv->samplepointer++;
								}
								cv->rampindex = 0;
							}
							cv->r.note = 0;
							break;
						default:
							m = ifMacro(r, 'P');
							if (m >= 0) // portamento
							{
								cv->portamento = r.note;
								cv->portamentospeed = m;
							} else
							{
								if (cv->r.note) /* old note, ramp it out */
								{
									iv = p->s->instrumentv[p->s->instrumenti[cv->r.inst]];
									if (cv->r.note && iv && iv->type < INSTRUMENT_TYPE_COUNT && t->f[iv->type].process != NULL)
									{
										/* freewheel to fill up the ramp buffer, potential speed issues? */
										for (uint16_t i = 0; i < cv->rampmax; i++)
										{
											if (!cv->r.note) break;
											t->f[iv->type].process(iv, cv, cv->samplepointer,
													&cv->rampbuffer[i * 2 + 0], &cv->rampbuffer[i * 2 + 1]);
											cv->samplepointer++;
										}
										cv->rampindex = 0;
									}
								}
								cv->r.inst = r.inst;
								cv->r.note = r.note;
								cv->portamento = 0;
								cv->samplepointer = 0;
								cv->cents = 0.0;
								cv->gain = 255;
								cv->sampleoffset = 0;
								cv->releasepointer = 0;
							}
							break;
					}

					m = ifMacro(r, 'M');
					if (m >= 0) // volume
						cv->gain = m;

					m = ifMacro(r, 'O');
					if (m >= 0) // offset
					{
						iv = p->s->instrumentv[p->s->instrumenti[cv->r.inst]];
						if (iv && iv->type < INSTRUMENT_TYPE_COUNT && t->f[iv->type].offset != NULL)
						{
							cv->sampleoffset = t->f[iv->type].offset(iv, cv, m);
							cv->samplepointer = 0;
						}
					}

					m = ifMacro(r, 'R');
					if (m >= 1) // retrigger
					{
						cv->rtrigpointer = cv->samplepointer;
						cv->rtrigsamples = p->s->spr / m;
					} else cv->rtrigsamples = 0;

					m = ifMacro(r, '1');
					if (m >= 0) // effect mix
					{
						iv = p->s->instrumentv[p->s->instrumenti[r.inst]];
						if (iv)
						{
							/* off to on */
							if      (!iv->processsend[cv->effectholdindex] && m%16)
								lilv_instance_activate(iv->plugininstance[    m>>4]);
							/* on to off */
							else if (iv->processsend[cv->effectholdindex] && !m%16)
								lilv_instance_deactivate(iv->plugininstance[  m>>4]);

							iv->processsend[m>>4] = m%16;
							cv->effectholdinst = r.inst;
							cv->effectholdindex =   m>>4;
						}
					} else if (cv->effectholdinst < 255) /* reset the state */
					{
						iv = p->s->instrumentv[p->s->instrumenti[cv->effectholdinst]];
						if (iv)
						{
							/* off to on */
							if      (!iv->processsend[cv->effectholdindex] && iv->send[cv->effectholdindex])
								lilv_instance_activate(iv->plugininstance[cv->effectholdindex]);
							/* on to off */
							else if (iv->processsend[cv->effectholdindex] && !iv->send[cv->effectholdindex])
								lilv_instance_deactivate(iv->plugininstance[cv->effectholdindex]);

							iv->processsend[cv->effectholdindex] = iv->send[cv->effectholdindex];
						}
						cv->effectholdinst = 255;
					}

					cv->r.macroc[0] = r.macroc[0];
					cv->r.macrov[0] = r.macrov[0];
					cv->r.macroc[1] = r.macroc[1];
					cv->r.macrov[1] = r.macrov[1];
				}
			}


			for (uint8_t c = 0; c < p->s->channelc; c++)
			{
				cv = &p->s->channelv[c];
				iv = p->s->instrumentv[p->s->instrumenti[cv->r.inst]];

				if (cv->portamento)
				{
					if (cv->r.note == cv->portamento)
					{ /* fine bend */
						if (cv->cents < 0.0) bendUp(cv);
						else                 bendDown(cv);
					} else if (cv->r.note < cv->portamento)
					{ /* bend up */
						bendUp(cv);
					} else
					{ /* bend down */
						bendDown(cv);
					}
				}

				playChannel(fptr, p, pb, cv, iv);
			}


			/* next row */
			if (p->s->sprp++ > p->s->spr)
			{
				p->s->sprp = 0;
				/* next pattern */
				if (p->s->songr++ >= p->s->patternv[p->s->patterni[p->s->songi[p->s->songp]]]->rowc)
				{
					p->s->songr = 0;
					if (p->w->songfx == p->s->songp)
					{
						p->w->trackerfy = 0;
						p->dirty = 1;
					}
					
					if (p->s->songa[p->s->songp]) /* loop */
					{
						/* do nothing, don't inc the song pointer */
					} else if (p->w->songnext)
					{
						p->s->songp = p->w->songnext;
						p->w->songnext = 0;
					} else if (p->s->songi[p->s->songp + 1] == 255)
					{ /* no next pattern, go to the beginning of the block */
						uint8_t blockstart = 0;
						for (uint8_t i = p->s->songp; i > 0; i--)
							if (p->s->songi[i] == 255)
							{
								blockstart = i;
								break;
							}

						if (p->w->songfx == p->s->songp)
						{
							p->w->songfx = blockstart;
							p->dirty = 1;
						}
						p->s->songp = blockstart;
					} else
					{
						if (p->w->songfx == p->s->songp)
						{
							p->w->songfx++;
							p->dirty = 1;
						}
						p->s->songp++;
					}
				} else
					if (p->w->songfx == p->s->songp)
					{
						p->w->trackerfy = p->s->songr;
						p->dirty = 1;
					}
			}
		}
	}

	return 0;
}
