bool ifCommand(Track *cv, Row *r, char m);

/* ifCommand(), but run .callback */
void ifCommandCallback(uint32_t fptr, uint16_t *spr, Track *cv, Row *r, char m, void (*callback)(uint32_t, uint16_t*, int, Track*, Row*));

/* access commands with (Row*)->command[i] */
#define FOR_ROW_COMMANDS(i, cv) for (int i = 0; i < cv->pattern->commandc + 1; i++)

/* if the row needs to be ramped in based on the commands present */
bool ifCommandRamp(Track *cv, Row *r);

bool changeCommand(int input, char *dest);
void addCommandBinds(const char *prettyname, unsigned int state, void (*callback)(void*));

/* there can be up to 6 of these (colours 1-6 (no black/white)) */
enum CommandColour
{
	MC_LOCAL,
	MC_GAIN,
	MC_FILTER,
	MC_RETRIGGER,
	MC_PITCH,
	MC_SEQUENCER,
};

void commandStateReset(CommandState *s);
void commandStateSet(CommandState *s, Command *m);
void commandStateSmooth(CommandState *s, Command *m);
void commandStateApply(CommandState *s);
float commandStateGetMono(CommandState *s, float rp);
void commandStateGetStereo(CommandState *s, float rp, float *l, float *r);

typedef struct CommandAPI
{
	void          (*clear)(Track *cv, void *state);
	void        (*pretrig)(uint32_t fptr, uint16_t *spr, Track *cv, Row *r, void *state);
	void       (*posttrig)(uint32_t fptr, uint16_t *spr, Track *cv, Row *r, void *state);
	void    (*triggernote)(uint32_t fptr, Track *cv, float oldnote, float note, short inst, void *state);
	float     (*samplerow)(uint32_t fptr, uint16_t count, uint16_t *spr, uint16_t sprp, Track *cv, void *state);
	void (*persistenttune)(uint32_t fptr, uint16_t count, uint16_t *spr, uint16_t sprp, Track *cv, void *state);
	void   (*volatiletune)(uint32_t fptr, uint16_t count, uint16_t *spr, uint16_t sprp, Track *cv, float *note, void *state);
	void    (*postsampler)(uint32_t fptr, Track *cv, float rp, float *lf, float *rf, void *state);
	size_t statesize;
} CommandAPI;

void commandCallbackClear(Track *cv);
void commandCallbackPreTrig(uint32_t fptr, uint16_t *spr, Track *cv, Row *r);
void commandCallbackPostTrig(uint32_t fptr, uint16_t *spr, Track *cv, Row *r);
void commandCallbackTriggerNote(uint32_t fptr, Track *cv, float oldnote, float note, short inst);
float commandCallbackSampleRow(uint32_t fptr, uint16_t count, uint16_t *spr, uint16_t sprp, Track *cv);
void commandCallbackPersistent(uint32_t fptr, uint16_t count, uint16_t *spr, uint16_t sprp, Track *cv);
void commandCallbackVolatile(uint32_t fptr, uint16_t count, uint16_t *spr, uint16_t sprp, Track *cv, float *note);
void commandCallbackPostSampler(uint32_t fptr, Track *cv, float rp, float *lf, float *rf);

#include "bpm.c"
#include "row.c"
#include "gain.c"
#include "pitch.c"
// #include "retrigger.c"
#include "chance.c"
#include "filter.c"
#include "midi.c"

