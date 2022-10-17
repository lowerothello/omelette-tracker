short tfxToVfx(short tfx) { if (tfx > 1) return 2 + (tfx - 2) / 2; return tfx; }
short vfxToTfx(short vfx) { if (vfx > 1) return 2 + (vfx - 2) * 2; return vfx; }

void yankPartPattern(short x1, short x2, short y1, short y2, uint8_t c1, uint8_t c2)
{
	/* walk over allocated channels and free them */
	for (short i = 0; i < w->pbchannelc; i++)
		free(w->pbvariantv[i]);

	w->pbfx[0] = x1;
	w->pbfx[1] = x2;
	w->pbchannelc = c2-c1 + 1;

	ChannelData *cd;
	for (uint8_t i = 0; i <= c2-c1; i++)
	{
		cd = &s->channel->v[c1+i].data;
		w->pbvariantv[i] = _copyVariant(NULL, y2-y1 + 1);
		for (uint16_t j = 0; j <= y2-y1; j++)
			w->pbvariantv[i]->rowv[j] = *getChannelRow(cd, y1+j);
	}
}
void putPartPattern(void)
{
	uint8_t i, j;
	int k;
	Row *dest, *src;
	unsigned char targetmacro;
	ChannelData *cd = &s->channel->v[w->channel].data;

	if (!w->pbchannelc) return;
	if (w->pbchannelc == 1) // only one channel
	{
		if (w->pbfx[0] > 1 && w->pbfx[0] == w->pbfx[1]) // just one macro column
		{
			if (w->trackerfx < 2) targetmacro = 0;
			else targetmacro = tfxToVfx(w->trackerfx)-2;

			for (j = 0; j < w->pbvariantv[0]->rowc; j++)
			{
				if (w->trackerfy + j >= s->songlen) break;
				dest = getChannelRow(cd, w->trackerfy+j);
				src = getVariantRow(w->pbvariantv[0], j);
				dest->macro[targetmacro].c = src->macro[w->pbfx[0]-2].c;
				dest->macro[targetmacro].v = src->macro[w->pbfx[0]-2].v;
			}
			w->trackerfx = vfxToTfx(targetmacro + 2);
		} else
		{
			for (j = 0; j < w->pbvariantv[0]->rowc; j++)
			{
				if (w->trackerfy + j >= s->songlen) break;
				dest = getChannelRow(cd, w->trackerfy+j);
				src = getVariantRow(w->pbvariantv[0], j);
				if (w->pbfx[0] <= 0 && w->pbfx[1] >= 0) dest->note = src->note;
				if (w->pbfx[0] <= 1 && w->pbfx[1] >= 1) dest->inst = src->inst;
				for (k = 0; k <= cd->macroc; k++)
					if (w->pbfx[0] <= k+2 && w->pbfx[1] >= k+2)
					{
						dest->macro[k].c = src->macro[k].c;
						dest->macro[k].v = src->macro[k].v;
					}
			} w->trackerfx = vfxToTfx(w->pbfx[0]);
		}
	} else
	{
		for (i = 0; i < w->pbchannelc; i++)
		{
			if (w->channel+i < s->channel->c)
				for (j = 0; j < w->pbvariantv[i]->rowc; j++)
				{
					if (w->trackerfy + j >= s->songlen) break;
					dest = getChannelRow(cd, w->trackerfy+j);
					src = getVariantRow(w->pbvariantv[i], j);

					if (i == 0) // first channel
					{
						if (w->pbfx[0] <= 0) dest->note = src->note;
						if (w->pbfx[0] <= 1) dest->inst = src->inst;
						for (k = 0; k <= cd->macroc; k++)
							if (w->pbfx[0] <= k+2)
							{
								dest->macro[k].c = src->macro[k].c;
								dest->macro[k].v = src->macro[k].v;
							}
					} else if (i == w->pbchannelc-1) // last channel
					{
						if (w->pbfx[1] >= 0) dest->note = src->note;
						if (w->pbfx[1] >= 1) dest->inst = src->inst;
						for (k = 0; k <= cd->macroc; k++)
							if (w->pbfx[1] >= k+2)
							{
								dest->macro[k].c = src->macro[k].c;
								dest->macro[k].v = src->macro[k].v;
							}
					} else *dest = *src;
				}
			else break;
		} w->trackerfx = vfxToTfx(w->pbfx[0]);
	}
	regenGlobalRowc(s);
}
void yankPartVtrig(short y1, short y2, uint8_t c1, uint8_t c2)
{
	for (short i = 0; i < w->vbchannelc; i++)
		free(w->vbtrig[i]);

	w->vbchannelc = c2-c1 + 1;
	w->vbrowc = y2-y1 + 1;

	for (uint8_t i = 0; i <= c2-c1; i++)
	{
		w->vbtrig[i] = calloc(y2-y1 + 1, sizeof(Vtrig));
		for (uint16_t j = 0; j <= y2-y1; j++)
			w->vbtrig[i][j] = s->channel->v[c1+i].data.trig[y1+j];
	}
}
void putPartVtrig(void)
{
	for (uint8_t i = 0; i < w->vbchannelc; i++)
		if (w->channel+i < s->channel->c)
			for (uint8_t j = 0; j < w->vbrowc; j++)
			{
				if (w->trackerfy + j >= s->songlen) break;
				s->channel->v[w->channel+i].data.trig[w->trackerfy+j] = w->vbtrig[i][j];
			}
		else break;
	regenGlobalRowc(s);
}
void mixPutPartVtrig(void)
{
	for (uint8_t i = 0; i < w->vbchannelc; i++)
		if (w->channel+i < s->channel->c)
			for (uint8_t j = 0; j < w->vbrowc; j++)
			{
				if (w->trackerfy + j >= s->songlen) break;
				if (w->vbtrig[i][j].index != VARIANT_VOID)
					s->channel->v[w->channel+i].data.trig[w->trackerfy+j] = w->vbtrig[i][j];
			}
		else break;
	regenGlobalRowc(s);
}
void mixPutPartPattern(void)
{
	uint8_t i;
	int k;
	Row *dest, *src;
	if (!w->pbchannelc) return;
	if (w->pbchannelc == 1) // only one channel
	{
		if (w->pbfx[0] > 1 && w->pbfx[0] == w->pbfx[1]) // just one macro column
		{
			unsigned char targetmacro;
			if (w->trackerfx < 2) targetmacro = 0;
			else targetmacro = tfxToVfx(w->trackerfx)-2;
			for (uint8_t j = 0; j < w->pbvariantv[0]->rowc; j++)
			{
				if (w->trackerfy + j >= s->songlen) break;
				dest = getChannelRow(&s->channel->v[w->channel].data, w->trackerfy+j);
				src = getVariantRow(w->pbvariantv[0], j);
				if (src->macro[w->pbfx[0]-2].c)
				{
					dest->macro[targetmacro].c = src->macro[w->pbfx[0]-2].c;
					dest->macro[targetmacro].v = src->macro[w->pbfx[0]-2].v;
				}
			}
			w->trackerfx = vfxToTfx(targetmacro + 2);
			w->trackerfx = vfxToTfx(targetmacro + 2);
		} else
		{
			for (uint8_t j = 0; j < w->pbvariantv[0]->rowc; j++)
			{
				if (w->trackerfy + j >= s->songlen) break;
				dest = getChannelRow(&s->channel->v[w->channel].data, w->trackerfy+j);
				src = getVariantRow(w->pbvariantv[0], j);
				if (w->pbfx[0] <= 0 && w->pbfx[1] >= 0 && src->note != NOTE_VOID) dest->note = src->note;
				if (w->pbfx[0] <= 1 && w->pbfx[1] >= 1 && src->inst != INST_VOID) dest->inst = src->inst;
				for (k = 0; k <= s->channel->v[w->channel].data.macroc; k++)
					if (w->pbfx[0] <= k+2 && w->pbfx[1] >= k+2 && src->macro[k].c)
					{
						dest->macro[k].c = src->macro[k].c;
						dest->macro[k].v = src->macro[k].v;
					}
			} w->trackerfx = vfxToTfx(w->pbfx[0]);
		}
	} else
	{
		for (i = 0; i < w->pbchannelc; i++)
		{
			if (w->channel+i < s->channel->c)
				for (uint8_t j = 0; j < w->pbvariantv[i]->rowc; j++)
				{
					if (w->trackerfy + j >= s->songlen) break;
					dest = getChannelRow(&s->channel->v[w->channel+i].data, w->trackerfy+j);
					src = getVariantRow(w->pbvariantv[i], j);

					if (i == 0) // first channel
					{
						if (w->pbfx[0] <= 0 && src->note != NOTE_VOID) dest->note = src->note;
						if (w->pbfx[0] <= 1 && src->inst != INST_VOID) dest->inst = src->inst;
						for (k = 0; k <= s->channel->v[w->channel+i].data.macroc; k++)
							if (w->pbfx[0] <= k+2 && src->macro[k].c)
							{
								dest->macro[k].c = src->macro[k].c;
								dest->macro[k].v = src->macro[k].v;
							}
					} else if (i == w->pbchannelc-1) // last channel
					{
						if (w->pbfx[1] >= 0 && src->note != NOTE_VOID) dest->note = src->note;
						if (w->pbfx[1] >= 1 && src->inst != INST_VOID) dest->inst = src->inst;
						for (k = 0; k <= s->channel->v[w->channel+i].data.macroc; k++)
							if (w->pbfx[1] >= k+2 && src->macro[k].c)
							{
								dest->macro[k].c = src->macro[k].c;
								dest->macro[k].v = src->macro[k].v;
							}
					} else // middle channel
					{
						if (src->note != NOTE_VOID) dest->note = src->note;
						if (src->inst != INST_VOID) dest->inst = src->inst;
						for (k = 0; k <= s->channel->v[w->channel+i].data.macroc; k++)
							if (src->macro[k].c)
							{
								dest->macro[k].c = src->macro[k].c;
								dest->macro[k].v = src->macro[k].v;
							}
					}
				}
			else break;
		} w->trackerfx = vfxToTfx(w->pbfx[0]);
	}
	regenGlobalRowc(s);
}
void delPartPattern(short x1, short x2, short y1, short y2, uint8_t c1, uint8_t c2)
{
	uint8_t i, j;
	Row *r;
	int k;
	if (c1 == c2) /* only one channel */
	{
		for (j = y1; j <= y2; j++)
		{
			r = getChannelRow(&s->channel->v[c1].data, j);
			if (x1 <= 0 && x2 >= 0) r->note = NOTE_VOID;
			if (x1 <= 1 && x2 >= 1) r->inst = INST_VOID;
			for (k = 0; k <= s->channel->v[c1].data.macroc; k++)
				if (x1 <= k+2 && x2 >= k+2)
				{
					r->macro[k].c = 0;
					r->macro[k].v = 0;
				}
		}
	} else
		for (i = c1; i <= c2; i++)
			if (i < s->channel->c)
				for (j = y1; j <= y2; j++)
				{
					r = getChannelRow(&s->channel->v[i].data, j);
					if (i == c1) /* first channel */
					{
						if (x1 <= 0) r->note = NOTE_VOID;
						if (x1 <= 1) r->inst = INST_VOID;
						for (k = 0; k <= s->channel->v[i].data.macroc; k++)
							if (x1 <= k+2)
							{
								r->macro[k].c = 0;
								r->macro[k].v = 0;
							}
					} else if (i == c2) /* last channel */
					{
						if (x2 >= 0) r->note = NOTE_VOID;
						if (x2 >= 1) r->inst = INST_VOID;
						for (k = 0; k <= s->channel->v[i].data.macroc; k++)
							if (x2 >= k+2)
							{
								r->macro[k].c = 0;
								r->macro[k].v = 0;
							}
					} else /* middle channel */
					{
						memset(r, 0, sizeof(Row));
						r->note = NOTE_VOID;
						r->inst = INST_VOID;
					}
				}
			else break;
	regenGlobalRowc(s);
}
void delPartVtrig(short y1, short y2, uint8_t c1, uint8_t c2)
{
	for (uint8_t i = c1; i <= c2; i++)
		if (i < s->channel->c)
			for (uint8_t j = y1; j <= y2; j++)
				setChannelTrig(&s->channel->v[i].data, j, VARIANT_VOID);
		else break;
}
void loopPartVtrig(short y1, short y2, uint8_t c1, uint8_t c2)
{
	for (uint8_t i = c1; i <= c2; i++)
		if (i < s->channel->c)
			for (uint8_t j = y1; j <= y2; j++)
			{
				if (s->channel->v[i].data.trig[j].index != VARIANT_VOID)
					s->channel->v[i].data.trig[j].flags ^= C_VTRIG_LOOP;
			}
		else break;
	regenGlobalRowc(s);
}
/* block inc/dec */
void addPartPattern(signed char value, short x1, short x2, short y1, short y2, uint8_t c1, uint8_t c2)
{
	uint8_t i, j;
	int k;
	Row *r;
	if (c1 == c2) /* only one channel */
	{
		for (j = y1; j <= y2; j++)
		{
			r = getChannelRow(&s->channel->v[c1].data, j);
			if (x1 <= 0 && x2 >= 0) r->note += value;
			if (x1 <= 1 && x2 >= 1) r->inst += value;
			for (k = 0; k <= s->channel->v[c1].data.macroc; k++)
				if (x1 <= k+2 && x2 >= k+2)
					switch (r->macro[k].c)
					{
						case 'G': case 'g': case 'K': case 'k': r->macro[k].v += value*16;
						default:                                r->macro[k].v += value;
					}
		}
	} else
		for (i = c1; i <= c2; i++)
			if (i < s->channel->c)
				for (j = y1; j <= y2; j++)
				{
					r = getChannelRow(&s->channel->v[i].data, j);
					if (i == c1) /* first channel */
					{
						if (x1 <= 0) r->note += value;
						if (x1 <= 1) r->inst += value;
						for (k = 0; k <= s->channel->v[i].data.macroc; k++)
							if (x1 <= k+2)
								switch (r->macro[k].c)
								{
									case 'G': case 'g': case 'K': case 'k': r->macro[k].v += value*16;
									default:                                r->macro[k].v += value;
								}
					} else if (i == c2) /* last channel */
					{
						if (x2 >= 0) r->note += value;
						if (x2 >= 1) r->inst += value;
						for (k = 0; k <= s->channel->v[i].data.macroc; k++)
							if (x2 >= k+2)
								switch (r->macro[k].c)
								{
									case 'G': case 'g': case 'K': case 'k': r->macro[k].v += value*16;
									default:                                r->macro[k].v += value;
								}
					} else /* middle channel */
					{
						r->note += value;
						r->inst += value;
						for (k = 0; k <= s->channel->v[i].data.macroc; k++)
							switch (r->macro[k].c)
							{
								case 'G': case 'g': case 'K': case 'k': r->macro[k].v += value*16;
								default:                                r->macro[k].v += value;
							}
					}
				}
			else break;
	regenGlobalRowc(s);
}
/* block toggle case */
void tildePartPattern(short x1, short x2, short y1, short y2, uint8_t c1, uint8_t c2)
{
	uint8_t i, j;
	int k;
	Row *r;
	if (c1 == c2) /* only one channel */
	{
		for (j = y1; j <= y2; j++)
		{
			r = getChannelRow(&s->channel->v[c1].data, j);
			for (k = 0; k <= s->channel->v[c1].data.macroc; k++)
				if (x1 <= k+2 && x2 >= k+2)
				{
					if      (isupper(r->macro[k].c)) changeMacro(r->macro[k].c, &r->macro[k].c);
					else if (islower(r->macro[k].c)) changeMacro(r->macro[k].c, &r->macro[k].c);
				}
		}
	} else
		for (i = c1; i <= c2; i++)
			if (i < s->channel->c)
				for (j = y1; j <= y2; j++)
				{
					r = getChannelRow(&s->channel->v[i].data, j);
					if (i == c1) /* first channel */
						for (k = 0; k <= s->channel->v[i].data.macroc; k++) {
							if (x1 <= k+2)
							{
								if      (isupper(r->macro[k].c)) changeMacro(r->macro[k].c, &r->macro[k].c);
								else if (islower(r->macro[k].c)) changeMacro(r->macro[k].c, &r->macro[k].c);
							}}
					else if (i == c2) /* last channel */
						for (k = 0; k <= s->channel->v[i].data.macroc; k++) {
							if (x2 >= k+2)
							{
								if      (isupper(r->macro[k].c)) changeMacro(r->macro[k].c, &r->macro[k].c);
								else if (islower(r->macro[k].c)) changeMacro(r->macro[k].c, &r->macro[k].c);
							}}
					else /* middle channel */
						for (k = 0; k <= s->channel->v[i].data.macroc; k++)
						{
							if      (isupper(r->macro[k].c)) changeMacro(r->macro[k].c, &r->macro[k].c);
							else if (islower(r->macro[k].c)) changeMacro(r->macro[k].c, &r->macro[k].c);
						}
				}
			else break;
	regenGlobalRowc(s);
}
/* block interpolate */
void interpolatePartPattern(short x1, short x2, short y1, short y2, uint8_t c1, uint8_t c2)
{
	uint8_t i, j;
	int k;
	Row *r, *r1, *r2;
	if (c1 == c2) /* only one channel */
	{
		r1 = getChannelRow(&s->channel->v[c1].data, y1);
		r2 = getChannelRow(&s->channel->v[c1].data, y2);
		for (j = y1; j <= y2; j++)
		{
			r = getChannelRow(&s->channel->v[c1].data, j);
			if (x1 <= 0 && x2 >= 0)
			{
				if      (r1->note == NOTE_VOID) r->note = r2->note;
				else if (r2->note == NOTE_VOID) r->note = r1->note;
				else r->note = r1->note + ((r2->note - r1->note) / (float)(y2-y1)) * (j-y1);
			}
			if (x1 <= 1 && x2 >= 1)
			{
				if      (r1->inst == INST_VOID) r->inst = r2->inst;
				else if (r2->inst == INST_VOID) r->inst = r1->inst;
				else r->inst = r1->inst + ((r2->inst - r1->inst) / (float)(y2-y1)) * (j-y1);
			}
			for (k = 0; k <= s->channel->v[c1].data.macroc; k++)
				if (x1 <= k+2 && x2 >= k+2)
				{
					r->macro[k].c = (r1->macro[k].c) ?  r1->macro[k].c : r2->macro[k].c;
					r->macro[k].v =  r1->macro[k].v + ((r2->macro[k].v - r1->macro[k].v) / (float)(y2-y1)) * (j-y1);
				}
		}
	} else
		for (i = c1; i <= c2; i++)
			if (i < s->channel->c) {
				r1 = getChannelRow(&s->channel->v[i].data, y1);
				r2 = getChannelRow(&s->channel->v[i].data, y2);
				for (j = y1; j <= y2; j++)
				{
					r = getChannelRow(&s->channel->v[i].data, j);
					if (i == c1) /* first channel */
					{
						if (x1 <= 0)
						{
							if      (r1->note == NOTE_VOID) r->note = r2->note;
							else if (r2->note == NOTE_VOID) r->note = r1->note;
							else r->note = r1->note + ((r2->note - r1->note) / (float)(y2-y1)) * (j-y1);
						}
						if (x1 <= 1)
						{
							if      (r1->inst == INST_VOID) r->inst = r2->inst;
							else if (r2->inst == INST_VOID) r->inst = r1->inst;
							else r->inst = r1->inst + ((r2->inst - r1->inst) / (float)(y2-y1)) * (j-y1);
						}
						for (k = 0; k <= s->channel->v[c1].data.macroc; k++)
							if (x1 <= k+2)
							{
								r->macro[k].c = (r1->macro[k].c) ?  r1->macro[k].c : r2->macro[k].c;
								r->macro[k].v =  r1->macro[k].v + ((r2->macro[k].v - r1->macro[k].v) / (float)(y2-y1)) * (j-y1);
							}
					}
					else if (i == c2) /* last channel */
					{
						if (x2 >= 0)
						{
							if      (r1->note == NOTE_VOID) r->note = r2->note;
							else if (r2->note == NOTE_VOID) r->note = r1->note;
							else r->note = r1->note + ((r2->note - r1->note) / (float)(y2-y1)) * (j-y1);
						}
						if (x2 >= 1)
						{
							if      (r1->inst == INST_VOID) r->inst = r2->inst;
							else if (r2->inst == INST_VOID) r->inst = r1->inst;
							else r->inst = r1->inst + ((r2->inst - r1->inst) / (float)(y2-y1)) * (j-y1);
						}
						for (k = 0; k <= s->channel->v[i].data.macroc; k++)
							if (x2 >= k+2)
							{
								r->macro[k].c = (r1->macro[k].c) ?  r1->macro[k].c : r2->macro[k].c;
								r->macro[k].v =  r1->macro[k].v + ((r2->macro[k].v - r1->macro[k].v) / (float)(y2-y1)) * (j-y1);
							}
					}
					else /* middle channel */
					{
						if      (r1->note == NOTE_VOID) r->note = r2->note;
						else if (r2->note == NOTE_VOID) r->note = r1->note;
						else r->note = r1->note + ((r2->note - r1->note) / (float)(y2-y1)) * (j-y1);

						if      (r1->inst == INST_VOID) r->inst = r2->inst;
						else if (r2->inst == INST_VOID) r->inst = r1->inst;
						else r->inst = r1->inst + ((r2->inst - r1->inst) / (float)(y2-y1)) * (j-y1);

						for (k = 0; k <= s->channel->v[i].data.macroc; k++)
						{
							r->macro[k].c = (r1->macro[k].c) ?  r1->macro[k].c : r2->macro[k].c;
							r->macro[k].v =  r1->macro[k].v + ((r2->macro[k].v - r1->macro[k].v) / (float)(y2-y1)) * (j-y1);
						}
					}
				}
			} else break;
	regenGlobalRowc(s);
}
/* block randomize */
void randPartPattern(short x1, short x2, short y1, short y2, uint8_t c1, uint8_t c2)
{
	uint8_t i, j, randinst;
	int k;
	Row *r;
	if (c1 == c2) /* only one channel */
	{
		for (j = y1; j <= y2; j++)
		{
			r = getChannelRow(&s->channel->v[c1].data, j);
			if (x1 <= 0 && x2 >= 0) r->note = MIN(NOTE_A10-1, rand()%36 +MIN(7, w->octave)*12);
			if (x1 <= 1 && x2 >= 1 && r->note != NOTE_VOID)
			{
				r->inst = NOTE_VOID;
				randinst = rand()%(s->instrument->c-1) + 1;
				for (k = 0; k < INSTRUMENT_MAX; k++)
					if (s->instrument->i[k] == randinst) { r->inst = k; break; }
			}
			for (k = 0; k <= s->channel->v[c1].data.macroc; k++)
				if (x1 <= k+2 && x2 >= k+2 && r->macro[k].c)
					r->macro[k].v = rand()%256;
		}
	} else
	{
		for (i = c1; i <= c2; i++)
			if (i < s->channel->c)
				for (j = y1; j <= y2; j++)
				{
					r = getChannelRow(&s->channel->v[i].data, j);
					if (i == c1) /* first channel */
					{
						if (x1 <= 0) r->note = MIN(NOTE_A10-1, rand()%36 +MIN(7, w->octave)*12);
						if (x1 <= 1 && r->note != NOTE_VOID)
						{
							r->inst = NOTE_VOID;
							randinst = rand()%(s->instrument->c-1) + 1;
							for (k = 0; k < INSTRUMENT_MAX; k++)
								if (s->instrument->i[k] == randinst) { r->inst = k; break; }
						}
						for (k = 0; k <= s->channel->v[i].data.macroc; k++)
							if (x1 <= k+2 && r->macro[k].c)
								r->macro[k].v = rand()%256;
					} else if (i == c2) /* last channel */
					{
						if (x2 >= 0) r->note = MIN(NOTE_A10-1, rand()%36 +MIN(7, w->octave)*12);
						if (x2 >= 1 && r->note != NOTE_VOID)
						{
							r->inst = NOTE_VOID;
							randinst = rand()%(s->instrument->c-1) + 1;
							for (k = 0; k < INSTRUMENT_MAX; k++)
								if (s->instrument->i[k] == randinst)
								{ r->inst = k; break; }
						}
						for (k = 0; k <= s->channel->v[i].data.macroc; k++)
							if (x2 >= k+2 && r->macro[k].c)
								r->macro[k].v = rand()%256;
					} else /* middle channel */
					{
						r->note = MIN(NOTE_A10-1, rand()%36 +MIN(7, w->octave)*12);
						if (r->note != NOTE_VOID)
						{
							r->inst = NOTE_VOID;
							randinst = rand()%(s->instrument->c-1) + 1;
							for (k = 0; k < INSTRUMENT_MAX; k++)
								if (s->instrument->i[k] == randinst) { r->inst = k; break; }
						}
						for (k = 0; k <= s->channel->v[i].data.macroc; k++)
							if (r->macro[k].c)
								r->macro[k].v = rand()%256;
					}
				}
			else break;
	}
	regenGlobalRowc(s);
}
void cycleUpPartPattern(short x1, short x2, short y1, short y2, uint8_t c1, uint8_t c2)
{
	uint8_t i;
	int j, k, l;
	Row hold, *r0, *r1, *r2;
	if (c1 == c2) /* only one channel */
	{
		for (l = 0; l < MAX(1, w->count); l++)
		{
			hold = *getChannelRow(&s->channel->v[c1].data, y1);
			r0 =    getChannelRow(&s->channel->v[c1].data, j+0);
			r1 =    getChannelRow(&s->channel->v[c1].data, j+1);
			r2 =    getChannelRow(&s->channel->v[c1].data, y2);
			if (x1 <= 0 && x2 >= 0)
			{
				for (j = y1; j < y2; j++) r0->note = r1->note;
				r2->note = hold.note;
			}
			if (x1 <= 1 && x2 >= 1)
			{
				for (j = y1; j < y2; j++) r0->inst = r1->inst;
				r2->inst = hold.inst;
			}
			for (k = 0; k <= s->channel->v[c1].data.macroc; k++)
				if (x1 <= k+2 && x2 >= k+2)
				{
					for (j = y1; j < y2; j++)
					{
						r0->macro[k].c = r1->macro[k].c;
						r0->macro[k].v = r1->macro[k].v;
					}
					r2->macro[k].c = hold.macro[k].c;
					r2->macro[k].v = hold.macro[k].v;
				}
		}
	} else
	{
		for (i = c1; i <= c2; i++)
			if (i < s->channel->c)
			{
				hold = *getChannelRow(&s->channel->v[i].data, y1);
				r0 =    getChannelRow(&s->channel->v[i].data, j+0);
				r1 =    getChannelRow(&s->channel->v[i].data, j+1);
				r2 =    getChannelRow(&s->channel->v[i].data, y2);
				if (i == c1) /* first channel */
					for (l = 0; l < MAX(1, w->count); l++)
					{
						if (x1 <= 0)
						{
							for (j = y1; j < y2; j++) r0->note = r1->note;
							r2->note = hold.note;
						}
						if (x1 <= 1)
						{
							for (j = y1; j < y2; j++) r0->inst = r1->inst;
							r2->inst = hold.inst;
						}
						for (k = 0; k <= s->channel->v[c1].data.macroc; k++)
							if (x1 <= k+2)
							{
								for (j = y1; j < y2; j++)
								{
									r0->macro[k].c = r1->macro[k].c;
									r0->macro[k].v = r1->macro[k].v;
								}
								r2->macro[k].c = hold.macro[k].c;
								r2->macro[k].v = hold.macro[k].v;
							}
					}
				else if (i == c2) /* last channel */
					for (l = 0; l < MAX(1, w->count); l++)
					{
						if (x2 >= 0)
						{
							for (j = y1; j < y2; j++) r0->note = r1->note;
							r2->note = hold.note;
						}
						if (x2 >= 1)
						{
							for (j = y1; j < y2; j++) r0->inst = r1->inst;
							r2->inst = hold.inst;
						}
						for (k = 0; k <= s->channel->v[c2].data.macroc; k++)
							if (x2 >= k+2)
							{
								for (j = y1; j < y2; j++)
								{
									r0->macro[k].c = r1->macro[k].c;
									r0->macro[k].v = r1->macro[k].v;
								}
								r2->macro[k].c = hold.macro[k].c;
								r2->macro[k].v = hold.macro[k].v;
							}
					}
				else /* middle channel */
					for (l = 0; l < MAX(1, w->count); l++)
					{
						for (j = y1; j < y2; j++) *r0 = *r1;
						*r2 = hold;
					}
			} else break;
	}
	regenGlobalRowc(s);
}
void cycleDownPartPattern(short x1, short x2, short y1, short y2, uint8_t c1, uint8_t c2)
{
	uint8_t i;
	int j, k, l;
	Row hold, *r0, *r1, *r2;
	if (c1 == c2) /* only one channel */
	{
		for (l = 0; l < MAX(1, w->count); l++)
		{
			hold = *getChannelRow(&s->channel->v[c1].data, y2);
			r0 =    getChannelRow(&s->channel->v[c1].data, j+1);
			r1 =    getChannelRow(&s->channel->v[c1].data, j+0);
			r2 =    getChannelRow(&s->channel->v[c1].data, y1);

			if (x1 <= 0 && x2 >= 0)
			{
				for (j = y2-1; j >= y1; j--) r0->note = r1->note;
				r2->note = hold.note;
			}
			if (x1 <= 1 && x2 >= 1)
			{
				for (j = y2-1; j >= y1; j--) r0->inst = r1->inst;
				r2->inst = hold.inst;
			}
			for (k = 0; k <= s->channel->v[c1].data.macroc; k++)
				if (x1 <= k+2 && x2 >= k+2)
				{
					for (j = y2-1; j >= y1; j--)
					{
						r0->macro[k].c = r1->macro[k].c;
						r0->macro[k].v = r1->macro[k].v;
					}
					r2->macro[k].c = hold.macro[k].c;
					r2->macro[k].v = hold.macro[k].v;
				}
		}
	} else
	{
		for (i = c1; i <= c2; i++)
			if (i < s->channel->c)
			{
				hold = *getChannelRow(&s->channel->v[i].data, y2);
				r0 =    getChannelRow(&s->channel->v[i].data, j+1);
				r1 =    getChannelRow(&s->channel->v[i].data, j+0);
				r2 =    getChannelRow(&s->channel->v[i].data, y1);

				if (i == c1) /* first channel */
					for (l = 0; l < MAX(1, w->count); l++)
					{
						if (x1 <= 0)
						{
							for (j = y2-1; j >= y1; j--) r0->note = r1->note;
							r2->note = hold.note;
						}
						if (x1 <= 1)
						{
							for (j = y2-1; j >= y1; j--) r0->inst = r1->inst;
							r2->inst = hold.inst;
						}
						for (k = 0; k <= s->channel->v[c1].data.macroc; k++)
							if (x1 <= k+2)
							{
								for (j = y2-1; j >= y1; j--)
								{
									r0->macro[k].c = r1->macro[k].c;
									r0->macro[k].v = r1->macro[k].v;
								}
								r2->macro[k].c = hold.macro[k].c;
								r2->macro[k].v = hold.macro[k].v;
							}
					}
				else if (i == c2) /* last channel */
					for (l = 0; l < MAX(1, w->count); l++)
					{
						if (x2 >= 0)
						{
							for (j = y2-1; j >= y1; j--) r0->note = r1->note;
							r2->note = hold.note;
						}
						if (x2 >= 1)
						{
							for (j = y2-1; j >= y1; j--) r0->inst = r1->inst;
							r2->inst = hold.inst;
						}
						for (k = 0; k <= s->channel->v[c2].data.macroc; k++)
							if (x2 >= k+2)
							{
								for (j = y2-1; j >= y1; j--)
								{
									r0->macro[k].c = r1->macro[k].c;
									r0->macro[k].v = r1->macro[k].v;
								}
								r2->macro[k].c = hold.macro[k].c;
								r2->macro[k].v = hold.macro[k].v;
							}
					}
				else /* middle channel */
					for (l = 0; l < MAX(1, w->count); l++)
					{
						for (j = y2-1; j >= y1; j--) *r0 = *r1;
						*r2 = hold;
					}
			} else break;
	}
	regenGlobalRowc(s);
}

