/* replace *find with *replace in *s */
/* only replaces the first instance of *find */
void strrep(char *string, char *find, char *replace)
{
	char *buffer = malloc(strlen(string) + 1);
	char *pos = strstr(string, find);
	if (pos)
	{
		strcpy(buffer, string);
		size_t len = pos - string + strlen(find);
		memmove(buffer, buffer+len, strlen(buffer) - len + 1);

		string[pos - string] = '\0';
		strcat(string, replace);

		strcat(string, buffer);
	}
	free(buffer);
}




void copyPattern(pattern *dest, pattern *src)
{
	dest->rowc = src->rowc;
	memcpy(dest->rowcc, src->rowcc, sizeof(uint8_t) * CHANNEL_MAX);
	memcpy(dest->rowv, src->rowv, sizeof(row) * CHANNEL_MAX * ROW_MAX);
}
void _delPattern(pattern *pv)
{
	for (int i = 0; i < 128; i++)
	{
		if (pv->history[i])
		{
			free(pv->history[i]);
			pv->history[i] = NULL;
		}
	}
}
void pushPatternHistory(pattern *pv)
{
	if (!pv) return;

	if (pv->historyptr == 255) pv->historyptr = 128;
	else                       pv->historyptr++;

	if (pv->historybehind) pv->historybehind--;
	if (pv->historyahead) pv->historyahead = 0;

	if (pv->history[pv->historyptr%128])
	{
		_delPattern(pv->history[pv->historyptr%128]);
		free(pv->history[pv->historyptr%128]);
		pv->history[pv->historyptr%128] = NULL;
	}
	pv->history[pv->historyptr%128] = calloc(1, sizeof(pattern));
	copyPattern(pv->history[pv->historyptr%128], pv);

	pv->historychannel[pv->historyptr%128] = w->channel;
	pv->historyfy[pv->historyptr%128] = w->trackerfy;
	pv->historyfx[pv->historyptr%128] = w->trackerfx;
}
void pushPatternHistoryIfNew(pattern *pv)
{
	if (!pv) return;
	pattern *pvh = pv->history[pv->historyptr%128];
	if (!pvh) return;

	if (pvh->rowc != pv->rowc
			|| memcmp(pvh->rowcc, pv->rowcc, sizeof(uint8_t) * CHANNEL_MAX)
			|| memcmp(pvh->rowv, pv->rowv, sizeof(row) * CHANNEL_MAX * ROW_MAX))
		pushPatternHistory(pv);
}

void popPatternHistory(uint8_t realindex) /* undo */
{
	pattern *pv = s->patternv[realindex];
	if (!pv) return;

	if (pv->historyptr <= 1 || pv->historybehind >= 127)
	{ strcpy(w->command.error, "already at oldest change"); return; }
	pushPatternHistoryIfNew(pv);

	if (pv->historyptr == 128) pv->historyptr = 255;
	else                       pv->historyptr--;

	copyPattern(pv, pv->history[pv->historyptr%128]);
	w->channel = pv->historychannel[pv->historyptr%128];
	w->trackerfy = pv->historyfy[pv->historyptr%128];
	w->trackerfx = pv->historyfx[pv->historyptr%128];

	pv->historybehind++;
	pv->historyahead++;
}
void unpopPatternHistory(uint8_t realindex) /* redo */
{
	pattern *pv = s->patternv[realindex];
	if (!pv) return;
	if (pv->historyahead == 0)
	{ strcpy(w->command.error, "already at newest change"); return; }
	pushPatternHistoryIfNew(pv);

	if (pv->historyptr == 255)
		pv->historyptr = 128;
	else
		pv->historyptr++;

	copyPattern(pv, pv->history[pv->historyptr%128]);
	w->channel = pv->historychannel[pv->historyptr%128];
	w->trackerfy = pv->historyfy[pv->historyptr%128];
	w->trackerfx = pv->historyfx[pv->historyptr%128];

	pv->historybehind--;
	pv->historyahead--;
}

void renderPatternChannel(pattern *pv, uint8_t channel, uint16_t count)
{
	int max = pv->rowcc[channel];
	if (count) pv->rowcc[channel] = count - 1;
	else       pv->rowcc[channel] = pv->rowc;
	while (max < pv->rowcc[channel])
		if (max < (ROW_MAX>>1)-1)
		{
			memcpy(&pv->rowv[channel][max + 1], pv->rowv[channel],
					sizeof(row) * (max + 1));
			max = (max + 1) * 2 - 1;
		} else
		{
			memcpy(&pv->rowv[channel][max + 1], pv->rowv[channel],
					sizeof(row) * ((ROW_MAX-1) - max));
			max = (ROW_MAX-1); break;
		}
}

