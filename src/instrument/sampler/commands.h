#define COMMAND_PITCH_SHIFT           'H'
#define COMMAND_SMOOTH_PITCH_SHIFT    'h'
#define COMMAND_PITCH_WIDTH           'W'
#define COMMAND_SMOOTH_PITCH_WIDTH    'w'
#define COMMAND_CYCLE_LENGTH_HI_BYTE  'L'
#define COMMAND_CYCLE_LENGTH_LO_BYTE  'l'
#define COMMAND_SAMPLERATE            'X'
#define COMMAND_SMOOTH_SAMPLERATE     'x'
#define COMMAND_ATT_DEC               'E'
#define COMMAND_SUS_REL               'e'
#define COMMAND_OFFSET                'O'
#define COMMAND_REVERSE_OFFSET        'o'
#define COMMAND_OFFSET_JITTER         'U'
#define COMMAND_REVERSE_OFFSET_JITTER 'u'


void commandInstSamplerPostTrig(uint32_t fptr, uint16_t *spr, Track *cv, Row *r, void *state);
void commandInstSamplerTriggerNote(uint32_t fptr, Track *cv, float oldnote, float note, short inst, void *state);
