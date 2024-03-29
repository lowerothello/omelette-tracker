/* debugging symbols */

/* DEBUG_LOGS
 *   writes debugging files to the current directory.
 *   these files are appended to and never reset so grow forever. */
// #define DEBUG_LOGS 1

#define DEBUG_DUMMY_SAMPLERATE 44100
#define DEBUG_DUMMY_BUFFERSIZE 256
/* DEBUG_DISABLE_AUDIO_OUTPUT
 *   disables registering the jack client, should allow
 *   running under gdb without stalling jackd.
 *
 *   pretty janky, but good enough for some debugging. */
// #define DEBUG_DISABLE_AUDIO_OUTPUT 1

/* DISABLE_RAW_INPUT
 *   only parse stdin, don't try to aquire raw events
 *   $OML_STDIN can be set in the environment to force this */
#define DISABLE_RAW_INPUT 1

/* NO_MULTITHREADING
 *   executes all track processing in a single thread
 *   instead of spawning a thread for each track. */
// #define NO_MULTITHREADING 1

/* filebrowser starting dir */
#define SAMPLES_DIR "/media/prod"

/* file extension used when writing omelette modules */
#define MODULE_EXTENSION ".omlm"

/* UPDATE_DELAY
 *   how often to poll for input. since omelette needs to be able
 *   to refresh the screen arbitrarily it can't block waiting for
 *   input. in nanoseconds, one million nanoseconds per milisecond. */
#define UPDATE_DELAY 1000000 /* max: 1000fps */

#define WAVEFORM_BLOCK_SIZE 8192
#define WAVEFORM_THREADS 2

/* new file bpm */
#define DEF_BPM 125

/* RAMPING
 *   how long it takes for notes to fade in/out.
 *   anything above ~300 risks overflowing at high sample rates.
 *   retrigger never has any ramping cos it causes artifacts. */
#define RAMP_MS 10 /* usual ramp time */
#define LOOP_RAMP_MS 80 /* loop crossfade time, will auto-lower to half the loop range */

/* real length of an iv->cyclelength unit */
/* I think this is actually in seconds, too lazy to check though */
#define TIMESTRETCH_CYCLE_UNIT_MS 0.005f

/* how many lines the mouse wheel should scroll */
#define WHEEL_SPEED 1

/* how many samples to draw per update, set to the uint32 limit to disable lazy drawing */
#define WAVEFORM_LAZY_BLOCK_SIZE 3000

/* how long the instrument indices flash for when they are triggered */
#define INSTRUMENT_TRIGGER_FLASH_S 0.15

/* how many samples to wait between MIDI pitch wheel events */
#define PITCH_WHEEL_SAMPLES 500


#define PROGRAM_TITLE "omelette tracker"

/* DISABLE_BOX_OUTLINE
 *   skip outlining boxes
 */
// #define DISABLE_BOX_OUTLINE 1

#define PREVIEW_TRACKS 6

/* transient detection envelope follower timings */
#define TRANS_ENV_FOLLOWER_MAX      .3f
#define TRANS_ENV_FOLLOWER_RELTHRES .03f
#define TRANS_ENV_FOLLOWER_SPEED    .1f

/* pow([-1 through +1] + 1, GAIN_STAGE_SLOPE); */
#define GAIN_STAGE_SLOPE 2.0f

#define STARTING_TRACKC 4 /* how many tracks to allocate for new files */
#define DEFAULT_PATTERN_LENGTH 0x0f
