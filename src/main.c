#define _POSIX_C_SOURCE 199309L
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <time.h>
#include <limits.h>
#include <math.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <termios.h>
#include <signal.h>
#include <dirent.h>
#include <libgen.h>
#include <jack/jack.h>
typedef jack_default_audio_sample_t sample_t;
#include <sndfile.h>
#include <lilv/lilv.h>
#include <lv2/units/units.h>
#include <lv2/port-props/port-props.h>


#define MIN(X, Y)   ((X) < (Y) ? (X) : (Y))
#define MAX(X, Y)   ((X) > (Y) ? (X) : (Y))

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


#define LINENO_COLS 5
#define ROW_COLS 17
#define ROW_FIELDS 6
#define SONG_COLS 5

#define ENVELOPE_ATTACK  0.025
#define ENVELOPE_DECAY   0.025
#define ENVELOPE_RELEASE 0.025

#define RAMP_MS 3 /* only up to about 300 is safe */
                  /* any higher risks overflows   */

#define PORTAMENTO_SEMITONES 0.00003 /* max speed: ~1oct a row */

#define TICKS_PER_ROW 16

#define INSTRUMENT_TYPE_COUNT 2
#define MIN_INSTRUMENT_INDEX -2
#define MIN_EFFECT_INDEX 0

#define INSTRUMENT_BODY_COLS 43
#define INSTRUMENT_BODY_ROWS 20
#define INSTRUMENT_TYPE_ROWS 13
#define INSTRUMENT_TYPE_COLS 41


#define RECORD_LENGTH 52920000 /* record length, in samples (600 secs) */

#define M_12_ROOT_2 1.0594630943592953

/* n=0 is the least significant digit */
char hexDigit32(uint32_t a, uint32_t n)
{ return a / pow32(16, n) % 16; }

int DEBUG;

/* prototypes */
int changeDirectory(void);
void redraw(void);
void startPlayback(void);
void stopPlayback(void);

#include "command.c"
#include "structures.c" /* declares typetable *t */

song *s;
window *w;

int ifMacro(row, char); /* (row, char) */

#include "input.c"

#include "effect.c"
#include "instrument.c"
#include "tracker.c" /* instrument dependancy */

#include "process.c" /* instrument dependancy */

playbackinfo *p;

jack_client_t *client;


void redraw(void)
{
	printf("\033[2J");
	printf("\033[0;0H%d", DEBUG);

	if (ws.ws_row < INSTRUMENT_BODY_ROWS + 2 || ws.ws_col < 43)
	{
		printf("\033[%d;%dH%s", w->centre, (ws.ws_col - (unsigned short)strlen("(terminal too small)")) / 2, "(terminal too small)");
		return;
	}

	trackerRedraw();
	instrumentRedraw();
	effectRedraw();
	drawCommand(&w->command, w->mode);

	fflush(stdout);
}

int commandCallback(char *command, unsigned char *mode)
{
	char *buffer = malloc(strlen(command) + 1);
	wordSplit(buffer, command, 0);
	if      (!strcmp(buffer, "q"))  { free(buffer); buffer = NULL; return 1; }
	else if (!strcmp(buffer, "q!")) { free(buffer); buffer = NULL; return 1; }
	else if (!strcmp(buffer, "w"))  { wordSplit(buffer, command, 1); writeSong(s, w, buffer); }
	else if (!strcmp(buffer, "wq"))
	{
		wordSplit(buffer, command, 1);
		if (!writeSong(s, w, buffer)) { free(buffer); buffer = NULL; return 1; } /* exit if writing the file succeeded */
	}
	else if (!strcmp(buffer, "e"))  { wordSplit(buffer, command, 1); s = readSong(s, w, t, buffer); w->songfx = 0; }

	free(buffer); buffer = NULL;
	return 0;
}

void startPlayback(void)
{
	s->songp = w->songfx;
	s->songr = 0;
	w->trackerfy = 0;
	s->playing = PLAYING_START;
	redraw();
}
void stopPlayback(void)
{
	if (s->playing)
	{
		if (w->instrumentrecv == INST_REC_LOCK_CONT)
			w->instrumentrecv = INST_REC_LOCK_PREP_END;
		s->playing = PLAYING_PREP_STOP;
	} else w->trackerfy = 0;
	redraw();
}

int input(void)
{
	int input;
	while (1)
	{
		input = getchar(); /* read a byte from stdin if it's available */
		if (input < 0) break;
		DEBUG = input;

		if (w->mode == 255) /* command */
		{
			if (commandInput(&w->command, input, &w->mode)) return 1;
			redraw();
		} else if (input == ':') /* enter command mode */
		{
			setCommand(&w->command, &commandCallback, NULL, 1, ":", "");
			w->mode = 255;
			redraw();
		} else if (input == 7) /* ^G, show file info */
		{
			if (strlen(w->filepath))
				strcpy(w->command.error, w->filepath);
			else
				strcpy(w->command.error, "No file loaded");
			redraw();
		} else switch (w->popup)
		{
			case 0:          trackerInput(input);     break;
			case 1: case 2:  instrumentInput(input);  break;
			case 3: case 4:  effectInput(input);      break;
		}
	}
	return 0;
}

