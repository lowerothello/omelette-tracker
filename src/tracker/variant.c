Variant *dupVariant(Variant *oldvariant, uint16_t newlen)
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

static VariantChain *reallocVariantChain(VariantChain *vc, uint8_t length)
{ return realloc(vc, sizeof(VariantChain) + length*sizeof(Variant*)); }

VariantChain *dupVariantChain(VariantChain *vc)
{
	VariantChain *ret = reallocVariantChain(NULL, vc->c);
	memcpy(ret, vc, sizeof(VariantChain) + vc->c*sizeof(Variant*));
	return ret;
}

uint8_t dupEmptyVariantIndex(VariantChain *vc, uint8_t fallbackindex)
{
	for (short i = 0; i < VARIANT_MAX; i++)
		if (vc->i[i] == VARIANT_VOID)
		{
			if (fallbackindex != VARIANT_VOID && fallbackindex != VARIANT_OFF && variantPopulated(vc, fallbackindex))
			{ /* duplicate the fallback index if it makes sense to do so */
				vc->i[i] = vc->c;
				vc->v[vc->c] = dupVariant(vc->v[vc->i[fallbackindex]], vc->v[vc->i[fallbackindex]]->rowc);
				vc->c++;
			} return i;
		}
	return fallbackindex;
}

/* the effective rowc, how many rows are actually used */
uint16_t getSignificantRowc(VariantChain *vc)
{
	/* get the length if only the variant triggers */
	/* walk through 0, expect underflow */
	int lvtrig = 0;
	for (uint16_t i = vc->main->rowc-1; i < USHRT_MAX; i--)
	{
		if (vc->trig[i].index != VARIANT_VOID)
		{
			lvtrig = i;
			if (vc->trig[i].index != VARIANT_OFF)
				lvtrig += vc->v[vc->i[vc->trig[i].index]]->rowc;
			break;
		}
	}

	for (uint16_t i = vc->main->rowc-1; i != USHRT_MAX; i--)
	{
		if (vc->main->rowv[i].note != NOTE_VOID) return MAX(lvtrig, i);
		if (vc->main->rowv[i].inst != INST_VOID) return MAX(lvtrig, i);
		for (short j = 0; j <= vc->macroc; j++)
			if (vc->main->rowv[i].macro[j].c) return MAX(lvtrig, i);
	}

	return lvtrig+1;
}

/* invalidates past getRow() calls */
void resizeVariantChain(VariantChain *vc, uint16_t newlen)
{
	Vtrig *newtrig = calloc(newlen, sizeof(Vtrig));

	/* can't initialize with memset cos flags will be set to VARIANT_VOID as well */
	for (uint16_t i = 0; i < newlen; i++)
		newtrig[i].index = VARIANT_VOID;

	vc->songlen = newlen;

	/* depends on songv->rowc */
	if (vc->main && vc->trig)
		memcpy(newtrig, vc->trig, MIN(vc->main->rowc, newlen) * sizeof(Vtrig));

	if (vc->trig) free(vc->trig);
	vc->trig = newtrig;

	Variant *temp = dupVariant(vc->main, newlen);
	free(vc->main); vc->main = temp;
}

int addVariant(VariantChain **vc, uint8_t index, uint8_t length)
{
	if (index == VARIANT_VOID || (*vc)->i[index] != VARIANT_VOID) return 1;

	*vc = reallocVariantChain(*vc, (*vc)->c+1);

	(*vc)->i[index] = (*vc)->c;
	(*vc)->v[(*vc)->c] = dupVariant(NULL, length);
	(*vc)->c++;
	return 0;
}
int delVariant(VariantChain **vc, uint8_t index)
{
	if ((*vc)->i[index] == VARIANT_VOID) return 1; /* index not occupied */

	uint8_t cutindex = (*vc)->i[index];
	free((*vc)->v[cutindex]); (*vc)->v[cutindex] = NULL;

	(*vc)->i[index] = VARIANT_VOID;

	(*vc)->c--;
	memmove(&(*vc)->v[cutindex], &(*vc)->v[cutindex+1], ((*vc)->c - cutindex)*sizeof(Variant*));
	*vc = reallocVariantChain(*vc, (*vc)->c);

	for (uint8_t i = 0; i < VARIANT_MAX; i++)
		if ((*vc)->i[i] > cutindex && (*vc)->i[i] != VARIANT_VOID)
			(*vc)->i[i]--;

	return 0;
}

void freeVariantChain(VariantChain **vc)
{
	if (*vc)
	{
		if ((*vc)->trig) free((*vc)->trig);
		if ((*vc)->main) free((*vc)->main);
		for (uint8_t i = 0; i < (*vc)->c; i++)
			free((*vc)->v[i]);
		free(*vc); *vc = NULL;
	}
}

/* returns true if the variant is popuplated */
bool variantPopulated(VariantChain *vc, uint8_t index)
{
	for (int i = 0; i < vc->v[vc->i[index]]->rowc; i++)
	{
		if (vc->v[vc->i[index]]->rowv[i].note != NOTE_VOID) return 1;
		for (short j = 0; j <= vc->macroc; j++)
			if (vc->v[vc->i[index]]->rowv[i].macro[j].c) return 1;
	} return 0;
}

