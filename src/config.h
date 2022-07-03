/* filebrowser starting dir */
#define SAMPLES_DIR "/media/prod/samples"

/* new file bpm */
#define DEF_BPM 120

/* RAMPING */
/* how long it takes for notes to fade in/out */
/* anything above ~300 risks overflowing at high sample rates,
 * idk why you'd ever want to use anything over ~100 though */
#define RAMP_MS 3 /* usual ramp time */
#define LOOP_RAMP_MS 80 /* loop crossfade time, will auto-lower to half the loop range */
#define TIMESTRETCH_RAMP_MS 5 /* timestretch ramp time */

/* how many lines the wheel should scroll */
#define WHEEL_SPEED 3

/* loop used when (loop[1] && (loop[0] == loop[1])) */
#define MIN_LOOP_MS 1

/* screenwidth * screenheight * WAVEFORM_OVERSAMPLING */
#define WAVEFORM_OVERSAMPLING 20
