/* process macro .m if it's present in the row */
bool ifMacro(jack_nframes_t fptr, uint16_t *spr, Track *cv, Row r, char m);

/* ifMacro(), but run .callback instead of guessing */
bool ifMacroCallback(jack_nframes_t fptr, uint16_t *spr, Track *cv, Row r, char m, bool (*callback)(jack_nframes_t, uint16_t*, int, Track*, Row));

/* if the row needs to be ramped in based on the macros present */
bool ifMacroRamp(Track *cv, Row r);

bool changeMacro(int input, char *dest);
void addMacroBinds(const char *prettyname, unsigned int state, void (*callback)(void*));

bool macroVibrato                 (jack_nframes_t fptr, uint16_t *spr, int m, Track *cv, Row r);
bool macroBpm                     (jack_nframes_t fptr, uint16_t *spr, int m, Track *cv, Row r);
bool macroCut                     (jack_nframes_t fptr, uint16_t *spr, int m, Track *cv, Row r);
bool macroPortamento              (jack_nframes_t fptr, uint16_t *spr, int m, Track *cv, Row r);
bool macroDelay                   (jack_nframes_t fptr, uint16_t *spr, int m, Track *cv, Row r);
bool macroGain                    (jack_nframes_t fptr, uint16_t *spr, int m, Track *cv, Row r);
bool macroSmoothGain              (jack_nframes_t fptr, uint16_t *spr, int m, Track *cv, Row r);
bool macroGainJitter              (jack_nframes_t fptr, uint16_t *spr, int m, Track *cv, Row r);
bool macroSmoothGainJitter        (jack_nframes_t fptr, uint16_t *spr, int m, Track *cv, Row r);
bool macroSend                    (jack_nframes_t fptr, uint16_t *spr, int m, Track *cv, Row r);
bool macroSmoothSend              (jack_nframes_t fptr, uint16_t *spr, int m, Track *cv, Row r);
bool macroSendJitter              (jack_nframes_t fptr, uint16_t *spr, int m, Track *cv, Row r);
bool macroSmoothSendJitter        (jack_nframes_t fptr, uint16_t *spr, int m, Track *cv, Row r);
bool macroTickRetrig              (jack_nframes_t fptr, uint16_t *spr, int m, Track *cv, Row r);
bool macroReverseTickRetrig       (jack_nframes_t fptr, uint16_t *spr, int m, Track *cv, Row r);
bool macroBlockRetrig             (jack_nframes_t fptr, uint16_t *spr, int m, Track *cv, Row r);
bool macroReverseBlockRetrig      (jack_nframes_t fptr, uint16_t *spr, int m, Track *cv, Row r);
bool macroPitchOffset             (jack_nframes_t fptr, uint16_t *spr, int m, Track *cv, Row r);
bool macroRowChance               (jack_nframes_t fptr, uint16_t *spr, int m, Track *cv, Row r);
bool macroOffset                  (jack_nframes_t fptr, uint16_t *spr, int m, Track *cv, Row r);
bool macroReverseOffset           (jack_nframes_t fptr, uint16_t *spr, int m, Track *cv, Row r);
bool macroOffsetJitter            (jack_nframes_t fptr, uint16_t *spr, int m, Track *cv, Row r);
bool macroReverseOffsetJitter     (jack_nframes_t fptr, uint16_t *spr, int m, Track *cv, Row r);
bool macroCutoff                  (jack_nframes_t fptr, uint16_t *spr, int m, Track *cv, Row r);
bool macroSmoothCutoff            (jack_nframes_t fptr, uint16_t *spr, int m, Track *cv, Row r);
bool macroCutoffJitter            (jack_nframes_t fptr, uint16_t *spr, int m, Track *cv, Row r);
bool macroSmoothCutoffJitter      (jack_nframes_t fptr, uint16_t *spr, int m, Track *cv, Row r);
bool macroResonance               (jack_nframes_t fptr, uint16_t *spr, int m, Track *cv, Row r);
bool macroSmoothResonance         (jack_nframes_t fptr, uint16_t *spr, int m, Track *cv, Row r);
bool macroResonanceJitter         (jack_nframes_t fptr, uint16_t *spr, int m, Track *cv, Row r);
bool macroSmoothResonanceJitter   (jack_nframes_t fptr, uint16_t *spr, int m, Track *cv, Row r);
bool macroFilterMode              (jack_nframes_t fptr, uint16_t *spr, int m, Track *cv, Row r);
bool macroSmoothFilterMode        (jack_nframes_t fptr, uint16_t *spr, int m, Track *cv, Row r);
bool macroLocalSamplerate         (jack_nframes_t fptr, uint16_t *spr, int m, Track *cv, Row r);
bool macroSmoothLocalSamplerate   (jack_nframes_t fptr, uint16_t *spr, int m, Track *cv, Row r);
bool macroLocalAttDec             (jack_nframes_t fptr, uint16_t *spr, int m, Track *cv, Row r);
bool macroLocalSusRel             (jack_nframes_t fptr, uint16_t *spr, int m, Track *cv, Row r);
bool macroLocalPitchShift         (jack_nframes_t fptr, uint16_t *spr, int m, Track *cv, Row r);
bool macroSmoothLocalPitchShift   (jack_nframes_t fptr, uint16_t *spr, int m, Track *cv, Row r);
bool macroLocalPitchWidth         (jack_nframes_t fptr, uint16_t *spr, int m, Track *cv, Row r);
bool macroSmoothLocalPitchWidth   (jack_nframes_t fptr, uint16_t *spr, int m, Track *cv, Row r);
bool macroLocalCycleLengthHighByte(jack_nframes_t fptr, uint16_t *spr, int m, Track *cv, Row r);
bool macroLocalCycleLengthLowByte (jack_nframes_t fptr, uint16_t *spr, int m, Track *cv, Row r);

