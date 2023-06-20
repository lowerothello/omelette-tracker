/* TODO: smooth variants */
/* TODO: proper state, for every cc */

#define COMMAND_MIDI_PC ':'

void commandMidiPostTrig(uint32_t fptr, uint16_t *spr, Track *cv, Row *r, void *state)
{
	/* TODO: need to get the midi channel from the instrument */
	uint8_t midichannel = 0;

	Command *m;
	FOR_ROW_COMMANDS(i, cv)
	{
		m = &r->command[i];
		switch (r->command[i].c)
		{
			case '0': midiCC(fptr, midichannel, 0x00 + (m->v>>4), m->v&0xf); break;
			case '1': midiCC(fptr, midichannel, 0x10 + (m->v>>4), m->v&0xf); break;
			case '2': midiCC(fptr, midichannel, 0x20 + (m->v>>4), m->v&0xf); break;
			case '3': midiCC(fptr, midichannel, 0x30 + (m->v>>4), m->v&0xf); break;
			case '4': midiCC(fptr, midichannel, 0x40 + (m->v>>4), m->v&0xf); break;
			case '5': midiCC(fptr, midichannel, 0x50 + (m->v>>4), m->v&0xf); break;
			case '6': midiCC(fptr, midichannel, 0x60 + (m->v>>4), m->v&0xf); break;
			case '7': midiCC(fptr, midichannel, 0x70 + (m->v>>4), m->v&0xf); break;
			case COMMAND_MIDI_PC: midiPC(fptr, midichannel, m->v&0x7f); break;
		}
	}
}
