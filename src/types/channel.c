void regenGlobalRowc(Song *); /* TODO: proper headers to avoid stragglers */
uint16_t getSignificantRowc(ChannelData *);

/* clears the playback state of a channel */
void clearChannelRuntime(Channel *cv)
{
	cv->r.note = cv->samplernote = NOTE_VOID;
	cv->r.inst = cv->samplerinst = INST_VOID;
	cv->rtrigsamples = 0;
	cv->data.rtrig_rev = 0;
	cv->data.target_rand = 0;
	cv->waveshaperstrength = 0; cv->targetwaveshaperstrength = -1;
	cv->gain = cv->randgain = 0x88; cv->targetgain = -1;
	cv->filtermode[0] = cv->filtermode[1] = 0;
	cv->targetfiltermode[0] = cv->targetfiltermode[1] = -1;
	cv->filtercut[0] = cv->filtercut[1] = 255;
	cv->targetfiltercut[0] = cv->targetfiltercut[1] = -1;
	cv->filterres[0] = cv->filterres[1] = 0;
	cv->targetfilterres[0] = cv->targetfilterres[1] = -1;
	cv->midiccindex = -1; cv->midicc = 0;
	cv->sendgain = cv->sendrandgain = 0x00; cv->targetsendgain = -1;
}

Variant *_copyVariant(Variant *oldvariant, uint16_t newlen)
{
	Variant *ret = malloc(sizeof(uint16_t) + (newlen+1) * sizeof(Row));

	/* properly zero out the new variant */
	memset(ret->rowv, 0, (newlen+1) * sizeof(Row));
	for (uint32_t i = 0; i <= newlen; i++)
	{
		ret->rowv[i].note = NOTE_VOID;
		ret->rowv[i].inst = INST_VOID;
	}

	ret->rowc = newlen;

	if (oldvariant)
		memcpy(ret->rowv, oldvariant->rowv, (MIN(oldvariant->rowc, newlen)+1) * sizeof(Row));

	return ret;
}

/* invalidates getRow() calls */
void resizeChanneldataGlobalVariant(ChannelData *cd, uint16_t newlen)
{
	Vtrig *newtrig = calloc(newlen, sizeof(Vtrig));

	/* can't initialize with memset cos flags will be set to VARIANT_VOID as well */
	for (uint16_t i = 0; i < newlen; i++)
		newtrig[i].index = VARIANT_VOID;

	if (cd->songv) /* depends on songv->rowc */
		if (cd->trig) memcpy(newtrig, cd->trig, MIN(cd->songv->rowc, newlen) * sizeof(Vtrig));

	if (cd->trig) free(cd->trig);
	cd->trig = newtrig;

	Variant *temp = _copyVariant(cd->songv, newlen);
	free(cd->songv); cd->songv = temp;
}

/* clears the global variant and frees all local variants */
void clearChanneldata(Song *cs, ChannelData *cd) /* TODO: should be atomic */
{
	/* resizing NULL will give a zero'ed out variant of size newlen */
	Variant *newsongv = _copyVariant(NULL, cs->songlen);
	free(cd->songv);
	cd->songv = newsongv;
	for (int i = 0; i < cs->songlen; i++)
	{
		cd->trig[i].index = VARIANT_VOID;
		cd->trig[i].flags = 0;
	}

	memset(cd->varianti, VARIANT_VOID, VARIANT_MAX);
	for (short i = 0; i < cd->variantc; i++)
	{ free(cd->variantv[i]); cd->variantv[i] = NULL; }
	cd->variantc = 0;

	cd->mute = 0;
	cd->macroc = 1;
}

