#define COMMAND_LENGTH 512 /* TODO: should be dynamically allocated */
#define COMMAND_HISTORY_LENGTH 32 /* TODO: should probably be dynamically allocated */

typedef struct {
	short historyc; /* count of history entries */
	char historyv[COMMAND_HISTORY_LENGTH][COMMAND_LENGTH + 1]; /* history entries */

	short          history;
	unsigned short commandptr;
	char           error[COMMAND_LENGTH + 1];
	char           prompt[COMMAND_LENGTH + 1];
	void          *arg;
	bool    (*callback)(char*, void *arg);
	void (*keycallback)(char*, void *arg);
	void (*tabcallback)(char*, void *arg);
} Command;

void wordSplit(char *output, char *line, int wordt);

void setCommand(
		bool    (*callback)(char*, void *arg),
		void (*keycallback)(char*, void *arg),
		void (*tabcallback)(char*, void *arg),
		void *arg,
		bool historyenabled,
		char *prompt,
		char *startvalue);

void drawCommand(void);

void initCommandInput(void);
