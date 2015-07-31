/* uaspace.c */
void UA_SpaceActorMove(Context *cx, Ts *ts, Obj *actor, Obj *to_grid, GridCoord to_row, GridCoord to_col);
void UA_Space_ActorTransportNear(Context *cx, Ts *ts, Obj *actor, Obj *to_grid, Obj *nearx);
void UA_Space(Context *cx, Ts *ts, Obj *in);
