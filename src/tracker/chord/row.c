void chordRowScaleToCursor(void *_)
{
	Track *cv = &s->track->v[w->track];
	int gcvret = getVariantChainPrevVtrig(cv->variant, w->trackerfy);
	if (gcvret == -1 || cv->variant->trig[gcvret].index == VARIANT_OFF) return;
	uint8_t vi = cv->variant->i[cv->variant->trig[gcvret].index];

	Variant *v = dupVariant(cv->variant->v[vi], w->trackerfy - gcvret);

	free(cv->variant->v[vi]); cv->variant->v[vi] = v;
	regenGlobalRowc(s);
}
void chordRowLengthToCount(void *_)
{
	Track *cv = &s->track->v[w->track];
	int gcvret = getVariantChainVariant(NULL, cv->variant, w->trackerfy);
	if (gcvret == -1) return;
	uint8_t vi = cv->variant->i[cv->variant->trig[w->trackerfy - gcvret].index];

	Variant *v = dupVariant(cv->variant->v[vi], w->count ? w->count-1 : w->defvariantlength);

	free(cv->variant->v[vi]); cv->variant->v[vi] = v;
	if (!(cv->variant->trig[w->trackerfy - gcvret].flags&C_VTRIG_LOOP)
			&& w->trackerfy > w->trackerfy - gcvret + cv->variant->v[vi]->rowc)
		w->trackerfy = w->trackerfy - gcvret + cv->variant->v[vi]->rowc;
	regenGlobalRowc(s);
}
void chordRowIncrementLength(void *_)
{
	Track *cv = &s->track->v[w->track];
	int gcvret = getVariantChainVariant(NULL, cv->variant, w->trackerfy);
	if (gcvret == -1) return;
	uint8_t vi = cv->variant->i[cv->variant->trig[w->trackerfy - gcvret].index];

	Variant *v = dupVariant(cv->variant->v[vi], MIN(VARIANT_ROWMAX, cv->variant->v[vi]->rowc + MAX(1, w->count)));

	free(cv->variant->v[vi]); cv->variant->v[vi] = v;
	if (!(cv->variant->trig[w->trackerfy - gcvret].flags&C_VTRIG_LOOP)
			&& w->trackerfy > w->trackerfy - gcvret + cv->variant->v[vi]->rowc)
		w->trackerfy = w->trackerfy - gcvret + cv->variant->v[vi]->rowc;
	regenGlobalRowc(s);
}
void chordRowDecrementLength(void *_)
{
	Track *cv = &s->track->v[w->track];
	int gcvret = getVariantChainVariant(NULL, cv->variant, w->trackerfy);
	if (gcvret == -1) return;
	uint8_t vi = cv->variant->i[cv->variant->trig[w->trackerfy - gcvret].index];

	Variant *v = dupVariant(cv->variant->v[vi], MAX(0, cv->variant->v[vi]->rowc - MAX(1, w->count)));

	free(cv->variant->v[vi]); cv->variant->v[vi] = v;
	if (!(cv->variant->trig[w->trackerfy - gcvret].flags&C_VTRIG_LOOP)
			&& w->trackerfy > w->trackerfy - gcvret + cv->variant->v[vi]->rowc)
		w->trackerfy = w->trackerfy - gcvret + cv->variant->v[vi]->rowc;
	regenGlobalRowc(s);
}
void chordRowCopyDown(void *_)
{
	Track *cv = &s->track->v[w->track];
	int gcvret = getVariantChainVariant(NULL, cv->variant, w->trackerfy);
	if (gcvret == -1) return;
	uint8_t vi = cv->variant->i[cv->variant->trig[w->trackerfy - gcvret].index];

	Variant *v = dupVariant(NULL, MIN(VARIANT_ROWMAX, (cv->variant->v[vi]->rowc+1)*(2*MAX(1, w->count)) - 1));
	for (int i = 0; i <= v->rowc; i++)
		v->rowv[i] = cv->variant->v[vi]->rowv[i%(cv->variant->v[vi]->rowc+1)];

	free(cv->variant->v[vi]); cv->variant->v[vi] = v;
	if (!(cv->variant->trig[w->trackerfy - gcvret].flags&C_VTRIG_LOOP)
			&& w->trackerfy > w->trackerfy - gcvret + cv->variant->v[vi]->rowc)
		w->trackerfy = w->trackerfy - gcvret + cv->variant->v[vi]->rowc;
	regenGlobalRowc(s);
}
void chordRowDiscardHalf(void *_)
{
	Track *cv = &s->track->v[w->track];
	int gcvret = getVariantChainVariant(NULL, cv->variant, w->trackerfy);
	if (gcvret == -1) return;
	uint8_t vi = cv->variant->i[cv->variant->trig[w->trackerfy - gcvret].index];

	Variant *v = dupVariant(cv->variant->v[vi], MAX(0, (cv->variant->v[vi]->rowc+1)/(2*MAX(1, w->count)) - 1));

	free(cv->variant->v[vi]); cv->variant->v[vi] = v;
	if (!(cv->variant->trig[w->trackerfy - gcvret].flags&C_VTRIG_LOOP)
			&& w->trackerfy > w->trackerfy - gcvret + cv->variant->v[vi]->rowc)
		w->trackerfy = w->trackerfy - gcvret + cv->variant->v[vi]->rowc;
	regenGlobalRowc(s);
}
void chordRowAddBlanks(void *_)
{
	Track *cv = &s->track->v[w->track];
	int gcvret = getVariantChainVariant(NULL, cv->variant, w->trackerfy);
	if (gcvret == -1) return;
	uint8_t vi = cv->variant->i[cv->variant->trig[w->trackerfy - gcvret].index];

	Variant *v = dupVariant(NULL, MIN(VARIANT_ROWMAX, (cv->variant->v[vi]->rowc+1)*(2*MAX(1, w->count)) - 1));
	for (int i = 0; i <= MIN(cv->variant->v[vi]->rowc, VARIANT_ROWMAX>>1); i++)
		v->rowv[i * MAX(1, w->count)*2] = cv->variant->v[vi]->rowv[i];

	free(cv->variant->v[vi]); cv->variant->v[vi] = v;
	if (!(cv->variant->trig[w->trackerfy - gcvret].flags&C_VTRIG_LOOP)
			&& w->trackerfy > w->trackerfy - gcvret + cv->variant->v[vi]->rowc)
		w->trackerfy = w->trackerfy - gcvret + cv->variant->v[vi]->rowc;
	regenGlobalRowc(s);
}
void chordRowDiscardEveryOther(void *_)
{
	Track *cv = &s->track->v[w->track];
	int gcvret = getVariantChainVariant(NULL, cv->variant, w->trackerfy);
	if (gcvret == -1) return;
	uint8_t vi = cv->variant->i[cv->variant->trig[w->trackerfy - gcvret].index];

	Variant *v = dupVariant(NULL, MAX(0, (cv->variant->v[vi]->rowc+1)/(2*MAX(1, w->count)) - 1));
	for (int i = 0; i <= v->rowc; i++)
		v->rowv[i] = cv->variant->v[vi]->rowv[i * MAX(1, w->count)*2];

	free(cv->variant->v[vi]); cv->variant->v[vi] = v;
	if (!(cv->variant->trig[w->trackerfy - gcvret].flags&C_VTRIG_LOOP)
			&& w->trackerfy > w->trackerfy - gcvret + cv->variant->v[vi]->rowc)
		w->trackerfy = w->trackerfy - gcvret + cv->variant->v[vi]->rowc;
	regenGlobalRowc(s);
}
void chordRowBurn(void *_)
{
	Track *cv = &s->track->v[w->track];
	int gcvret = getVariantChainVariant(NULL, cv->variant, w->trackerfy);
	if (gcvret == -1) return;
	uint8_t vvi = cv->variant->trig[w->trackerfy - gcvret].index;
	uint8_t vi = cv->variant->i[vvi];

	cv->variant->trig[w->trackerfy - gcvret].index = VARIANT_VOID;

	/* replace the trig with an off message if needed */
	if (getVariantChainVariant(NULL, cv->variant, w->trackerfy) != -1)
		cv->variant->trig[w->trackerfy - gcvret].index = VARIANT_OFF;

	for (int i = 0; i < cv->variant->v[vi]->rowc; i++)
	{
		/* stop burning if another vtrig is encountered */
		if (getVariantChainVariant(NULL, cv->variant, (w->trackerfy - gcvret) + i) != -1)
			break;

		memcpy(getTrackRow(cv, (w->trackerfy - gcvret) + i), &cv->variant->v[vi]->rowv[i], sizeof(Row));
	}

	pruneVariant(&cv->variant, vvi);
	regenGlobalRowc(s);
}


