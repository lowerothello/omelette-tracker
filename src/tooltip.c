void clearTooltip(void)
{
	if (tt.entryv)
	{
		for (size_t i = 0; i < tt.entryc; i++)
		{
			if (tt.entryv[i].prettyname)   free(tt.entryv[i].prettyname  );
			if (tt.entryv[i].prettykeysym) free(tt.entryv[i].prettykeysym);
		}

		free(tt.entryv);
		tt.entryv = NULL;
	}

	if (tt.prettytitle)
	{
		free(tt.prettytitle);
		tt.prettytitle = NULL;
	}

	tt.mousecallback = NULL;
	tt.entryc = 0;
	tt.maxprettynamelen = 0;
}

void setTooltipTitle(char *prettytitle)
{
	if (tt.prettytitle) free(tt.prettytitle);
	tt.prettytitle = strdup(prettytitle);
}

void setTooltipMouseCallback(void (*callback)(enum Button, int, int))
{
	tt.mousecallback = callback;
}

/* strlen(prettykeysym) should be <= MOD_WIDTH */
void addTooltipPrettyPrint(const char *prettyname, unsigned int state, const char *prettykeysym)
{
	TooltipEntry *newentryv = realloc(tt.entryv, (tt.entryc+1) * sizeof(TooltipEntry));
	memset(&newentryv[tt.entryc], 0, sizeof(TooltipEntry));

	newentryv[tt.entryc].prettyname = strdup(prettyname);
	tt.maxprettynamelen = MAX(tt.maxprettynamelen, strlen(prettyname));
	newentryv[tt.entryc].prettykeysym = strdup(prettykeysym);

	newentryv[tt.entryc].state = state;
	newentryv[tt.entryc].flags = TT_DRAW;

	tt.entryv = newentryv;
	tt.entryc++;
}
void addTooltipBind(const char *prettyname, unsigned int state, KeySym keysym, uint8_t flags, void (*callback)(void *), void *arg)
{
	TooltipEntry *newentryv = realloc(tt.entryv, (tt.entryc+1) * sizeof(TooltipEntry));
	memset(&newentryv[tt.entryc], 0, sizeof(TooltipEntry));

	if (prettyname)
	{
		newentryv[tt.entryc].prettyname = strdup(prettyname);
		tt.maxprettynamelen = MAX(tt.maxprettynamelen, strlen(prettyname));
	}

	newentryv[tt.entryc].state = state;
	newentryv[tt.entryc].flags = flags;
	newentryv[tt.entryc].keysym = keysym;
	newentryv[tt.entryc].callback = callback;
	newentryv[tt.entryc].arg = arg;

	tt.entryv = newentryv;
	tt.entryc++;
}

/* procs the first relevant bind */
void inputTooltip(unsigned int state, KeySym input, bool release)
{
	/* don't try to process lower-case control keys */
	if ((state&ControlMask) && isascii(input))
		input = toupper(input);

	bool dead;
	for (size_t i = 0; i < tt.entryc; i++)
		if (tt.entryv[i].keysym == input
				&& tt.entryv[i].state == state
				&& (bool)(tt.entryv[i].flags&TT_RELEASE) == release)
		{
			dead = tt.entryv[i].flags&TT_DEAD; /* save flags in case the callback fucks with tt */
			if (tt.entryv[i].callback)
				tt.entryv[i].callback(tt.entryv[i].arg);
			p->redraw = 1;

			/* if the bind is dead then don't reset the tooltip based on the current mode/page */
			if (!dead)
				resetInput();
			break;
		}
}

