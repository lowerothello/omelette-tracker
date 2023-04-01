void instUIMidiCallback(short x, short y, Inst *iv, uint8_t index)
{
	switch (index)
	{
		case 0:
			printf("\033[%d;%dHMIDI channel:  [ ]", y, x);
			addControlInt(x+16, y, &((InstMidiState*)iv->state)->channel, 1, -1, 15, -1, 0, 0, (void(*)(void*))instControlCallback, NULL);
			break;
		case 1:
			printf("\033[%d;%dHMIDI program: [  ]", y, x);
			break;
	}
}

const InstUI midiInstUI = { 18, 2, 2, instUIMidiCallback, };

void midiDraw(Inst *iv, short x, short y, short width, short height, short minx, short maxx)
{
	drawInstUI(&midiInstUI, iv, x, width, y, 0, midiInstUI.count);
}
