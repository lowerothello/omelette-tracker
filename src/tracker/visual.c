#define FOR_BUFFER_TRACKS(iter, cv, xstart, xend)\
	Track *cv;\
	int8_t xstart, xend;\
	for (uint8_t iter = 0; iter < w->pbtrackc; i++)\
	{\
		cv = s->track->v[w->track+iter];\
		xstart = 0;\
		xend = cv->pattern->macroc+3;\
		if (iter == 0) xstart = w->pbfx[0];\
		if (iter == w->pbtrackc-1) xend = w->pbfx[1];
	//}

/* expects x1, x2, c1, c2 to be set to the visual range */
#define FOR_VISUAL_TRACKS(iter, cv, xstart, xend)\
	Track *cv;\
	int8_t xstart, xend;\
	for (uint8_t iter = c1; iter <= c2; iter++)\
	{\
		if (iter >= s->track->c) break;\
		cv = s->track->v[iter];\
		xstart = 0;\
		xend = cv->pattern->macroc+3;\
		if (iter == c1) xstart = x1;\
		if (iter == c2) xend = x2;
	//}

short tfxToVfx(int8_t tfx) { if (tfx > 1) return 2 + (tfx - 2) / 2; return tfx; }
short vfxToTfx(int8_t vfx) { if (vfx > 1) return 2 + (vfx - 2) * 2; return vfx; }

/* VMO: visual macro order */
short tfxToVmo(Track *cv, short tfx)
{
	if (tfx < 2) return tfx; /* no change for note and inst columns */
	if (tfx&0x1) /* macrov */ return (4 + (cv->pattern->macroc<<1)) - tfx;
	else         /* macroc */ return (2 + (cv->pattern->macroc<<1)) - tfx;
}
short vfxToVmo(Track *cv, short vfx)
{
	if (vfx < 2) return vfx; /* no change for note and inst columns */
	return (2 + (cv->pattern->macroc<<1)) - vfx;
}

short vfxVmoMin(short x1, short x2)
{
	if (x1 < 2 || x2 < 2) return MIN(x1, x2); /* either is not a macro */
	return MAX(x1, x2); /* both are macros */
}
short vfxVmoMax(short x1, short x2)
{
	if (x1 < 2 || x2 < 2) return MAX(x1, x2); /* either is not a macro */
	return MIN(x1, x2); /* both are macros */
}

/* returns true if (x >= min && x <= max) in visual macro order */
bool vfxVmoRangeIncl(short min, short max, short x)
{
	if (min > 1) /* range is all macros */
		return (x <= min && x >= max); /* fully inverted */

	if (max > 1) /* range goes from non-macro to macro */
	{
		if (x > 1) /* x is in the macro part     */ return (x >= max);
		else       /* x is in the non-macro part */ return (x >= min);
	}

	/* range goes from non-macro to non-macro */
	return (x >= min && x <= max); /* not inverted */
}

void yankPartPattern(int8_t x1, int8_t x2, short y1, short y2, uint8_t c1, uint8_t c2)
{
	// /* walk over allocated tracks and free them */
	// for (short i = 0; i < w->pbtrackc; i++)
	// 	free(w->pbvariantv[i]);
	//
	// w->pbfx[0] = x1;
	// w->pbfx[1] = x2;
	// w->pbtrackc = c2-c1 + 1;
	//
	// Track *cv;
	// Row *r;
	// for (uint8_t i = 0; i <= c2-c1; i++)
	// {
	// 	cv = s->track->v[c1+i];
	// 	w->pbvariantv[i] = dupVariant(NULL, y2-y1 + 1);
	// 	for (uint16_t j = 0; j <= y2-y1; j++)
	// 	{
	// 		r = getTrackRow(cv->pattern, y1+j, 0);
	// 		if (r)
	// 			w->pbvariantv[i]->rowv[j] = *r;
	// 	}
	// }
}
void yankPartPatternOrder(short y1, short y2, uint8_t c1, uint8_t c2)
{
	// p->redraw = 1;
}

void putPartPattern(bool step) /* TODO: count */
{
	// uint8_t j;
	// int k;
	// Row *dest, *src;
	// char targetmacro;
	//
	// if (!w->pbtrackc) return;
	// FOR_BUFFER_TRACKS(i, cv, xstart, xend) // {
	// 	if (xstart > 1 && xend > 1) /* just macro columns */
	// 	{
	// 		if (w->trackerfx < 2) targetmacro = 0;
	// 		else                  targetmacro = tfxToVfx(w->trackerfx)-2;
	//
	// 		targetmacro -= xend - xstart;
	//
	// 		for (j = 0; j < w->pbvariantv[0]->rowc; j++)
	// 		{
	// 			dest = getTrackRow(cv->pattern, w->trackerfy+j, 1);
	// 			src = getVariantRow(w->pbvariantv[0], j);
	// 			for (k = 0; k <= xend - xstart; k++)
	// 			{
	// 				if (targetmacro+k < 0) continue;
	// 				memcpy(&dest->macro[targetmacro+k], &src->macro[xstart-2+k], sizeof(Macro));
	// 			}
	// 		}
	// 	} else
	// 	{
	// 		for (j = 0; j < w->pbvariantv[0]->rowc; j++)
	// 		{
	// 			dest = getTrackRow(cv->pattern, w->trackerfy+j, 1);
	// 			src = getVariantRow(w->pbvariantv[0], j);
	// 			if (xstart <= 0 && xend >= 0) dest->note = src->note;
	// 			if (xstart <= 1 && xend >= 1) dest->inst = src->inst;
	// 			for (k = 0; k <= cv->pattern->macroc; k++)
	// 				if (xstart <= k+2 && xend >= k+2)
	// 					memcpy(&dest->macro[k], &src->macro[k], sizeof(Macro));
	// 		}
	// 	}
	// }
	//
	// if (step)
	// 	trackerDownArrow(w->pbvariantv[0]->rowc);
	// p->redraw = 1;
}

void mixPutPartPattern(bool step) /* TODO: count */
{
	// uint8_t j;
	// int k;
	// Row *dest, *src;
	// char targetmacro;
	//
	// if (!w->pbtrackc) return;
	// FOR_BUFFER_TRACKS(i, cv, xstart, xend) // {
	// 	if (xstart > 1 && xend > 1) /* just macro columns */
	// 	{
	// 		if (w->trackerfx < 2) targetmacro = 0;
	// 		else                  targetmacro = tfxToVfx(w->trackerfx)-2;
	//
	// 		targetmacro -= xend - xstart;
	//
	// 		for (j = 0; j < w->pbvariantv[0]->rowc; j++)
	// 		{
	// 			dest = getTrackRow(cv->pattern, w->trackerfy+j, 1);
	// 			src = getVariantRow(w->pbvariantv[0], j);
	// 			for (k = 0; k <= xend - xstart; k++)
	// 			{
	// 				if (targetmacro+k < 0) continue;
	// 				if (src->macro[xstart-2+k].c)
	// 					memcpy(&dest->macro[targetmacro+k], &src->macro[xstart-2+k], sizeof(Macro));
	// 			}
	// 		} w->trackerfx = vfxToTfx(targetmacro+(xend - xstart)+2);
	// 	} else
	// 	{
	// 		for (j = 0; j < w->pbvariantv[0]->rowc; j++)
	// 		{
	// 			dest = getTrackRow(cv->pattern, w->trackerfy+j, 1);
	// 			src = getVariantRow(w->pbvariantv[0], j);
	// 			if (xstart <= 0 && xend >= 0 && src->note != NOTE_VOID) dest->note = src->note;
	// 			if (xstart <= 1 && xend >= 1 && src->inst != INST_VOID) dest->inst = src->inst;
	// 			for (k = 0; k <= cv->pattern->macroc; k++)
	// 				if (xstart <= k+2 && xend >= k+2 && src->macro[k].c)
	// 					memcpy(&dest->macro[k], &src->macro[k], sizeof(Macro));
	// 		} w->trackerfx = vfxToTfx(xstart);
	// 	}
	// }
	//
	// if (step)
	// 	trackerDownArrow(w->pbvariantv[0]->rowc);
	// p->redraw = 1;
}

void delPartPattern(int8_t x1, int8_t x2, short y1, short y2, uint8_t c1, uint8_t c2)
{
	uint8_t j;
	Row *r;
	int k;

	FOR_VISUAL_TRACKS(i, cv, xstart, xend) // {
		for (j = y1; j <= y2; j++)
		{
			r = getTrackRow(cv->pattern, j, 0);
			if (!r) continue;
			if (xstart <= 0 && xend >= 0) r->note = NOTE_VOID;
			if (xstart <= 1 && xend >= 1) r->inst = INST_VOID;
			for (k = 0; k <= cv->pattern->macroc; k++)
				if (xstart <= k+2 && xend >= k+2) memset(&r->macro[k], 0, sizeof(Macro));
		}
	}
	p->redraw = 1;
}
void delPartPatternOrder(short y1, short y2, uint8_t c1, uint8_t c2)
{
	// for (i = c1; i <= c2; i++)
	// {
	// 	if (i >= s->track->c) break;
	//
	// 	for (j = y1; j <= y2; j++)
	// 		// _setPatternOrder(&s->track->v[i]->pattern, j, PATTERN_VOID); /* TODO: can't be atomic cos it would overflow the queue pretty badly */
	// 		setPatternOrder(&s->track->v[i]->pattern, j, PATTERN_VOID);
	// }
	setPatternOrderBlock(y1, y2, c1, c2, PATTERN_VOID);
	p->redraw = 1;
}

/* block inc/dec */
void addPartPattern(signed char value, int8_t x1, int8_t x2, short y1, short y2, uint8_t c1, uint8_t c2, bool noteonly)
{
	uint8_t j;
	int k;
	Row *r;

	FOR_VISUAL_TRACKS(i, cv, xstart, xend) // {
		for (j = y1; j <= y2; j++)
		{
			r = getTrackRow(cv->pattern, j, 1);
			if (xstart <= 0 && xend >= 0 && r->note != NOTE_VOID && r->note != NOTE_OFF) r->note = r->note + value;
		if (noteonly) continue;
			if (xstart <= 1 && xend >= 1 && r->inst != INST_VOID) r->inst += value;
			for (k = 0; k <= cv->pattern->macroc; k++)
				if (xstart <= k+2 && xend >= k+2)
				{
					if (MACRO_STEREO(r->macro[k].c)) r->macro[k].v += value*16;
					else                             r->macro[k].v += value;
				}
		}
	}
	p->redraw = 1;
}
void addPartPatternOrder(signed char value, short y1, short y2, uint8_t c1, uint8_t c2)
{
	uint8_t i, j;
	PatternChain *pc;

	for (i = c1; i <= c2; i++)
	{
		if (i >= s->track->c) break;
		pc = s->track->v[i]->pattern;

		for (j = y1; j <= y2; j++)
			/* can't use pc as the first arg to the below call cos it'd be a stale pointer by the time it's followed */
			_setPatternOrder(&s->track->v[i]->pattern, j, pc->order[j] + value); /* TODO: can't be atomic cos it would overflow the queue pretty badly */
	}
	p->redraw = 1;
}
/* block toggle case */
void tildePartPattern(int8_t x1, int8_t x2, short y1, short y2, uint8_t c1, uint8_t c2)
{
	uint8_t j;
	int k;
	Row *r;

	FOR_VISUAL_TRACKS(i, cv, xstart, xend) // {
		for (j = y1; j <= y2; j++)
		{
			r = getTrackRow(cv->pattern, j, 0);
			if (!r) continue;
			if (xstart <= 0 && xend >= 0)
			{
				if (r->note >= NOTE_MIN && r->note <= NOTE_MAX)
					r->note += NOTE_SMOOTH_OFFSET;
				else if (r->note >= NOTE_MIN+NOTE_SMOOTH_OFFSET && r->note <= NOTE_MAX+NOTE_SMOOTH_OFFSET)
					r->note -= NOTE_SMOOTH_OFFSET;
			}
			for (k = 0; k <= cv->pattern->macroc; k++)
				if (xstart <= k+2 && xend >= k+2 && isalpha(r->macro[k].c))
					changeMacro(r->macro[k].c, &r->macro[k].c);
		}
	}
	p->redraw = 1;
}

