void changeMacro(int input, char *dest);
void descMacro(char c, uint8_t v);

char DUMMY(jack_nframes_t fptr, int m, channel *cv, row r) { return 1; }
char ocPRERAMP(jack_nframes_t fptr, int m, channel *cv, row r) { if (m) return 1; return 0; }

char Vc(jack_nframes_t fptr, int m, channel *cv, row r);
char Bc(jack_nframes_t fptr, int m, channel *cv, row r);
char Cc(jack_nframes_t fptr, int m, channel *cv, row r);
char Pc(jack_nframes_t fptr, int m, channel *cv, row r);
char Dc(jack_nframes_t fptr, int m, channel *cv, row r);
char Gc(jack_nframes_t fptr, int m, channel *cv, row r);
char gc(jack_nframes_t fptr, int m, channel *cv, row r);
char Ic(jack_nframes_t fptr, int m, channel *cv, row r);
char ic(jack_nframes_t fptr, int m, channel *cv, row r);
char Qc(jack_nframes_t fptr, int m, channel *cv, row r);
char qc(jack_nframes_t fptr, int m, channel *cv, row r);
char Rc(jack_nframes_t fptr, int m, channel *cv, row r);
char rc(jack_nframes_t fptr, int m, channel *cv, row r);
char Wc(jack_nframes_t fptr, int m, channel *cv, row r);
char wc(jack_nframes_t fptr, int m, channel *cv, row r);
char Mc(jack_nframes_t fptr, int m, channel *cv, row r);
char Sc(jack_nframes_t fptr, int m, channel *cv, row r);
char sc(jack_nframes_t fptr, int m, channel *cv, row r);
char percentc(jack_nframes_t fptr, int m, channel *cv, row r);
char midicctargetc(jack_nframes_t fptr, int m, channel *cv, row r);
char midipcc(jack_nframes_t fptr, int m, channel *cv, row r);
char midiccc(jack_nframes_t fptr, int m, channel *cv, row r);
char smoothmidiccc(jack_nframes_t fptr, int m, channel *cv, row r);
char Oc(jack_nframes_t fptr, int m, channel *cv, row r);
char oc(jack_nframes_t fptr, int m, channel *cv, row r);
char Uc(jack_nframes_t fptr, int m, channel *cv, row r);
char uc(jack_nframes_t fptr, int m, channel *cv, row r);
char Fc(jack_nframes_t fptr, int m, channel *cv, row r);
char fc(jack_nframes_t fptr, int m, channel *cv, row r);
char Zc(jack_nframes_t fptr, int m, channel *cv, row r);
char zc(jack_nframes_t fptr, int m, channel *cv, row r);
char Ec(jack_nframes_t fptr, int m, channel *cv, row r);
char Hc(jack_nframes_t fptr, int m, channel *cv, row r);
char Lc(jack_nframes_t fptr, int m, channel *cv, row r);
char lc(jack_nframes_t fptr, int m, channel *cv, row r);