void samplerInit(Inst *iv)
{
	iv->type = INST_TYPE_SAMPLER;

	InstSamplerState *s = iv->state = calloc(1, sizeof(InstSamplerState));
	s->sample = calloc(1, sizeof(SampleChain));

	s->samplerate = 0xff;
	s->bitdepth = 0xf;
	s->envelope = 0x00f0;

	s->granular.cyclelength = 0x3fff;
	s->granular.rampgrains = 8;
	s->granular.beatsensitivity = 0x80;
	s->granular.beatdecay = 0xff;
}

void samplerFree(Inst *iv)
{
	freeWaveform();

	InstSamplerState *s = iv->state;
	FOR_SAMPLECHAIN(i, s->sample)
		free((*s->sample)[i]);
	free(s->sample);
	free(s);
}

/* dest has already been free'd */
void samplerCopy(Inst *dest, Inst *src)
{
	dest->type = INST_TYPE_SAMPLER;
	InstSamplerState *s = dest->state = calloc(1, sizeof(InstSamplerState));

	memcpy(dest->state, src->state, sizeof(InstSamplerState));

	s->sample = calloc(1, sizeof(SampleChain));
	copySampleChain(s->sample, ((InstSamplerState*)src->state)->sample);
}

void samplerGetIndexInfo(Inst *iv, char *buffer)
{
	uint32_t samplesize = 0;
	InstSamplerState *s = iv->state;
	FOR_SAMPLECHAIN(i, s->sample)
		samplesize +=
				(*s->sample)[i]->length *
				(*s->sample)[i]->channels;
	humanReadableSize(samplesize, buffer);
}

void samplerTriggerNote(uint32_t fptr, Inst *iv, Track *cv, uint8_t oldnote, uint8_t note, short inst)
{
	InstSamplerState *s = iv->state;
	InstSamplerPlaybackState *ps = cv->inststate;
	ps->sampleslot = s->samplemap[cv->r.note];
}

#include "macros.c"
#include "input.c"
#include "draw.c"
#include "process.c"