static void drawTooltipLine(short x, short y, short maxprettynamelen, const char *prettyname, unsigned int state, const char *keynameupper, const char *keynamelower)
{
	char modbuffer[4] = {'\0'};
	if (state&Mod1Mask)    strcat(modbuffer, "^[");
	if (state&ControlMask) strcat(modbuffer, "^" );
	if (state&(ShiftMask^LockMask))
		printf("\033[%d;%dH│ %s\033[%ldX\033[%ldC\033[2m%s%s\033[m │", y, x, prettyname,
				(maxprettynamelen - strlen(prettyname)) + (MOD_WIDTH-1 - strlen(keynameupper) - strlen(modbuffer)),
				(maxprettynamelen - strlen(prettyname)) + (MOD_WIDTH-1 - strlen(keynameupper) - strlen(modbuffer)),
				modbuffer, keynameupper);
	else
		printf("\033[%d;%dH│ %s\033[%ldX\033[%ldC\033[2m%s%s\033[m │", y, x, prettyname,
				(maxprettynamelen - strlen(prettyname)) + (MOD_WIDTH-1 - strlen(keynamelower) - strlen(modbuffer)),
				(maxprettynamelen - strlen(prettyname)) + (MOD_WIDTH-1 - strlen(keynamelower) - strlen(modbuffer)),
				modbuffer, keynamelower);
}
void drawTooltip(void)
{
	if (!tt.entryc) return;
	printf("\033[s"); /* save the cursor pos */

	short drawc = 0;
	for (size_t i = 0; i < tt.entryc; i++)
		if (tt.entryv[i].prettyname && (tt.entryv[i].flags&TT_DRAW)) drawc++;

	short w = tt.maxprettynamelen + 2 + MOD_WIDTH;
	short h = drawc + 2;
	short x = ws.ws_col - w - 1;
	short y = ws.ws_row - h;

	char charbufferupper[2] = {'\0'};
	char charbufferlower[2] = {'\0'};

	short yo = 0;
	for (size_t i = 0; i < tt.entryc; i++)
	{
		if (tt.entryv[i].prettyname && (tt.entryv[i].flags&TT_DRAW))
		{
			if (tt.entryv[i].prettykeysym)
				drawTooltipLine(x, y+1+yo, tt.maxprettynamelen, tt.entryv[i].prettyname, tt.entryv[i].state, tt.entryv[i].prettykeysym, tt.entryv[i].prettykeysym);
			else
				switch (tt.entryv[i].keysym)
				{
					case XK_Up:        drawTooltipLine(x, y+1+yo, tt.maxprettynamelen, tt.entryv[i].prettyname, tt.entryv[i].state, "UP",    "up"   ); break;
					case XK_Down:      drawTooltipLine(x, y+1+yo, tt.maxprettynamelen, tt.entryv[i].prettyname, tt.entryv[i].state, "DOWN",  "down" ); break;
					case XK_Left:      drawTooltipLine(x, y+1+yo, tt.maxprettynamelen, tt.entryv[i].prettyname, tt.entryv[i].state, "LEFT",  "left" ); break;
					case XK_Right:     drawTooltipLine(x, y+1+yo, tt.maxprettynamelen, tt.entryv[i].prettyname, tt.entryv[i].state, "RIGHT", "right"); break;
					case XK_Home:      drawTooltipLine(x, y+1+yo, tt.maxprettynamelen, tt.entryv[i].prettyname, tt.entryv[i].state, "HOME",  "home" ); break;
					case XK_End:       drawTooltipLine(x, y+1+yo, tt.maxprettynamelen, tt.entryv[i].prettyname, tt.entryv[i].state, "END",   "end"  ); break;
					case XK_Page_Up:   drawTooltipLine(x, y+1+yo, tt.maxprettynamelen, tt.entryv[i].prettyname, tt.entryv[i].state, "PGUP",  "pgup" ); break;
					case XK_Page_Down: drawTooltipLine(x, y+1+yo, tt.maxprettynamelen, tt.entryv[i].prettyname, tt.entryv[i].state, "PGDN",  "pgdn" ); break;
					case XK_F1:        drawTooltipLine(x, y+1+yo, tt.maxprettynamelen, tt.entryv[i].prettyname, tt.entryv[i].state, "F1",    "f1"   ); break;
					case XK_F2:        drawTooltipLine(x, y+1+yo, tt.maxprettynamelen, tt.entryv[i].prettyname, tt.entryv[i].state, "F2",    "f2"   ); break;
					case XK_F3:        drawTooltipLine(x, y+1+yo, tt.maxprettynamelen, tt.entryv[i].prettyname, tt.entryv[i].state, "F3",    "f3"   ); break;
					case XK_F4:        drawTooltipLine(x, y+1+yo, tt.maxprettynamelen, tt.entryv[i].prettyname, tt.entryv[i].state, "F4",    "f4"   ); break;
					case XK_F5:        drawTooltipLine(x, y+1+yo, tt.maxprettynamelen, tt.entryv[i].prettyname, tt.entryv[i].state, "F5",    "f5"   ); break;
					case XK_F6:        drawTooltipLine(x, y+1+yo, tt.maxprettynamelen, tt.entryv[i].prettyname, tt.entryv[i].state, "F6",    "f6"   ); break;
					case XK_F7:        drawTooltipLine(x, y+1+yo, tt.maxprettynamelen, tt.entryv[i].prettyname, tt.entryv[i].state, "F7",    "f7"   ); break;
					case XK_F8:        drawTooltipLine(x, y+1+yo, tt.maxprettynamelen, tt.entryv[i].prettyname, tt.entryv[i].state, "F8",    "f8"   ); break;
					case XK_F9:        drawTooltipLine(x, y+1+yo, tt.maxprettynamelen, tt.entryv[i].prettyname, tt.entryv[i].state, "F9",    "f9"   ); break;
					case XK_F10:       drawTooltipLine(x, y+1+yo, tt.maxprettynamelen, tt.entryv[i].prettyname, tt.entryv[i].state, "F10",   "f10"  ); break;
					case XK_F11:       drawTooltipLine(x, y+1+yo, tt.maxprettynamelen, tt.entryv[i].prettyname, tt.entryv[i].state, "F11",   "f11"  ); break;
					case XK_F12:       drawTooltipLine(x, y+1+yo, tt.maxprettynamelen, tt.entryv[i].prettyname, tt.entryv[i].state, "F12",   "f12"  ); break;
					case XK_BackSpace: drawTooltipLine(x, y+1+yo, tt.maxprettynamelen, tt.entryv[i].prettyname, tt.entryv[i].state, "\\B",   "\\b"  ); break;
					case XK_Return:    drawTooltipLine(x, y+1+yo, tt.maxprettynamelen, tt.entryv[i].prettyname, tt.entryv[i].state, "\\R",   "\\r"  ); break;
					case XK_Tab:       drawTooltipLine(x, y+1+yo, tt.maxprettynamelen, tt.entryv[i].prettyname, tt.entryv[i].state, "\\T",   "\\t"  ); break;
					case XK_Escape:    drawTooltipLine(x, y+1+yo, tt.maxprettynamelen, tt.entryv[i].prettyname, tt.entryv[i].state, "\\E",   "\\e"  ); break;
					default:
						if (isascii(tt.entryv[i].keysym))
						{
							charbufferupper[0] = toupper(tt.entryv[i].keysym);
							charbufferlower[0] = tolower(tt.entryv[i].keysym);
							drawTooltipLine(x, y+1+yo, tt.maxprettynamelen, tt.entryv[i].prettyname, tt.entryv[i].state, charbufferupper, charbufferlower);
						} break;
				}
			yo++;
		}
	}

	if (tt.prettytitle)
	{
		printf("\033[%d;%dH┌\033[%d;%dH┐", y-1, x, y-1, x+w);
		printf("\033[%d;%dH│\033[%d;%dH│", y,   x, y,   x+w);
		for (short i = 1; i < w; i++) printf("\033[%d;%dH ", y,   x+i);
		for (short i = 1; i < w; i++) printf("\033[%d;%dH─", y-1, x+i);
		printf("\033[%d;%dH\033[1m%s\033[m", y, x + ((w - ((short)strlen(tt.prettytitle)-1))>>1), tt.prettytitle);
	} else
	{
		printf("\033[%d;%dH┌\033[%d;%dH┐", y, x, y, x+w);
		for (short i = 1; i < w; i++) printf("\033[%d;%dH─", y, x+i);
	}

	printf("\033[%d;%dH└\033[%d;%dH┘", y+h-1, x, y+h-1, x+w);
	for (short i = 1; i < w; i++) printf("\033[%d;%dH─", y+h-1, x+i);
	
	printf("\033[u"); /* restore the cursor pos */
}

