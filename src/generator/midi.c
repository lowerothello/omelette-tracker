#define INSTUI_MIDI_WIDTH 18

static void midiInstUICallback(short x, short y, Instrument *iv, uint8_t index)
{
	switch (index)
	{
		case 0:
			printf("\033[%d;%dHMIDI channel:  [ ]", y, x);
			addControlInt(x+16, y, &iv->midi.channel, 1, -1, 15, -1, 0, 0, (void(*)(void*))instrumentControlCallback, NULL);
			break;
		case 1:
			printf("\033[%d;%dHMIDI program: [  ]", y, x);
			break;
	}
}
InstUI *initInstUIMidi(void)
{
	InstUI *iui = malloc(sizeof(InstUI));
	iui->width = INSTUI_MIDI_WIDTH;
	iui->count = 2;
	iui->callback = midiInstUICallback;
	return iui;
}
