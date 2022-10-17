int inputChannelVariant(int input)
{
	uint8_t vindex, vlength;
	int i, j;
	short macro, vminrow;
	Row *r;
	ChannelData *cd = &s->channel->v[w->channel].data;

	switch (w->mode)
	{
		case T_MODE_VISUALREPLACE:
			if (input == '\n' || input == '\r') { toggleChannelMute(w->channel); p->dirty = 1; }
			else
			{
				switch (w->trackerfx)
				{
					case 0: /* note */
						for (i = MIN(w->trackerfy, w->visualfy); i <= MAX(w->trackerfy, w->visualfy); i++)
							insertNote(getChannelRow(cd, i), input);
						/* <preview> */
						previewNote(input, getChannelRow(cd, w->trackerfy)->inst);
						/* </preview> */
						break;
					case 1: /* inst */
						for (i = MIN(w->trackerfy, w->visualfy); i <= MAX(w->trackerfy, w->visualfy); i++)
							insertInst(getChannelRow(cd, i), input);
						break;
					default: /* macro */
						macro = (w->trackerfx - 2)>>1;
						if (!(w->trackerfx%2))
						{ /* macroc */
							for (i = MIN(w->trackerfy, w->visualfy); i <= MAX(w->trackerfy, w->visualfy); i++)
								insertMacroc(getChannelRow(cd, i), macro, input);
						} else
						{ /* macrov */
							for (i = MIN(w->trackerfy, w->visualfy); i <= MAX(w->trackerfy, w->visualfy); i++)
								insertMacrov(getChannelRow(cd, i), macro, input);
						} break;
				} p->dirty = 1; break;
			}
		case T_MODE_VISUAL: case T_MODE_VISUALLINE:
			switch (input)
			{
				case '\n': case '\r': toggleChannelMute(w->channel); p->dirty = 1; break;
				case 'v': case 22: /* visual */
					switch (w->mode)
					{
						case T_MODE_VISUAL:     w->mode = T_MODE_NORMAL; p->dirty = 1; break;
						case T_MODE_VISUALLINE: w->mode = T_MODE_VISUAL; p->dirty = 1; break;
					} break;
				case 'V': /* visual line */
					switch (w->mode)
					{
						case T_MODE_VISUAL:     w->mode = T_MODE_VISUALLINE; p->dirty = 1; break;
						case T_MODE_VISUALLINE: w->mode = T_MODE_NORMAL;     p->dirty = 1; break;
					} break;
				case 'r': /* replace     */ w->mode = T_MODE_VISUALREPLACE; p->dirty = 1; break;;
				case 'k': /* up arrow    */ trackerUpArrow  (1); p->dirty = 1; break;
				case 'j': /* down arrow  */ trackerDownArrow(1); p->dirty = 1; break;
				case 'h': /* left arrow  */ trackerLeftArrow (); p->dirty = 1; break;
				case 'l': /* right arrow */ trackerRightArrow(); p->dirty = 1; break;
				case '[': /* chnl left   */ channelLeft      (); p->dirty = 1; break;
				case ']': /* chnl right  */ channelRight     (); p->dirty = 1; break;
				case '{': /* cycle up    */ cycleUp          (); p->dirty = 1; break;
				case '}': /* cycle down  */ cycleDown        (); p->dirty = 1; break;
				case '~': /* vi tilde */
					switch (w->mode)
					{
						case T_MODE_VISUAL:     tildePartPattern(MIN(tfxToVfx(w->trackerfx), w->visualfx), MAX(tfxToVfx(w->trackerfx), w->visualfx), MIN(w->trackerfy, w->visualfy), MAX(w->trackerfy, w->visualfy), MIN(w->channel, w->visualchannel), MAX(w->channel, w->visualchannel)); break;
						case T_MODE_VISUALLINE: tildePartPattern(0, 2+cd->macroc, MIN(w->trackerfy, w->visualfy), MAX(w->trackerfy, w->visualfy), MIN(w->channel, w->visualchannel), MAX(w->channel, w->visualchannel)); break;
					} p->dirty = 1; break;
				case 'i': /* interpolate */
					switch (w->mode)
					{
						case T_MODE_VISUAL:     interpolatePartPattern(MIN(tfxToVfx(w->trackerfx), w->visualfx), MAX(tfxToVfx(w->trackerfx), w->visualfx), MIN(w->trackerfy, w->visualfy), MAX(w->trackerfy, w->visualfy), MIN(w->channel, w->visualchannel), MAX(w->channel, w->visualchannel)); break;
						case T_MODE_VISUALLINE: interpolatePartPattern(0, 2+cd->macroc, MIN(w->trackerfy, w->visualfy), MAX(w->trackerfy, w->visualfy), MIN(w->channel, w->visualchannel), MAX(w->channel, w->visualchannel)); break;
					} regenGlobalRowc(s); p->dirty = 1; break;
				case '%': /* random */
					switch (w->mode)
					{
						case T_MODE_VISUAL:     randPartPattern(MIN(tfxToVfx(w->trackerfx), w->visualfx), MAX(tfxToVfx(w->trackerfx), w->visualfx), MIN(w->trackerfy, w->visualfy), MAX(w->trackerfy, w->visualfy), MIN(w->channel, w->visualchannel), MAX(w->channel, w->visualchannel)); break;
						case T_MODE_VISUALLINE: randPartPattern(0, 2+cd->macroc, MIN(w->trackerfy, w->visualfy), MAX(w->trackerfy, w->visualfy), MIN(w->channel, w->visualchannel), MAX(w->channel, w->visualchannel)); break;
					} regenGlobalRowc(s); p->dirty = 1; break;
				case 1: /* ^a */
					switch (w->mode)
					{
						case T_MODE_VISUAL:     addPartPattern(MAX(1, w->count), MIN(tfxToVfx(w->trackerfx), w->visualfx), MAX(tfxToVfx(w->trackerfx), w->visualfx), MIN(w->trackerfy, w->visualfy), MAX(w->trackerfy, w->visualfy), MIN(w->channel, w->visualchannel), MAX(w->channel, w->visualchannel)); break;
						case T_MODE_VISUALLINE: addPartPattern(MAX(1, w->count), 0, 2+cd->macroc, MIN(w->trackerfy, w->visualfy), MAX(w->trackerfy, w->visualfy), MIN(w->channel, w->visualchannel), MAX(w->channel, w->visualchannel)); break;
					} regenGlobalRowc(s); p->dirty = 1; break;
				case 24: /* ^x */
					switch (w->mode)
					{
						case T_MODE_VISUAL:     addPartPattern(-MAX(1, w->count), MIN(tfxToVfx(w->trackerfx), w->visualfx), MAX(tfxToVfx(w->trackerfx), w->visualfx), MIN(w->trackerfy, w->visualfy), MAX(w->trackerfy, w->visualfy), MIN(w->channel, w->visualchannel), MAX(w->channel, w->visualchannel)); break;
						case T_MODE_VISUALLINE: addPartPattern(-MAX(1, w->count), 0, 2+cd->macroc, MIN(w->trackerfy, w->visualfy), MAX(w->trackerfy, w->visualfy), MIN(w->channel, w->visualchannel), MAX(w->channel, w->visualchannel)); break;
					} regenGlobalRowc(s); p->dirty = 1; break;
				case 'b': /* bounce visual line to sample */
					bouncePartPattern(MIN(w->trackerfy, w->visualfy), MAX(w->trackerfy, w->visualfy),
							MIN(w->channel, w->visualchannel), MAX(w->channel, w->visualchannel));
					w->trackerfx = 0;
					w->trackerfy = MIN(w->trackerfy, w->visualfy);
					w->channel = MIN(w->channel, w->visualchannel);
					w->mode = T_MODE_NORMAL;
					regenGlobalRowc(s); p->dirty = 1; break;
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

						/* avoid overwriting a variant, causes a memory corruption */
						// setChannelTrig(cd, vminrow, VARIANT_VOID);

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
					} p->dirty = 1; break;
				case 'd': case 'x': case 127: case '\b': /* pattern cut */
					switch (w->mode)
					{
						case T_MODE_VISUAL:
							yankPartPattern(MIN(tfxToVfx(w->trackerfx), w->visualfx), MAX(tfxToVfx(w->trackerfx), w->visualfx), MIN(w->trackerfy, w->visualfy), MAX(w->trackerfy, w->visualfy), MIN(w->channel, w->visualchannel), MAX(w->channel, w->visualchannel));
							delPartPattern (MIN(tfxToVfx(w->trackerfx), w->visualfx), MAX(tfxToVfx(w->trackerfx), w->visualfx), MIN(w->trackerfy, w->visualfy), MAX(w->trackerfy, w->visualfy), MIN(w->channel, w->visualchannel), MAX(w->channel, w->visualchannel));
							break;
						case T_MODE_VISUALLINE:
							yankPartPattern(0, 2+cd->macroc, MIN(w->trackerfy, w->visualfy), MAX(w->trackerfy, w->visualfy), MIN(w->channel, w->visualchannel), MAX(w->channel, w->visualchannel));
							delPartPattern (0, 2+cd->macroc, MIN(w->trackerfy, w->visualfy), MAX(w->trackerfy, w->visualfy), MIN(w->channel, w->visualchannel), MAX(w->channel, w->visualchannel));
							break;
					}
					w->trackerfx = MIN(w->trackerfx, vfxToTfx(w->visualfx));
					w->trackerfy = MIN(w->trackerfy, w->visualfy);
					w->channel = MIN(w->channel, w->visualchannel);
					w->mode = T_MODE_NORMAL;
					regenGlobalRowc(s); p->dirty = 1; break;
				case 'y': /* pattern copy */
					switch (w->mode)
					{
						case T_MODE_VISUAL:     yankPartPattern(MIN(tfxToVfx(w->trackerfx), w->visualfx), MAX(tfxToVfx(w->trackerfx), w->visualfx), MIN(w->trackerfy, w->visualfy), MAX(w->trackerfy, w->visualfy), MIN(w->channel, w->visualchannel), MAX(w->channel, w->visualchannel)); break;
						case T_MODE_VISUALLINE: yankPartPattern(0, 2+cd->macroc, MIN(w->trackerfy, w->visualfy), MAX(w->trackerfy, w->visualfy), MIN(w->channel, w->visualchannel), MAX(w->channel, w->visualchannel)); break;
					}
					w->trackerfx = MIN(w->trackerfx, vfxToTfx(w->visualfx));
					w->trackerfy = MIN(w->trackerfy, w->visualfy);
					w->channel = MIN(w->channel, w->visualchannel);
					w->mode = T_MODE_NORMAL;
					p->dirty = 1; break;
				case ';': /* loop */
					setLoopRange(MIN(w->trackerfy, w->visualfy), MAX(w->trackerfy, w->visualfy));
					regenGlobalRowc(s); p->dirty = 1; break;
			} break;
		case T_MODE_VTRIG:
			switch (input)
			{ /* set count first */
				case '0': w->count *= 10; w->count += 0; p->dirty = 1; return 1;
				case '1': w->count *= 10; w->count += 1; p->dirty = 1; return 1;
				case '2': w->count *= 10; w->count += 2; p->dirty = 1; return 1;
				case '3': w->count *= 10; w->count += 3; p->dirty = 1; return 1;
				case '4': w->count *= 10; w->count += 4; p->dirty = 1; return 1;
				case '5': w->count *= 10; w->count += 5; p->dirty = 1; return 1;
				case '6': w->count *= 10; w->count += 6; p->dirty = 1; return 1;
				case '7': w->count *= 10; w->count += 7; p->dirty = 1; return 1;
				case '8': w->count *= 10; w->count += 8; p->dirty = 1; return 1;
				case '9': w->count *= 10; w->count += 9; p->dirty = 1; return 1;
				default:
					if (w->chord)
					{
						w->count = MIN(256, w->count);
						switch (w->chord)
						{
							case 'I': /* macro insert */ changeMacro(input, &w->keyboardmacro); w->mode = T_MODE_INSERT; break;
							case 'd': if (input == 'd') /* delete */
								{
									yankPartVtrig(w->trackerfy, w->trackerfy+MAX(1, w->count)-1, w->channel, w->channel);
									delPartVtrig (w->trackerfy, w->trackerfy+MAX(1, w->count)-1, w->channel, w->channel);
									trackerDownArrow(MAX(1, w->count));
									regenGlobalRowc(s); p->dirty = 1;
								} break;
							case 'y': if (input == 'y') /* yank */
								{
									yankPartVtrig(w->trackerfy, w->trackerfy+MAX(1, w->count)-1, w->channel, w->channel);
									trackerDownArrow(MAX(1, w->count));
									p->dirty = 1;
								} break;
							case 'g': /* graphic */ if (input == 'g') w->trackerfy = 0; p->dirty = 1; break;
							case 'r': /* row     */ inputTooltip(&tt, input); p->dirty = 1; break;
							case ';': /* loop    */ inputTooltip(&tt, input); p->dirty = 1; break;
							case 'c': /* channel */ inputTooltip(&tt, input); resize(0);    break;
						} w->count = 0; p->dirty = 1;
					} else
						switch (input)
						{
							case '\n': case '\r': toggleChannelMute(w->channel); p->dirty = 1; break;
							case 's': toggleChannelSolo(w->channel); p->dirty = 1; break;
							case '\t': /* leave vtrig mode   */ w->mode = T_MODE_NORMAL; p->dirty = 1; break;
							case 'f':  /* toggle song follow */ w->follow = !w->follow; if (s->playing) w->trackerfy = s->playfy; p->dirty = 1; break;
							case 'k':  /* up arrow           */ trackerUpArrow  (1); p->dirty = 1; break;
							case 'j':  /* down arrow         */ trackerDownArrow(1); p->dirty = 1; break;
							case 'h':  /* left arrow         */ trackerLeftArrow (); p->dirty = 1; break;
							case 'l':  /* right arrow        */ trackerRightArrow(); p->dirty = 1; break;
							case '[':  /* chnl left          */ channelLeft (); p->dirty = 1; break;
							case ']':  /* chnl right         */ channelRight(); p->dirty = 1; break;
							case '{':  /* cycle up           */ cycleUp  (); p->dirty = 1; break;
							case '}':  /* cycle down         */ cycleDown(); p->dirty = 1; break;
							case 'b':  /* bpm                */ if (w->count) { s->songbpm = MIN(255, MAX(32, w->count)); reapplyBpm(); } p->dirty = 1; break;
							case 't':  /* row highlight      */ if (w->count) { s->rowhighlight = MIN(16, w->count); regenGlobalRowc(s); } p->dirty = 1; break;
							case 'o':  /* octave             */ w->octave = MIN(9, w->count); p->dirty = 1; break;
							case 'r':  /* row                */ setChordRow (); p->dirty = 1; return 1;
							case ';':  /* loop               */ setChordLoop(); p->dirty = 1; return 1;
							case 'g':  /* graphic misc       */ w->chord = 'g'; p->dirty = 1; return 1;
							case 'G':  /* graphic end        */ trackerEnd(); p->dirty = 1; break;
							case 'y':  /* copy               */ w->chord = 'y'; p->dirty = 1; return 1;
							case 'd':  /* cut                */ w->chord = 'd'; p->dirty = 1; return 1;
							case 'I':          /* macro insert mode */ w->chord = 'I'; p->dirty = 1; return 1;
							case 'i':          /* enter insert mode */ w->mode = T_MODE_VTRIG_INSERT; p->dirty = 1; break;
							case 'v': case 22: /* enter visual mode */
								w->visualfy = w->trackerfy;
								w->visualchannel = w->channel;
								w->mode = T_MODE_VTRIG_VISUAL;
								p->dirty = 1; break;
							case 1:  /* ^a */         setChannelTrig(cd, w->trackerfy, cd->trig[w->trackerfy].index+MAX(1, w->count));                 p->dirty = 1; break;
							case 24: /* ^x */         setChannelTrig(cd, w->trackerfy, cd->trig[w->trackerfy].index-MAX(1, w->count));                 p->dirty = 1; break;
							case 'a': /* add empty */ setChannelTrig(cd, w->trackerfy, _duplicateEmptyVariantIndex(cd, cd->trig[w->trackerfy].index)); p->dirty = 1; break;
							case 'p': /* pattern put */ /* TODO: count */
								putPartVtrig();
								if (w->vbchannelc)
									trackerDownArrow(w->vbrowc);
								p->dirty = 1; break;
							case 'P': /* vtrig put before */ mixPutPartVtrig(); p->dirty = 1; break;
							case 'x': case 127: case '\b': /* vtrig delete */
								yankPartVtrig (w->trackerfy, w->trackerfy, w->channel, w->channel);
								setChannelTrig(cd, w->trackerfy, VARIANT_VOID);
								p->dirty = 1; break;
							case '.': /* vtrig loop */
								i = getPrevVtrig(cd, w->trackerfy);
								if (i != -1)
								{
									cd->trig[i].flags ^= C_VTRIG_LOOP;
									regenGlobalRowc(s); p->dirty = 1;
								} break;
							case 'c': /* channel */ setChordChannel(); p->dirty = 1; return 1;
						} break;
			} break;
		case T_MODE_NORMAL:
			switch (input)
			{ /* set count first */
				case '0': w->count *= 10; w->count += 0; p->dirty = 1; return 1;
				case '1': w->count *= 10; w->count += 1; p->dirty = 1; return 1;
				case '2': w->count *= 10; w->count += 2; p->dirty = 1; return 1;
				case '3': w->count *= 10; w->count += 3; p->dirty = 1; return 1;
				case '4': w->count *= 10; w->count += 4; p->dirty = 1; return 1;
				case '5': w->count *= 10; w->count += 5; p->dirty = 1; return 1;
				case '6': w->count *= 10; w->count += 6; p->dirty = 1; return 1;
				case '7': w->count *= 10; w->count += 7; p->dirty = 1; return 1;
				case '8': w->count *= 10; w->count += 8; p->dirty = 1; return 1;
				case '9': w->count *= 10; w->count += 9; p->dirty = 1; return 1;
				default:
					if (w->chord)
					{
						w->count = MIN(256, w->count);
						switch (w->chord)
						{
							case 'd': if (input == 'd') /* delete */
								{
									yankPartPattern(0, 2+cd->macroc, w->trackerfy, w->trackerfy+MAX(1, w->count)-1, w->channel, w->channel);
									delPartPattern (0, 2+cd->macroc, w->trackerfy, w->trackerfy+MAX(1, w->count)-1, w->channel, w->channel);
									trackerDownArrow(MAX(1, w->count));
									p->dirty = 1;
								} break;
							case 'y': if (input == 'y') /* yank */
								{
									yankPartPattern(0, 2+cd->macroc, w->trackerfy, w->trackerfy+MAX(1, w->count)-1, w->channel, w->channel);
									trackerDownArrow(MAX(1, w->count));
									p->dirty = 1;
								} break;
							case 'c': /* channel      */ inputTooltip(&tt, input); resize(0); break;
							case 'm': /* macro        */ inputTooltip(&tt, input); resize(0); break;
							case 'I': /* macro insert */ changeMacro(input, &w->keyboardmacro); w->mode = T_MODE_INSERT; p->dirty = 1; break;
							case 'g': /* graphic      */ if (input == 'g') w->trackerfy = 0; p->dirty = 1; break;
							case 'r': /* row          */ inputTooltip(&tt, input); p->dirty = 1; break;
							case ';': /* loop         */ inputTooltip(&tt, input); p->dirty = 1; break;
						} w->count = 0;
					} else
					{
						switch (input)
						{
							case '\n': case '\r': toggleChannelMute(w->channel); p->dirty = 1; break;
							case 's': toggleChannelSolo(w->channel); p->dirty = 1; break;
							case '\t':/* enter vtrig mode   */ w->mode = T_MODE_VTRIG; p->dirty = 1; break;
							case 'f': /* toggle song follow */ w->follow = !w->follow; if (s->playing) w->trackerfy = s->playfy; p->dirty = 1; break;
							case 'I': /* macro insert mode  */ w->chord = 'I'; p->dirty = 1; return 1;
							case 'i': /* enter insert mode  */ w->mode = T_MODE_INSERT; p->dirty = 1; break;
							// case 'u': /* undo               */ popPatternHistory(s->patterni[s->trig[w->songfy].index]); p->dirty = 1; break;
							// case 18:  /* redo               */ unpopPatternHistory(s->patterni[s->trig[w->songfy].index]); p->dirty = 1; break;
							case 'k': /* up arrow           */ trackerUpArrow  (1); p->dirty = 1; break;
							case 'j': /* down arrow         */ trackerDownArrow(1); p->dirty = 1; break;
							case 'h': /* left arrow         */ trackerLeftArrow (); p->dirty = 1; break;
							case 'l': /* right arrow        */ trackerRightArrow(); p->dirty = 1; break;
							case '[': /* chnl left          */ channelLeft (); p->dirty = 1; break;
							case ']': /* chnl right         */ channelRight(); p->dirty = 1; break;
							case '{': /* cycle up           */ cycleUp  (); p->dirty = 1; break;
							case '}': /* cycle down         */ cycleDown(); p->dirty = 1; break;
							case 'v': case 22: /* enter visual mode */
								w->visualfx = tfxToVfx(w->trackerfx);
								w->visualfy = w->trackerfy;
								w->visualchannel = w->channel;
								w->mode = T_MODE_VISUAL;
								p->dirty = 1; break;
							case 'V': /* enter visual line mode */
								w->visualfx = tfxToVfx(w->trackerfx);
								w->visualfy = w->trackerfy;
								w->visualchannel = w->channel;
								w->mode = T_MODE_VISUALLINE;
								p->dirty = 1; break;
							case 'y':  /* pattern copy     */ w->chord = 'y'; p->dirty = 1; return 1;
							case 'd':  /* pattern cut      */ w->chord = 'd'; p->dirty = 1; return 1;
							case 'c':  /* channel          */ setChordChannel(); p->dirty = 1; return 1;
							case 'm':  /* macro            */ setChordMacro  (); p->dirty = 1; return 1;
							case 'r':  /* row              */ setChordRow    (); p->dirty = 1; return 1;
							case ';':  /* loop             */ setChordLoop   (); p->dirty = 1; return 1;
							case 'g':  /* graphic misc     */ w->chord = 'g'; p->dirty = 1; return 1;
							case 'G':  /* graphic end      */ trackerEnd(); p->dirty = 1; break;
							case 'b':  /* bpm              */ if (w->count) { s->songbpm = MIN(255, MAX(32, w->count)); reapplyBpm(); } p->dirty = 1; break;
							case 't':  /* row highlight    */ if (w->count) { s->rowhighlight = MIN(16, w->count); regenGlobalRowc(s); } p->dirty = 1; break;
							case 'o':  /* octave           */
								r = getChannelRow(cd, w->trackerfy);
								if (!w->trackerfx) r->note = changeNoteOctave(MIN(9, w->count), r->note);
								else               w->octave = MIN(9, w->count);
								p->dirty = 1; break;
							case 'p':  /* pattern put */ /* TODO: count */
								putPartPattern();
								if (w->pbchannelc)
									trackerDownArrow(w->pbvariantv[0]->rowc);
								p->dirty = 1; break;
							case 'P': /* pattern put before */
								mixPutPartPattern();
								p->dirty = 1; break;
							case 'x': case 127: case '\b': /* backspace */
								if (w->trackerfx == 0)
								{
									yankPartPattern(0, 1, w->trackerfy, w->trackerfy, w->channel, w->channel);
									delPartPattern (0, 1, w->trackerfy, w->trackerfy, w->channel, w->channel);
								} else
								{
									yankPartPattern(tfxToVfx(w->trackerfx), tfxToVfx(w->trackerfx), w->trackerfy, w->trackerfy, w->channel, w->channel);
									delPartPattern (tfxToVfx(w->trackerfx), tfxToVfx(w->trackerfx), w->trackerfy, w->trackerfy, w->channel, w->channel);
								} p->dirty = 1; break;
							case '%': /* random */
								randPartPattern(tfxToVfx(w->trackerfx), tfxToVfx(w->trackerfx), w->trackerfy, w->trackerfy, w->channel, w->channel);
								p->dirty = 1; break;
							default: /* column specific */
								r = getChannelRow(cd, w->trackerfy);
								switch (w->trackerfx)
								{
									case 0: /* note */
										switch (input)
										{
											case 1:  /* ^a */ r->note+=MAX(1, w->count); break;
											case 24: /* ^x */ r->note-=MAX(1, w->count); break;
										} break;
									case 1: /* instrument */
										switch (input)
										{
											case 1: /* ^a */
												r->inst+=MAX(1, w->count);
												if (w->instrumentrecv == INST_REC_LOCK_OK) w->instrument = r->inst;
												break;
											case 24: /* ^x */
												r->inst-=MAX(1, w->count);
												if (w->instrumentrecv == INST_REC_LOCK_OK) w->instrument = r->inst;
												break;
										} break;
									default:
										macro = (w->trackerfx - 2)>>1;
										switch (input)
										{
											case 1:  /* ^a */
												switch (r->macro[macro].c)
												{
													case 'G': case 'g': case 'K': case 'k': r->macro[macro].v += MAX(1, w->count)*16;
													default:                                r->macro[macro].v += MAX(1, w->count);
												} break;
											case 24: /* ^x */
												switch (r->macro[macro].c)
												{
													case 'G': case 'g': case 'K': case 'k': r->macro[macro].v -= MAX(1, w->count)*16;
													default:                                r->macro[macro].v -= MAX(1, w->count);
												} break;
											case '~': /* toggle case */
												if      (isupper(r->macro[macro].c)) changeMacro(r->macro[macro].c, &r->macro[macro].c);
												else if (islower(r->macro[macro].c)) changeMacro(r->macro[macro].c, &r->macro[macro].c);
												break;
										} break;
								} regenGlobalRowc(s); p->dirty = 1; break;
						}
					} break;
			} break;
		case T_MODE_VTRIG_VISUAL:
			switch (input)
			{
				case 'v': case 22: w->mode = T_MODE_VTRIG; p->dirty = 1; break;
				case 'k': /* up arrow    */ trackerUpArrow  (1); p->dirty = 1; break;
				case 'j': /* down arrow  */ trackerDownArrow(1); p->dirty = 1; break;
				case 'h': /* left arrow  */ trackerLeftArrow (); p->dirty = 1; break;
				case 'l': /* right arrow */ trackerRightArrow(); p->dirty = 1; break;
				case '[': /* chnl left   */ channelLeft (); p->dirty = 1; break;
				case ']': /* chnl right  */ channelRight(); p->dirty = 1; break;
				case '{': /* cycle up    */ cycleUp  (); p->dirty = 1; break;
				case '}': /* cycle down  */ cycleDown(); p->dirty = 1; break;
				case 'd': case 'x': case 127: case '\b': /* cut */
					yankPartVtrig(MIN(w->trackerfy, w->visualfy), MAX(w->trackerfy, w->visualfy), MIN(w->channel, w->visualchannel), MAX(w->channel, w->visualchannel));
					delPartVtrig (MIN(w->trackerfy, w->visualfy), MAX(w->trackerfy, w->visualfy), MIN(w->channel, w->visualchannel), MAX(w->channel, w->visualchannel));

					w->trackerfy = MIN(w->trackerfy, w->visualfy);
					w->channel = MIN(w->channel, w->visualchannel);
					w->mode = T_MODE_VTRIG;
					p->dirty = 1; break;
				case 'y': /* pattern copy */
					yankPartVtrig(MIN(w->trackerfy, w->visualfy), MAX(w->trackerfy, w->visualfy), MIN(w->channel, w->visualchannel), MAX(w->channel, w->visualchannel));

					w->trackerfy = MIN(w->trackerfy, w->visualfy);
					w->channel = MIN(w->channel, w->visualchannel);
					w->mode = T_MODE_VTRIG;
					p->dirty = 1; break;
				case '.': /* vtrig loop */
					loopPartVtrig(MIN(w->trackerfy, w->visualfy), MAX(w->trackerfy, w->visualfy), MIN(w->channel, w->visualchannel), MAX(w->channel, w->visualchannel));
					w->mode = T_MODE_VTRIG;
					p->dirty = 1; break;
			} break;
		case T_MODE_VTRIG_INSERT:
			switch (input)
			{
				case '0':           inputChannelTrig(cd, w->trackerfy, 0);  p->dirty = 1; break;
				case '1':           inputChannelTrig(cd, w->trackerfy, 1);  p->dirty = 1; break;
				case '2':           inputChannelTrig(cd, w->trackerfy, 2);  p->dirty = 1; break;
				case '3':           inputChannelTrig(cd, w->trackerfy, 3);  p->dirty = 1; break;
				case '4':           inputChannelTrig(cd, w->trackerfy, 4);  p->dirty = 1; break;
				case '5':           inputChannelTrig(cd, w->trackerfy, 5);  p->dirty = 1; break;
				case '6':           inputChannelTrig(cd, w->trackerfy, 6);  p->dirty = 1; break;
				case '7':           inputChannelTrig(cd, w->trackerfy, 7);  p->dirty = 1; break;
				case '8':           inputChannelTrig(cd, w->trackerfy, 8);  p->dirty = 1; break;
				case '9':           inputChannelTrig(cd, w->trackerfy, 9);  p->dirty = 1; break;
				case 'A': case 'a': inputChannelTrig(cd, w->trackerfy, 10); p->dirty = 1; break;
				case 'B': case 'b': inputChannelTrig(cd, w->trackerfy, 11); p->dirty = 1; break;
				case 'C': case 'c': inputChannelTrig(cd, w->trackerfy, 12); p->dirty = 1; break;
				case 'D': case 'd': inputChannelTrig(cd, w->trackerfy, 13); p->dirty = 1; break;
				case 'E': case 'e': inputChannelTrig(cd, w->trackerfy, 14); p->dirty = 1; break;
				case 'F': case 'f': inputChannelTrig(cd, w->trackerfy, 15); p->dirty = 1; break;
				case ' ': setChannelTrig(cd, w->trackerfy, VARIANT_OFF); p->dirty = 1; break;
				case 127: case '\b': /* backspace */ setChannelTrig(cd, w->trackerfy, VARIANT_VOID);                                    p->dirty = 1; break;
				case 1:  /* ^a */                    setChannelTrig(cd, w->trackerfy, cd->trig[w->trackerfy].index+MAX(1, w->count)-1); p->dirty = 1; break;
				case 24: /* ^x */                    setChannelTrig(cd, w->trackerfy, cd->trig[w->trackerfy].index-MAX(1, w->count)+1); p->dirty = 1; break;
			} break;
		case T_MODE_INSERT:
			r = getChannelRow(cd, w->trackerfy);
			if (input == '\n' || input == '\r') { toggleChannelMute(w->channel); p->dirty = 1; }
			else
				switch (w->trackerfx)
				{
					case 0: /* note */
						insertNote(r, input); /* invalidates r, TODO: previewNote should be event aware, there's probably a race condition currently */
						previewNote(input, getChannelRow(cd, w->trackerfy)->inst);
						trackerDownArrow(w->step);
						break;
					case 1: /* instrument */
						insertInst(r, input);
						break;
					default: /* macros */
						macro = (w->trackerfx - 2)>>1;
						switch (input)
						{
							case '~': /* toggle case */
								if      (isupper(r->macro[macro].c)) r->macro[macro].c += 32;
								else if (islower(r->macro[macro].c)) r->macro[macro].c -= 32;
								regenGlobalRowc(s); break;
							default:
								if (!(w->trackerfx%2)) insertMacroc(r, macro, input);
								else                   insertMacrov(r, macro, input);
								break;
						} break;
				} p->dirty = 1; break;
	}
	return 0;
}
