bool ifMacro(Track *cv, Row *r, char m);

/* ifMacro(), but run .callback */
void ifMacroCallback(jack_nframes_t fptr, uint16_t *spr, Track *cv, Row *r, char m, void (*callback)(jack_nframes_t, uint16_t*, int, Track*, Row*));

/* access macros with (Row*)->macro[i] */
#define FOR_ROW_MACROS(i, cv) for (int i = 0; i < cv->variant->macroc; i++)

/* if the row needs to be ramped in based on the macros present */
bool ifMacroRamp(Track *cv, Row *r);

bool changeMacro(int input, char *dest);
void addMacroBinds(const char *prettyname, unsigned int state, void (*callback)(void*));

/* there can be up to 6 of these (colours 1-6 (no black/white)) */
enum MacroColour
{
	MC_LOCAL,
	MC_GAIN,
	MC_FILTER,
	MC_RETRIGGER,
	MC_PITCH,
	MC_SEQUENCER,
};

void macroStateReset(MacroState *s);
void macroStateSet(MacroState *s, Macro macro);
void macroStateSmooth(MacroState *s, Macro macro);
void macroStateApply(MacroState *s);
float macroStateGetMono(MacroState *s, float rp);
void macroStateGetStereo(MacroState *s, float rp, float *l, float *r);

#include "bpm.c"
#include "row.c"
#include "gain.c"
#include "send.c"
#include "pitch.c"
#include "retrigger.c"
#include "chance.c"
#include "offset.c"
#include "filter.c"
#include "granular.c"

typedef struct MacroCallbacks
{
	void        (*pretrig)(jack_nframes_t fptr, uint16_t *spr, Track *cv, Row *r);
	void       (*posttrig)(jack_nframes_t fptr, uint16_t *spr, Track *cv, Row *r);
	void    (*triggernote)(jack_nframes_t fptr, Track *cv, uint8_t oldnote, uint8_t note, short inst);
	uint8_t   (*samplerow)(jack_nframes_t fptr, uint16_t count, uint16_t *spr, uint16_t sprp, Track *cv);
	void (*persistenttune)(jack_nframes_t fptr, uint16_t count, uint16_t *spr, uint16_t sprp, Track *cv);
	void   (*volatiletune)(jack_nframes_t fptr, uint16_t count, uint16_t *spr, uint16_t sprp, Track *cv, float *finetune, uint32_t *pointer, uint32_t *pitchedpointer);
	void    (*postsampler)(jack_nframes_t fptr, Track *cv, float rp, float *lf, float *rf);
} MacroCallbacks;

#define MACRO_CALLBACK_MAX 128
const MacroCallbacks global_macro_callbacks[MACRO_CALLBACK_MAX] =
{
	{ macroBpmPreTrig, NULL, NULL, NULL, NULL, NULL, NULL },
	{ macroRowPreTrig, NULL, NULL, macroRowSampleRow, NULL, NULL, NULL },
	{ macroGainPreTrig, NULL, NULL, NULL, NULL, NULL, NULL },
	{ macroSendPreTrig, NULL, NULL, NULL, NULL, NULL, NULL },
	{ macroPitchPreTrig, NULL, macroPitchTriggerNote, NULL, macroPitchPersistent, macroPitchVolatile, NULL },
	{ NULL, macroRetrigPostTrig, macroRetrigTriggerNote, NULL, NULL, macroRetrigVolatile, NULL },
	{ macroChancePreTrig, NULL, NULL, NULL, NULL, NULL, NULL },
	{ NULL, macroOffsetPostTrig, NULL, NULL, NULL, NULL, NULL },
	{ NULL, macroFilterPostTrig, NULL, NULL, NULL, NULL, macroFilterPostSampler },
	{ NULL, macroGranularPostTrig, macroGranularTriggerNote, NULL, NULL, NULL, NULL },
};


/* not an enum so they can be accessed within macros */
#define MF_STEREO      (1<<0) /* treat both nibbles as equally significant               */
#define MF_RAMP        (1<<1) /* calls to this macro cause clicks, ramping needed        */
#define MF_SEQUENCED   (1<<2) /* macro only applies to tracks with an inst/note pair     */
#define MF_EXTENDSTATE (1<<3) /* generate a smooth variant, and lfo/inc+dec/jitter binds */


typedef struct MacroDef
{
	bool             set; /* set high if this index is significant */
	const char      *prettyname;
	enum MacroColour colour;
	uint8_t          flags; /* MF declares or'd together */
} MacroDef;


