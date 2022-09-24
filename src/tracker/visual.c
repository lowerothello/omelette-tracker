short tfxToVfx(short tfx)
{
	if (tfx > 1) return 2 + (tfx - 2) / 2;
	return tfx;
}
short vfxToTfx(short vfx)
{
	if (vfx > 1) return 2 + (vfx - 2) * 2;
	return vfx;
}
void yankPartPattern(short x1, short x2, short y1, short y2, uint8_t c1, uint8_t c2)
{
	/* walk over allocated channels and free them */
	for (short i = 0; i < w->pbchannelc; i++)
		free(w->pbvariantv[i]);

	w->pbfx[0] = x1;
	w->pbfx[1] = x2;
	w->pbchannelc = c2-c1 + 1;

	channeldata *cd;
	for (uint8_t i = 0; i <= c2-c1; i++)
	{
		cd = &s->channelv[c1+i].data;
		w->pbvariantv[i] = _copyVariant(NULL, y2-y1 + 1);
		for (uint16_t j = 0; j <= y2-y1; j++)
			w->pbvariantv[i]->rowv[j] = *getChannelRow(cd, y1+j);
	}
}
void putPartPattern(void)
{
	uint8_t i;
	int k;
	row *dest, *src;
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
				dest = getChannelRow(&s->channelv[w->channel].data, w->trackerfy+j);
				src = getVariantRow(w->pbvariantv[0], j);
				dest->macro[targetmacro].c = src->macro[w->pbfx[0]-2].c;
				dest->macro[targetmacro].v = src->macro[w->pbfx[0]-2].v;
			}
			w->trackerfx = vfxToTfx(targetmacro + 2);
		} else
		{
			for (uint8_t j = 0; j < w->pbvariantv[0]->rowc; j++)
			{
				if (w->trackerfy + j >= s->songlen) break;
				dest = getChannelRow(&s->channelv[w->channel].data, w->trackerfy+j);
				src = getVariantRow(w->pbvariantv[0], j);
				if (w->pbfx[0] <= 0 && w->pbfx[1] >= 0) dest->note = src->note;
				if (w->pbfx[0] <= 1 && w->pbfx[1] >= 1) dest->inst = src->inst;
				for (k = 0; k <= s->channelv[w->channel].data.macroc; k++)
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
			if (w->channel+i < s->channelc)
				for (uint8_t j = 0; j < w->pbvariantv[i]->rowc; j++)
				{
					if (w->trackerfy + j >= s->songlen) break;
					dest = getChannelRow(&s->channelv[w->channel+i].data, w->trackerfy+j);
					src = getVariantRow(w->pbvariantv[i], j);

					if (i == 0) // first channel
					{
						if (w->pbfx[0] <= 0) dest->note = src->note;
						if (w->pbfx[0] <= 1) dest->inst = src->inst;
						for (k = 0; k <= s->channelv[w->channel+i].data.macroc; k++)
							if (w->pbfx[0] <= k+2)
							{
								dest->macro[k].c = src->macro[k].c;
								dest->macro[k].v = src->macro[k].v;
							}
					} else if (i == w->pbchannelc-1) // last channel
					{
						if (w->pbfx[1] >= 0) dest->note = src->note;
						if (w->pbfx[1] >= 1) dest->inst = src->inst;
						for (k = 0; k <= s->channelv[w->channel+i].data.macroc; k++)
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
}
void yankPartVtrig(short y1, short y2, uint8_t c1, uint8_t c2)
{
	for (short i = 0; i < w->vbchannelc; i++)
		free(w->vbtrig[i]);

	w->vbchannelc = c2-c1 + 1;
	w->vbrowc = y2-y1 + 1;

	for (uint8_t i = 0; i <= c2-c1; i++)
	{
		w->vbtrig[i] = calloc(y2-y1 + 1, sizeof(vtrig));
		for (uint16_t j = 0; j <= y2-y1; j++)
			w->vbtrig[i][j] = s->channelv[c1+i].data.trig[y1+j];
	}
}
void putPartVtrig(void)
{
	for (uint8_t i = 0; i < w->vbchannelc; i++)
		if (w->channel+i < s->channelc)
			for (uint8_t j = 0; j < w->vbrowc; j++)
			{
				if (w->trackerfy + j >= s->songlen) break;
				s->channelv[w->channel+i].data.trig[w->trackerfy+j] = w->vbtrig[i][j];
			}
		else break;
}
void mixPutPartVtrig(void)
{
	for (uint8_t i = 0; i < w->vbchannelc; i++)
		if (w->channel+i < s->channelc)
			for (uint8_t j = 0; j < w->vbrowc; j++)
			{
				if (w->trackerfy + j >= s->songlen) break;
				if (w->vbtrig[i][j].index != VARIANT_VOID)
					s->channelv[w->channel+i].data.trig[w->trackerfy+j] = w->vbtrig[i][j];
			}
		else break;
}
void mixPutPartPattern(void)
{
	uint8_t i;
	int k;
	row *dest, *src;
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
				dest = getChannelRow(&s->channelv[w->channel].data, w->trackerfy+j);
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
				dest = getChannelRow(&s->channelv[w->channel].data, w->trackerfy+j);
				src = getVariantRow(w->pbvariantv[0], j);
				if (w->pbfx[0] <= 0 && w->pbfx[1] >= 0 && src->note != NOTE_VOID) dest->note = src->note;
				if (w->pbfx[0] <= 1 && w->pbfx[1] >= 1 && src->inst != INST_VOID) dest->inst = src->inst;
				for (k = 0; k <= s->channelv[w->channel].data.macroc; k++)
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
			if (w->channel+i < s->channelc)
				for (uint8_t j = 0; j < w->pbvariantv[i]->rowc; j++)
				{
					if (w->trackerfy + j >= s->songlen) break;
					dest = getChannelRow(&s->channelv[w->channel+i].data, w->trackerfy+j);
					src = getVariantRow(w->pbvariantv[i], j);

					if (i == 0) // first channel
					{
						if (w->pbfx[0] <= 0 && src->note != NOTE_VOID) dest->note = src->note;
						if (w->pbfx[0] <= 1 && src->inst != INST_VOID) dest->inst = src->inst;
						for (k = 0; k <= s->channelv[w->channel+i].data.macroc; k++)
							if (w->pbfx[0] <= k+2 && src->macro[k].c)
							{
								dest->macro[k].c = src->macro[k].c;
								dest->macro[k].v = src->macro[k].v;
							}
					} else if (i == w->pbchannelc-1) // last channel
					{
						if (w->pbfx[1] >= 0 && src->note != NOTE_VOID) dest->note = src->note;
						if (w->pbfx[1] >= 1 && src->inst != INST_VOID) dest->inst = src->inst;
						for (k = 0; k <= s->channelv[w->channel+i].data.macroc; k++)
							if (w->pbfx[1] >= k+2 && src->macro[k].c)
							{
								dest->macro[k].c = src->macro[k].c;
								dest->macro[k].v = src->macro[k].v;
							}
					} else // middle channel
					{
						if (src->note != NOTE_VOID) dest->note = src->note;
						if (src->inst != INST_VOID) dest->inst = src->inst;
						for (k = 0; k <= s->channelv[w->channel+i].data.macroc; k++)
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
}
void delPartPattern(short x1, short x2, short y1, short y2, uint8_t c1, uint8_t c2)
{
	uint8_t i, j;
	row *r;
	int k;
	if (c1 == c2) /* only one channel */
	{
		for (j = y1; j <= y2; j++)
		{
			r = getChannelRow(&s->channelv[c1].data, j);
			if (x1 <= 0 && x2 >= 0) r->note = NOTE_VOID;
			if (x1 <= 1 && x2 >= 1) r->inst = INST_VOID;
			for (k = 0; k <= s->channelv[c1].data.macroc; k++)
				if (x1 <= k+2 && x2 >= k+2)
				{
					r->macro[k].c = 0;
					r->macro[k].v = 0;
				}
		}
	} else
		for (i = c1; i <= c2; i++)
			if (i < s->channelc)
				for (j = y1; j <= y2; j++)
				{
					r = getChannelRow(&s->channelv[i].data, j);
					if (i == c1) /* first channel */
					{
						if (x1 <= 0) r->note = NOTE_VOID;
						if (x1 <= 1) r->inst = INST_VOID;
						for (k = 0; k <= s->channelv[i].data.macroc; k++)
							if (x1 <= k+2)
							{
								r->macro[k].c = 0;
								r->macro[k].v = 0;
							}
					} else if (i == c2) /* last channel */
					{
						if (x2 >= 0) r->note = NOTE_VOID;
						if (x2 >= 1) r->inst = INST_VOID;
						for (k = 0; k <= s->channelv[i].data.macroc; k++)
							if (x2 >= k+2)
							{
								r->macro[k].c = 0;
								r->macro[k].v = 0;
							}
					} else /* middle channel */
					{
						memset(r, 0, sizeof(row));
						r->note = NOTE_VOID;
						r->inst = INST_VOID;
					}
				}
			else break;
}
void delPartVtrig(short y1, short y2, uint8_t c1, uint8_t c2)
{
	for (uint8_t i = c1; i <= c2; i++)
		if (i < s->channelc)
			for (uint8_t j = y1; j <= y2; j++)
				setChannelTrig(&s->channelv[i].data, j, VARIANT_VOID);
		else break;
}
void loopPartVtrig(short y1, short y2, uint8_t c1, uint8_t c2)
{
	for (uint8_t i = c1; i <= c2; i++)
		if (i < s->channelc)
			for (uint8_t j = y1; j <= y2; j++)
			{
				if (s->channelv[i].data.trig[j].index != VARIANT_VOID)
					s->channelv[i].data.trig[j].flags ^= C_VTRIG_LOOP;
			}
		else break;
}
/* block inc/dec */
void addPartPattern(signed char value, short x1, short x2, short y1, short y2, uint8_t c1, uint8_t c2)
{
	uint8_t i, j;
	int k;
	row *r;
	if (c1 == c2) /* only one channel */
	{
		for (j = y1; j <= y2; j++)
		{
			r = getChannelRow(&s->channelv[c1].data, j);
			if (x1 <= 0 && x2 >= 0) r->note += value;
			if (x1 <= 1 && x2 >= 1) r->inst += value;
			for (k = 0; k <= s->channelv[c1].data.macroc; k++)
				if (x1 <= k+2 && x2 >= k+2)
					switch (r->macro[k].c)
					{
						case 'G': case 'g': case 'K': case 'k': r->macro[k].v += value*16;
						default:                                r->macro[k].v += value;
					}
		}
	} else
		for (i = c1; i <= c2; i++)
			if (i < s->channelc)
				for (j = y1; j <= y2; j++)
				{
					r = getChannelRow(&s->channelv[i].data, j);
					if (i == c1) /* first channel */
					{
						if (x1 <= 0) r->note += value;
						if (x1 <= 1) r->inst += value;
						for (k = 0; k <= s->channelv[i].data.macroc; k++)
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
						for (k = 0; k <= s->channelv[i].data.macroc; k++)
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
						for (k = 0; k <= s->channelv[i].data.macroc; k++)
							switch (r->macro[k].c)
							{
								case 'G': case 'g': case 'K': case 'k': r->macro[k].v += value*16;
								default:                                r->macro[k].v += value;
							}
					}
				}
			else break;
}
/* block toggle case */
void tildePartPattern(short x1, short x2, short y1, short y2, uint8_t c1, uint8_t c2)
{
	uint8_t i, j;
	int k;
	row *r;
	if (c1 == c2) /* only one channel */
	{
		for (j = y1; j <= y2; j++)
		{
			r = getChannelRow(&s->channelv[c1].data, j);
			for (k = 0; k <= s->channelv[c1].data.macroc; k++)
				if (x1 <= k+2 && x2 >= k+2)
				{
					if      (isupper(r->macro[k].c)) changeMacro(r->macro[k].c, &r->macro[k].c);
					else if (islower(r->macro[k].c)) changeMacro(r->macro[k].c, &r->macro[k].c);
				}
		}
	} else
		for (i = c1; i <= c2; i++)
			if (i < s->channelc)
				for (j = y1; j <= y2; j++)
				{
					r = getChannelRow(&s->channelv[i].data, j);
					if (i == c1) /* first channel */
						for (k = 0; k <= s->channelv[i].data.macroc; k++) {
							if (x1 <= k+2)
							{
								if      (isupper(r->macro[k].c)) changeMacro(r->macro[k].c, &r->macro[k].c);
								else if (islower(r->macro[k].c)) changeMacro(r->macro[k].c, &r->macro[k].c);
							}}
					else if (i == c2) /* last channel */
						for (k = 0; k <= s->channelv[i].data.macroc; k++) {
							if (x2 >= k+2)
							{
								if      (isupper(r->macro[k].c)) changeMacro(r->macro[k].c, &r->macro[k].c);
								else if (islower(r->macro[k].c)) changeMacro(r->macro[k].c, &r->macro[k].c);
							}}
					else /* middle channel */
						for (k = 0; k <= s->channelv[i].data.macroc; k++)
						{
							if      (isupper(r->macro[k].c)) changeMacro(r->macro[k].c, &r->macro[k].c);
							else if (islower(r->macro[k].c)) changeMacro(r->macro[k].c, &r->macro[k].c);
						}
				}
			else break;
}
/* block interpolate */
void interpolatePartPattern(short x1, short x2, short y1, short y2, uint8_t c1, uint8_t c2)
{
	uint8_t i, j;
	int k;
	row *r, *r1, *r2;
	if (c1 == c2) /* only one channel */
	{
		r1 = getChannelRow(&s->channelv[c1].data, y1);
		r2 = getChannelRow(&s->channelv[c1].data, y2);
		for (j = y1; j <= y2; j++)
		{
			r = getChannelRow(&s->channelv[c1].data, j);
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
			for (k = 0; k <= s->channelv[c1].data.macroc; k++)
				if (x1 <= k+2 && x2 >= k+2)
				{
					r->macro[k].c = (r1->macro[k].c) ?  r1->macro[k].c : r2->macro[k].c;
					r->macro[k].v =  r1->macro[k].v + ((r2->macro[k].v - r1->macro[k].v) / (float)(y2-y1)) * (j-y1);
				}
		}
	} else
		for (i = c1; i <= c2; i++)
			if (i < s->channelc) {
				r1 = getChannelRow(&s->channelv[i].data, y1);
				r2 = getChannelRow(&s->channelv[i].data, y2);
				for (j = y1; j <= y2; j++)
				{
					r = getChannelRow(&s->channelv[i].data, j);
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
						for (k = 0; k <= s->channelv[c1].data.macroc; k++)
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
						for (k = 0; k <= s->channelv[i].data.macroc; k++)
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

						for (k = 0; k <= s->channelv[i].data.macroc; k++)
						{
							r->macro[k].c = (r1->macro[k].c) ?  r1->macro[k].c : r2->macro[k].c;
							r->macro[k].v =  r1->macro[k].v + ((r2->macro[k].v - r1->macro[k].v) / (float)(y2-y1)) * (j-y1);
						}
					}
				}
			} else break;
}
/* block randomize */
void randPartPattern(short x1, short x2, short y1, short y2, uint8_t c1, uint8_t c2)
{
	uint8_t i, j, randinst;
	int k;
	row *r;
	if (c1 == c2) /* only one channel */
	{
		for (j = y1; j <= y2; j++)
		{
			r = getChannelRow(&s->channelv[c1].data, j);
			if (x1 <= 0 && x2 >= 0) r->note = MIN(NOTE_A10-1, rand()%36 +MIN(7, w->octave)*12);
			if (x1 <= 1 && x2 >= 1 && r->note != NOTE_VOID)
			{
				r->inst = NOTE_VOID;
				randinst = rand()%(s->instrumentc-1) + 1;
				for (k = 0; k < INSTRUMENT_MAX; k++)
					if (s->instrumenti[k] == randinst) { r->inst = k; break; }
			}
			for (k = 0; k <= s->channelv[c1].data.macroc; k++)
				if (x1 <= k+2 && x2 >= k+2 && r->macro[k].c)
					r->macro[k].v = rand()%256;
		}
	} else
	{
		for (i = c1; i <= c2; i++)
			if (i < s->channelc)
				for (j = y1; j <= y2; j++)
				{
					r = getChannelRow(&s->channelv[i].data, j);
					if (i == c1) /* first channel */
					{
						if (x1 <= 0) r->note = MIN(NOTE_A10-1, rand()%36 +MIN(7, w->octave)*12);
						if (x1 <= 1 && r->note != NOTE_VOID)
						{
							r->inst = NOTE_VOID;
							randinst = rand()%(s->instrumentc-1) + 1;
							for (k = 0; k < INSTRUMENT_MAX; k++)
								if (s->instrumenti[k] == randinst) { r->inst = k; break; }
						}
						for (k = 0; k <= s->channelv[i].data.macroc; k++)
							if (x1 <= k+2 && r->macro[k].c)
								r->macro[k].v = rand()%256;
					} else if (i == c2) /* last channel */
					{
						if (x2 >= 0) r->note = MIN(NOTE_A10-1, rand()%36 +MIN(7, w->octave)*12);
						if (x2 >= 1 && r->note != NOTE_VOID)
						{
							r->inst = NOTE_VOID;
							randinst = rand()%(s->instrumentc-1) + 1;
							for (k = 0; k < INSTRUMENT_MAX; k++)
								if (s->instrumenti[k] == randinst)
								{ r->inst = k; break; }
						}
						for (k = 0; k <= s->channelv[i].data.macroc; k++)
							if (x2 >= k+2 && r->macro[k].c)
								r->macro[k].v = rand()%256;
					} else /* middle channel */
					{
						r->note = MIN(NOTE_A10-1, rand()%36 +MIN(7, w->octave)*12);
						if (r->note != NOTE_VOID)
						{
							r->inst = NOTE_VOID;
							randinst = rand()%(s->instrumentc-1) + 1;
							for (k = 0; k < INSTRUMENT_MAX; k++)
								if (s->instrumenti[k] == randinst) { r->inst = k; break; }
						}
						for (k = 0; k <= s->channelv[i].data.macroc; k++)
							if (r->macro[k].c)
								r->macro[k].v = rand()%256;
					}
				}
			else break;
	}
}
void cycleUpPartPattern(short x1, short x2, short y1, short y2, uint8_t c1, uint8_t c2)
{
	uint8_t i;
	int j, k, l;
	row hold, *r0, *r1, *r2;
	if (c1 == c2) /* only one channel */
	{
		for (l = 0; l < MAX(1, w->count); l++)
		{
			hold = *getChannelRow(&s->channelv[c1].data, y1);
			r0 =    getChannelRow(&s->channelv[c1].data, j+0);
			r1 =    getChannelRow(&s->channelv[c1].data, j+1);
			r2 =    getChannelRow(&s->channelv[c1].data, y2);
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
			for (k = 0; k <= s->channelv[c1].data.macroc; k++)
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
			if (i < s->channelc)
			{
				hold = *getChannelRow(&s->channelv[i].data, y1);
				r0 =    getChannelRow(&s->channelv[i].data, j+0);
				r1 =    getChannelRow(&s->channelv[i].data, j+1);
				r2 =    getChannelRow(&s->channelv[i].data, y2);
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
						for (k = 0; k <= s->channelv[c1].data.macroc; k++)
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
						for (k = 0; k <= s->channelv[c2].data.macroc; k++)
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
}
void cycleDownPartPattern(short x1, short x2, short y1, short y2, uint8_t c1, uint8_t c2)
{
	uint8_t i;
	int j, k, l;
	row hold, *r0, *r1, *r2;
	if (c1 == c2) /* only one channel */
	{
		for (l = 0; l < MAX(1, w->count); l++)
		{
			hold = *getChannelRow(&s->channelv[c1].data, y2);
			r0 =    getChannelRow(&s->channelv[c1].data, j+1);
			r1 =    getChannelRow(&s->channelv[c1].data, j+0);
			r2 =    getChannelRow(&s->channelv[c1].data, y1);

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
			for (k = 0; k <= s->channelv[c1].data.macroc; k++)
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
			if (i < s->channelc)
			{
				hold = *getChannelRow(&s->channelv[i].data, y2);
				r0 =    getChannelRow(&s->channelv[i].data, j+1);
				r1 =    getChannelRow(&s->channelv[i].data, j+0);
				r2 =    getChannelRow(&s->channelv[i].data, y1);

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
						for (k = 0; k <= s->channelv[c1].data.macroc; k++)
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
						for (k = 0; k <= s->channelv[c2].data.macroc; k++)
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
}
