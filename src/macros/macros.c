/* process macro .m if it's present in the row */
bool ifMacro(jack_nframes_t fptr, uint16_t *spr, Track *cv, Row *r, char m)
{
	return ifMacroCallback(fptr, spr, cv, r, m, MACRO_TRIGGERCALLBACK(m));
}

/* ifMacro(), but run .triggercallback instead of guessing */
bool ifMacroCallback(jack_nframes_t fptr, uint16_t *spr, Track *cv, Row *r, char m, bool (*triggercallback)(jack_nframes_t, uint16_t*, int, Track*, Row*))
{
	for (int i = 0; i <= cv->variant->macroc; i++)
		if (r->macro[i].c == m && MACRO_SET(r->macro[i].c))
			return triggercallback(fptr, spr, r->macro[i].v, cv, r);
	return triggercallback(fptr, spr, -1, cv, r);
}

/* if the row needs to be ramped in based on the macros present */
bool ifMacroRamp(Track *cv, Row *r)
{
	for (int i = 0; i <= cv->variant->macroc; i++)
		if (MACRO_RAMP(r->macro[i].c)) return 1;

	return 0;
}

void handleMacroType(enum MacroType type, jack_nframes_t fptr, uint16_t *spr, Track *cv, Row *r)
{
	for (size_t i = 0; i < MACRO_MAX; i++)
		if (MACRO_TYPE(i))
			ifMacroCallback(fptr, spr, cv, r, i, MACRO_TRIGGERCALLBACK(i));
}

static int swapCase(int x)
{
	if (islower(x)) return toupper(x);
	else            return tolower(x);
}

bool changeMacro(int input, char *dest)
{
	/* use the current value if input is '\0' */
	if (!input) input = *dest; /* dest is pre-swapped */
	else        input = swapCase(input);

	if (MACRO_SET(input))
		*dest = input;

	return 0;
}

void addMacroBinds(const char *prettyname, unsigned int state, void (*callback)(void*))
{
	addTooltipPrettyPrint(prettyname, state, "macro");
	for (size_t i = 0; i < MACRO_MAX; i++)
		if (MACRO_SET(i))
			addTooltipBind(MACRO_PRETTYNAME(i), state, swapCase(i), 0, callback, (void*)i);
}


// bool midicctargetc(jack_nframes_t fptr, uint16_t *spr, int m, Track *cv, Row r) { cv->midiccindex = m%128; return 1; }
// bool midipcc(jack_nframes_t fptr, uint16_t *spr, int m, Track *cv, Row r)
// {
// 	if (!cv->mute && p->s->instrument->i[(r.inst != INST_VOID) ? r.inst : cv->r.inst] < p->s->instrument->c)
// 	{
// 		Instrument *iv = &p->s->instrument->v[p->s->instrument->i[(r.inst != INST_VOID) ? r.inst : cv->r.inst]];
// 		if (iv->algorithm == INST_ALG_MIDI) midiPC(fptr, iv->midi.channel, m%128);
// 	} return 1;
// }
// bool midiccc(jack_nframes_t fptr, uint16_t *spr, int m, Track *cv, Row r)
// {
// 	cv->midicc = m%128;
// 	if (cv->midiccindex != -1 && !cv->mute && p->s->instrument->i[(r.inst != INST_VOID) ? r.inst : cv->r.inst] < p->s->instrument->c)
// 	{
// 		Instrument *iv = &p->s->instrument->v[p->s->instrument->i[(r.inst != INST_VOID) ? r.inst : cv->r.inst]];
// 		if (iv->algorithm == INST_ALG_MIDI) midiCC(fptr, iv->midi.channel, cv->midiccindex, cv->midicc);
// 	} return 1;
// }
