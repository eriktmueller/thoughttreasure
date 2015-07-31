/* uagoal.c */
void UA_GoalSetStatus(Actor *ac, Subgoal *sg, Obj *new_status, Obj *spin_emotion);
void UA_GoalSubgoal(Actor *ac, Ts *ts, Obj *a, Obj *in, Obj *goal, Obj *subgoal);
void UA_Goal(Actor *ac, Ts *ts, Obj *a, Obj *in);
Bool UA_GoalDoesHandle(Obj *rel);
