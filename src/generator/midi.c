#define INSTUI_MIDI_WIDTH 18

static void midiInstUICallback(short x, short y, Instrument *iv, uint8_t index)
{
	switch (index)
	{
		case 0:
			printf("\033[%d;%dHMIDI channel:  [ ]", y, x);
			addControlInt(&cc, x+16, y, &iv->midi.channel, 1, -1, 15, -1, 0, 0, instrumentSamplerControlCallback, NULL);
			break;
		case 1:
			printf("\033[%d;%dHMIDI program: [  ]", y, x);
			break;
	}
}
void initInstUIMidi(InstUI **iui)
{
	*iui = allocInstUI(1);
	(*iui)->width = INSTUI_MIDI_WIDTH;
	(*iui)->block[0].count = 2;
	(*iui)->block[0].callback = midiInstUICallback;
}
