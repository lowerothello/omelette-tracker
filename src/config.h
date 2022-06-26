/* filebrowser starting dir */
#define SAMPLES_DIR "/media/prod/samples"

/* new file bpm */
#define DEF_BPM 120

/* RAMPING */
/* how long it takes for notes to fade in/out */
/* anything above ~300 risks overflowing at high sample rates,
 * idk why you'd ever want to use anything over ~100 though */
#define RAMP_MS 5 /* usual ramp time */
#define LOOP_RAMP_MS 60 /* loop crossfade time, will auto-lower to half the loop range */
#define TIMESTRETCH_RAMP_MS 12 /* timestretch ramp time */

/* how many lines the wheel should scroll */
#define WHEEL_SPEED 3
