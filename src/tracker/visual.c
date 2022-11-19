short tfxToVfx(int8_t tfx)
{
	if (tfx > 1) return 2 + (tfx - 2) / 2;
	return tfx;
}
short vfxToTfx(int8_t vfx)
{
	if (vfx > 1) return 2 + (vfx - 2) * 2;
	return vfx;
}

/* VMO: visual macro order */
short tfxToVmo(TrackData *cd, short tfx)
{
	if (tfx < 2) return tfx; /* no change for note and inst columns */
	if (tfx&0x1) /* macrov */ return (4 + (cd->variant->macroc<<1)) - tfx;
	else         /* macroc */ return (2 + (cd->variant->macroc<<1)) - tfx;
}
/* VMO: visual macro order */
short vfxToVmo(TrackData *cd, short vfx)
{
	if (vfx < 2) return vfx; /* no change for note and inst columns */
	return (2 + (cd->variant->macroc<<1)) - vfx;
}

short vfxVmoMin(short x1, short x2)
{
	if (x1 < 2 || x2 < 2) return MIN(x1, x2); /* either are macros */
	return MAX(x1, x2); /* both are macros */
}
short vfxVmoMax(short x1, short x2)
{
	if (x1 < 2 || x2 < 2) return MAX(x1, x2); /* either are macros */
	return MIN(x1, x2); /* both are macros */
}

/* returns true if (x >= min && x <= max) in visual macro order */
bool vfxVmoRangeIncl(short min, short max, short x)
{
	if (min > 1) /* range is all macros */
		return (x <= min && x >= max); /* fully inverted */

	if (max > 1) /* range goes from non-macro to macro */
	{
		if (x > 1) /* x is in the macro part */
			return (x >= max);
		else /* x is in the non-macro part */
			return (x >= min);
	}

	/* range goes from non-macro to non-macro */
	return (x >= min && x <= max); /* not inverted */
}