CommandAPI global_command_callbacks[] =
{
	{ NULL, commandBpmPreTrig, NULL, NULL, NULL, NULL, NULL, NULL, 0 },
	{ commandRowClear, commandRowPreTrig, NULL, NULL, commandRowSampleRow, NULL, NULL, NULL, sizeof(CommandRowState) },
	{ commandGainClear, commandGainPreTrig, NULL, NULL, NULL, NULL, NULL, commandGainPostSampler, sizeof(CommandState) },
	{ NULL, commandPitchPreTrig, NULL, commandPitchTriggerNote, NULL, commandPitchPersistent, commandPitchVolatile, NULL, sizeof(CommandPitchState) },
	// { commandRetrigClear, NULL, commandRetrigPostTrig, commandRetrigTriggerNote, NULL, NULL, commandRetrigVolatile, NULL, sizeof(CommandRetrigState) },
	{ NULL, commandChancePreTrig, NULL, NULL, NULL, NULL, NULL, NULL, 0 },
	{ commandFilterClear, NULL, commandFilterPostTrig, NULL, NULL, NULL, NULL, commandFilterPostSampler, sizeof(CommandFilterState) },
	{ NULL, NULL, commandInstSamplerPostTrig, commandInstSamplerTriggerNote, NULL, NULL, NULL, NULL, 0 },
	{ NULL, NULL, commandMidiPostTrig, NULL, NULL, NULL, NULL, NULL, 0 },
};
const size_t COMMAND_CALLBACK_MAX = sizeof(global_command_callbacks) / sizeof(global_command_callbacks[0]);

/* not an enum so they can be accessed within commands */
#define MF_STEREO      (1<<0) /* treat both nibbles as equally significant               */
#define MF_RAMP        (1<<1) /* calls to this command cause clicks, ramping needed        */
#define MF_SEQUENCED   (1<<2) /* command only applies to tracks with an inst/note pair     */
#define MF_EXTENDSTATE (1<<3) /* generate a smooth variant, and lfo/inc+dec/jitter binds */


typedef struct CommandDef
{
	bool             set; /* set high if this index is significant */
	const char      *prettyname;
	enum CommandColour colour;
	uint8_t          flags; /* MF declares or'd together */
} CommandDef;


