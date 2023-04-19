#define MACRO_PITCH_SHIFT           'H'
#define MACRO_SMOOTH_PITCH_SHIFT    'h'
#define MACRO_PITCH_WIDTH           'W'
#define MACRO_SMOOTH_PITCH_WIDTH    'w'
#define MACRO_CYCLE_LENGTH_HI_BYTE  'L'
#define MACRO_CYCLE_LENGTH_LO_BYTE  'l'
#define MACRO_SAMPLERATE            'X'
#define MACRO_SMOOTH_SAMPLERATE     'x'
#define MACRO_ATT_DEC               'E'
#define MACRO_SUS_REL               'e'
#define MACRO_OFFSET                'O'
#define MACRO_REVERSE_OFFSET        'o'
#define MACRO_OFFSET_JITTER         'U'
#define MACRO_REVERSE_OFFSET_JITTER 'u'


void macroInstSamplerPostTrig(uint32_t fptr, uint16_t *spr, Track *cv, Row *r, void *state);
void macroInstSamplerTriggerNote(uint32_t fptr, Track *cv, float oldnote, float note, short inst, void *state);