/* __ layer of abstraction for initializing the previewchannel, ugly */
void __addChannel(Channel *cv)
{
	cv->rampindex = rampmax;
	cv->rampbuffer = malloc(sizeof(short) * rampmax * 2); /* *2 for stereo */
	cv->output[0] = calloc(buffersize, sizeof(float));
	cv->output[1] = calloc(buffersize, sizeof(float));
	cv->pluginoutput[0] = calloc(buffersize, sizeof(float));
	cv->pluginoutput[1] = calloc(buffersize, sizeof(float));
	cv->mainmult[0] = calloc(buffersize, sizeof(float));
	cv->mainmult[1] = calloc(buffersize, sizeof(float));
	cv->sendmult[0] = calloc(buffersize, sizeof(float));
	cv->sendmult[1] = calloc(buffersize, sizeof(float));
	memcpy(&cv->data.effect.input,  &cv->output,       sizeof(float *) * 2);
	memcpy(&cv->data.effect.output, &cv->pluginoutput, sizeof(float *) * 2);
	clearChannelRuntime(cv);
}
void __delChannel(Channel *cv)
{
	if (cv->rampbuffer) { free(cv->rampbuffer); cv->rampbuffer = NULL; }
	if (cv->output[0]) { free(cv->output[0]); cv->output[0] = NULL; }
	if (cv->output[1]) { free(cv->output[1]); cv->output[1] = NULL; }
	if (cv->pluginoutput[0]) { free(cv->pluginoutput[0]); cv->pluginoutput[0] = NULL; }
	if (cv->pluginoutput[1]) { free(cv->pluginoutput[1]); cv->pluginoutput[1] = NULL; }
	if (cv->mainmult[0]) { free(cv->mainmult[0]); cv->mainmult[0] = NULL; }
	if (cv->mainmult[1]) { free(cv->mainmult[1]); cv->mainmult[1] = NULL; }
	if (cv->sendmult[0]) { free(cv->sendmult[0]); cv->sendmult[0] = NULL; }
	if (cv->sendmult[1]) { free(cv->sendmult[1]); cv->sendmult[1] = NULL; }
}

void _addChannel(Song *cs, Channel *cv)
{
	memset(cv->data.varianti, VARIANT_VOID, VARIANT_MAX);
	resizeChanneldataGlobalVariant(&cv->data, cs->songlen);

	__addChannel(cv);

	clearChanneldata(cs, &cv->data);
}

void debug_dumpChannelState(Song *cs)
{
	FILE *fp = fopen(".oml_channeldump", "w");

	fprintf(fp, "===== CHANNEL DUMP =====\n");
	fprintf(fp, "s->channel: %p\n", cs->channel);
	fprintf(fp, "channelc:   %02x\n\n", cs->channel->c);

	for (int i = 0; i < cs->channel->c; i++)
	{
		fprintf(fp, "CHANNEL %02x:\n", i);
		fprintf(fp, "length: %d\n", getSignificantRowc(&cs->channel->v[i].data));
		fprintf(fp, "\n");
	}

	fclose(fp);
}

void cb_addChannel(Event *e)
{
	free(e->swap2);
	e->swap2 = NULL;
	debug_dumpChannelState(s);
	regenGlobalRowc(s);
	p->dirty = 1;
}
int addChannel(Song *cs, uint8_t index, uint16_t count)
{ /* fully atomic */
	if (cs->channel->c + count > CHANNEL_MAX) return 1; /* TODO: should add fewer channels if the requested amount wouldn't fit */

	ChannelChain *newchannel = calloc(1, sizeof(ChannelChain) + (cs->channel->c+count) * sizeof(Channel));
	newchannel->c = cs->channel->c;

	if (index > 0)
		memcpy(&newchannel->v[0],
				&cs->channel->v[0],
				index * sizeof(Channel));

	if (index < cs->channel->c)
		memcpy(&newchannel->v[index+count],
				&cs->channel->v[index],
				(cs->channel->c-index) * sizeof(Channel));

	/* allocate new channels */
	for (uint16_t i = 0; i < count; i++)
		_addChannel(cs, &newchannel->v[index+i]);

	newchannel->c += count;

	Event e;
	e.sem = M_SEM_SWAP_REQ;
	e.swap1 = s->channel;
	e.swap2 = newchannel;
	e.callback = cb_addChannel;
	pushEvent(&e);
	return 0;
}