#define COMMAND_MAX 128
const CommandDef global_command_db[COMMAND_MAX] =
{
	[COMMAND_GAIN]                  = { 1, "stereo gain"                 , MC_GAIN     , MF_STEREO|MF_RAMP|MF_EXTENDSTATE },//S~ +- %
	[COMMAND_SMOOTH_GAIN]           = { 1, "smooth stereo gain"          , MC_GAIN     , MF_STEREO|0      |0              },//--
	// [COMMAND_BLOCK_RETRIG]          = { 1, "block retrigger"             , MC_SEQUENCER, 0        |0      |0              },//
	// [COMMAND_REVERSE_BLOCK_RETRIG]  = { 1, "reverse block retrigger"     , MC_SEQUENCER, 0        |0      |0              },//
	// [COMMAND_TICK_RETRIG]           = { 1, "tick retrigger"              , MC_SEQUENCER, 0        |0      |0              },//
	// [COMMAND_REVERSE_TICK_RETRIG]   = { 1, "reverse tick retrigger"      , MC_SEQUENCER, 0        |0      |0              },//
	[COMMAND_PORTAMENTO]            = { 1, "portamento slide"            , MC_PITCH    , 0        |0      |0              },//
	[COMMAND_PITCH_OFFSET]          = { 1, "microtonal pitch offset"     , MC_PITCH    , 0        |0      |MF_EXTENDSTATE },//S~ +- %
	[COMMAND_VIBRATO]               = { 1, "vibrato"                     , MC_PITCH    , 0        |0      |0              },//--
	[COMMAND_BPM]                   = { 1, "bpm"                         , MC_SEQUENCER, 0        |0      |0              },//
	[COMMAND_ROW_CUT]               = { 1, "note cut"                    , MC_SEQUENCER, 0        |0      |0              },//
	[COMMAND_ROW_DELAY]             = { 1, "note delay"                  , MC_SEQUENCER, 0        |0      |0              },//
	[COMMAND_NOTE_CHANCE]           = { 1, "note chance"                 , MC_SEQUENCER, 0        |0      |0              },//
	[COMMAND_FILTER]                = { 1, "stereo cutoff"               , MC_FILTER   , MF_STEREO|MF_RAMP|MF_EXTENDSTATE },//S~ +- %
	[COMMAND_SMOOTH_FILTER]         = { 1, "smooth stereo cutoff"        , MC_FILTER   , MF_STEREO|0      |0              },//--
	[COMMAND_RESONANCE]             = { 1, "stereo resonance"            , MC_FILTER   , MF_STEREO|MF_RAMP|MF_EXTENDSTATE },//S~ +- %
	[COMMAND_SMOOTH_RESONANCE]      = { 1, "smooth stereo resonance"     , MC_FILTER   , MF_STEREO|0      |0              },//--
	[COMMAND_FILTER_MODE]           = { 1, "stereo filter mode"          , MC_FILTER   , MF_STEREO|0      |0              },//S %
	[COMMAND_SMOOTH_FILTER_MODE]    = { 1, "smooth stereo filter mode"   , MC_FILTER   , MF_STEREO|0      |0              },//
	[COMMAND_ATT_DEC]               = { 1, "local att/dec"               , MC_LOCAL    , 0        |0      |0              },//+- %
	[COMMAND_SUS_REL]               = { 1, "local sus/rel"               , MC_LOCAL    , 0        |0      |0              },//+- %
	[COMMAND_PITCH_SHIFT]           = { 1, "local pitch shift"           , MC_PITCH    , 0        |0      |MF_EXTENDSTATE },//S~ +- %
	[COMMAND_SMOOTH_PITCH_SHIFT]    = { 1, "smooth local pitch shift"    , MC_PITCH    , 0        |0      |0              },//--
	[COMMAND_OFFSET]                = { 1, "sample offset"               , MC_LOCAL    , 0        |MF_RAMP|0              },//
	[COMMAND_REVERSE_OFFSET]        = { 1, "reverse sample offset"       , MC_LOCAL    , 0        |MF_RAMP|0              },//
	[COMMAND_OFFSET_JITTER]         = { 1, "sample offset jitter"        , MC_LOCAL    , 0        |MF_RAMP|0              },//
	[COMMAND_REVERSE_OFFSET_JITTER] = { 1, "reverse sample offset jitter", MC_LOCAL    , 0        |MF_RAMP|0              },//
	[COMMAND_PITCH_WIDTH]           = { 1, "pitch width"                 , MC_PITCH    , 0        |0      |MF_EXTENDSTATE },//S~ +- %
	[COMMAND_SMOOTH_PITCH_WIDTH]    = { 1, "smooth pitch width"          , MC_PITCH    , 0        |0      |0              },//--
	[COMMAND_SAMPLERATE]            = { 1, "samplerate"                  , MC_LOCAL    , 0        |0      |MF_EXTENDSTATE },//S~ +- %
	[COMMAND_SMOOTH_SAMPLERATE]     = { 1, "smooth samplerate"           , MC_LOCAL    , 0        |0      |0              },//--
	[COMMAND_CYCLE_LENGTH_HI_BYTE]  = { 1, "cycle length high byte"      , MC_LOCAL    , 0        |0      |MF_EXTENDSTATE },//S~ +- %
	[COMMAND_CYCLE_LENGTH_LO_BYTE]  = { 1, "cycle length low byte"       , MC_LOCAL    , 0        |0      |MF_EXTENDSTATE },//S~ +- %
};

/* takes a command index char (cast to (size_t) cos chars are allowed) */
#define COMMAND_SET(x) (global_command_db[(size_t)x].set)
#define COMMAND_STEREO(x) (global_command_db[(size_t)x].flags&MF_STEREO)
#define COMMAND_RAMP(x) (global_command_db[(size_t)x].flags&MF_RAMP)
#define COMMAND_COLOUR(x) (global_command_db[(size_t)x].colour)
#define COMMAND_TYPE(x) (global_command_db[(size_t)x].type)
#define COMMAND_PRETTYNAME(x) (global_command_db[(size_t)x].prettyname)
