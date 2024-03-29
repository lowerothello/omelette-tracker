typedef struct _BrowserState {
	short    x, y, w, h; /* screen region to draw at   */
	void    *data;       /* data passed to callbacks   */
	uint32_t cursor;     /* actual cursor y pos        */
	short    fyoffset;   /* staged cursor y pos offset */
	char    *search;

	char*        (*getTitle)(void *data);                   /* REQ: init data for drawing and return the title, caller must free the result     */
	bool          (*getNext)(void *data);                   /* REQ: walk to the next line, returns whether the new line is valid                */
	void         (*drawLine)(struct _BrowserState*, int y); /* REQ: draw the current line, passed through a BrowserState for xywh geometry      */
	char*      (*searchLine)(void *data);                   /* REQ: returns the search string for the current line, caller must free the result */
	uint32_t (*getLineCount)(void *data);                   /* REQ: get the number of lines                                                     */
	void         (*cursorCB)(void *data);                   /* OPT: callback when the cursor is moved                                           */
	void           (*commit)(struct _BrowserState*);        /* REQ: selection callback                                                          */
} BrowserState;

void resizeBrowser(BrowserState*, short x, short y, short w, short h);
void browserFree(BrowserState*);

void drawBrowser(BrowserState*);

void browserMouse(BrowserState*, enum Button button, int x, int y);
void addBrowserBinds(BrowserState*);
void addBrowserSearchBinds(BrowserState*);

#include "browser.c"
