/* clears the playback state of a channel */
void clearChannelRuntime(channel *cv)
{
	cv->r.note = cv->samplernote = NOTE_VOID;
	cv->r.inst = cv->samplerinst = INST_VOID;
	cv->rtrigsamples = 0;
	cv->data.rtrig_rev = 0;
	cv->data.target_rand = 0;
	cv->waveshaperstrength = 0; cv->targetwaveshaperstrength = -1;
	cv->gain = cv->randgain = 0x88; cv->targetgain = -1;
	cv->filtermode = 0; cv->targetfiltermode = -1;
	cv->filtercut = 255; cv->targetfiltercut = -1;
	cv->filterres = 0; cv->targetfilterres = -1;
	cv->midiccindex = -1; cv->midicc = 0; cv->targetmidicc = -1;
	cv->sendgroup = 0; cv->sendgain = 0; cv->targetsendgain = -1;
}

variant *_copyVariant(variant *oldvariant, uint16_t newlen)
{
	variant *ret = malloc(sizeof(uint16_t) + (newlen+1) * sizeof(row));

	/* properly zero out the new variant */
	memset(ret->rowv, 0, (newlen+1) * sizeof(row));
	for (uint32_t i = 0; i <= newlen; i++)
	{
		ret->rowv[i].note = NOTE_VOID;
		ret->rowv[i].inst = INST_VOID;
	}

	ret->rowc = newlen;

	if (oldvariant)
		memcpy(ret->rowv, oldvariant->rowv, (MIN(oldvariant->rowc, newlen)+1) * sizeof(row));

	return ret;
}

void resizeChanneldataGlobalVariant(channeldata *cd, uint16_t newlen)
{
	vtrig *newtrig = calloc(newlen, sizeof(vtrig));

	memset(newtrig, VARIANT_VOID, newlen * sizeof(vtrig));
	if (cd->songv) /* depends on songv->rowc */
		if (cd->trig) memcpy(newtrig, cd->trig, MIN(cd->songv->rowc, newlen) * sizeof(vtrig));

	if (cd->trig) free(cd->trig);
	cd->trig = newtrig;

	variant *temp = _copyVariant(cd->songv, newlen);
	free(cd->songv); cd->songv = temp;
}

/* clears the global variant and frees all local variants */
void clearChanneldata(song *cs, channeldata *cd)
{
	/* resizing NULL will give a zero'ed out variant of size newlen */
	variant *newsongv = _copyVariant(NULL, cs->songlen);
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
void _addChannel(song *cs, channel *cv)
{
	memset(cv->data.varianti, VARIANT_VOID, VARIANT_MAX);
	resizeChanneldataGlobalVariant(&cv->data, cs->songlen);

	cv->rampindex = rampmax;
	cv->rampbuffer = malloc(sizeof(short) * rampmax * 2); /* *2 for stereo */
	cv->stretchrampindex = stretchrampmax;

	clearChanneldata(cs, &cv->data);
	clearChannelRuntime(cv);
}

int addChannel(song *cs, uint8_t index)
{
	if (cs->channelc >= CHANNEL_MAX) return 1;

	channel *newchannelv = calloc(cs->channelc+1, sizeof(channel));

	if (index > 0)
		memcpy(&newchannelv[0],
				&cs->channelv[0],
				index * sizeof(channel));

	if (index < cs->channelc)
		memcpy(&newchannelv[index+1],
				&cs->channelv[index],
				(cs->channelc-index) * sizeof(channel));

	/* init new channel */
	_addChannel(cs, &newchannelv[index]); /* allocate memory */

	if (cs->channelv) free(cs->channelv);
	cs->channelv = newchannelv;

	cs->channelc++;
	return 0;
}

void _delChannel(channel *cv)
{
	clearChanneldata(s, &cv->data);
	free(cv->data.trig); cv->data.trig = NULL;
	free(cv->data.songv); cv->data.songv = NULL;
	if (cv->rampbuffer) { free(cv->rampbuffer); cv->rampbuffer = NULL; }
}

void delChannel(uint8_t index)
{
	/* if there's only one channel then clear it */
	if (s->channelc == 1)
		clearChanneldata(s, &s->channelv[0].data);
	else
	{
		_delChannel(&s->channelv[index]);

		channel *newchannelv = calloc(s->channelc-1, sizeof(channel));

		if (index > 0)
			memcpy(&newchannelv[0],
					&s->channelv[0],
					sizeof(channel)*index);

		if (index < s->channelc)
			memcpy(&newchannelv[index],
					&s->channelv[index+1],
					sizeof(channel)*(s->channelc-index-1));

		if (s->channelv) free(s->channelv);
		s->channelv = newchannelv;

		s->channelc--;
	}
}

void copyChanneldata(channeldata *dest, channeldata *src)
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
	dest->trig = calloc(s->songlen, sizeof(vtrig));
	if (src->trig) memcpy(dest->trig, src->trig,    s->songlen * sizeof(vtrig));
	else           memset(dest->trig, VARIANT_VOID, s->songlen * sizeof(vtrig));

	dest->macroc = src->macroc;
}

