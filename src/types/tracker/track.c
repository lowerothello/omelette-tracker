/* clears the playback state of a track */
void clearTrackRuntime(Track *cv)
{
	cv->r.note = cv->samplernote = NOTE_VOID;
	cv->r.inst = cv->samplerinst = INST_VOID;
	cv->rtrigsamples = 0;
	cv->data.rtrig_rev = 0;

	cv->gain.base = 0x88;
	cv->gain.rand = 0x88;
	cv->gain.target = -1;
	cv->gain.target_rand = 0;

	cv->send.base = 0x00;
	cv->send.rand = 0x00;
	cv->send.target = -1;
	cv->send.target_rand = 0;

	cv->filter.mode[0] = cv->filter.mode[1] = 0;
	cv->filter.targetmode[0] = cv->filter.targetmode[1] = -1;
	cv->filter.cut[0] = cv->filter.cut[1] = 255;
	cv->filter.randcut[0] = cv->filter.randcut[1] = 255;
	cv->filter.targetcut[0] = cv->filter.targetcut[1] = -1;
	cv->filter.targetcut_rand = 0;
	cv->filter.res[0] = cv->filter.res[1] = 0;
	cv->filter.randres[0] = cv->filter.randres[1] = 0;
	cv->filter.targetres[0] = cv->filter.targetres[1] = -1;
	cv->filter.targetres_rand = 0;

	cv->midiccindex = -1; cv->midicc = 0;
}

/* clears the global variant and frees all local variants */
void initTrackData(Song *cs, TrackData *cd) /* TODO: should be atomic */
{

	freeVariantChain(&cd->variant);
	cd->variant = calloc(1, sizeof(VariantChain));

	/* resizing NULL will give a zero'ed out variant of size newlen */
	// cd->variant->main = dupVariant(NULL, cs->songlen);
	// cd->variant->trig = calloc(cs->songlen, sizeof(Vtrig));
	resizeVariantChain(cd->variant, cs->songlen);

	memset(cd->variant->i, VARIANT_VOID, sizeof(uint8_t) * VARIANT_MAX);
	cd->variant->macroc = 1;

	cd->mute = 0;

	if (cd->effect) clearEffectChain(cd->effect);
}

void clearTrackdata(Song *cs, TrackData *cd) /* TODO: should be atomic */
{
	initTrackData(cs, cd);
	freeVariantChain(&cd->variant);
	if (cd->effect) { free(cd->effect); cd->effect = NULL; }
}

void __addTrack(Track *cv) /* __ layer of abstraction for initializing previewtrack */
{
	cv->rampindex = rampmax;
	cv->rampbuffer = malloc(sizeof(float) * rampmax * 2); /* *2 for stereo */
	cv->output[0] =       calloc(buffersize, sizeof(float));
	cv->output[1] =       calloc(buffersize, sizeof(float));
	cv->pluginoutput[0] = calloc(buffersize, sizeof(float));
	cv->pluginoutput[1] = calloc(buffersize, sizeof(float));
	cv->mainmult[0] = calloc(buffersize, sizeof(float));
	cv->mainmult[1] = calloc(buffersize, sizeof(float));
	cv->sendmult[0] = calloc(buffersize, sizeof(float));
	cv->sendmult[1] = calloc(buffersize, sizeof(float));
	clearTrackRuntime(cv);
}

