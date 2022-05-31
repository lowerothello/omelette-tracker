typedef jack_default_audio_sample_t sample_t;


uint8_t min8(uint8_t a, uint8_t b) { return (a > b) ? b : a; }
uint16_t min16(uint16_t a, uint16_t b) { return (a > b) ? b : a; }
uint16_t max16(uint16_t a, uint16_t b) { return (a < b) ? b : a; }
unsigned short minushort(unsigned short a, unsigned short b) { return (a > b) ? b : a; }
unsigned short maxushort(unsigned short a, unsigned short b) { return (a < b) ? b : a; }
unsigned short maxshort(short a, short b) { return (a < b) ? b : a; }
uint32_t min32(uint32_t a, uint32_t b) { return (a > b) ? b : a; }
uint32_t max32(uint32_t a, uint32_t b) { return (a < b) ? b : a; }
uint32_t clamp32(uint32_t x, uint32_t min, uint32_t max)
{
	uint32_t y = (x < max) ? x : max;
	return (y > min) ? y : min;
}
uint32_t pow32(uint32_t a, uint32_t b)
{
	if (b == 0) return 1;
	uint32_t i;
	uint32_t c = a;
	for (i = 1; i < b; i++)
		c = c * a;
	return c;
}


jack_nframes_t samplerate;
jack_nframes_t buffersize;
struct winsize ws;


struct termios term, origterm;
int fl;
void common_init(void)
{
	printf("\033[?1049h"); /* switch to the back buffer */
	printf("\033[?1002h"); /* enable mouse events */

	tcgetattr(1, &term);
	origterm = term;
	term.c_lflag &= (~ECHO & ~ICANON);
	tcsetattr(1, TCSANOW, &term); /* disable ECHO and ICANON */

	fl = fcntl(0, F_SETFL, O_NONBLOCK); /* non-blocking stdin reads */

	/* ignore sigint to prevent habitual <C-c> breaking stuff */
	signal(SIGINT, SIG_IGN);
}

void common_cleanup(int ret)
{
	fcntl(0, F_SETFL, fl & ~O_NONBLOCK); /* reset to blocking stdin reads */
	tcsetattr(1, TCSANOW, &origterm); /* reset to the original termios */
	printf("\033[?1002l"); /* disable mouse */
	printf("\033[?1049l"); /* reset to the front buffer */

	exit(ret);
}
