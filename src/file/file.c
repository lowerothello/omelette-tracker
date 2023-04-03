FILE *checkPath(char *path, char *mode)
{
	FILE *fp = fopen(path, mode);
	if (!fp)
		return NULL;

	DIR *dp = opendir(path);
	if (dp)
	{
		closedir(dp);
		fclose(fp);
		return NULL;
	}

	return fp;
}

/* caller should free the returned value */
static char *fileExtension(char *path, char *ext)
{
	char *ret;
	if (strlen(path) < strlen(ext) || strcmp(path+(strlen(path) - strlen(ext)), ext))
	{
		ret = malloc(strlen(path) + strlen(ext) + 1);
		strcpy(ret, path);
		strcat(ret, ext);
	} else
	{
		ret = malloc(strlen(path) + 1);
		strcpy(ret, path);
	} return ret;
}

int writeSongJson(Song *cs, char *path)
{
	char *pathext = fileExtension(path, MODULE_EXTENSION);
	if (!strcmp(pathext, MODULE_EXTENSION))
	{
		free(pathext);
		if (!strlen(w->filepath))
		{
			strcpy(w->command.error, "no file name");
			p->redraw = 1;
			return 1;
		}
		pathext = malloc(sizeof(w->filepath) + 1);
		strcpy(pathext, w->filepath);
	} else
		strcpy(w->filepath, pathext);

	fcntl(0, F_SETFL, 0); /* blocking */
	FILE *fp = checkPath(pathext, "w");
	if (!fp)
	{
		fcntl(0, F_SETFL, O_NONBLOCK); /* non-blocking */
		snprintf(w->command.error, COMMAND_LENGTH, "failed to open file '%s' for writing", path);
		p->redraw = 1; return 1;
	}

	serializeSong(fp, cs);

	fclose(fp);
	fcntl(0, F_SETFL, O_NONBLOCK); /* non-blocking */
	snprintf(w->command.error, COMMAND_LENGTH, "'%s' written", pathext);
	free(pathext);
	p->redraw = 1;
	return 0;
}

Song *readSongJson(char *path)
{
	fcntl(0, F_SETFL, 0); /* blocking */
	FILE *fp = checkPath(path, "r");
	if (!fp)
	{
		fcntl(0, F_SETFL, O_NONBLOCK); /* non-blocking */
		snprintf(w->command.error, COMMAND_LENGTH, "failed to open file '%s' for reading", path);
		p->redraw = 1;
		return NULL;
	}

	Song *ret = deserializeSong(fp);

	fclose(fp);
	fcntl(0, F_SETFL, O_NONBLOCK); /* non-blocking */
	p->redraw = 1;
	return ret;
}
