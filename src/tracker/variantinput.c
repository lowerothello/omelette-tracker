int inputChannelVariantEscape(int input)
{
	ChannelData *cd = &s->channel->v[w->channel].data;
	Row *r = getChannelRow(cd, w->trackerfy);
	bool ret = 0;

	switch (input)
	{
		case 1:  /* alt+^a */
			switch (w->mode)
			{
				case T_MODE_VISUAL:     addPartPattern( MAX(1, w->count)*12, MIN(tfxToVfx(w->trackerfx), w->visualfx), MAX(tfxToVfx(w->trackerfx), w->visualfx), MIN(w->trackerfy, w->visualfy), MAX(w->trackerfy, w->visualfy), MIN(w->channel, w->visualchannel), MAX(w->channel, w->visualchannel), 1, 1); break;
				case T_MODE_VISUALLINE: addPartPattern( MAX(1, w->count)*12, TRACKERFX_VISUAL_MIN, 2+cd->macroc, MIN(w->trackerfy, w->visualfy), MAX(w->trackerfy, w->visualfy), MIN(w->channel, w->visualchannel), MAX(w->channel, w->visualchannel), 1, 0); break;
				case T_MODE_NORMAL: case T_MODE_INSERT: addPartPattern(MAX(1, w->count)*12, tfxToVfx(w->trackerfx), tfxToVfx(w->trackerfx), w->trackerfy, w->trackerfy, w->channel, w->channel, 1, 1); break;
			} regenGlobalRowc(s); return 1;
		case 24: /* alt+^x */
			switch (w->mode)
			{
				case T_MODE_VISUAL:     addPartPattern(-MAX(1, w->count)*12, MIN(tfxToVfx(w->trackerfx), w->visualfx), MAX(tfxToVfx(w->trackerfx), w->visualfx), MIN(w->trackerfy, w->visualfy), MAX(w->trackerfy, w->visualfy), MIN(w->channel, w->visualchannel), MAX(w->channel, w->visualchannel), 1, 1); break;
				case T_MODE_VISUALLINE: addPartPattern(-MAX(1, w->count)*12, TRACKERFX_VISUAL_MIN, 2+cd->macroc, MIN(w->trackerfy, w->visualfy), MAX(w->trackerfy, w->visualfy), MIN(w->channel, w->visualchannel), MAX(w->channel, w->visualchannel), 1, 0); break;
				case T_MODE_NORMAL: case T_MODE_INSERT: addPartPattern(-MAX(1, w->count)*12, tfxToVfx(w->trackerfx), tfxToVfx(w->trackerfx), w->trackerfy, w->trackerfy, w->channel, w->channel, 1, 1); break;
			} regenGlobalRowc(s); return 1;
		default: /* alt macros */
			if (w->mode == T_MODE_NORMAL && w->chord == 'I')
			{
				changeMacro(input, &w->keyboardmacro, &w->keyboardmacroalt, 1);
				w->mode = T_MODE_INSERT;
				p->redraw = 1; return 1;
			} else
				switch (w->mode)
				{
					case T_MODE_INSERT: case T_MODE_NORMAL:
						if (w->trackerfx > 1) /* cursor is over a macro column */
						{
							short macro = (w->trackerfx - 2)>>1;
							switch (input)
							{
								case '~': /* toggle alt */
									r->macro[macro].alt = !r->macro[macro].alt;
									p->redraw = 1; ret = 1;
								default:
									p->redraw = 1; /* no race condition here cos this function is run from the same thread that redraws the screen */
									if (!(w->trackerfx&1)) ret = insertMacroc(&r->macro[macro], input, 1);
									else                   ret = insertMacrov(&r->macro[macro], input);
							} regenGlobalRowc(s); return ret;
						} break;
					case T_MODE_VISUALREPLACE:
						if (w->trackerfx > 1) /* cursor is over a macro column */
						{
							short macro = (w->trackerfx - 2)>>1;
							if (!(w->trackerfx&1))
							{ /* macroc */
								for (int i = MIN(w->trackerfy, w->visualfy); i <= MAX(w->trackerfy, w->visualfy); i++)
									ret = insertMacroc(&getChannelRow(cd, i)->macro[macro], input, 1);
							} else
							{ /* macrov */
								for (int i = MIN(w->trackerfy, w->visualfy); i <= MAX(w->trackerfy, w->visualfy); i++)
									ret = insertMacrov(&getChannelRow(cd, i)->macro[macro], input);
							} regenGlobalRowc(s); return ret;
						} break;
					case T_MODE_VISUAL:     if (input == '~') { altTildePartPattern(MIN(tfxToVfx(w->trackerfx), w->visualfx), MAX(tfxToVfx(w->trackerfx), w->visualfx), MIN(w->trackerfy, w->visualfy), MAX(w->trackerfy, w->visualfy), MIN(w->channel, w->visualchannel), MAX(w->channel, w->visualchannel)); return 1; } break;
					case T_MODE_VISUALLINE: if (input == '~') { altTildePartPattern(TRACKERFX_VISUAL_MIN, 2+cd->macroc, MIN(w->trackerfy, w->visualfy), MAX(w->trackerfy, w->visualfy), MIN(w->channel, w->visualchannel), MAX(w->channel, w->visualchannel)); return 1; } break;
				}
	} return 0;
}

