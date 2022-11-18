/* clears the playback state of a channel */
void clearChannelRuntime(Channel *cv)
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
void initChannelData(Song *cs, ChannelData *cd) /* TODO: should be atomic */
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

void clearChanneldata(Song *cs, ChannelData *cd) /* TODO: should be atomic */
{
	initChannelData(cs, cd);
	freeVariantChain(&cd->variant);
	if (cd->effect) { free(cd->effect); cd->effect = NULL; }
}

void __addChannel(Channel *cv) /* __ layer of abstraction for initializing previewchannel */
{
	cv->rampindex = rampmax;
	cv->rampbuffer = malloc(sizeof(short) * rampmax * 2); /* *2 for stereo */
	cv->output[0] =       calloc(buffersize, sizeof(float));
	cv->output[1] =       calloc(buffersize, sizeof(float));
	cv->pluginoutput[0] = calloc(buffersize, sizeof(float));
	cv->pluginoutput[1] = calloc(buffersize, sizeof(float));
	cv->mainmult[0] = calloc(buffersize, sizeof(float));
	cv->mainmult[1] = calloc(buffersize, sizeof(float));
	cv->sendmult[0] = calloc(buffersize, sizeof(float));
	cv->sendmult[1] = calloc(buffersize, sizeof(float));
	clearChannelRuntime(cv);
}

void _addChannel(Song *cs, Channel *cv)
{
	__addChannel(cv);

	cv->data.effect = newEffectChain(cv->output, cv->pluginoutput);

	// resizeVariantChain(cv->data.variant, cs->songlen); /* TODO: unnecessary? */
	initChannelData(cs, &cv->data);
}

void debug_dumpChannelState(Song *cs)
{
#ifdef DEBUG_LOGS
	FILE *fp = fopen(".oml_channeldump", "w");

	fprintf(fp, "===== CHANNEL DUMP =====\n");
	fprintf(fp, "s->channel: %p\n", cs->channel);
	fprintf(fp, "channelc:   %02x\n\n", cs->channel->c);

	for (int i = 0; i < cs->channel->c; i++)
	{
		fprintf(fp, "CHANNEL %02x:\n", i);
		fprintf(fp, "length: %d\n", getSignificantRowc(cs->channel->v[i].data.variant));
		fprintf(fp, "output[0]: %p\n", cs->channel->v[i].data.effect->output[0]);
		fprintf(fp, "\n");
	}

	fprintf(fp, "\n");
	fclose(fp);
#endif
}

static void cb_addChannel(Event *e)
{
	free(e->src); e->src = NULL;
	regenGlobalRowc(s); /* sets p->redraw */
}
int addChannel(Song *cs, uint8_t index, uint16_t count)
{ /* fully atomic */
	/* scale down count if necessary */
	/* if (         (index + count) - (int)s->channel->c < 0)
		count += (index + count) - (int)s->channel->c; */
	if (cs->channel->c + count > CHANNEL_MAX) return 1; /* TODO: should add fewer channels if the requested amount wouldn't fit */

	ChannelChain *newchannel = calloc(1, sizeof(ChannelChain) + (cs->channel->c+count) * sizeof(Channel));
	newchannel->c = cs->channel->c;

	if (index)
		memcpy(&newchannel->v[0],
				&cs->channel->v[0],
				index * sizeof(Channel));

	if (index < cs->channel->c)
		memcpy(&newchannel->v[index+count],
				&cs->channel->v[index],
				(cs->channel->c - index) * sizeof(Channel));

	/* allocate new channels */
	for (uint16_t i = 0; i < count; i++)
		_addChannel(cs, &newchannel->v[index+i]);

	newchannel->c += count;

	Event e;
	e.sem = M_SEM_SWAP_REQ;
	e.dest = (void **)&s->channel;
	e.src = newchannel;
	e.callback = cb_addChannel;
	pushEvent(&e);
	return 0;
}

