/* pagrasp.c */
ObjList *PA_Graspers(Ts *ts, Obj *obj);
Obj *PA_GetFreeHand(Ts *ts, Obj *human);
void PA_MoveObject(Ts *ts, Obj *obj, Obj *grid, GridCoord torow, GridCoord tocol);
void PA_SmallContainedObjectMove(Ts *ts, Obj *small_contained, Obj *grid, GridCoord torow, GridCoord tocol);
void PA_HeldObjectMove(Ts *ts, Obj *held, Obj *grid, GridCoord torow, GridCoord tocol);
void PA_GrasperMove(Ts *ts, Obj *grasper, Obj *grid, GridCoord torow, GridCoord tocol);
void PA_ActorMove(Ts *ts, Obj *a, Obj *grid, GridCoord torow, GridCoord tocol, int is_walk);
void PA_LargeContainerMove(Ts *ts, Obj *large_container, Obj *grid, GridCoord torow, GridCoord tocol);
void PA_Grasp(Context *cx, Subgoal *sg, Ts *ts, Obj *a, Obj *o);
void PA_Release(Context *cx, Subgoal *sg, Ts *ts, Obj *a, Obj *o);
void PA_Inside(Context *cx, Subgoal *sg, Ts *ts, Obj *a, Obj *o);
void PA_MoveTo(Context *cx, Subgoal *sg, Ts *ts, Obj *a, Obj *o);
void PA_NearGraspable(Context *cx, Subgoal *sg, Ts *ts, Obj *a, Obj *o);
void PA_Holding(Context *cx, Subgoal *sg, Ts *ts, Obj *a, Obj *o);
void PA_Open(Context *cx, Subgoal *sg, Ts *ts, Obj *a, Obj *o);
void PA_Closed(Context *cx, Subgoal *sg, Ts *ts, Obj *a, Obj *o);
void PA_ActionOpen(Context *cx, Subgoal *sg, Ts *ts, Obj *a, Obj *o);
void PA_ActionClose(Context *cx, Subgoal *sg, Ts *ts, Obj *a, Obj *o);
void PA_ConnectedTo(Context *cx, Subgoal *sg, Ts *ts, Obj *a, Obj *o);
void PA_ConnectTo(Context *cx, Subgoal *sg, Ts *ts, Obj *a, Obj *o);
void PA_Rub(Context *cx, Subgoal *sg, Ts *ts, Obj *a, Obj *o);
void PA_PourOnto(Context *cx, Subgoal *sg, Ts *ts, Obj *a, Obj *o);
void PA_KnobPosition(Context *cx, Subgoal *sg, Ts *ts, Obj *a, Obj *o);
void PA_SwitchX(Context *cx, Subgoal *sg, Ts *ts, Obj *a, Obj *o);
void PA_FlipTo(Context *cx, Subgoal *sg, Ts *ts, Obj *a, Obj *o);
void PA_GestureHere(Context *cx, Subgoal *sg, Ts *ts, Obj *a, Obj *o);
void PA_HandTo(Context *cx, Subgoal *sg, Ts *ts, Obj *a, Obj *o);
void PA_ReceiveFrom(Context *cx, Subgoal *sg, Ts *ts, Obj *a, Obj *o);