/* window->dirpath should be set to the path */
int changeDirectory(void)
{
	if (w->dir != NULL) closedir(w->dir);

	/* avoid paths that start with "//" */
	if (w->dirpath[1] == '/')
	{
		char tmp[NAME_MAX];
		memcpy(tmp, w->dirpath+1, NAME_MAX);
		memcpy(w->dirpath, tmp, NAME_MAX);
	}

	w->dir = opendir(w->dirpath);
	if (w->dir == NULL)
	{
		strcpy(w->command.error, "failed to open directory");
		return 1; /* failed, probably because window->dir doesn't point to a dir */
	}

	struct dirent *dirent = readdir(w->dir);
	w->dirc = 0;
	w->dirmaxwidth = 0;
	while (dirent != NULL)
	{
		if (strcmp(dirent->d_name, ".") && strcmp(dirent->d_name, ".."))
		{
			if (strlen(dirent->d_name) > w->dirmaxwidth)
				w->dirmaxwidth = strlen(dirent->d_name);
			w->dirc++;
		}
		dirent = readdir(w->dir);
	}
	rewinddir(w->dir);
	w->dirmaxwidth = MIN(w->dirmaxwidth, INSTRUMENT_BODY_COLS - 4);
	return 0;
}

void resize(int)
{
	ioctl(1, TIOCGWINSZ, &ws);
	w->visiblechannels =          (ws.ws_col - LINENO_COLS - 2) / ROW_COLS;
	w->trackercelloffset =       ((ws.ws_col - LINENO_COLS - 2) % ROW_COLS) / 2 + 1;
	w->instrumentcelloffset =     (ws.ws_col - INSTRUMENT_BODY_COLS) / 2 + 1;
	w->instrumentrowoffset =      (ws.ws_row - INSTRUMENT_BODY_ROWS - 6) / 2 + 4;
	w->songvisible =               ws.ws_col / SONG_COLS;
	w->songcelloffset =           (ws.ws_col % SONG_COLS) / 2 + 1;
	w->centre =                    ws.ws_row / 2;
	redraw();
	signal(SIGWINCH, &resize); /* not sure why this needs to be redefined every time */
}

void common_cleanup(int ret)
{
	fcntl(0, F_SETFL, fl & ~O_NONBLOCK); /* reset to blocking stdin reads */
	tcsetattr(1, TCSANOW, &origterm); /* reset to the original termios */
	printf("\033[?1002l"); /* disable mouse */
	printf("\033[?1049l"); /* reset to the front buffer */

	exit(ret);
}
void cleanup(int ret)
{
	free(t);
	if (w->dir) closedir(w->dir);
	if (w->search) free(w->search);
	jack_deactivate(client);
	jack_client_close(client);

	free(w->previewinstrument.state);
	free(w->pluginlist);

	free(w);
	delSong(s);
	free(p);

	lilv_node_free(lv2.inputport);
	lilv_node_free(lv2.outputport);
	lilv_node_free(lv2.audioport);
	lilv_node_free(lv2.controlport);
	lilv_node_free(lv2.integer);
	lilv_node_free(lv2.toggled);
	lilv_node_free(lv2.render);
	lilv_node_free(lv2.samplerate);
	lilv_node_free(lv2.unit);
	lilv_node_free(lv2.enumeration);
	lilv_node_free(lv2.logarithmic);
	lilv_node_free(lv2.rangesteps);
	lilv_world_free(lv2.world);

	common_cleanup(ret);
}