void _delChannel(Channel *cv)
{
	clearChanneldata(s, &cv->data);
	free(cv->data.trig); cv->data.trig = NULL;
	free(cv->data.songv); cv->data.songv = NULL;

	for (uint8_t i = 0; i < cv->data.effect.c; i++)
		freeEffect(&cv->data.effect.v[i]);
	cv->data.effect.c = 0;

	__delChannel(cv);
}

void cb_delChannel(Event *e)
{
	uint16_t count = (size_t)e->callbackarg & 0xffff;
	uint8_t index = ((size_t)e->callbackarg & 0xff0000) >> 16;
	for (uint16_t i = 0; i < count; i++)
		_delChannel(&((ChannelChain *)e->swap2)->v[index+i]);

	free(e->swap2);
	e->swap2 = NULL;
	if (w->channel > s->channel->c-1)
		w->channel = s->channel->c-1;
	regenGlobalRowc(s);
	p->dirty = 1;
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

	if (index > 0)
		memcpy(&newchannel->v[0],
				&s->channel->v[0],
				sizeof(Channel)*index);

	if (index < s->channel->c)
		memcpy(&newchannel->v[index],
				&s->channel->v[index+count],
				sizeof(Channel)*(s->channel->c - index - count));

	Event e;
	e.sem = M_SEM_SWAP_REQ;
	e.swap1 = s->channel;
	e.swap2 = newchannel;
	e.callback = cb_delChannel;
	e.callbackarg = (void *)((((size_t)index)<<16) + (size_t)count);
	pushEvent(&e);
}

void copyChanneldata(ChannelData *dest, ChannelData *src)
{
	memcpy(&dest->varianti, &src->varianti, sizeof(uint8_t) * VARIANT_MAX);
	dest->variantc = src->variantc;
	for (short i = 0; i < dest->variantc; i++)
	{
		if (dest->variantv[i]) free(dest->variantv[i]);
		dest->variantv[i] = _copyVariant(src->variantv[i], src->variantv[i]->rowc);
	}

	if (dest->songv) free(dest->songv);
	dest->songv = _copyVariant(src->songv, s->songlen);

	if (dest->trig) free(dest->trig);
	dest->trig = calloc(s->songlen, sizeof(Vtrig));
	if (src->trig) memcpy(dest->trig, src->trig,    s->songlen * sizeof(Vtrig));
	else           memset(dest->trig, VARIANT_VOID, s->songlen * sizeof(Vtrig));

	for (uint8_t i = 0; i < src->effect.c; i++)
		copyEffect(&dest->effect.v[i], &src->effect.v[i]);
	dest->effect.c = src->effect.c;

	dest->macroc = src->macroc;
}

int delVariant(ChannelData *cd, uint8_t index)
{
	if (cd->varianti[index] == VARIANT_VOID) return 1; /* index not occupied */

	uint8_t cutindex = cd->varianti[index];
	free(cd->variantv[cutindex]); cd->variantv[cutindex] = NULL;

	cd->varianti[index] = VARIANT_VOID;

	/* contiguity */
	short i;
	for (i = cutindex; i < cd->variantc-1; i++)
		cd->variantv[i] = cd->variantv[i + 1];

	for (i = 0; i < VARIANT_MAX; i++)
		if (cd->varianti[i] > cutindex && cd->varianti[i] != VARIANT_VOID)
			cd->varianti[i]--;

	cd->variantc--;
	return 0;
}

/* returns true if the variant given is popuplated */
bool variantPopulated(ChannelData *cd, Variant *v)
{
	for (int i = 0; i < v->rowc; i++)
	{
		if (v->rowv[i].note != NOTE_VOID) return 1;
		for (short j = 0; j < cd->macroc+1; j++)
			if (v->rowv[i].macro[j].c) return 1;
	} return 0;
}

