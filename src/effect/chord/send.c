static void chordAddSend(void)
{
	EffectChain *ec = s->track->v[w->track]->effect;

	for (int i = 0; i < MAX(1, w->count); i++)
		if (ec->sendc < SEND_COUNT)
		{
			ec->sendv[ec->sendc].target = 0;
			ec->sendv[ec->sendc].inputgain = 0;
			ec->sendc++;
		}
	p->redraw = 1;
}
static void chordDelSend(void)
{
	EffectChain *ec = s->track->v[w->track]->effect;

	for (int i = 0; i < MAX(1, w->count); i++)
		if (ec->sendc)
			ec->sendc--;
	p->redraw = 1;
}
static void chordSetSend(void)
{
	EffectChain *ec = s->track->v[w->track]->effect;

	uint8_t oldsendc = ec->sendc;
	ec->sendc = MIN(SEND_COUNT, w->count);

	for (uint8_t i = oldsendc; i < ec->sendc; i++)
	{
		ec->sendv[i].target = 0;
		ec->sendv[i].inputgain = 0;
	}
}

void setChordSend(void)
{
	clearTooltip();
	setTooltipTitle("send");
	addCountBinds(0);
	addTooltipBind("increment send channels   ", 0, XK_a     , TT_DRAW, (void(*)(void*))chordAddSend, NULL);
	addTooltipBind("decrement send channels   ", 0, XK_d     , TT_DRAW, (void(*)(void*))chordDelSend, NULL);
	addTooltipBind("set send channels to count", 0, XK_e     , TT_DRAW, (void(*)(void*))chordSetSend, NULL);
	addTooltipBind("return"                    , 0, XK_Escape, 0      , NULL                        , NULL);
	w->chord = 'e'; p->redraw = 1;
}

