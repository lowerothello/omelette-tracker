#define MOD_WIDTH 9 /* max width for the key name */

#define TT_DEAD    (1<<0) /* skip regenerating the tooltip state */
#define TT_DRAW    (1<<1) /* include in the info popup           */
#define TT_RELEASE (1<<2) /* trigger on key release              */
typedef struct {
	char        *prettyname;
	char        *prettykeysym;
	unsigned int state;  /* (XEvent*)->state compatible   */
	KeySym       keysym; /* X11 keysym (ascii compatible) */
	void       (*callback)(void*);
	void        *arg;    /* arg passed to callback */
	uint8_t      flags;
} TooltipEntry;

/* TODO: w->count should be here */
typedef struct {
	short  maxprettynamelen;
	char  *prettytitle;
	void (*mousecallback)(enum _BUTTON, int, int);

	uint8_t       entryc;
	TooltipEntry *entryv;
} TooltipState;


void clearTooltip(TooltipState *tt)
{
	if (tt->entryv)
	{
		for (int i = 0; i < tt->entryc; i++)
		{
			if (tt->entryv[i].prettyname)   free(tt->entryv[i].prettyname  );
			if (tt->entryv[i].prettykeysym) free(tt->entryv[i].prettykeysym);
		}

		free(tt->entryv);
		tt->entryv = NULL;
	}

	if (tt->prettytitle)
	{
		free(tt->prettytitle);
		tt->prettytitle = NULL;
	}

	tt->mousecallback = NULL;
	tt->entryc = 0;
	tt->maxprettynamelen = 0;
}

void setTooltipTitle(TooltipState *tt, char *prettytitle)
{
	if (tt->prettytitle) free(tt->prettytitle);
	tt->prettytitle = strdup(prettytitle);
}

void setTooltipMouseCallback(TooltipState *tt, void (*callback)(enum _BUTTON, int, int))
{
	tt->mousecallback = callback;
}

/* strlen(prettykeysym) should be <= MOD_WIDTH */
void addTooltipPrettyPrint(TooltipState *tt, const char *prettyname, const char *prettykeysym)
{
	TooltipEntry *newentryv = realloc(tt->entryv, (tt->entryc+1) * sizeof(TooltipEntry));
	memset(&newentryv[tt->entryc], 0, sizeof(TooltipEntry));

	newentryv[tt->entryc].prettyname = strdup(prettyname);
	tt->maxprettynamelen = MAX(tt->maxprettynamelen, strlen(prettyname));
	newentryv[tt->entryc].prettykeysym = strdup(prettykeysym);

	newentryv[tt->entryc].flags = TT_DRAW;

	tt->entryv = newentryv;
	tt->entryc++;
}
void addTooltipBind(TooltipState *tt, const char *prettyname, unsigned int state, KeySym keysym, uint8_t flags, void (*callback)(void *), void *arg)
{
	TooltipEntry *newentryv = realloc(tt->entryv, (tt->entryc+1) * sizeof(TooltipEntry));
	memset(&newentryv[tt->entryc], 0, sizeof(TooltipEntry));

	if (prettyname)
	{
		newentryv[tt->entryc].prettyname = strdup(prettyname);
		tt->maxprettynamelen = MAX(tt->maxprettynamelen, strlen(prettyname));
	}

	newentryv[tt->entryc].state = state;
	newentryv[tt->entryc].flags = flags;
	newentryv[tt->entryc].keysym = keysym;
	newentryv[tt->entryc].callback = callback;
	newentryv[tt->entryc].arg = arg;

	tt->entryv = newentryv;
	tt->entryc++;
}


