#include <ctype.h>

#define COMMAND_LENGTH 512
#define HISTORY_LENGTH 32


typedef struct
{
	short historyc; /* count of history entries */
	char historyv[HISTORY_LENGTH][COMMAND_LENGTH + 1]; /* history entries */

	short history; /* current point in history */
	unsigned short commandptr; /* command char */
	char error[COMMAND_LENGTH + 1]; /* error code */
	char prompt[COMMAND_LENGTH + 1]; /* prompt */
	int (*callback)(char *, unsigned char *); /* (command, mode) mode is *window->mode */
	void (*keycallback)(char *); /* (command) */
} command_t;

void wordSplit(char *output, char *line, int wordt)
{
	int wordc = 0;
	size_t p;
	strcpy(output, "");
	char lastwhitespace = 0;

	for (size_t i = 0; i < strlen(line); i++)
	{
		if (isspace(line[i]))
		{
			if (!lastwhitespace)
				if (wordc++ > wordt) break;
			lastwhitespace = 1;
		} else
		{
			if (wordc == wordt)
			{
				p = strlen(output);
				output[p + 1] = '\0';
				output[p + 0] = line[i];
			}
			lastwhitespace = 0;
		}
	}
}

void setCommand(command_t *command, int (*callback)(char *, unsigned char *), void (*keycallback)(char *), char historyenabled, char *prompt, char *startvalue)
{
	command->callback = callback;
	command->keycallback = keycallback;
	if (historyenabled)
		command->history = 0;
	else
		command->history = -1;

	strcpy(command->prompt, prompt);
	strcpy(command->historyv[command->historyc], startvalue);
	command->commandptr = strlen(startvalue);
}

void drawCommand(command_t *command, unsigned char mode)
{
	if (mode == 255) /* command mode */
	{
		printf("\033[%d;0H%s%s\033[%d;%dH", ws.ws_row, command->prompt, command->historyv[command->historyc], ws.ws_row, (command->commandptr + (unsigned short)strlen(command->prompt) + 1) % ws.ws_col);
		command->error[0] = '\0';
	} else if (strlen(command->error) > 0)
	{
		printf("\033[s\033[%d;0H%s\033[u", ws.ws_row, command->error);
	}
}

char buffer[COMMAND_LENGTH];
int commandInput(command_t *command, int input, unsigned char *mode)
{
	switch (input)
	{
		case '\033':
			if (getchar() == '[')
			{
				switch (getchar())
				{
					case 'A': /* up arrow */
						if (command->history < 0) break;
						command->history = MIN((command->history + 1) % HISTORY_LENGTH, command->historyc);
						strcpy(command->historyv[command->historyc], command->historyv[command->historyc - command->history]);
						command->commandptr = strlen(command->historyv[command->historyc]);
						break;
					case 'B': /* down arrow */
						if (command->history < 0) break;
						command->history = MAX(command->history - 1, 0);
						strcpy(command->historyv[command->historyc], command->historyv[command->historyc - command->history]);
						command->commandptr = strlen(command->historyv[command->historyc]);
						break;
					case 'D': /* left arrow */
						if (command->commandptr > 0) command->commandptr--;
						break;
					case 'C': /* right arrow */
						if (command->commandptr < strlen(command->historyv[command->historyc]))
							command->commandptr++;
						break;
					case 'H': /* home */
						command->commandptr = 0;
						break;
					case '4': /* end */
						getchar();
						command->commandptr = strlen(command->historyv[command->historyc]);
						break;
					case 'M': /* mouse */
						getchar();
						getchar();
						getchar();
						break;
				}
				break;
			} else /* assume escape */
			{
				*mode = 0;
				break;
			}
		case 10: case 13: /* return */
			*mode = 0;

			if (strcmp(command->historyv[command->historyc], ""))
			{
				if (command->keycallback) command->keycallback(command->historyv[command->historyc]);
				if (command->callback) if (command->callback(command->historyv[command->historyc], mode)) return 1;

				if (command->history < 0) break;
				command->historyc++;

				/* protect against reaching the short limit */
				if (command->historyc >= HISTORY_LENGTH * 2)
					command->historyc = HISTORY_LENGTH;
			}
			break;
		case 127: /* backspace */
			if (command->commandptr > 0)
			{
				for (int i = 0; i < strlen(command->historyv[command->historyc]); i++)
					if (command->historyv[command->historyc][i] != '\0' && i > command->commandptr - 2)
						command->historyv[command->historyc][i] = command->historyv[command->historyc][i + 1];
				command->commandptr--;
				if (command->keycallback) command->keycallback(command->historyv[command->historyc]);
			}
			break;
		case 21: /* <C-u> */
			memcpy(command->historyv[command->historyc], command->historyv[command->historyc] + command->commandptr, COMMAND_LENGTH - command->commandptr);
			command->commandptr = 0;
			if (command->keycallback) command->keycallback(command->historyv[command->historyc]);
			break;
		case 11: /* <C-k> */
			command->historyv[command->historyc][command->commandptr] = '\0';
			if (command->keycallback) command->keycallback(command->historyv[command->historyc]);
			break;
		default:
			command->historyv[command->historyc][strlen(command->historyv[command->historyc]) + 1] = '\0';
			for (int i = strlen(command->historyv[command->historyc]); i >= 0; i--)
			{
				if      (i >  command->commandptr) command->historyv[command->historyc][i + 1] = command->historyv[command->historyc][i];
				else if (i == command->commandptr) command->historyv[command->historyc][i] = input;
				else break;
			}
			command->commandptr++;
			if (command->keycallback) command->keycallback(command->historyv[command->historyc]);
			break;
	}
	return 0;
}
