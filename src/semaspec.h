/* semaspec.c */
Bool AspectISA(Obj *class, Obj *obj);
Obj *AspectDetermine1(TsRange *focus_tsr, Obj *obj, int situational);
Obj *AspectDetermine(TsRange *focus_tsr, Obj *obj, int situational);
Obj *AspectToTense1(Obj *aspect, int tensestep, int is_literary, int lang);
Obj *AspectToTense(Obj *aspect, int tensestep, int is_literary, int lang);
Obj *AspectDetermineSingleton(Obj *obj);
void AspectDetermineRelatedPair(Obj *rel, Obj *obj1, Obj *obj2, Obj **aspect1, Obj **aspect2);
Bool AspectIsCompatibleTense(Obj *aspect, Obj *tense, int lang);
Bool AspectIsAccomplished(Discourse *dc);
void AspectTempRelQueryTsr(Obj *rel, int qi, TsRange *tsr_in, TsRange *tsr_out);
Obj *AspectTempRelGen(TsRange *tsr1, TsRange *tsr2, Ts *now);
