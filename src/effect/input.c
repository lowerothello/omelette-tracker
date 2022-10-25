#include "chord/add.c"

void effectUpArrow    (int count) { decControlCursor(&cc, count); }
void effectDownArrow  (int count) { incControlCursor(&cc, count); }
void effectCtrlUpArrow  (EffectChain *chain, int count) { cc.cursor = getCursorFromEffect(chain, MAX(0, getEffectFromCursor(chain, cc.cursor) - count));          }
void effectCtrlDownArrow(EffectChain *chain, int count) { cc.cursor = getCursorFromEffect(chain, MIN(chain->c-1, getEffectFromCursor(chain, cc.cursor) + count)); }
void effectLeftArrow (void) { incControlFieldpointer(&cc); }
void effectRightArrow(void) { decControlFieldpointer(&cc); }
void effectHome(void) { setControlCursor(&cc, 0);             }
void effectEnd (void) { setControlCursor(&cc, cc.controlc-1); }

int inputEffect(EffectChain **chain, int input)
{
	uint8_t selectedindex;

	if (w->chord)
	{
		inputTooltip(&tt, input); p->redraw = 1;
	} else
		switch (input)
		{
			case '\n': case '\r': toggleKeyControl(&cc); p->redraw = 1; break;
			case '\b': case 127:  revertKeyControl(&cc); p->redraw = 1; break;
			case 'a': setChordAddEffect(chain);       p->redraw = 1; return 1; break;
			case 'A': setChordAddEffectBefore(chain); p->redraw = 1; return 1; break;
			case 'd':
				selectedindex = getEffectFromCursor(*chain, cc.cursor);
				cc.cursor = MAX(0, cc.cursor - (short)getEffectControlCount(&(*chain)->v[selectedindex]));
				delEffect(chain, selectedindex);
				p->redraw = 1; break;
			case 1:  /* ^a */ incControlValue(&cc); p->redraw = 1; break;
			case 24: /* ^x */ decControlValue(&cc); p->redraw = 1; break;
		}
	return 0;
}
