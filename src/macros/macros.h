/* process macro .m if it's present in the row */
bool ifMacro(jack_nframes_t fptr, uint16_t *spr, Track *cv, Row *r, char m);

/* ifMacro(), but run .callback instead of guessing */
bool ifMacroCallback(jack_nframes_t fptr, uint16_t *spr, Track *cv, Row *r, char m, bool (*callback)(jack_nframes_t, uint16_t*, int, Track*, Row*));

/* if the row needs to be ramped in based on the macros present */
bool ifMacroRamp(Track *cv, Row *r);

bool changeMacro(int input, char *dest);
void addMacroBinds(const char *prettyname, unsigned int state, void (*callback)(void*));

#include "bpm.c"
#include "cut.c"
#include "delay.c"
#include "gain.c"
#include "send.c"
#include "pitch.c"
#include "retrigger.c"
#include "chance.c"
#include "offset.c"
#include "filter.c"
#include "samplerate.c"
#include "envelope.c"
#include "granular.c"

/* there can be up to 6 of these (colours 1-6 (no black/white)) */
enum MacroColour {
	MC_LOCAL,
	MC_GAIN,
	MC_FILTER,
	MC_RETRIGGER,
	MC_PITCH,
	MC_SEQUENCER,
};

enum MacroType {
	MT_LOCAL,
};
void handleMacroType(enum MacroType type, jack_nframes_t fptr, uint16_t *spr, Track *cv, Row *r);

/* not an enum so they can be accessed within macros */
#define MF_LINKNIBBLES (1<<0) /* treat both nibbles as equally significant */
#define MF_RAMP        (1<<1) /* calls to this macro cause clicks, ramping needed */
#define MF_SEQUENCED   (1<<2) /* macro only applies to tracks with an inst/note pair */

typedef struct {
	bool             set; /* set high if this index is significant */
	const char      *prettyname;
	enum MacroColour colour;
	enum MacroType   type;
	uint8_t          flags; /* MF declares or'd together */
	bool           (*triggercallback)(jack_nframes_t fptr, uint16_t *spr, int m, Track *cv, Row *c); /* m == -1 if the macro isn't found */
} MacroDef;