/* there can be up to 6 of these (colours 1-6 (no black/white)) */
enum MacroGroup {
	MACRO_GROUP_LOCAL,
	MACRO_GROUP_GAIN,
	MACRO_GROUP_FILTER,
	MACRO_GROUP_RETRIGGER,
	MACRO_GROUP_PITCH,
	MACRO_GROUP_SEQUENCER,
};

/* not an enum so they can be accessed within macros */
#define MF_LINKNIBBLES (1<<0) /* treat both nibbles as equally significant */
#define MF_RAMP        (1<<1) /* calls to this macro cause clicks, ramping needed */
#define MF_SEQUENCED   (1<<2) /* macro only applies to tracks with an inst/note pair */

typedef struct {
	bool            set; /* set high if this index is significant */
	const char     *prettyname;
	enum MacroGroup group;
	uint8_t         flags; /* MF declares or'd together */
	bool          (*callback)(jack_nframes_t, uint16_t*, int, Track*, Row);
} MacroDef;

/* TODO: redo this table lol */
const MacroDef global_macro_db[128] =
{
	['G'] = { 1, "stereo gain"                 , MACRO_GROUP_GAIN     , MF_LINKNIBBLES|MF_RAMP, macroGain                 , },
	['g'] = { 1, "smooth stereo gain"          , MACRO_GROUP_GAIN     , MF_LINKNIBBLES        , macroSmoothGain           , },
	['J'] = { 1, "stereo gain jitter"          , MACRO_GROUP_GAIN     , MF_LINKNIBBLES|MF_RAMP, macroGainJitter           , },
	['j'] = { 1, "smooth stereo gain jitter"   , MACRO_GROUP_GAIN     , MF_LINKNIBBLES        , macroSmoothGainJitter     , },
	['S'] = { 1, "stereo send"                 , MACRO_GROUP_GAIN     , MF_LINKNIBBLES|MF_RAMP, macroSend                 , },
	['s'] = { 1, "smooth stereo send"          , MACRO_GROUP_GAIN     , MF_LINKNIBBLES|MF_RAMP, macroSmoothSend           , },
	['K'] = { 1, "stereo send jitter"          , MACRO_GROUP_GAIN     , MF_LINKNIBBLES        , macroSendJitter           , },
	['k'] = { 1, "smooth stereo send jitter"   , MACRO_GROUP_GAIN     , MF_LINKNIBBLES        , macroSmoothSendJitter     , },
	// ['N'] = { 1, "tremolo"                     , MACRO_GROUP_GAIN     , 0                                     , NULL                      , }, /* TODO */
	// ['n'] = { 1, "autopan"                     , MACRO_GROUP_GAIN     , 0                                     , NULL                      , }, /* TODO */
	['F'] = { 1, "stereo cutoff"               , MACRO_GROUP_FILTER   , MF_LINKNIBBLES|MF_RAMP, macroCutoff               , },
	['f'] = { 1, "smooth stereo cutoff"        , MACRO_GROUP_FILTER   , MF_LINKNIBBLES        , macroSmoothCutoff         , },
	/* TODO: cutoff jitter */
	['Z'] = { 1, "stereo resonance"            , MACRO_GROUP_FILTER   , MF_LINKNIBBLES|MF_RAMP, macroResonance            , },
	['z'] = { 1, "smooth stereo resonance"     , MACRO_GROUP_FILTER   , MF_LINKNIBBLES        , macroSmoothResonance      , },
	/* TODO: resonance jitter */
	['M'] = { 1, "stereo filter mode"          , MACRO_GROUP_FILTER   , MF_LINKNIBBLES        , macroFilterMode           , },
	['m'] = { 1, "smooth stereo filter mode"   , MACRO_GROUP_FILTER   , MF_LINKNIBBLES        , macroSmoothFilterMode     , },
	['R'] = { 1, "block retrigger"             , MACRO_GROUP_SEQUENCER, 0                     , macroBlockRetrig          , },
	['r'] = { 1, "reverse block retrigger"     , MACRO_GROUP_SEQUENCER, 0                     , macroReverseBlockRetrig   , },
	['Q'] = { 1, "tick retrigger"              , MACRO_GROUP_SEQUENCER, 0                     , macroTickRetrig           , },
	['q'] = { 1, "reverse tick retrigger"      , MACRO_GROUP_SEQUENCER, 0                     , macroReverseTickRetrig    , },
	['P'] = { 1, "portamento slide"            , MACRO_GROUP_PITCH    , 0                     , macroPortamento           , },
	['p'] = { 1, "microtonal pitch offset"     , MACRO_GROUP_PITCH    , 0                     , macroPitchOffset          , },
	['V'] = { 1, "vibrato"                     , MACRO_GROUP_PITCH    , 0                     , macroVibrato              , },
	// ['v'] = { 1, "stereo vibrato"              , MACRO_GROUP_PITCH    , 0                     , macroStereoVibrato        , },
	['B'] = { 1, "bpm"                         , MACRO_GROUP_SEQUENCER, 0                     , macroBpm                  , },
	['C'] = { 1, "note cut"                    , MACRO_GROUP_SEQUENCER, 0                     , macroCut                  , },
	['D'] = { 1, "note delay (trig)"           , MACRO_GROUP_SEQUENCER, 0                     , macroDelay                , },
	['%'] = { 1, "row chance"                  , MACRO_GROUP_SEQUENCER, 0                     , macroRowChance            , },
	// ['^'] = { 1, "row repititions"             , MACRO_GROUP_SEQUENCER, 0                     , NULL                      , }, /* TODO */
	// ['I'] = { 1, "instrument jitter"           , MACRO_GROUP_SEQUENCER, 0                     , NULL                      , }, /* TODO */
	['E'] = { 1, "local att/dec"               , MACRO_GROUP_LOCAL    , 0                     , macroLocalAttDec          , },
	['e'] = { 1, "local sus/rel"               , MACRO_GROUP_LOCAL    , 0                     , macroLocalSusRel          , },
	['H'] = { 1, "local pitch shift"           , MACRO_GROUP_LOCAL    , 0                     , macroLocalPitchShift      , },
	['h'] = { 1, "smooth local pitch shift"    , MACRO_GROUP_LOCAL    , 0                     , macroSmoothLocalPitchShift, },
	['O'] = { 1, "sample offset"               , MACRO_GROUP_LOCAL    , MF_RAMP               , macroOffset               , },
	['o'] = { 1, "reverse sample offset"       , MACRO_GROUP_LOCAL    , MF_RAMP               , macroReverseOffset        , },
	['U'] = { 1, "sample offset jitter"        , MACRO_GROUP_LOCAL    , MF_RAMP               , macroOffsetJitter         , },
	['u'] = { 1, "reverse sample offset jitter", MACRO_GROUP_LOCAL    , MF_RAMP               , macroReverseOffsetJitter  , },
};

/* takes a macro index char (cast to (size_t) cos chars are allowed) */
#define MACRO_SET(x)         (global_macro_db[(size_t)x].set                 )
#define MACRO_LINKNIBBLES(x) (global_macro_db[(size_t)x].flags&MF_LINKNIBBLES)
#define MACRO_RAMP(x)        (global_macro_db[(size_t)x].flags&MF_RAMP       )
#define MACRO_GROUP(x)       (global_macro_db[(size_t)x].group               )
#define MACRO_PRETTYNAME(x)  (global_macro_db[(size_t)x].prettyname          )
#define MACRO_CALLBACK(x)    (global_macro_db[(size_t)x].callback            )