/* the callback's arg is the note offset cast to (void*) */
void addNotePressBinds(const char *prettyname, unsigned int state, signed char octave, void (*callback)(void*))
{
	addTooltipPrettyPrint(prettyname, state, "press note keys");
	addTooltipBind("press C-+0", state, XK_q           , 0, callback, (void*)(size_t)(NOTE_MIN + NOTE_C_OFFSET + octave*12 + 0 )); addTooltipBind("press c-+0", state, XK_Q         , 0, callback, (void*)(size_t)(NOTE_MIN + NOTE_SMOOTH_OFFSET + NOTE_C_OFFSET + octave*12 + 0 ));
	addTooltipBind("press C#+0", state, XK_w           , 0, callback, (void*)(size_t)(NOTE_MIN + NOTE_C_OFFSET + octave*12 + 1 )); addTooltipBind("press c#+0", state, XK_W         , 0, callback, (void*)(size_t)(NOTE_MIN + NOTE_SMOOTH_OFFSET + NOTE_C_OFFSET + octave*12 + 1 ));
	addTooltipBind("press D-+0", state, XK_e           , 0, callback, (void*)(size_t)(NOTE_MIN + NOTE_C_OFFSET + octave*12 + 2 )); addTooltipBind("press d-+0", state, XK_E         , 0, callback, (void*)(size_t)(NOTE_MIN + NOTE_SMOOTH_OFFSET + NOTE_C_OFFSET + octave*12 + 2 ));
	addTooltipBind("press D#+0", state, XK_r           , 0, callback, (void*)(size_t)(NOTE_MIN + NOTE_C_OFFSET + octave*12 + 3 )); addTooltipBind("press d#+0", state, XK_R         , 0, callback, (void*)(size_t)(NOTE_MIN + NOTE_SMOOTH_OFFSET + NOTE_C_OFFSET + octave*12 + 3 ));
	addTooltipBind("press E-+0", state, XK_t           , 0, callback, (void*)(size_t)(NOTE_MIN + NOTE_C_OFFSET + octave*12 + 4 )); addTooltipBind("press e-+0", state, XK_T         , 0, callback, (void*)(size_t)(NOTE_MIN + NOTE_SMOOTH_OFFSET + NOTE_C_OFFSET + octave*12 + 4 ));
	addTooltipBind("press F-+0", state, XK_y           , 0, callback, (void*)(size_t)(NOTE_MIN + NOTE_C_OFFSET + octave*12 + 5 )); addTooltipBind("press f-+0", state, XK_Y         , 0, callback, (void*)(size_t)(NOTE_MIN + NOTE_SMOOTH_OFFSET + NOTE_C_OFFSET + octave*12 + 5 ));
	addTooltipBind("press F#+0", state, XK_u           , 0, callback, (void*)(size_t)(NOTE_MIN + NOTE_C_OFFSET + octave*12 + 6 )); addTooltipBind("press f#+0", state, XK_U         , 0, callback, (void*)(size_t)(NOTE_MIN + NOTE_SMOOTH_OFFSET + NOTE_C_OFFSET + octave*12 + 6 ));
	addTooltipBind("press G-+0", state, XK_i           , 0, callback, (void*)(size_t)(NOTE_MIN + NOTE_C_OFFSET + octave*12 + 7 )); addTooltipBind("press g-+0", state, XK_I         , 0, callback, (void*)(size_t)(NOTE_MIN + NOTE_SMOOTH_OFFSET + NOTE_C_OFFSET + octave*12 + 7 ));
	addTooltipBind("press G#+0", state, XK_o           , 0, callback, (void*)(size_t)(NOTE_MIN + NOTE_C_OFFSET + octave*12 + 8 )); addTooltipBind("press g#+0", state, XK_O         , 0, callback, (void*)(size_t)(NOTE_MIN + NOTE_SMOOTH_OFFSET + NOTE_C_OFFSET + octave*12 + 8 ));
	addTooltipBind("press A-+1", state, XK_p           , 0, callback, (void*)(size_t)(NOTE_MIN + NOTE_C_OFFSET + octave*12 + 9 )); addTooltipBind("press a-+1", state, XK_P         , 0, callback, (void*)(size_t)(NOTE_MIN + NOTE_SMOOTH_OFFSET + NOTE_C_OFFSET + octave*12 + 9 ));
	addTooltipBind("press A#+1", state, XK_bracketleft , 0, callback, (void*)(size_t)(NOTE_MIN + NOTE_C_OFFSET + octave*12 + 10)); addTooltipBind("press a#+1", state, XK_braceleft , 0, callback, (void*)(size_t)(NOTE_MIN + NOTE_SMOOTH_OFFSET + NOTE_C_OFFSET + octave*12 + 10));
	addTooltipBind("press B-+1", state, XK_bracketright, 0, callback, (void*)(size_t)(NOTE_MIN + NOTE_C_OFFSET + octave*12 + 11)); addTooltipBind("press b-+1", state, XK_braceright, 0, callback, (void*)(size_t)(NOTE_MIN + NOTE_SMOOTH_OFFSET + NOTE_C_OFFSET + octave*12 + 11));

	addTooltipBind("press C-+1", state, XK_a           , 0, callback, (void*)(size_t)(NOTE_MIN + NOTE_C_OFFSET + octave*12 + 12)); addTooltipBind("press c-+1", state, XK_A         , 0, callback, (void*)(size_t)(NOTE_MIN + NOTE_SMOOTH_OFFSET + NOTE_C_OFFSET + octave*12 + 12));
	addTooltipBind("press C#+1", state, XK_s           , 0, callback, (void*)(size_t)(NOTE_MIN + NOTE_C_OFFSET + octave*12 + 13)); addTooltipBind("press c#+1", state, XK_S         , 0, callback, (void*)(size_t)(NOTE_MIN + NOTE_SMOOTH_OFFSET + NOTE_C_OFFSET + octave*12 + 13));
	addTooltipBind("press D-+1", state, XK_d           , 0, callback, (void*)(size_t)(NOTE_MIN + NOTE_C_OFFSET + octave*12 + 14)); addTooltipBind("press d-+1", state, XK_D         , 0, callback, (void*)(size_t)(NOTE_MIN + NOTE_SMOOTH_OFFSET + NOTE_C_OFFSET + octave*12 + 14));
	addTooltipBind("press D#+1", state, XK_f           , 0, callback, (void*)(size_t)(NOTE_MIN + NOTE_C_OFFSET + octave*12 + 15)); addTooltipBind("press d#+1", state, XK_F         , 0, callback, (void*)(size_t)(NOTE_MIN + NOTE_SMOOTH_OFFSET + NOTE_C_OFFSET + octave*12 + 15));
	addTooltipBind("press E-+1", state, XK_g           , 0, callback, (void*)(size_t)(NOTE_MIN + NOTE_C_OFFSET + octave*12 + 16)); addTooltipBind("press e-+1", state, XK_G         , 0, callback, (void*)(size_t)(NOTE_MIN + NOTE_SMOOTH_OFFSET + NOTE_C_OFFSET + octave*12 + 16));
	addTooltipBind("press F-+1", state, XK_h           , 0, callback, (void*)(size_t)(NOTE_MIN + NOTE_C_OFFSET + octave*12 + 17)); addTooltipBind("press f-+1", state, XK_H         , 0, callback, (void*)(size_t)(NOTE_MIN + NOTE_SMOOTH_OFFSET + NOTE_C_OFFSET + octave*12 + 17));
	addTooltipBind("press F#+1", state, XK_j           , 0, callback, (void*)(size_t)(NOTE_MIN + NOTE_C_OFFSET + octave*12 + 18)); addTooltipBind("press f#+1", state, XK_J         , 0, callback, (void*)(size_t)(NOTE_MIN + NOTE_SMOOTH_OFFSET + NOTE_C_OFFSET + octave*12 + 18));
	addTooltipBind("press G-+1", state, XK_k           , 0, callback, (void*)(size_t)(NOTE_MIN + NOTE_C_OFFSET + octave*12 + 19)); addTooltipBind("press g-+1", state, XK_K         , 0, callback, (void*)(size_t)(NOTE_MIN + NOTE_SMOOTH_OFFSET + NOTE_C_OFFSET + octave*12 + 19));
	addTooltipBind("press G#+1", state, XK_l           , 0, callback, (void*)(size_t)(NOTE_MIN + NOTE_C_OFFSET + octave*12 + 20)); addTooltipBind("press g#+1", state, XK_L         , 0, callback, (void*)(size_t)(NOTE_MIN + NOTE_SMOOTH_OFFSET + NOTE_C_OFFSET + octave*12 + 20));
	addTooltipBind("press A-+2", state, XK_semicolon   , 0, callback, (void*)(size_t)(NOTE_MIN + NOTE_C_OFFSET + octave*12 + 21)); addTooltipBind("press a-+2", state, XK_colon     , 0, callback, (void*)(size_t)(NOTE_MIN + NOTE_SMOOTH_OFFSET + NOTE_C_OFFSET + octave*12 + 21));
	addTooltipBind("press A#+2", state, XK_apostrophe  , 0, callback, (void*)(size_t)(NOTE_MIN + NOTE_C_OFFSET + octave*12 + 22)); addTooltipBind("press a#+2", state, XK_quotedbl  , 0, callback, (void*)(size_t)(NOTE_MIN + NOTE_SMOOTH_OFFSET + NOTE_C_OFFSET + octave*12 + 22));
	addTooltipBind("press B-+2", state, XK_backslash   , 0, callback, (void*)(size_t)(NOTE_MIN + NOTE_C_OFFSET + octave*12 + 23)); addTooltipBind("press b-+2", state, XK_bar       , 0, callback, (void*)(size_t)(NOTE_MIN + NOTE_SMOOTH_OFFSET + NOTE_C_OFFSET + octave*12 + 23));

	addTooltipBind("press C-+2", state, XK_z           , 0, callback, (void*)(size_t)(NOTE_MIN + NOTE_C_OFFSET + octave*12 + 24)); addTooltipBind("press c-+2", state, XK_Z         , 0, callback, (void*)(size_t)(NOTE_MIN + NOTE_SMOOTH_OFFSET + NOTE_C_OFFSET + octave*12 + 24));
	addTooltipBind("press C#+2", state, XK_x           , 0, callback, (void*)(size_t)(NOTE_MIN + NOTE_C_OFFSET + octave*12 + 25)); addTooltipBind("press c#+2", state, XK_X         , 0, callback, (void*)(size_t)(NOTE_MIN + NOTE_SMOOTH_OFFSET + NOTE_C_OFFSET + octave*12 + 25));
	addTooltipBind("press D-+2", state, XK_c           , 0, callback, (void*)(size_t)(NOTE_MIN + NOTE_C_OFFSET + octave*12 + 26)); addTooltipBind("press d-+2", state, XK_C         , 0, callback, (void*)(size_t)(NOTE_MIN + NOTE_SMOOTH_OFFSET + NOTE_C_OFFSET + octave*12 + 26));
	addTooltipBind("press D#+2", state, XK_v           , 0, callback, (void*)(size_t)(NOTE_MIN + NOTE_C_OFFSET + octave*12 + 27)); addTooltipBind("press d#+2", state, XK_V         , 0, callback, (void*)(size_t)(NOTE_MIN + NOTE_SMOOTH_OFFSET + NOTE_C_OFFSET + octave*12 + 27));
	addTooltipBind("press E-+2", state, XK_b           , 0, callback, (void*)(size_t)(NOTE_MIN + NOTE_C_OFFSET + octave*12 + 28)); addTooltipBind("press e-+2", state, XK_B         , 0, callback, (void*)(size_t)(NOTE_MIN + NOTE_SMOOTH_OFFSET + NOTE_C_OFFSET + octave*12 + 28));
	addTooltipBind("press F-+2", state, XK_n           , 0, callback, (void*)(size_t)(NOTE_MIN + NOTE_C_OFFSET + octave*12 + 29)); addTooltipBind("press f-+2", state, XK_N         , 0, callback, (void*)(size_t)(NOTE_MIN + NOTE_SMOOTH_OFFSET + NOTE_C_OFFSET + octave*12 + 29));
	addTooltipBind("press F#+2", state, XK_m           , 0, callback, (void*)(size_t)(NOTE_MIN + NOTE_C_OFFSET + octave*12 + 30)); addTooltipBind("press f#+2", state, XK_M         , 0, callback, (void*)(size_t)(NOTE_MIN + NOTE_SMOOTH_OFFSET + NOTE_C_OFFSET + octave*12 + 30));
	addTooltipBind("press G-+2", state, XK_comma       , 0, callback, (void*)(size_t)(NOTE_MIN + NOTE_C_OFFSET + octave*12 + 31)); addTooltipBind("press g-+2", state, XK_less      , 0, callback, (void*)(size_t)(NOTE_MIN + NOTE_SMOOTH_OFFSET + NOTE_C_OFFSET + octave*12 + 31));
	addTooltipBind("press G#+2", state, XK_period      , 0, callback, (void*)(size_t)(NOTE_MIN + NOTE_C_OFFSET + octave*12 + 32)); addTooltipBind("press g#+2", state, XK_greater   , 0, callback, (void*)(size_t)(NOTE_MIN + NOTE_SMOOTH_OFFSET + NOTE_C_OFFSET + octave*12 + 32));
	addTooltipBind("press A-+3", state, XK_slash       , 0, callback, (void*)(size_t)(NOTE_MIN + NOTE_C_OFFSET + octave*12 + 33)); addTooltipBind("press a-+3", state, XK_question  , 0, callback, (void*)(size_t)(NOTE_MIN + NOTE_SMOOTH_OFFSET + NOTE_C_OFFSET + octave*12 + 33));

	addTooltipBind("note off" , state, XK_equal       , 0, callback, (void*)NOTE_OFF);
	addTooltipBind("note cut" , state, XK_asciicircum , 0, callback, (void*)NOTE_CUT);
}
void addNoteReleaseBinds(const char *prettyname, unsigned int state, signed char octave, void (*callback)(void*))
{
	addTooltipPrettyPrint(prettyname, state, "release note keys");
	addTooltipBind("release C-+0", state, XK_q           , TT_RELEASE, callback, (void*)(size_t)(NOTE_MIN + NOTE_C_OFFSET + octave*12 + 0 )); addTooltipBind("release c-+0", state, XK_Q         , TT_RELEASE, callback, (void*)(size_t)(NOTE_MIN + NOTE_SMOOTH_OFFSET + NOTE_C_OFFSET + octave*12 + 0 ));
	addTooltipBind("release C#+0", state, XK_w           , TT_RELEASE, callback, (void*)(size_t)(NOTE_MIN + NOTE_C_OFFSET + octave*12 + 1 )); addTooltipBind("release c#+0", state, XK_W         , TT_RELEASE, callback, (void*)(size_t)(NOTE_MIN + NOTE_SMOOTH_OFFSET + NOTE_C_OFFSET + octave*12 + 1 ));
	addTooltipBind("release D-+0", state, XK_e           , TT_RELEASE, callback, (void*)(size_t)(NOTE_MIN + NOTE_C_OFFSET + octave*12 + 2 )); addTooltipBind("release d-+0", state, XK_E         , TT_RELEASE, callback, (void*)(size_t)(NOTE_MIN + NOTE_SMOOTH_OFFSET + NOTE_C_OFFSET + octave*12 + 2 ));
	addTooltipBind("release D#+0", state, XK_r           , TT_RELEASE, callback, (void*)(size_t)(NOTE_MIN + NOTE_C_OFFSET + octave*12 + 3 )); addTooltipBind("release d#+0", state, XK_R         , TT_RELEASE, callback, (void*)(size_t)(NOTE_MIN + NOTE_SMOOTH_OFFSET + NOTE_C_OFFSET + octave*12 + 3 ));
	addTooltipBind("release E-+0", state, XK_t           , TT_RELEASE, callback, (void*)(size_t)(NOTE_MIN + NOTE_C_OFFSET + octave*12 + 4 )); addTooltipBind("release e-+0", state, XK_T         , TT_RELEASE, callback, (void*)(size_t)(NOTE_MIN + NOTE_SMOOTH_OFFSET + NOTE_C_OFFSET + octave*12 + 4 ));
	addTooltipBind("release F-+0", state, XK_y           , TT_RELEASE, callback, (void*)(size_t)(NOTE_MIN + NOTE_C_OFFSET + octave*12 + 5 )); addTooltipBind("release f-+0", state, XK_Y         , TT_RELEASE, callback, (void*)(size_t)(NOTE_MIN + NOTE_SMOOTH_OFFSET + NOTE_C_OFFSET + octave*12 + 5 ));
	addTooltipBind("release F#+0", state, XK_u           , TT_RELEASE, callback, (void*)(size_t)(NOTE_MIN + NOTE_C_OFFSET + octave*12 + 6 )); addTooltipBind("release f#+0", state, XK_U         , TT_RELEASE, callback, (void*)(size_t)(NOTE_MIN + NOTE_SMOOTH_OFFSET + NOTE_C_OFFSET + octave*12 + 6 ));
	addTooltipBind("release G-+0", state, XK_i           , TT_RELEASE, callback, (void*)(size_t)(NOTE_MIN + NOTE_C_OFFSET + octave*12 + 7 )); addTooltipBind("release g-+0", state, XK_I         , TT_RELEASE, callback, (void*)(size_t)(NOTE_MIN + NOTE_SMOOTH_OFFSET + NOTE_C_OFFSET + octave*12 + 7 ));
	addTooltipBind("release G#+0", state, XK_o           , TT_RELEASE, callback, (void*)(size_t)(NOTE_MIN + NOTE_C_OFFSET + octave*12 + 8 )); addTooltipBind("release g#+0", state, XK_O         , TT_RELEASE, callback, (void*)(size_t)(NOTE_MIN + NOTE_SMOOTH_OFFSET + NOTE_C_OFFSET + octave*12 + 8 ));
	addTooltipBind("release A-+1", state, XK_p           , TT_RELEASE, callback, (void*)(size_t)(NOTE_MIN + NOTE_C_OFFSET + octave*12 + 9 )); addTooltipBind("release a-+1", state, XK_P         , TT_RELEASE, callback, (void*)(size_t)(NOTE_MIN + NOTE_SMOOTH_OFFSET + NOTE_C_OFFSET + octave*12 + 9 ));
	addTooltipBind("release A#+1", state, XK_bracketleft , TT_RELEASE, callback, (void*)(size_t)(NOTE_MIN + NOTE_C_OFFSET + octave*12 + 10)); addTooltipBind("release a#+1", state, XK_braceleft , TT_RELEASE, callback, (void*)(size_t)(NOTE_MIN + NOTE_SMOOTH_OFFSET + NOTE_C_OFFSET + octave*12 + 10));
	addTooltipBind("release B-+1", state, XK_bracketright, TT_RELEASE, callback, (void*)(size_t)(NOTE_MIN + NOTE_C_OFFSET + octave*12 + 11)); addTooltipBind("release b-+1", state, XK_braceright, TT_RELEASE, callback, (void*)(size_t)(NOTE_MIN + NOTE_SMOOTH_OFFSET + NOTE_C_OFFSET + octave*12 + 11));

	addTooltipBind("release C-+1", state, XK_a           , TT_RELEASE, callback, (void*)(size_t)(NOTE_MIN + NOTE_C_OFFSET + octave*12 + 12)); addTooltipBind("release c-+1", state, XK_A         , TT_RELEASE, callback, (void*)(size_t)(NOTE_MIN + NOTE_SMOOTH_OFFSET + NOTE_C_OFFSET + octave*12 + 12));
	addTooltipBind("release C#+1", state, XK_s           , TT_RELEASE, callback, (void*)(size_t)(NOTE_MIN + NOTE_C_OFFSET + octave*12 + 13)); addTooltipBind("release c#+1", state, XK_S         , TT_RELEASE, callback, (void*)(size_t)(NOTE_MIN + NOTE_SMOOTH_OFFSET + NOTE_C_OFFSET + octave*12 + 13));
	addTooltipBind("release D-+1", state, XK_d           , TT_RELEASE, callback, (void*)(size_t)(NOTE_MIN + NOTE_C_OFFSET + octave*12 + 14)); addTooltipBind("release d-+1", state, XK_D         , TT_RELEASE, callback, (void*)(size_t)(NOTE_MIN + NOTE_SMOOTH_OFFSET + NOTE_C_OFFSET + octave*12 + 14));
	addTooltipBind("release D#+1", state, XK_f           , TT_RELEASE, callback, (void*)(size_t)(NOTE_MIN + NOTE_C_OFFSET + octave*12 + 15)); addTooltipBind("release d#+1", state, XK_F         , TT_RELEASE, callback, (void*)(size_t)(NOTE_MIN + NOTE_SMOOTH_OFFSET + NOTE_C_OFFSET + octave*12 + 15));
	addTooltipBind("release E-+1", state, XK_g           , TT_RELEASE, callback, (void*)(size_t)(NOTE_MIN + NOTE_C_OFFSET + octave*12 + 16)); addTooltipBind("release e-+1", state, XK_G         , TT_RELEASE, callback, (void*)(size_t)(NOTE_MIN + NOTE_SMOOTH_OFFSET + NOTE_C_OFFSET + octave*12 + 16));
	addTooltipBind("release F-+1", state, XK_h           , TT_RELEASE, callback, (void*)(size_t)(NOTE_MIN + NOTE_C_OFFSET + octave*12 + 17)); addTooltipBind("release f-+1", state, XK_H         , TT_RELEASE, callback, (void*)(size_t)(NOTE_MIN + NOTE_SMOOTH_OFFSET + NOTE_C_OFFSET + octave*12 + 17));
	addTooltipBind("release F#+1", state, XK_j           , TT_RELEASE, callback, (void*)(size_t)(NOTE_MIN + NOTE_C_OFFSET + octave*12 + 18)); addTooltipBind("release f#+1", state, XK_J         , TT_RELEASE, callback, (void*)(size_t)(NOTE_MIN + NOTE_SMOOTH_OFFSET + NOTE_C_OFFSET + octave*12 + 18));
	addTooltipBind("release G-+1", state, XK_k           , TT_RELEASE, callback, (void*)(size_t)(NOTE_MIN + NOTE_C_OFFSET + octave*12 + 19)); addTooltipBind("release g-+1", state, XK_K         , TT_RELEASE, callback, (void*)(size_t)(NOTE_MIN + NOTE_SMOOTH_OFFSET + NOTE_C_OFFSET + octave*12 + 19));
	addTooltipBind("release G#+1", state, XK_l           , TT_RELEASE, callback, (void*)(size_t)(NOTE_MIN + NOTE_C_OFFSET + octave*12 + 20)); addTooltipBind("release g#+1", state, XK_L         , TT_RELEASE, callback, (void*)(size_t)(NOTE_MIN + NOTE_SMOOTH_OFFSET + NOTE_C_OFFSET + octave*12 + 20));
	addTooltipBind("release A-+2", state, XK_semicolon   , TT_RELEASE, callback, (void*)(size_t)(NOTE_MIN + NOTE_C_OFFSET + octave*12 + 21)); addTooltipBind("release a-+2", state, XK_colon     , TT_RELEASE, callback, (void*)(size_t)(NOTE_MIN + NOTE_SMOOTH_OFFSET + NOTE_C_OFFSET + octave*12 + 21));
	addTooltipBind("release A#+2", state, XK_apostrophe  , TT_RELEASE, callback, (void*)(size_t)(NOTE_MIN + NOTE_C_OFFSET + octave*12 + 22)); addTooltipBind("release a#+2", state, XK_quotedbl  , TT_RELEASE, callback, (void*)(size_t)(NOTE_MIN + NOTE_SMOOTH_OFFSET + NOTE_C_OFFSET + octave*12 + 22));
	addTooltipBind("release B-+2", state, XK_backslash   , TT_RELEASE, callback, (void*)(size_t)(NOTE_MIN + NOTE_C_OFFSET + octave*12 + 23)); addTooltipBind("release b-+2", state, XK_bar       , TT_RELEASE, callback, (void*)(size_t)(NOTE_MIN + NOTE_SMOOTH_OFFSET + NOTE_C_OFFSET + octave*12 + 23));

	addTooltipBind("release C-+2", state, XK_z           , TT_RELEASE, callback, (void*)(size_t)(NOTE_MIN + NOTE_C_OFFSET + octave*12 + 24)); addTooltipBind("release c-+2", state, XK_Z         , TT_RELEASE, callback, (void*)(size_t)(NOTE_MIN + NOTE_SMOOTH_OFFSET + NOTE_C_OFFSET + octave*12 + 24));
	addTooltipBind("release C#+2", state, XK_x           , TT_RELEASE, callback, (void*)(size_t)(NOTE_MIN + NOTE_C_OFFSET + octave*12 + 25)); addTooltipBind("release c#+2", state, XK_X         , TT_RELEASE, callback, (void*)(size_t)(NOTE_MIN + NOTE_SMOOTH_OFFSET + NOTE_C_OFFSET + octave*12 + 25));
	addTooltipBind("release D-+2", state, XK_c           , TT_RELEASE, callback, (void*)(size_t)(NOTE_MIN + NOTE_C_OFFSET + octave*12 + 26)); addTooltipBind("release d-+2", state, XK_C         , TT_RELEASE, callback, (void*)(size_t)(NOTE_MIN + NOTE_SMOOTH_OFFSET + NOTE_C_OFFSET + octave*12 + 26));
	addTooltipBind("release D#+2", state, XK_v           , TT_RELEASE, callback, (void*)(size_t)(NOTE_MIN + NOTE_C_OFFSET + octave*12 + 27)); addTooltipBind("release d#+2", state, XK_V         , TT_RELEASE, callback, (void*)(size_t)(NOTE_MIN + NOTE_SMOOTH_OFFSET + NOTE_C_OFFSET + octave*12 + 27));
	addTooltipBind("release E-+2", state, XK_b           , TT_RELEASE, callback, (void*)(size_t)(NOTE_MIN + NOTE_C_OFFSET + octave*12 + 28)); addTooltipBind("release e-+2", state, XK_B         , TT_RELEASE, callback, (void*)(size_t)(NOTE_MIN + NOTE_SMOOTH_OFFSET + NOTE_C_OFFSET + octave*12 + 28));
	addTooltipBind("release F-+2", state, XK_n           , TT_RELEASE, callback, (void*)(size_t)(NOTE_MIN + NOTE_C_OFFSET + octave*12 + 29)); addTooltipBind("release f-+2", state, XK_N         , TT_RELEASE, callback, (void*)(size_t)(NOTE_MIN + NOTE_SMOOTH_OFFSET + NOTE_C_OFFSET + octave*12 + 29));
	addTooltipBind("release F#+2", state, XK_m           , TT_RELEASE, callback, (void*)(size_t)(NOTE_MIN + NOTE_C_OFFSET + octave*12 + 30)); addTooltipBind("release f#+2", state, XK_M         , TT_RELEASE, callback, (void*)(size_t)(NOTE_MIN + NOTE_SMOOTH_OFFSET + NOTE_C_OFFSET + octave*12 + 30));
	addTooltipBind("release G-+2", state, XK_comma       , TT_RELEASE, callback, (void*)(size_t)(NOTE_MIN + NOTE_C_OFFSET + octave*12 + 31)); addTooltipBind("release g-+2", state, XK_less      , TT_RELEASE, callback, (void*)(size_t)(NOTE_MIN + NOTE_SMOOTH_OFFSET + NOTE_C_OFFSET + octave*12 + 31));
	addTooltipBind("release G#+2", state, XK_period      , TT_RELEASE, callback, (void*)(size_t)(NOTE_MIN + NOTE_C_OFFSET + octave*12 + 32)); addTooltipBind("release g#+2", state, XK_greater   , TT_RELEASE, callback, (void*)(size_t)(NOTE_MIN + NOTE_SMOOTH_OFFSET + NOTE_C_OFFSET + octave*12 + 32));
	addTooltipBind("release A-+3", state, XK_slash       , TT_RELEASE, callback, (void*)(size_t)(NOTE_MIN + NOTE_C_OFFSET + octave*12 + 33)); addTooltipBind("release a-+3", state, XK_question  , TT_RELEASE, callback, (void*)(size_t)(NOTE_MIN + NOTE_SMOOTH_OFFSET + NOTE_C_OFFSET + octave*12 + 33));
}