/* remove variant if it's empty           */
/* returns true if the variant was pruned */
bool pruneVariant(ChannelData *cd, uint8_t index)
{
	if (index == VARIANT_VOID || index == VARIANT_OFF
			|| cd->varianti[index] == VARIANT_VOID)
		return 0;

	/* fail if variant is still referenced */
	for (int i = 0; i < cd->songv->rowc; i++)
		if (cd->trig[i].index == index) return 0;

	/* fail if variant if populated */
	if (variantPopulated(cd, cd->variantv[cd->varianti[index]])) return 0;

	delVariant(cd, index);
	return 1;
}

int addVariant(ChannelData *cd, uint8_t index, uint8_t length)
{
	if (index == VARIANT_VOID || cd->varianti[index] != VARIANT_VOID) return 1;

	cd->varianti[index] = cd->variantc;
	cd->variantv[cd->variantc] = _copyVariant(NULL, length);
	cd->variantc++;
	return 0;
}
uint8_t _duplicateEmptyVariantIndex(ChannelData *cd, uint8_t fallbackindex)
{
	for (short i = 0; i < VARIANT_MAX; i++)
		if (cd->varianti[i] == VARIANT_VOID)
		{
			if (fallbackindex != VARIANT_VOID && fallbackindex != VARIANT_OFF && variantPopulated(cd, cd->variantv[cd->varianti[fallbackindex]]))
			{ /* duplicate the fallback index if it makes sense to do so */
				cd->varianti[i] = cd->variantc;
				cd->variantv[cd->variantc] =
					_copyVariant(cd->variantv[cd->varianti[fallbackindex]],
								 cd->variantv[cd->varianti[fallbackindex]]->rowc);
				cd->variantc++;
			} return i;
		}
	return fallbackindex;
}
uint8_t _getEmptyVariantIndex(ChannelData *cd, uint8_t fallbackindex)
{
	for (short i = 0; i < VARIANT_MAX; i++)
		if (cd->varianti[i] == VARIANT_VOID)
			return i;
	return fallbackindex;
}

Row *getVariantRow(Variant *v, uint16_t row) { return &v->rowv[row%(v->rowc+1)]; }

/*
 * returns the last (if any) variant trigger
 * returns -1 if no vtrig is within range
 */
int getPrevVtrig(ChannelData *cd, uint16_t index)
{
	/* walk through 0, expect underflow */
	uint16_t iterstop = USHRT_MAX;
	if (index > 256) iterstop = index - 256; /* only walk up to 256 steps backwards (the longest a vtrig can be) */
	for (uint16_t i = index; i != iterstop; i--)
		if (cd->trig[i].index != VARIANT_VOID) return i;

	return -1;
}

Row *getChannelRow(ChannelData *cd, uint16_t index)
{
	int i = getPrevVtrig(cd, index);
	if (i != -1 && cd->trig[i].index != VARIANT_OFF
			&& (cd->trig[i].flags&C_VTRIG_LOOP || cd->variantv[cd->varianti[cd->trig[i].index]]->rowc >= index - i))
		return getVariantRow(cd->variantv[cd->varianti[cd->trig[i].index]], index - i);
	else
		return getVariantRow(cd->songv, index);
}

char checkBpmCache(jack_nframes_t fptr, uint16_t *spr, int m, Channel *cv, Row r)
{ /* use fptr as the songlen index, and *spr as a pointer to the new bpm cache */
	((short *)spr)[fptr] = m;
	return 0;
}
void cb_regenBpmCache(Event *e)
{ /* using cb_addChannel for this causes a loop */
	free(e->swap2);
	e->swap2 = NULL;
	p->dirty = 1;
}
void regenBpmCache(Song *cs)
{ /* fully atomic */
	short *newbpmcache = malloc(sizeof(short) * cs->songlen);
	memset(newbpmcache, -1, sizeof(short) * cs->songlen);

	for (uint16_t i = 0; i < cs->songlen; i++)
		for (uint8_t j = 0; j < cs->channel->c; j++)
			ifMacro(i, (uint16_t *)newbpmcache, &cs->channel->v[j], *getChannelRow(&cs->channel->v[j].data, i), 'B', &checkBpmCache);

	Event e;
	e.sem = M_SEM_SWAP_REQ;
	e.swap1 = s->bpmcache;
	e.swap2 = newbpmcache;
	e.callback = cb_regenBpmCache;
	pushEvent(&e);
}

