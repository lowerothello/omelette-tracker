void drawMidi(ControlState *cc, Instrument *iv)
{
	clearControls(cc);

	short x = INSTRUMENT_INDEX_COLS + ((ws.ws_col - INSTRUMENT_INDEX_COLS - 18)>>1) +1;
	short y = ws.ws_row - 12;

	printf("\033[%d;%dHMIDI track:  [ ]", y+0, x);
	printf("\033[%d;%dHMIDI program: [  ]", y+1, x);
	addControlInt(cc, x+16, y+0, &iv->midi.track, 1, -1, 15, -1, 0, 0, instrumentSamplerControlCallback, NULL);

	drawControls(cc);
}
