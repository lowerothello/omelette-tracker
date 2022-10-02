void changeMacro(int input, char *dest);
void descMacro(char c, uint8_t v);

char DUMMY(jack_nframes_t fptr, int m, Channel *cv, Row r) { return 1; }
char ocPRERAMP(jack_nframes_t fptr, int m, Channel *cv, Row r) { if (m) return 1; return 0; }

char Vc(jack_nframes_t fptr, int m, Channel *cv, Row r);
char Bc(jack_nframes_t fptr, int m, Channel *cv, Row r);
char Cc(jack_nframes_t fptr, int m, Channel *cv, Row r);
char Pc(jack_nframes_t fptr, int m, Channel *cv, Row r);
char Dc(jack_nframes_t fptr, int m, Channel *cv, Row r);
char Gc(jack_nframes_t fptr, int m, Channel *cv, Row r);
char gc(jack_nframes_t fptr, int m, Channel *cv, Row r);
char Ic(jack_nframes_t fptr, int m, Channel *cv, Row r);
char ic(jack_nframes_t fptr, int m, Channel *cv, Row r);
char Qc(jack_nframes_t fptr, int m, Channel *cv, Row r);
char qc(jack_nframes_t fptr, int m, Channel *cv, Row r);
char Rc(jack_nframes_t fptr, int m, Channel *cv, Row r);
char rc(jack_nframes_t fptr, int m, Channel *cv, Row r);
char Wc(jack_nframes_t fptr, int m, Channel *cv, Row r);
char wc(jack_nframes_t fptr, int m, Channel *cv, Row r);
char Mc(jack_nframes_t fptr, int m, Channel *cv, Row r);
char Sc(jack_nframes_t fptr, int m, Channel *cv, Row r);
char sc(jack_nframes_t fptr, int m, Channel *cv, Row r);
char percentc(jack_nframes_t fptr, int m, Channel *cv, Row r);
char midicctargetc(jack_nframes_t fptr, int m, Channel *cv, Row r);
char midipcc(jack_nframes_t fptr, int m, Channel *cv, Row r);
char midiccc(jack_nframes_t fptr, int m, Channel *cv, Row r);
char Oc(jack_nframes_t fptr, int m, Channel *cv, Row r);
char oc(jack_nframes_t fptr, int m, Channel *cv, Row r);
char Uc(jack_nframes_t fptr, int m, Channel *cv, Row r);
char uc(jack_nframes_t fptr, int m, Channel *cv, Row r);
char Fc(jack_nframes_t fptr, int m, Channel *cv, Row r);
char fc(jack_nframes_t fptr, int m, Channel *cv, Row r);
char Zc(jack_nframes_t fptr, int m, Channel *cv, Row r);
char zc(jack_nframes_t fptr, int m, Channel *cv, Row r);
char Ec(jack_nframes_t fptr, int m, Channel *cv, Row r);
char Hc(jack_nframes_t fptr, int m, Channel *cv, Row r);
char Lc(jack_nframes_t fptr, int m, Channel *cv, Row r);
char lc(jack_nframes_t fptr, int m, Channel *cv, Row r);