#define MACRO_MAX 128
const MacroDef global_macro_db[MACRO_MAX] =
{
	[MACRO_GAIN]                  = { 1, "stereo gain"                 , MC_GAIN     , MF_STEREO|MF_RAMP|MF_EXTENDSTATE },//S~ +- %
	[MACRO_SMOOTH_GAIN]           = { 1, "smooth stereo gain"          , MC_GAIN     , MF_STEREO|0      |0              },//--
	// [MACRO_GAIN_JITTER]           = { 1, "stereo gain jitter"          , MC_GAIN     , MF_STEREO|MF_RAMP|0              },//--
	// [MACRO_SMOOTH_GAIN_JITTER]    = { 1, "smooth stereo gain jitter"   , MC_GAIN     , MF_STEREO|0      |0              },//--
	[MACRO_SEND]                  = { 1, "stereo send"                 , MC_GAIN     , MF_STEREO|MF_RAMP|MF_EXTENDSTATE },//S~ +- %
	[MACRO_SMOOTH_SEND]           = { 1, "smooth stereo send"          , MC_GAIN     , MF_STEREO|MF_RAMP|0              },//--
	// [MACRO_SEND_JITTER]           = { 1, "stereo send jitter"          , MC_GAIN     , MF_STEREO|0      |0              },//--
	// [MACRO_SMOOTH_SEND_JITTER]    = { 1, "smooth stereo send jitter"   , MC_GAIN     , MF_STEREO|0      |0              },//--
	[MACRO_BLOCK_RETRIG]          = { 1, "block retrigger"             , MC_SEQUENCER, 0        |0      |0              },//
	[MACRO_REVERSE_BLOCK_RETRIG]  = { 1, "reverse block retrigger"     , MC_SEQUENCER, 0        |0      |0              },//
	[MACRO_TICK_RETRIG]           = { 1, "tick retrigger"              , MC_SEQUENCER, 0        |0      |0              },//
	[MACRO_REVERSE_TICK_RETRIG]   = { 1, "reverse tick retrigger"      , MC_SEQUENCER, 0        |0      |0              },//
	[MACRO_PORTAMENTO]            = { 1, "portamento slide"            , MC_PITCH    , 0        |0      |0              },//
	[MACRO_PITCH_OFFSET]          = { 1, "microtonal pitch offset"     , MC_PITCH    , 0        |0      |MF_EXTENDSTATE },//S~ +- %
	[MACRO_VIBRATO]               = { 1, "vibrato"                     , MC_PITCH    , 0        |0      |0              },//--
	[MACRO_BPM]                   = { 1, "bpm"                         , MC_SEQUENCER, 0        |0      |0              },//
	[MACRO_ROW_CUT]               = { 1, "note cut"                    , MC_SEQUENCER, 0        |0      |0              },//
	[MACRO_ROW_DELAY]             = { 1, "note delay"                  , MC_SEQUENCER, 0        |0      |0              },//
	[MACRO_NOTE_CHANCE]           = { 1, "note chance"                 , MC_SEQUENCER, 0        |0      |0              },//
	[MACRO_FILTER]                = { 1, "stereo cutoff"               , MC_FILTER   , MF_STEREO|MF_RAMP|MF_EXTENDSTATE },//S~ +- %
	[MACRO_SMOOTH_FILTER]         = { 1, "smooth stereo cutoff"        , MC_FILTER   , MF_STEREO|0      |0              },//--
	[MACRO_RESONANCE]             = { 1, "stereo resonance"            , MC_FILTER   , MF_STEREO|MF_RAMP|MF_EXTENDSTATE },//S~ +- %
	[MACRO_SMOOTH_RESONANCE]      = { 1, "smooth stereo resonance"     , MC_FILTER   , MF_STEREO|0      |0              },//--
	[MACRO_FILTER_MODE]           = { 1, "stereo filter mode"          , MC_FILTER   , MF_STEREO|0      |0              },//S %
	[MACRO_SMOOTH_FILTER_MODE]    = { 1, "smooth stereo filter mode"   , MC_FILTER   , MF_STEREO|0      |0              },//
	[MACRO_ATT_DEC]               = { 1, "local att/dec"               , MC_LOCAL    , 0        |0      |0              },//+- %
	[MACRO_SUS_REL]               = { 1, "local sus/rel"               , MC_LOCAL    , 0        |0      |0              },//+- %
	[MACRO_PITCH_SHIFT]           = { 1, "local pitch shift"           , MC_PITCH    , 0        |0      |MF_EXTENDSTATE },//S~ +- %
	[MACRO_SMOOTH_PITCH_SHIFT]    = { 1, "smooth local pitch shift"    , MC_PITCH    , 0        |0      |0              },//--
	[MACRO_OFFSET]                = { 1, "sample offset"               , MC_LOCAL    , 0        |MF_RAMP|0              },//
	[MACRO_REVERSE_OFFSET]        = { 1, "reverse sample offset"       , MC_LOCAL    , 0        |MF_RAMP|0              },//
	[MACRO_OFFSET_JITTER]         = { 1, "sample offset jitter"        , MC_LOCAL    , 0        |MF_RAMP|0              },//
	[MACRO_REVERSE_OFFSET_JITTER] = { 1, "reverse sample offset jitter", MC_LOCAL    , 0        |MF_RAMP|0              },//
	[MACRO_PITCH_WIDTH]           = { 1, "pitch width"                 , MC_PITCH    , 0        |0      |MF_EXTENDSTATE },//S~ +- %
	[MACRO_SMOOTH_PITCH_WIDTH]    = { 1, "smooth pitch width"          , MC_PITCH    , 0        |0      |0              },//--
	[MACRO_SAMPLERATE]            = { 1, "samplerate"                  , MC_LOCAL    , 0        |0      |MF_EXTENDSTATE },//S~ +- %
	[MACRO_SMOOTH_SAMPLERATE]     = { 1, "smooth samplerate"           , MC_LOCAL    , 0        |0      |0              },//--
	[MACRO_CYCLE_LENGTH_HI_BYTE]  = { 1, "cycle length high byte"      , MC_LOCAL    , 0        |0      |MF_EXTENDSTATE },//S~ +- %
	[MACRO_CYCLE_LENGTH_LO_BYTE]  = { 1, "cycle length low byte"       , MC_LOCAL    , 0        |0      |MF_EXTENDSTATE },//S~ +- %
};

/* takes a macro index char (cast to (size_t) cos chars are allowed) */
#define MACRO_SET(x) (global_macro_db[(size_t)x].set)
#define MACRO_STEREO(x) (global_macro_db[(size_t)x].flags&MF_STEREO)
#define MACRO_RAMP(x) (global_macro_db[(size_t)x].flags&MF_RAMP)
#define MACRO_COLOUR(x) (global_macro_db[(size_t)x].colour)
#define MACRO_TYPE(x) (global_macro_db[(size_t)x].type)
#define MACRO_PRETTYNAME(x) (global_macro_db[(size_t)x].prettyname)