/* the effective rowc, how many rows are actually used */
/* only implemented for the global variant             */
uint16_t getSignificantRowc(ChannelData *cd)
{
	/* get the length if only the variant triggers */
	/* walk through 0, expect underflow */
	int lvtrig = 0;
	for (uint16_t i = cd->songv->rowc-1; i < USHRT_MAX; i--)
	{
		if (cd->trig[i].index != VARIANT_VOID)
		{
			lvtrig = i;
			if (cd->trig[i].index != VARIANT_OFF)
				lvtrig += cd->variantv[cd->varianti[cd->trig[i].index]]->rowc;
			break;
		}
	}

	for (uint16_t i = cd->songv->rowc-1; i != USHRT_MAX; i--)
	{
		if (cd->songv->rowv[i].note != NOTE_VOID) return MAX(lvtrig, i);
		for (short j = 0; j <= cd->macroc; j++)
			if (cd->songv->rowv[i].macro[j].c) return MAX(lvtrig, i);
	}

	return lvtrig+1;
}

void regenGlobalRowc(Song *cs)
{
	cs->songlen = STATE_ROWS;
	for (uint8_t i = 0; i < cs->channel->c; i++)
		cs->songlen = MAX(cs->songlen, getSignificantRowc(&cs->channel->v[i].data));

	/* both zeroed out if the loop range is unset              */
	/* only check loop1 cos loop1 is always greater than loop0 */
	if (cs->loop[1])
		cs->songlen = MAX(cs->loop[1]+1, cs->songlen);

	cs->songlen += 4*cs->rowhighlight;

	for (uint8_t i = 0; i < cs->channel->c; i++)
		resizeChanneldataGlobalVariant(&cs->channel->v[i].data, cs->songlen);

	w->trackerfy = MIN(cs->songlen-1, w->trackerfy);

	regenBpmCache(cs);
}

void inputChannelTrig(ChannelData *cd, uint16_t index, char value)
{
	uint8_t oldvariant = cd->trig[index].index;
	if (cd->trig[index].index == VARIANT_VOID)
		cd->trig[index].index = 0;
	cd->trig[index].index <<= 4; cd->trig[index].index += value;
	if (cd->trig[index].index == VARIANT_VOID)
		cd->trig[index].flags = 0;
	pruneVariant(cd, oldvariant);
	addVariant(cd, cd->trig[index].index, w->defvariantlength);
	regenGlobalRowc(s);
}

/* prunes the old index and sets the new index */
void setChannelTrig(ChannelData *cd, uint16_t index, uint8_t value)
{
	uint8_t oldvariant = cd->trig[index].index;
	cd->trig[index].index = value;
	if (cd->trig[index].index == VARIANT_VOID)
		cd->trig[index].flags = 0;
	pruneVariant(cd, oldvariant);
	addVariant(cd, cd->trig[index].index, w->defvariantlength);
	regenGlobalRowc(s);
}

void cycleVariantUp(Variant *v, uint16_t bound)
{
	bound = bound%(v->rowc+1); /* ensure bound is in range */
	Row hold = v->rowv[bound]; /* hold the first row */
	for (uint16_t i = bound; i < v->rowc; i++)
		v->rowv[i] = v->rowv[i+1];
	v->rowv[v->rowc] = hold;
	regenGlobalRowc(s);
}
void cycleVariantDown(Variant *v, uint16_t bound)
{
	bound = bound%(v->rowc+1); /* ensure bound is in range */
	Row hold = v->rowv[v->rowc]; /* hold the last row */
	for (int i = v->rowc - 1; i >= bound; i--)
		v->rowv[i+1] = v->rowv[i];
	v->rowv[bound] = hold;
	regenGlobalRowc(s);
}

