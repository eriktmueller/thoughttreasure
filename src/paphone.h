/* paphone.c */
Obj *PAObj_DigitsToPhone(Ts *ts, Obj *digits);
Obj *PAObj_PhoneToDigits(Ts *ts, Obj *phone);
Obj *PAObj_LookupPhoneNumber(Ts *ts, Obj *human);
Obj *PAObj_Phonestate(Ts *ts, Obj *phone, Obj **r2);
void PAObj_ChangePhoneState(Ts *ts, Obj *phone, Obj *prevstate, Obj *nextstate, Obj *r2);
Obj *PAObj_Phone2(Ts *ts, Obj *phone, Obj *state, Obj *otherphone, Obj *hookstate, Obj *digits, Obj **otherphone_r);
void PAObj_Phone1(Ts *ts, Obj *phone);
void PAObj_Phone(TsRange *tsrange, Obj *phone);
void PA_Call(Context *cx, Subgoal *sg, Ts *ts, Obj *a, Obj *o);
void PA_HandleCall(Context *cx, Subgoal *sg, Ts *ts, Obj *a, Obj *o);
void PA_OffHook(Context *cx, Subgoal *sg, Ts *ts, Obj *a, Obj *o);
void PA_OnHook(Context *cx, Subgoal *sg, Ts *ts, Obj *a, Obj *o);
void PA_PickUp(Context *cx, Subgoal *sg, Ts *ts, Obj *a, Obj *o);
void PA_HangUp(Context *cx, Subgoal *sg, Ts *ts, Obj *a, Obj *o);
void PA_Dial(Context *cx, Subgoal *sg, Ts *ts, Obj *a, Obj *o);
