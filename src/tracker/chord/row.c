void chordRowScaleToCursor(void *_)
{
	ChannelData *cd = &s->channel->v[w->channel].data;
	int gcvret = getPrevVtrig(cd, w->trackerfy);
	if (gcvret == -1 || cd->trig[gcvret].index == VARIANT_OFF) return;
	uint8_t vi = cd->varianti[cd->trig[gcvret].index];

	Variant *v = _copyVariant(cd->variantv[vi], w->trackerfy - gcvret);

	free(cd->variantv[vi]); cd->variantv[vi] = v;
	regenGlobalRowc(s);
}
void chordRowLengthToCount(void *_)
{
	ChannelData *cd = &s->channel->v[w->channel].data;
	int gcvret = getChannelVariant(NULL, cd, w->trackerfy);
	if (gcvret == -1) return;
	uint8_t vi = cd->varianti[cd->trig[w->trackerfy - gcvret].index];

	Variant *v = _copyVariant(cd->variantv[vi], w->count ? w->count-1 : w->defvariantlength);

	free(cd->variantv[vi]); cd->variantv[vi] = v;
	if (!(cd->trig[w->trackerfy - gcvret].flags&C_VTRIG_LOOP)
			&& w->trackerfy > w->trackerfy - gcvret + cd->variantv[vi]->rowc)
		w->trackerfy = w->trackerfy - gcvret + cd->variantv[vi]->rowc;
	regenGlobalRowc(s);
}
void chordRowIncrementLength(void *_)
{
	ChannelData *cd = &s->channel->v[w->channel].data;
	int gcvret = getChannelVariant(NULL, cd, w->trackerfy);
	if (gcvret == -1) return;
	uint8_t vi = cd->varianti[cd->trig[w->trackerfy - gcvret].index];

	Variant *v = _copyVariant(cd->variantv[vi], MIN(VARIANT_ROWMAX, cd->variantv[vi]->rowc + MAX(1, w->count)));

	free(cd->variantv[vi]); cd->variantv[vi] = v;
	if (!(cd->trig[w->trackerfy - gcvret].flags&C_VTRIG_LOOP)
			&& w->trackerfy > w->trackerfy - gcvret + cd->variantv[vi]->rowc)
		w->trackerfy = w->trackerfy - gcvret + cd->variantv[vi]->rowc;
	regenGlobalRowc(s);
}
void chordRowDecrementLength(void *_)
{
	ChannelData *cd = &s->channel->v[w->channel].data;
	int gcvret = getChannelVariant(NULL, cd, w->trackerfy);
	if (gcvret == -1) return;
	uint8_t vi = cd->varianti[cd->trig[w->trackerfy - gcvret].index];

	Variant *v = _copyVariant(cd->variantv[vi], MAX(0, cd->variantv[vi]->rowc - MAX(1, w->count)));

	free(cd->variantv[vi]); cd->variantv[vi] = v;
	if (!(cd->trig[w->trackerfy - gcvret].flags&C_VTRIG_LOOP)
			&& w->trackerfy > w->trackerfy - gcvret + cd->variantv[vi]->rowc)
		w->trackerfy = w->trackerfy - gcvret + cd->variantv[vi]->rowc;
	regenGlobalRowc(s);
}
void chordRowCopyDown(void *_)
{
	ChannelData *cd = &s->channel->v[w->channel].data;
	int gcvret = getChannelVariant(NULL, cd, w->trackerfy);
	if (gcvret == -1) return;
	uint8_t vi = cd->varianti[cd->trig[w->trackerfy - gcvret].index];

	Variant *v = _copyVariant(NULL, MIN(VARIANT_ROWMAX, (cd->variantv[vi]->rowc+1)*(2*MAX(1, w->count)) - 1));
	for (int i = 0; i <= v->rowc; i++)
		v->rowv[i] = cd->variantv[vi]->rowv[i%(cd->variantv[vi]->rowc+1)];

	free(cd->variantv[vi]); cd->variantv[vi] = v;
	if (!(cd->trig[w->trackerfy - gcvret].flags&C_VTRIG_LOOP)
			&& w->trackerfy > w->trackerfy - gcvret + cd->variantv[vi]->rowc)
		w->trackerfy = w->trackerfy - gcvret + cd->variantv[vi]->rowc;
	regenGlobalRowc(s);
}
void chordRowDiscardHalf(void *_)
{
	ChannelData *cd = &s->channel->v[w->channel].data;
	int gcvret = getChannelVariant(NULL, cd, w->trackerfy);
	if (gcvret == -1) return;
	uint8_t vi = cd->varianti[cd->trig[w->trackerfy - gcvret].index];

	Variant *v = _copyVariant(cd->variantv[vi], MAX(0, (cd->variantv[vi]->rowc+1)/(2*MAX(1, w->count)) - 1));

	free(cd->variantv[vi]); cd->variantv[vi] = v;
	if (!(cd->trig[w->trackerfy - gcvret].flags&C_VTRIG_LOOP)
			&& w->trackerfy > w->trackerfy - gcvret + cd->variantv[vi]->rowc)
		w->trackerfy = w->trackerfy - gcvret + cd->variantv[vi]->rowc;
	regenGlobalRowc(s);
}
void chordRowAddBlanks(void *_)
{
	ChannelData *cd = &s->channel->v[w->channel].data;
	int gcvret = getChannelVariant(NULL, cd, w->trackerfy);
	if (gcvret == -1) return;
	uint8_t vi = cd->varianti[cd->trig[w->trackerfy - gcvret].index];

	Variant *v = _copyVariant(NULL, MIN(VARIANT_ROWMAX, (cd->variantv[vi]->rowc+1)*(2*MAX(1, w->count)) - 1));
	for (int i = 0; i <= cd->variantv[vi]->rowc; i++)
		v->rowv[i * MAX(1, w->count)*2] = cd->variantv[vi]->rowv[i];

	free(cd->variantv[vi]); cd->variantv[vi] = v;
	if (!(cd->trig[w->trackerfy - gcvret].flags&C_VTRIG_LOOP)
			&& w->trackerfy > w->trackerfy - gcvret + cd->variantv[vi]->rowc)
		w->trackerfy = w->trackerfy - gcvret + cd->variantv[vi]->rowc;
	regenGlobalRowc(s);
}
void chordRowDiscardEveryOther(void *_)
{
	ChannelData *cd = &s->channel->v[w->channel].data;
	int gcvret = getChannelVariant(NULL, cd, w->trackerfy);
	if (gcvret == -1) return;
	uint8_t vi = cd->varianti[cd->trig[w->trackerfy - gcvret].index];

	Variant *v = _copyVariant(NULL, MAX(0, (cd->variantv[vi]->rowc+1)/(2*MAX(1, w->count)) - 1));
	for (int i = 0; i <= v->rowc; i++)
		v->rowv[i] = cd->variantv[vi]->rowv[i * MAX(1, w->count)*2];

	free(cd->variantv[vi]); cd->variantv[vi] = v;
	if (!(cd->trig[w->trackerfy - gcvret].flags&C_VTRIG_LOOP)
			&& w->trackerfy > w->trackerfy - gcvret + cd->variantv[vi]->rowc)
		w->trackerfy = w->trackerfy - gcvret + cd->variantv[vi]->rowc;
	regenGlobalRowc(s);
}
void chordRowBurn(void *_)
{
	ChannelData *cd = &s->channel->v[w->channel].data;
	int gcvret = getChannelVariant(NULL, cd, w->trackerfy);
	if (gcvret == -1) return;
	uint8_t vvi = cd->trig[w->trackerfy - gcvret].index;
	uint8_t vi = cd->varianti[vvi];

	cd->trig[w->trackerfy - gcvret].index = VARIANT_VOID;

	/* replace the trig with an off message if needed */
	if (getChannelVariant(NULL, cd, w->trackerfy) != -1)
		cd->trig[w->trackerfy - gcvret].index = VARIANT_OFF;

	for (int i = 0; i < cd->variantv[vi]->rowc; i++)
	{
		/* stop burning if another vtrig is encountered */
		if (getChannelVariant(NULL, cd, (w->trackerfy - gcvret) + i) != -1)
			break;

		memcpy(getChannelRow(cd, (w->trackerfy - gcvret) + i), &cd->variantv[vi]->rowv[i], sizeof(Row));
	}

	pruneVariant(cd, vvi);
	regenGlobalRowc(s);
}


void setChordRow(void) {
	clearTooltip(&tt);
	setTooltipTitle(&tt, "variant rows");
	addTooltipBind(&tt, "scale variant to cursor    ", 'c', chordRowScaleToCursor,     NULL);
	addTooltipBind(&tt, "set variant length to count", 'r', chordRowLengthToCount,     NULL);
	addTooltipBind(&tt, "increment variant length   ", 'a', chordRowIncrementLength,   NULL);
	addTooltipBind(&tt, "decrement variant length   ", 'd', chordRowDecrementLength,   NULL);
	addTooltipBind(&tt, "double variant length      ", '+', chordRowCopyDown,          NULL);
	addTooltipBind(&tt, "halve variant length       ", '-', chordRowDiscardHalf,       NULL);
	addTooltipBind(&tt, "stretch variant length     ", '*', chordRowAddBlanks,         NULL);
	addTooltipBind(&tt, "shrink variant length      ", '/', chordRowDiscardEveryOther, NULL);
	addTooltipBind(&tt, "burn variant               ", 'b', chordRowBurn,              NULL);
	w->chord = 'r';
}
