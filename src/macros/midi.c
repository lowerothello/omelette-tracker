/* TODO: smooth variants */
/* TODO: proper state, for every cc */

#define MACRO_MIDI_PC ':'

void macroMidiPostTrig(uint32_t fptr, uint16_t *spr, Track *cv, Row *r, void *state)
{
	/* TODO: need to get the midi channel from the instrument */
	uint8_t midichannel = 0;

	FOR_ROW_MACROS(i, cv)
		switch (r->macro[i].c)
		{
			case '0': midiCC(fptr, midichannel, 0x00 + (r->macro[i].v>>4), r->macro[i].v&0xf); break;
			case '1': midiCC(fptr, midichannel, 0x10 + (r->macro[i].v>>4), r->macro[i].v&0xf); break;
			case '2': midiCC(fptr, midichannel, 0x20 + (r->macro[i].v>>4), r->macro[i].v&0xf); break;
			case '3': midiCC(fptr, midichannel, 0x30 + (r->macro[i].v>>4), r->macro[i].v&0xf); break;
			case '4': midiCC(fptr, midichannel, 0x40 + (r->macro[i].v>>4), r->macro[i].v&0xf); break;
			case '5': midiCC(fptr, midichannel, 0x50 + (r->macro[i].v>>4), r->macro[i].v&0xf); break;
			case '6': midiCC(fptr, midichannel, 0x60 + (r->macro[i].v>>4), r->macro[i].v&0xf); break;
			case '7': midiCC(fptr, midichannel, 0x70 + (r->macro[i].v>>4), r->macro[i].v&0xf); break;
			case MACRO_MIDI_PC: midiPC(fptr, midichannel, r->macro[i].v&0x7f); break;
		}
}