void addHexBinds(const char *prettyname, unsigned int state, void (*callback)(void*))
{
	addTooltipPrettyPrint(prettyname, state, "0-f");
	addTooltipBind("0x0", state, XK_0, 0, callback, (void*)0x0);
	addTooltipBind("0x1", state, XK_1, 0, callback, (void*)0x1);
	addTooltipBind("0x2", state, XK_2, 0, callback, (void*)0x2);
	addTooltipBind("0x3", state, XK_3, 0, callback, (void*)0x3);
	addTooltipBind("0x4", state, XK_4, 0, callback, (void*)0x4);
	addTooltipBind("0x5", state, XK_5, 0, callback, (void*)0x5);
	addTooltipBind("0x6", state, XK_6, 0, callback, (void*)0x6);
	addTooltipBind("0x7", state, XK_7, 0, callback, (void*)0x7);
	addTooltipBind("0x8", state, XK_8, 0, callback, (void*)0x8);
	addTooltipBind("0x9", state, XK_9, 0, callback, (void*)0x9);
	addTooltipBind("0xa", state, XK_a, 0, callback, (void*)0xa); addTooltipBind("0xA", state, XK_A, 0, callback, (void*)0xa);
	addTooltipBind("0xb", state, XK_b, 0, callback, (void*)0xb); addTooltipBind("0xB", state, XK_B, 0, callback, (void*)0xb);
	addTooltipBind("0xc", state, XK_c, 0, callback, (void*)0xc); addTooltipBind("0xC", state, XK_C, 0, callback, (void*)0xc);
	addTooltipBind("0xd", state, XK_d, 0, callback, (void*)0xd); addTooltipBind("0xD", state, XK_D, 0, callback, (void*)0xd);
	addTooltipBind("0xe", state, XK_e, 0, callback, (void*)0xe); addTooltipBind("0xE", state, XK_E, 0, callback, (void*)0xe);
	addTooltipBind("0xf", state, XK_f, 0, callback, (void*)0xf); addTooltipBind("0xF", state, XK_F, 0, callback, (void*)0xf);
}
void addDecimalBinds(const char *prettyname, unsigned int state, void (*callback)(void*))
{
	addTooltipPrettyPrint(prettyname, state, "0-9");
	addTooltipBind("0", state, XK_0, 0, callback, (void*)0);
	addTooltipBind("1", state, XK_1, 0, callback, (void*)1);
	addTooltipBind("2", state, XK_2, 0, callback, (void*)2);
	addTooltipBind("3", state, XK_3, 0, callback, (void*)3);
	addTooltipBind("4", state, XK_4, 0, callback, (void*)4);
	addTooltipBind("5", state, XK_5, 0, callback, (void*)5);
	addTooltipBind("6", state, XK_6, 0, callback, (void*)6);
	addTooltipBind("7", state, XK_7, 0, callback, (void*)7);
	addTooltipBind("8", state, XK_8, 0, callback, (void*)8);
	addTooltipBind("9", state, XK_9, 0, callback, (void*)9);
}

