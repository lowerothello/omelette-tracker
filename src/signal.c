/* return true to block the main thread */
bool mainM_SEM(void)
{
	Song *cs; /* TODO: should this be static? */
	switch (p->sem)
	{
		case M_SEM_RELOAD_REQ:
		case M_SEM_SWAPINST_REQ:
			return 1;
		case M_SEM_RELOAD:
			cs = readSong(w->newfilename);
			if (cs) { delSong(s); s = cs; }
			p->s = s;
			w->trackerfy = STATE_ROWS;
			p->sem = M_SEM_OK;
			break;
		case M_SEM_CALLBACK:
			if (p->semcallback)
				p->semcallback(p->semcallbackarg);
			p->semcallback = p->semcallbackarg = NULL;
			p->sem = M_SEM_OK;
			break;
	}
	return 0;
}

/* return true to skip processing entirely */
bool processM_SEM(void)
{
	void *hold;
	switch (p->sem)
	{
		case M_SEM_RELOAD_REQ: /* blocking */
			p->sem = M_SEM_RELOAD;
		case M_SEM_RELOAD:
			return 1;
		case M_SEM_SWAPINST_REQ: /* non-blocking */
			hold = p->s->instrument;
			p->s->instrument = p->semarg;
			p->semarg = hold;
			p->sem = M_SEM_CALLBACK;
			break;
	}
	return 0;
}
