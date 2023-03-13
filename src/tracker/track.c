/* clears the playback state of a track */
void clearTrackRuntime(Track *cv)
{
	cv->r.note = NOTE_VOID;
	cv->r.inst = INST_VOID;
	cv->file = 0;
	cv->rtrigsamples = 0;
	cv->rtrig_rev = 0;

	cv->gain.base = cv->gain.rand = 0x88;
	cv->gain.target = -1;
	cv->gain.target_rand = 0;

	cv->send.base = cv->send.rand = 0x00;
	cv->send.target = -1;
	cv->send.target_rand = 0;

	cv->filter.mode[0] = cv->filter.mode[1] = 0;
	cv->filter.targetmode[0] = cv->filter.targetmode[1] = -1;

	cv->filter.cut.base = cv->filter.cut.rand = 0xff;
	cv->filter.cut.target = -1;
	cv->filter.cut.target_rand = 0;

	cv->filter.res.base = cv->filter.res.rand = 0x00;
	cv->filter.res.target = -1;
	cv->filter.res.target_rand = 0;

	cv->midiccindex = -1; cv->midicc = 0;
}

/* clears the global variant and frees all local variants */
void initTrackData(Track *cv, uint16_t songlen) /* TODO: should be atomic */
{

	freeVariantChain(&cv->variant);
	cv->variant = calloc(1, sizeof(VariantChain));

	/* resizing NULL will give a zero'ed out variant of size newlen */
	// cv->variant->main = dupVariant(NULL, cs->songlen);
	// cv->variant->trig = calloc(cs->songlen, sizeof(Vtrig));
	resizeVariantChain(cv->variant, songlen);

	memset(cv->variant->i, VARIANT_VOID, sizeof(uint8_t) * VARIANT_MAX);
	cv->variant->macroc = 1;

	cv->mute = 0;

	if (cv->effect) clearEffectChain(cv->effect);
}

void clearTrackData(Track *cv, uint16_t songlen)
{ /* NOT atomic */
	initTrackData(cv, songlen);
	freeVariantChain(&cv->variant);
	if (cv->effect) { free(cv->effect); cv->effect = NULL; }
}

void addTrackRuntime(Track *cv)
{
	cv->rampindex = rampmax;
	cv->rampbuffer = malloc(sizeof(float) * rampmax * 2); /* *2 for stereo */
	cv->output[0] = calloc(buffersize, sizeof(float));
	cv->output[1] = calloc(buffersize, sizeof(float));
	cv->pluginoutput[0] = calloc(buffersize, sizeof(float));
	cv->pluginoutput[1] = calloc(buffersize, sizeof(float));
	cv->mainmult[0] = calloc(buffersize, sizeof(float));
	cv->mainmult[1] = calloc(buffersize, sizeof(float));
	cv->sendmult[0] = calloc(buffersize, sizeof(float));
	cv->sendmult[1] = calloc(buffersize, sizeof(float));
	clearTrackRuntime(cv);
}

void addTrackData(Track *cv, uint16_t songlen)
{
	cv->effect = newEffectChain(cv->output, cv->pluginoutput);
	initTrackData(cv, songlen);
}

void debug_dumpTrackState(Song *cs)
{
#ifdef DEBUG_LOGS
	FILE *fp = fopen(".oml_trackdump", "w");

	fprintf(fp, "===== TRACK DUMP =====\n");
	fprintf(fp, "s->track: %p\n", cs->track);
	fprintf(fp, "trackc:   %02x\n\n", cs->track->c);

	for (int i = 0; i < cs->track->c; i++)
	{
		fprintf(fp, "TRACK %02x:\n", i);
		fprintf(fp, "length: %d\n", getSignificantRowc(cs->track->v[i].variant));
		fprintf(fp, "output[0]: %p\n", cs->track->v[i].effect->output[0]);
		fprintf(fp, "\n");
	}

	fprintf(fp, "\n");
	fclose(fp);
#endif
}

static void cb_addTrack(Event *e)
{
	free(e->src); e->src = NULL;
	regenGlobalRowc(s); /* sets p->redraw */
}

/* copyfrom can be NULL */
void addTrack(Song *cs, uint8_t index, uint16_t count, Track *copyfrom)
{ /* fully atomic */
	/* scale down count if necessary */
	count = MIN(count, TRACK_MAX - cs->track->c);

	TrackChain *newtrack = calloc(1, sizeof(TrackChain) + (cs->track->c+count)*sizeof(Track));
	newtrack->c = cs->track->c;

	if (index)
		memcpy(&newtrack->v[0],
				&cs->track->v[0],
				index * sizeof(Track));

	if (index < cs->track->c)
		memcpy(&newtrack->v[index+count],
				&cs->track->v[index],
				(cs->track->c - index) * sizeof(Track));

	/* allocate new tracks */
	for (uint16_t i = 0; i < count; i++)
	{
		addTrackRuntime(&newtrack->v[index+i]);
		addTrackData(&newtrack->v[index+i], cs->songlen);
		copyTrack(&newtrack->v[index+i], copyfrom);
	}

	newtrack->c += count;

	Event e;
	e.sem = M_SEM_SWAP_REQ;
	e.dest = (void **)&cs->track;
	e.src = newtrack;
	e.callback = cb_addTrack;
	pushEvent(&e);
}

