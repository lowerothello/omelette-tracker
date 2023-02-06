#define COMMAND_LENGTH 512 /* TODO: should be dynamically allocated */
#define COMMAND_HISTORY_LENGTH 32 /* TODO: should probably be dynamically allocated */

typedef struct {
	short historyc; /* count of history entries */
	char historyv[COMMAND_HISTORY_LENGTH][COMMAND_LENGTH + 1]; /* history entries */

	short          history;                          /* current point in history */
	unsigned short commandptr;                       /* command char */
	char           error[COMMAND_LENGTH + 1];        /* visual error code */
	char           prompt[COMMAND_LENGTH + 1];       /* prompt */
	bool          (*callback)(char*, enum _Mode*);   /* (command, mode) mode is *window->mode */
	void          (*keycallback)(char*);             /* (text) */
	void          (*tabcallback)(char*);             /* (text) */
} Command;

void wordSplit(char *output, char *line, int wordt);

void setCommand(Command*,
		bool (*callback)(char*, enum _Mode*),
		void (*keycallback)(char*),
		void (*tabcallback)(char*),
		char historyenabled,
		char *prompt,
		char *startvalue);

void drawCommand(Command*, enum _Mode mode);

void initCommandInput(void);