int main(int argc, char **argv)
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

	signal(SIGTERM, &cleanup);


	/* jack stuffs */
	p = malloc(sizeof(playbackinfo));
	if (!p)
	{
		printf("out of memory");
		common_cleanup(1);
	}
	memset(p, 0, sizeof(playbackinfo));
	client = jack_client_open("omutrack", JackNullOption, NULL);
	if (client == NULL)
	{
		printf("failed to init the jack client");
		free(p);
		common_cleanup(1);
	}

	p->inl = jack_port_register(client, "in_l", JACK_DEFAULT_AUDIO_TYPE, JackPortIsInput, 0);
	p->inr = jack_port_register(client, "in_r", JACK_DEFAULT_AUDIO_TYPE, JackPortIsInput, 0);

	p->outl = jack_port_register(client, "out_l", JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0);
	p->outr = jack_port_register(client, "out_r", JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0);

	samplerate = jack_get_sample_rate(client);
	buffersize = jack_get_buffer_size(client);

	w = calloc(1, sizeof(window));
	if (w == NULL)
	{
		printf("out of memory");
		free(p);
		common_cleanup(1);
	}
	w->octave = 4;

	s = addSong(s, w);
	if (s == NULL)
	{
		printf("out of memory");
		free(w);
		free(p);
		common_cleanup(1);
	}

	p->s = s;
	p->w = w;


	jack_set_process_callback(client, process, p);
	jack_activate(client);


	w->previewchannel.gain = 255;
	w->previewinstrument.type = 0;
	w->previewinstrument.state = malloc(sizeof(sampler_state));
	if (!w->previewinstrument.state)
	{
		printf("out of memory");
		jack_deactivate(client);
		jack_client_close(client);

		free(w);
		delSong(s);
		free(p);

		common_cleanup(1);
	}
	sampler_state *ss = w->previewinstrument.state;
	ss->volume.s = 255;


	addPattern(s, w, 0, 0);
	s->songi[0] = 0;


	getcwd(w->dirpath, sizeof(w->dirpath));
	changeDirectory();


	t = calloc(1, sizeof(typetable));
	if (t == NULL)
	{
		printf("out of memory");

		if (w->dir != NULL) closedir(w->dir);
		jack_deactivate(client);
		jack_client_close(client);

		free(w->previewinstrument.state);

		free(w);
		delSong(s);
		free(p);

		common_cleanup(1);
	}
	initInstrumentTypes();


	/* lv2 */
	lv2.world = lilv_world_new();
	lilv_world_load_all(lv2.world);
	lv2.plugins = (LilvPlugins *)lilv_world_get_all_plugins(lv2.world);
	lv2.pluginc = lilv_plugins_size(lv2.plugins);
	w->pluginlist = malloc((INSTRUMENT_TYPE_COLS - 2) * lv2.pluginc);
	const LilvPlugin *plugin;
	LilvNode *n;
	unsigned int xc = 0;
	LILV_FOREACH(plugins, i, lv2.plugins)
	{
		plugin = lilv_plugins_get(lv2.plugins, i);
		n = lilv_plugin_get_name(plugin);
		strcpy(w->pluginlist[xc], lilv_node_as_string(n));
		lilv_node_free(n);
		xc++;
	}

	lv2.inputport =   lilv_new_uri(lv2.world, LV2_CORE__InputPort);
	lv2.outputport =  lilv_new_uri(lv2.world, LV2_CORE__OutputPort);
	lv2.audioport =   lilv_new_uri(lv2.world, LV2_CORE__AudioPort);
	lv2.controlport = lilv_new_uri(lv2.world, LV2_CORE__ControlPort);
	lv2.integer =     lilv_new_uri(lv2.world, LV2_CORE__integer);
	lv2.toggled =     lilv_new_uri(lv2.world, LV2_CORE__toggled);
	lv2.enumeration = lilv_new_uri(lv2.world, LV2_CORE__enumeration);
	lv2.samplerate =  lilv_new_uri(lv2.world, LV2_CORE__sampleRate);
	lv2.render =      lilv_new_uri(lv2.world, LV2_UNITS__render);
	lv2.unit =        lilv_new_uri(lv2.world, LV2_UNITS__unit);
	lv2.logarithmic = lilv_new_uri(lv2.world, LV2_PORT_PROPS__logarithmic);
	lv2.rangesteps =  lilv_new_uri(lv2.world, LV2_PORT_PROPS__rangeSteps);


	if (argc > 1)
	{
		song *temp = readSong(s, w, t, argv[1]);
		if (temp) s = temp;
	}

	resize(0);

	/* loop over input */
	struct timespec req;
	int running = 0;
	while(!running)
	{
		running = input();
		// if (p->dirty)
		{
			p->dirty = 0;
			redraw();
		}

		if (w->previewsamplestatus == 3)
		{
			free(w->previewinstrument.sampledata);
			w->previewinstrument.sampledata = NULL;
			w->previewsamplestatus = 0;
		}

		/* perform any pending instrument actions */
		changeInstrumentType(s, w, t, 0);

		/* finish freeing the record buffer */
		if (w->instrumentrecv == INST_REC_LOCK_END)
		{
			if (w->recptr > 0)
			{
				instrument *iv = s->instrumentv[w->instrumentreci];
				if (iv->sampledata)
				{ free(iv->sampledata); iv->sampledata = NULL; }
				iv->sampledata = malloc(w->recptr * sizeof(short));
				if (iv->sampledata == NULL)
				{
					strcpy(w->command.error, "saving recording failed, out of memory");
				} else
				{
					memcpy(iv->sampledata, w->recbuffer, w->recptr * sizeof(short));
					iv->samplelength = w->recptr;

					sampler_state *ss = iv->state;
					ss->channels = 2;
					ss->length = w->recptr / 2;
					ss->c5rate = samplerate;
					ss->trim[0] = 0;
					ss->trim[1] = w->recptr / 2;
					ss->loop[0] = 0;
					ss->loop[1] = 0;
				}
			}

			free(w->recbuffer); w->recbuffer = NULL;
			w->instrumentrecv = INST_REC_LOCK_OK;
		}

		req.tv_sec  = 0; /* nanosleep can set this higher sometimes, so set every cycle */
		req.tv_nsec = 10000000; // wait ~>10ms
		while(nanosleep(&req, &req) < 0);
	}

	cleanup(0);
}
