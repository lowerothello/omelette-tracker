void instrumentUpArrow(size_t count)
{
	if (!instrumentSafe(s, w->instrument)) return;
	if (w->showfilebrowser)
		browserUpArrow(fbstate, count);
	else
		decControlCursor(&cc, count*MAX(1, w->count));
	p->redraw = 1;
}
void instrumentDownArrow(size_t count)
{
	if (!instrumentSafe(s, w->instrument)) return;
	if (w->showfilebrowser)
		browserDownArrow(fbstate, count);
	else
		incControlCursor(&cc, count*MAX(1, w->count));
	p->redraw = 1;
}
void instrumentLeftArrow(void)
{
	if (!instrumentSafe(s, w->instrument)) return;
	if (!w->showfilebrowser)
		incControlFieldpointer(&cc);
	p->redraw = 1;
}
void instrumentRightArrow(void)
{
	if (!instrumentSafe(s, w->instrument)) return;
	if (!w->showfilebrowser)
		decControlFieldpointer(&cc);
	p->redraw = 1;
}
void instrumentHome(void)
{
	if (!instrumentSafe(s, w->instrument)) return;
	if (w->showfilebrowser)
		browserHome(fbstate);
	else
		setControlCursor(&cc, 0);
	p->redraw = 1;
}
void instrumentEnd(void)
{
	if (!instrumentSafe(s, w->instrument)) return;
	if (w->showfilebrowser)
		browserEnd(fbstate);
	else
		setControlCursor(&cc, cc.controlc-1);
	p->redraw = 1;
}
void instrumentSampleReturn(void *arg)
{
	if (!instrumentSafe(s, w->instrument)) return;
	if (w->showfilebrowser)
		fbstate->commit(fbstate);
	else
		toggleKeyControl(&cc);
	p->redraw = 1;
}
void instrumentSampleBackspace(void *arg)
{
	if (!instrumentSafe(s, w->instrument)) return;
	if (w->showfilebrowser)
		filebrowserBackspace(fbstate);
	else
		revertKeyControl(&cc);
	p->redraw = 1;
}
void instrumentSamplePreview(void *input)
{
	if (!instrumentSafe(s, w->instrument)) return;
	if (w->showfilebrowser)
		filebrowserPreview(fbstate->data, fbstate->cursor, (size_t)input);
	else
		previewNote((size_t)input, w->instrument);
	p->redraw = 1;
}
