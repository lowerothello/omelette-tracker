/* caller should free the returned value */
char *fileExtension(char *path, char *ext);

void writeSongNew(Song *cs, FILE *fp);
Song *readSongNew(FILE *fp);

/* every key that can be stored in mod files */
/* each enum must fit in 1 byte */
enum FileKeyHeader
{
	FKH_EOF          = 0, /* return */
	FKH_VERSION      = 1, /* file version */
	FKH_SAMPLERATE   = 2, /* samplerate the file was saved with */
	FKH_BPM          = 3,
	FKH_ROWHIGHLIGHT = 4,
	FKH_LOOP         = 5, /* loop range */
	FKH_GOTO_TRACK   = 100,
	FKH_GOTO_INST    = 101,
};

/* no key for variant chain song length, it's implied from the global song length */
enum FileKeyTrack
{
	FKT_EOF           = 0, /* return */
	FKT_COUNT         = 1, /* how many tracks to expect */
	FKT_TRACK         = 2, /* set target track */
	FKT_VARIANT       = 3, /* set target variant */
	FKT_ROWSIZE       = 4, /* set expected row size */
	FKT_VTRIGSIZE     = 5, /* set expected vtrig size */
	FKT_MUTE          = 10,
	FKT_MACROC        = 11,
	FKT_VTRIG         = 12,
	FKT_MAIN          = 13,
	FKT_VARIANTC      = 14, /* variant count */
	FKT_VARIANTI      = 15, /* variant backref */
	FKT_VARIANTV_ROWC = 16,
	FKT_VARIANTV_ROWV = 17,
	FKT_GOTO_EFFECT   = 100,
};

/* separate enum to leave open the possibility of saving effect chain presets */
enum FileKeyEffect
{
	FKE_EOF             = 0, /* return */
	FKE_COUNT           = 1, /* how many effects to expect */
	FKE_EFFECT          = 2, /* set target effect */
	FKE_TYPE            = 3, /* enum EFFECT_TYPE */
	FKE_LADSPA_UUID     = 10,
	FKE_LADSPA_CONTROLC = 11,
	FKE_LADSPA_CONTROLV = 12,
	FKE_LV2_URI         = 20, /* NULL terminated string */
	FKE_LV2_CONTROLC    = 21,
	FKE_LV2_CONTROLV    = 22,
};

enum FileKeyInst
{
	FKI_EOF   = 0, /* return */
	FKI_COUNT = 1, /* how many instruments to expect */
	FKI_INST  = 2, /* set target instrument */
};

#include "file.c"