void swapTracks(uint8_t index1, uint8_t index2)
{ /* fully atomic */
	TrackChain *newtrack = calloc(1, sizeof(TrackChain) + s->track->c*sizeof(Track));
	newtrack->c = s->track->c;

	/* copy each track over individually, but swap the destinations for index{1,2} */
	for (uint8_t i = 0; i < newtrack->c; i++)
	{
		if      (i == index1) memcpy(&newtrack->v[index2], &s->track->v[i], sizeof(Track));
		else if (i == index2) memcpy(&newtrack->v[index1], &s->track->v[i], sizeof(Track));
		else                  memcpy(&newtrack->v[i],      &s->track->v[i], sizeof(Track));
	}

	Event e;
	e.sem = M_SEM_SWAP_REQ;
	e.dest = (void **)&s->track;
	e.src = newtrack;
	e.callback = cb_addTrack;
	pushEvent(&e);
}

void _delTrack(Song *cs, Track *cv)
{ /* NOT atomic */
	clearTrackData(cv, cs->songlen);
	if (cv->rampbuffer) { free(cv->rampbuffer); cv->rampbuffer = NULL; }
	if (cv->output      [0]) { free(cv->output      [0]); cv->output      [0] = NULL; }
	if (cv->output      [1]) { free(cv->output      [1]); cv->output      [1] = NULL; }
	if (cv->pluginoutput[0]) { free(cv->pluginoutput[0]); cv->pluginoutput[0] = NULL; }
	if (cv->pluginoutput[1]) { free(cv->pluginoutput[1]); cv->pluginoutput[1] = NULL; }
	if (cv->mainmult[0]) { free(cv->mainmult[0]); cv->mainmult[0] = NULL; }
	if (cv->mainmult[1]) { free(cv->mainmult[1]); cv->mainmult[1] = NULL; }
	if (cv->sendmult[0]) { free(cv->sendmult[0]); cv->sendmult[0] = NULL; }
	if (cv->sendmult[1]) { free(cv->sendmult[1]); cv->sendmult[1] = NULL; }
}

static void cb_delTrack(Event *e)
{
	uint16_t count = (size_t)e->callbackarg & 0xffff;
	uint8_t index = ((size_t)e->callbackarg & 0xff0000) >> 16;
	for (uint16_t i = 0; i < count; i++)
		_delTrack(s, &((TrackChain *)e->src)->v[index+i]);
	free(e->src); e->src = NULL;
	if (w->track > s->track->c-1)
		w->track = s->track->c-1;

	regenGlobalRowc(s); /* sets p->redraw */
}
void delTrack(uint8_t index, uint16_t count)
{ /* fully atomic */
	/* scale down count if necessary */
	count = MIN(count, s->track->c - 1);
	if (index + count > s->track->c)
		index = s->track->c - count;

	/* TODO: if the last track would be deleted then call clearTrackData(&s->track->v[0], s->songlen) */

	TrackChain *newtrack = calloc(1, sizeof(TrackChain) + (s->track->c - count) * sizeof(Track));
	newtrack->c = s->track->c - count;

	if (index)
		memcpy(&newtrack->v[0],
				&s->track->v[0],
				index * sizeof(Track));

	if (index < s->track->c)
		memcpy(&newtrack->v[index],
				&s->track->v[index+count],
				(s->track->c - index - count) * sizeof(Track));

	Event e;
	e.sem = M_SEM_SWAP_REQ;
	e.dest = (void **)&s->track;
	e.src = newtrack;
	e.callback = cb_delTrack;
	e.callbackarg = (void *)((((size_t)index)<<16) + (size_t)count);
	pushEvent(&e);
}

void copyTrack(Track *dest, Track *src) /* NOT atomic */
{
	if (!dest || !src) return;

	freeVariantChain(&dest->variant);
	dest->variant = dupVariantChain(src->variant);

	dest->variant->main = dupVariant(src->variant->main, src->variant->songlen);
	dest->variant->trig = calloc(src->variant->songlen, sizeof(Vtrig));

	if (src->variant->trig) memcpy(dest->variant->trig, src->variant->trig, src->variant->songlen * sizeof(Vtrig));
	else                    memset(dest->variant->trig, VARIANT_VOID, src->variant->songlen * sizeof(Vtrig));
	resizeVariantChain(dest->variant, src->variant->songlen);

	copyEffectChain(&dest->effect, src->effect);
}

