void chordRowScaleToCursor(void *_)
{
	TrackData *cd = &s->track->v[w->track].data;
	int gcvret = getVariantChainPrevVtrig(cd->variant, w->trackerfy);
	if (gcvret == -1 || cd->variant->trig[gcvret].index == VARIANT_OFF) return;
	uint8_t vi = cd->variant->i[cd->variant->trig[gcvret].index];

	Variant *v = dupVariant(cd->variant->v[vi], w->trackerfy - gcvret);

	free(cd->variant->v[vi]); cd->variant->v[vi] = v;
	regenGlobalRowc(s);
}
void chordRowLengthToCount(void *_)
{
	TrackData *cd = &s->track->v[w->track].data;
	int gcvret = getVariantChainVariant(NULL, cd->variant, w->trackerfy);
	if (gcvret == -1) return;
	uint8_t vi = cd->variant->i[cd->variant->trig[w->trackerfy - gcvret].index];

	Variant *v = dupVariant(cd->variant->v[vi], w->count ? w->count-1 : w->defvariantlength);

	free(cd->variant->v[vi]); cd->variant->v[vi] = v;
	if (!(cd->variant->trig[w->trackerfy - gcvret].flags&C_VTRIG_LOOP)
			&& w->trackerfy > w->trackerfy - gcvret + cd->variant->v[vi]->rowc)
		w->trackerfy = w->trackerfy - gcvret + cd->variant->v[vi]->rowc;
	regenGlobalRowc(s);
}
void chordRowIncrementLength(void *_)
{
	TrackData *cd = &s->track->v[w->track].data;
	int gcvret = getVariantChainVariant(NULL, cd->variant, w->trackerfy);
	if (gcvret == -1) return;
	uint8_t vi = cd->variant->i[cd->variant->trig[w->trackerfy - gcvret].index];

	Variant *v = dupVariant(cd->variant->v[vi], MIN(VARIANT_ROWMAX, cd->variant->v[vi]->rowc + MAX(1, w->count)));

	free(cd->variant->v[vi]); cd->variant->v[vi] = v;
	if (!(cd->variant->trig[w->trackerfy - gcvret].flags&C_VTRIG_LOOP)
			&& w->trackerfy > w->trackerfy - gcvret + cd->variant->v[vi]->rowc)
		w->trackerfy = w->trackerfy - gcvret + cd->variant->v[vi]->rowc;
	regenGlobalRowc(s);
}
void chordRowDecrementLength(void *_)
{
	TrackData *cd = &s->track->v[w->track].data;
	int gcvret = getVariantChainVariant(NULL, cd->variant, w->trackerfy);
	if (gcvret == -1) return;
	uint8_t vi = cd->variant->i[cd->variant->trig[w->trackerfy - gcvret].index];

	Variant *v = dupVariant(cd->variant->v[vi], MAX(0, cd->variant->v[vi]->rowc - MAX(1, w->count)));

	free(cd->variant->v[vi]); cd->variant->v[vi] = v;
	if (!(cd->variant->trig[w->trackerfy - gcvret].flags&C_VTRIG_LOOP)
			&& w->trackerfy > w->trackerfy - gcvret + cd->variant->v[vi]->rowc)
		w->trackerfy = w->trackerfy - gcvret + cd->variant->v[vi]->rowc;
	regenGlobalRowc(s);
}
void chordRowCopyDown(void *_)
{
	TrackData *cd = &s->track->v[w->track].data;
	int gcvret = getVariantChainVariant(NULL, cd->variant, w->trackerfy);
	if (gcvret == -1) return;
	uint8_t vi = cd->variant->i[cd->variant->trig[w->trackerfy - gcvret].index];

	Variant *v = dupVariant(NULL, MIN(VARIANT_ROWMAX, (cd->variant->v[vi]->rowc+1)*(2*MAX(1, w->count)) - 1));
	for (int i = 0; i <= v->rowc; i++)
		v->rowv[i] = cd->variant->v[vi]->rowv[i%(cd->variant->v[vi]->rowc+1)];

	free(cd->variant->v[vi]); cd->variant->v[vi] = v;
	if (!(cd->variant->trig[w->trackerfy - gcvret].flags&C_VTRIG_LOOP)
			&& w->trackerfy > w->trackerfy - gcvret + cd->variant->v[vi]->rowc)
		w->trackerfy = w->trackerfy - gcvret + cd->variant->v[vi]->rowc;
	regenGlobalRowc(s);
}
void chordRowDiscardHalf(void *_)
{
	TrackData *cd = &s->track->v[w->track].data;
	int gcvret = getVariantChainVariant(NULL, cd->variant, w->trackerfy);
	if (gcvret == -1) return;
	uint8_t vi = cd->variant->i[cd->variant->trig[w->trackerfy - gcvret].index];

	Variant *v = dupVariant(cd->variant->v[vi], MAX(0, (cd->variant->v[vi]->rowc+1)/(2*MAX(1, w->count)) - 1));

	free(cd->variant->v[vi]); cd->variant->v[vi] = v;
	if (!(cd->variant->trig[w->trackerfy - gcvret].flags&C_VTRIG_LOOP)
			&& w->trackerfy > w->trackerfy - gcvret + cd->variant->v[vi]->rowc)
		w->trackerfy = w->trackerfy - gcvret + cd->variant->v[vi]->rowc;
	regenGlobalRowc(s);
}
void chordRowAddBlanks(void *_)
{
	TrackData *cd = &s->track->v[w->track].data;
	int gcvret = getVariantChainVariant(NULL, cd->variant, w->trackerfy);
	if (gcvret == -1) return;
	uint8_t vi = cd->variant->i[cd->variant->trig[w->trackerfy - gcvret].index];

	Variant *v = dupVariant(NULL, MIN(VARIANT_ROWMAX, (cd->variant->v[vi]->rowc+1)*(2*MAX(1, w->count)) - 1));
	for (int i = 0; i <= cd->variant->v[vi]->rowc; i++)
		v->rowv[i * MAX(1, w->count)*2] = cd->variant->v[vi]->rowv[i];

	free(cd->variant->v[vi]); cd->variant->v[vi] = v;
	if (!(cd->variant->trig[w->trackerfy - gcvret].flags&C_VTRIG_LOOP)
			&& w->trackerfy > w->trackerfy - gcvret + cd->variant->v[vi]->rowc)
		w->trackerfy = w->trackerfy - gcvret + cd->variant->v[vi]->rowc;
	regenGlobalRowc(s);
}
void chordRowDiscardEveryOther(void *_)
{
	TrackData *cd = &s->track->v[w->track].data;
	int gcvret = getVariantChainVariant(NULL, cd->variant, w->trackerfy);
	if (gcvret == -1) return;
	uint8_t vi = cd->variant->i[cd->variant->trig[w->trackerfy - gcvret].index];

	Variant *v = dupVariant(NULL, MAX(0, (cd->variant->v[vi]->rowc+1)/(2*MAX(1, w->count)) - 1));
	for (int i = 0; i <= v->rowc; i++)
		v->rowv[i] = cd->variant->v[vi]->rowv[i * MAX(1, w->count)*2];

	free(cd->variant->v[vi]); cd->variant->v[vi] = v;
	if (!(cd->variant->trig[w->trackerfy - gcvret].flags&C_VTRIG_LOOP)
			&& w->trackerfy > w->trackerfy - gcvret + cd->variant->v[vi]->rowc)
		w->trackerfy = w->trackerfy - gcvret + cd->variant->v[vi]->rowc;
	regenGlobalRowc(s);
}
void chordRowBurn(void *_)
{
	TrackData *cd = &s->track->v[w->track].data;
	int gcvret = getVariantChainVariant(NULL, cd->variant, w->trackerfy);
	if (gcvret == -1) return;
	uint8_t vvi = cd->variant->trig[w->trackerfy - gcvret].index;
	uint8_t vi = cd->variant->i[vvi];

	cd->variant->trig[w->trackerfy - gcvret].index = VARIANT_VOID;

	/* replace the trig with an off message if needed */
	if (getVariantChainVariant(NULL, cd->variant, w->trackerfy) != -1)
		cd->variant->trig[w->trackerfy - gcvret].index = VARIANT_OFF;

	for (int i = 0; i < cd->variant->v[vi]->rowc; i++)
	{
		/* stop burning if another vtrig is encountered */
		if (getVariantChainVariant(NULL, cd->variant, (w->trackerfy - gcvret) + i) != -1)
			break;

		memcpy(getTrackRow(cd, (w->trackerfy - gcvret) + i), &cd->variant->v[vi]->rowv[i], sizeof(Row));
	}

	pruneVariant(&cd->variant, vvi);
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