int addVariant(channeldata *cd, uint8_t index)
{
	if (index == VARIANT_VOID || cd->varianti[index] != VARIANT_VOID) return 1;

	cd->varianti[index] = cd->variantc;
	cd->variantv[cd->variantc] = _copyVariant(NULL, w->defvariantlength);
	cd->variantc++;
	return 0;
}
uint8_t _getEmptyVariantIndex(channeldata *cd, uint8_t fallbackindex)
{
	for (short i = 0; i < VARIANT_MAX; i++)
		if (cd->varianti[i] == VARIANT_VOID)
			return i;
	return fallbackindex;
}
uint8_t duplicateVariant(channeldata *cd, uint8_t oldindex)
{
	uint8_t index;
	if ((index = _getEmptyVariantIndex(cd, oldindex)) != oldindex)
	{
		cd->varianti[index] = cd->variantc;
		cd->variantv[cd->varianti[index]] =
			_copyVariant(cd->variantv[cd->varianti[oldindex]],
			             cd->variantv[cd->varianti[oldindex]]->rowc);
		cd->variantc++;
		return index;
	} return oldindex;
}
int delVariant(channeldata *cd, uint8_t index)
{
	if (cd->varianti[index] == VARIANT_VOID) return 1; /* index not occupied */
	uint8_t cutindex = cd->varianti[index];

	free(cd->variantv[index]); cd->variantv[index] = NULL;

	cd->varianti[index] = VARIANT_VOID;

	/* contiguity */
	short i;
	for (i = cutindex; i < cd->variantc-1; i++)
		cd->variantv[i] = cd->variantv[i + 1];

	for (i = 0; i < VARIANT_MAX; i++) /* for every backref index */
		if (cd->varianti[i] > cutindex && cd->varianti[i] != VARIANT_VOID)
			cd->varianti[i]--;

	cd->variantc--;
	return 0;
}
/* remove variant if it's empty */
void pruneVariant(channeldata *cd, uint8_t index)
{
// DEBUG=0;
	if (index == VARIANT_VOID || cd->varianti[index] == VARIANT_VOID) return;

// DEBUG=1;
	/* fail if variant is still referenced */
	for (int i = 0; i < cd->songv->rowc; i++)
		if (cd->trig[i].index == index) return;

// DEBUG=2;
	/* fail if variant if populated */
	variant *vv = cd->variantv[cd->varianti[index]];
	for (int i = 0; i < vv->rowc; i++)
	{
		row r = vv->rowv[i];
		if (r.note != NOTE_VOID) return;
		for (short j = 0; j < cd->macroc+1; j++)
			if (r.macro[j].c) return;
	}

	delVariant(cd, index);
// DEBUG=4;
}

