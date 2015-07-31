/* paenter.c */
void PA_SEntertainment(Context *cx, Subgoal *sg, Ts *ts, Obj *a, Obj *o);
void PA_AttendPerformance(Context *cx, Subgoal *sg, Ts *ts, Obj *a, Obj *o);
Obj *TVSetFreqToSSP(Ts *ts, Obj *freq);
Obj *TVShowSelect(Ts *ts, Obj *viewer1, Obj *viewer2, Obj *viewer3, Obj *class, Obj *tvset, Ts *tsend, Obj **ssp);
void PA_TVSetOn(Context *cx, Subgoal *sg, Ts *ts, Obj *a, Obj *o);
void PA_TVSetOff(Context *cx, Subgoal *sg, Ts *ts, Obj *a, Obj *o);
void PA_WatchTV(Context *cx, Subgoal *sg, Ts *ts, Obj *a, Obj *o);
Bool PAObj_HasAntenna(Ts *ts, Obj *obj);
Bool PAObj_IsPluggedIn(Ts *ts, Obj *obj);
void PAObj_TVSet(TsRange *tsrange, Obj *obj);