/* block interpolate */
void interpolatePartPattern(int8_t x1, int8_t x2, short y1, short y2, uint8_t c1, uint8_t c2)
{
	uint8_t j;
	int k;
	Row *r, *r1, *r2;

	FOR_VISUAL_TRACKS(i, cv, xstart, xend) // {
		r1 = getTrackRow(cv->pattern, y1, 1);
		r2 = getTrackRow(cv->pattern, y2, 1);
		for (j = y1; j <= y2; j++)
		{
			r = getTrackRow(cv->pattern, j, 1);
			if (xstart <= 0 && xend >= 0)
			{
				if      (r1->note == NOTE_VOID) r->note = r2->note;
				else if (r2->note == NOTE_VOID) r->note = r1->note;
				else r->note = r1->note + ((r2->note - r1->note) / (float)(y2-y1)) * (j-y1);
			}
			if (xstart <= 1 && xend >= 1)
			{
				if      (r1->inst == INST_VOID) r->inst = r2->inst;
				else if (r2->inst == INST_VOID) r->inst = r1->inst;
				else r->inst = r1->inst + ((r2->inst - r1->inst) / (float)(y2-y1)) * (j-y1);
			}
			for (k = 0; k <= cv->pattern->macroc; k++)
				if (xstart <= k+2 && xend >= k+2)
				{
					r->macro[k].c = (r1->macro[k].c) ?  r1->macro[k].c : r2->macro[k].c;
					r->macro[k].v =  r1->macro[k].v + ((r2->macro[k].v - r1->macro[k].v) / (float)(y2-y1)) * (j-y1);
				}
		}
	}
	p->redraw = 1;
}
/* block randomize */
void randPartPattern(int8_t x1, int8_t x2, short y1, short y2, uint8_t c1, uint8_t c2)
{
	uint8_t j, randinst;
	int k;
	Row *r;

	FOR_VISUAL_TRACKS(i, cv, xstart, xend) // {
		for (j = y1; j <= y2; j++)
		{
			r = getTrackRow(cv->pattern, j, 1);
			if (xstart <= 0 && xend >= 0) r->note = rand()%36 +MIN(w->octave, MAX_OCTAVE)*12;
			if (xstart <= 1 && xend >= 1 && r->note != NOTE_VOID)
			{
				r->inst = NOTE_VOID;
				randinst = rand()%(s->inst->c-1) + 1;
				for (k = 0; k < INSTRUMENT_MAX; k++)
					if (s->inst->i[k] == randinst) { r->inst = k; break; }
			}
			for (k = 0; k <= cv->pattern->macroc; k++)
				if (xstart <= k+2 && xend >= k+2 && r->macro[k].c)
					r->macro[k].v = rand()%256;
		}
	}
	p->redraw = 1;
}
void cycleUpPartPattern(uint8_t count, int8_t x1, int8_t x2, short y1, short y2, uint8_t c1, uint8_t c2)
{
	int j, k, l;
	Row hold, *r0, *r1, *r2;

	FOR_VISUAL_TRACKS(i, cv, xstart, xend) // {
		for (l = 0; l < MAX(1, count); l++)
		{
			hold = *getTrackRow(cv->pattern, y1, 1);
			r2 =    getTrackRow(cv->pattern, y2, 1);
			if (xstart <= 0 && xend >= 0)
			{
				for (j = y1; j < y2; j++)
				{
					r0 = getTrackRow(cv->pattern, j+0, 1);
					r1 = getTrackRow(cv->pattern, j+1, 1);
					r0->note = r1->note;
				}
				r2->note = hold.note;
			}
			if (xstart <= 1 && xend >= 1)
			{
				for (j = y1; j < y2; j++)
				{
					r0 = getTrackRow(cv->pattern, j+0, 1);
					r1 = getTrackRow(cv->pattern, j+1, 1);
					r0->inst = r1->inst;
				}
				r2->inst = hold.inst;
			}
			for (k = 0; k <= cv->pattern->macroc; k++)
				if (xstart <= k+2 && xend >= k+2)
				{
					for (j = y1; j < y2; j++)
					{
						r0 = getTrackRow(cv->pattern, j+0, 1);
						r1 = getTrackRow(cv->pattern, j+1, 1);
						memcpy(&r0->macro[k], &r1->macro[k], sizeof(Macro));
					}
					memcpy(&r2->macro[k], &hold.macro[k], sizeof(Macro));
				}
		}
	}
	p->redraw = 1;
}
void cycleDownPartPattern(uint8_t count, int8_t x1, int8_t x2, short y1, short y2, uint8_t c1, uint8_t c2)
{
	int j, k, l;
	Row hold, *r0, *r1, *r2;

	FOR_VISUAL_TRACKS(i, cv, xstart, xend) // {
		for (l = 0; l < MAX(1, count); l++)
		{
			hold = *getTrackRow(cv->pattern, y2, 1);
			r2 =    getTrackRow(cv->pattern, y1, 1);

			if (xstart <= 0 && xend >= 0)
			{
				for (j = y2-1; j >= y1; j--)
				{
					r0 = getTrackRow(cv->pattern, j+1, 1);
					r1 = getTrackRow(cv->pattern, j+0, 1);
					r0->note = r1->note;
				}
				r2->note = hold.note;
			}
			if (xstart <= 1 && xend >= 1)
			{
				for (j = y2-1; j >= y1; j--)
				{
					r0 = getTrackRow(cv->pattern, j+1, 1);
					r1 = getTrackRow(cv->pattern, j+0, 1);
					r0->inst = r1->inst;
				}
				r2->inst = hold.inst;
			}
			for (k = 0; k <= cv->pattern->macroc; k++)
				if (xstart <= k+2 && xend >= k+2)
				{
					for (j = y2-1; j >= y1; j--)
					{
						r0 = getTrackRow(cv->pattern, j+1, 1);
						r1 = getTrackRow(cv->pattern, j+0, 1);
						memcpy(&r0->macro[k], &r1->macro[k], sizeof(Macro));
					}
					memcpy(&r2->macro[k], &hold.macro[k], sizeof(Macro));
				}
		}
	}
	p->redraw = 1;
}