void inputChannelTrig(channeldata *cd, uint16_t index, char value)
{
	uint8_t oldvariant = cd->trig[index].index;
	if (cd->trig[index].index == VARIANT_VOID)
		cd->trig[index].index = 0;
	cd->trig[index].index <<= 4; cd->trig[index].index += value;
	if (cd->trig[index].index == VARIANT_VOID)
		cd->trig[index].flags = 0;
	pruneVariant(cd, oldvariant);
	addVariant(cd, cd->trig[index].index);
}
void setChannelTrig(channeldata *cd, uint16_t index, uint8_t value)
{
	uint8_t oldvariant = cd->trig[index].index;
	cd->trig[index].index = value;
	if (cd->trig[index].index == VARIANT_VOID)
		cd->trig[index].flags = 0;
	pruneVariant(cd, oldvariant);
	addVariant(cd, cd->trig[index].index);
}

void cycleVariantUp(variant *v, uint16_t bound)
{
	bound = bound%(v->rowc+1); /* ensure bound is in range */
	row hold = v->rowv[bound]; /* hold the first row */
	for (uint16_t i = bound; i < v->rowc; i++)
		v->rowv[i] = v->rowv[i+1];
	v->rowv[v->rowc] = hold;
}
void cycleVariantDown(variant *v, uint16_t bound)
{
	bound = bound%(v->rowc+1); /* ensure bound is in range */
	row hold = v->rowv[v->rowc]; /* hold the last row */
	for (int i = v->rowc - 1; i >= bound; i--)
		v->rowv[i+1] = v->rowv[i];
	v->rowv[bound] = hold;
}

row *getVariantRow(variant *v, uint16_t row)
{ return &v->rowv[row%(v->rowc+1)]; }

/*
 * returns the last (if any) variant trigger
 * returns -1 if no vtrig is within range
 */
int getPrevVtrig(channeldata *cd, uint16_t index)
{
	/* walk through 0, expect underflow */
	uint16_t iterstop = USHRT_MAX;
	if (index > 256) iterstop = index - 256; /* only walk up to 256 steps backwards (the longes a vtrig can be) */
	for (uint16_t i = index; i != iterstop; i--)
		if (cd->trig[i].index != VARIANT_VOID) return i;

	return -1;
}

row *getChannelRow(channeldata *cd, uint16_t index)
{
	int i = getPrevVtrig(cd, index);
	if (i != -1 && cd->trig[i].index != VARIANT_OFF
			&& (cd->trig[i].flags&C_VTRIG_LOOP || cd->variantv[cd->varianti[cd->trig[i].index]]->rowc >= index - i))
		return getVariantRow(cd->variantv[cd->varianti[cd->trig[i].index]], index - i);

	return getVariantRow(cd->songv, index);
}
/* returns the index within the variant, writes the variant pointer to **output */
/* **output == NULL is allowed */
/* returns -1 if not in a variant */
int getChannelVariant(variant **output, channeldata *cd, uint16_t index)
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

/* the effective rowc, how many rows are actually used */
uint16_t getSignificantRowc(channeldata *cd)
{
	/* get the length if only the variant triggers */
	/* walk through 0, expect overflow */
	int lvtrig = 0;
	for (uint16_t i = cd->songv->rowc-1; i < USHRT_MAX; i--)
	{
		if (cd->trig[i].index != VARIANT_VOID)
		{
			if (cd->trig[i].index == VARIANT_OFF) lvtrig = i;
			else lvtrig = i + cd->variantv[cd->varianti[cd->trig[i].index]]->rowc;
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

void regenGlobalRowc(song *cs)
{
	uint16_t oldsonglen = cs->songlen;
	cs->songlen = STATE_ROWS;
	for (uint8_t i = 0; i < cs->channelc; i++)
		cs->songlen = MAX(cs->songlen, getSignificantRowc(&cs->channelv[i].data));

	cs->songlen += 4*s->rowhighlight;

	for (uint8_t i = 0; i < cs->channelc; i++)
		resizeChanneldataGlobalVariant(&cs->channelv[i].data, cs->songlen);

	w->trackerfy = MIN(cs->songlen-1, w->trackerfy);

	if (cs->loop[0] > cs->songlen-1)
		cs->loop[0] = cs->songlen-1;

	if (cs->loop[1] > cs->songlen-1 || cs->loop[1] == oldsonglen-1)
		cs->loop[1] = cs->songlen-1;
}
