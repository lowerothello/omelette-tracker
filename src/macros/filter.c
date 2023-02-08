/* TODO: temp name */
#define AA(TYPE, I) \
	if (cv->filter.target##TYPE[I] != -1) \
	{ \
		if (cv->filter.target##TYPE##_rand) cv->filter.rand##TYPE[I] = cv->filter.target##TYPE[I]; \
		else                                cv->filter.TYPE[I] = cv->filter.rand##TYPE[I] = cv->filter.target##TYPE[I]; \
		cv->filter.target##TYPE[I] = -1; \
	}

bool macroCutoff(jack_nframes_t fptr, uint16_t *spr, int m, Track *cv, Row *r)
{
	if (m < 0) return 0;

	cv->filter.cut[0] = cv->filter.randcut[0] =  m&0xf0;
	cv->filter.cut[1] = cv->filter.randcut[1] = (m&0x0f)<<4;
	return 1;
}
bool macroSmoothCutoff(jack_nframes_t fptr, uint16_t *spr, int m, Track *cv, Row *r)
{
	AA(cut, 0); AA(cut, 1); cv->filter.targetcut_rand = 0;
	if (m < 0) return 0;

	cv->filter.targetcut[0] =  m&0xf0;
	cv->filter.targetcut[1] = (m&0x0f)<<4;
	return 1;
}
bool macroCutoffJitter(jack_nframes_t fptr, uint16_t *spr, int m, Track *cv, Row *r)
{
	if (m < 0) return 0;

	short stereo = rand()%((m&0xf0)+1);
	cv->filter.randcut[0] = MAX(0, cv->filter.cut[0] - stereo - rand()%(((m&0x0f)<<4)+1));
	cv->filter.randcut[1] = MAX(0, cv->filter.cut[1] - stereo - rand()%(((m&0x0f)<<4)+1));
	return 1;
}
bool macroSmoothCutoffJitter(jack_nframes_t fptr, uint16_t *spr, int m, Track *cv, Row *r)
{
	if (m < 0) return 0;

	cv->filter.targetcut_rand = 1;
	short stereo = rand()%((m&0xf0)+1);
	cv->filter.targetcut[0] = MAX(0, cv->filter.cut[0] - stereo - rand()%(((m&0x0f)<<4)+1));
	cv->filter.targetcut[1] = MAX(0, cv->filter.cut[1] - stereo - rand()%(((m&0x0f)<<4)+1));
	return 1;
}

bool macroResonance(jack_nframes_t fptr, uint16_t *spr, int m, Track *cv, Row *r)
{
	if (m < 0) return 0;

	cv->filter.res[0] = cv->filter.randres[0] =  m&0xf0;
	cv->filter.res[1] = cv->filter.randres[1] = (m&0x0f)<<4;
	return 1;
}
bool macroSmoothResonance(jack_nframes_t fptr, uint16_t *spr, int m, Track *cv, Row *r)
{
	AA(res, 0); AA(res, 1); cv->filter.targetres_rand = 0;
	if (m < 0) return 0;

	cv->filter.targetres[0] =  m&0xf0;
	cv->filter.targetres[1] = (m&0x0f)<<4;
	return 1;
}
bool macroResonanceJitter(jack_nframes_t fptr, uint16_t *spr, int m, Track *cv, Row *r)
{
	if (m < 0) return 0;

	short stereo = rand()%((m&0xf0)+1);
	cv->filter.randres[0] = MAX(0, cv->filter.res[0] - stereo - rand()%(((m&0x0f)<<4)+1));
	cv->filter.randres[1] = MAX(0, cv->filter.res[1] - stereo - rand()%(((m&0x0f)<<4)+1));
	return 1;
}
bool macroSmoothResonanceJitter(jack_nframes_t fptr, uint16_t *spr, int m, Track *cv, Row *r)
{
	if (m < 0) return 0;

	cv->filter.targetres_rand = 1;
	short stereo = rand()%((m&0xf0)+1);
	cv->filter.targetres[0] = MAX(0, cv->filter.res[0] - stereo - rand()%(((m&0x0f)<<4)+1));
	cv->filter.targetres[1] = MAX(0, cv->filter.res[1] - stereo - rand()%(((m&0x0f)<<4)+1));
	return 1;
}
bool macroFilterMode(jack_nframes_t fptr, uint16_t *spr, int m, Track *cv, Row *r)
{
	if (m < 0) return 0;

	cv->filter.mode[0] = (m&0x70)>>4; /* ignore the '8' bit */
	cv->filter.mode[1] =  m&0x07;     /* ignore the '8' bit */
	return 1;
}
bool macroSmoothFilterMode(jack_nframes_t fptr, uint16_t *spr, int m, Track *cv, Row *r)
{
	if (cv->filter.targetmode[0] != -1) { cv->filter.mode[0] = cv->filter.targetmode[0]; cv->filter.targetmode[0] = -1; }
	if (cv->filter.targetmode[1] != -1) { cv->filter.mode[1] = cv->filter.targetmode[1]; cv->filter.targetmode[1] = -1; }
	if (m < 0) return 0;

	cv->filter.targetmode[0] = (m&0x70)>>4; /* ignore the '8' bit */
	cv->filter.targetmode[1] =  m&0x07;     /* ignore the '8' bit */
	return 1;
}
