#include <ctype.h>

#define COMMAND_LENGTH 512
#define HISTORY_LENGTH 32


typedef struct
{
	unsigned short historyc; /* count of history entries */
	char historyv[COMMAND_LENGTH][HISTORY_LENGTH]; /* history entries */

	short history; /* current point in history */
	unsigned short commandptr; /* command char */
	char command[COMMAND_LENGTH]; /* current point in history */
	char error[COMMAND_LENGTH]; /* error code */
	char prompt[COMMAND_LENGTH]; /* prompt */
	// uint8_t type; /* type, used for different callback functions */
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

/* update the current point in history */
void updateHistory(command_t *command)
{
	if (command->history < 0) return;
	strcpy(command->historyv[command->historyc % HISTORY_LENGTH], command->command);
}
/* set a new point in history */
void pushHistory(command_t *command)
{
	if (command->history < 0) return;
	command->historyc++;

	/* protect against reaching the short limit */
	if (command->historyc >= HISTORY_LENGTH * 2)
		command->historyc = HISTORY_LENGTH;
}

/* index is how far back in time to go */
/* data will be filled with the string */
/* data should be of size COMMAND_LENGTH */
int getHistory(command_t *command, unsigned short index)
{
	if (command->historyc < 0) return 1;
	if (index > MIN(command->historyc, HISTORY_LENGTH)) return 1; /* too far back in time */

	strcpy(command->command, command->historyv[(command->historyc - index) % HISTORY_LENGTH]);
	return 0;
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
	strcpy(command->command, startvalue);
	command->commandptr = strlen(startvalue);
}

void drawCommand(command_t *command, unsigned char mode)
{
	if (mode == 255) /* command mode */
	{
		command->error[0] = '\0';
		printf("\033[%d;0H%s%s\033[%d;%dH", ws.ws_row, command->prompt, command->command, ws.ws_row, (command->commandptr + (unsigned short)strlen(command->prompt) + 1) % ws.ws_col);
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
						if (!getHistory(command, command->historyc + 1))
						{ /* if getting the history succeeded */
							command->commandptr = strlen(command->command);
							command->historyc++;
						}
						break;
					case 'B': /* down arrow */
						if (!getHistory(command, command->historyc - 1))
						{ /* if getting the history succeeded */
							command->commandptr = strlen(command->command);
							command->historyc--;
						}
						break;
					case 'D': /* left arrow */
						if (command->commandptr > 0) command->commandptr--;
						break;
					case 'C': /* right arrow */
						if (command->commandptr < strlen(command->command)) command->commandptr++;
						break;
					case 'H': /* home */
						command->commandptr = 0;
						break;
					case '4':
						if (getchar() == '~') /* end */
							command->commandptr = strlen(command->command);
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
		case 10: case 13: /* newline */
			updateHistory(command);
			pushHistory(command);
			*mode = 0;
			if (command->keycallback)
				command->keycallback(command->command);
			if (command->callback)
				if (command->callback(command->command, mode)) return 1; /* exit if the command says to */
			break;
		case 127: /* backspace */
			if (command->commandptr > 0)
			{
				int i;
				for (i = 0; i < strlen(command->command); i++)
				{
					if (command->command[i] != '\0' && i > command->commandptr - 2)
						command->command[i] = command->command[i + 1];
				}
				command->commandptr--;
				updateHistory(command);
				if (command->keycallback)
					command->keycallback(command->command);
			}
			break;
		case 21: /* <C-u> */
			memcpy(command->command, command->command + command->commandptr, COMMAND_LENGTH - command->commandptr);
			command->commandptr = 0;
			updateHistory(command);
			if (command->keycallback)
				command->keycallback(command->command);
			break;
		case 11: /* <C-k> */
			command->command[command->commandptr] = '\0';
			updateHistory(command);
			if (command->keycallback)
				command->keycallback(command->command);
			break;
		default:
			int i;
			for (i = strlen(command->command); i >= 0; i--)
			{
				if (i >= command->commandptr)
					command->command[i + 1] = command->command[i];
				if (i == command->commandptr)
					command->command[i] = input;
			}
			command->commandptr++;
			updateHistory(command);
			if (command->keycallback)
				command->keycallback(command->command);
			break;
	}
	return 0;
}
