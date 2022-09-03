/* filebrowser starting dir */
#define SAMPLES_DIR "/media/prod"

/* draw the samples being output to the background */
#define ENABLE_BACKGROUND 0

/* in nanoseconds, one million nanoseconds per milisecond */
#define UPDATE_DELAY 10000000

/* new file bpm */
#define DEF_BPM 125

/* RAMPING */
/* how long it takes for notes to fade in/out                 */
/* anything above ~300 risks overflowing at high sample rates */
/* retrigger never has any ramping cos it causes artifacts    */
#define RAMP_MS 3 /* usual ramp time */
#define LOOP_RAMP_MS 80 /* loop crossfade time, will auto-lower to half the loop range */
#define TIMESTRETCH_RAMP_MS 15 /* timestretch ramp time */

/* real length of an iv->cyclelength unit */
#define TIMESTRETCH_CYCLE_UNIT_MS 0.005f

/* how many lines the mouse wheel should scroll */
#define WHEEL_SPEED 1

/* how many slices to divide the screen into for arrow left/right */
#define WAVEFORM_COARSE_SLICES 16
/* how many slices to divide the screen into for ctrl+arrow left/right */
#define WAVEFORM_FINE_SLICES 128
/* how many samples to draw per update, set to the uint32 limit to disable lazy drawing */
#define WAVEFORM_LAZY_BLOCK_SIZE 3000

/* how long the instrument indices flashes last for */
#define INSTRUMENT_TRIGGER_FLASH_MS 150

/* approximately how many samples to wait between pitch wheel events */
#define PITCH_WHEEL_SAMPLES 500


/* hardcoded compressor state */
#define COMPRESSOR_ATTACK_S 0.00007f
#define COMPRESSOR_RELEASE_S 0.00014f
#define COMPRESSOR_THRESHOLD 10.0f
