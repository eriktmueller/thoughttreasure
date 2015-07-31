/* repgroup.c */
ObjList *GroupMembers(Ts *ts, Obj *group);
Bool GroupConsistsOf(Ts *ts, Obj *group, ObjList *members);
Obj *GroupGet(Ts *ts, ObjList *members);
Obj *GroupCreate(Ts *ts, ObjList *members);
