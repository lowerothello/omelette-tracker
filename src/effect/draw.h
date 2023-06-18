#define MIN_EFFECT_WIDTH 38

#define NULL_EFFECT_HEIGHT 3
#define NULL_EFFECT_TEXT "NO EFFECTS"

/* draw the full effect page */
void drawEffect(void);

/* won't centre properly if multibyte chars are present */
void drawCentreText(short x, short y, short w, const char *text);

void drawAutogenPluginLine(short x, short y, short w,
		short ymin, short ymax,
		const char *name, float *value,
		bool toggled, bool integer,
		float min, float max, float def,
		char *prefix, char *postfix,
		uint32_t scalepointlen, uint32_t scalepointcount);