int inputChannelVariant(int input)
{
	uint8_t vindex, vlength;
	int i, j;
	short macro, vminrow;
	Row *r;
	ChannelData *cd = &s->channel->v[w->channel].data;

	switch (w->mode)
	{
		case T_MODE_NORMAL:
			switch (input)
			{
				case '\n': case '\r': /*  mute  */ toggleChannelMute(w->channel); p->redraw = 1; break;
				case 's':             /*  solo  */ toggleChannelSolo(w->channel); p->redraw = 1; break;
				case 'f': /* toggle song follow */ w->follow = !w->follow; if (s->playing) w->trackerfy = s->playfy; p->redraw = 1; break;
				case 'I': /* macro insert mode  */ w->chord = 'I'; p->redraw = 1; return 1;
				case 'i': /* enter insert mode  */ w->mode = T_MODE_INSERT; p->redraw = 1; break;
				// case 'u': /* undo               */ popPatternHistory(s->patterni[s->trig[w->songfy].index]); p->redraw = 1; break;
				// case 18:  /* redo               */ unpopPatternHistory(s->patterni[s->trig[w->songfy].index]); p->redraw = 1; break;
				case 'k': /* up arrow           */ trackerUpArrow  (MAX(1, w->count)); p->redraw = 1; break;
				case 'j': /* down arrow         */ trackerDownArrow(MAX(1, w->count)); p->redraw = 1; break;
				case 'h': /* left arrow         */ trackerLeftArrow (MAX(1, w->count)); p->redraw = 1; break;
				case 'l': /* right arrow        */ trackerRightArrow(MAX(1, w->count)); p->redraw = 1; break;
				case '[': /* chnl left          */ channelLeft (MAX(1, w->count)); p->redraw = 1; break;
				case ']': /* chnl right         */ channelRight(MAX(1, w->count)); p->redraw = 1; break;
				case '{': /* variant cycle up   */ cycleUp  (MAX(1, w->count)); break;
				case '}': /* variant cycle down */ cycleDown(MAX(1, w->count)); break;
				case '<': /* song shift up      */ shiftUp  (MAX(1, w->count)); break;
				case '>': /* song shift down    */ shiftDown(MAX(1, w->count)); break;
				case 'v': case 22: /* enter visual mode */
					w->visualfx = tfxToVfx(w->trackerfx);
					w->visualfy = w->trackerfy;
					w->visualchannel = w->channel;
					w->mode = T_MODE_VISUAL;
					p->redraw = 1; break;
				case 'V': /* enter visual line mode */
					w->visualfx = tfxToVfx(w->trackerfx);
					w->visualfy = w->trackerfy;
					w->visualchannel = w->channel;
					w->mode = T_MODE_VISUALLINE;
					p->redraw = 1; break;
				case 'y':  /* yank             */ setChordYankRow  (); p->redraw = 1; return 1;
				case 'd':  /* delete           */ setChordDeleteRow(); p->redraw = 1; return 1;
				case 'c':  /* channel          */ setChordChannel  (); p->redraw = 1; return 1;
				case 'm':  /* macro            */ setChordMacro    (); p->redraw = 1; return 1;
				case 'r':  /* row              */ setChordRow      (); p->redraw = 1; return 1;
				case ';':  /* loop             */ setChordLoop     (); p->redraw = 1; return 1;
				// case 'e':  /* add empty inst   */
					/* w->instrument = emptyInstrument(0);
					setChordAddInst(); w->chord = 'e';
					p->redraw = 1; return 1; */
				case 'g':  /* graphic misc     */ w->chord = 'g'; p->redraw = 1; return 1;
				case 'G':  /* graphic end      */ trackerEnd(); p->redraw = 1; break;
				case 't':  /* row highlight    */ if (w->count) { s->rowhighlight = MIN(16, w->count); regenGlobalRowc(s); } break;
				case 'o':  /* octave           */
					r = getChannelRow(cd, w->trackerfy);
					if (!w->trackerfx) r->note = changeNoteOctave(MIN(9, w->count), r->note);
					else               w->octave = MIN(9, w->count);
					p->redraw = 1; break;
				case 'p':  /* pattern put */ /* TODO: count */
					putPartPattern();
					if (w->pbchannelc)
						trackerDownArrow(w->pbvariantv[0]->rowc);
					break;
				case 'P': /* pattern put before */
					mixPutPartPattern();
					break;
				case 'x': case 127: case '\b': /* backspace */
					if (!w->trackerfx) /* edge case to clear both the note and inst columns */
					{
						yankPartPattern(0, 1, w->trackerfy, w->trackerfy, w->channel, w->channel);
						delPartPattern (0, 1, w->trackerfy, w->trackerfy, w->channel, w->channel);
					} else
					{
						yankPartPattern(tfxToVfx(w->trackerfx), tfxToVfx(w->trackerfx), w->trackerfy, w->trackerfy, w->channel, w->channel);
						delPartPattern (tfxToVfx(w->trackerfx), tfxToVfx(w->trackerfx), w->trackerfy, w->trackerfy, w->channel, w->channel);
					} break;
				case '%': /* jump between loop points */
					if (s->loop[1])
					{
						if      (w->trackerfy == s->loop[0]) w->trackerfy = s->loop[1];
						else if (w->trackerfy == s->loop[1]) w->trackerfy = s->loop[0];
						else if (w->trackerfy < (s->loop[0] + s->loop[1])>>1) w->trackerfy = s->loop[0];
						else                                                  w->trackerfy = s->loop[1];
					} p->redraw = 1; break;
				default: /* column specific */
					r = getChannelRow(cd, w->trackerfy);
					switch (w->trackerfx)
					{
						case -1: /* vtriggers */
							switch (input)
							{
								case 1:   /* ^a         */ addPartPattern( MAX(1, w->count), tfxToVfx(w->trackerfx), tfxToVfx(w->trackerfx), w->trackerfy, w->trackerfy, w->channel, w->channel, 0, 1); break;
								case 24:  /* ^x         */ addPartPattern(-MAX(1, w->count), tfxToVfx(w->trackerfx), tfxToVfx(w->trackerfx), w->trackerfy, w->trackerfy, w->channel, w->channel, 0, 1); break;
								case 'a': /* add empty  */ setChannelTrig(cd, w->trackerfy, _duplicateEmptyVariantIndex(cd, cd->trig[w->trackerfy].index)); break;
								case '.': /* vtrig loop */ i = getPrevVtrig(cd, w->trackerfy); if (i != -1) cd->trig[i].flags ^= C_VTRIG_LOOP; break;
							} break;
						case 0: /* note */
							switch (input)
							{
								case 1:  /* ^a */ addPartPattern( MAX(1, w->count), tfxToVfx(w->trackerfx), tfxToVfx(w->trackerfx), w->trackerfy, w->trackerfy, w->channel, w->channel, 0, 1); break;
								case 24: /* ^x */ addPartPattern(-MAX(1, w->count), tfxToVfx(w->trackerfx), tfxToVfx(w->trackerfx), w->trackerfy, w->trackerfy, w->channel, w->channel, 0, 1); break;
							} break;
						case 1: /* instrument */
							switch (input)
							{
								case 1:  /* ^a */ addPartPattern( MAX(1, w->count), tfxToVfx(w->trackerfx), tfxToVfx(w->trackerfx), w->trackerfy, w->trackerfy, w->channel, w->channel, 0, 1);
									if (w->instrumentrecv == INST_REC_LOCK_OK) w->instrument = r->inst;
									break;
								case 24: /* ^x */ addPartPattern(-MAX(1, w->count), tfxToVfx(w->trackerfx), tfxToVfx(w->trackerfx), w->trackerfy, w->trackerfy, w->channel, w->channel, 0, 1);
									if (w->instrumentrecv == INST_REC_LOCK_OK) w->instrument = r->inst;
									break;
							} break;
						default: /* macro */
							macro = (w->trackerfx - 2)>>1;
							switch (input)
							{
								case 1:  /* ^a */ addPartPattern( MAX(1, w->count), tfxToVfx(w->trackerfx), tfxToVfx(w->trackerfx), w->trackerfy, w->trackerfy, w->channel, w->channel, 0, 1); break;
								case 24: /* ^x */ addPartPattern(-MAX(1, w->count), tfxToVfx(w->trackerfx), tfxToVfx(w->trackerfx), w->trackerfy, w->trackerfy, w->channel, w->channel, 0, 1); break;
								case '~': /* toggle case */
									if      (isupper(r->macro[macro].c)) changeMacro(r->macro[macro].c, &r->macro[macro].c, &r->macro[macro].alt, r->macro[macro].alt);
									else if (islower(r->macro[macro].c)) changeMacro(r->macro[macro].c, &r->macro[macro].c, &r->macro[macro].alt, r->macro[macro].alt);
									break;
							} break;
					} regenGlobalRowc(s); break;
			} break;
		case T_MODE_INSERT:
			r = getChannelRow(cd, w->trackerfy);
			if (input == '\n' || input == '\r') /* mute */ { toggleChannelMute(w->channel); p->redraw = 1; }
			else
				switch (w->trackerfx)
				{
					case -1: /* vtriggers  */ insertVtrig(cd, w->trackerfy, input); regenGlobalRowc(s); break;
					case 0:  /* note       */
						insertNote(r, input); /* invalidates r, TODO: previewNote should be event aware, there's probably a race condition currently */
						regenGlobalRowc(s);
						previewNote(input, getChannelRow(cd, w->trackerfy)->inst);
						trackerDownArrow(w->step);
						break;
					case 1:  /* instrument */ insertInst(r, input); regenGlobalRowc(s); break;
					default: /* macro      */ insertMacro(&r->macro[(w->trackerfx - 2)>>1], input, 0); regenGlobalRowc(s); break;
				} break;
		case T_MODE_VISUALREPLACE:
			if (input == '\n' || input == '\r') /* mute */ { toggleChannelMute(w->channel); p->redraw = 1; }
			else
			{
				switch (w->trackerfx)
				{
					case -1: /* vtrig */
						for (i = MIN(w->trackerfy, w->visualfy); i <= MAX(w->trackerfy, w->visualfy); i++)
							insertVtrig(cd, i, input);
						regenGlobalRowc(s); break;
					case 0: /* note */
						for (i = MIN(w->trackerfy, w->visualfy); i <= MAX(w->trackerfy, w->visualfy); i++)
							insertNote(getChannelRow(cd, i), input);
						/* <preview> */
						previewNote(input, getChannelRow(cd, w->trackerfy)->inst);
						/* </preview> */
						regenGlobalRowc(s); break;
					case 1: /* inst */
						for (i = MIN(w->trackerfy, w->visualfy); i <= MAX(w->trackerfy, w->visualfy); i++)
							insertInst(getChannelRow(cd, i), input);
						regenGlobalRowc(s); break;
					default: /* macro */
						macro = (w->trackerfx - 2)>>1;
						if (!(w->trackerfx&1))
						{ /* macroc */
							for (i = MIN(w->trackerfy, w->visualfy); i <= MAX(w->trackerfy, w->visualfy); i++)
								insertMacroc(&getChannelRow(cd, i)->macro[macro], input, 0);
						} else
						{ /* macrov */
							for (i = MIN(w->trackerfy, w->visualfy); i <= MAX(w->trackerfy, w->visualfy); i++)
								insertMacrov(&getChannelRow(cd, i)->macro[macro], input);
						} regenGlobalRowc(s); break;
				} break;
			} break;
		case T_MODE_VISUAL: case T_MODE_VISUALLINE:
			switch (input)
			{
				case '\n': case '\r': /* channel mute */
					for (i = MIN(w->channel, w->visualchannel); i <= MAX(w->channel, w->visualchannel); i++)
						toggleChannelMute(i);
					p->redraw = 1; break;
				case 's': /* channel solo */
					for (i = MIN(w->channel, w->visualchannel); i <= MAX(w->channel, w->visualchannel); i++)
						toggleChannelSolo(i);
					p->redraw = 1; break;
				case 'v': case 22: /* visual */
					switch (w->mode)
					{
						case T_MODE_VISUAL:     w->mode = T_MODE_NORMAL; p->redraw = 1; break;
						case T_MODE_VISUALLINE: w->mode = T_MODE_VISUAL; p->redraw = 1; break;
					} break;
				case 'V': /* visual line */
					switch (w->mode)
					{
						case T_MODE_VISUAL:     w->mode = T_MODE_VISUALLINE; p->redraw = 1; break;
						case T_MODE_VISUALLINE: w->mode = T_MODE_NORMAL;     p->redraw = 1; break;
					} break;
				case 'r': /* replace     */ w->mode = T_MODE_VISUALREPLACE; p->redraw = 1; break;;
				case 'k': /* up arrow    */ trackerUpArrow  (MAX(1, w->count)); p->redraw = 1; break;
				case 'j': /* down arrow  */ trackerDownArrow(MAX(1, w->count)); p->redraw = 1; break;
				case 'h': /* left arrow  */ trackerLeftArrow (MAX(1, w->count)); p->redraw = 1; break;
				case 'l': /* right arrow */ trackerRightArrow(MAX(1, w->count)); p->redraw = 1; break;
				case '[': /* chnl left   */ channelLeft      (MAX(1, w->count)); p->redraw = 1; break;
				case ']': /* chnl right  */ channelRight     (MAX(1, w->count)); p->redraw = 1; break;
				case '{': /* variant cycle up   */ cycleUp  (MAX(1, w->count)); break;
				case '}': /* variant cycle down */ cycleDown(MAX(1, w->count)); break;
				case '~': /* vi tilde */
					switch (w->mode)
					{
						case T_MODE_VISUAL:     tildePartPattern(MIN(tfxToVfx(w->trackerfx), w->visualfx), MAX(tfxToVfx(w->trackerfx), w->visualfx), MIN(w->trackerfy, w->visualfy), MAX(w->trackerfy, w->visualfy), MIN(w->channel, w->visualchannel), MAX(w->channel, w->visualchannel)); break;
						case T_MODE_VISUALLINE: tildePartPattern(TRACKERFX_VISUAL_MIN, 2+cd->macroc, MIN(w->trackerfy, w->visualfy), MAX(w->trackerfy, w->visualfy), MIN(w->channel, w->visualchannel), MAX(w->channel, w->visualchannel)); break;
					} break;
				case 'i': /* interpolate */
					switch (w->mode)
					{
						case T_MODE_VISUAL:     interpolatePartPattern(MIN(tfxToVfx(w->trackerfx), w->visualfx), MAX(tfxToVfx(w->trackerfx), w->visualfx), MIN(w->trackerfy, w->visualfy), MAX(w->trackerfy, w->visualfy), MIN(w->channel, w->visualchannel), MAX(w->channel, w->visualchannel)); break;
						case T_MODE_VISUALLINE: interpolatePartPattern(TRACKERFX_VISUAL_MIN, 2+cd->macroc, MIN(w->trackerfy, w->visualfy), MAX(w->trackerfy, w->visualfy), MIN(w->channel, w->visualchannel), MAX(w->channel, w->visualchannel)); break;
					} break;
				case '%': /* random */
					switch (w->mode)
					{
						case T_MODE_VISUAL:     randPartPattern(MIN(tfxToVfx(w->trackerfx), w->visualfx), MAX(tfxToVfx(w->trackerfx), w->visualfx), MIN(w->trackerfy, w->visualfy), MAX(w->trackerfy, w->visualfy), MIN(w->channel, w->visualchannel), MAX(w->channel, w->visualchannel)); break;
						case T_MODE_VISUALLINE: randPartPattern(TRACKERFX_VISUAL_MIN, 2+cd->macroc, MIN(w->trackerfy, w->visualfy), MAX(w->trackerfy, w->visualfy), MIN(w->channel, w->visualchannel), MAX(w->channel, w->visualchannel)); break;
					} break;
				case 1: /* ^a */
					switch (w->mode)
					{
						case T_MODE_VISUAL:     addPartPattern( MAX(1, w->count), MIN(tfxToVfx(w->trackerfx), w->visualfx), MAX(tfxToVfx(w->trackerfx), w->visualfx), MIN(w->trackerfy, w->visualfy), MAX(w->trackerfy, w->visualfy), MIN(w->channel, w->visualchannel), MAX(w->channel, w->visualchannel), 0, 1); break;
						case T_MODE_VISUALLINE: addPartPattern( MAX(1, w->count), TRACKERFX_VISUAL_MIN, 2+cd->macroc, MIN(w->trackerfy, w->visualfy), MAX(w->trackerfy, w->visualfy), MIN(w->channel, w->visualchannel), MAX(w->channel, w->visualchannel), 0, 0); break;
					} regenGlobalRowc(s); break;
				case 24: /* ^x */
					switch (w->mode)
					{
						case T_MODE_VISUAL:     addPartPattern(-MAX(1, w->count), MIN(tfxToVfx(w->trackerfx), w->visualfx), MAX(tfxToVfx(w->trackerfx), w->visualfx), MIN(w->trackerfy, w->visualfy), MAX(w->trackerfy, w->visualfy), MIN(w->channel, w->visualchannel), MAX(w->channel, w->visualchannel), 0, 1); break;
						case T_MODE_VISUALLINE: addPartPattern(-MAX(1, w->count), TRACKERFX_VISUAL_MIN, 2+cd->macroc, MIN(w->trackerfy, w->visualfy), MAX(w->trackerfy, w->visualfy), MIN(w->channel, w->visualchannel), MAX(w->channel, w->visualchannel), 0, 0); break;
					} regenGlobalRowc(s); break;
				case 'b': /* bounce visual line to sample */
					bouncePartPattern(MIN(w->trackerfy, w->visualfy), MAX(w->trackerfy, w->visualfy),
							MIN(w->channel, w->visualchannel), MAX(w->channel, w->visualchannel));
					w->trackerfx = 0;
					w->trackerfy = MIN(w->trackerfy, w->visualfy);
					w->channel = MIN(w->channel, w->visualchannel);
					w->mode = T_MODE_NORMAL;
					regenGlobalRowc(s); break;
				case 'a': /* rip visual line to new variant */
					vindex = 0;
					vminrow = MIN(w->trackerfy, w->visualfy);
					/* loop to find the lowest common free index */
					for (i = MIN(w->channel, w->visualchannel); i <= MAX(w->channel, w->visualchannel); i++)
						vindex = MAX(vindex, _getEmptyVariantIndex(&s->channel->v[i].data, vindex));

					/* loop to actually rip */
					for (i = MIN(w->channel, w->visualchannel); i <= MAX(w->channel, w->visualchannel); i++)
					{
						vlength = MIN(VARIANT_ROWMAX, MAX(w->trackerfy, w->visualfy) - vminrow);
						cd = &s->channel->v[i].data;

						addVariant(cd, vindex, vlength);

						/* move rows to the new variant */
						for (j = 0; j < vlength; j++)
						{
							r = getChannelRow(cd, vminrow + j);
							memcpy(&cd->variantv[cd->varianti[vindex]]->rowv[j], r, sizeof(Row));

							/* only clear the source row if it's in the global variant */
							if (getChannelVariant(NULL, cd, vminrow + j) == -1)
							{
								memset(r, 0, sizeof(Row));
								r->note = NOTE_VOID;
								r->inst = INST_VOID;
							}
						}

						/* unnecessarily complex edge case handling */
						if (cd->trig[vminrow + vlength].index == VARIANT_VOID)
							for (j = vminrow + vlength; j > vminrow; j--)
								if (cd->trig[j].index != VARIANT_VOID)
								{
									if (cd->varianti[cd->trig[j].index] != VARIANT_VOID && j + cd->variantv[cd->varianti[cd->trig[j].index]]->rowc > vminrow + vlength)
									{
										cd->trig[vminrow + vlength].index = cd->trig[j].index;
										// TODO: set vtrig offset to align this correctly
									} break;
								}

						setChannelTrig(cd, vminrow, vindex);
					} regenGlobalRowc(s); break;
				case 'd': case 'x': case 127: case '\b': /* pattern cut */
					switch (w->mode)
					{
						case T_MODE_VISUAL:
							yankPartPattern(MIN(tfxToVfx(w->trackerfx), w->visualfx), MAX(tfxToVfx(w->trackerfx), w->visualfx), MIN(w->trackerfy, w->visualfy), MAX(w->trackerfy, w->visualfy), MIN(w->channel, w->visualchannel), MAX(w->channel, w->visualchannel));
							delPartPattern (MIN(tfxToVfx(w->trackerfx), w->visualfx), MAX(tfxToVfx(w->trackerfx), w->visualfx), MIN(w->trackerfy, w->visualfy), MAX(w->trackerfy, w->visualfy), MIN(w->channel, w->visualchannel), MAX(w->channel, w->visualchannel));
							break;
						case T_MODE_VISUALLINE:
							yankPartPattern(TRACKERFX_VISUAL_MIN, 2+cd->macroc, MIN(w->trackerfy, w->visualfy), MAX(w->trackerfy, w->visualfy), MIN(w->channel, w->visualchannel), MAX(w->channel, w->visualchannel));
							delPartPattern (TRACKERFX_VISUAL_MIN, 2+cd->macroc, MIN(w->trackerfy, w->visualfy), MAX(w->trackerfy, w->visualfy), MIN(w->channel, w->visualchannel), MAX(w->channel, w->visualchannel));
							break;
					}
					w->trackerfx = MIN(w->trackerfx, vfxToTfx(w->visualfx));
					w->trackerfy = MIN(w->trackerfy, w->visualfy);
					w->channel = MIN(w->channel, w->visualchannel);
					w->mode = T_MODE_NORMAL;
					break;
				case 'y': /* pattern copy */
					switch (w->mode)
					{
						case T_MODE_VISUAL:     yankPartPattern(MIN(tfxToVfx(w->trackerfx), w->visualfx), MAX(tfxToVfx(w->trackerfx), w->visualfx), MIN(w->trackerfy, w->visualfy), MAX(w->trackerfy, w->visualfy), MIN(w->channel, w->visualchannel), MAX(w->channel, w->visualchannel)); break;
						case T_MODE_VISUALLINE: yankPartPattern(TRACKERFX_VISUAL_MIN, 2+cd->macroc, MIN(w->trackerfy, w->visualfy), MAX(w->trackerfy, w->visualfy), MIN(w->channel, w->visualchannel), MAX(w->channel, w->visualchannel)); break;
					}
					w->trackerfx = MIN(w->trackerfx, vfxToTfx(w->visualfx));
					w->trackerfy = MIN(w->trackerfy, w->visualfy);
					w->channel = MIN(w->channel, w->visualchannel);
					w->mode = T_MODE_NORMAL;
					p->redraw = 1; break;
				case '.': /* vtrig loop */
					switch (w->mode)
					{
						case T_MODE_VISUAL:     loopPartPattern(MIN(tfxToVfx(w->trackerfx), w->visualfx), MAX(tfxToVfx(w->trackerfx), w->visualfx), MIN(w->trackerfy, w->visualfy), MAX(w->trackerfy, w->visualfy), MIN(w->channel, w->visualchannel), MAX(w->channel, w->visualchannel)); break;
						case T_MODE_VISUALLINE: loopPartPattern(TRACKERFX_VISUAL_MIN, 2+cd->macroc, MIN(w->trackerfy, w->visualfy), MAX(w->trackerfy, w->visualfy), MIN(w->channel, w->visualchannel), MAX(w->channel, w->visualchannel)); break;
					}
					w->mode = T_MODE_NORMAL;
					p->redraw = 1; break;
				case ';': /* loop range */
					setLoopRange(MIN(w->trackerfy, w->visualfy), MAX(w->trackerfy, w->visualfy));
					regenGlobalRowc(s); break;
			} break;
	} return 0;
}
