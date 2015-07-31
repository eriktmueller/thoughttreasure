/* uapaslp.c */
void PA_Sleep_Times(Context *cx, Subgoal *sg, Ts *ts, Obj *a, Tod *go_to_sleep, Tod *awaken);
Float PA_Sleep_Recharge(Obj *level, Float elapsed, Float sleeptime);
Float PA_Sleep_Uncharge(Obj *level, Float elapsed, Float sleeptime, Float circadian);
Float PA_Sleep_Levels(Context *cx, Subgoal *sg, Ts *ts, Obj *a, int ts_skip, int is_asleep);
void PA_Sleep(Context *cx, Subgoal *sg, Ts *ts, Obj *a, Obj *o);
void UA_Sleep(Actor *ac, Ts *ts, Obj *a, Obj *in);