/* returns the index within the variant, writes the variant pointer to **output */
/* **output == NULL is allowed */
/* returns -1 if not in a variant */
int getChannelVariant(Variant **output, ChannelData *cd, uint16_t index)
{
	int i = getPrevVtrig(cd, index);
	if (i != -1 && cd->trig[i].index != VARIANT_OFF
			&& (cd->trig[i].flags&C_VTRIG_LOOP || cd->variantv[cd->varianti[cd->trig[i].index]]->rowc >= index - i))
	{
		if (output) *output = cd->variantv[cd->varianti[cd->trig[i].index]];
		return index - i;
	}

	return -1;
}
/* like getChannelVariant(), but ignore looping variants */
int getChannelVariantNoLoop(Variant **output, ChannelData *cd, uint16_t index)
{
	int i = getPrevVtrig(cd, index);
	if (i != -1 && cd->trig[i].index != VARIANT_OFF
			&& cd->variantv[cd->varianti[cd->trig[i].index]]->rowc >= index - i)
	{
		if (output) *output = cd->variantv[cd->varianti[cd->trig[i].index]];
		return index - i;
	}

	return -1;
}

void serializeVariant(Variant *v, FILE *fp)
{
	fwrite(&v->rowc, sizeof(uint16_t), 1, fp);
	fwrite(v->rowv, sizeof(Row), v->rowc, fp);
}
void deserializeVariant(Variant **v, FILE *fp)
{
	uint16_t rowc;
	fread(&rowc, sizeof(uint16_t), 1, fp);
	*v = _copyVariant(NULL, rowc);
	fread((*v)->rowv, sizeof(Row), rowc, fp);
}

void serializeChannel(Song *cs, Channel *cv, FILE *fp)
{
	fputc(cv->data.mute, fp);
	fputc(cv->data.macroc, fp);
	for (int i = 0; i < VARIANT_MAX; i++)
		fputc(cv->data.varianti[i], fp);
	fputc(cv->data.variantc, fp);
	for (int i = 0; i < cv->data.variantc; i++)
		serializeVariant(cv->data.variantv[i], fp);

	fwrite(cv->data.trig, sizeof(Vtrig), cs->songlen, fp);
	fwrite(cv->data.songv->rowv, sizeof(Row), cs->songlen, fp);

	fwrite(&cv->data.effect.c, sizeof(uint8_t), 1, fp);
	for (int i = 0; i < cv->data.effect.c; i++)
		serializeEffect(&cv->data.effect.v[i], fp);
}
void deserializeChannel(Song *cs, Channel *cv, FILE *fp, uint8_t major, uint8_t minor)
{
	_addChannel(cs, cv);
	cv->data.mute = fgetc(fp);
	cv->data.macroc = fgetc(fp);
	for (int i = 0; i < VARIANT_MAX; i++)
		cv->data.varianti[i] = fgetc(fp);
	cv->data.variantc = fgetc(fp);
	for (int i = 0; i < cv->data.variantc; i++)
		deserializeVariant(&cv->data.variantv[i], fp);

	fread(cv->data.trig, sizeof(Vtrig), cs->songlen, fp);
	fread(cv->data.songv->rowv, sizeof(Row), cs->songlen, fp);

	if (!(major == 1 && minor < 97))
	{
		fread(&cv->data.effect.c, sizeof(uint8_t), 1, fp);
		for (int i = 0; i < cv->data.effect.c; i++)
			deserializeEffect(&cv->data.effect, &cv->data.effect.v[i], fp, major, minor);
	}
}
