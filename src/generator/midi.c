void drawMidi(ControlState *cc, Instrument *iv, short x)
{
	clearControls(cc);

	short y = ws.ws_row - 12;

	printf("\033[%d;%dHMIDI track:  [ ]", y+0, x);
	printf("\033[%d;%dHMIDI program: [  ]", y+1, x);
	addControlInt(cc, x+16, y+0, &iv->midi.channel, 1, -1, 15, -1, 0, 0, instrumentSamplerControlCallback, NULL);

	drawControls(cc);
}