/* TODO: pretty sure this function is like completely broken :3 */
void bouncePartPattern(short y1, short y2, uint8_t c1, uint8_t c2)
{
	short inst = emptyInst(0);
	if (inst == -1)
	{
		strcpy(w->command.error, "visual render failed, no empty inst slot");
		return;
	}

	uint16_t row;
	uint8_t chnl;
	uint32_t buflen, bufptr, fptr;
	uint16_t spr, sprp;
	Track *cv;

	/* allocate temp track state */
	TrackChain *chain = calloc(1, sizeof(TrackChain) + (c2-c1+1)*sizeof(Track*));

	chain->c = s->track->c;
	for (uint8_t i = 0; i < chain->c; i++)
		chain->v[i] = allocTrack(s, s->track->v[i]);

	/* calculate the buffer length needed */
	setBpm(&spr, s->songbpm);
	for (row = 0; row < y1; row++)
		for (chnl = 0; chnl <= c2-c1; chnl++)
		{
			cv = chain->v[chnl];
			ifMacroCallback(0, &spr, cv, getTrackRow(cv->pattern, row, 0), 'B', macroBpm);
		}
	buflen = 0;
	for (row = y1; row <= y2; row++)
	{
		for (chnl = 0; chnl <= c2-c1; chnl++)
		{
			cv = chain->v[chnl];
			ifMacroCallback(0, &spr, cv, getTrackRow(cv->pattern, row, 0), 'B', macroBpm);
		}
		buflen += spr;
	}

	/* allocate the sample buffer */
	Sample *sample = malloc(sizeof(Sample) + (buflen<<1) * sizeof(short));
	sample->length = buflen;
	sample->channels = 2;
	sample->rate = sample->defrate = samplerate;

	/* lookback */
	setBpm(&spr, s->songbpm);
	for (chnl = 0; chnl <= c2-c1; chnl++)
		lookback(0, &spr, y1, chain->v[chnl]);

	bufptr = 0;
	Row *r;
	for (row = y1; row <= y2; row++)
	{
		for (chnl = 0; chnl <= c2-c1; chnl++)
		{
			cv = chain->v[chnl];
			r = getTrackRow(cv->pattern, row, 0);
			if (r) processRow(0, &spr, 0, cv, r);
		}
		/* TODO: apply master track macros, such as bpm */

		sprp = 0;
		while (sprp < spr)
		{
			for (fptr = 0; fptr < buffersize; fptr++)
			{
				if (sprp >= spr) break;
				for (chnl = 0; chnl <= c2-c1; chnl++)
				{
					playTrack(fptr, &spr, fptr, chain->v[chnl]); /* TODO: shouldn't trigger midi */
					sample->data[((bufptr+sprp)<<1)+0] += chain->v[chnl]->effect->output[0][fptr]*BOUNCE_SCALE*SHRT_MAX;
					sample->data[((bufptr+sprp)<<1)+1] += chain->v[chnl]->effect->output[1][fptr]*BOUNCE_SCALE*SHRT_MAX;
				} sprp++;
			}
		} bufptr += sprp;
	}

	/* run channnel insert effects then free temp track state */
	for (chnl = 0; chnl <= c2-c1; chnl++)
	{
		/* TODO: do this buffersize-wise, using buflen will overflow the plugin ports */
		runEffectChain(buflen, chain->v[chnl]->effect);

		_delTrack(s, chain->v[chnl]);
		free(chain->v[chnl]);
	} free(chain);

	/* init the inst */
	addReparentInst(inst, INST_TYPE_SAMPLER, sample);

	w->instrument = inst;
}
