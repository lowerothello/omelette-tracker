/* ifMacro() is declared in types/track.h TODO: it should be declared here */
bool changeMacro(int input, char *dest, bool *destalt, bool alt);
int changeMacroVtrig(int input, char *dest, bool *destalt, bool alt);
void descMacro(char c, uint8_t v, bool alt);
bool linkMacroNibbles(char m);

char DUMMY(jack_nframes_t fptr, uint16_t *spr, int m, Track *cv, Row r) { return 1; }
char ocPRERAMP(jack_nframes_t fptr, uint16_t *spr, int m, Track *cv, Row r) { if (m) return 1; return 0; }

char Vc(jack_nframes_t fptr, uint16_t *spr, int m, Track *cv, Row r);
char Bc(jack_nframes_t fptr, uint16_t *spr, int m, Track *cv, Row r);
char Cc(jack_nframes_t fptr, uint16_t *spr, int m, Track *cv, Row r);
char Pc(jack_nframes_t fptr, uint16_t *spr, int m, Track *cv, Row r);
char pc(jack_nframes_t fptr, uint16_t *spr, int m, Track *cv, Row r);
char Dc(jack_nframes_t fptr, uint16_t *spr, int m, Track *cv, Row r);
char Gc(jack_nframes_t fptr, uint16_t *spr, int m, Track *cv, Row r);
char gc(jack_nframes_t fptr, uint16_t *spr, int m, Track *cv, Row r);
char altGc(jack_nframes_t fptr, uint16_t *spr, int m, Track *cv, Row r);
char altgc(jack_nframes_t fptr, uint16_t *spr, int m, Track *cv, Row r);
char Rc(jack_nframes_t fptr, uint16_t *spr, int m, Track *cv, Row r);
char rc(jack_nframes_t fptr, uint16_t *spr, int m, Track *cv, Row r);
char altRc(jack_nframes_t fptr, uint16_t *spr, int m, Track *cv, Row r);
char altrc(jack_nframes_t fptr, uint16_t *spr, int m, Track *cv, Row r);
char Sc(jack_nframes_t fptr, uint16_t *spr, int m, Track *cv, Row r);
char sc(jack_nframes_t fptr, uint16_t *spr, int m, Track *cv, Row r);
char altSc(jack_nframes_t fptr, uint16_t *spr, int m, Track *cv, Row r);
char altsc(jack_nframes_t fptr, uint16_t *spr, int m, Track *cv, Row r);
char percentc(jack_nframes_t fptr, uint16_t *spr, int m, Track *cv, Row r);
char midicctargetc(jack_nframes_t fptr, uint16_t *spr, int m, Track *cv, Row r);
char midipcc(jack_nframes_t fptr, uint16_t *spr, int m, Track *cv, Row r);
char midiccc(jack_nframes_t fptr, uint16_t *spr, int m, Track *cv, Row r);
char Oc(jack_nframes_t fptr, uint16_t *spr, int m, Track *cv, Row r);
char oc(jack_nframes_t fptr, uint16_t *spr, int m, Track *cv, Row r);
char altOc(jack_nframes_t fptr, uint16_t *spr, int m, Track *cv, Row r);
char altoc(jack_nframes_t fptr, uint16_t *spr, int m, Track *cv, Row r);
char Fc(jack_nframes_t fptr, uint16_t *spr, int m, Track *cv, Row r);
char fc(jack_nframes_t fptr, uint16_t *spr, int m, Track *cv, Row r);
char altFc(jack_nframes_t fptr, uint16_t *spr, int m, Track *cv, Row r);
char altfc(jack_nframes_t fptr, uint16_t *spr, int m, Track *cv, Row r);
char Zc(jack_nframes_t fptr, uint16_t *spr, int m, Track *cv, Row r);
char zc(jack_nframes_t fptr, uint16_t *spr, int m, Track *cv, Row r);
char altZc(jack_nframes_t fptr, uint16_t *spr, int m, Track *cv, Row r);
char altzc(jack_nframes_t fptr, uint16_t *spr, int m, Track *cv, Row r);
char Mc(jack_nframes_t fptr, uint16_t *spr, int m, Track *cv, Row r);
char mc(jack_nframes_t fptr, uint16_t *spr, int m, Track *cv, Row r);
char Ec(jack_nframes_t fptr, uint16_t *spr, int m, Track *cv, Row r);
char ec(jack_nframes_t fptr, uint16_t *spr, int m, Track *cv, Row r);
char Hc(jack_nframes_t fptr, uint16_t *spr, int m, Track *cv, Row r);
char hc(jack_nframes_t fptr, uint16_t *spr, int m, Track *cv, Row r);
char Wc(jack_nframes_t fptr, uint16_t *spr, int m, Track *cv, Row r);
char wc(jack_nframes_t fptr, uint16_t *spr, int m, Track *cv, Row r);
char Lc(jack_nframes_t fptr, uint16_t *spr, int m, Track *cv, Row r);
char lc(jack_nframes_t fptr, uint16_t *spr, int m, Track *cv, Row r);
char Xc(jack_nframes_t fptr, uint16_t *spr, int m, Track *cv, Row r);
char xc(jack_nframes_t fptr, uint16_t *spr, int m, Track *cv, Row r);
