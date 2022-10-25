void drawPluginEffectBrowser(void)
{
	if (ladspa_db.descc == 0)
	{
		char *text = "no effect plugins available";
		printf("\033[%d;%ldH%s", w->centre, (ws.ws_col - strlen(text))>>1, text);
		return;
	}
	int y; /* technically this can overflow, shouldn't be a problem */
	for (size_t i = 0; i < ladspa_db.descc; i++)
	{
		y = i + w->centre - w->plugineffectindex;

		if (y >= 2 && y < ws.ws_row)
		{
			printf("\033[%d;1H%.*s", y, (ws.ws_col>>1) - 4, ladspa_db.descv[i]->Name);
			printf("\033[%d;%dH%.*s", y, ws.ws_col>>1, ws.ws_col - 6 - (ws.ws_col>>1) - 2, ladspa_db.descv[i]->Maker);
			printf("\033[%d;%dH%s", y, ws.ws_col - 6, "LADSPA");
		}
	}

	printf("\033[%d;0H", w->centre + w->fyoffset);
}


void cb_addEffectLadspaAfter(Event *e)
{
	cc.cursor = getCursorFromEffect(*w->pluginbrowserchain, (size_t)e->callbackarg);
	cb_addEffect(e);
}

void pluginEffectBrowserInput(int input)
{
	uint8_t newindex;
	switch (input)
	{
		case '\n': case '\r':
			switch (w->page)
			{
				case PAGE_CHANNEL_EFFECT_PLUGINBROWSER: w->page = PAGE_CHANNEL_EFFECT; break;
				case PAGE_INSTRUMENT_EFFECT_PLUGINBROWSER: w->page = PAGE_INSTRUMENT_EFFECT; break;
			}

			if (w->pluginbrowserbefore)
			{
				newindex = getEffectFromCursor(*w->pluginbrowserchain, cc.cursor);
				addEffect(w->pluginbrowserchain, EFFECT_TYPE_LADSPA, w->plugineffectindex, newindex, cb_addEffect);
			} else
			{
				newindex = MIN(getEffectFromCursor(*w->pluginbrowserchain, cc.cursor) + 1, (*w->pluginbrowserchain)->c);
				addEffect(w->pluginbrowserchain, EFFECT_TYPE_LADSPA, w->plugineffectindex, newindex, cb_addEffectLadspaAfter);
			}

			p->redraw = 1; break;
		case '\033':
			switch (getchar())
			{
				case 'O':
					switch (getchar())
					{
						case 'P': /* xterm f1 */ showTracker   (); break;
						case 'Q': /* xterm f2 */ showInstrument(); break;
					} p->redraw = 1; break;
				case '[': /* CSI */
					switch (getchar())
					{
						case '[':
							switch (getchar())
							{
								case 'A': /* linux f1 */ showTracker   (); break;
								case 'B': /* linux f2 */ showInstrument(); break;
								case 'E': /* linux f5 */ startPlayback(); break;
							} p->redraw = 1; break;
						case 'A': /* up arrow   */ if (w->plugineffectindex)                     { w->plugineffectindex--; p->redraw = 1; } break;
						case 'B': /* down arrow */ if (w->plugineffectindex < ladspa_db.descc-1) { w->plugineffectindex++; p->redraw = 1; } break;
						case 'H': /* xterm home */ w->plugineffectindex = 0; p->redraw = 1; break;
						case '4': /* end */ if (getchar() == '~') { w->plugineffectindex = ladspa_db.descc-1; p->redraw = 1; } break;
						case '1':
							switch (getchar())
							{
								case '5': /* xterm f5   */ getchar(); startPlayback(); break;
								case '7': /*       f6   */ getchar(); stopPlayback (); break;
								case ';': /* mod+arrow  */ getchar(); break;
								case '~': /* linux home */ w->plugineffectindex = 0; p->redraw = 1; break;
							} break;
					} break;
				default: /* escape */
					switch (w->page)
					{
						case PAGE_CHANNEL_EFFECT_PLUGINBROWSER: w->page = PAGE_CHANNEL_EFFECT; break;
						case PAGE_INSTRUMENT_EFFECT_PLUGINBROWSER: w->page = PAGE_INSTRUMENT_EFFECT; break;
					} p->redraw = 1; break;
			} break;
	}
}
