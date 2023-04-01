void instUIEmptyCallback(short x, short y, Inst *iv, uint8_t index)
{
	printf("\033[%d;%d%s", y, x, EMPTY_INST_UI_TEXT);
}

static short drawInstIndex(short bx, short minx, short maxx)
{
	Inst *iv;
	const InstAPI *api;
	char buffer[9];
	short x = 0;
	for (int i = 0; i < INSTRUMENT_MAX; i++)
		if (w->centre - w->instrument + i > TRACK_ROW && w->centre - w->instrument + i < ws.ws_row)
		{
			x = bx;

			if (instSafe(s->inst, i))
				if (s->inst->v[s->inst->i[i]].triggerflash)
					printf("\033[3%dm", i%6+1);

			if (w->instrument + w->fyoffset == i) /* TODO: fyoffset can get stuck set sometimes */
				printf("\033[1;7m");

			if (x <= ws.ws_col)
			{
				snprintf(buffer, 4, "%02x ", i);
				printCulling(buffer, x, w->centre - w->instrument + i, minx, maxx);
			} x += 3;

			if (x <= ws.ws_col)
			{
				if (instSafe(s->inst, i))
				{
					iv = &s->inst->v[s->inst->i[i]];
					if ((api = instGetAPI(iv->type)))
						api->getindexinfo(iv, buffer);
				} else snprintf(buffer, 9, "........");
				printCulling("        ", x, w->centre - w->instrument + i, minx, maxx); /* flush with attribute, TODO: is there an escape code to do this in a better way? */
				printCulling(buffer, x + 8 - strlen(buffer), w->centre - w->instrument + i, minx, maxx);
			} x += 9;

			printf("\033[40;37;22;27m");
		}
	return x - bx;
}

short getInstUIRows(const InstUI *iui, short cols)
{
	size_t entryc = iui->count;
	short ret = entryc / cols;

	/* round up instead of down */
	if (entryc%cols)
		ret++;

	return ret;
}
short getInstUICols(const InstUI *iui, short rows)
{
	size_t entryc = iui->count;
	short ret = entryc / rows;

	/* round up instead of down */
	if (entryc%rows)
		ret++;

	return ret;
}
short getMaxInstUICols(const InstUI *iui, short width)
{
	return (width + (iui->padding<<1)) / (iui->width + iui->padding);
}

void drawInstUI(const InstUI *iui, void *callbackarg, short x, short w, short y, short scrolloffset, short rows)
{
	short cols = MIN(getMaxInstUICols(iui, w), getInstUICols(iui, rows));
	x += (w - (cols*(iui->width + iui->padding)) + iui->padding)>>1;
	short cx, cy;
	for (uint8_t i = 0; i < iui->count; i++)
	{
		cx = x + (i/rows)*(iui->width + iui->padding);
		cy = y + i%rows;
		if (cy < ws.ws_row - 1 && cy > scrolloffset)
			iui->callback(cx, cy - scrolloffset, callbackarg, i);
	}
}

void drawInstrument(void)
{
	switch (w->mode)
	{
		case MODE_INSERT:
			if (cc.mouseadjust || cc.keyadjust) printf("\033[%d;0H\033[1m-- INSERT ADJUST --\033[m\033[4 q", ws.ws_row);
			else                                printf("\033[%d;0H\033[1m-- INSERT --\033[m\033[6 q",        ws.ws_row);
			w->command.error[0] = '\0';
			break;
		default:
			if (cc.mouseadjust || cc.keyadjust) { printf("\033[%d;0H\033[1m-- ADJUST --\033[m\033[4 q", ws.ws_row); w->command.error[0] = '\0'; }
			break;
	}

	short minx = 1;
	short maxx = ws.ws_col;
	short x = drawInstIndex(1, minx, maxx) + 2;

	if (instSafe(s->inst, w->instrument))
	{
		clearControls();

		Inst *iv = &s->inst->v[s->inst->i[w->instrument]];
		const InstAPI *api;
		if ((api = instGetAPI(iv->type)))
			api->draw(iv, x, TRACK_ROW+1, ws.ws_col - x, ws.ws_row - 1 - (TRACK_ROW+1), minx, maxx);

		drawControls();
	} else
		drawBrowser(fbstate);
}
