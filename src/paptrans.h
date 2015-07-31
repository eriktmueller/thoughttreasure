/* paptrans.c */
void PA_NearReachable(Context *cx, Subgoal *sg, Ts *ts, Obj *a, Obj *o);
void PA_GridWalk(Context *cx, Subgoal *sg, Ts *ts, Obj *a, Obj *o);
void PA_Sitting(Context *cx, Subgoal *sg, Ts *ts, Obj *a, Obj *o);
void PA_Standing(Context *cx, Subgoal *sg, Ts *ts, Obj *a, Obj *o);
void PA_Lying(Context *cx, Subgoal *sg, Ts *ts, Obj *a, Obj *o);
void PA_SitOn(Context *cx, Subgoal *sg, Ts *ts, Obj *a, Obj *o);
void PA_StandOn(Context *cx, Subgoal *sg, Ts *ts, Obj *a, Obj *o);
void PA_LieOn(Context *cx, Subgoal *sg, Ts *ts, Obj *a, Obj *o);
void PA_Warp(Context *cx, Subgoal *sg, Ts *ts, Obj *a, Obj *o);
void PA_Drive(Context *cx, Subgoal *sg, Ts *ts, Obj *a, Obj *o);
void PA_GridDriveCar(Context *cx, Subgoal *sg, Ts *ts, Obj *a, Obj *o);
void PA_MotorVehicleOn(Context *cx, Subgoal *sg, Ts *ts, Obj *a, Obj *o);
void PA_MotorVehicleOff(Context *cx, Subgoal *sg, Ts *ts, Obj *a, Obj *o);
void PAObj_MotorVehicle(TsRange *tsrange, Obj *obj);
void PA_NearAudible(Context *cx, Subgoal *sg, Ts *ts, Obj *a, Obj *o);
void PA_Stay(Context *cx, Subgoal *sg, Ts *ts, Obj *a, Obj *o);
void PA_Pack(Context *cx, Subgoal *sg, Ts *ts, Obj *a, Obj *o);
void PA_Unpack(Context *cx, Subgoal *sg, Ts *ts, Obj *a, Obj *o);
