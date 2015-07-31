/* utildbg.c */
void DbgInit(void);
void DbgLogClear(void);
int DbgStringToLevel(char *s);
int DbgStringToFlags(char *s);
void DbgSet(int flags, int level);
void DbgSetStdoutLevel(int level);
Bool DbgOn(int flag, int level);
void DbgOPP(int flag, int level, Obj *obj);
void DbgOP(int flag, int level, Obj *obj);
void Dbg(int flag, int level, char *fmt, ...);
int inter(void);
void UnwindProtect(void);
void Panic(char *fmt, ...);
void Exit(int code);
void Debugger(void);
void Stop(void);
void Nop(void);