void _delChannel(Song *cs, Channel *cv)
{
	clearChanneldata(cs, &cv->data);
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

static void cb_delChannel(Event *e)
{
	uint16_t count = (size_t)e->callbackarg & 0xffff;
	uint8_t index = ((size_t)e->callbackarg & 0xff0000) >> 16;
	for (uint16_t i = 0; i < count; i++)
		_delChannel(s, &((ChannelChain *)e->src)->v[index+i]);
	free(e->src); e->src = NULL;
	if (w->channel > s->channel->c-1)
		w->channel = s->channel->c-1;

	regenGlobalRowc(s); /* sets p->redraw */
}
void delChannel(uint8_t index, uint16_t count)
{ /* fully atomic */
	/* scale down count if necessary */
	if (         (int)s->channel->c - MAX(1, index) - count < 0)
		count += (int)s->channel->c - MAX(1, index) - count;
	/* TODO: if the last channel would be deleted then call clearChanneldata(s, &s->channel->v[0].data) instead */
	/*       currently MAX(1, index) stops this from ever happening */

	ChannelChain *newchannel = calloc(1, sizeof(ChannelChain) + (s->channel->c - count) * sizeof(Channel));
	newchannel->c = s->channel->c - count;

	if (index)
		memcpy(&newchannel->v[0],
				&s->channel->v[0],
				index * sizeof(Channel));

	if (index < s->channel->c)
		memcpy(&newchannel->v[index],
				&s->channel->v[index+count],
				(s->channel->c - index - count) * sizeof(Channel));

	Event e;
	e.sem = M_SEM_SWAP_REQ;
	e.dest = (void **)&s->channel;
	e.src = newchannel;
	e.callback = cb_delChannel;
	e.callbackarg = (void *)((((size_t)index)<<16) + (size_t)count);
	pushEvent(&e);
}

void copyChanneldata(ChannelData *dest, ChannelData *src) /* TODO: atomicity */
{
	freeVariantChain(&dest->variant);
	dest->variant = dupVariantChain(src->variant);

	dest->variant->main = dupVariant(src->variant->main, s->songlen);
	dest->variant->trig = calloc(s->songlen, sizeof(Vtrig));

	if (src->variant->trig) memcpy(dest->variant->trig, src->variant->trig, s->songlen * sizeof(Vtrig));
	else                    memset(dest->variant->trig, VARIANT_VOID, s->songlen * sizeof(Vtrig));

	copyEffectChain(&dest->effect, src->effect);
}

Row *getChannelRow(ChannelData *cd, uint16_t index)
{
	int i = getVariantChainPrevVtrig(cd->variant, index);
	if (i != -1 && cd->variant->trig[i].index != VARIANT_OFF
			&& (cd->variant->trig[i].flags&C_VTRIG_LOOP
			|| (cd->variant->i[cd->variant->trig[i].index] < cd->variant->c && cd->variant->v[cd->variant->i[cd->variant->trig[i].index]]->rowc >= index - i)))
		return getVariantRow(cd->variant->v[cd->variant->i[cd->variant->trig[i].index]], index - i);
	else
		return getVariantRow(cd->variant->main, index);
}

char checkBpmCache(jack_nframes_t fptr, uint16_t *spr, int m, Channel *cv, Row r)
{ /* use fptr as the songlen index, and *spr as a pointer to the new bpm cache */
	((short *)spr)[fptr] = m;
	return 0;
}
static void cb_regenBpmCache(Event *e)
{ /* using cb_addChannel for this causes a loop */
	free(e->src); e->src = NULL;

	s->bpmcachelen = (uint16_t)(size_t)e->callbackarg;

	p->redraw = 1;
}
void regenBpmCache(Song *cs)
{ /* fully atomic */
	short *newbpmcache = malloc(sizeof(short) * cs->songlen);
	memset(newbpmcache, -1, sizeof(short) * cs->songlen);

	for (uint16_t i = 0; i < cs->songlen; i++)
		for (uint8_t j = 0; j < cs->channel->c; j++)
			ifMacro(i, (uint16_t *)newbpmcache, &cs->channel->v[j], *getChannelRow(&cs->channel->v[j].data, i), 'B', 0, &checkBpmCache);

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
	for (uint8_t i = 0; i < cs->channel->c; i++)
		cs->songlen = MAX(cs->songlen, getSignificantRowc(cs->channel->v[i].data.variant));

	/* both zeroed out if the loop range is unset              */
	/* only check loop1 cos loop1 is always greater than loop0 */
	if (cs->loop[1])
		cs->songlen = MAX(cs->loop[1]+1, cs->songlen);

	cs->songlen += 4*cs->rowhighlight;

	for (uint8_t i = 0; i < cs->channel->c; i++)
		resizeVariantChain(cs->channel->v[i].data.variant, cs->songlen);

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

void serializeChannel(Song *cs, Channel *cv, FILE *fp)
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
void deserializeChannel(Song *cs, Channel *cv, FILE *fp, uint8_t major, uint8_t minor)
{
	_addChannel(cs, cv);
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