#define BOUNCE_SCALE 2.0f /* multiply samples by this before saving */
void bouncePartPattern(short y1, short y2, uint8_t c1, uint8_t c2)
{
	short inst = emptyInstrument(0);
	if (inst == -1)
	{
		strcpy(w->command.error, "visual render failed, no empty instrument slot");
		return;
	}

	uint16_t row;
	uint8_t chnl;
	jack_nframes_t buflen, bufptr, fptr;
	uint16_t spr, sprp;

	/* allocate temp channel state */
	Channel *cv;
	cv = calloc(c2-c1 + 1, sizeof(Channel));
	for (chnl = 0; chnl <= c2-c1; chnl++)
	{
		_addChannel(s, &cv[chnl]);
		copyChanneldata(&cv[chnl].data, &s->channel->v[c1+chnl].data);
	}

	/* calculate the buffer length needed */
	setBpm(&spr, s->songbpm);
	for (row = 0; row < y1; row++)
		for (chnl = 0; chnl <= c2-c1; chnl++)
			ifMacro(0, &spr, &cv[chnl], *getChannelRow(&cv[chnl].data, row), 'B', &Bc);
	buflen = 0;
	for (row = y1; row <= y2; row++)
	{
		for (chnl = 0; chnl <= c2-c1; chnl++)
			ifMacro(0, &spr, &cv[chnl], *getChannelRow(&cv[chnl].data, row), 'B', &Bc);
		buflen += spr;
	}

	/* allocate the sample buffer */
	short *buffer = calloc((buflen<<1), sizeof(short));
	/* for (uint32_t i = 0; i < buflen<<1; i++)
		buffer[i] = rand()%USHRT_MAX - SHRT_MAX; */

	/* lookback */
	setBpm(&spr, s->songbpm);
	for (chnl = 0; chnl <= c2-c1; chnl++)
		lookback(0, &spr, &cv[chnl]);

	bufptr = 0;
	Row *r;
	for (row = y1; row <= y2; row++)
	{
		for (chnl = 0; chnl <= c2-c1; chnl++)
		{
			r = getChannelRow(&cv[chnl].data, row);
			preprocessRow(0, &spr, 0, &cv[chnl], *r);
		}
		if (s->bpmcache[row] != -1) Bc(fptr, &spr, s->bpmcache[row], NULL, *r);

		sprp = 0;
		while (sprp < spr)
		{
			for (fptr = 0; fptr < buffersize; fptr++)
			{
				if (sprp >= spr) break;
				for (chnl = 0; chnl <= c2-c1; chnl++)
				{
					playChannel(0, fptr, &spr, fptr, &cv[chnl]);
					buffer[((bufptr+sprp)<<1)+0] += cv[chnl].output[0][fptr]*cv[chnl].mainmult[0][fptr]*BOUNCE_SCALE*SHRT_MAX;
					buffer[((bufptr+sprp)<<1)+1] += cv[chnl].output[1][fptr]*cv[chnl].mainmult[1][fptr]*BOUNCE_SCALE*SHRT_MAX;
				} sprp++;
			}
		} bufptr += sprp;
	}

	/* run channnel insert effects then free temp channel state */
	for (chnl = 0; chnl <= c2-c1; chnl++)
	{
		for (uint8_t i = 0; i < cv[chnl].data.effect.c; i++)
			runEffect(buflen, &cv[chnl].data.effect.v[i]);

		_delChannel(&cv[chnl]);
	} free(cv);

	/* init the instrument */
	addReparentInstrument(inst, 0, buffer, buflen, samplerate);

	w->instrument = inst;
}