void addPrintableAsciiBinds(const char *prettyname, unsigned int state, void (*callback)(void*))
{
	addTooltipPrettyPrint(prettyname, state, "ascii");
	addTooltipBind(" " , state, XK_space       , 0, callback, (void*)' ');
	addTooltipBind("!" , state, XK_exclam      , 0, callback, (void*)'!');
	addTooltipBind("\"", state, XK_quotedbl    , 0, callback, (void*)'"');
	addTooltipBind("#" , state, XK_numbersign  , 0, callback, (void*)'#');
	addTooltipBind("$" , state, XK_dollar      , 0, callback, (void*)'$');
	addTooltipBind("%" , state, XK_percent     , 0, callback, (void*)'%');
	addTooltipBind("&" , state, XK_ampersand   , 0, callback, (void*)'&');
	addTooltipBind("'" , state, XK_apostrophe  , 0, callback, (void*)'\'');
	addTooltipBind("(" , state, XK_parenleft   , 0, callback, (void*)'(');
	addTooltipBind(")" , state, XK_parenright  , 0, callback, (void*)')');
	addTooltipBind("*" , state, XK_asterisk    , 0, callback, (void*)'*');
	addTooltipBind("+" , state, XK_plus        , 0, callback, (void*)'+');
	addTooltipBind("," , state, XK_comma       , 0, callback, (void*)',');
	addTooltipBind("-" , state, XK_minus       , 0, callback, (void*)'-');
	addTooltipBind("." , state, XK_period      , 0, callback, (void*)'.');
	addTooltipBind("/" , state, XK_slash       , 0, callback, (void*)'/');
	addTooltipBind("0" , state, XK_0           , 0, callback, (void*)'0');
	addTooltipBind("1" , state, XK_1           , 0, callback, (void*)'1');
	addTooltipBind("2" , state, XK_2           , 0, callback, (void*)'2');
	addTooltipBind("3" , state, XK_3           , 0, callback, (void*)'3');
	addTooltipBind("4" , state, XK_4           , 0, callback, (void*)'4');
	addTooltipBind("5" , state, XK_5           , 0, callback, (void*)'5');
	addTooltipBind("6" , state, XK_6           , 0, callback, (void*)'6');
	addTooltipBind("7" , state, XK_7           , 0, callback, (void*)'7');
	addTooltipBind("8" , state, XK_8           , 0, callback, (void*)'8');
	addTooltipBind("9" , state, XK_9           , 0, callback, (void*)'9');
	addTooltipBind(":" , state, XK_colon       , 0, callback, (void*)':');
	addTooltipBind(";" , state, XK_semicolon   , 0, callback, (void*)';');
	addTooltipBind("<" , state, XK_less        , 0, callback, (void*)'<');
	addTooltipBind("=" , state, XK_equal       , 0, callback, (void*)'=');
	addTooltipBind(">" , state, XK_greater     , 0, callback, (void*)'>');
	addTooltipBind("?" , state, XK_question    , 0, callback, (void*)'?');

	addTooltipBind("@" , state, XK_at          , 0, callback, (void*)'@');
	addTooltipBind("A" , state, XK_A           , 0, callback, (void*)'A');
	addTooltipBind("B" , state, XK_B           , 0, callback, (void*)'B');
	addTooltipBind("C" , state, XK_C           , 0, callback, (void*)'C');
	addTooltipBind("D" , state, XK_D           , 0, callback, (void*)'D');
	addTooltipBind("E" , state, XK_E           , 0, callback, (void*)'E');
	addTooltipBind("F" , state, XK_F           , 0, callback, (void*)'F');
	addTooltipBind("G" , state, XK_G           , 0, callback, (void*)'G');
	addTooltipBind("H" , state, XK_H           , 0, callback, (void*)'H');
	addTooltipBind("I" , state, XK_I           , 0, callback, (void*)'I');
	addTooltipBind("J" , state, XK_J           , 0, callback, (void*)'J');
	addTooltipBind("K" , state, XK_K           , 0, callback, (void*)'K');
	addTooltipBind("L" , state, XK_L           , 0, callback, (void*)'L');
	addTooltipBind("M" , state, XK_M           , 0, callback, (void*)'M');
	addTooltipBind("N" , state, XK_N           , 0, callback, (void*)'N');
	addTooltipBind("O" , state, XK_O           , 0, callback, (void*)'O');
	addTooltipBind("P" , state, XK_P           , 0, callback, (void*)'P');
	addTooltipBind("Q" , state, XK_Q           , 0, callback, (void*)'Q');
	addTooltipBind("R" , state, XK_R           , 0, callback, (void*)'R');
	addTooltipBind("S" , state, XK_S           , 0, callback, (void*)'S');
	addTooltipBind("T" , state, XK_T           , 0, callback, (void*)'T');
	addTooltipBind("U" , state, XK_U           , 0, callback, (void*)'U');
	addTooltipBind("V" , state, XK_V           , 0, callback, (void*)'V');
	addTooltipBind("W" , state, XK_W           , 0, callback, (void*)'W');
	addTooltipBind("X" , state, XK_X           , 0, callback, (void*)'X');
	addTooltipBind("Y" , state, XK_Y           , 0, callback, (void*)'Y');
	addTooltipBind("Z" , state, XK_Z           , 0, callback, (void*)'Z');
	addTooltipBind("[" , state, XK_bracketleft , 0, callback, (void*)'[');
	addTooltipBind("\\", state, XK_backslash   , 0, callback, (void*)'\\');
	addTooltipBind("]" , state, XK_bracketright, 0, callback, (void*)']');
	addTooltipBind("^" , state, XK_asciicircum , 0, callback, (void*)'^');
	addTooltipBind("_" , state, XK_underscore  , 0, callback, (void*)'_');
	addTooltipBind("`" , state, XK_grave       , 0, callback, (void*)'`');
	addTooltipBind("a" , state, XK_a           , 0, callback, (void*)'a');
	addTooltipBind("b" , state, XK_b           , 0, callback, (void*)'b');
	addTooltipBind("c" , state, XK_c           , 0, callback, (void*)'c');
	addTooltipBind("d" , state, XK_d           , 0, callback, (void*)'d');
	addTooltipBind("e" , state, XK_e           , 0, callback, (void*)'e');
	addTooltipBind("f" , state, XK_f           , 0, callback, (void*)'f');
	addTooltipBind("g" , state, XK_g           , 0, callback, (void*)'g');
	addTooltipBind("h" , state, XK_h           , 0, callback, (void*)'h');
	addTooltipBind("i" , state, XK_i           , 0, callback, (void*)'i');
	addTooltipBind("j" , state, XK_j           , 0, callback, (void*)'j');
	addTooltipBind("k" , state, XK_k           , 0, callback, (void*)'k');
	addTooltipBind("l" , state, XK_l           , 0, callback, (void*)'l');
	addTooltipBind("m" , state, XK_m           , 0, callback, (void*)'m');
	addTooltipBind("n" , state, XK_n           , 0, callback, (void*)'n');
	addTooltipBind("o" , state, XK_o           , 0, callback, (void*)'o');
	addTooltipBind("p" , state, XK_p           , 0, callback, (void*)'p');
	addTooltipBind("q" , state, XK_q           , 0, callback, (void*)'q');
	addTooltipBind("r" , state, XK_r           , 0, callback, (void*)'r');
	addTooltipBind("s" , state, XK_s           , 0, callback, (void*)'s');
	addTooltipBind("t" , state, XK_t           , 0, callback, (void*)'t');
	addTooltipBind("u" , state, XK_u           , 0, callback, (void*)'u');
	addTooltipBind("v" , state, XK_v           , 0, callback, (void*)'v');
	addTooltipBind("w" , state, XK_w           , 0, callback, (void*)'w');
	addTooltipBind("x" , state, XK_x           , 0, callback, (void*)'x');
	addTooltipBind("y" , state, XK_y           , 0, callback, (void*)'y');
	addTooltipBind("z" , state, XK_z           , 0, callback, (void*)'z');
	addTooltipBind("{" , state, XK_braceleft   , 0, callback, (void*)'{');
	addTooltipBind("|" , state, XK_bar         , 0, callback, (void*)'|');
	addTooltipBind("}" , state, XK_braceright  , 0, callback, (void*)'}');
	addTooltipBind("~" , state, XK_asciitilde  , 0, callback, (void*)'~');
}

