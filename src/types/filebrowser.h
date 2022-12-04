BrowserState *fbstate;

void freePreviewSample(void);
BrowserState *initFileBrowser(char *path, void (*callback)(char*));
void freeFileBrowser(BrowserState *b);

void fileBrowserBackspace(BrowserState *b);
void fileBrowserPreview(BrowserState *b, int input);