void yankPartPattern(int8_t x1, int8_t x2, short y1, short y2, uint8_t c1, uint8_t c2)
{
	/* walk over allocated tracks and free them */
	for (short i = 0; i < w->pbtrackc; i++)
	{
		free(w->pbvariantv[i]);
		free(w->vbtrig[i]);
	}

	w->pbfx[0] = x1;
	w->pbfx[1] = x2;
	w->pbtrackc = c2-c1 + 1;

	TrackData *cd;
	for (uint8_t i = 0; i <= c2-c1; i++)
	{
		cd = &s->track->v[c1+i].data;
		w->pbvariantv[i] = dupVariant(NULL, y2-y1 + 1);
		for (uint16_t j = 0; j <= y2-y1; j++)
			w->pbvariantv[i]->rowv[j] = *getTrackRow(cd, y1+j);

		w->vbtrig[i] = calloc(y2-y1 + 1, sizeof(Vtrig));
		for (uint16_t j = 0; j <= y2-y1; j++)
			w->vbtrig[i][j] = cd->variant->trig[y1+j];
	}
}
void putPartPattern(void)
{
	uint8_t i, j;
	int k;
	Row *dest, *src;
	char targetmacro;
	TrackData *cd;

	if (!w->pbtrackc) return;
	if (w->pbtrackc == 1) // only one track
	{
		cd = &s->track->v[w->track].data;
		if (w->pbfx[0] > 1 && w->pbfx[1] > 1) /* just macro columns */
		{
			if (w->trackerfx < 2) targetmacro = 0;
			else                  targetmacro = tfxToVfx(w->trackerfx)-2;

			targetmacro -= w->pbfx[1] - w->pbfx[0];

			for (j = 0; j < w->pbvariantv[0]->rowc; j++)
			{
				if (w->trackerfy + j >= s->songlen) break;
				dest = getTrackRow(cd, w->trackerfy+j);
				src = getVariantRow(w->pbvariantv[0], j);
				for (k = 0; k <= w->pbfx[1] - w->pbfx[0]; k++)
				{
					if (targetmacro+k < 0) continue;
					dest->macro[targetmacro+k].c =   src->macro[w->pbfx[0]-2+k].c;
					dest->macro[targetmacro+k].v =   src->macro[w->pbfx[0]-2+k].v;
					dest->macro[targetmacro+k].alt = src->macro[w->pbfx[0]-2+k].alt;
				}
			} w->trackerfx = vfxToTfx(targetmacro+(w->pbfx[1] - w->pbfx[0])+2);
		} else
		{
			for (j = 0; j < w->pbvariantv[0]->rowc; j++)
			{
				if (w->trackerfy + j >= s->songlen) break;
				dest = getTrackRow(cd, w->trackerfy+j);
				src = getVariantRow(w->pbvariantv[0], j);
				if (w->pbfx[0] <= -1 && w->pbfx[1] >= -1)
				{
					cd->variant->trig[w->trackerfy+j] = w->vbtrig[0][j];
					addVariant(&cd->variant, cd->variant->trig[w->trackerfy+j].index, w->defvariantlength);
				}
				if (w->pbfx[0] <= 0 && w->pbfx[1] >= 0) dest->note = src->note;
				if (w->pbfx[0] <= 1 && w->pbfx[1] >= 1) dest->inst = src->inst;
				for (k = 0; k <= cd->variant->macroc; k++)
					if (w->pbfx[0] <= k+2 && w->pbfx[1] >= k+2)
					{
						dest->macro[k].c = src->macro[k].c;
						dest->macro[k].v = src->macro[k].v;
						dest->macro[k].alt = src->macro[k].alt;
					}
			} w->trackerfx = vfxToTfx(w->pbfx[0]);
		}
	} else
	{
		for (i = 0; i < w->pbtrackc; i++)
		{
			cd = &s->track->v[w->track+i].data;
			if (w->track+i < s->track->c)
				for (j = 0; j < w->pbvariantv[i]->rowc; j++)
				{
					if (w->trackerfy + j >= s->songlen) break;
					dest = getTrackRow(cd, w->trackerfy+j);
					src = getVariantRow(w->pbvariantv[i], j);

					if (i == 0) // first track
					{
						if (w->pbfx[0] <= -1)
						{
							cd->variant->trig[w->trackerfy+j] = w->vbtrig[i][j];
							addVariant(&cd->variant, cd->variant->trig[w->trackerfy+j].index, w->defvariantlength);
						}
						if (w->pbfx[0] <= 0) dest->note = src->note;
						if (w->pbfx[0] <= 1) dest->inst = src->inst;
						for (k = 0; k <= cd->variant->macroc; k++)
							if (w->pbfx[0] <= k+2)
							{
								dest->macro[k].c = src->macro[k].c;
								dest->macro[k].v = src->macro[k].v;
								dest->macro[k].alt = src->macro[k].alt;
							}
					} else if (i == w->pbtrackc-1) // last track
					{
						if (w->pbfx[0] >= -1)
						{
							cd->variant->trig[w->trackerfy+j] = w->vbtrig[i][j];
							addVariant(&cd->variant, cd->variant->trig[w->trackerfy+j].index, w->defvariantlength);
						}
						if (w->pbfx[1] >= 0) dest->note = src->note;
						if (w->pbfx[1] >= 1) dest->inst = src->inst;
						for (k = 0; k <= cd->variant->macroc; k++)
							if (w->pbfx[1] >= k+2)
							{
								dest->macro[k].c = src->macro[k].c;
								dest->macro[k].v = src->macro[k].v;
								dest->macro[k].alt = src->macro[k].alt;
							}
					} else
					{
						cd->variant->trig[w->trackerfy+j] = w->vbtrig[i][j];
						addVariant(&cd->variant, cd->variant->trig[w->trackerfy+j].index, w->defvariantlength);
						*dest = *src;
					}
				}
			else break;
		} w->trackerfx = vfxToTfx(w->pbfx[0]);
	} regenGlobalRowc(s);
}
void mixPutPartPattern(void)
{
	uint8_t i, j;
	int k;
	Row *dest, *src;
	char targetmacro;
	TrackData *cd;

	if (!w->pbtrackc) return;
	if (w->pbtrackc == 1) // only one track
	{
		cd = &s->track->v[w->track].data;
		if (w->pbfx[0] > 1 && w->pbfx[1] > 1) /* just macro columns */
		{
			if (w->trackerfx < 2) targetmacro = 0;
			else                  targetmacro = tfxToVfx(w->trackerfx)-2;

			targetmacro -= w->pbfx[1] - w->pbfx[0];

			for (j = 0; j < w->pbvariantv[0]->rowc; j++)
			{
				if (w->trackerfy + j >= s->songlen) break;
				dest = getTrackRow(cd, w->trackerfy+j);
				src = getVariantRow(w->pbvariantv[0], j);
				for (k = 0; k <= w->pbfx[1] - w->pbfx[0]; k++)
				{
					if (targetmacro+k < 0) continue;
					if (src->macro[w->pbfx[0]-2+k].c)
					{
						dest->macro[targetmacro+k].c =   src->macro[w->pbfx[0]-2+k].c;
						dest->macro[targetmacro+k].v =   src->macro[w->pbfx[0]-2+k].v;
						dest->macro[targetmacro+k].alt = src->macro[w->pbfx[0]-2+k].alt;
					}
				}
			} w->trackerfx = vfxToTfx(targetmacro+(w->pbfx[1] - w->pbfx[0])+2);
		} else
		{
			for (j = 0; j < w->pbvariantv[0]->rowc; j++)
			{
				if (w->trackerfy + j >= s->songlen) break;
				dest = getTrackRow(cd, w->trackerfy+j);
				src = getVariantRow(w->pbvariantv[0], j);
				if (w->pbfx[0] <= -1 && w->pbfx[1] >= -1 && w->vbtrig[0][j].index != VARIANT_VOID)
					cd->variant->trig[w->trackerfy+j] = w->vbtrig[0][j];
				if (w->pbfx[0] <= 0 && w->pbfx[1] >= 0 && src->note != NOTE_VOID) dest->note = src->note;
				if (w->pbfx[0] <= 1 && w->pbfx[1] >= 1 && src->inst != INST_VOID) dest->inst = src->inst;
				for (k = 0; k <= cd->variant->macroc; k++)
					if (w->pbfx[0] <= k+2 && w->pbfx[1] >= k+2 && src->macro[k].c)
					{
						dest->macro[k].c =   src->macro[k].c;
						dest->macro[k].v =   src->macro[k].v;
						dest->macro[k].alt = src->macro[k].alt;
					}
			} w->trackerfx = vfxToTfx(w->pbfx[0]);
		}
	} else
	{
		for (i = 0; i < w->pbtrackc; i++)
		{
			cd = &s->track->v[w->track+i].data;
			if (w->track+i < s->track->c)
				for (j = 0; j < w->pbvariantv[i]->rowc; j++)
				{
					if (w->trackerfy + j >= s->songlen) break;
					dest = getTrackRow(cd, w->trackerfy+j);
					src = getVariantRow(w->pbvariantv[i], j);

					if (i == 0) // first track
					{
						if (w->pbfx[0] <= -1 && w->vbtrig[0][j].index != VARIANT_VOID)
							cd->variant->trig[w->trackerfy+j] = w->vbtrig[i][j];
						if (w->pbfx[0] <= 0 && src->note != NOTE_VOID) dest->note = src->note;
						if (w->pbfx[0] <= 1 && src->inst != INST_VOID) dest->inst = src->inst;
						for (k = 0; k <= cd->variant->macroc; k++)
							if (w->pbfx[0] <= k+2 && src->macro[k].c)
							{
								dest->macro[k].c =   src->macro[k].c;
								dest->macro[k].v =   src->macro[k].v;
								dest->macro[k].alt = src->macro[k].alt;
							}
					} else if (i == w->pbtrackc-1) // last track
					{
						if (w->pbfx[0] >= -1 && w->vbtrig[0][j].index != VARIANT_VOID)
							cd->variant->trig[w->trackerfy+j] = w->vbtrig[i][j];
						if (w->pbfx[1] >= 0 && src->note != NOTE_VOID) dest->note = src->note;
						if (w->pbfx[1] >= 1 && src->inst != INST_VOID) dest->inst = src->inst;
						for (k = 0; k <= cd->variant->macroc; k++)
							if (w->pbfx[1] >= k+2 && src->macro[k].c)
							{
								dest->macro[k].c =   src->macro[k].c;
								dest->macro[k].v =   src->macro[k].v;
								dest->macro[k].alt = src->macro[k].alt;
							}
					} else // middle track
					{
						if (w->vbtrig[0][j].index != VARIANT_VOID)
							cd->variant->trig[w->trackerfy+j] = w->vbtrig[i][j];
						if (src->note != NOTE_VOID) dest->note = src->note;
						if (src->inst != INST_VOID) dest->inst = src->inst;
						for (k = 0; k <= cd->variant->macroc; k++)
							if (src->macro[k].c)
							{
								dest->macro[k].c =   src->macro[k].c;
								dest->macro[k].v =   src->macro[k].v;
								dest->macro[k].alt = src->macro[k].alt;
							}
					}
				}
			else break;
		} w->trackerfx = vfxToTfx(w->pbfx[0]);
	} regenGlobalRowc(s);
}
void delPartPattern(int8_t x1, int8_t x2, short y1, short y2, uint8_t c1, uint8_t c2)
{
	uint8_t i, j;
	Row *r;
	int k;
	if (c1 == c2) /* only one track */
		for (j = y1; j <= y2; j++)
		{
			if (x1 <= -1 && x2 >= -1) setVariantChainTrig(&s->track->v[c1].data.variant, j, VARIANT_VOID);
			r = getTrackRow(&s->track->v[c1].data, j);
			if (x1 <= 0 && x2 >= 0) r->note = NOTE_VOID;
			if (x1 <= 1 && x2 >= 1) r->inst = INST_VOID;
			for (k = 0; k <= s->track->v[c1].data.variant->macroc; k++)
				if (x1 <= k+2 && x2 >= k+2)
				{
					r->macro[k].c   = 0;
					r->macro[k].v   = 0;
					r->macro[k].alt = 0;
				}
		}
	else for (i = c1; i <= c2; i++)
		if (i < s->track->c)
			for (j = y1; j <= y2; j++)
			{
				if (i == c1) /* first track */
				{
					if (x1 <= -1) setVariantChainTrig(&s->track->v[i].data.variant, j, VARIANT_VOID);
					r = getTrackRow(&s->track->v[i].data, j);
					if (x1 <= 0) r->note = NOTE_VOID;
					if (x1 <= 1) r->inst = INST_VOID;
					for (k = 0; k <= s->track->v[i].data.variant->macroc; k++)
						if (x1 <= k+2)
						{
							r->macro[k].c   = 0;
							r->macro[k].v   = 0;
							r->macro[k].alt = 0;
						}
				} else if (i == c2) /* last track */
				{
					if (x1 >= -1) setVariantChainTrig(&s->track->v[i].data.variant, j, VARIANT_VOID);
					r = getTrackRow(&s->track->v[i].data, j);
					if (x2 >= 0) r->note = NOTE_VOID;
					if (x2 >= 1) r->inst = INST_VOID;
					for (k = 0; k <= s->track->v[i].data.variant->macroc; k++)
						if (x2 >= k+2)
						{
							r->macro[k].c   = 0;
							r->macro[k].v   = 0;
							r->macro[k].alt = 0;
						}
				} else /* middle track */
				{
					setVariantChainTrig(&s->track->v[i].data.variant, j, VARIANT_VOID);
					r = getTrackRow(&s->track->v[i].data, j);
					memset(r, 0, sizeof(Row));
					r->note = NOTE_VOID;
					r->inst = INST_VOID;
				}
			}
		else break;
	regenGlobalRowc(s);
}
void loopPartPattern(int8_t x1, int8_t x2, short y1, short y2, uint8_t c1, uint8_t c2)
{
	if (c1 == c2) /* only one track */
		for (uint8_t j = y1; j <= y2; j++)
		{
			if (x1 <= -1 && x2 >= -1 && s->track->v[c1].data.variant->trig[j].index != VARIANT_VOID)
				s->track->v[c1].data.variant->trig[j].flags ^= C_VTRIG_LOOP;
		}
	else for (uint8_t i = c1; i <= c2; i++)
		if (i < s->track->c)
			for (uint8_t j = y1; j <= y2; j++)
			{
				if ((i == c1 && x1 <= -1) /* first track */
				 || (i != c1))            /* other track */
					if (s->track->v[i].data.variant->trig[j].index != VARIANT_VOID)
						s->track->v[i].data.variant->trig[j].flags ^= C_VTRIG_LOOP;
			}
		else break;
	regenGlobalRowc(s);
}
/* block inc/dec */
void addPartPattern(signed char value, int8_t x1, int8_t x2, short y1, short y2, uint8_t c1, uint8_t c2, bool noteonly, bool affectvtrig)
{
	uint8_t i, j;
	int k;
	Row *r;
	if (c1 == c2) /* only one track */
	{
		for (j = y1; j <= y2; j++)
		{
			r = getTrackRow(&s->track->v[c1].data, j);
			if (x1 <= 0 && x2 >= 0 && r->note != NOTE_VOID && r->note != NOTE_OFF) r->note += value;
		if (noteonly) continue;
			if (x1 <= -1 && x2 >= -1 && affectvtrig) setVariantChainTrig(&s->track->v[c1].data.variant, j, s->track->v[c1].data.variant->trig[j].index + value);
			if (x1 <= 1 && x2 >= 1 && r->inst != INST_VOID) r->inst += value;
			for (k = 0; k <= s->track->v[c1].data.variant->macroc; k++)
				if (x1 <= k+2 && x2 >= k+2)
				{
					if (linkMacroNibbles(r->macro[k].c)) r->macro[k].v += value*16;
					else                                 r->macro[k].v += value;
				}
		}
	} else
		for (i = c1; i <= c2; i++)
			if (i < s->track->c)
				for (j = y1; j <= y2; j++)
				{
					r = getTrackRow(&s->track->v[i].data, j);
					if (i == c1) /* first track */
					{
						if (x1 <= 0 && r->note != NOTE_VOID && r->note != NOTE_OFF) r->note += value;
					if (noteonly) continue;
						if (x1 <= -1 && x2 >= -1 && affectvtrig) setVariantChainTrig(&s->track->v[i].data.variant, j, s->track->v[i].data.variant->trig[j].index + value);
						if (x1 <= 1 && r->inst != INST_VOID) r->inst += value;
						for (k = 0; k <= s->track->v[i].data.variant->macroc; k++)
							if (x1 <= k+2)
							{
								if (linkMacroNibbles(r->macro[k].c)) r->macro[k].v += value*16;
								else                                 r->macro[k].v += value;
							}
					} else if (i == c2) /* last track */
					{
						if (x2 >= 0 && r->note != NOTE_VOID && r->note != NOTE_OFF) r->note += value;
					if (noteonly) continue;
						if (x1 <= -1 && x2 >= -1 && affectvtrig) setVariantChainTrig(&s->track->v[i].data.variant, j, s->track->v[i].data.variant->trig[j].index + value);
						if (x2 >= 1 && r->inst != INST_VOID) r->inst += value;
						for (k = 0; k <= s->track->v[i].data.variant->macroc; k++)
							if (x2 >= k+2)
							{
								if (linkMacroNibbles(r->macro[k].c)) r->macro[k].v += value*16;
								else                                 r->macro[k].v += value;
							}
					} else /* middle track */
					{
						if (r->note != NOTE_VOID && r->note != NOTE_OFF) r->note += value;
					if (noteonly) continue;
						if (affectvtrig) setVariantChainTrig(&s->track->v[i].data.variant, j, s->track->v[i].data.variant->trig[j].index + value);
						if (r->inst != INST_VOID) r->inst += value;
						for (k = 0; k <= s->track->v[i].data.variant->macroc; k++)
						{
							if (linkMacroNibbles(r->macro[k].c)) r->macro[k].v += value*16;
							else                                 r->macro[k].v += value;
						}
					}
				}
			else break;
}
/* block toggle case */
void tildePartPattern(int8_t x1, int8_t x2, short y1, short y2, uint8_t c1, uint8_t c2)
{
	uint8_t i, j;
	int k;
	Row *r;
	if (c1 == c2) /* only one track */
	{
		for (j = y1; j <= y2; j++)
		{
			r = getTrackRow(&s->track->v[c1].data, j);
			for (k = 0; k <= s->track->v[c1].data.variant->macroc; k++)
				if (x1 <= k+2 && x2 >= k+2 && isalpha(r->macro[k].c))
					changeMacro(r->macro[k].c, &r->macro[k].c, &r->macro[k].alt, r->macro[k].alt);
		}
	} else
		for (i = c1; i <= c2; i++)
			if (i < s->track->c)
				for (j = y1; j <= y2; j++)
				{
					r = getTrackRow(&s->track->v[i].data, j);
					if (i == c1) /* first track */
						for (k = 0; k <= s->track->v[i].data.variant->macroc; k++) {
							if (x1 <= k+2 && isalpha(r->macro[k].c))
								changeMacro(r->macro[k].c, &r->macro[k].c, &r->macro[k].alt, r->macro[k].alt); }
					else if (i == c2) /* last track */
						for (k = 0; k <= s->track->v[i].data.variant->macroc; k++) {
							if (x2 >= k+2 && isalpha(r->macro[k].c))
								changeMacro(r->macro[k].c, &r->macro[k].c, &r->macro[k].alt, r->macro[k].alt); }
					else /* middle track */
						for (k = 0; k <= s->track->v[i].data.variant->macroc; k++)
							if (isalpha(r->macro[k].c))
								changeMacro(r->macro[k].c, &r->macro[k].c, &r->macro[k].alt, r->macro[k].alt);
				}
			else break;
	regenGlobalRowc(s);
}
/* block toggle alt */
void altTildePartPattern(int8_t x1, int8_t x2, short y1, short y2, uint8_t c1, uint8_t c2)
{
	uint8_t i, j;
	int k;
	Row *r;
	if (c1 == c2) /* only one track */
	{
		for (j = y1; j <= y2; j++)
		{
			r = getTrackRow(&s->track->v[c1].data, j);
			for (k = 0; k <= s->track->v[c1].data.variant->macroc; k++)
				if (x1 <= k+2 && x2 >= k+2 && isalpha(r->macro[k].c))
					changeMacro('\0', &r->macro[k].c, &r->macro[k].alt, !r->macro[k].alt);
		}
	} else
		for (i = c1; i <= c2; i++)
			if (i < s->track->c)
				for (j = y1; j <= y2; j++)
				{
					r = getTrackRow(&s->track->v[i].data, j);
					if (i == c1) /* first track */
						for (k = 0; k <= s->track->v[i].data.variant->macroc; k++) {
							if (x1 <= k+2 && isalpha(r->macro[k].c))
								changeMacro('\0', &r->macro[k].c, &r->macro[k].alt, !r->macro[k].alt); }
					else if (i == c2) /* last track */
						for (k = 0; k <= s->track->v[i].data.variant->macroc; k++) {
							if (x2 >= k+2 && isalpha(r->macro[k].c))
								changeMacro('\0', &r->macro[k].c, &r->macro[k].alt, !r->macro[k].alt); }
					else /* middle track */
						for (k = 0; k <= s->track->v[i].data.variant->macroc; k++)
							if (isalpha(r->macro[k].c))
								changeMacro('\0', &r->macro[k].c, &r->macro[k].alt, !r->macro[k].alt);
				}
			else break;
	regenGlobalRowc(s);
}
/* block interpolate */
void interpolatePartPattern(int8_t x1, int8_t x2, short y1, short y2, uint8_t c1, uint8_t c2)
{
	uint8_t i, j;
	int k;
	Row *r, *r1, *r2;
	if (c1 == c2) /* only one track */
	{
		r1 = getTrackRow(&s->track->v[c1].data, y1);
		r2 = getTrackRow(&s->track->v[c1].data, y2);
		for (j = y1; j <= y2; j++)
		{
			r = getTrackRow(&s->track->v[c1].data, j);
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
			for (k = 0; k <= s->track->v[c1].data.variant->macroc; k++)
				if (x1 <= k+2 && x2 >= k+2)
				{
					r->macro[k].c = (r1->macro[k].c) ?  r1->macro[k].c : r2->macro[k].c;
					r->macro[k].v =  r1->macro[k].v + ((r2->macro[k].v - r1->macro[k].v) / (float)(y2-y1)) * (j-y1);
				}
		}
	} else
		for (i = c1; i <= c2; i++)
			if (i < s->track->c) {
				r1 = getTrackRow(&s->track->v[i].data, y1);
				r2 = getTrackRow(&s->track->v[i].data, y2);
				for (j = y1; j <= y2; j++)
				{
					r = getTrackRow(&s->track->v[i].data, j);
					if (i == c1) /* first track */
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
						for (k = 0; k <= s->track->v[c1].data.variant->macroc; k++)
							if (x1 <= k+2)
							{
								r->macro[k].c = (r1->macro[k].c) ?  r1->macro[k].c : r2->macro[k].c;
								r->macro[k].v =  r1->macro[k].v + ((r2->macro[k].v - r1->macro[k].v) / (float)(y2-y1)) * (j-y1);
							}
					}
					else if (i == c2) /* last track */
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
						for (k = 0; k <= s->track->v[i].data.variant->macroc; k++)
							if (x2 >= k+2)
							{
								r->macro[k].c = (r1->macro[k].c) ?  r1->macro[k].c : r2->macro[k].c;
								r->macro[k].v =  r1->macro[k].v + ((r2->macro[k].v - r1->macro[k].v) / (float)(y2-y1)) * (j-y1);
							}
					}
					else /* middle track */
					{
						if      (r1->note == NOTE_VOID) r->note = r2->note;
						else if (r2->note == NOTE_VOID) r->note = r1->note;
						else r->note = r1->note + ((r2->note - r1->note) / (float)(y2-y1)) * (j-y1);

						if      (r1->inst == INST_VOID) r->inst = r2->inst;
						else if (r2->inst == INST_VOID) r->inst = r1->inst;
						else r->inst = r1->inst + ((r2->inst - r1->inst) / (float)(y2-y1)) * (j-y1);

						for (k = 0; k <= s->track->v[i].data.variant->macroc; k++)
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
void randPartPattern(int8_t x1, int8_t x2, short y1, short y2, uint8_t c1, uint8_t c2)
{
	uint8_t i, j, randinst;
	int k;
	Row *r;
	if (c1 == c2) /* only one track */
	{
		for (j = y1; j <= y2; j++)
		{
			r = getTrackRow(&s->track->v[c1].data, j);
			if (x1 <= 0 && x2 >= 0) r->note = MIN(NOTE_A10-1, rand()%36 +MIN(7, w->octave)*12);
			if (x1 <= 1 && x2 >= 1 && r->note != NOTE_VOID)
			{
				r->inst = NOTE_VOID;
				randinst = rand()%(s->instrument->c-1) + 1;
				for (k = 0; k < INSTRUMENT_MAX; k++)
					if (s->instrument->i[k] == randinst) { r->inst = k; break; }
			}
			for (k = 0; k <= s->track->v[c1].data.variant->macroc; k++)
				if (x1 <= k+2 && x2 >= k+2 && r->macro[k].c)
					r->macro[k].v = rand()%256;
		}
	} else
	{
		for (i = c1; i <= c2; i++)
			if (i < s->track->c)
				for (j = y1; j <= y2; j++)
				{
					r = getTrackRow(&s->track->v[i].data, j);
					if (i == c1) /* first track */
					{
						if (x1 <= 0) r->note = MIN(NOTE_A10-1, rand()%36 +MIN(7, w->octave)*12);
						if (x1 <= 1 && r->note != NOTE_VOID)
						{
							r->inst = NOTE_VOID;
							randinst = rand()%(s->instrument->c-1) + 1;
							for (k = 0; k < INSTRUMENT_MAX; k++)
								if (s->instrument->i[k] == randinst) { r->inst = k; break; }
						}
						for (k = 0; k <= s->track->v[i].data.variant->macroc; k++)
							if (x1 <= k+2 && r->macro[k].c)
								r->macro[k].v = rand()%256;
					} else if (i == c2) /* last track */
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
						for (k = 0; k <= s->track->v[i].data.variant->macroc; k++)
							if (x2 >= k+2 && r->macro[k].c)
								r->macro[k].v = rand()%256;
					} else /* middle track */
					{
						r->note = MIN(NOTE_A10-1, rand()%36 +MIN(7, w->octave)*12);
						if (r->note != NOTE_VOID)
						{
							r->inst = NOTE_VOID;
							randinst = rand()%(s->instrument->c-1) + 1;
							for (k = 0; k < INSTRUMENT_MAX; k++)
								if (s->instrument->i[k] == randinst) { r->inst = k; break; }
						}
						for (k = 0; k <= s->track->v[i].data.variant->macroc; k++)
							if (r->macro[k].c)
								r->macro[k].v = rand()%256;
					}
				}
			else break;
	} regenGlobalRowc(s);
}
void cycleUpPartPattern(int8_t x1, int8_t x2, short y1, short y2, uint8_t c1, uint8_t c2)
{
	uint8_t i;
	int j, k, l;
	Row hold, *r0, *r1, *r2;
	if (c1 == c2) /* only one track */
	{
		for (l = 0; l < MAX(1, w->count); l++)
		{
			hold = *getTrackRow(&s->track->v[c1].data, y1);
			r2 =    getTrackRow(&s->track->v[c1].data, y2);
			if (x1 <= 0 && x2 >= 0)
			{
				for (j = y1; j < y2; j++)
				{
					r0 = getTrackRow(&s->track->v[c1].data, j+0);
					r1 = getTrackRow(&s->track->v[c1].data, j+1);
					r0->note = r1->note;
				}
				r2->note = hold.note;
			}
			if (x1 <= 1 && x2 >= 1)
			{
				for (j = y1; j < y2; j++)
				{
					r0 = getTrackRow(&s->track->v[c1].data, j+0);
					r1 = getTrackRow(&s->track->v[c1].data, j+1);
					r0->inst = r1->inst;
				}
				r2->inst = hold.inst;
			}
			for (k = 0; k <= s->track->v[c1].data.variant->macroc; k++)
				if (x1 <= k+2 && x2 >= k+2)
				{
					for (j = y1; j < y2; j++)
					{
						r0 = getTrackRow(&s->track->v[c1].data, j+0);
						r1 = getTrackRow(&s->track->v[c1].data, j+1);
						r0->macro[k].c =   r1->macro[k].c;
						r0->macro[k].v =   r1->macro[k].v;
						r0->macro[k].alt = r1->macro[k].alt;
					}
					r2->macro[k].c =   hold.macro[k].c;
					r2->macro[k].v =   hold.macro[k].v;
					r2->macro[k].alt = hold.macro[k].alt;
				}
		}
	} else
	{
		for (i = c1; i <= c2; i++)
			if (i < s->track->c)
			{
				hold = *getTrackRow(&s->track->v[i].data, y1);
				r2 =    getTrackRow(&s->track->v[i].data, y2);
				if (i == c1) /* first track */
					for (l = 0; l < MAX(1, w->count); l++)
					{
						if (x1 <= 0)
						{
							for (j = y1; j < y2; j++)
							{
								r0 = getTrackRow(&s->track->v[i].data, j+0);
								r1 = getTrackRow(&s->track->v[i].data, j+1);
								r0->note = r1->note;
							}
							r2->note = hold.note;
						}
						if (x1 <= 1)
						{
							for (j = y1; j < y2; j++)
							{
								r0 = getTrackRow(&s->track->v[i].data, j+0);
								r1 = getTrackRow(&s->track->v[i].data, j+1);
								r0->inst = r1->inst;
							}
							r2->inst = hold.inst;
						}
						for (k = 0; k <= s->track->v[c1].data.variant->macroc; k++)
							if (x1 <= k+2)
							{
								for (j = y1; j < y2; j++)
								{
									r0 = getTrackRow(&s->track->v[i].data, j+0);
									r1 = getTrackRow(&s->track->v[i].data, j+1);
									r0->macro[k].c =   r1->macro[k].c;
									r0->macro[k].v =   r1->macro[k].v;
									r0->macro[k].alt = r1->macro[k].alt;
								}
								r2->macro[k].c =   hold.macro[k].c;
								r2->macro[k].v =   hold.macro[k].v;
								r2->macro[k].alt = hold.macro[k].alt;
							}
					}
				else if (i == c2) /* last track */
					for (l = 0; l < MAX(1, w->count); l++)
					{
						if (x2 >= 0)
						{
							for (j = y1; j < y2; j++)
							{
								r0 = getTrackRow(&s->track->v[i].data, j+0);
								r1 = getTrackRow(&s->track->v[i].data, j+1);
								r0->note = r1->note;
							}
							r2->note = hold.note;
						}
						if (x2 >= 1)
						{
							for (j = y1; j < y2; j++)
							{
								r0 = getTrackRow(&s->track->v[i].data, j+0);
								r1 = getTrackRow(&s->track->v[i].data, j+1);
								r0->inst = r1->inst;
							}
							r2->inst = hold.inst;
						}
						for (k = 0; k <= s->track->v[c2].data.variant->macroc; k++)
							if (x2 >= k+2)
							{
								for (j = y1; j < y2; j++)
								{
									r0 = getTrackRow(&s->track->v[i].data, j+0);
									r1 = getTrackRow(&s->track->v[i].data, j+1);
									r0->macro[k].c =   r1->macro[k].c;
									r0->macro[k].v =   r1->macro[k].v;
									r0->macro[k].alt = r1->macro[k].alt;
								}
								r2->macro[k].c   = hold.macro[k].c;
								r2->macro[k].v   = hold.macro[k].v;
								r2->macro[k].alt = hold.macro[k].alt;
							}
					}
				else /* middle track */
					for (l = 0; l < MAX(1, w->count); l++)
					{
						for (j = y1; j < y2; j++)
						{
							r0 = getTrackRow(&s->track->v[i].data, j+0);
							r1 = getTrackRow(&s->track->v[i].data, j+1);
							*r0 = *r1;
						}
						*r2 = hold;
					}
			} else break;
	} regenGlobalRowc(s);
}
void cycleDownPartPattern(int8_t x1, int8_t x2, short y1, short y2, uint8_t c1, uint8_t c2)
{
	uint8_t i;
	int j, k, l;
	Row hold, *r0, *r1, *r2;
	if (c1 == c2) /* only one track */
	{
		for (l = 0; l < MAX(1, w->count); l++)
		{
			hold = *getTrackRow(&s->track->v[c1].data, y2);
			r2 =    getTrackRow(&s->track->v[c1].data, y1);

			if (x1 <= 0 && x2 >= 0)
			{
				for (j = y2-1; j >= y1; j--)
				{
					r0 = getTrackRow(&s->track->v[c1].data, j+1);
					r1 = getTrackRow(&s->track->v[c1].data, j+0);
					r0->note = r1->note;
				}
				r2->note = hold.note;
			}
			if (x1 <= 1 && x2 >= 1)
			{
				for (j = y2-1; j >= y1; j--)
				{
					r0 = getTrackRow(&s->track->v[c1].data, j+1);
					r1 = getTrackRow(&s->track->v[c1].data, j+0);
					r0->inst = r1->inst;
				}
				r2->inst = hold.inst;
			}
			for (k = 0; k <= s->track->v[c1].data.variant->macroc; k++)
				if (x1 <= k+2 && x2 >= k+2)
				{
					for (j = y2-1; j >= y1; j--)
					{
						r0 = getTrackRow(&s->track->v[c1].data, j+1);
						r1 = getTrackRow(&s->track->v[c1].data, j+0);
						r0->macro[k].c =   r1->macro[k].c;
						r0->macro[k].v =   r1->macro[k].v;
						r0->macro[k].alt = r1->macro[k].alt;
					}
					r2->macro[k].c =   hold.macro[k].c;
					r2->macro[k].v =   hold.macro[k].v;
					r2->macro[k].alt = hold.macro[k].alt;
				}
		}
	} else
	{
		for (i = c1; i <= c2; i++)
			if (i < s->track->c)
			{
				hold = *getTrackRow(&s->track->v[i].data, y2);
				r2 =    getTrackRow(&s->track->v[i].data, y1);

				if (i == c1) /* first track */
					for (l = 0; l < MAX(1, w->count); l++)
					{
						if (x1 <= 0)
						{
							for (j = y2-1; j >= y1; j--)
							{
								r0 = getTrackRow(&s->track->v[i].data, j+1);
								r1 = getTrackRow(&s->track->v[i].data, j+0);
								r0->note = r1->note;
							}
							r2->note = hold.note;
						}
						if (x1 <= 1)
						{
							for (j = y2-1; j >= y1; j--)
							{
								r0 = getTrackRow(&s->track->v[i].data, j+1);
								r1 = getTrackRow(&s->track->v[i].data, j+0);
								r0->inst = r1->inst;
							}
							r2->inst = hold.inst;
						}
						for (k = 0; k <= s->track->v[c1].data.variant->macroc; k++)
							if (x1 <= k+2)
							{
								for (j = y2-1; j >= y1; j--)
								{
									r0 = getTrackRow(&s->track->v[i].data, j+1);
									r1 = getTrackRow(&s->track->v[i].data, j+0);
									r0->macro[k].c =   r1->macro[k].c;
									r0->macro[k].v =   r1->macro[k].v;
									r0->macro[k].alt = r1->macro[k].alt;
								}
								r2->macro[k].c =   hold.macro[k].c;
								r2->macro[k].v =   hold.macro[k].v;
								r2->macro[k].alt = hold.macro[k].alt;
							}
					}
				else if (i == c2) /* last track */
					for (l = 0; l < MAX(1, w->count); l++)
					{
						if (x2 >= 0)
						{
							for (j = y2-1; j >= y1; j--)
							{
								r0 = getTrackRow(&s->track->v[i].data, j+1);
								r1 = getTrackRow(&s->track->v[i].data, j+0);
								r0->note = r1->note;
							}
							r2->note = hold.note;
						}
						if (x2 >= 1)
						{
							for (j = y2-1; j >= y1; j--)
							{
								r0 = getTrackRow(&s->track->v[i].data, j+1);
								r1 = getTrackRow(&s->track->v[i].data, j+0);
								r0->inst = r1->inst;
							}
							r2->inst = hold.inst;
						}
						for (k = 0; k <= s->track->v[c2].data.variant->macroc; k++)
							if (x2 >= k+2)
							{
								for (j = y2-1; j >= y1; j--)
								{
									r0 = getTrackRow(&s->track->v[i].data, j+1);
									r1 = getTrackRow(&s->track->v[i].data, j+0);
									r0->macro[k].c =   r1->macro[k].c;
									r0->macro[k].v =   r1->macro[k].v;
									r0->macro[k].alt = r1->macro[k].alt;
								}
								r2->macro[k].c =   hold.macro[k].c;
								r2->macro[k].v =   hold.macro[k].v;
								r2->macro[k].alt = hold.macro[k].alt;
							}
					}
				else /* middle track */
					for (l = 0; l < MAX(1, w->count); l++)
					{
						for (j = y2-1; j >= y1; j--)
						{
							r0 = getTrackRow(&s->track->v[i].data, j+1);
							r1 = getTrackRow(&s->track->v[i].data, j+0);
							*r0 = *r1;
						}
						*r2 = hold;
					}
			} else break;
	} regenGlobalRowc(s);
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

	/* allocate temp track state */
	Track *cv;
	cv = calloc(c2-c1 + 1, sizeof(Track));
	for (chnl = 0; chnl <= c2-c1; chnl++)
	{
		_addTrack(s, &cv[chnl]);
		copyTrackdata(&cv[chnl].data, &s->track->v[c1+chnl].data);
	}

	/* calculate the buffer length needed */
	setBpm(&spr, s->songbpm);
	for (row = 0; row < y1; row++)
		for (chnl = 0; chnl <= c2-c1; chnl++)
			ifMacro(0, &spr, &cv[chnl], *getTrackRow(&cv[chnl].data, row), 'B', 0, &Bc);
	buflen = 0;
	for (row = y1; row <= y2; row++)
	{
		for (chnl = 0; chnl <= c2-c1; chnl++)
			ifMacro(0, &spr, &cv[chnl], *getTrackRow(&cv[chnl].data, row), 'B', 0, &Bc);
		buflen += spr;
	}

	/* allocate the sample buffer */
	Sample *sample = malloc(sizeof(Sample) + (buflen<<1) * sizeof(short));
	sample->length = buflen;
	sample->tracks = 2;
	sample->rate = sample->defrate = samplerate;

	/* lookback */
	setBpm(&spr, s->songbpm);
	for (chnl = 0; chnl <= c2-c1; chnl++)
		lookback(0, &spr, y1, &cv[chnl]);

	bufptr = 0;
	Row *r;
	for (row = y1; row <= y2; row++)
	{
		for (chnl = 0; chnl <= c2-c1; chnl++)
		{
			r = getTrackRow(&cv[chnl].data, row);
			processRow(0, &spr, 0, &cv[chnl], *r);
		}
		if (s->bpmcachelen > row && s->bpmcache[row] != -1) Bc(fptr, &spr, s->bpmcache[row], NULL, *r);

		sprp = 0;
		while (sprp < spr)
		{
			for (fptr = 0; fptr < buffersize; fptr++)
			{
				if (sprp >= spr) break;
				for (chnl = 0; chnl <= c2-c1; chnl++)
				{
					playTrack(fptr, &spr, fptr, &cv[chnl]); /* TODO: shouldn't trigger midi */
					sample->data[((bufptr+sprp)<<1)+0] += cv[chnl].output[0][fptr]*cv[chnl].mainmult[0][fptr]*BOUNCE_SCALE*SHRT_MAX;
					sample->data[((bufptr+sprp)<<1)+1] += cv[chnl].output[1][fptr]*cv[chnl].mainmult[1][fptr]*BOUNCE_SCALE*SHRT_MAX;
				} sprp++;
			}
		} bufptr += sprp;
	}

	/* run channnel insert effects then free temp track state */
	for (chnl = 0; chnl <= c2-c1; chnl++)
	{
		/* TODO: do this buffersize-wise, using buflen will overflow the plugin ports */
		for (uint8_t i = 0; i < cv[chnl].data.effect->c; i++)
			runEffect(buflen, cv[chnl].data.effect, &cv[chnl].data.effect->v[i]);

		_delTrack(s, &cv[chnl]);
	} free(cv);

	/* init the instrument */
	addReparentInstrument(inst, 0, sample);

	w->instrument = inst;
}