/* remove variant if it's empty           */
/* returns true if the variant was pruned */
bool pruneVariant(VariantChain **vc, uint8_t index)
{
	if (index == VARIANT_VOID || index == VARIANT_OFF
			|| (*vc)->i[index] == VARIANT_VOID)
		return 0;

	/* fail if variant is still referenced */
	for (int i = 0; i < (*vc)->main->rowc; i++)
		if ((*vc)->trig[i].index == index) return 0;

	/* fail if variant if populated */
	if (variantPopulated(*vc, index)) return 0;

	delVariant(vc, index);
	return 1;
}

/* returns the last (if any) variant trigger */
/* returns -1 if no vtrig is within range    */
int getVariantChainPrevVtrig(VariantChain *vc, uint16_t index)
{
	/* walk through 0, expect underflow */
	uint16_t iterstop = USHRT_MAX;
	if (index > 256) iterstop = index - 256; /* only walk up to 256 steps backwards (the longest a vtrig can be) */
	for (uint16_t i = index; i != iterstop; i--)
		if (vc->trig[i].index != VARIANT_VOID) return i;

	return -1;
}

uint8_t getEmptyVariantIndex(VariantChain *vc, uint8_t fallbackindex)
{
	for (short i = 0; i < VARIANT_MAX; i++)
		if (vc->i[i] == VARIANT_VOID)
			return i;
	return fallbackindex;
}

void inputVariantChainTrig(VariantChain **vc, uint16_t index, char value)
{
	uint8_t oldvariant = (*vc)->trig[index].index;

	/* initialize to zero before adjusting */
	if ((*vc)->trig[index].index == VARIANT_VOID)
		(*vc)->trig[index].index = 0;

	(*vc)->trig[index].index <<= 4; (*vc)->trig[index].index += value;
	if ((*vc)->trig[index].index == VARIANT_VOID)
		(*vc)->trig[index].flags = 0;

	pruneVariant(vc, oldvariant);
	addVariant(vc, (*vc)->trig[index].index, w->defvariantlength);
}

/* prunes the old index and sets the new index */
void setVariantChainTrig(VariantChain **vc, uint16_t index, uint8_t value)
{
	uint8_t oldvariant = (*vc)->trig[index].index;

	(*vc)->trig[index].index = value;
	if ((*vc)->trig[index].index == VARIANT_VOID)
		(*vc)->trig[index].flags = 0;

	pruneVariant(vc, oldvariant);
	addVariant(vc, (*vc)->trig[index].index, w->defvariantlength);
}

int getVariantChainVariant(Variant **output, VariantChain *vc, uint16_t index)
{
	int i = getVariantChainPrevVtrig(vc, index);
	if (i != -1 && vc->trig[i].index != VARIANT_OFF
			&& (vc->trig[i].flags&C_VTRIG_LOOP
			|| (vc->i[vc->trig[i].index] < vc->c && vc->v[vc->i[vc->trig[i].index]]->rowc >= index - i)))
	{
		if (output) *output = vc->v[vc->i[vc->trig[i].index]];
		return index - i;
	} return -1; /* fallback */
}
int getTrackVariantNoLoop(Variant **output, VariantChain *vc, uint16_t index)
{
	int i = getVariantChainPrevVtrig(vc, index);
	if (i != -1 && vc->trig[i].index != VARIANT_OFF
			&& vc->v[vc->i[vc->trig[i].index]]->rowc >= index - i)
	{
		if (output) *output = vc->v[vc->i[vc->trig[i].index]];
		return index - i;
	} return -1; /* fallback */
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
	*v = dupVariant(NULL, rowc);
	fread((*v)->rowv, sizeof(Row), rowc, fp);
}

void serializeVariantChain(VariantChain *vc, FILE *fp)
{
	fwrite(&vc->songlen, sizeof(uint16_t), 1, fp);
	fwrite(vc->trig, sizeof(Vtrig), vc->songlen, fp);
	fwrite(vc->main->rowv, sizeof(Row), vc->songlen, fp);

	fputc(vc->macroc, fp);

	fputc(vc->c, fp);
	fwrite(vc->i, sizeof(uint8_t), VARIANT_MAX, fp);

	for (int i = 0; i < vc->c; i++)
		serializeVariant(vc->v[i], fp);
}
void deserializeVariantChain(VariantChain *vc, FILE *fp, uint8_t major, uint8_t minor)
{
	fread(&vc->songlen, sizeof(uint16_t), 1, fp);
	resizeVariantChain(vc, vc->songlen);
	fread(vc->trig, sizeof(Vtrig), vc->songlen, fp);
	fread(vc->main->rowv, sizeof(Row), vc->songlen, fp);


	vc->macroc = fgetc(fp);

	vc->c = fgetc(fp);
	fread(vc->i, sizeof(uint8_t), VARIANT_MAX, fp);

	for (int i = 0; i < vc->c; i++)
		deserializeVariant(&vc->v[i], fp);

}
