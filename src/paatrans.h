/* paatrans.c */
void AmountParse(Obj *amount, Float *notional, Obj **currency);
ObjList *CashPickOutAmount(Ts *ts, Obj *container, Float *change, Obj *amount);
ObjList *TicketFindNextAvailable(Ts *ts, Obj *container, ObjList *exclude, Obj *performance);
Bool PersonalCheckCalendar(Ts *ts, Obj *human, TsRange *candidate);
Bool PersonalCheckBudget(Ts *ts, Obj *human, Obj *amount);
void PA_BuyTicket(Context *cx, Subgoal *sg, Ts *ts, Obj *a, Obj *o);
void PA_PayInPerson(Context *cx, Subgoal *sg, Ts *ts, Obj *a, Obj *o);
void PA_PayCash(Context *cx, Subgoal *sg, Ts *ts, Obj *a, Obj *o);
void PA_PayByCard(Context *cx, Subgoal *sg, Ts *ts, Obj *a, Obj *o);
void PA_PayByCheck(Context *cx, Subgoal *sg, Ts *ts, Obj *a, Obj *o);
void PA_WorkBoxOffice(Context *cx, Subgoal *sg, Ts *ts, Obj *a, Obj *o);
void PA_CollectPayment(Context *cx, Subgoal *sg, Ts *ts, Obj *a, Obj *o);
void PA_CollectCash(Context *cx, Subgoal *sg, Ts *ts, Obj *a, Obj *o);
