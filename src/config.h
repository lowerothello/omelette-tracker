/* filebrowser starting dir */
#define SAMPLES_DIR "/media/prod"

/* draw the samples being output to the background */
#define ENABLE_BACKGROUND 0

/* new file bpm */
#define DEF_BPM 120

/* RAMPING */
/* how long it takes for notes to fade in/out */
/* anything above ~300 risks overflowing at high sample rates */
#define RAMP_MS 3 /* usual ramp time */
#define LOOP_RAMP_MS 80 /* loop crossfade time, will auto-lower to half the loop range */
#define TIMESTRETCH_RAMP_MS 8 /* timestretch ramp time */

/* how many lines the mouse wheel should scroll */
#define WHEEL_SPEED 3

/* screenwidth * screenheight * WAVEFORM_OVERSAMPLING */
#define WAVEFORM_OVERSAMPLING 1

/* size of the oscillator lookup tables */
#define OSCILLATOR_TABLE_LEN 512
