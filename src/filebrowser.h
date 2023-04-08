BrowserState *fbstate;

void freePreviewSample(void);
BrowserState *initFileBrowser(char *path);
void freeFileBrowser(BrowserState *b);

void fileBrowserBackspace(BrowserState *b);
void fileBrowserPreview(BrowserState *b, size_t note, bool release);

char *humanReadableSize(double bytes, char *buffer);

static char *fileBrowserSearchLine(void *data);