Row *getTrackRow(Track *cv, uint16_t index)
{
	int i = getVariantChainPrevVtrig(cv->variant, index);
	if (i != -1 && cv->variant->trig[i].index != VARIANT_OFF
			&& (cv->variant->trig[i].flags&C_VTRIG_LOOP
			|| (cv->variant->i[cv->variant->trig[i].index] < cv->variant->c && cv->variant->v[cv->variant->i[cv->variant->trig[i].index]]->rowc >= index - i)))
		return getVariantRow(cv->variant->v[cv->variant->i[cv->variant->trig[i].index]], index - i);
	else
		return getVariantRow(cv->variant->main, index);
}

static void checkBpmCache(uint32_t fptr, uint16_t *spr, int m, Track *cv, Row *r)
{ /* use fptr as the songlen index, and *spr as a pointer to the new bpm cache */
	((short *)spr)[fptr] = m;
}
static void cb_regenBpmCache(Event *e)
{ /* using cb_addTrack for this causes a loop */
	free(e->src); e->src = NULL;

	s->bpmcachelen = (uint16_t)(size_t)e->callbackarg;

	p->redraw = 1;
}
void regenBpmCache(Song *cs)
{ /* fully atomic */
	short *newbpmcache = malloc(sizeof(short) * cs->songlen);
	memset(newbpmcache, -1, sizeof(short) * cs->songlen);

	for (uint16_t i = 0; i < cs->songlen; i++)
		for (uint8_t j = 0; j < cs->track->c; j++)
			ifMacroCallback(i, (uint16_t *)newbpmcache, &cs->track->v[j], getTrackRow(&cs->track->v[j], i), 'B', checkBpmCache);

	Event e;
	e.sem = M_SEM_SWAP_REQ;
	e.dest = (void **)&cs->bpmcache;
	e.src = newbpmcache;
	e.callback = cb_regenBpmCache;
	e.callbackarg = (void *)(size_t)cs->songlen;
	pushEvent(&e);
}

void regenGlobalRowc(Song *cs)
{
	cs->songlen = STATE_ROWS;
	for (uint8_t i = 0; i < cs->track->c; i++)
		cs->songlen = MAX(cs->songlen, getSignificantRowc(cs->track->v[i].variant));

	/* both zeroed out if the loop range is unset              */
	/* only check loop1 cos loop1 is always greater than loop0 */
	if (cs->loop[1])
		cs->songlen = MAX(cs->loop[1]+1, cs->songlen);

	cs->songlen += 4*cs->rowhighlight;

	for (uint8_t i = 0; i < cs->track->c; i++)
		resizeVariantChain(cs->track->v[i].variant, cs->songlen);

	w->trackerfy = MIN(cs->songlen-1, w->trackerfy);

	regenBpmCache(cs);
}

void cycleVariantUp(Variant *v, uint16_t bound)
{
	bound = bound%(v->rowc+1); /* ensure bound is in range */
	Row hold = v->rowv[bound]; /* hold the first row */
	memmove(&v->rowv[bound], &v->rowv[bound + 1], sizeof(Row) * (v->rowc));
	v->rowv[v->rowc] = hold;
	regenGlobalRowc(s);
}
void cycleVariantDown(Variant *v, uint16_t bound)
{
	bound = bound%(v->rowc+1);   /* ensure bound is in range */
	Row hold = v->rowv[v->rowc]; /* hold the last row */
	memmove(&v->rowv[bound + 1], &v->rowv[bound], sizeof(Row) * (v->rowc));
	v->rowv[bound] = hold;
	regenGlobalRowc(s);
}

void applyTrackMutes(void)
{
	Event e;
	e.sem = M_SEM_TRACK_MUTE;
	pushEvent(&e);
}
void toggleTrackMute(uint8_t track)
{
	s->track->v[track].mute = !s->track->v[track].mute;
	applyTrackMutes();
}
void toggleTrackSolo(uint8_t track)
{
	int i;
	bool reset = 0;

	if (!s->track->v[track].mute)
	{
		for (i = 0; i < s->track->c; i++)
			if (s->track->v[i].mute)
				reset = 1;

		for (i = 0; i < s->track->c; i++)
			if (i != track && !s->track->v[i].mute)
				reset = 0;
	}

	if (reset)
	{
		for (i = 0; i < s->track->c; i++)
			s->track->v[i].mute = 0;
	} else
	{
		for (i = 0; i < s->track->c; i++)
			s->track->v[i].mute = 1;
		s->track->v[track].mute = 0;
	}
	applyTrackMutes();
}