static void tooltipAddCount(void *add)
{
	w->count *= 10;
	w->count += (size_t)add;
	p->redraw = 1;
}
void addCountBinds(bool draw)
{
	unsigned int flags = TT_DEAD;
	if (draw) flags |= TT_DRAW;
	addTooltipBind("add 0 to count", 0, XK_0, flags, tooltipAddCount, (void*)0);
	addTooltipBind("add 1 to count", 0, XK_1, flags, tooltipAddCount, (void*)1);
	addTooltipBind("add 2 to count", 0, XK_2, flags, tooltipAddCount, (void*)2);
	addTooltipBind("add 3 to count", 0, XK_3, flags, tooltipAddCount, (void*)3);
	addTooltipBind("add 4 to count", 0, XK_4, flags, tooltipAddCount, (void*)4);
	addTooltipBind("add 5 to count", 0, XK_5, flags, tooltipAddCount, (void*)5);
	addTooltipBind("add 6 to count", 0, XK_6, flags, tooltipAddCount, (void*)6);
	addTooltipBind("add 7 to count", 0, XK_7, flags, tooltipAddCount, (void*)7);
	addTooltipBind("add 8 to count", 0, XK_8, flags, tooltipAddCount, (void*)8);
	addTooltipBind("add 9 to count", 0, XK_9, flags, tooltipAddCount, (void*)9);
}


