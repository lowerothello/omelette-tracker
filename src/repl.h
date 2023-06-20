#define REPL_LENGTH 512 /* TODO: should be dynamically allocated */
#define REPL_HISTORY_LENGTH 32 /* TODO: should probably be dynamically allocated */

typedef struct {
	short historyc; /* count of history entries */
	char historyv[REPL_HISTORY_LENGTH][REPL_LENGTH + 1]; /* history entries */

	short          history;
	unsigned short reploffset;
	char           error[REPL_LENGTH + 1];
	char           prompt[REPL_LENGTH + 1];
	void          *arg;
	bool    (*callback)(char*, void *arg);
	void (*keycallback)(char*, void *arg);
	void (*tabcallback)(char*, void *arg);
} Repl;

void wordSplit(char *output, char *line, int wordt);

void setRepl(
		bool    (*callback)(char*, void *arg),
		void (*keycallback)(char*, void *arg),
		void (*tabcallback)(char*, void *arg),
		void *arg,
		bool historyenabled,
		char *prompt,
		char *startvalue);

void drawRepl(void);

void initReplInput(void);
