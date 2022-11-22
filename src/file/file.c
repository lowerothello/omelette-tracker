/* caller should free the returned value */
char *fileExtension(char *path, char *ext)
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

#include "song.c"