void _addTrack(Song *cs, Track *cv)
{
	__addTrack(cv);

	cv->data.effect = newEffectChain(cv->output, cv->pluginoutput);

	// resizeVariantChain(cv->data.variant, cs->songlen); /* TODO: unnecessary? */
	initTrackData(cs, &cv->data);
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
		fprintf(fp, "length: %d\n", getSignificantRowc(cs->track->v[i].data.variant));
		fprintf(fp, "output[0]: %p\n", cs->track->v[i].data.effect->output[0]);
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
int addTrack(Song *cs, uint8_t index, uint16_t count)
{ /* fully atomic */
	/* scale down count if necessary */
	/* if (         (index + count) - (int)s->track->c < 0)
		count += (index + count) - (int)s->track->c; */
	if (cs->track->c + count > TRACK_MAX) return 1; /* TODO: should add fewer tracks if the requested amount wouldn't fit */

	TrackChain *newtrack = calloc(1, sizeof(TrackChain) + (cs->track->c+count) * sizeof(Track));
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
		_addTrack(cs, &newtrack->v[index+i]);

	newtrack->c += count;

	Event e;
	e.sem = M_SEM_SWAP_REQ;
	e.dest = (void **)&s->track;
	e.src = newtrack;
	e.callback = cb_addTrack;
	pushEvent(&e);
	return 0;
}

void _delTrack(Song *cs, Track *cv)
{
	clearTrackdata(cs, &cv->data);
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
	if (         (int)s->track->c - MAX(1, index) - count < 0)
		count += (int)s->track->c - MAX(1, index) - count;
	/* TODO: if the last track would be deleted then call clearTrackdata(s, &s->track->v[0].data) instead */
	/*       currently MAX(1, index) stops this from ever happening */

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

void copyTrackdata(TrackData *dest, TrackData *src) /* TODO: atomicity */
{
	freeVariantChain(&dest->variant);
	dest->variant = dupVariantChain(src->variant);

	dest->variant->main = dupVariant(src->variant->main, s->songlen);
	dest->variant->trig = calloc(s->songlen, sizeof(Vtrig));

	if (src->variant->trig) memcpy(dest->variant->trig, src->variant->trig, s->songlen * sizeof(Vtrig));
	else                    memset(dest->variant->trig, VARIANT_VOID, s->songlen * sizeof(Vtrig));

	copyEffectChain(&dest->effect, src->effect);
}

Row *getTrackRow(TrackData *cd, uint16_t index)
{
	int i = getVariantChainPrevVtrig(cd->variant, index);
	if (i != -1 && cd->variant->trig[i].index != VARIANT_OFF
			&& (cd->variant->trig[i].flags&C_VTRIG_LOOP
			|| (cd->variant->i[cd->variant->trig[i].index] < cd->variant->c && cd->variant->v[cd->variant->i[cd->variant->trig[i].index]]->rowc >= index - i)))
		return getVariantRow(cd->variant->v[cd->variant->i[cd->variant->trig[i].index]], index - i);
	else
		return getVariantRow(cd->variant->main, index);
}

char checkBpmCache(jack_nframes_t fptr, uint16_t *spr, int m, Track *cv, Row r)
{ /* use fptr as the songlen index, and *spr as a pointer to the new bpm cache */
	((short *)spr)[fptr] = m;
	return 0;
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
			ifMacro(i, (uint16_t *)newbpmcache, &cs->track->v[j], *getTrackRow(&cs->track->v[j].data, i), 'B', 0, &checkBpmCache);

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
		cs->songlen = MAX(cs->songlen, getSignificantRowc(cs->track->v[i].data.variant));

	/* both zeroed out if the loop range is unset              */
	/* only check loop1 cos loop1 is always greater than loop0 */
	if (cs->loop[1])
		cs->songlen = MAX(cs->loop[1]+1, cs->songlen);

	cs->songlen += 4*cs->rowhighlight;

	for (uint8_t i = 0; i < cs->track->c; i++)
		resizeVariantChain(cs->track->v[i].data.variant, cs->songlen);

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

void serializeTrack(Song *cs, Track *cv, FILE *fp)
{
	fputc(cv->data.mute, fp);
	fputc(cv->data.variant->macroc, fp);
	for (int i = 0; i < VARIANT_MAX; i++)
		fputc(cv->data.variant->i[i], fp);
	fputc(cv->data.variant->c, fp);
	for (int i = 0; i < cv->data.variant->c; i++)
		serializeVariant(cv->data.variant->v[i], fp);

	fwrite(cv->data.variant->trig, sizeof(Vtrig), cs->songlen, fp);
	fwrite(cv->data.variant->main->rowv, sizeof(Row), cs->songlen, fp);

	serializeEffectChain(cv->data.effect, fp);
}
void deserializeTrack(Song *cs, Track *cv, FILE *fp, uint8_t major, uint8_t minor)
{
	_addTrack(cs, cv);
	cv->data.mute = fgetc(fp);
	cv->data.variant->macroc = fgetc(fp);
	for (int i = 0; i < VARIANT_MAX; i++)
		cv->data.variant->i[i] = fgetc(fp);
	cv->data.variant->c = fgetc(fp);
	for (int i = 0; i < cv->data.variant->c; i++)
		deserializeVariant(&cv->data.variant->v[i], fp);

	if (major <= 1 && minor < 1)
		for (int i = 0; i < cs->songlen; i++)
		{
			fread(cv->data.variant->trig, sizeof(Vtrig), 1, fp);
			fseek(fp, sizeof(Macro), SEEK_CUR);
		}
	else fread(cv->data.variant->trig, sizeof(Vtrig), cs->songlen, fp);

	fread(cv->data.variant->main->rowv, sizeof(Row), cs->songlen, fp);

	deserializeEffectChain(&cv->data.effect, fp, major, minor);
}

void applyTrackMutes(void)
{
	Event e;
	e.sem = M_SEM_TRACK_MUTE;
	pushEvent(&e);
}
void toggleTrackMute(uint8_t track)
{
	s->track->v[track].data.mute = !s->track->v[track].data.mute;
	applyTrackMutes();
}
void toggleTrackSolo(uint8_t track)
{
	bool flush = 1; /* all tracks except the toggled one should be muted */
	bool reset = 1; /* all tracks should be unmuted                      */

	for (int i = 0; i < s->track->c; i++)
	{
		if ( s->track->v[i].data.mute)                 flush = 0;
		if ( s->track->v[i].data.mute && i == track) reset = 0;
		if (!s->track->v[i].data.mute && i != track) reset = 0;
	}

	if (flush && !reset)
	{
		for (int i = 0; i < s->track->c; i++)
			if (i != track) s->track->v[i].data.mute = 1;
		applyTrackMutes();
	} else if (reset && !flush)
	{
		for (int i = 0; i < s->track->c; i++)
			s->track->v[i].data.mute = 0;
		applyTrackMutes();
	} else toggleTrackMute(track);
}
