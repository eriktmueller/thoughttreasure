/* repsubgl.c */
Subgoal *SubgoalCreate(Actor *ac, Obj *actor, Ts *ts, Subgoal *supergoal, int onsuccess, int onfailure, Obj *subgoal, Subgoal *next);
Subgoal *SubgoalCopy(Subgoal *sg_parent, Actor *ac_child, Context *cx_parent, Context *cx_child, Subgoal *next);
Subgoal *SubgoalMapSubgoal(Subgoal *sg_parents, Subgoal *sg_parent);
Subgoal *SubgoalCopyAll(Subgoal *sg_parents, Actor *ac_child, Context *cx_parent, Context *cx_child);
Obj *SubgoalStatus(Subgoal *sg);
void SubgoalStatePrint(FILE *stream, SubgoalState state);
void SubgoalPrint(FILE *stream, Subgoal *sg, int detail);
void SubgoalStatusChange(Subgoal *sg, Obj *goal_status, ObjList *causes);
Subgoal *SubgoalFind(Actor *ac, Obj *class);