void inputTooltip(TooltipState *tt, unsigned int state, KeySym input, bool release)
{
	/* don't try to process lower-case control keys */
	if ((state&ControlMask) && isascii(input))
		input = toupper(input);

	DEBUG = input;
	bool dead;
	for (int i = 0; i < tt->entryc; i++)
		if ( tt->entryv[i].keysym == input
		 &&  tt->entryv[i].state  == state
		 && (tt->entryv[i].flags&TT_RELEASE) == release)
		{
			dead = tt->entryv[i].flags&TT_DEAD; /* save flags in case the callback fucks with tt */
			if (tt->entryv[i].callback)
				tt->entryv[i].callback(tt->entryv[i].arg);

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
void drawTooltip(TooltipState *tt)
{
	if (!tt->entryc) return;
	printf("\033[s"); /* save the cursor pos */

	short drawc = 0;
	for (short i = 0; i < tt->entryc; i++)
		if (tt->entryv[i].prettyname && (tt->entryv[i].flags&TT_DRAW)) drawc++;

	short w = tt->maxprettynamelen + 2 + MOD_WIDTH;
	short h = drawc + 2;
	short x = ws.ws_col - w - 1;
	short y = ws.ws_row - h;

	char charbufferupper[2] = {'\0'};
	char charbufferlower[2] = {'\0'};

	short yo = 0;
	for (short i = 0; i < tt->entryc; i++)
	{
		if (tt->entryv[i].prettyname && (tt->entryv[i].flags&TT_DRAW))
		{
			if (tt->entryv[i].prettykeysym)
				drawTooltipLine(x, y+1+yo, tt->maxprettynamelen, tt->entryv[i].prettyname, tt->entryv[i].state, tt->entryv[i].prettykeysym, tt->entryv[i].prettykeysym);
			else
				switch (tt->entryv[i].keysym)
				{
					case XK_Up:        drawTooltipLine(x, y+1+yo, tt->maxprettynamelen, tt->entryv[i].prettyname, tt->entryv[i].state, "UP",    "up"); break;
					case XK_Down:      drawTooltipLine(x, y+1+yo, tt->maxprettynamelen, tt->entryv[i].prettyname, tt->entryv[i].state, "DOWN",  "down"); break;
					case XK_Left:      drawTooltipLine(x, y+1+yo, tt->maxprettynamelen, tt->entryv[i].prettyname, tt->entryv[i].state, "LEFT",  "left"); break;
					case XK_Right:     drawTooltipLine(x, y+1+yo, tt->maxprettynamelen, tt->entryv[i].prettyname, tt->entryv[i].state, "RIGHT", "right"); break;
					case XK_Home:      drawTooltipLine(x, y+1+yo, tt->maxprettynamelen, tt->entryv[i].prettyname, tt->entryv[i].state, "HOME",  "home"); break;
					case XK_End:       drawTooltipLine(x, y+1+yo, tt->maxprettynamelen, tt->entryv[i].prettyname, tt->entryv[i].state, "END",   "end"); break;
					case XK_Page_Up:   drawTooltipLine(x, y+1+yo, tt->maxprettynamelen, tt->entryv[i].prettyname, tt->entryv[i].state, "PGUP",  "pgup"); break;
					case XK_Page_Down: drawTooltipLine(x, y+1+yo, tt->maxprettynamelen, tt->entryv[i].prettyname, tt->entryv[i].state, "PGDN",  "pgdn"); break;
					case XK_F1:        drawTooltipLine(x, y+1+yo, tt->maxprettynamelen, tt->entryv[i].prettyname, tt->entryv[i].state, "F1",    "f1"); break;
					case XK_F2:        drawTooltipLine(x, y+1+yo, tt->maxprettynamelen, tt->entryv[i].prettyname, tt->entryv[i].state, "F2",    "f2"); break;
					case XK_F3:        drawTooltipLine(x, y+1+yo, tt->maxprettynamelen, tt->entryv[i].prettyname, tt->entryv[i].state, "F3",    "f3"); break;
					case XK_F4:        drawTooltipLine(x, y+1+yo, tt->maxprettynamelen, tt->entryv[i].prettyname, tt->entryv[i].state, "F4",    "f4"); break;
					case XK_F5:        drawTooltipLine(x, y+1+yo, tt->maxprettynamelen, tt->entryv[i].prettyname, tt->entryv[i].state, "F5",    "f5"); break;
					case XK_F6:        drawTooltipLine(x, y+1+yo, tt->maxprettynamelen, tt->entryv[i].prettyname, tt->entryv[i].state, "F6",    "f6"); break;
					case XK_F7:        drawTooltipLine(x, y+1+yo, tt->maxprettynamelen, tt->entryv[i].prettyname, tt->entryv[i].state, "F7",    "f7"); break;
					case XK_F8:        drawTooltipLine(x, y+1+yo, tt->maxprettynamelen, tt->entryv[i].prettyname, tt->entryv[i].state, "F8",    "f8"); break;
					case XK_F9:        drawTooltipLine(x, y+1+yo, tt->maxprettynamelen, tt->entryv[i].prettyname, tt->entryv[i].state, "F9",    "f9"); break;
					case XK_F10:       drawTooltipLine(x, y+1+yo, tt->maxprettynamelen, tt->entryv[i].prettyname, tt->entryv[i].state, "F10",   "f10"); break;
					case XK_F11:       drawTooltipLine(x, y+1+yo, tt->maxprettynamelen, tt->entryv[i].prettyname, tt->entryv[i].state, "F11",   "f11"); break;
					case XK_F12:       drawTooltipLine(x, y+1+yo, tt->maxprettynamelen, tt->entryv[i].prettyname, tt->entryv[i].state, "F12",   "f12"); break;
					case XK_BackSpace: drawTooltipLine(x, y+1+yo, tt->maxprettynamelen, tt->entryv[i].prettyname, tt->entryv[i].state, "\\B",   "\\b"); break;
					case XK_Return:    drawTooltipLine(x, y+1+yo, tt->maxprettynamelen, tt->entryv[i].prettyname, tt->entryv[i].state, "\\R",   "\\r"); break;
					case XK_Tab:       drawTooltipLine(x, y+1+yo, tt->maxprettynamelen, tt->entryv[i].prettyname, tt->entryv[i].state, "\\T",   "\\t"); break;
					case XK_Escape:    drawTooltipLine(x, y+1+yo, tt->maxprettynamelen, tt->entryv[i].prettyname, tt->entryv[i].state, "\\E",   "\\e"); break;
					default:
						if (isascii(tt->entryv[i].keysym))
						{
							charbufferupper[0] = toupper(tt->entryv[i].keysym);
							charbufferlower[0] = tolower(tt->entryv[i].keysym);
							drawTooltipLine(x, y+1+yo, tt->maxprettynamelen, tt->entryv[i].prettyname, tt->entryv[i].state, charbufferupper, charbufferlower);
						} break;
				}
			yo++;
		}
	}

	if (tt->prettytitle)
	{
		printf("\033[%d;%dH┌\033[%d;%dH┐", y-1, x, y-1, x+w);
		printf("\033[%d;%dH│\033[%d;%dH│", y,   x, y,   x+w);
		for (short i = 1; i < w; i++) printf("\033[%d;%dH ", y,   x+i);
		for (short i = 1; i < w; i++) printf("\033[%d;%dH─", y-1, x+i);
		printf("\033[%d;%dH\033[1m%s\033[m", y, x + ((w - ((short)strlen(tt->prettytitle)-1))>>1), tt->prettytitle);
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
void addNoteBinds(TooltipState *tt, const char *prettyname, unsigned int state, void (*callback)(void*))
{
	addTooltipPrettyPrint(tt, prettyname, "0x0-0xf");
	addTooltipBind(tt, "play C-+0", state, XK_q           , 0, callback, (void*)0 ); addTooltipBind(tt, "ALT CASE", state, XK_Q         , 0, callback, (void*)0 );
	addTooltipBind(tt, "play C#+0", state, XK_w           , 0, callback, (void*)1 ); addTooltipBind(tt, "ALT CASE", state, XK_W         , 0, callback, (void*)1 );
	addTooltipBind(tt, "play D-+0", state, XK_e           , 0, callback, (void*)2 ); addTooltipBind(tt, "ALT CASE", state, XK_E         , 0, callback, (void*)2 );
	addTooltipBind(tt, "play D#+0", state, XK_r           , 0, callback, (void*)3 ); addTooltipBind(tt, "ALT CASE", state, XK_R         , 0, callback, (void*)3 );
	addTooltipBind(tt, "play E-+0", state, XK_t           , 0, callback, (void*)4 ); addTooltipBind(tt, "ALT CASE", state, XK_T         , 0, callback, (void*)4 );
	addTooltipBind(tt, "play F-+0", state, XK_y           , 0, callback, (void*)5 ); addTooltipBind(tt, "ALT CASE", state, XK_Y         , 0, callback, (void*)5 );
	addTooltipBind(tt, "play F#+0", state, XK_u           , 0, callback, (void*)6 ); addTooltipBind(tt, "ALT CASE", state, XK_U         , 0, callback, (void*)6 );
	addTooltipBind(tt, "play G-+0", state, XK_i           , 0, callback, (void*)7 ); addTooltipBind(tt, "ALT CASE", state, XK_I         , 0, callback, (void*)7 );
	addTooltipBind(tt, "play G#+0", state, XK_o           , 0, callback, (void*)8 ); addTooltipBind(tt, "ALT CASE", state, XK_O         , 0, callback, (void*)8 );
	addTooltipBind(tt, "play A-+1", state, XK_p           , 0, callback, (void*)9 ); addTooltipBind(tt, "ALT CASE", state, XK_P         , 0, callback, (void*)9 );
	addTooltipBind(tt, "play A#+1", state, XK_bracketleft , 0, callback, (void*)10); addTooltipBind(tt, "ALT CASE", state, XK_braceleft , 0, callback, (void*)10);
	addTooltipBind(tt, "play B-+1", state, XK_bracketright, 0, callback, (void*)11); addTooltipBind(tt, "ALT CASE", state, XK_braceright, 0, callback, (void*)11);

	addTooltipBind(tt, "play C-+1", state, XK_a           , 0, callback, (void*)12); addTooltipBind(tt, "ALT CASE", state, XK_A         , 0, callback, (void*)12);
	addTooltipBind(tt, "play C#+1", state, XK_s           , 0, callback, (void*)13); addTooltipBind(tt, "ALT CASE", state, XK_S         , 0, callback, (void*)13);
	addTooltipBind(tt, "play D-+1", state, XK_d           , 0, callback, (void*)14); addTooltipBind(tt, "ALT CASE", state, XK_D         , 0, callback, (void*)14);
	addTooltipBind(tt, "play D#+1", state, XK_f           , 0, callback, (void*)15); addTooltipBind(tt, "ALT CASE", state, XK_F         , 0, callback, (void*)15);
	addTooltipBind(tt, "play E-+1", state, XK_g           , 0, callback, (void*)16); addTooltipBind(tt, "ALT CASE", state, XK_G         , 0, callback, (void*)16);
	addTooltipBind(tt, "play F-+1", state, XK_h           , 0, callback, (void*)17); addTooltipBind(tt, "ALT CASE", state, XK_H         , 0, callback, (void*)17);
	addTooltipBind(tt, "play F#+1", state, XK_j           , 0, callback, (void*)18); addTooltipBind(tt, "ALT CASE", state, XK_J         , 0, callback, (void*)18);
	addTooltipBind(tt, "play G-+1", state, XK_k           , 0, callback, (void*)19); addTooltipBind(tt, "ALT CASE", state, XK_K         , 0, callback, (void*)19);
	addTooltipBind(tt, "play G#+1", state, XK_l           , 0, callback, (void*)20); addTooltipBind(tt, "ALT CASE", state, XK_L         , 0, callback, (void*)20);
	addTooltipBind(tt, "play A-+2", state, XK_semicolon   , 0, callback, (void*)21); addTooltipBind(tt, "ALT CASE", state, XK_colon     , 0, callback, (void*)21);
	addTooltipBind(tt, "play A#+2", state, XK_apostrophe  , 0, callback, (void*)22); addTooltipBind(tt, "ALT CASE", state, XK_quotedbl  , 0, callback, (void*)22);
	addTooltipBind(tt, "play B-+2", state, XK_backslash   , 0, callback, (void*)23); addTooltipBind(tt, "ALT CASE", state, XK_bar       , 0, callback, (void*)23);

	addTooltipBind(tt, "play C-+2", state, XK_z           , 0, callback, (void*)24); addTooltipBind(tt, "ALT CASE", state, XK_Z         , 0, callback, (void*)24);
	addTooltipBind(tt, "play C#+2", state, XK_x           , 0, callback, (void*)25); addTooltipBind(tt, "ALT CASE", state, XK_X         , 0, callback, (void*)25);
	addTooltipBind(tt, "play D-+2", state, XK_c           , 0, callback, (void*)26); addTooltipBind(tt, "ALT CASE", state, XK_C         , 0, callback, (void*)26);
	addTooltipBind(tt, "play D#+2", state, XK_v           , 0, callback, (void*)27); addTooltipBind(tt, "ALT CASE", state, XK_V         , 0, callback, (void*)27);
	addTooltipBind(tt, "play E-+2", state, XK_b           , 0, callback, (void*)28); addTooltipBind(tt, "ALT CASE", state, XK_B         , 0, callback, (void*)28);
	addTooltipBind(tt, "play F-+2", state, XK_n           , 0, callback, (void*)29); addTooltipBind(tt, "ALT CASE", state, XK_N         , 0, callback, (void*)29);
	addTooltipBind(tt, "play F#+2", state, XK_m           , 0, callback, (void*)30); addTooltipBind(tt, "ALT CASE", state, XK_M         , 0, callback, (void*)30);
	addTooltipBind(tt, "play G-+2", state, XK_period      , 0, callback, (void*)31); addTooltipBind(tt, "ALT CASE", state, XK_less      , 0, callback, (void*)31);
	addTooltipBind(tt, "play G#+2", state, XK_comma       , 0, callback, (void*)32); addTooltipBind(tt, "ALT CASE", state, XK_greater   , 0, callback, (void*)32);
	addTooltipBind(tt, "play A-+3", state, XK_slash       , 0, callback, (void*)33); addTooltipBind(tt, "ALT CASE", state, XK_question  , 0, callback, (void*)33);
}
void addHexBinds(TooltipState *tt, const char *prettyname, unsigned int state, void (*callback)(void*))
{
	addTooltipPrettyPrint(tt, prettyname, "0-f");
	addTooltipBind(tt, "0x0", state, XK_0, 0, callback, (void*)0x0);
	addTooltipBind(tt, "0x1", state, XK_1, 0, callback, (void*)0x1);
	addTooltipBind(tt, "0x2", state, XK_2, 0, callback, (void*)0x2);
	addTooltipBind(tt, "0x3", state, XK_3, 0, callback, (void*)0x3);
	addTooltipBind(tt, "0x4", state, XK_4, 0, callback, (void*)0x4);
	addTooltipBind(tt, "0x5", state, XK_5, 0, callback, (void*)0x5);
	addTooltipBind(tt, "0x6", state, XK_6, 0, callback, (void*)0x6);
	addTooltipBind(tt, "0x7", state, XK_7, 0, callback, (void*)0x7);
	addTooltipBind(tt, "0x8", state, XK_8, 0, callback, (void*)0x8);
	addTooltipBind(tt, "0x9", state, XK_9, 0, callback, (void*)0x9);
	addTooltipBind(tt, "0xa", state, XK_a, 0, callback, (void*)0xa); addTooltipBind(tt, "0xA", state, XK_A, 0, callback, (void*)0xa);
	addTooltipBind(tt, "0xb", state, XK_b, 0, callback, (void*)0xb); addTooltipBind(tt, "0xB", state, XK_B, 0, callback, (void*)0xb);
	addTooltipBind(tt, "0xc", state, XK_c, 0, callback, (void*)0xc); addTooltipBind(tt, "0xC", state, XK_C, 0, callback, (void*)0xc);
	addTooltipBind(tt, "0xd", state, XK_d, 0, callback, (void*)0xd); addTooltipBind(tt, "0xD", state, XK_D, 0, callback, (void*)0xd);
	addTooltipBind(tt, "0xe", state, XK_e, 0, callback, (void*)0xe); addTooltipBind(tt, "0xE", state, XK_E, 0, callback, (void*)0xe);
	addTooltipBind(tt, "0xf", state, XK_f, 0, callback, (void*)0xf); addTooltipBind(tt, "0xF", state, XK_F, 0, callback, (void*)0xf);
}
void addDecimalBinds(TooltipState *tt, const char *prettyname, unsigned int state, void (*callback)(void*))
{
	addTooltipPrettyPrint(tt, prettyname, "0-9");
	addTooltipBind(tt, "0", state, XK_0, 0, callback, (void*)0);
	addTooltipBind(tt, "1", state, XK_1, 0, callback, (void*)1);
	addTooltipBind(tt, "2", state, XK_2, 0, callback, (void*)2);
	addTooltipBind(tt, "3", state, XK_3, 0, callback, (void*)3);
	addTooltipBind(tt, "4", state, XK_4, 0, callback, (void*)4);
	addTooltipBind(tt, "5", state, XK_5, 0, callback, (void*)5);
	addTooltipBind(tt, "6", state, XK_6, 0, callback, (void*)6);
	addTooltipBind(tt, "7", state, XK_7, 0, callback, (void*)7);
	addTooltipBind(tt, "8", state, XK_8, 0, callback, (void*)8);
	addTooltipBind(tt, "9", state, XK_9, 0, callback, (void*)9);
}

void addPrintableAsciiBinds(TooltipState *tt, const char *prettyname, unsigned int state, void (*callback)(void*))
{
	addTooltipPrettyPrint(tt, prettyname, "ascii");
	addTooltipBind(tt, " " , state, XK_space       , 0, callback, (void*)' ' );
	addTooltipBind(tt, "!" , state, XK_exclam      , 0, callback, (void*)'!' );
	addTooltipBind(tt, "\"", state, XK_quotedbl    , 0, callback, (void*)'"' );
	addTooltipBind(tt, "#" , state, XK_numbersign  , 0, callback, (void*)'#' );
	addTooltipBind(tt, "$" , state, XK_dollar      , 0, callback, (void*)'$' );
	addTooltipBind(tt, "%" , state, XK_percent     , 0, callback, (void*)'%' );
	addTooltipBind(tt, "&" , state, XK_ampersand   , 0, callback, (void*)'&' );
	addTooltipBind(tt, "'" , state, XK_apostrophe  , 0, callback, (void*)'\'');
	addTooltipBind(tt, "(" , state, XK_parenleft   , 0, callback, (void*)'(' );
	addTooltipBind(tt, ")" , state, XK_parenright  , 0, callback, (void*)')' );
	addTooltipBind(tt, "*" , state, XK_asterisk    , 0, callback, (void*)'*' );
	addTooltipBind(tt, "+" , state, XK_plus        , 0, callback, (void*)'+' );
	addTooltipBind(tt, "," , state, XK_comma       , 0, callback, (void*)',' );
	addTooltipBind(tt, "-" , state, XK_minus       , 0, callback, (void*)'-' );
	addTooltipBind(tt, "." , state, XK_period      , 0, callback, (void*)'.' );
	addTooltipBind(tt, "/" , state, XK_slash       , 0, callback, (void*)'/' );
	addTooltipBind(tt, "0" , state, XK_0           , 0, callback, (void*)'0' );
	addTooltipBind(tt, "1" , state, XK_1           , 0, callback, (void*)'1' );
	addTooltipBind(tt, "2" , state, XK_2           , 0, callback, (void*)'2' );
	addTooltipBind(tt, "3" , state, XK_3           , 0, callback, (void*)'3' );
	addTooltipBind(tt, "4" , state, XK_4           , 0, callback, (void*)'4' );
	addTooltipBind(tt, "5" , state, XK_5           , 0, callback, (void*)'5' );
	addTooltipBind(tt, "6" , state, XK_6           , 0, callback, (void*)'6' );
	addTooltipBind(tt, "7" , state, XK_7           , 0, callback, (void*)'7' );
	addTooltipBind(tt, "8" , state, XK_8           , 0, callback, (void*)'8' );
	addTooltipBind(tt, "9" , state, XK_9           , 0, callback, (void*)'9' );
	addTooltipBind(tt, ":" , state, XK_colon       , 0, callback, (void*)':' );
	addTooltipBind(tt, ";" , state, XK_semicolon   , 0, callback, (void*)';' );
	addTooltipBind(tt, "<" , state, XK_less        , 0, callback, (void*)'<' );
	addTooltipBind(tt, "=" , state, XK_equal       , 0, callback, (void*)'=' );
	addTooltipBind(tt, ">" , state, XK_greater     , 0, callback, (void*)'>' );
	addTooltipBind(tt, "?" , state, XK_question    , 0, callback, (void*)'?' );

	addTooltipBind(tt, "@" , state, XK_at          , 0, callback, (void*)'@' );
	addTooltipBind(tt, "A" , state, XK_A           , 0, callback, (void*)'A' );
	addTooltipBind(tt, "B" , state, XK_B           , 0, callback, (void*)'B' );
	addTooltipBind(tt, "C" , state, XK_C           , 0, callback, (void*)'C' );
	addTooltipBind(tt, "D" , state, XK_D           , 0, callback, (void*)'D' );
	addTooltipBind(tt, "E" , state, XK_E           , 0, callback, (void*)'E' );
	addTooltipBind(tt, "F" , state, XK_F           , 0, callback, (void*)'F' );
	addTooltipBind(tt, "G" , state, XK_G           , 0, callback, (void*)'G' );
	addTooltipBind(tt, "H" , state, XK_H           , 0, callback, (void*)'H' );
	addTooltipBind(tt, "I" , state, XK_I           , 0, callback, (void*)'I' );
	addTooltipBind(tt, "J" , state, XK_J           , 0, callback, (void*)'J' );
	addTooltipBind(tt, "K" , state, XK_K           , 0, callback, (void*)'K' );
	addTooltipBind(tt, "L" , state, XK_L           , 0, callback, (void*)'L' );
	addTooltipBind(tt, "M" , state, XK_M           , 0, callback, (void*)'M' );
	addTooltipBind(tt, "N" , state, XK_N           , 0, callback, (void*)'N' );
	addTooltipBind(tt, "O" , state, XK_O           , 0, callback, (void*)'O' );
	addTooltipBind(tt, "P" , state, XK_P           , 0, callback, (void*)'P' );
	addTooltipBind(tt, "Q" , state, XK_Q           , 0, callback, (void*)'Q' );
	addTooltipBind(tt, "R" , state, XK_R           , 0, callback, (void*)'R' );
	addTooltipBind(tt, "S" , state, XK_S           , 0, callback, (void*)'S' );
	addTooltipBind(tt, "T" , state, XK_T           , 0, callback, (void*)'T' );
	addTooltipBind(tt, "U" , state, XK_U           , 0, callback, (void*)'U' );
	addTooltipBind(tt, "V" , state, XK_V           , 0, callback, (void*)'V' );
	addTooltipBind(tt, "W" , state, XK_W           , 0, callback, (void*)'W' );
	addTooltipBind(tt, "X" , state, XK_X           , 0, callback, (void*)'X' );
	addTooltipBind(tt, "Y" , state, XK_Y           , 0, callback, (void*)'Y' );
	addTooltipBind(tt, "Z" , state, XK_Z           , 0, callback, (void*)'Z' );
	addTooltipBind(tt, "[" , state, XK_bracketleft , 0, callback, (void*)'[' );
	addTooltipBind(tt, "\\", state, XK_backslash   , 0, callback, (void*)'\\');
	addTooltipBind(tt, "]" , state, XK_bracketright, 0, callback, (void*)']' );
	addTooltipBind(tt, "^" , state, XK_asciicircum , 0, callback, (void*)'^' );
	addTooltipBind(tt, "_" , state, XK_underscore  , 0, callback, (void*)'_' );
	addTooltipBind(tt, "`" , state, XK_grave       , 0, callback, (void*)'`' );
	addTooltipBind(tt, "a" , state, XK_a           , 0, callback, (void*)'a' );
	addTooltipBind(tt, "b" , state, XK_b           , 0, callback, (void*)'b' );
	addTooltipBind(tt, "c" , state, XK_c           , 0, callback, (void*)'c' );
	addTooltipBind(tt, "d" , state, XK_d           , 0, callback, (void*)'d' );
	addTooltipBind(tt, "e" , state, XK_e           , 0, callback, (void*)'e' );
	addTooltipBind(tt, "f" , state, XK_f           , 0, callback, (void*)'f' );
	addTooltipBind(tt, "g" , state, XK_g           , 0, callback, (void*)'g' );
	addTooltipBind(tt, "h" , state, XK_h           , 0, callback, (void*)'h' );
	addTooltipBind(tt, "i" , state, XK_i           , 0, callback, (void*)'i' );
	addTooltipBind(tt, "j" , state, XK_j           , 0, callback, (void*)'j' );
	addTooltipBind(tt, "k" , state, XK_k           , 0, callback, (void*)'k' );
	addTooltipBind(tt, "l" , state, XK_l           , 0, callback, (void*)'l' );
	addTooltipBind(tt, "m" , state, XK_m           , 0, callback, (void*)'m' );
	addTooltipBind(tt, "n" , state, XK_n           , 0, callback, (void*)'n' );
	addTooltipBind(tt, "o" , state, XK_o           , 0, callback, (void*)'o' );
	addTooltipBind(tt, "p" , state, XK_p           , 0, callback, (void*)'p' );
	addTooltipBind(tt, "q" , state, XK_q           , 0, callback, (void*)'q' );
	addTooltipBind(tt, "r" , state, XK_r           , 0, callback, (void*)'r' );
	addTooltipBind(tt, "s" , state, XK_s           , 0, callback, (void*)'s' );
	addTooltipBind(tt, "t" , state, XK_t           , 0, callback, (void*)'t' );
	addTooltipBind(tt, "u" , state, XK_u           , 0, callback, (void*)'u' );
	addTooltipBind(tt, "v" , state, XK_v           , 0, callback, (void*)'v' );
	addTooltipBind(tt, "w" , state, XK_w           , 0, callback, (void*)'w' );
	addTooltipBind(tt, "x" , state, XK_x           , 0, callback, (void*)'x' );
	addTooltipBind(tt, "y" , state, XK_y           , 0, callback, (void*)'y' );
	addTooltipBind(tt, "z" , state, XK_z           , 0, callback, (void*)'z' );
	addTooltipBind(tt, "{" , state, XK_braceleft   , 0, callback, (void*)'{' );
	addTooltipBind(tt, "|" , state, XK_bar         , 0, callback, (void*)'|' );
	addTooltipBind(tt, "}" , state, XK_braceright  , 0, callback, (void*)'}' );
	addTooltipBind(tt, "~" , state, XK_asciitilde  , 0, callback, (void*)'~' );
}