static unsigned int ansiToModState(int input)
{
	unsigned int ret = 0;
	input -= '1'; /* '2' => 1 */
	if (input&1) ret |= ShiftMask;
	if (input&2) ret |= Mod1Mask;
	if (input&4) ret |= ControlMask;
	return ret;
}

void handleStdin(void)
{
	unsigned int mod = 0;
	static int input;
	while (1)
		switch (input = getchar())
		{
			case EOF: return;
			case '\033': /* escape */ switch (input = getchar()) {
					case EOF: inputTooltip(mod, XK_Escape, 0); return;
					// case ']': /* OSC */ switch (input = getchar()) {
					// 		case '4': {
					// 				int colour;
					// 				unsigned int r, g, b;
					// 				scanf(";%d;rgb:%02x%*02x/%02x%*02x/%02x%*02x\007",
					// 						&colour, &r, &g, &b);
					// 				setDefaultColour(colour, r, g, b);
					// 			} break;
					// 	} break;
					case 'O': /* SS3 */ switch (input = getchar()) {
							case 'P': inputTooltip(mod, XK_F1, 0); break;
							case 'Q': inputTooltip(mod, XK_F2, 0); break;
							case 'R': inputTooltip(mod, XK_F3, 0); break;
							case 'S': inputTooltip(mod, XK_F4, 0); break;
						} break;
					case '[': /* CSI */ switch (input = getchar()) {
							case '[': switch (input = getchar()) {
									case 'A': inputTooltip(mod, XK_F1, 0); break;
									case 'B': inputTooltip(mod, XK_F2, 0); break;
									case 'C': inputTooltip(mod, XK_F3, 0); break;
									case 'D': inputTooltip(mod, XK_F4, 0); break;
									case 'E': inputTooltip(mod, XK_F5, 0); break;
								} break;
							case 'A': inputTooltip(mod, XK_Up   , 0); break;
							case 'B': inputTooltip(mod, XK_Down , 0); break;
							case 'D': inputTooltip(mod, XK_Left , 0); break;
							case 'C': inputTooltip(mod, XK_Right, 0); break;
							case '1': switch (input = getchar()) {
									case '5': switch (input = getchar()) {
											case '~': inputTooltip(mod, XK_F5, 0); break;
											case ';': inputTooltip(mod|ansiToModState(getchar()), XK_F5, 0); getchar(); break;
										} break;
									case '7': switch (input = getchar()) {
											case '~': inputTooltip(mod, XK_F6, 0); break;
											case ';': inputTooltip(mod|ansiToModState(getchar()), XK_F6, 0); getchar(); break;
										} break;
									case '8': switch (input = getchar()) {
											case '~': inputTooltip(mod, XK_F7, 0); break;
											case ';': inputTooltip(mod|ansiToModState(getchar()), XK_F7, 0); getchar(); break;
										} break;
									case '9': switch (input = getchar()) {
											case '~': inputTooltip(mod, XK_F8, 0); break;
											case ';': inputTooltip(mod|ansiToModState(getchar()), XK_F8, 0); getchar(); break;
										} break;
									case ';':
										mod |= ansiToModState(getchar());
										switch (input = getchar()) {
											case 'A': inputTooltip(mod, XK_Up   , 0); break;
											case 'B': inputTooltip(mod, XK_Down , 0); break;
											case 'D': inputTooltip(mod, XK_Left , 0); break;
											case 'C': inputTooltip(mod, XK_Right, 0); break;
											case 'P': inputTooltip(mod, XK_F1, 0); break;
											case 'Q': inputTooltip(mod, XK_F2, 0); break;
											case 'R': inputTooltip(mod, XK_F3, 0); break;
											case 'S': inputTooltip(mod, XK_F4, 0); break;
										} break;
									case '~': inputTooltip(mod, XK_Home, 0); break;
								} break;
							case '2': switch (input = getchar()) {
									case '0': switch (input = getchar()) {
											case '~': inputTooltip(mod, XK_F9, 0); break;
											case ';': inputTooltip(mod|ansiToModState(getchar()), XK_F9, 0); getchar(); break;
										} break;
									case '1': switch (input = getchar()) {
											case '~': inputTooltip(mod, XK_F10, 0); break;
											case ';': inputTooltip(mod|ansiToModState(getchar()), XK_F10, 0); getchar(); break;
										} break;
									case '3': switch (input = getchar()) {
											case '~': inputTooltip(mod, XK_F11, 0); break;
											case ';': inputTooltip(mod|ansiToModState(getchar()), XK_F11, 0); getchar(); break;
										} break;
									case '4': switch (input = getchar()) {
											case '~': inputTooltip(mod, XK_F12, 0); break;
											case ';': inputTooltip(mod|ansiToModState(getchar()), XK_F12, 0); getchar(); break;
										} break;
									case 'J': inputTooltip(mod|ShiftMask, XK_Home  , 0); break;
									case 'K': inputTooltip(mod|ShiftMask, XK_Delete, 0); break;
								} break;
							case '4': if (getchar() == '~') inputTooltip(mod, XK_End, 0); break;
							case '5': switch (input = getchar()) {
									case '~': inputTooltip(mod, XK_Page_Up, 0); break;
									case ';': inputTooltip(mod|ansiToModState(getchar()), XK_Page_Up, 0); getchar(); break;
								} break;
							case '6': switch (input = getchar()) {
									case '~': inputTooltip(mod, XK_Page_Down, 0); getchar(); break;
									case ';': inputTooltip(mod|ansiToModState(getchar()), XK_Page_Down, 0); getchar(); break;
								} break;
							case 'H': inputTooltip(mod, XK_Home, 0); break;
							case 'J': inputTooltip(mod|ControlMask, XK_End, 0); break;
							case 'K': inputTooltip(mod|ShiftMask  , XK_End, 0); break;
							case 'M': { /* TODO: ctrl+delete maps to CSI M, which conflicts */
									enum Button button = getchar();
									int x = getchar() - ' ';
									int y = getchar() - ' ';
									if (tt.mousecallback)
										tt.mousecallback(button, x, y);
									resetInput();
								} break;
							case 'P': inputTooltip(mod, XK_Delete, 0); break;
						} break;
					default:
						if (input <= 0x20) inputTooltip(mod|Mod1Mask|ControlMask, input+0x40, 0);
						else               inputTooltip(mod|Mod1Mask            , input     , 0);
						break;
				} break;
			case '\n': case '\r': inputTooltip(mod, XK_Return, 0); break;
			case '\t': inputTooltip(mod, XK_Tab, 0); break;
			case '\b': case 127: inputTooltip(mod, XK_BackSpace, 0); break;
			case ' ': inputTooltip(mod, XK_space, 0); break;
			default:
				if (input <= 0x20) inputTooltip(mod|ControlMask, input+0x40, 0);
				else               inputTooltip(mod            , input     , 0);
				break;
		}
}