void setChordRow(void)
{
	clearTooltip();
	setTooltipTitle("variant rows");
	addCountBinds(0);
	addTooltipBind("scale variant to cursor    ", 0, XK_c       , TT_DRAW, chordRowScaleToCursor    , NULL);
	addTooltipBind("set variant length to count", 0, XK_r       , TT_DRAW, chordRowLengthToCount    , NULL);
	addTooltipBind("increment variant length   ", 0, XK_a       , TT_DRAW, chordRowIncrementLength  , NULL);
	addTooltipBind("decrement variant length   ", 0, XK_d       , TT_DRAW, chordRowDecrementLength  , NULL);
	addTooltipBind("double variant length      ", 0, XK_plus    , TT_DRAW, chordRowCopyDown         , NULL);
	addTooltipBind("halve variant length       ", 0, XK_minus   , TT_DRAW, chordRowDiscardHalf      , NULL);
	addTooltipBind("stretch variant length     ", 0, XK_asterisk, TT_DRAW, chordRowAddBlanks        , NULL);
	addTooltipBind("shrink variant length      ", 0, XK_slash   , TT_DRAW, chordRowDiscardEveryOther, NULL);
	addTooltipBind("burn variant               ", 0, XK_b       , TT_DRAW, chordRowBurn             , NULL);
	addTooltipBind("return"                     , 0, XK_Escape  , 0      , NULL                     , NULL);
	w->chord = 'r'; p->redraw = 1;
}