/* TODO: redo this table lol */
#define MACRO_MAX 128
const MacroDef global_macro_db[MACRO_MAX] =
{
	['G'] = { 1, "stereo gain"                 , MC_GAIN     , 0       , MF_LINKNIBBLES|MF_RAMP, macroGain                    , },
	['g'] = { 1, "smooth stereo gain"          , MC_GAIN     , 0       , MF_LINKNIBBLES        , macroSmoothGain              , },
	['J'] = { 1, "stereo gain jitter"          , MC_GAIN     , 0       , MF_LINKNIBBLES|MF_RAMP, macroGainJitter              , },
	['j'] = { 1, "smooth stereo gain jitter"   , MC_GAIN     , 0       , MF_LINKNIBBLES        , macroSmoothGainJitter        , },
	['S'] = { 1, "stereo send"                 , MC_GAIN     , 0       , MF_LINKNIBBLES|MF_RAMP, macroSend                    , },
	['s'] = { 1, "smooth stereo send"          , MC_GAIN     , 0       , MF_LINKNIBBLES|MF_RAMP, macroSmoothSend              , },
	['K'] = { 1, "stereo send jitter"          , MC_GAIN     , 0       , MF_LINKNIBBLES        , macroSendJitter              , },
	['k'] = { 1, "smooth stereo send jitter"   , MC_GAIN     , 0       , MF_LINKNIBBLES        , macroSmoothSendJitter        , },
	['R'] = { 1, "block retrigger"             , MC_SEQUENCER, 0       , 0                     , macroBlockRetrig             , },
	['r'] = { 1, "reverse block retrigger"     , MC_SEQUENCER, 0       , 0                     , macroReverseBlockRetrig      , },
	['Q'] = { 1, "tick retrigger"              , MC_SEQUENCER, 0       , 0                     , macroTickRetrig              , },
	['q'] = { 1, "reverse tick retrigger"      , MC_SEQUENCER, 0       , 0                     , macroReverseTickRetrig       , },
	['P'] = { 1, "portamento slide"            , MC_PITCH    , 0       , 0                     , macroPortamento              , },
	['p'] = { 1, "microtonal pitch offset"     , MC_PITCH    , 0       , 0                     , macroPitchOffset             , },
	['V'] = { 1, "vibrato"                     , MC_PITCH    , 0       , 0                     , macroVibrato                 , },
	['B'] = { 1, "bpm"                         , MC_SEQUENCER, 0       , 0                     , macroBpm                     , },
	['C'] = { 1, "note cut"                    , MC_SEQUENCER, 0       , 0                     , macroCut                     , },
	['D'] = { 1, "note delay (trig)"           , MC_SEQUENCER, 0       , 0                     , macroDelay                   , },
	['%'] = { 1, "row chance"                  , MC_SEQUENCER, 0       , 0                     , macroRowChance               , },
	['F'] = { 1, "stereo cutoff"               , MC_FILTER   , MT_LOCAL, MF_LINKNIBBLES|MF_RAMP, macroCutoff                  , },
	['f'] = { 1, "smooth stereo cutoff"        , MC_FILTER   , MT_LOCAL, MF_LINKNIBBLES        , macroSmoothCutoff            , },
	['Z'] = { 1, "stereo resonance"            , MC_FILTER   , MT_LOCAL, MF_LINKNIBBLES|MF_RAMP, macroResonance               , },
	['z'] = { 1, "smooth stereo resonance"     , MC_FILTER   , MT_LOCAL, MF_LINKNIBBLES        , macroSmoothResonance         , },
	['M'] = { 1, "stereo filter mode"          , MC_FILTER   , MT_LOCAL, MF_LINKNIBBLES        , macroFilterMode              , },
	['m'] = { 1, "smooth stereo filter mode"   , MC_FILTER   , MT_LOCAL, MF_LINKNIBBLES        , macroSmoothFilterMode        , },
	['E'] = { 1, "local att/dec"               , MC_LOCAL    , MT_LOCAL, 0                     , macroLocalAttDec             , },
	['e'] = { 1, "local sus/rel"               , MC_LOCAL    , MT_LOCAL, 0                     , macroLocalSusRel             , },
	['H'] = { 1, "local pitch shift"           , MC_LOCAL    , MT_LOCAL, 0                     , macroLocalPitchShift         , },
	['h'] = { 1, "smooth local pitch shift"    , MC_LOCAL    , MT_LOCAL, 0                     , macroSmoothLocalPitchShift   , },
	['O'] = { 1, "sample offset"               , MC_LOCAL    , MT_LOCAL, MF_RAMP               , macroOffset                  , },
	['o'] = { 1, "reverse sample offset"       , MC_LOCAL    , MT_LOCAL, MF_RAMP               , macroReverseOffset           , },
	['U'] = { 1, "sample offset jitter"        , MC_LOCAL    , MT_LOCAL, MF_RAMP               , macroOffsetJitter            , },
	['u'] = { 1, "reverse sample offset jitter", MC_LOCAL    , MT_LOCAL, MF_RAMP               , macroReverseOffsetJitter     , },
	['W'] = { 1, "pitch width"                 , MC_LOCAL    , MT_LOCAL, 0                     , macroLocalPitchWidth         , },
	['w'] = { 1, "smooth pitch width"          , MC_LOCAL    , MT_LOCAL, 0                     , macroSmoothLocalPitchWidth   , },
	['X'] = { 1, "samplerate"                  , MC_LOCAL    , MT_LOCAL, 0                     , macroLocalSamplerate         , },
	['x'] = { 1, "smooth samplerate"           , MC_LOCAL    , MT_LOCAL, 0                     , macroSmoothLocalSamplerate   , },
	['L'] = { 1, "cycle length high byte"      , MC_LOCAL    , MT_LOCAL, 0                     , macroLocalCycleLengthHighByte, },
	['l'] = { 1, "cycle length low byte"       , MC_LOCAL    , MT_LOCAL, 0                     , macroLocalCycleLengthLowByte , },
// bool macroLocalCycleLengthHighByte(jack_nframes_t fptr, uint16_t *spr, int m, Track *cv, Row r);
// bool macroLocalCycleLengthLowByte (jack_nframes_t fptr, uint16_t *spr, int m, Track *cv, Row r);
};

/* takes a macro index char (cast to (size_t) cos chars are allowed) */
#define MACRO_SET(x) (global_macro_db[(size_t)x].set)
#define MACRO_LINKNIBBLES(x) (global_macro_db[(size_t)x].flags&MF_LINKNIBBLES)
#define MACRO_RAMP(x) (global_macro_db[(size_t)x].flags&MF_RAMP)
#define MACRO_COLOUR(x) (global_macro_db[(size_t)x].colour)
#define MACRO_TYPE(x) (global_macro_db[(size_t)x].type)
#define MACRO_PRETTYNAME(x) (global_macro_db[(size_t)x].prettyname)
#define MACRO_TRIGGERCALLBACK(x) (global_macro_db[(size_t)x].triggercallback)
